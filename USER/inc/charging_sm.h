#ifndef __CHARGING_SM_H
#define __CHARGING_SM_H

#include "stdint.h" // Use standard integer types

// Define the main charging states based on GB/T standard
typedef enum {
    SM_STATE_INIT,          // Initial state after power-on/reset
    SM_STATE_IDLE,          // State A: Vehicle not connected, CP = +12V
    SM_STATE_CONNECTED,     // State B: Vehicle connected, ready, CP = +9V
    SM_STATE_CHARGING_REQ,  // State C: Charging requested by EV, CP = +6V
    SM_STATE_CHARGING,      // State C active: Contactor closed, power flowing
    SM_STATE_VENTILATION,   // State D: Ventilation required (if supported), CP = +3V
    SM_STATE_FAULT          // State E/F or other error condition
} SM_State_t;

// Function Prototypes
void SM_Init(void);             // Initialize the state machine and related modules
void SM_RunStateMachine(void);  // Execute one iteration of the state machine logic
SM_State_t SM_GetCurrentState(void); // Get the current state of the machine

#endif // __CHARGING_SM_H
