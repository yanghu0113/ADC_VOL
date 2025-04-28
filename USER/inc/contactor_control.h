#ifndef __CONTACTOR_CONTROL_H
#define __CONTACTOR_CONTROL_H

#include <stdbool.h> // Include for bool type

// Enum for physical contactor state based on feedback
typedef enum {
    CONTACTOR_PHYS_OPEN,
    CONTACTOR_PHYS_CLOSED,
    CONTACTOR_PHYS_UNKNOWN // Optional: for initial state or read errors
} ContactorPhysicalState_t;

// Function Prototypes
void Contactor_Init(void);
void Contactor_Open(void);
void Contactor_Close(void);
bool Contactor_IsClosed(void); // Returns the *commanded* state
ContactorPhysicalState_t Contactor_ReadFeedbackState(void); // Returns the *physical* state

#endif // __CONTACTOR_CONTROL_H
