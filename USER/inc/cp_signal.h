#ifndef __CP_SIGNAL_H
#define __CP_SIGNAL_H

#include "stdint.h" // Use standard integer types

// Define CP Voltage States (Values are approximate and need calibration with real hardware)
// Based on typical interpretations of GB/T 20234.2-2015 / IEC 61851-1 Mode 3
typedef enum {
    CP_STATE_UNKNOWN = 0, // Error or invalid state
    CP_STATE_A_12V,       // State A: Vehicle not connected (+12V)
    CP_STATE_B_9V,        // State B: Vehicle connected, not ready (+9V)
    CP_STATE_C_6V,        // State C: Vehicle connected, ready, charging requested (+6V)
    CP_STATE_D_3V,        // State D: Vehicle connected, ready, ventilation required (+3V) - Often treated like C for AC
    CP_STATE_E_0V,        // State E: Error - Short circuit (0V)
    CP_STATE_F_NEG_12V    // State F: Error - EVSE malfunction (-12V)
} CP_State_t;

// Function Prototypes
void CP_Signal_Init(void); // Initialize PWM output and ADC input for CP
void CP_SetMaxCurrentPWM(uint8_t max_current_amps); // Set PWM duty cycle based on allowed current
CP_State_t CP_ReadState(void); // Read ADC voltage and return the interpreted CP state

#endif // __CP_SIGNAL_H
