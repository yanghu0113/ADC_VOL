#ifndef __PWM_DRIVER_H
#define __PWM_DRIVER_H

#include "cw32f003.h"
#include <stdint.h>
#include <stdbool.h> // Include for bool type

// Function prototypes
bool PWM_Driver_Init(uint32_t freqHz, uint8_t dutyCyclePercent); // Changed return type to bool
uint32_t PWM_Get_Frequency(void);
uint8_t PWM_Get_DutyCycle(void);

#endif // __PWM_DRIVER_H
