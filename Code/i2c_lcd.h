#ifndef I2C_LCD_H
#define I2C_LCD_H

#include "stm32f4xx.h"
#include <stdint.h>

// Default I2C address for PCF8574T is 0x27, PCF8574AT is 0x3F.
// STM32 requires the 8-bit shifted address (0x27 << 1 = 0x4E)
#define LCD_I2C_ADDR 0x4E 

// PCF8574 Pin Mapping to HD44780
#define LCD_RS (1 << 0)
#define LCD_RW (1 << 1)
#define LCD_EN (1 << 2)
#define LCD_BL (1 << 3) // Backlight

void I2C1_Init(void);
void LCD_Init(void);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);
void LCD_Print(const char* str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Clear(void);

#endif
