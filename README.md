# Dual Node CAN-BUS telemetry Logger using STM32F401CCU6 – Keil µVision Setup Instructions

### Instructions:

1. Create a new project in Keil µVision for the target device:
   STM32F401CCUx

2. Download the required `.c` source files and `.h` header files from this repository.

3. Create a folder for the new Keil project and add the downloaded `.c` and `.h` files to the project folder.

4. Open the project in Keil µVision and add the required `.c` source files to the project.

5. Make sure the STM32F401CCUx device and required STM32F4xx Device Family Pack are selected.

6. Build the project and verify that there are no compilation or linker errors.

7. Connect the ST-LINK programmer/debugger to the STM32F401CCU6.

8. Flash the project onto the STM32F401CCU6 using Keil µVision.

9. After successful flashing, reset the STM32F401CCU6 and verify that the program is running correctly.
