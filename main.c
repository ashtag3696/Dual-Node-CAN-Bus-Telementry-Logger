#include <stdint.h>
#include <stdio.h>

#include "stm32f401_hw.h"
#include "mcp2515.h"

/*
 * ========================= Build-time node selection =========================
 * Set NODE_ROLE to one of:
 *   - NODE_ROLE_A : LCD + CAN receiver (displays temperature from Node B)
 *   - NODE_ROLE_B : DS18B20 + CAN transmitter (sends temperature to Node A)
 */
#define NODE_ROLE_A 1
#define NODE_ROLE_B 2

#ifndef NODE_ROLE
#define NODE_ROLE NODE_ROLE_B
#endif

#if (NODE_ROLE != NODE_ROLE_A) && (NODE_ROLE != NODE_ROLE_B)
#error "Invalid NODE_ROLE. Use NODE_ROLE_A or NODE_ROLE_B."
#endif

/* CAN standard IDs */
#define CAN_ID_TEMP_FRAME   0x122u

/* Timing */
#define TEMP_REQUEST_PERIOD_MS      1000u
#define DS18B20_CONVERSION_WAIT_MS   750u
#define RX_TIMEOUT_MS               5000u

#if NODE_ROLE == NODE_ROLE_A
#include "i2c_lcd.h"
#endif

#if NODE_ROLE == NODE_ROLE_B
#include "ds18b20.h"
#endif

static void pulse_led(uint32_t ms) {
    led_set(1u);
    delay_ms(ms);
    led_set(0u);
}

/* Encode temperature as signed centi-degrees Celsius to avoid float-over-CAN issues */
static int16_t temp_to_centi(float t) {
    if (t >= 0.0f) {
        return (int16_t)(t * 100.0f + 0.5f);
    }
    return (int16_t)(t * 100.0f - 0.5f);
}

static float centi_to_temp(int16_t c) {
    return ((float)c) / 100.0f;
}

#if NODE_ROLE == NODE_ROLE_A
static void lcd_show_temperature(float temp) {
    char line[17];

    int int_part = (int)temp;
    int frac_part = (int)((temp - (float)int_part) * 100.0f);
    if (frac_part < 0) {
        frac_part = -frac_part;
    }

    /* 16-char LCD: keep it compact */
    snprintf(line, sizeof(line), "TEMP:%3d.%02d C", int_part, frac_part);

    LCD_SetCursor(0, 0);
    LCD_Print(line);
}

static void lcd_show_status(const char *status) {
    LCD_SetCursor(1, 0);
    LCD_Print("STAT:");
    LCD_Print(status);
}
#endif

int main(void) {
    clock_init_minimal();
    systick_init_1ms();

    gpio_init_led_button();
    spi1_init_for_mcp2515();

#if NODE_ROLE == NODE_ROLE_A
    LCD_Init();
    LCD_SetCursor(0, 0);
    LCD_Print("TEMP: Wait...");
    lcd_show_status("INIT");
#endif

#if NODE_ROLE == NODE_ROLE_B
    ds18b20_init();
#endif

    if (mcp2515_init_500kbps() != MCP_RET_OK) {
#if NODE_ROLE == NODE_ROLE_A
        lcd_show_status("CAN INIT ERR");
#endif
        while (1) {
            pulse_led(100u);
            delay_ms(200u);
        }
    }

    if (mcp2515_set_normal_mode() != MCP_RET_OK) {
#if NODE_ROLE == NODE_ROLE_A
        lcd_show_status("CAN MODE ERR");
#endif
        while (1) {
            pulse_led(300u);
            delay_ms(300u);
        }
    }

#if NODE_ROLE == NODE_ROLE_A
    {
        uint32_t last_rx_ms = millis();
        lcd_show_status("WAIT NODE B");

        while (1) {
            uint16_t rx_id = 0u;
            uint8_t rx_len = 0u;
            uint8_t rx_data[8];
            uint32_t now = millis();

            if (mcp2515_receive_std(&rx_id, rx_data, &rx_len) == MCP_RET_OK) {
                if ((rx_id == CAN_ID_TEMP_FRAME) && (rx_len >= 2u)) {
                    int16_t t_centi = (int16_t)(((uint16_t)rx_data[0] << 8) | rx_data[1]);
                    float temp = centi_to_temp(t_centi);

                    lcd_show_temperature(temp);
                    lcd_show_status("RX OK      ");
                    last_rx_ms = now;
                    pulse_led(20u);
                }
            }

            if ((now - last_rx_ms) > RX_TIMEOUT_MS) {
                lcd_show_status("NO DATA    ");
            }
        }
    }
#endif

#if NODE_ROLE == NODE_ROLE_B
    {
        uint32_t last_temp_request_ms = 0u;
        uint8_t conversion_pending = 0u;

        while (1) {
            uint32_t now = millis();

            /* 1) Start conversion every TEMP_REQUEST_PERIOD_MS */
            if ((!conversion_pending) && ((now - last_temp_request_ms) >= TEMP_REQUEST_PERIOD_MS)) {
                if (ds18b20_request_conversion() == 1u) {
                    conversion_pending = 1u;
                }
                last_temp_request_ms = now;
            }

            /* 2) Read when conversion is done, then send over CAN */
            if (conversion_pending && ((now - last_temp_request_ms) >= DS18B20_CONVERSION_WAIT_MS)) {
                if (ds18b20_is_conversion_done() == 1u) {
                    float temp = ds18b20_read_temperature();
                    int16_t t_centi = temp_to_centi(temp);

                    uint8_t tx_data[2];
                    tx_data[0] = (uint8_t)((uint16_t)t_centi >> 8);
                    tx_data[1] = (uint8_t)((uint16_t)t_centi & 0xFFu);

                    if (mcp2515_send_std(CAN_ID_TEMP_FRAME, tx_data, 2u) == MCP_RET_OK) {
                        pulse_led(10u);
                    }

                    conversion_pending = 0u;
                }
            }
        }
    }
#endif
}
