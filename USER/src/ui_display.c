#include "ui_display.h"
#include "oled_driver.h"    // For OLED functions
#include "charging_sm.h"    // To get current state
#include "ac_measurement.h" // To get current reading
#include <stdio.h>          // For sprintf

// --- Initialization ---

/**
 * @brief Initializes the UI display.
 *        (Currently does nothing extra beyond OLED_Init called elsewhere)
 */
void UI_Display_Init(void)
{
    // OLED_Init() is assumed to be called in System_Init
    // Add any specific UI initialization here if needed
    OLED_Clear(); // Start with a clear screen
}

// --- Display Update ---

/**
 * @brief Updates the OLED screen based on the current charging state and data.
 */
void UI_UpdateDisplay(void)
{
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

    // Update OLED (Example layout: State on line 1, Current on line 2)
    // Clear previous content might be needed depending on font/update method
    // A simple approach is to clear lines before writing.
    OLED_ShowString(0, 1, "                ", 6); // Clear line 1 (assuming 6x8 font)
    OLED_ShowString(0, 1, state_str, 6);

    OLED_ShowString(0, 2, "                ", 6); // Clear line 2
    OLED_ShowString(0, 2, current_str, 6);

    // Add other info (Voltage, Temperature from main.c?) to other lines if desired
    // Example: Keep Voltage/Temp from main.c on lines 3 & 4
}
