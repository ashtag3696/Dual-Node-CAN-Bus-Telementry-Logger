#ifndef MCP2515_H
#define MCP2515_H

#include <stdint.h>

#define MCP_RET_OK  (1u)
#define MCP_RET_ERR (0u)

uint8_t mcp2515_init_500kbps(void);
uint8_t mcp2515_set_normal_mode(void);
uint8_t mcp2515_set_loopback_mode(void);

uint8_t mcp2515_send_std(uint16_t std_id, const uint8_t *data, uint8_t len);
uint8_t mcp2515_receive_std(uint16_t *std_id, uint8_t *data, uint8_t *len);

uint8_t mcp2515_read_canstat(void);

#endif
