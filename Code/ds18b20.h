#ifndef DS18B20_H
#define DS18B20_H

#include "stm32f4xx.h"
#include <stdint.h>

// Initializes PA1 for 1-Wire communication and enables DWT for microsecond delays
void ds18b20_init(void);

// Triggers a temperature conversion and returns the value in Celsius
// Note: This function will block for up to 750ms while the sensor calculates
uint8_t ds18b20_request_conversion(void);
float ds18b20_read_temperature(void);
uint8_t ds18b20_is_conversion_done(void);
#endif // DS18B20_H
