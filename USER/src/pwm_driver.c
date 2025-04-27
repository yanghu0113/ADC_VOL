#include "pwm_driver.h"
#include "config.h" // Include the configuration header
#include "cw32f003_rcc.h"
#include "cw32f003_gpio.h"
#include "cw32f003_atim.h"
#include "error_handler.h" // Include the error handler

// Store configuration for getter functions
static uint32_t pwm_frequency = 0;
static uint8_t pwm_duty_cycle = 0;


/**
 * @brief Initializes ATIM for PWM output.
 * @param freqHz Target frequency in Hz.
 * @param dutyCyclePercent Target duty cycle (0-100).
 * @return true if initialization successful, false otherwise.
 */
bool PWM_Driver_Init(uint32_t freqHz, uint8_t dutyCyclePercent) // Changed return type to bool
{
    ATIM_InitTypeDef ATIM_InitStruct;
    ATIM_OCInitTypeDef ATIM_OCInitStruct;
    GPIO_InitTypeDef GPIO_InitStructure;
	
	uint32_t timerClockFreq = 0;
    uint16_t prescalerValue = 0; // Actual prescaler value (PSC register)
    uint16_t arrValue = 0;       // Auto-reload value (ARR register)
    uint16_t ccrValue = 0;       // Capture/compare value (CCR register)

    if (freqHz == 0 || dutyCyclePercent > 100)
    {
        // Invalid parameters
        ErrorHandler_Handle(ERROR_INVALID_PARAM, "PWM_Init", __LINE__);
        return false; // Return failure
    }
    // Enable clocks using macros from config.h
    // RCC_HSI_Enable(RCC_HSIOSC_DIV1); // REMOVED: System clock should be set in SystemInit, not here.
    PWM_TIMER_CLK_ENABLE();
    PWM_GPIO_CLK_ENABLE();
    // GPIOB clock enable was unnecessary for PA06 PWM output

    // Configure GPIO Alternate Function using macro
    PWM_GPIO_AF_FUNC();

    // Configure GPIO pin using macros
    GPIO_InitStructure.IT = GPIO_IT_NONE;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output
    GPIO_InitStructure.Pins = PWM_GPIO_PIN;
    GPIO_Init(PWM_GPIO_PORT, &GPIO_InitStructure);

    // Interrupt configuration removed as the handler is empty and not needed
    // NVIC_SetPriority(PWM_TIMER_IRQn, 1);
    // NVIC_EnableIRQ(PWM_TIMER_IRQn);

    // Calculate Prescaler and ARR
    timerClockFreq = RCC_Sysctrl_GetPClkFreq(); // Assuming PWM_TIMER_PERIPH uses PCLK
    // Find suitable prescaler and ARR. Start with prescaler 0.
    // ARR = (TimerClock / Freq) - 1
    // CCR = (ARR + 1) * Duty / 100
    // Keep ARR <= 65535
    prescalerValue = 0; // Start with no prescaling
    arrValue = (timerClockFreq / (freqHz * (prescalerValue + 1))) - 1;
		
		
		 // Adjust prescaler if ARR is too large
    while (arrValue > 65535)
    {
        prescalerValue++;
        arrValue = (timerClockFreq / (freqHz * (prescalerValue + 1))) - 1;
        // Check if prescaler exceeded a reasonable limit (ATIM PSC is 8-bit, 0-255 maps to DIV1-DIV256)
        if (prescalerValue > 255) {
             // Cannot achieve frequency with available prescalers/clock
             ErrorHandler_Handle(ERROR_PWM_INIT_FAILED, "PWM_Init_FreqCalc", __LINE__);
             return false; // Return failure
        }
    }
		
		
		 //  Configure ATIM Time Base
    ATIM_InitStruct.BufferState = DISABLE;

    ATIM_InitStruct.CounterAlignedMode = ATIM_COUNT_MODE_EDGE_ALIGN;
    ATIM_InitStruct.CounterDirection = ATIM_COUNTING_UP;
    ATIM_InitStruct.CounterOPMode = ATIM_OP_MODE_REPETITIVE;

		
	  // Map calculated prescaler value to register setting (assuming direct mapping or find correct enum)
    // This part might need adjustment based on how ATIM_Prescaler_DIVx maps to PSC values
    if (prescalerValue == 0) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV1;
    else if (prescalerValue == 1) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV2;
    else if (prescalerValue < 4) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV4; // Example mapping, check datasheet
    else if (prescalerValue < 8) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV8;
    else if (prescalerValue < 16) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV16;
    else if (prescalerValue < 32) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV32; 
    else if (prescalerValue < 64) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV64; 
    else if (prescalerValue < 256) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV256; 
    else {
        // Prescaler too large for defined enums or calculation failed
        ErrorHandler_Handle(ERROR_PWM_INIT_FAILED, "PWM_Init_PrescalerMap", __LINE__);
        return false; // Return failure
    }

    // Use the calculated prescaler enum value
    ATIM_InitStruct.ClockSelect = ATIM_CLOCK_PCLK;
    // ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV8; // Remove hardcoded value
    ATIM_InitStruct.ReloadValue = arrValue;
    ATIM_InitStruct.RepetitionCounter = 0;
    ATIM_InitStruct.UnderFlowMask = DISABLE;
    ATIM_InitStruct.OverFlowMask = DISABLE;
    ATIM_Init(&ATIM_InitStruct); 

    // Configure Output Compare (Channel 2B for PA06)
    ATIM_OCInitStruct.BufferState = ENABLE;
    ATIM_OCInitStruct.OCInterruptSelect = ATIM_OC_IT_UP_COUNTER; // This setting is irrelevant if interrupt state is disabled
    ATIM_OCInitStruct.OCInterruptState = DISABLE; // Disable OC interrupt generation
    ATIM_OCInitStruct.OCMode = ATIM_OCMODE_PWM1;
    ATIM_OCInitStruct.OCPolarity = ATIM_OCPOLARITY_NONINVERT;
    ATIM_OC2BInit(&ATIM_OCInitStruct); // Removed incorrect PWM_TIMER_PERIPH argument

    // Calculate and Set Compare Value (CCR) for Duty Cycle
    ccrValue = ((uint32_t)(arrValue + 1) * dutyCyclePercent) / 100;
    ATIM_SetCompare2B(ccrValue); // Removed incorrect PWM_TIMER_PERIPH argument

    // Timer Interrupts are not enabled
    // ATIM_ITConfig(PWM_TIMER_PERIPH, ATIM_CR_IT_OVE, ENABLE);
    // ATIM_CH2Config(PWM_TIMER_PERIPH, ATIM_CHxB_CIE, ENABLE); 

    // Enable PWM Output
    ATIM_CtrlPWMOutputs(ENABLE); 

    // Store values
    pwm_frequency = freqHz;
    pwm_duty_cycle = dutyCyclePercent;

    // Enable Timer Counter
    ATIM_Cmd(ENABLE); // Removed incorrect PWM_TIMER_PERIPH argument

    return true; // Indicate successful initialization
}

// ATIM IRQ Handler Callback is not needed if interrupts are disabled
/*
void ATIM_IRQHandlerCallBack(void)
{
    // This handler would be called if ATIM interrupts were enabled and occurred.
    // Currently, interrupts are disabled in PWM_Driver_Init.
}
*/



/**
 * @brief Gets the configured PWM frequency.
 * @return uint32_t Frequency in Hz.
 */
uint32_t PWM_Get_Frequency(void)
{
    return pwm_frequency;
}

/**
 * @brief Gets the configured PWM duty cycle.
 * @return uint8_t Duty cycle percentage (0-100).
 */
uint8_t PWM_Get_DutyCycle(void)
{
    return pwm_duty_cycle;
}

/**
 * @brief Starts the PWM timer counter.
 */
void PWM_Start(void)
{
    // Assuming ATIM is the timer used (as configured in Init)
    ATIM_Cmd(ENABLE);
}

/**
 * @brief Stops the PWM timer counter.
 */
void PWM_Stop(void)
{
    // Assuming ATIM is the timer used
    ATIM_Cmd(DISABLE);
}

/**
 * @brief Sets the PWM duty cycle percentage.
 * @param dutyCyclePercent New duty cycle (0-100).
 * @return true if successful, false otherwise (e.g., invalid parameter).
 */
bool PWM_Set_DutyCycle(uint8_t dutyCyclePercent)
{
    uint16_t arrValue = 0;
    uint16_t ccrValue = 0;

    if (dutyCyclePercent > 100)
    {
        ErrorHandler_Handle(ERROR_INVALID_PARAM, "PWM_SetDuty", __LINE__);
        return false; // Invalid parameter
    }

    // Get the current auto-reload value (period)
    // Assuming ATIM is the timer used
    arrValue = CW_ATIM->ARR; // Read ARR directly

    // Calculate new compare value (CCR)
    // Ensure calculation handles potential overflow if arrValue is large
    ccrValue = (uint16_t)(((uint32_t)(arrValue + 1) * dutyCyclePercent) / 100);

    // Set the compare value for the correct channel (assuming CH2B for PA06 as in Init)
    // Assuming ATIM is the timer used
    ATIM_SetCompare2B(ccrValue);

    // Update stored duty cycle
    pwm_duty_cycle = dutyCyclePercent;

    return true;
}

/**
 * @brief Sets the PWM frequency.
 * @param freqHz New frequency in Hz.
 * @return true if successful, false otherwise (e.g., frequency not achievable).
 * @note This is more complex as it requires recalculating prescaler and ARR.
 *       It might be simpler to call PWM_Driver_Init again.
 *       Placeholder implementation returns false.
 */
bool PWM_Set_Frequency(uint32_t freqHz)
{
    // TODO: Implement frequency change logic (recalculate PSC, ARR, CCR)
    // This involves similar logic to PWM_Driver_Init's calculation part.
    // For now, just update the stored value if needed, but don't change hardware.
    // pwm_frequency = freqHz; // Optional: update stored value even if hardware not changed
    return false; // Indicate frequency change is not implemented yet
}
