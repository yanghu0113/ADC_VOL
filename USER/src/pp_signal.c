#include "pp_signal.h"
#include "adc_driver.h" // Assumes adc_driver handles ADC reading
#include "config.h"     // For pin/peripheral definitions
#include "cw32f003_adc.h" // Include ADC peripheral header for channel constants
#include "cw32f003_rcc.h"
#include "cw32f003_gpio.h"

// Define which peripherals/pins are used (Update these in config.h later)
// Note: ADC_ExInputCH2 corresponds to PA04 according to cw32f003_adc.h
#define PP_ADC_CHANNEL          ADC_ExInputCH2 // Use the library constant directly
#define PP_ADC_GPIO_PORT        CW_GPIOA
#define PP_ADC_GPIO_PIN         GPIO_PIN_4 // PA04 for ADC Channel 2

// --- Initialization ---

/**
 * @brief Initializes ADC input for the Proximity Pilot (PP) signal.
 */
void PP_Signal_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // Configure ADC Input Pin (e.g., PA2)
    // Assuming RCC clock for GPIOA and ADC is enabled elsewhere
    // PP_ADC_GPIO_CLK_ENABLE(); // Clock should be enabled centrally
    // ADC_PERIPH_CLK_ENABLE();  // Clock should be enabled centrally

    GPIO_InitStruct.Pins = PP_ADC_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG; // Analog mode for ADC
    GPIO_Init(PP_ADC_GPIO_PORT, &GPIO_InitStruct);

    // Initialize ADC Driver (assuming it's already called in System_Init)
    // Ensure the specific channel (PP_ADC_CHANNEL) is configured if needed by ADC_Driver_Init
    // ADC_Driver_Init(); // Might not be needed if called globally
}

// --- Capacity Reading ---

/**
 * @brief Reads the PP resistance via ADC and determines the cable capacity.
 * @return uint16_t Cable capacity in Amperes (e.g., 13, 20, 32) or 0 if unknown/error.
 *
 * @note This is a SIMULATED version. It needs calibration with the actual
 *       voltage divider circuit, known pull-up/pull-down resistors, and ADC reference voltage.
 */
uint16_t PP_GetCableCapacity(void)
{
    // --- Real ADC Reading ---
    // Assumes PP connected to PA04 (ADC_ExInputCH2)
    // Assumes Vref = 3.3V, 12-bit ADC (0-4095)
    // Assumes 1kOhm pull-up resistor (R_pullup) to 3.3V. Cable provides R_pp to GND.
    // ADC_Voltage = 3.3V * R_pp / (R_pullup + R_pp)
    // ADC_Voltage_mV = RawValue * 3300 / 4095
    // R_pp = R_pullup * ADC_Voltage / (3.3V - ADC_Voltage)
    // R_pp = 1000 * (RawValue * 3300 / 4095) / (3300 - (RawValue * 3300 / 4095))
    // R_pp = 1000 * RawValue / (4095 - RawValue)

    // Define thresholds based on RAW ADC values (approximate, needs calibration)
    // R_pp = 1500 (13A) => Raw = 4095 * 1500 / (1000 + 1500) = 2457. Range: 2200 - 2700
    // R_pp = 680  (20A) => Raw = 4095 * 680 / (1000 + 680) = 1656. Range: 1400 - 1900
    // R_pp = 220  (32A) => Raw = 4095 * 220 / (1000 + 220) = 738. Range: 500 - 1000
    // R_pp = 100  (63A) => Raw = 4095 * 100 / (1000 + 100) = 372. Range: 200 - 500
    // Open circuit (No cable): R_pp = infinity => Raw = 4095. Treat as Unknown/Error?
    // Short circuit (Error): R_pp = 0 => Raw = 0. Treat as Unknown/Error.

    const uint16_t THRESHOLD_13A_LOW = 2200;
    const uint16_t THRESHOLD_13A_HIGH = 2700;
    const uint16_t THRESHOLD_20A_LOW = 1400;
    const uint16_t THRESHOLD_20A_HIGH = 1900;
    const uint16_t THRESHOLD_32A_LOW = 500;
    const uint16_t THRESHOLD_32A_HIGH = 1000;
    const uint16_t THRESHOLD_63A_LOW = 200;
    const uint16_t THRESHOLD_63A_HIGH = 500;

    uint16_t adc_raw = ADC_Read_Channel_Raw(PP_ADC_CHANNEL); // Read PP channel using defined constant

    // Add simple averaging or filtering here if needed for stability

    if (adc_raw >= THRESHOLD_13A_LOW && adc_raw <= THRESHOLD_13A_HIGH) {
        return PP_CAPACITY_13A;
    } else if (adc_raw >= THRESHOLD_20A_LOW && adc_raw <= THRESHOLD_20A_HIGH) {
        return PP_CAPACITY_20A;
    } else if (adc_raw >= THRESHOLD_32A_LOW && adc_raw <= THRESHOLD_32A_HIGH) {
        return PP_CAPACITY_32A;
    } else if (adc_raw >= THRESHOLD_63A_LOW && adc_raw <= THRESHOLD_63A_HIGH) {
        return PP_CAPACITY_63A;
    } else {
        // Outside known ranges, or very low/high values indicating open/short
        return PP_CAPACITY_UNKNOWN;
    }
}
