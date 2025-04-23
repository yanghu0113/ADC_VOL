#ifndef __HLW_UART_DRIVER_H
#define __HLW_UART_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Defines -------------------------------------------------------------------*/
// Define buffer size - adjust if needed for HLW8032 packet handling
#define HLW_UART_BUFFER_SIZE 64 // Example size, ensure it's >= 24 bytes

/* Typedefs ------------------------------------------------------------------*/
// Simple Ring Buffer Structure
typedef struct {
    uint8_t buffer[HLW_UART_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} HLW_RingBuffer_t;

/* Function Prototypes -------------------------------------------------------*/

/**
 * @brief Initializes UART2 peripheral for HLW8032 communication.
 * @param baudRate The desired baud rate (should be 4800).
 * @return true if initialization successful, false otherwise.
 */
bool HLW_UART_Init(uint32_t baudRate);

/**
 * @brief Reads a single byte from the HLW UART RX buffer.
 * @return The byte read, or -1 if the buffer is empty.
 */
int16_t HLW_UART_Read(void);

/**
 * @brief Checks if there is data available in the HLW UART RX buffer.
 * @return true if data is available, false otherwise.
 */
bool HLW_UART_DataAvailable(void);

/**
 * @brief Internal function to handle UART reception from ISR.
 *        Should be called from UART2_IRQHandler when RC interrupt occurs.
 */
void HLW_UART_Handle_RC(void);

// Note: TX functions are omitted as HLW8032 only sends data to MCU

#ifdef __cplusplus
}
#endif

#endif /* __HLW_UART_DRIVER_H */
