#ifndef __UI_DISPLAY_H
#define __UI_DISPLAY_H

#include "stdint.h" // Use standard integer types

// Function Prototypes
void UI_Display_Init(void);     // Initialize the display (if needed beyond OLED_Init)
void UI_UpdateDisplay(void);    // Update the OLED screen based on current charging state and data

#endif // __UI_DISPLAY_H
