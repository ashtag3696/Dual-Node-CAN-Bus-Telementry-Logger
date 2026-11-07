#include "stm32f401_hw.h"

// Pin assignment (Black Pill STM32F401CCU6)
// LED: PC13 (active-low on most boards)
// Button: PA0 (KEY, pulled-up externally on many variants)
// SPI1 SCK/MISO/MOSI: PA5/PA6/PA7 (AF5)
// MCP2515 CS: PB6

static volatile uint32_t g_ms = 0;

void clock_init_minimal(void) {
    // Use default reset clock tree (HSI) for deterministic bring-up.
    // Keil startup usually configures SystemCoreClock.
    SystemCoreClockUpdate();
}

void systick_init_1ms(void) {
    SysTick->LOAD = (SystemCoreClock / 1000u) - 1u;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void) {
    g_ms++;
}

uint32_t millis(void) {
    return g_ms;
}

void delay_ms(uint32_t ms) {
    uint32_t start = millis();
    while ((millis() - start) < ms) {
        __NOP();
    }
}

void gpio_init_led_button(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN;

    // PC13 output (LED)
    GPIOC->MODER &= ~(0x3u << (13u * 2u));
    GPIOC->MODER |=  (0x1u << (13u * 2u));
    GPIOC->OTYPER &= ~(1u << 13u);
    GPIOC->OSPEEDR |= (0x1u << (13u * 2u));
    GPIOC->PUPDR &= ~(0x3u << (13u * 2u));

    // PA0 input (button)
    GPIOA->MODER &= ~(0x3u << (0u * 2u));
    GPIOA->PUPDR &= ~(0x3u << (0u * 2u));
    GPIOA->PUPDR |=  (0x1u << (0u * 2u)); // pull-up

    // PA4 output (CS)
    GPIOA->MODER &= ~(0x3u << (4u * 2u));
    GPIOA->MODER |=  (0x1u << (4u * 2u));
    GPIOA->OTYPER &= ~(1u << 4u);
    GPIOA->OSPEEDR |= (0x2u << (4u * 2u));
    GPIOA->PUPDR &= ~(0x3u << (4u * 2u));

    mcp2515_cs_high();
    led_set(0u);
}

void spi1_init_for_mcp2515(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // PA5 SCK, PA6 MISO, PA7 MOSI -> AF5
    GPIOA->MODER &= ~((0x3u << (5u * 2u)) |
                      (0x3u << (6u * 2u)) |
                      (0x3u << (7u * 2u)));
    GPIOA->MODER |=  ((0x2u << (5u * 2u)) |
                      (0x2u << (6u * 2u)) |
                      (0x2u << (7u * 2u)));

    GPIOA->AFR[0] &= ~((0xFu << (5u * 4u)) |
                       (0xFu << (6u * 4u)) |
                       (0xFu << (7u * 4u)));
    GPIOA->AFR[0] |=  ((0x5u << (5u * 4u)) |
                       (0x5u << (6u * 4u)) |
                       (0x5u << (7u * 4u)));

    GPIOA->OSPEEDR |= ((0x2u << (5u * 2u)) |
                       (0x2u << (6u * 2u)) |
                       (0x2u << (7u * 2u)));

    // SPI1: master, SW NSS, CPOL=0, CPHA=0, fPCLK/16
    SPI1->CR1 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR;
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR1 |= SPI_CR1_BR_1 | SPI_CR1_BR_0; // /16

    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t button_raw_pressed(void) {
    // Active-low with pull-up
    if ((GPIOA->IDR & (1u << 0u)) == 0u) {
        return 1u;
    }
    return 0u;
}

uint8_t button_pressed_edge(void) {
    static uint8_t prev = 0u;
    uint8_t now = button_raw_pressed();
    uint8_t edge = 0u;

    if ((now != 0u) && (prev == 0u)) {
        edge = 1u;
    }

    prev = now;
    return edge;
}

void led_set(uint8_t on) {
    // PC13 LED is usually active-low
    if (on != 0u) {
        GPIOC->BSRR = (1u << (13u + 16u));
    } else {
        GPIOC->BSRR = (1u << 13u);
    }
}

void led_toggle(void) {
    GPIOC->ODR ^= (1u << 13u);
}

void mcp2515_cs_low(void) {
    GPIOA->BSRR = (1u << (4u + 16u));
}

void mcp2515_cs_high(void) {
    GPIOA->BSRR = (1u << 4u);
}

uint8_t spi1_transfer(uint8_t tx) {
    while ((SPI1->SR & SPI_SR_TXE) == 0u) {
        __NOP();
    }
    *((volatile uint8_t *)&SPI1->DR) = tx;

    while ((SPI1->SR & SPI_SR_RXNE) == 0u) {
        __NOP();
    }
    return *((volatile uint8_t *)&SPI1->DR);
}

