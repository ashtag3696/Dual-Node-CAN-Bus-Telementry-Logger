#include "ds18b20.h"

// Assume default 16 MHz HSI clock on startup
#define SYSTEM_CORE_CLOCK 16000000

// Microsecond delay using DWT cycle counter
static void delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = (SYSTEM_CORE_CLOCK / 1000000) * us;
    while ((DWT->CYCCNT - start) < cycles);
}

void ds18b20_init(void) {
    // 1. Enable DWT cycle counter for precise microsecond delays
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // 2. Enable Clock for GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // 3. Configure PA1 as Output Open-Drain
    GPIOA->MODER &= ~GPIO_MODER_MODE1_1;
    GPIOA->MODER |= GPIO_MODER_MODE1_0;     // General purpose output mode (01)
    
    GPIOA->OTYPER |= GPIO_OTYPER_OT1;       // Output open-drain
    GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED1; // High speed
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD1;      // No internal pull-up (use external 4.7k)

    // Set pin high initially (release the 1-Wire bus)
    GPIOA->ODR |= (1 << 1); 
}

// 1-Wire Protocol: Reset Pulse
static uint8_t ds18b20_reset(void) {
    uint8_t presence = 0;
    
    GPIOA->ODR &= ~(1 << 1); // Pull line low
    delay_us(480);           // Wait 480 us
    GPIOA->ODR |= (1 << 1);  // Release line
    delay_us(70);            // Wait for sensor to pull low
    
    // Read presence pulse (0 means sensor is present)
    presence = (GPIOA->IDR & (1 << 1)) ? 1 : 0; 
    
    delay_us(410);           // Wait for slot to finish
    return presence;
}

// 1-Wire Protocol: Write a single bit
static void ds18b20_write_bit(uint8_t bit) {
    GPIOA->ODR &= ~(1 << 1); // Pull line low
    delay_us(2);             // Wait 2 us
    
    if (bit) {
        GPIOA->ODR |= (1 << 1); // If bit is 1, release line early
    }
    
    delay_us(60);            // Wait out the rest of the time slot
    GPIOA->ODR |= (1 << 1);  // Release line for recovery
    delay_us(2);
}

// 1-Wire Protocol: Read a single bit
static uint8_t ds18b20_read_bit(void) {
    uint8_t bit = 0;
    
    GPIOA->ODR &= ~(1 << 1); // Pull line low
    delay_us(2);             // Wait 2 us
    GPIOA->ODR |= (1 << 1);  // Release line
    delay_us(12);            // Wait 12 us before sampling
    
    bit = (GPIOA->IDR & (1 << 1)) ? 1 : 0; // Read the pin
    
    delay_us(50);            // Wait out the rest of the time slot
    return bit;
}

// Write a full byte
static void ds18b20_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(data & 0x01);
        data >>= 1;
    }
}
// Read a full byte
static uint8_t ds18b20_read_byte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (ds18b20_read_bit()) {
            data |= (1 << i);
        }
    }
    return data;
}
// 1. Request conversion
uint8_t ds18b20_request_conversion(void) {
    if (ds18b20_reset() != 0) {
        return 0; // Sensor not responding
    }
    ds18b20_write_byte(0xCC); // Skip ROM
    ds18b20_write_byte(0x44); // Convert Temperature
    return 1; // Success
}

// 2. Check if done
uint8_t ds18b20_is_conversion_done(void) {
    return ds18b20_read_bit();
}

// 3. Read the actual temperature
float ds18b20_read_temperature(void) {
    ds18b20_reset();
    ds18b20_write_byte(0xCC); // Skip ROM
    ds18b20_write_byte(0xBE); // Read Scratchpad

    uint8_t lsb = ds18b20_read_byte();
    uint8_t msb = ds18b20_read_byte();

    int16_t temp_raw = (msb << 8) | lsb;
    return (float)temp_raw / 16.0f;
}
