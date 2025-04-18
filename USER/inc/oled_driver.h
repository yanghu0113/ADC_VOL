#ifndef __OLED_DRIVER_H
#define __OLED_DRIVER_H

#include "cw32f003.h"
#include "base_types.h"
#include <stdbool.h> // Include for bool type

#define  I2C_SCL_GPIO_PORT       CW_GPIOB
#define  I2C_SCL_GPIO_PIN        GPIO_PIN_4   
#define  I2C_SDA_GPIO_PORT       CW_GPIOB   
#define  I2C_SDA_GPIO_PIN        GPIO_PIN_3 

//-----------------OLED Definition----------------
#define OLED_I2C_ADDRESS    0x78 // 7-bit address -> 8-bit address (write)
#define OLED_WIDTH          128
#define OLED_HEIGHT         64

// OLED Control Byte Defines
#define OLED_CONTROL_BYTE_CMD   0x00
#define OLED_CONTROL_BYTE_DATA  0x40

// Function Prototypes
bool OLED_Init(void); // Changed return type to bool
void OLED_Clear(void);
void OLED_SetCursor(uint8_t x, uint8_t y);
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color); // color: 1=white, 0=black
void OLED_Fill(uint8_t data);
void OLED_UpdateScreen(void); // Update the display with buffer content (if using buffer)
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size); // size: 6 or 8 (for 6x8 or 8x16 font)
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t BMP[]);

// Low-level I2C communication functions specific to OLED
void OLED_WriteCommand(uint8_t command);
void OLED_WriteData(uint8_t data);
void OLED_WriteBytes(uint8_t* data, uint16_t length, uint8_t controlByte); // Generic byte writing function

// Optional: Define if using a screen buffer
// #define OLED_USE_BUFFER

#ifdef OLED_USE_BUFFER
extern uint8_t OLED_GRAM[OLED_WIDTH * OLED_HEIGHT / 8];
#endif

#endif // __OLED_DRIVER_H
