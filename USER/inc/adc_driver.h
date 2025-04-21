#ifndef __ADC_DRIVER_H
#define __ADC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "base_types.h" // For bool type if needed, or include stdint.h/stdbool.h directly
#include <stdint.h>    // For uint16_t
#include <stdbool.h>   // For bool

/* Defines -------------------------------------------------------------------*/
// Define the reference voltage used for calculation (e.g., 3.3V)
// Adjust this value based on your actual VDD or external reference voltage
#define ADC_REFERENCE_VOLTAGE_MV (3300) // Voltage in millivolts

/* Function Prototypes -------------------------------------------------------*/

/**
 * @brief Initializes the ADC peripheral for single channel conversion.
 * @param None
 * @return true: Initialization successful, false: Initialization failed.
 */
bool ADC_Driver_Init(void);

/**
 * @brief Reads the raw ADC conversion value from the configured channel.
 * @param None
 * @return Raw 12-bit ADC conversion result.
 */
uint16_t ADC_Read_RawValue(void);

/**
 * @brief Reads the ADC conversion value and converts it to millivolts.
 * @param None
 * @return Calculated voltage in millivolts (mV).
 */
uint16_t ADC_Read_Voltage_mV(void);

/**
 * @brief Reads the internal temperature sensor.
 * @param None
 * @return Calculated temperature in degrees Celsius (float).
 */
float ADC_Read_Internal_Temperature(void);

/**
 * @brief Reads the raw ADC conversion value from a specific channel.
 * @param channel The ADC channel to read (e.g., ADC_ExInputCH1, ADC_ExInputCH2).
 * @return Raw 12-bit ADC conversion result, or 0xFFFF on error (e.g., invalid channel).
 */
uint16_t ADC_Read_Channel_Raw(ADC_Mux_TypeDef channel);


#ifdef __cplusplus
}
#endif

#endif /* __ADC_DRIVER_H */
