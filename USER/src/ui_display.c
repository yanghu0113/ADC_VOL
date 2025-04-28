#include "ui_display.h"
#include "spi_oled_driver.h" // Use SPI OLED driver
#include "charging_sm.h"     // To get current state
#include "ac_measurement.h"  // To get current reading
#include "error_handler.h"   // Include error handler
#include <stdio.h>          // For sprintf
#include <string.h>         // For memset

// --- Initialization ---

/**
 * @brief Initializes the UI display.
 *        (Currently does nothing extra beyond OLED_Init called elsewhere)
 */
void UI_Display_Init(void)
{
    // SPI_OLED_Init() is assumed to be called in System_Init
    // Add any specific UI initialization here if needed
    OLED_Clear(); // Start with a clear screen using SPI driver
}

// --- Display Update ---

/**
 * @brief Updates the OLED screen based on the current charging state and data.
 */
void UI_UpdateDisplay(void)
{
    ErrorCode_t current_error = ErrorHandler_GetLast();

    // Clear the screen at the beginning of each update
    OLED_Clear(); // Assuming this returns bool, handle if needed

    if (current_error != ERROR_NONE) {
        // --- Display Error Information ---
        char error_msg[20];
        // Display generic fault message or map code to specific text
        sprintf(error_msg, "FAULT: Code %d", (int)current_error);

        // Display the error message (e.g., using 8x16 font for visibility)
        OLED_ShowString(0, 0, "----------------", 8); // Example separator
        OLED_ShowString(0, 2, error_msg, 8);          // Display error code on line 3 (pages 2,3)
        OLED_ShowString(0, 4, "----------------", 8); // Example separator

        // Optionally display specific Chinese error text if available and mapped
        // const uint8_t fault_indices[] = { /* indices for "故障" */ };
        // OLED_ShowChineseString(0, 0, fault_indices, sizeof(fault_indices));

    } else {
        // --- Display Normal Status Information ---

        // Display Chinese Status (e.g., "充电状态")
        // Indices: 0="充", 1="电", 2="枪", 3="状", 4="态"
        // NOTE: Index 5 is out of bounds for the current aFontChinese16 array! Correcting.
        const uint8_t status_indices[] = {0, 1, 3, 4}; // 充, 电, 状, 态
    uint8_t num_chars = sizeof(status_indices) / sizeof(status_indices[0]);//数组元素个数=数组总字节数/单个元素字节数

    OLED_ShowChineseString(0, 0, status_indices, num_chars);
    // Add error checking if needed: if (!OLED_ShowChineseString(...)) { /* handle error */ }

    // --- Display ASCII Status ---
    SM_State_t current_sm_state = SM_GetCurrentState();
    float current_amps = 0.0f;
    char state_str[20] = "State: ";
    char current_str[20] = "Current: ";
    char temp_buf[10]; // Buffer for sprintf

    // Get state string
    switch (current_sm_state)
    {
        case SM_STATE_INIT:         strcat(state_str, "INIT"); break;//将字符串常量 "INIT" 追加到目标字符串 state_str 的末尾
        case SM_STATE_IDLE:         strcat(state_str, "IDLE (A)"); break;
        case SM_STATE_CONNECTED:    strcat(state_str, "CONN (B)"); break;
        case SM_STATE_CHARGING_REQ: strcat(state_str, "REQ (C)"); break;
        case SM_STATE_CHARGING:     strcat(state_str, "CHARGE(C)"); break;
        case SM_STATE_VENTILATION:  strcat(state_str, "VENT (D)"); break;
        case SM_STATE_FAULT:        strcat(state_str, "FAULT!"); break;
        default:                    strcat(state_str, "UNKNOWN"); break;
    }

    // Get current only if charging
    if (current_sm_state == SM_STATE_CHARGING) {
        current_amps = AC_GetCurrent(); 
        sprintf(temp_buf, "%.1f A", current_amps); // Format current with 1 decimal place
        strcat(current_str, temp_buf);
    } else {
        strcat(current_str, "0.0 A");
    }

    // Update SPI OLED - Adjust Y coordinates for ASCII text
    // Display ASCII state on line 3 (page 2)
    OLED_ShowString(0,2 , state_str, 8); // Use SPI function, moved to y=2
    // Display ASCII current on line 4 (page 3)
    OLED_ShowString(0,4 , current_str, 8); // Use SPI function, moved to y=3


    // Add other info (Voltage, Temperature from main.c?) to other lines if desired
    // Example: Keep Voltage/Temp from main.c on lines 3 & 4

    // Update the physical screen from the buffer if enabled
    #ifdef SPI_OLED_USE_BUFFER // Check SPI buffer define if used
    // SPI_OLED_UpdateScreen(); // Or OLED_UpdateScreen() depending on definition
    #endif
    } // End of else block (normal display)
}
