#include <stdint.h>
#include "stm32f401_hw.h"
#include "mcp2515.h"

/*
 * Two-way CAN node.
 * Pressing this node's button sends its own token.
 * A valid frame from the peer produces a short, non-blocking LED pulse.
 */

#define NODE_ID_TX          0x121u
#define NODE_ID_RX          0x122u
#define LOCAL_TOKEN         0xA1u
#define PEER_TOKEN          0xB1u

#define BUTTON_DEBOUNCE_MS  180u
#define LED_PULSE_MS        100u

int main(void)
{
    uint32_t last_button_time = 0u;
    uint32_t led_off_time = 0u;
    uint8_t led_pulse_active = 0u;

    clock_init_minimal();
    systick_init_1ms();
    gpio_init_led_button();
    spi1_init_for_mcp2515();

    if (!mcp2515_init_500kbps()) {
        while (1) {
            led_toggle();
            delay_ms(120u);
        }
    }

    if (!mcp2515_set_normal_mode()) {
        while (1) {
            led_toggle();
            delay_ms(300u);
        }
    }

    led_set(0u);

    while (1) {
        uint32_t now = millis();

        /* Send this node's token when its button is pressed. */
        if (button_pressed_edge() &&
            ((now - last_button_time) > BUTTON_DEBOUNCE_MS)) {
            uint8_t payload[1];

            last_button_time = now;
            payload[0] = LOCAL_TOKEN;

            if (mcp2515_send_std(NODE_ID_TX, payload, 1u)) {
                led_set(1u);
                led_pulse_active = 1u;
                led_off_time = millis() + LED_PULSE_MS;
            }
        }

        /* Accept only the peer's CAN ID and expected token. */
        {
            uint16_t rx_id;
            uint8_t rx_data[8];
            uint8_t rx_len;

            if (mcp2515_receive_std(&rx_id, rx_data, &rx_len)) {
                if ((rx_id == NODE_ID_RX) &&
                    (rx_len >= 1u) &&
                    (rx_data[0] == PEER_TOKEN)) {
                    led_set(1u);
                    led_pulse_active = 1u;
                    led_off_time = millis() + LED_PULSE_MS;
                }
            }
        }

        /* Turn the LED off without blocking CAN reception. */
        if (led_pulse_active &&
            ((int32_t)(millis() - led_off_time) >= 0)) {
            led_set(0u);
            led_pulse_active = 0u;
        }
    }
}
