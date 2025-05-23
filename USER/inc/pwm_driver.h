#ifndef __PWM_DRIVER_H
#define __PWM_DRIVER_H

#include "cw32f003.h"
#include <stdint.h>
#include <stdbool.h> // Include for bool type

// Function prototypes
bool PWM_Driver_Init(uint32_t freqHz, uint8_t dutyCyclePercent); 
void PWM_Start(void); // Added prototype
void PWM_Stop(void);  // Added prototype 
bool PWM_Set_Frequency(uint32_t freqHz); // Added prototype 
bool PWM_Set_DutyCycle(uint8_t dutyCyclePercent); // Added prototype
uint32_t PWM_Get_Frequency(void);
uint8_t PWM_Get_DutyCycle(void);

#endif // __PWM_DRIVER_H
