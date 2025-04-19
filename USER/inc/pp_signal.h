#ifndef __PP_SIGNAL_H
#define __PP_SIGNAL_H

#include "stdint.h" // Use standard integer types

// Define Cable Capacities (Amperes) based on typical PP resistance values
// Values according to GB/T 20234.2-2015 / IEC 61851-1
#define PP_CAPACITY_UNKNOWN 0    // Error or invalid resistance
#define PP_CAPACITY_13A     13   // ~1500 Ohms
#define PP_CAPACITY_20A     20   // ~680 Ohms
#define PP_CAPACITY_32A     32   // ~220 Ohms
#define PP_CAPACITY_63A     63   // ~100 Ohms (Less common for AC Type 2 style)

// Function Prototypes
void PP_Signal_Init(void); // Initialize ADC input for PP
uint16_t PP_GetCableCapacity(void); // Read ADC, calculate resistance, return capacity in Amps

#endif // __PP_SIGNAL_H
