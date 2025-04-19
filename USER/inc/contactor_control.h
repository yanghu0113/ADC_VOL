#ifndef __CONTACTOR_CONTROL_H
#define __CONTACTOR_CONTROL_H

#include "stdbool.h" // For bool type

// Function Prototypes
void Contactor_Init(void);      // Initialize GPIO pin for contactor control
void Contactor_Open(void);      // Open the contactor (stop power flow)
void Contactor_Close(void);     // Close the contactor (allow power flow)
bool Contactor_IsClosed(void);  // Check if the contactor is currently closed (optional feedback)

#endif // __CONTACTOR_CONTROL_H
