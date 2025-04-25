#ifndef __CONFIG_H
#define __CONFIG_H

#include "cw32f003.h"
// Removed GPIO include, should be included in driver .c files where needed

//-----------------------------------------------------------------------------
// Hardware Pin Assignments & Peripheral Choices
//-----------------------------------------------------------------------------



// PWM Output
#define PWM_TIMER_PERIPH        CW_ATIM
#define PWM_TIMER_CLK_ENABLE()  __RCC_ATIM_CLK_ENABLE()
#define PWM_TIMER_IRQn          ATIM_IRQn
#define PWM_GPIO_PORT           CW_GPIOA
#define PWM_GPIO_CLK_ENABLE()   __RCC_GPIOA_CLK_ENABLE()
#define PWM_GPIO_PIN            GPIO_PIN_6
#define PWM_GPIO_AF_FUNC()      PA06_AFx_ATIMCH2B() // Using ATIM Channel 2B

// UART for Debug/Communication
#define DEBUG_USART_PERIPH      CW_UART1 // Or CW_UART2 depending on pins
#define DEBUG_USART_CLK_ENABLE() __RCC_UART1_CLK_ENABLE() // Match peripheral
#define DEBUG_USART_IRQn        UART1_IRQn // Match peripheral
// Reverted to original pin configuration: TX=PB2, RX=PA0
#define DEBUG_USART_TX_GPIO_PORT      CW_GPIOB
#define DEBUG_USART_RX_GPIO_PORT      CW_GPIOA
#define DEBUG_USART_GPIO_CLK_ENABLE() (__RCC_GPIOA_CLK_ENABLE(), __RCC_GPIOB_CLK_ENABLE()) // Enable both GPIOA and GPIOB clocks
#define DEBUG_USART_TX_PIN      GPIO_PIN_2 // PB2 for UART1_TX
#define DEBUG_USART_RX_PIN      GPIO_PIN_0 // PA0 for UART1_RX
#define DEBUG_USART_TX_AF_FUNC() PB02_AFx_UART1TXD() // Reverted to original AF macro
#define DEBUG_USART_RX_AF_FUNC() PA00_AFx_UART1RXD() // Reverted to original AF macro

// UART for HLW8032 Communication
#define HLW_USART_PERIPH        CW_UART2
#define HLW_USART_CLK_ENABLE()  __RCC_UART2_CLK_ENABLE()
#define HLW_USART_IRQn          UART2_IRQn
#define HLW_USART_GPIO_PORT     CW_GPIOC
#define HLW_USART_GPIO_CLK_ENABLE() __RCC_GPIOC_CLK_ENABLE()
#define HLW_USART_RX_PIN        GPIO_PIN_0 // PC0 for UART2_RX (Verify AF mapping)
#define HLW_USART_TX_PIN        GPIO_PIN_1 // PC1 for UART2_TX (Verify AF mapping, assign even if unused)
#define HLW_USART_RX_AF_FUNC()  PC00_AFx_UART2RXD() // Verify this macro exists and is correct
#define HLW_USART_TX_AF_FUNC()  PC01_AFx_UART2TXD() // Verify this macro exists and is correct


//-----------------------------------------------------------------------------
// Peripheral Configuration Defaults
//-----------------------------------------------------------------------------

#define DEBUG_UART_BAUDRATE     9600
#define HLW_UART_BAUDRATE       4800 // Baud rate for HLW8032
#define INITIAL_PWM_FREQ_HZ     1000
#define INITIAL_PWM_DUTY_PERCENT 50


// Define which peripherals/pins are used 
#define CP_PWM_TIMER            CW_ATIM // Use ATIM as defined in config.h
#define CP_PWM_FREQ_HZ          1000    // 1 kHz for Control Pilot
#define CP_PWM_GPIO_PORT        CW_GPIOA // Use GPIOA as defined in config.h
#define CP_PWM_GPIO_PIN         GPIO_PIN_6 // Use PA06 as defined in config.h for PWM

#define CP_ADC_CHANNEL          ADC_ExInputCH1 // Use library constant for Channel 1 (PA1)
#define CP_ADC_GPIO_PORT        CW_GPIOA
#define CP_ADC_GPIO_PIN         GPIO_PIN_1 // Example: PA1 for ADC Channel 1

// Define which peripherals/pins are used 
// Note: ADC_ExInputCH2 corresponds to PA04 according to cw32f003_adc.h
#define PP_ADC_CHANNEL          ADC_ExInputCH2 // Use the library constant directly
#define PP_ADC_GPIO_PORT        CW_GPIOA
#define PP_ADC_GPIO_PIN         GPIO_PIN_4 // PA04 for ADC Channel 2



#endif // __CONFIG_H
