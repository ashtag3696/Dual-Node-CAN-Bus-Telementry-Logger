#include "i2c_lcd.h"

// Simple blocking delay (calibrate for your system clock, assumes 16MHz default HSI)
static void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 4000; i++); 
}

// Bare-metal I2C Transmit
static void I2C1_Write(uint8_t dev_addr, uint8_t data) {
    while (I2C1->SR2 & I2C_SR2_BUSY);       // Wait until bus is free
    I2C1->CR1 |= I2C_CR1_START;             // Generate START
    while (!(I2C1->SR1 & I2C_SR1_SB));      // Wait for START sent

    I2C1->DR = dev_addr;                    // Send device address (Write)
    while (!(I2C1->SR1 & I2C_SR1_ADDR));    // Wait for address match
    (void)I2C1->SR2;                        // Clear ADDR flag by reading SR2

    I2C1->DR = data;                        // Send data
    while (!(I2C1->SR1 & I2C_SR1_TXE));     // Wait for Tx buffer empty

    I2C1->CR1 |= I2C_CR1_STOP;              // Generate STOP
}

void I2C1_Init(void) {
    // 1. Enable Clocks for I2C1 and GPIOB
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // 2. Configure PB8 (SCL) and PB9 (SDA) for Alternate Function
    GPIOB->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9);
    GPIOB->MODER |= (GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1); // AF mode
    GPIOB->OTYPER |= (GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9);        // Open-drain
    GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR8_1 | GPIO_OSPEEDER_OSPEEDR9_1); // High speed
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPDR8_0 | GPIO_PUPDR_PUPDR9_0); // Pull-ups

    GPIOB->AFR[1] |= (4 << (0 * 4)) | (4 << (1 * 4)); // AF4 for PB8 and PB9

    // 3. Configure I2C1 Peripheral
    I2C1->CR1 = I2C_CR1_SWRST;              // Reset I2C1
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2 = 16;                         // Set APB1 clock frequency (16 MHz default HSI)
    I2C1->CCR = 80;                         // Standard mode, 100kHz (16MHz / (2 * 100kHz))
    I2C1->TRISE = 17;                       // Maximum rise time

    I2C1->CR1 |= I2C_CR1_PE;                // Enable I2C1
}

// Send 4 bits to PCF8574 and pulse the Enable pin
static void LCD_WriteNibble(uint8_t nibble, uint8_t rs) {
    uint8_t data = (nibble & 0xF0) | rs | LCD_BL;
    I2C1_Write(LCD_I2C_ADDR, data);             // Set data and RS
    I2C1_Write(LCD_I2C_ADDR, data | LCD_EN);    // Pulse EN High
    delay_ms(1);                                // Give LCD time to read
    I2C1_Write(LCD_I2C_ADDR, data & ~LCD_EN);   // Pulse EN Low
    delay_ms(1);
}

void LCD_SendCommand(uint8_t cmd) {
    LCD_WriteNibble(cmd & 0xF0, 0);         // Send higher nibble
    LCD_WriteNibble((cmd << 4) & 0xF0, 0);  // Send lower nibble
}

void LCD_SendData(uint8_t data) {
    LCD_WriteNibble(data & 0xF0, LCD_RS);         // Send higher nibble
    LCD_WriteNibble((data << 4) & 0xF0, LCD_RS);  // Send lower nibble
}

void LCD_Init(void) {
    I2C1_Init();
    delay_ms(50); // Wait for LCD to power up

    // HD44780 initialization sequence for 4-bit mode
    LCD_WriteNibble(0x30, 0); delay_ms(5);
    LCD_WriteNibble(0x30, 0); delay_ms(1);
    LCD_WriteNibble(0x30, 0); delay_ms(1);
    LCD_WriteNibble(0x20, 0); delay_ms(1); // Enable 4-bit mode

    LCD_SendCommand(0x28); // Function set: 4-bit, 2 lines, 5x8 font
    LCD_SendCommand(0x0C); // Display ON, Cursor OFF, Blink OFF
    LCD_SendCommand(0x06); // Entry mode: Increment cursor
    LCD_Clear();
}

void LCD_Clear(void) {
    LCD_SendCommand(0x01);
    delay_ms(2); // Clear display requires more time
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? 0x80 : 0xC0;
    LCD_SendCommand(address + col);
}

void LCD_Print(const char* str) {
    while (*str) {
        LCD_SendData(*str++);
    }
}
