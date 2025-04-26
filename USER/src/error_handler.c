#include "error_handler.h"
#include "uart_driver.h" // For printing error messages to debug UART
#include <stdio.h>       // For snprintf or printf

// --- Private Variables ---

// Store the last error code that occurred.
// Initialize to ERROR_NONE. Volatile might be needed if accessed from ISRs,
// but typically error handling is done in the main context or specific tasks.
static ErrorCode_t last_error_code = ERROR_NONE;

// --- Public Function Implementations ---

/**
 * @brief Handles a reported system error.
 */
void ErrorHandler_Handle(ErrorCode_t code, const char* module_name, uint32_t line_number)
{
    // Store the error code
    last_error_code = code;

    // Log the error to the debug UART (UART1)
    // Ensure printf is working correctly via uart_driver.c
    printf("ERROR: Code %d in %s at line %u\r\n", (int)code, module_name, line_number);

    // --- Add specific actions based on error code ---
    switch (code)
    {
        // --- Fatal Initialization Errors ---
        case ERROR_UART1_INIT_FAILED:
        case ERROR_SYSTICK_INIT_FAILED:
            // Critical failures - perhaps halt or enter safe mode
            printf("FATAL: Critical peripheral init failed. Halting.\r\n");
            // Optional: Blink an LED rapidly
            // Optional: Disable interrupts
             __disable_irq();
            while(1) {} // Halt execution
            break; // Technically unreachable

        // --- Safety Critical Runtime Errors ---
        case ERROR_CONTACTOR_FAULT:
        case ERROR_OVERCURRENT:
        case ERROR_OVERVOLTAGE:
        case ERROR_GFCI_FAULT: // If implemented
            printf("SAFETY CRITICAL ERROR: Opening contactor.\r\n");
            // TODO: Call function to safely open the contactor
            // Contactor_Open(); // Example
            // TODO: Stop CP PWM signal
            // CP_Signal_Stop(); // Example
            // TODO: Update UI to show critical fault
            // UI_Display_ShowError(code); // Example
            break;

        // --- Other Runtime Errors ---
        case ERROR_HLW_CHECKSUM:
        case ERROR_CP_VOLTAGE_INVALID:
        case ERROR_PP_RESISTANCE_INVALID:
            // Logged via printf above.
            // TODO: Update UI to indicate a non-critical error/warning
            // UI_Display_ShowWarning(code); // Example
            // State machine should handle transitions based on these errors.
            break;

        // --- Buffer Full / Timeouts (Might be warnings or recoverable) ---
        case ERROR_BUFFER_FULL:
        case ERROR_TIMEOUT:
            // Logged via printf above.
            // These might not require immediate action but indicate performance issues.
            break;

        // --- Default for other errors ---
        default:
            // Logged via printf above.
            // Consider if specific UI update or action is needed.
            break;
    }
}

/**
 * @brief Gets the last error code that was handled.
 */
ErrorCode_t ErrorHandler_GetLast(void)
{
    return last_error_code;
}

/* Optional: Implement ErrorHandler_ClearLast if needed
void ErrorHandler_ClearLast(void)
{
    last_error_code = ERROR_NONE;
}
*/
