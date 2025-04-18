#include "pwm_driver.h"
#include "config.h" // Include the configuration header
#include "cw32f003_rcc.h"
#include "cw32f003_gpio.h"
#include "cw32f003_atim.h"

// Store configuration for getter functions
static uint32_t pwm_frequency = 0;
static uint8_t pwm_duty_cycle = 0;
// Unused variable removed

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
        return false; // Return failure
    }
    // Enable clocks using macros from config.h
    RCC_HSI_Enable(RCC_HSIOSC_DIV6); // Keep HSI config here for now
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
        if (prescalerValue > 255) { // Check against max prescaler hardware might support (often 16-bit, but ATIM uses specific values)
             // Cannot achieve frequency with available prescalers/clock
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
    else if (prescalerValue < 32) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV32; // Check if DIV32 exists
    else if (prescalerValue < 64) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV64; // Check if DIV64 exists
    else if (prescalerValue < 256) ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV256; // Check if DIV256 exists
    else return false; // Prescaler too large for defined enums - Return failure

    // Use the calculated prescaler enum value
    ATIM_InitStruct.ClockSelect = ATIM_CLOCK_PCLK;
    // ATIM_InitStruct.Prescaler = ATIM_Prescaler_DIV8; // Remove hardcoded value
    ATIM_InitStruct.ReloadValue = arrValue;
    ATIM_InitStruct.RepetitionCounter = 0;
    ATIM_InitStruct.UnderFlowMask = DISABLE;
    ATIM_InitStruct.OverFlowMask = DISABLE;
    ATIM_Init(&ATIM_InitStruct); // Removed incorrect PWM_TIMER_PERIPH argument

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
    // ATIM_CH2Config(PWM_TIMER_PERIPH, ATIM_CHxB_CIE, ENABLE); // Interrupts disabled anyway

    // Enable PWM Output
    ATIM_CtrlPWMOutputs(ENABLE); // Removed incorrect PWM_TIMER_PERIPH argument

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
