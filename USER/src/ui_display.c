#include "ui_display.h"
#include "spi_oled_driver.h" // Use SPI OLED driver
#include "charging_sm.h"     // To get current state
#include "ac_measurement.h"  // To get current reading
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
    // Clear the screen at the beginning of each update
    OLED_Clear(); // Use the bool return if you want to check status

    // --- Display Chinese Status ---

    const uint8_t status_indices[] = {0, 1, 2, 3, 4, 5}; // 充, 电, 状, 态
    uint8_t num_chars = sizeof(status_indices) / sizeof(status_indices[0]);

    // Show the Chinese string at x=0, y=0 (first line, takes pages 0 and 1)
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
        case SM_STATE_INIT:         strcat(state_str, "INIT"); break;
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
        current_amps = AC_GetCurrent(); // Get (simulated) current
        sprintf(temp_buf, "%.1f A", current_amps); // Format current with 1 decimal place
        strcat(current_str, temp_buf);
    } else {
        strcat(current_str, "0.0 A");
    }

    // Update SPI OLED - Adjust Y coordinates for ASCII text
    // Display ASCII state on line 3 (page 2)
    OLED_ShowString(0, 2, state_str, 6); // Use SPI function, moved to y=2
    // Display ASCII current on line 4 (page 3)
    OLED_ShowString(0, 3, current_str, 6); // Use SPI function, moved to y=3


    // Add other info (Voltage, Temperature from main.c?) to other lines if desired
    // Example: Keep Voltage/Temp from main.c on lines 3 & 4

    // Update the physical screen from the buffer if enabled
    #ifdef SPI_OLED_USE_BUFFER // Check SPI buffer define if used
    SPI_OLED_UpdateScreen();
    #endif
}
