# V-TRAC

### CAN-Based Distributed Temperature Telemetry System

**1. Problem Statement & Motivation** Distributed embedded systems in automotive and industrial environments require noise-immune, deterministic communication between sensor nodes and display units. Standard single-ended protocols (UART, I2C) degrade over long cable runs and lack hardware-level error detection. This project implements a two-node Controller Area Network (CAN) telemetry link on bare-metal STM32F401CCU6 microcontrollers, decoupling remote temperature acquisition (Node B) from real-time dashboard visualization and link-health monitoring (Node A).   

**2. System Architecture & Hardware Design** Because the STM32F401CCU6 lacks an integrated CAN controller, each node interfaces with an external **MCP2515 CAN controller** via **SPI1** (`PA4` CS, `PA5` SCK, `PA6` MISO, `PA7` MOSI) and a **TJA1050 transceiver** driving a differential twisted-pair bus (`CAN_H`, `CAN_L`) terminated with $120\,\Omega$ resistors:   

- **Node A (Dashboard Display Unit):** Drives a 16x2 HD44780 LCD over **I2C1** (`PB8` SCL, `PB9` SDA) via a PCF8574 I/O expander (`0x4E` 8-bit address) to display live temperature readings and timeout status (`RX OK` vs. `NO DATA` after 5 seconds).   
- **Node B (Remote Sensor Unit):** Interfaces with a **DS18B20 digital thermometer** on `PA1` configured in open-drain output mode with an external $4.7\,\text{k}\Omega$ pull-up resistor, using the ARM Cortex-M4 DWT cycle counter for microsecond-accurate 1-Wire signaling.   

**3. Bare-Metal Firmware & Communication Protocol** Written in register-level C using CMSIS headers (zero HAL overhead) at the 16 MHz HSI system clock, the firmware configures the MCP2515 (8 MHz crystal) for **500 kbps** (`CNF1=0x00`, `CNF2=0x90`, `CNF3=0x82`) with non-blocking transmit verification (`TXB0CTRL` polling) and a 1 ms SysTick timebase:   

- **Temperature Telemetry Frame (`ID: 0x122`, `DLC: 2`, Periodic @ 1000 ms):** Node B triggers a DS18B20 conversion (`0x44`), waits 750 ms for completion, reads the scratchpad (`0xBE`), and encodes the temperature as a big-endian signed 16-bit integer in centi-degrees Celsius (`int16_t`, e.g., `2545` = `25.45 °C`) to avoid floating-point serialization overhead across the bus.   

**4. Validation & Results** System functionality was verified through SPI register readback (`CANSTAT` mode verification), internal MCP2515 loopback testing (`0x40`), and live two-node bus operation. Node B reliably acquires and transmits 12-bit temperature samples at 1 Hz, while Node A decodes incoming frames, updates the I2C LCD with 0.01 °C resolution, pulses the `PC13` status LED on packet arrival, and detects bus disconnections within the 5-second timeout window.  
