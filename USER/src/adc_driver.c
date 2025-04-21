#include "adc_driver.h"
#include "cw32f003.h"      // Include the main device header
#include "cw32f003_rcc.h"  // Include RCC for clock enabling
#include "cw32f003_gpio.h" // Include GPIO for pin configuration
#include "cw32f003_adc.h"  // Include ADC peripheral driver
#include <stdio.h>         // For printf debugging
#include <math.h>          // Include for potential float operations (though likely not strictly needed for this formula)

/* FLASH Calibration Value Addresses */
#define CAL_T0_ADDRESS      (0x001007C5UL) // 8-bit T0 value (unit: 0.5 deg C)
#define CAL_TRIM1V5_ADDRESS (0x001007C6UL) // 16-bit Trim value for 1.5V ref

#define VOLTAGE_AVG_SAMPLES 8 // Number of samples to average for voltage reading

/**
 * @brief Initializes the ADC peripheral for single channel conversion on PA01.
 * @param None
 * @return true: Initialization successful, false: Initialization failed.
 */
bool ADC_Driver_Init(void)
{
    ADC_SingleChTypeDef ADC_SingleChStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable ADC and GPIOA clocks */
    RCC_AHBPeriphClk_Enable(RCC_AHB_PERIPH_GPIOA, ENABLE);
    RCC_APBPeriphClk_Enable2(RCC_APB2_PERIPH_ADC, ENABLE);

    /* Configure PA01 (CP) and PA02 (PP) as analog inputs */
    GPIO_InitStructure.Pins = GPIO_PIN_1 | GPIO_PIN_2; // Configure both pins
    GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
    GPIO_Init(CW_GPIOA, &GPIO_InitStructure);

    /* Reset ADC to default state */
    ADC_DeInit();

    /* Configure for single channel, single conversion mode */
    ADC_SingleChStructure.ADC_Chmux = ADC_ExInputCH1;          // Select Channel 1 (PA01)
    ADC_SingleChStructure.ADC_DiscardEn = ADC_DiscardNull;     // Keep the result
    ADC_SingleChStructure.ADC_InitStruct.ADC_OpMode = ADC_SingleChOneMode; // Single channel, single conversion
    ADC_SingleChStructure.ADC_InitStruct.ADC_ClkDiv = ADC_Clk_Div32; // ADC clock divider (Adjusted for 48MHz HCLK)
    ADC_SingleChStructure.ADC_InitStruct.ADC_SampleTime = ADC_SampTime5Clk; // Sample time (adjust as needed)
    ADC_SingleChStructure.ADC_InitStruct.ADC_VrefSel = ADC_Vref_VDD; // Reference voltage (adjust as needed, e.g., ADC_Vref_BGR1p5)
    ADC_SingleChStructure.ADC_InitStruct.ADC_InBufEn = ADC_BufDisable; // Input Buffer (adjust as needed)
    ADC_SingleChStructure.ADC_InitStruct.ADC_TsEn = ADC_TsDisable;    // Temperature sensor disabled
    ADC_SingleChStructure.ADC_InitStruct.ADC_Align = ADC_AlignRight;  // Right alignment
    ADC_SingleChStructure.ADC_InitStruct.ADC_AccEn = ADC_AccDisable;  // Accumulation disabled

    // Initialize watchdog structure to defaults (disabled)
    ADC_WdtInit(&ADC_SingleChStructure.ADC_WdtStruct);

    // Apply the configuration
    ADC_SingleChOneModeCfg(&ADC_SingleChStructure);

    /* Enable ADC */
    ADC_Enable();

    printf("ADC Driver Initialized (PA01/CH1, PA02/CH2)\r\n"); // Updated message
    return true; // Assuming initialization is always successful for now
}

/**
 * @brief Reads the raw ADC conversion value from the configured channel (PA01).
 * @param None
 * @return Raw 12-bit ADC conversion result.
 */
uint16_t ADC_Read_RawValue(void)
{
    /* Start software conversion */
    ADC_SoftwareStartConvCmd(ENABLE);

    /* Wait for conversion to complete */
    while(ADC_GetITStatus(ADC_IT_EOC) == RESET);

    /* Clear the End Of Conversion flag */
    ADC_ClearITPendingBit(ADC_IT_EOC);

    /* Return the conversion result */
    // The result is 12-bit, right-aligned in a 16-bit register
    return ADC_GetConversionValue();
}

/**
 * @brief Reads the ADC conversion value and converts it to millivolts.
 * @param None
 * @return Calculated voltage in millivolts (mV).
 */
uint16_t ADC_Read_Voltage_mV(void)
{
    ADC_SingleChTypeDef ADC_SingleChStructure_Volt; // Structure for voltage config
    // uint16_t rawValue; // Removed unused variable

    // --- Re-configure ADC for Voltage Sensing (PA01, VDD ref) ---
    // Ensure ADC clock is enabled (usually done in init, but good practice)
    RCC_APBPeriphClk_Enable2(RCC_APB2_PERIPH_ADC, ENABLE);
    ADC_Enable(); // Ensure ADC is enabled

    // Configure structure for voltage reading (similar to ADC_Driver_Init)
    ADC_SingleChStructure_Volt.ADC_InitStruct.ADC_OpMode = ADC_SingleChOneMode;
    ADC_SingleChStructure_Volt.ADC_InitStruct.ADC_ClkDiv = ADC_Clk_Div32;      // Adjusted for 48MHz HCLK
    ADC_SingleChStructure_Volt.ADC_InitStruct.ADC_SampleTime = ADC_SampTime5Clk; // Use sample time from Init
    ADC_SingleChStructure_Volt.ADC_InitStruct.ADC_VrefSel = ADC_Vref_VDD;      // Use VDD reference
    ADC_SingleChStructure_Volt.ADC_InitStruct.ADC_InBufEn = ADC_BufDisable;    // Use buffer setting from Init
    ADC_SingleChStructure_Volt.ADC_InitStruct.ADC_TsEn = ADC_TsDisable;       // Disable Temperature Sensor
    ADC_SingleChStructure_Volt.ADC_InitStruct.ADC_Align = ADC_AlignRight;
    ADC_SingleChStructure_Volt.ADC_InitStruct.ADC_AccEn = ADC_AccDisable;

    ADC_SingleChStructure_Volt.ADC_Chmux = ADC_ExInputCH1; // Select Channel 1 (PA01)
    ADC_SingleChStructure_Volt.ADC_DiscardEn = ADC_DiscardNull;

    // Initialize watchdog structure to defaults (disabled)
    ADC_WdtInit(&ADC_SingleChStructure_Volt.ADC_WdtStruct);

    // Apply the configuration
    ADC_SingleChOneModeCfg(&ADC_SingleChStructure_Volt);

    // --- Perform Averaged Conversion and Read ---
    uint32_t totalRawValue = 0;
    for (int i = 0; i < VOLTAGE_AVG_SAMPLES; i++) {
        // ADC_Read_RawValue handles start, wait, clear, read for each sample
        totalRawValue += ADC_Read_RawValue();
    }
    uint16_t averageRawValue = (uint16_t)(totalRawValue / VOLTAGE_AVG_SAMPLES);


    // --- Calculate Voltage ---
    // Calculate voltage: (AverageRawValue / MaxRawValue) * ReferenceVoltage
    // MaxRawValue for 12-bit ADC is 4095 (2^12 - 1)
    // Use integer arithmetic to avoid floating point: (AverageRawValue * RefVoltage_mV) / 4095
    uint32_t voltage = ((uint32_t)averageRawValue * ADC_REFERENCE_VOLTAGE_MV) / 4095;
    return (uint16_t)voltage;
}

/**
 * @brief Reads the raw ADC conversion value from a specific channel.
 * @param channel The ADC channel to read (e.g., ADC_ExInputCH1, ADC_ExInputCH2).
 * @return Raw 12-bit ADC conversion result, or 0xFFFF on error (e.g., invalid channel).
 */
uint16_t ADC_Read_Channel_Raw(ADC_Mux_TypeDef channel)
{
    ADC_SingleChTypeDef ADC_SingleChStructure;

    // Basic check for valid external channel range if needed, though type system helps
    // if (channel > ADC_ExInputCH7) return 0xFFFF; // Example check

    // Ensure ADC is enabled (might be redundant if always enabled, but safe)
    RCC_APBPeriphClk_Enable2(RCC_APB2_PERIPH_ADC, ENABLE);
    ADC_Enable();

    // Configure structure for the specific channel reading
    ADC_SingleChStructure.ADC_InitStruct.ADC_OpMode = ADC_SingleChOneMode;
    ADC_SingleChStructure.ADC_InitStruct.ADC_ClkDiv = ADC_Clk_Div32;      // Use same settings as Init
    ADC_SingleChStructure.ADC_InitStruct.ADC_SampleTime = ADC_SampTime5Clk; // Use same settings as Init
    ADC_SingleChStructure.ADC_InitStruct.ADC_VrefSel = ADC_Vref_VDD;      // Use VDD reference
    ADC_SingleChStructure.ADC_InitStruct.ADC_InBufEn = ADC_BufDisable;    // Use same settings as Init
    ADC_SingleChStructure.ADC_InitStruct.ADC_TsEn = ADC_TsDisable;       // Disable Temperature Sensor
    ADC_SingleChStructure.ADC_InitStruct.ADC_Align = ADC_AlignRight;
    ADC_SingleChStructure.ADC_InitStruct.ADC_AccEn = ADC_AccDisable;

    ADC_SingleChStructure.ADC_Chmux = channel; // Select the requested channel
    ADC_SingleChStructure.ADC_DiscardEn = ADC_DiscardNull;

    // Initialize watchdog structure to defaults (disabled)
    ADC_WdtInit(&ADC_SingleChStructure.ADC_WdtStruct);

    // Apply the configuration
    ADC_SingleChOneModeCfg(&ADC_SingleChStructure);

    // Start software conversion
    ADC_SoftwareStartConvCmd(ENABLE);

    // Wait for conversion to complete
    // Add a timeout mechanism here in a real application to prevent infinite loops
    while(ADC_GetITStatus(ADC_IT_EOC) == RESET);

    // Clear the End Of Conversion flag
    ADC_ClearITPendingBit(ADC_IT_EOC);

    // Return the conversion result
    return ADC_GetConversionValue();
}


/**
 * @brief Reads the internal temperature sensor.
 * @param None
 * @return Calculated temperature in degrees Celsius (float).
 */
float ADC_Read_Internal_Temperature(void)
{
    ADC_SingleChTypeDef ADC_SingleChStructure_Temp;
    uint16_t adc_raw_result;
    float temperature;
    volatile uint8_t T0_cal;
    volatile uint16_t Trim_cal;

    // --- Read Calibration Values ---
    T0_cal = *(volatile uint8_t*)CAL_T0_ADDRESS;
    Trim_cal = *(volatile uint16_t*)CAL_TRIM1V5_ADDRESS;

    // --- Configure ADC for Temperature Sensing (following steps from image) ---

    // Step 1: Enable ADC Clock (already done in ADC_Driver_Init, but ensure it's enabled)
    RCC_APBPeriphClk_Enable2(RCC_APB2_PERIPH_ADC, ENABLE);

    // Step 2: Enable ADC Module
    ADC_Enable(); // Ensure ADC is enabled before configuration

    // Step 3: Wait for ADC Ready (Optional but good practice)
    // A small delay might be sufficient, or check ADC_ISR_READY if needed.
    // Let's assume a short delay or proceed directly as the HAL might handle readiness checks.

    // Configure ADC structure specifically for temperature reading
    ADC_SingleChStructure_Temp.ADC_InitStruct.ADC_OpMode = ADC_SingleChOneMode; // Single channel, single conversion
    ADC_SingleChStructure_Temp.ADC_InitStruct.ADC_ClkDiv = ADC_Clk_Div32;       // Adjusted for 48MHz HCLK
    ADC_SingleChStructure_Temp.ADC_InitStruct.ADC_SampleTime = ADC_SampTime10Clk; // Use longest defined sample time
    ADC_SingleChStructure_Temp.ADC_InitStruct.ADC_VrefSel = ADC_Vref_BGR1p5;    // Step 5: Select 1.5V internal reference
    ADC_SingleChStructure_Temp.ADC_InitStruct.ADC_InBufEn = ADC_BufEnable;      // Step 9: Enable input buffer
    ADC_SingleChStructure_Temp.ADC_InitStruct.ADC_TsEn = ADC_TsEnable;         // Step 7: Enable Temperature Sensor
    ADC_SingleChStructure_Temp.ADC_InitStruct.ADC_Align = ADC_AlignRight;       // Right alignment
    ADC_SingleChStructure_Temp.ADC_InitStruct.ADC_AccEn = ADC_AccDisable;     // Accumulation disabled

    ADC_SingleChStructure_Temp.ADC_Chmux = ADC_TsInput; // Step 8: Select Temperature Sensor Channel (0x0E)
    ADC_SingleChStructure_Temp.ADC_DiscardEn = ADC_DiscardNull; // Keep the result

    // Initialize watchdog structure to defaults (disabled)
    ADC_WdtInit(&ADC_SingleChStructure_Temp.ADC_WdtStruct);

    // Apply the configuration (Steps 4, 5, 6, 7, 8, 9 combined)
    ADC_SingleChOneModeCfg(&ADC_SingleChStructure_Temp);

    // Step 10: Clear EOC flag
    ADC_ClearITPendingBit(ADC_IT_EOC);

    // Step 11: Start ADC Conversion
    ADC_SoftwareStartConvCmd(ENABLE);

    // Step 12: Wait for EOC (End of Conversion)
    while(ADC_GetITStatus(ADC_IT_EOC) == RESET);
    ADC_ClearITPendingBit(ADC_IT_EOC); // Clear flag after reading

    // Read ADC Result
    adc_raw_result = ADC_GetConversionValue();

    // Step 13: Disable ADC Module (Optional - depends if other ADC reads need it enabled)
    // Disabling might interfere if main loop reads voltage immediately after.
    // Consider if ADC_Driver_Init needs to be called again by main loop if we disable here.
    // For now, let's leave it enabled, assuming ADC_Read_Voltage_mV will handle its own start/wait.
    // ADC_Disable(); // Optional: Disable ADC

    // --- Calculate Temperature (Step 14) ---
    // Formula: Temp = T0 * 0.5 + 0.0924 * Vref * (AdcValue - Trim)
    // Vref = 1.5V
    temperature = (float)T0_cal * 0.5f + 0.0924f * 1.5f * ((float)adc_raw_result - (float)Trim_cal);

    return temperature;
}
