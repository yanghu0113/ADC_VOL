#include "uart_driver.h"
#include "config.h" // Include the configuration header
#include "cw32f003_rcc.h"
#include "cw32f003_gpio.h"
#include "cw32f003_uart.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h> // For memcpy if needed, though likely not for single byte ops

// Static ring buffer instances
static RingBuffer_t tx_buffer;
static RingBuffer_t rx_buffer;

// --- Ring Buffer Helper Functions ---

/**
 * @brief Initializes a ring buffer.
 * @param rb Pointer to the RingBuffer_t structure.
 */
static void RingBuffer_Init(RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    // No need to clear rb->buffer explicitly unless required
}

/**
 * @brief Checks if the ring buffer is full.
 * @param rb Pointer to the RingBuffer_t structure.
 * @return true if full, false otherwise.
 */
static bool RingBuffer_IsFull(const RingBuffer_t *rb) {
    return rb->count >= UART_RX_BUFFER_SIZE; // Use RX size as it's the same or larger
}

/**
 * @brief Checks if the ring buffer is empty.
 * @param rb Pointer to the RingBuffer_t structure.
 * @return true if empty, false otherwise.
 */
static bool RingBuffer_IsEmpty(const RingBuffer_t *rb) {
    return rb->count == 0;
}

/**
 * @brief Adds a byte to the ring buffer.
 * @param rb Pointer to the RingBuffer_t structure.
 * @param data The byte to add.
 * @return true if successful, false if buffer is full.
 */
static bool RingBuffer_Put(RingBuffer_t *rb, uint8_t data) {
    if (RingBuffer_IsFull(rb)) {
        return false; // Buffer full
    }
    rb->buffer[rb->head] = data;
		//环绕效果，到达SIZE后head = 0
    rb->head = (rb->head + 1) % UART_RX_BUFFER_SIZE; // Use RX size
    __disable_irq(); // Enter critical section
    rb->count++;
    __enable_irq();  // Exit critical section
    return true;
}

/**
 * @brief Retrieves a byte from the ring buffer.
 * @param rb Pointer to the RingBuffer_t structure.
 * @param data Pointer to store the retrieved byte.
 * @return true if successful, false if buffer is empty.
 */
static bool RingBuffer_Get(RingBuffer_t *rb, uint8_t *data) {
    if (RingBuffer_IsEmpty(rb)) {
        return false; // Buffer empty
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % UART_RX_BUFFER_SIZE; // Use RX size
		//通过暂时关闭中断通道，创建一个保护区确保rb->count在自增自减时不被打断，避免错误
    __disable_irq(); // Enter critical section
    rb->count--;
    __enable_irq();  // Exit critical section
    return true;
}

// --- End Ring Buffer Helper Functions ---



#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */


/**
 * @brief Initializes UART1 peripheral, GPIO pins, and ring buffers.
 * @param baudRate The desired baud rate.
 * @return true if initialization successful, false otherwise.
 */
bool UART_Driver_Init(uint32_t baudRate) // Changed return type to bool
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // Initialize ring buffers
    RingBuffer_Init(&tx_buffer);
    RingBuffer_Init(&rx_buffer);

    // Enable peripheral clocks using macros from config.h
    // RCC_HSI_Enable(RCC_HSIOSC_DIV6); // REMOVED: System clock should be set in SystemInit, not here.
    DEBUG_USART_GPIO_CLK_ENABLE(); // Enable GPIO clock(s)
    DEBUG_USART_CLK_ENABLE();      // Enable UART peripheral clock

    // Configure GPIO Alternate Functions using macros
    DEBUG_USART_TX_AF_FUNC();
    DEBUG_USART_RX_AF_FUNC();

    // Configure TX Pin
    GPIO_InitStructure.Pins = DEBUG_USART_TX_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(DEBUG_USART_TX_GPIO_PORT, &GPIO_InitStructure);

    // Configure RX Pin
    GPIO_InitStructure.Pins = DEBUG_USART_RX_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT_PULLUP; // Or GPIO_MODE_INPUT_FLOATING
    GPIO_Init(DEBUG_USART_RX_GPIO_PORT, &GPIO_InitStructure);

    // Configure NVIC using macro
    NVIC_SetPriority(DEBUG_USART_IRQn, 0); // Set interrupt priority
    NVIC_EnableIRQ(DEBUG_USART_IRQn);      // Enable UART interrupt in NVIC

    // Configure UART using macros
    USART_InitStructure.USART_BaudRate = baudRate; // Use passed-in baudRate
    USART_InitStructure.USART_Over = USART_Over_16;
    USART_InitStructure.USART_Source = USART_Source_PCLK; // Assuming PCLK source
    USART_InitStructure.USART_UclkFreq = RCC_Sysctrl_GetPClkFreq(); // Get PCLK dynamically
    USART_InitStructure.USART_StartBit = USART_StartBit_FE;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(DEBUG_USART_PERIPH, &USART_InitStructure); // Use peripheral macro

    // Enable Receive interrupt, Transmit interrupt is enabled only when needed
    USART_ITConfig(DEBUG_USART_PERIPH, USART_IT_RC, ENABLE); // Use peripheral macro
    USART_ITConfig(DEBUG_USART_PERIPH, USART_IT_TXE, DISABLE); // Use peripheral macro

    return true; // Indicate successful initialization
}


// --- Public Non-Blocking Functions ---

/**
 * @brief Writes data to the UART TX buffer for asynchronous sending.
 * @param data Pointer to the data buffer.
 * @param length Number of bytes to write.
 * @return true if all data was successfully added to the buffer, false otherwise (buffer full).
 */
bool UART_Write(const uint8_t* data, uint16_t length) {
    uint16_t i;
    for (i = 0; i < length; ++i) {
        // Wait if buffer is full - this makes UART_Write blocking if buffer fills
        // A truly non-blocking version would return false immediately.
        while (RingBuffer_IsFull(&tx_buffer)) {
             // Optional: Add a small delay or yield if in an RTOS
        }
        if (!RingBuffer_Put(&tx_buffer, data[i])) {
             // This should ideally not happen due to the while loop above,
             // but included for robustness.
            return false;
        }
    }

    // If TXE interrupt is not enabled and there's data, enable it to start transmission
    __disable_irq();
    // Check IER register directly using the interrupt bit definition
    if (!RingBuffer_IsEmpty(&tx_buffer) && !(DEBUG_USART_PERIPH->IER & USART_IT_TXE)) { // Use peripheral macro
        USART_ITConfig(DEBUG_USART_PERIPH, USART_IT_TXE, ENABLE); // Use peripheral macro
    }
    __enable_irq();

    return true;
}


/**
 * @brief Reads a single byte from the UART RX buffer.
 * @return The byte read, or -1 if the buffer is empty.
 */
int16_t UART_Read(void) {
    uint8_t data;
    if (RingBuffer_Get(&rx_buffer, &data)) {
        return (int16_t)data;
    } else {
        return -1; // No data available
    }
}

/**
 * @brief Checks if there is data available in the UART RX buffer.
 * @return true if data is available, false otherwise.
 */
bool UART_DataAvailable(void) {
    return !RingBuffer_IsEmpty(&rx_buffer);
}

// --- End Public Non-Blocking Functions ---




/**
 * @brief Retargets the C library printf function to use the non-blocking UART write.
 */
PUTCHAR_PROTOTYPE
{
    uint8_t c = (uint8_t)ch;
    // This makes printf blocking if the buffer is full.
    // A more advanced implementation might handle buffer overflow differently.
    while (RingBuffer_IsFull(&tx_buffer)) {
        // Wait for space in the buffer
        // Optional: Add a small delay or yield if in an RTOS
    }

    RingBuffer_Put(&tx_buffer, c);

    // Ensure TXE interrupt is enabled if it's not already
    __disable_irq();
    // Check IER register directly using the interrupt bit definition
    if (!(DEBUG_USART_PERIPH->IER & USART_IT_TXE)) { // Use peripheral macro
        USART_ITConfig(DEBUG_USART_PERIPH, USART_IT_TXE, ENABLE); // Use peripheral macro
    }
    __enable_irq();

    return ch;
}


// --- Interrupt Handler Helper Functions (Called from ISR) ---

/**
 * @brief Internal function to handle UART transmission from ISR.
 *        Should be called when TXE interrupt occurs.
 */
void UART_Driver_Handle_TXE(void) { // Removed static
    uint8_t data;
    if (RingBuffer_Get(&tx_buffer, &data)) {
        USART_SendData_8bit(DEBUG_USART_PERIPH, data); // Use peripheral macro
    } else {
        // Buffer is empty, disable TXE interrupt
        USART_ITConfig(DEBUG_USART_PERIPH, USART_IT_TXE, DISABLE); // Use peripheral macro
    }
}

/**
 * @brief Internal function to handle UART reception from ISR.
 *        Should be called when RC interrupt occurs.
 */
void UART_Driver_Handle_RC(void) { // Removed static
    if (USART_GetFlagStatus(DEBUG_USART_PERIPH, USART_FLAG_RC) != RESET) { // Use peripheral macro
        uint8_t data = USART_ReceiveData_8bit(DEBUG_USART_PERIPH); // Use peripheral macro
        // Attempt to put data into buffer. If full, data is lost.
        // Consider adding overflow handling/flag if necessary.
        RingBuffer_Put(&rx_buffer, data);
        USART_ClearITPendingBit(DEBUG_USART_PERIPH, USART_IT_RC); // Clear RC flag *after* reading data - Use peripheral macro
    }
    // Note: Overrun flag (USART_FLAG_OV) is not defined in cw32f003_uart.h
     // Overrun conditions might need to be inferred or handled differently if required.
     /*
     if (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_OV)) { // USART_FLAG_OV undefined
         USART_ClearFlag(DEBUG_USARTx, USART_FLAG_OV);
         // Optional: Log error or increment an error counter
     }
     */
}

// --- End Interrupt Handler Helper Functions ---
