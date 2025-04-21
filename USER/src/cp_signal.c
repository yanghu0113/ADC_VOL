#include "cp_signal.h"
#include "pwm_driver.h" // Assumes pwm_driver handles PWM generation - Ensure this is included
#include "adc_driver.h" // Assumes adc_driver handles ADC reading
#include "config.h"     // For pin/peripheral definitions
// #include "pwm_driver.h" // Remove duplicate include
#include "cw32f003_rcc.h"
#include "cw32f003_gpio.h"
#include "cw32f003_atim.h" // Assuming ATIM is used for PWM

// Define which peripherals/pins are used (Update these in config.h later)
#define CP_PWM_TIMER            CW_ATIM // Example: Use Advanced Timer
#define CP_PWM_FREQ_HZ          1000    // 1 kHz for Control Pilot
#define CP_PWM_GPIO_PORT        CW_GPIOA
#define CP_PWM_GPIO_PIN         GPIO_PIN_0 // Example: PA0 for ATIM CH1
#define CP_PWM_AF_FUNC          GPIO_AFx_ATIMCH1 // Example: Alternate function for PA0

#define CP_ADC_CHANNEL          ADC_CHANNEL_1 // Example: Use ADC Channel 1 (PA1)
#define CP_ADC_GPIO_PORT        CW_GPIOA
#define CP_ADC_GPIO_PIN         GPIO_PIN_1 // Example: PA1 for ADC Channel 1

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
    // GPIO_InitStruct.Speed = GPIO_SPEED_HIGH; // Speed setting not available in this HAL
    GPIO_Init(CP_PWM_GPIO_PORT, &GPIO_InitStruct);

    // Set Alternate Function for PA0 to ATIM CH1 (Assuming AF4 - VERIFY THIS in datasheet!)
    // Using macro style from cw32f003_gpio.h
    CW_GPIOA->AFRL_f.AFR0 = 4; // Set PA0 to AF4 (ATIM CH1A?) - NEEDS VERIFICATION

    // Initialize PWM Driver (assuming it uses the correct timer - ATIM in this case)
    // Note: pwm_driver.c might need modification if it's hardcoded to use BTIM/GTIM
    // For now, assume it can be initialized with the desired frequency.
    // Initial state is +12V (State A), which corresponds to 100% duty cycle (always high)
    // or potentially a specific non-PWM state depending on hardware design.
    // Let's assume 100% duty cycle for now.
    if (!PWM_Driver_Init(CP_PWM_FREQ_HZ, 100)) {
        // Handle PWM init error if necessary
        printf("Error: CP PWM Init Failed!\r\n");
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

    // GB/T Formula (simplified): Duty Cycle % = Amps / 0.6 (for 6A to 51A)
    // Clamp values according to standard
    if (max_current_amps < 6) {
        // Below minimum, potentially indicate error or use lowest duty?
        // Standard implies PWM stops below 6A threshold? Check standard.
        // For now, set a low duty cycle (e.g., 10% = 6A)
         duty_cycle_percent = 10;
    } else if (max_current_amps <= 51) {
        duty_cycle_percent = (uint8_t)(((float)max_current_amps / 0.6f) + 0.5f); // Calculate and round
    } else if (max_current_amps <= 80) {
         // Different formula for higher currents (check standard)
         // Placeholder: duty_cycle_percent = some_other_calculation;
         duty_cycle_percent = 85; // Example placeholder > 51A
    } else {
         duty_cycle_percent = 85; // Max duty for >80A? Check standard.
    }

    // Clamp duty cycle to valid range (e.g., 5% to 96% might be practical limits)
    if (duty_cycle_percent < 5) duty_cycle_percent = 5;
    if (duty_cycle_percent > 96) duty_cycle_percent = 96; // Avoid 0% and 100% if they have special meanings

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

    uint16_t adc_raw = ADC_Read_Channel_Raw(ADC_ExInputCH1); // Read CP channel

    // Add simple averaging or filtering here if needed for stability

    if (adc_raw >= THRESHOLD_A_MIN) {
        return CP_STATE_A_12V;
    } else if (adc_raw >= THRESHOLD_B_MIN) {
        return CP_STATE_B_9V;
    } else if (adc_raw >= THRESHOLD_C_MIN) {
        return CP_STATE_C_6V;
    } else if (adc_raw >= THRESHOLD_D_MIN) {
        return CP_STATE_D_3V;
    } else {
        // Treat values below D threshold as E or F (Fault)
        // Distinguishing E (0V) from F (-12V clamped) might require specific circuit checks
        // For now, group them as a fault condition.
        return CP_STATE_E_0V; // Or return a specific FAULT state if defined differently
    }
}
