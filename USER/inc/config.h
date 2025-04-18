#ifndef __CONFIG_H
#define __CONFIG_H

#include "cw32f003.h"
// Removed GPIO include, should be included in driver .c files where needed

//-----------------------------------------------------------------------------
// Hardware Pin Assignments & Peripheral Choices
//-----------------------------------------------------------------------------

// I2C for OLED Display
#define OLED_I2C_PERIPH         CW_I2C
#define OLED_I2C_CLK_ENABLE()   __RCC_I2C_CLK_ENABLE()
#define OLED_I2C_GPIO_PORT      CW_GPIOB
#define OLED_I2C_GPIO_CLK_ENABLE() __RCC_GPIOB_CLK_ENABLE()
#define OLED_I2C_SCL_PIN        GPIO_PIN_4
#define OLED_I2C_SDA_PIN        GPIO_PIN_3
#define OLED_I2C_SCL_AF_FUNC()  PB04_AFx_I2CSCL()
#define OLED_I2C_SDA_AF_FUNC()  PB03_AFx_I2CSDA()
#define OLED_I2C_ADDRESS        0x78 // Or 0x7A depending on SA0 pin

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

//-----------------------------------------------------------------------------
// Peripheral Configuration Defaults
//-----------------------------------------------------------------------------

#define DEBUG_UART_BAUDRATE     9600
#define INITIAL_PWM_FREQ_HZ     1000
#define INITIAL_PWM_DUTY_PERCENT 50

//-----------------------------------------------------------------------------
// Feature Flags
//-----------------------------------------------------------------------------

// #define OLED_USE_BUFFER // Uncomment if using a screen buffer in oled_driver

#endif // __CONFIG_H
