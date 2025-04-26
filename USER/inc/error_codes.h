#ifndef __ERROR_CODES_H
#define __ERROR_CODES_H

#include <stdint.h> // Include for uint32_t if needed later, good practice

// Define error codes for the system
typedef enum {
    // --- General Errors ---
    ERROR_NONE = 0,         // No error
    ERROR_UNKNOWN,          // An unspecified error occurred
    ERROR_TIMEOUT,          // A timeout occurred
    ERROR_BUFFER_FULL,      // A buffer (e.g., UART RX) became full
    ERROR_INVALID_PARAM,    // Invalid parameter passed to a function
    ERROR_NOT_IMPLEMENTED,  // Feature or function not yet implemented

    // --- Peripheral Initialization Errors ---
    ERROR_UART1_INIT_FAILED,
    ERROR_UART2_INIT_FAILED,
    ERROR_PWM_INIT_FAILED,
    ERROR_ADC_INIT_FAILED,
    ERROR_OLED_INIT_FAILED,
    ERROR_IWDT_INIT_FAILED,
    ERROR_SYSTICK_INIT_FAILED,
    ERROR_GPIO_INIT_FAILED,
    ERROR_SPI_INIT_FAILED,    // Added for SPI OLED

    // --- Runtime Errors ---
    // HLW8032 / AC Measurement
    ERROR_HLW_CHECKSUM,       // HLW8032 packet checksum mismatch
    ERROR_HLW_UART_TIMEOUT,   // Timeout waiting for HLW8032 data (if implemented)
    ERROR_HLW_FRAME,          // Framing error or unexpected data format

    // Charging Process / Safety
    ERROR_CP_VOLTAGE_INVALID, // Control Pilot voltage out of expected range
    ERROR_PP_RESISTANCE_INVALID,// Proximity Pilot resistance unexpected
    ERROR_CONTACTOR_FAULT,    // Contactor failed to switch or feedback mismatch
    ERROR_OVERCURRENT,        // Measured AC current exceeds limit
    ERROR_OVERVOLTAGE,        // Measured AC voltage exceeds limit
    ERROR_UNDERVOLTAGE,       // Measured AC voltage below limit
    ERROR_TEMPERATURE_HIGH,   // Temperature sensor reading too high (if sensor exists)
    ERROR_GFCI_FAULT,         // Ground Fault Circuit Interrupter tripped (if implemented)

    // Add more specific errors as needed...
    ERROR_STATE_INVALID,      // Reached an invalid state in state machine

} ErrorCode_t;

#endif // __ERROR_CODES_H
