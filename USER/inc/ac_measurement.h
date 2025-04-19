#ifndef __AC_MEASUREMENT_H
#define __AC_MEASUREMENT_H

#include "stdint.h" // Use standard integer types

// Function Prototypes
void AC_Measurement_Init(void); // Initialize ADC input for AC current sensor
float AC_GetCurrent(void);      // Read ADC, calculate RMS current, return Amps

#endif // __AC_MEASUREMENT_H
