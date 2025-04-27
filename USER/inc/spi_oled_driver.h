#ifndef __SPI_OLED_DRIVER_H
#define __SPI_OLED_DRIVER_H

#include "cw32f003.h"
#include "base_types.h"
#include <stdbool.h> // Include for bool type

//-----------------OLED SPI Pin Definition----------------
// SPI Pins (Using SPI1 based on PC0/PC2)
//#define OLED_SPI_PORT           CW_GPIOB
//#define OLED_SCK_PIN            GPIO_PIN_7  // SCK: PC0
//#define OLED_MOSI_PIN           GPIO_PIN_2  // MOSI: PC2
// MISO (PC1) is not typically needed for display output

// Control Pins
#define OLED_CS_PORT            CW_GPIOB
#define OLED_CS_PIN             GPIO_PIN_0  // CS: PB0
#define OLED_DC_PORT            CW_GPIOC
#define OLED_DC_PIN             GPIO_PIN_3  // DC: PC3
#define OLED_RES_PORT           CW_GPIOC
#define OLED_RES_PIN            GPIO_PIN_4  // RES: PC4

// SPI Peripheral (Assuming SPI1)
#define OLED_SPI_PERIPH         CW_SPI1

//-----------------OLED Definition----------------
#define OLED_WIDTH              128
#define OLED_HEIGHT             64 // Assuming 128x64 resolution for SSD1309

// Helper Macros for Pin Control (Using GPIO_WritePin)
#define OLED_CS_LOW()           GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_Pin_RESET)
#define OLED_CS_HIGH()          GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_Pin_SET)
#define OLED_DC_LOW()           GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_Pin_RESET) // Command
#define OLED_DC_HIGH()          GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_Pin_SET)  // Data
#define OLED_RES_LOW()          GPIO_WritePin(OLED_RES_PORT, OLED_RES_PIN, GPIO_Pin_RESET)
#define OLED_RES_HIGH()         GPIO_WritePin(OLED_RES_PORT, OLED_RES_PIN, GPIO_Pin_SET)


// Function Prototypes
bool OLED_Init(void);
bool OLED_Clear(void); // Changed return type
bool OLED_SetCursor(uint8_t x, uint8_t y); // Changed return type
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color); // color: 1=white, 0=black (Keep void if not checking status)
void OLED_Fill(uint8_t data); // Keep void if not checking status
void OLED_UpdateScreen(void); // Keep void if not checking status
bool OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size); // Changed return type
bool OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size); // Changed return type
bool OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size); // Changed return type
bool OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size); // Changed return type
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t BMP[]); // Keep void if not checking status
bool OLED_ShowChineseChar(uint8_t x, uint8_t y, uint8_t index); // Function to show one Chinese char by index
bool OLED_ShowChineseString(uint8_t x, uint8_t y, const uint8_t* indices, uint8_t count);// Function to show string by indices

// Low-level SPI communication functions specific to OLED
bool OLED_WriteCommand(uint8_t command); // Changed return type
bool OLED_WriteData(uint8_t data);       // Changed return type
// Removed: void OLED_WriteBytes(uint8_t* data, uint16_t length, uint8_t controlByte); // I2C specific

// Optional: Define if using a screen buffer
// #define OLED_USE_BUFFER

#ifdef OLED_USE_BUFFER
extern uint8_t OLED_GRAM[OLED_WIDTH * OLED_HEIGHT / 8];
#endif

#endif /* __SPI_OLED_DRIVER_H */
