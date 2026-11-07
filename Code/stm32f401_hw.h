#ifndef STM32F401_HW_H
#define STM32F401_HW_H

#include <stdint.h>
#include "stm32f401xe.h"

void clock_init_minimal(void);
void systick_init_1ms(void);
void delay_ms(uint32_t ms);
uint32_t millis(void);

void gpio_init_led_button(void);
void spi1_init_for_mcp2515(void);

void mcp2515_cs_low(void);
void mcp2515_cs_high(void);

uint8_t spi1_transfer(uint8_t tx);

uint8_t button_pressed_edge(void);
void led_set(uint8_t on);
void led_toggle(void);

#endif

