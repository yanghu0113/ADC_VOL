# CW32F003 ADC Voltage and Temperature Monitor Project

## Overview

This project demonstrates the use of various peripherals on the CW32F003 microcontroller, including ADC, ATIM (for PWM), UART, and an OLED display. It reads an external analog voltage, the internal chip temperature, and displays these values along with PWM status and system clock speed on an attached OLED screen.

## Target MCU

*   CW32F003

## System Clock

*   Configured to run at **48 MHz** using the internal HSI oscillator (`RCC_HSIOSC_DIV1`).
*   Flash latency is set to `FLASH_Latency_2` for stable operation at 48 MHz.

## Features

*   **ADC:**
    *   Reads external analog voltage on pin **PA01**.
    *   Uses software averaging (**8 samples**) for the voltage reading to improve stability.
    *   Reads the internal temperature sensor using the 1.5V internal reference.
    *   ADC clock is configured with a divider of 32 (`ADC_Clk_Div32`) based on the 48 MHz system clock.
*   **PWM:**
    *   Generates PWM output on pin **PA06** using the ATIM peripheral.
    *   PWM frequency and duty cycle are configurable via `INITIAL_PWM_FREQ_HZ` and `INITIAL_PWM_DUTY_PERCENT` in `config.h`.
    *   The driver dynamically calculates the required timer prescaler and reload values based on the system clock.
*   **UART:**
    *   Uses UART1 for debug output (e.g., `printf`).
    *   Configured with baud rate `DEBUG_UART_BAUDRATE` from `config.h`.
    *   Uses non-blocking ring buffers for TX and RX.
*   **OLED Display:**
    *   Displays the following information:
        *   Line 0: System Clock Speed (e.g., "Clk: 48MHz")
        *   Line 1: PWM Frequency and Duty Cycle (e.g., "Freq:1000Hz Duty:50%")
        *   Line 2: External ADC Voltage (e.g., "V: 1.234V")
        *   Line 3: Internal Temperature (e.g., "T: 25.3C")
*   **Drivers:**
    *   Modular drivers are implemented in the `USER/src` and `USER/inc` directories for ADC, PWM, UART, and OLED.
    *   Configuration parameters are centralized in `USER/inc/config.h`.

## Hardware Connections (Assumed)

*   Analog voltage source connected to PA01.
*   PWM output available on PA06.
*   UART1 TX/RX pins connected to a serial adapter (check `config.h` for specific pins, likely PA02/PA03 or PA09/PA10).
*   OLED display connected via I2C or SPI (check `oled_driver.c` for specific pins).

## Building and Running

1.  Open the project in Keil MDK or IAR EWARM.
2.  Compile the project.
3.  Flash the resulting binary to the CW32F003 target board.
4.  Observe the output on the OLED display and optionally connect a serial terminal to view debug messages.
