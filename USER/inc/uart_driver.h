#ifndef __UART_DRIVER_H
#define __UART_DRIVER_H

#include "cw32f003.h"
#include <stdint.h>
#include <stdbool.h> // Include for bool type

// Define buffer sizes
#define UART_TX_BUFFER_SIZE 64
#define UART_RX_BUFFER_SIZE 64

// Ring buffer structure
typedef struct {
    volatile uint8_t buffer[UART_RX_BUFFER_SIZE]; // Use RX size for both for simplicity or define separate TX size
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} RingBuffer_t;

// Function prototypes
bool UART_Driver_Init(uint32_t baudRate); // Changed return type to bool
// void UART_Send_Char(char c); // Replaced by non-blocking write or printf
// void USART_SendString(UART_TypeDef* USARTx, char *String); // Replaced by non-blocking write or printf

// New non-blocking functions
bool UART_Write(const uint8_t* data, uint16_t length); // Non-blocking write (can still block if buffer full)
int16_t UART_Read(void); // Non-blocking read, returns -1 if no data
bool UART_DataAvailable(void); // Check if data is available in RX buffer

// Interrupt handler helper functions (called from ISR)
void UART_Driver_Handle_TXE(void);
void UART_Driver_Handle_RC(void);

#endif // __UART_DRIVER_H
