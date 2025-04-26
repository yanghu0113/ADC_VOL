#include "hlw_uart_driver.h"
#include "config.h" // Include the configuration header
#include "cw32f003_rcc.h"
#include "cw32f003_gpio.h"
#include "cw32f003_uart.h"
#include "error_handler.h" // Include the error handler
#include <stdio.h> // Keep for potential debugging printf inside driver
#include <stdbool.h>
#include <string.h>

// Static ring buffer instance for RX
static HLW_RingBuffer_t hlw_rx_buffer;

// --- Ring Buffer Helper Functions ---

/**
 * @brief Initializes the HLW ring buffer.
 * @param rb Pointer to the HLW_RingBuffer_t structure.
 */
static void HLW_RingBuffer_Init(HLW_RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

/**
 * @brief Checks if the HLW ring buffer is full.
 * @param rb Pointer to the HLW_RingBuffer_t structure.
 * @return true if full, false otherwise.
 */
static bool HLW_RingBuffer_IsFull(const HLW_RingBuffer_t *rb) {
    return rb->count >= HLW_UART_BUFFER_SIZE;
}

/**
 * @brief Checks if the HLW ring buffer is empty.
 * @param rb Pointer to the HLW_RingBuffer_t structure.
 * @return true if empty, false otherwise.
 */
static bool HLW_RingBuffer_IsEmpty(const HLW_RingBuffer_t *rb) {
    return rb->count == 0;
}

/**
 * @brief Adds a byte to the HLW ring buffer.
 * @param rb Pointer to the HLW_RingBuffer_t structure.
 * @param data The byte to add.
 * @return true if successful, false if buffer is full.
 */
static bool HLW_RingBuffer_Put(HLW_RingBuffer_t *rb, uint8_t data) {
    if (HLW_RingBuffer_IsFull(rb)) {
        // Consider adding overflow handling/flag here
        return false; // Buffer full
    }
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % HLW_UART_BUFFER_SIZE;
    __disable_irq(); // Enter critical section
    rb->count++;
    __enable_irq();  // Exit critical section
    return true;
}

/**
 * @brief Retrieves a byte from the HLW ring buffer.
 * @param rb Pointer to the HLW_RingBuffer_t structure.
 * @param data Pointer to store the retrieved byte.
 * @return true if successful, false if buffer is empty.
 */
static bool HLW_RingBuffer_Get(HLW_RingBuffer_t *rb, uint8_t *data) {
    if (HLW_RingBuffer_IsEmpty(rb)) {
        return false; // Buffer empty
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % HLW_UART_BUFFER_SIZE;
    __disable_irq(); // Enter critical section
    rb->count--;
    __enable_irq();  // Exit critical section
    return true;
}

// --- End Ring Buffer Helper Functions ---


/**
 * @brief Initializes UART2 peripheral for HLW8032, GPIO pins, and RX ring buffer.
 * @param baudRate The desired baud rate (should be 4800).
 * @return true if initialization successful, false otherwise.
 */
bool HLW_UART_Init(uint32_t baudRate)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // Initialize RX ring buffer
    HLW_RingBuffer_Init(&hlw_rx_buffer);

    // Enable peripheral clocks using macros from config.h
    // Assuming HSI is enabled elsewhere (e.g., main System_Init)
    HLW_USART_GPIO_CLK_ENABLE(); // Enable GPIO clock(s) for UART2 pins (GPIOC)
    HLW_USART_CLK_ENABLE();      // Enable UART2 peripheral clock

    // Configure GPIO Alternate Functions using macros
    // HLW_USART_TX_AF_FUNC(); // TX not used by HLW8032 for receiving data
    HLW_USART_RX_AF_FUNC(); // Configure PC0 for UART2 RX

    // Configure RX Pin (PC0)
    GPIO_InitStructure.Pins = HLW_USART_RX_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT_PULLUP; // Or GPIO_MODE_INPUT_FLOATING, check HLW datasheet/optocoupler output
    GPIO_Init(HLW_USART_GPIO_PORT, &GPIO_InitStructure);

    // Configure TX Pin (PC1) - Initialize even if unused to avoid floating input
    // If TX is truly unused, configuring as output might be safer than floating input.
    GPIO_InitStructure.Pins = HLW_USART_TX_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; // Configure as output push-pull
    GPIO_Init(HLW_USART_GPIO_PORT, &GPIO_InitStructure);

    // Configure NVIC using macro
    NVIC_SetPriority(HLW_USART_IRQn, 1); // Set interrupt priority (adjust as needed)
    NVIC_EnableIRQ(HLW_USART_IRQn);      // Enable UART2 interrupt in NVIC

    // Configure UART using macros
    USART_InitStructure.USART_BaudRate = baudRate; // Use passed-in baudRate (4800)
    USART_InitStructure.USART_Over = USART_Over_16;
    USART_InitStructure.USART_Source = USART_Source_PCLK; // Assuming PCLK source
    USART_InitStructure.USART_UclkFreq = RCC_Sysctrl_GetPClkFreq(); // Get PCLK dynamically
    USART_InitStructure.USART_StartBit = USART_StartBit_FE;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_Even ; // Set Even Parity for HLW8032
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx; // Enable RX only
    USART_Init(HLW_USART_PERIPH, &USART_InitStructure); // Use peripheral macro for UART2

    // Enable Receive interrupt
    USART_ITConfig(HLW_USART_PERIPH, USART_IT_RC, ENABLE); // Use peripheral macro for UART2

    return true; // Indicate successful initialization
}


// --- Public Non-Blocking Functions ---

/**
 * @brief Reads a single byte from the HLW UART RX buffer.
 * @return The byte read, or -1 if the buffer is empty.
 */
int16_t HLW_UART_Read(void) {
    uint8_t data;
    if (HLW_RingBuffer_Get(&hlw_rx_buffer, &data)) {
        return (int16_t)data;
    } else {
        return -1; // No data available
    }
}

/**
 * @brief Checks if there is data available in the HLW UART RX buffer.
 * @return true if data is available, false otherwise.
 */
bool HLW_UART_DataAvailable(void) {
    return !HLW_RingBuffer_IsEmpty(&hlw_rx_buffer);
}

// --- End Public Non-Blocking Functions ---


// --- Interrupt Handler Helper Functions (Called from ISR) ---

/**
 * @brief Internal function to handle UART reception from ISR.
 *        Should be called from UART2_IRQHandler when RC interrupt occurs.
 */
void HLW_UART_Handle_RC(void) {
    if (USART_GetFlagStatus(HLW_USART_PERIPH, USART_FLAG_RC) != RESET) { // Use peripheral macro for UART2
        uint8_t data = USART_ReceiveData_8bit(HLW_USART_PERIPH); // Use peripheral macro for UART2

        // Attempt to put data into buffer.
        if (!HLW_RingBuffer_Put(&hlw_rx_buffer, data)) {
            // Buffer is full, data is lost. Report the error.
            // WARNING: Calling complex handlers from ISR can be problematic.
            // Consider setting a flag for the main loop instead in critical systems.
            ErrorHandler_Handle(ERROR_BUFFER_FULL, "HLW_UART_ISR", __LINE__);
        }
        // Clear RC flag *after* reading data and attempting to store it
        USART_ClearITPendingBit(HLW_USART_PERIPH, USART_IT_RC); // Use peripheral macro for UART2
    }
    // Check for Overrun Error if necessary and flag definition exists
    // if (USART_GetFlagStatus(HLW_USART_PERIPH, USART_FLAG_OV) != RESET) {
    //     USART_ClearITPendingBit(HLW_USART_PERIPH, USART_FLAG_OV); // Or USART_ClearFlag
    //     // Handle overrun: data lost, maybe log error
    // }
}

// --- End Interrupt Handler Helper Functions ---
