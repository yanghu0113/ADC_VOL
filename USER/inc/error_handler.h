#ifndef __ERROR_HANDLER_H
#define __ERROR_HANDLER_H

#include "error_codes.h" // Include the error code definitions
#include <stdint.h>      // For uint32_t

/**
 * @brief Handles a reported system error.
 *
 * This function is intended to be called whenever a module detects an error condition.
 * It logs the error details and takes appropriate action based on the severity
 * and type of the error.
 *
 * @param code The ErrorCode_t representing the specific error.
 * @param module_name A string literal indicating the source module (e.g., "ADC_Driver").
 *                    Using __FILE__ macro is also common here.
 * @param line_number The line number where the error was detected (use __LINE__ macro).
 */
void ErrorHandler_Handle(ErrorCode_t code, const char* module_name, uint32_t line_number);

/**
 * @brief Gets the last error code that was handled.
 *
 * Can be used by modules (like the state machine) to check if an error occurred.
 * Note: This might be overwritten quickly if multiple errors occur in succession.
 * Consider a more robust error queue if needed.
 *
 * @return The last ErrorCode_t passed to ErrorHandler_Handle, or ERROR_NONE if no error
 *         has occurred since reset or the last clear.
 */
ErrorCode_t ErrorHandler_GetLast(void);

/**
 * @brief Clears the last recorded error code, setting it back to ERROR_NONE.
 */
void ErrorHandler_ClearLast(void);

#endif // __ERROR_HANDLER_H
