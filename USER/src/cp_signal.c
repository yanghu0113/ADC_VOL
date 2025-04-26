#include "cp_signal.h"
#include "pwm_driver.h" 
#include "adc_driver.h" 
#include "config.h"     // For pin/peripheral definitions
#include "cw32f003_adc.h" 
#include "cw32f003_rcc.h"
#include "cw32f003_gpio.h"
#include "cw32f003_atim.h"
#include "error_handler.h" // Include the error handler
#include <stdio.h>         // Keep for now, maybe remove later



// --- Initialization ---

/**
 * @brief Initializes PWM output and ADC input for the Control Pilot signal.
 */
void CP_Signal_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // 1. Configure PWM Output Pin (e.g., PA0)
    // Assuming RCC clock for GPIOA and ATIM is enabled elsewhere (e.g., System_Init)
    // CP_PWM_GPIO_CLK_ENABLE(); // Clock should be enabled centrally
    // ATIM_TIMER_CLK_ENABLE();  // Clock should be enabled centrally

    GPIO_InitStruct.Pins = CP_PWM_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Push-pull output for PWM

    // Initialize PWM Driver (which uses ATIM and PA06 via config.h)
    // For now, assume it can be initialized with the desired frequency.
    // Initial state is +12V (State A), which corresponds to 100% duty cycle (always high)
    // or potentially a specific non-PWM state depending on hardware design.
    // Let's assume 100% duty cycle for now.
    if (!PWM_Driver_Init(CP_PWM_FREQ_HZ, 100)) {
        // Error is already handled by PWM_Driver_Init calling ErrorHandler_Handle
        // printf("Error: CP PWM Init Failed!\r\n"); // Removed redundant printf
    }
    PWM_Start(); // Start the PWM output

    // 2. Configure ADC Input Pin (e.g., PA1)
    // Assuming RCC clock for GPIOA and ADC is enabled elsewhere
    // CP_ADC_GPIO_CLK_ENABLE(); // Clock should be enabled centrally
    // ADC_PERIPH_CLK_ENABLE();  // Clock should be enabled centrally

    GPIO_InitStruct.Pins = CP_ADC_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG; // Analog mode for ADC
    GPIO_Init(CP_ADC_GPIO_PORT, &GPIO_InitStruct);

    // Initialize ADC Driver (assuming it's already called in System_Init)
    // Ensure the specific channel (CP_ADC_CHANNEL) is configured if needed by ADC_Driver_Init
    // ADC_Driver_Init(); // Might not be needed if called globally
}

// --- PWM Control ---

/**
 * @brief Sets the maximum charging current by adjusting the CP PWM duty cycle.
 * @param max_current_amps Maximum allowed current in Amperes.
 */
void CP_SetMaxCurrentPWM(uint8_t max_current_amps)
{
    uint8_t duty_cycle_percent;

    // Handle State A (0 Amps) - Set 100% duty cycle for constant +12V
    if (max_current_amps == 0) {
        duty_cycle_percent = 100;
    }
    // Handle currents below 6A (but not 0A) - Use minimum allowed PWM (e.g., 5% or 10% = 6A)
    else if (max_current_amps < 6) {
        // Standard implies PWM stops below 6A threshold? Or use 6A minimum?
        // Let's use 10% duty cycle (corresponds to 6A) as a minimum signal.
        duty_cycle_percent = 10;
    }
    // Handle 6A to 51A range
    else if (max_current_amps <= 51) {
        // Duty Cycle (%) = Amps / 0.6
        duty_cycle_percent = (uint8_t)(((float)max_current_amps / 0.6f) + 0.5f); // Calculate and round
    }
    // Handle 51A to 80A range
    else if (max_current_amps <= 80) {
        // Duty Cycle (%) = (Amps / 2.5) + 64
        duty_cycle_percent = (uint8_t)(((float)max_current_amps / 2.5f) + 64.0f + 0.5f); // Calculate and round
    }
    // Handle currents above 80A (treat as 80A or specific max duty?)
    else {
        // Set duty cycle corresponding to 80A as a maximum practical limit
        duty_cycle_percent = (uint8_t)(((float)80 / 2.5f) + 64.0f + 0.5f); // Calculate for 80A
    }

    // Clamp duty cycle to practical PWM range (e.g., 5% to 96%) *unless* it's State A (100%)
    if (duty_cycle_percent != 100) {
        if (duty_cycle_percent < 5) duty_cycle_percent = 5;
        if (duty_cycle_percent > 96) duty_cycle_percent = 96;
    }
    // Note: A duty cycle of 100% should result in CCR >= ARR+1 in the PWM driver,
    // effectively creating a constant high output.

    // Use the existing PWM driver function
    PWM_Set_DutyCycle(duty_cycle_percent);
}

// --- State Reading ---

/**
 * @brief Reads the CP voltage via ADC and determines the vehicle's state.
 * @return CP_State_t The interpreted state based on voltage levels.
 *
 * @note This is a SIMULATED version. It needs calibration with the actual
 *       voltage divider circuit and ADC reference voltage.
 */
CP_State_t CP_ReadState(void)
{
    // --- Real ADC Reading ---
    // Assumes CP connected to PA1 (ADC_ExInputCH1)
    // Assumes Vref = 3.3V, 12-bit ADC (0-4095)
    // Assumes divider: 2.7k to ADC, 1k to GND (ADC voltage = CP_Voltage * 1k / (2.7k + 1k))
    // CP_Voltage = ADC_Voltage * 3.7
    // ADC_Voltage = RawValue * 3300 / 4095 mV
    // CP_Voltage_mV = RawValue * 3300 / 4095 * 3.7 = RawValue * 12210 / 4095 ~= RawValue * 2.98

    // Define thresholds based on RAW ADC values (approximate, needs calibration)
    // State A (+12V): CP_mV > ~10500mV => Raw > ~3520? Let's use 3600 for margin.
    // State B (+9V):  CP_mV ~ 9000mV => Raw ~ 3020. Range: 2600 - 3600
    // State C (+6V):  CP_mV ~ 6000mV => Raw ~ 2013. Range: 1600 - 2600
    // State D (+3V):  CP_mV ~ 3000mV => Raw ~ 1006. Range: 600 - 1600
    // State E (0V):   CP_mV ~ 0V => Raw ~ 0. Range: < 600
    // State F (-12V): CP_mV ~ -12000mV (Clamped by diode to ~0V) => Raw ~ 0. Range: < 600

    const uint16_t THRESHOLD_A_MIN = 3600; // Min raw value for State A
    const uint16_t THRESHOLD_B_MIN = 2600; // Min raw value for State B
    const uint16_t THRESHOLD_C_MIN = 1600; // Min raw value for State C
    const uint16_t THRESHOLD_D_MIN = 600;  // Min raw value for State D
    const uint16_t ADC_ERROR_VALUE = 0xFFFF; // Value returned by ADC_Read_Channel_Raw on timeout

    uint16_t adc_raw = ADC_Read_Channel_Raw(ADC_ExInputCH1); // Read CP channel

    // Check for ADC read error (timeout)
    if (adc_raw == ADC_ERROR_VALUE) {
        // Error already reported by ADC_Read_Channel_Raw via ErrorHandler_Handle
        return CP_STATE_FAULT; // Return fault state
    }

    // Add simple averaging or filtering here if needed for stability
    // Note: If averaging, check each raw read for ADC_ERROR_VALUE

    // Determine state based on thresholds
    if (adc_raw >= THRESHOLD_A_MIN) {
        return CP_STATE_A_12V;
    } else if (adc_raw >= THRESHOLD_B_MIN) {
        return CP_STATE_B_9V;
    } else if (adc_raw >= THRESHOLD_C_MIN) {
        return CP_STATE_C_6V;
    } else if (adc_raw >= THRESHOLD_D_MIN) {
        return CP_STATE_D_3V;
    } else {
        // Treat values below D threshold as E (0V), F (-12V clamped), or other fault
        // Report this potentially invalid voltage level
        ErrorHandler_Handle(ERROR_CP_VOLTAGE_INVALID, "CP_ReadState", __LINE__);
        return CP_STATE_FAULT; // Return general fault state
    }
}
