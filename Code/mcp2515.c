#include "mcp2515.h"
#include "stm32f401_hw.h"

// MCP2515 SPI instructions
#define MCP_RESET           0xC0
#define MCP_READ            0x03
#define MCP_WRITE           0x02
#define MCP_BIT_MODIFY      0x05
#define MCP_READ_STATUS     0xA0
#define MCP_RTS_TXB0        0x81

// Registers
#define REG_CANSTAT         0x0E
#define REG_CANCTRL         0x0F
#define REG_CNF3            0x28
#define REG_CNF2            0x29
#define REG_CNF1            0x2A
#define REG_CANINTE         0x2B
#define REG_CANINTF         0x2C

#define REG_EFLG            0x2D        ////

#define REG_RXB0CTRL        0x60
#define REG_RXB0SIDH        0x61
#define REG_RXB0SIDL        0x62
#define REG_RXB0DLC         0x65
#define REG_RXB0D0          0x66

#define REG_TXB0CTRL        0x30
#define REG_TXB0SIDH        0x31
#define REG_TXB0SIDL        0x32
#define REG_TXB0DLC         0x35
#define REG_TXB0D0          0x36

#define REG_RXM0SIDH        0x20
#define REG_RXM0SIDL        0x21
#define REG_RXM1SIDH        0x24
#define REG_RXM1SIDL        0x25
#define REG_RXF0SIDH        0x00
#define REG_RXF0SIDL        0x01
#define REG_RXF1SIDH        0x04
#define REG_RXF1SIDL        0x05

// CANCTRL/STAT mode bits
#define CANCTRL_REQOP_MASK  0xE0
#define MODE_NORMAL         0x00
#define MODE_LOOPBACK       0x40
#define MODE_CONFIG         0x80

#define CANINTF_RX0IF       (1u << 0)
#define CANINTF_TX0IF       (1u << 2)

#define TXB0CTRL_TXREQ      (1u << 3)


#define TXB0CTRL_ABTF        (1u << 6) ///////////////
#define TXB0CTRL_MLOA        (1u << 5)
#define TXB0CTRL_TXERR       (1u << 4)

#define EFLG_TXBO            (1u << 5)
#define EFLG_TXEP            (1u << 4)
#define EFLG_TXWAR           (1u << 2)
#define EFLG_EWARN           (1u << 0) ////////////////


// Active low for selection MCP slave
static void mcp_select(void) {
    mcp2515_cs_low(); 
}
static void mcp_deselect(void) {
    mcp2515_cs_high();
}


// Read the data from a register
static uint8_t mcp_read_reg(uint8_t addr) {
    mcp_select(); 
    spi1_transfer(MCP_READ); // transfer the READ command
    spi1_transfer(addr); // register addr
    uint8_t v = spi1_transfer(0xFF);
    mcp_deselect();
    return v;
}

static void mcp_write_reg(uint8_t addr, uint8_t value) {
    mcp_select();
    spi1_transfer(MCP_WRITE);
    spi1_transfer(addr);
    spi1_transfer(value);
    mcp_deselect();
}

static void mcp_bit_modify(uint8_t addr, uint8_t mask, uint8_t data) {
    mcp_select();
    spi1_transfer(MCP_BIT_MODIFY);
    spi1_transfer(addr);
    spi1_transfer(mask);
    spi1_transfer(data);
    mcp_deselect();
}

static uint8_t mcp_read_status(void) {
    mcp_select();
    spi1_transfer(MCP_READ_STATUS);
    uint8_t s = spi1_transfer(0xFF);
    mcp_deselect();
    return s;
}

static void mcp_reset(void) {
    mcp_select();
    spi1_transfer(MCP_RESET);
    mcp_deselect();
    delay_ms(5);
}

static uint8_t mcp_set_mode(uint8_t mode) {
    uint32_t i;

    mcp_bit_modify(REG_CANCTRL, CANCTRL_REQOP_MASK, mode);
    for (i = 0; i < 10000u; i++) {
        uint8_t stat = mcp_read_reg(REG_CANSTAT) & CANCTRL_REQOP_MASK;
        if (stat == mode) {
            return MCP_RET_OK;
        }
    }
    return MCP_RET_ERR;
}

static void mcp_program_bit_timing_500kbps(void) {
    // Default here targets a 8 MHz MCP2515 crystal: 500 kbps
    // CNF1=0x00, CNF2=0xF0, CNF3=0x86
    // If your module is 8 MHz, use CNF2=0x90, CNF3=0x82.
    mcp_write_reg(REG_CNF1, 0x00);
    mcp_write_reg(REG_CNF2, 0x90);
    mcp_write_reg(REG_CNF3, 0x82);
}

uint8_t mcp2515_init_500kbps(void) {
    mcp_reset();

    if (!mcp_set_mode(MODE_CONFIG)) {
        return MCP_RET_ERR;
    }

    mcp_program_bit_timing_500kbps();

    // Accept all standard frames on RXB0
    mcp_write_reg(REG_RXM0SIDH, 0x00);
    mcp_write_reg(REG_RXM0SIDL, 0x00);
    mcp_write_reg(REG_RXM1SIDH, 0x00);
    mcp_write_reg(REG_RXM1SIDL, 0x00);
    mcp_write_reg(REG_RXF0SIDH, 0x00);
    mcp_write_reg(REG_RXF0SIDL, 0x00);
    mcp_write_reg(REG_RXF1SIDH, 0x00);
    mcp_write_reg(REG_RXF1SIDL, 0x00);

    // RXM1:RXM0=11 -> receive any message, BUKT=1
    mcp_write_reg(REG_RXB0CTRL, 0x64);

    // Enable RX0 interrupt flag generation inside MCP2515 (optional for polling)
    mcp_write_reg(REG_CANINTE, CANINTF_RX0IF);

    // Clear pending flags
    mcp_write_reg(REG_CANINTF, 0x00);

    return MCP_RET_OK;
}

uint8_t mcp2515_set_normal_mode(void) {
    return mcp_set_mode(MODE_NORMAL);
}

uint8_t mcp2515_set_loopback_mode(void) {
    return mcp_set_mode(MODE_LOOPBACK);
}

//uint8_t mcp2515_send_std(uint16_t std_id, const uint8_t *data, uint8_t len) {
//    uint8_t i;

//    if (len > 8) {
//        return MCP_RET_ERR;
//    }

//    if (mcp_read_reg(REG_TXB0CTRL) & TXB0CTRL_TXREQ) {
//        return MCP_RET_ERR; // non-blocking: TX buffer busy
//    }

//    uint8_t sidh = (uint8_t)(std_id >> 3);
//    uint8_t sidl = (uint8_t)((std_id & 0x07u) << 5);

//    mcp_write_reg(REG_TXB0SIDH, sidh);
//    mcp_write_reg(REG_TXB0SIDL, sidl);
//    mcp_write_reg(REG_TXB0DLC, len & 0x0Fu);

//    for (i = 0u; i < len; i++) {
//        mcp_write_reg((uint8_t)(REG_TXB0D0 + i), data[i]);
//    }

//    // Request to send TXB0
//    mcp_select();
//    spi1_transfer(MCP_RTS_TXB0);
//    mcp_deselect();

//    // Clear TX0IF so software can observe next complete event if desired
//    mcp_bit_modify(REG_CANINTF, CANINTF_TX0IF, 0x00);

//    return MCP_RET_OK;
//}




uint8_t mcp2515_send_std(uint16_t std_id, const uint8_t *data, uint8_t len) {
    uint8_t i;

    if (len > 8) {
        return MCP_RET_ERR;
    }

    uint8_t txctrl = mcp_read_reg(REG_TXB0CTRL);

    // TX buffer still busy
    if (txctrl & TXB0CTRL_TXREQ) {
        return MCP_RET_ERR;
    }

    uint8_t sidh = (uint8_t)(std_id >> 3);
    uint8_t sidl = (uint8_t)((std_id & 0x07u) << 5);

    mcp_write_reg(REG_TXB0SIDH, sidh);
    mcp_write_reg(REG_TXB0SIDL, sidl);
    mcp_write_reg(REG_TXB0DLC, len & 0x0Fu);

    for (i = 0u; i < len; i++) {
        mcp_write_reg((uint8_t)(REG_TXB0D0 + i), data[i]);
    }

    // Clear previous TX0IF
    mcp_bit_modify(REG_CANINTF, CANINTF_TX0IF, 0x00);

    // Request transmission
    mcp_select();
    spi1_transfer(MCP_RTS_TXB0);
    mcp_deselect();

    /*
     * Wait briefly for transmission to complete.
     */
    for (i = 0u; i < 100u; i++) {
        delay_ms(1);

        txctrl = mcp_read_reg(REG_TXB0CTRL);

        // TXREQ cleared = transmission completed
        if ((txctrl & TXB0CTRL_TXREQ) == 0u) {

            // Transmission error / arbitration lost / abort
            if (txctrl & (TXB0CTRL_ABTF |
                          TXB0CTRL_MLOA |
                          TXB0CTRL_TXERR)) {
                return MCP_RET_ERR;
            }

            return MCP_RET_OK;
        }
    }

    // TXREQ still set after timeout
    return MCP_RET_ERR;
}





uint8_t mcp2515_receive_std(uint16_t *std_id, uint8_t *data, uint8_t *len) {
    uint8_t i;

    if ((mcp_read_reg(REG_CANINTF) & CANINTF_RX0IF) == 0) {
        return MCP_RET_ERR;
    }

    uint8_t sidh = mcp_read_reg(REG_RXB0SIDH);
    uint8_t sidl = mcp_read_reg(REG_RXB0SIDL);
    uint8_t dlc  = mcp_read_reg(REG_RXB0DLC) & 0x0Fu;

    if (dlc > 8) {
        dlc = 8;
    }

    *std_id = (uint16_t)((((uint16_t)sidh) << 3) | (sidl >> 5));
    *len = dlc;

    for (i = 0u; i < dlc; i++) {
        data[i] = mcp_read_reg((uint8_t)(REG_RXB0D0 + i));
    }

    // Clear RX0IF
    mcp_bit_modify(REG_CANINTF, CANINTF_RX0IF, 0x00);

    return MCP_RET_OK;
}

uint8_t mcp2515_read_canstat(void) {
    (void)mcp_read_status();
    return mcp_read_reg(REG_CANSTAT);
}

