#include "oled_driver.h"
#include "config.h" // Include the configuration header
#include "cw32f003_i2c.h"
#include "cw32f003_gpio.h" // Added missing GPIO header
#include "Font.h"
#include <string.h>

// Define the screen buffer if used (moved to top)
#ifdef OLED_USE_BUFFER
#include "oled_driver.h" // Need OLED_WIDTH/HEIGHT defines
uint8_t OLED_GRAM[OLED_WIDTH * OLED_HEIGHT / 8];
#endif

extern void FirmwareDelay(uint32_t Cnt);

//--------------------------------------------------------------------------------------------------
// I2C Low-Level Communication Functions for OLED
//--------------------------------------------------------------------------------------------------

/**
 * @brief Writes multiple bytes (command or data) to the OLED via I2C.
 * @param data Pointer to the byte array.
 * @param length Number of bytes to write.
 * @param controlByte OLED_CONTROL_BYTE_CMD or OLED_CONTROL_BYTE_DATA.
 */
void OLED_WriteBytes(uint8_t* data, uint16_t length, uint8_t controlByte)
{
    // Interrupts should be disabled externally if required before calling this function
    uint8_t u8State;
    uint16_t i = 0;
    uint8_t sentControlByte = 0;

    I2C_GenerateSTART(ENABLE); 

    while(1)
    {
        while(0 == I2C_GetIrq()) {} 

        u8State = I2C_GetState(); 
        switch(u8State)
        {
            case 0x08: // START condition transmitted
                I2C_GenerateSTART(DISABLE); 
                I2C_Send7bitAddress(OLED_I2C_ADDRESS, I2C_Direction_Transmitter); 
                break;

            case 0x18: // SLA+W transmitted, ACK received
                I2C_SendData(controlByte); 
                sentControlByte = 1;
                break;

            case 0x28: // Data byte transmitted, ACK received
                if (!sentControlByte) // Should have sent control byte first
                {
                    // Error condition or unexpected state, try to recover
                    I2C_GenerateSTOP(ENABLE); 
                    I2C_ClearIrq();                     
                    return; // Exit
                 }
                 if (i < length)
                {
                    I2C_SendData(data[i++]);
                }
                else // All bytes sent
                {
                     I2C_GenerateSTOP(ENABLE); 
                     I2C_ClearIrq(); 
                     while(OLED_I2C_PERIPH->CR & bv4); // Keep peripheral access here for direct register check
        
                     return; // Transmission complete
                 }
                 break;

            case 0x20: // SLA+W transmitted, NACK received (Error)
            case 0x30: // Data byte transmitted, NACK received (Error)
            case 0x38: // Arbitration lost
                 I2C_GenerateSTOP(ENABLE); 
                 I2C_ClearIrq(); 
                 while(OLED_I2C_PERIPH->CR & bv4); // Keep peripheral access here for direct register check
                 
                 return; // Exit on error

             default: // Unexpected state
                 I2C_GenerateSTOP(ENABLE); 
                 I2C_ClearIrq(); 
                 while(OLED_I2C_PERIPH->CR & bv4); // Keep peripheral access here for direct register check
                 
                 return; // Exit
         }
         I2C_ClearIrq(); 
    }
}

/**
 * @brief Writes a single command byte to the OLED.
 * @param command The command byte to write.
 */
void OLED_WriteCommand(uint8_t command)
{
    OLED_WriteBytes(&command, 1, OLED_CONTROL_BYTE_CMD);
}

/**
 * @brief Writes a single data byte to the OLED.
 * @param data The data byte to write.
 */
void OLED_WriteData(uint8_t data)
{
    OLED_WriteBytes(&data, 1, OLED_CONTROL_BYTE_DATA);
}

//--------------------------------------------------------------------------------------------------
// OLED Initialization
//--------------------------------------------------------------------------------------------------
bool OLED_Init(void) // Changed return type to bool
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStruct;

    // Enable clocks using macros from config.h
    OLED_I2C_GPIO_CLK_ENABLE();
    OLED_I2C_CLK_ENABLE();

    // Configure GPIO Alternate Functions using macros
    OLED_I2C_SDA_AF_FUNC();
    OLED_I2C_SCL_AF_FUNC();

    // Configure GPIO pins using macros
    GPIO_InitStructure.Pins = OLED_I2C_SCL_PIN | OLED_I2C_SDA_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD; // Open-drain for I2C
    GPIO_Init(OLED_I2C_GPIO_PORT, &GPIO_InitStructure);

    // Configure I2C using macros
    I2C_InitStruct.I2C_BaudEn = ENABLE;
    I2C_InitStruct.I2C_Baud = 0x01;  // 500K=(8000000/(8*(1+1)) 
    I2C_InitStruct.I2C_FLT = DISABLE;
    I2C_InitStruct.I2C_AA = DISABLE; // Master mode doesn't need slave address acknowledge

    I2C_DeInit(); 
    I2C_Master_Init(&I2C_InitStruct); 
    I2C_Cmd(ENABLE); 


    // Initialization sequence follows
    FirmwareDelay(10000); // Delay > 100ms after power-on

    OLED_WriteCommand(0xAE); // Display OFF

    OLED_WriteCommand(0xD5); // Set Display Clock Divide Ratio/Oscillator Frequency
    OLED_WriteCommand(0x80); // Default Ratio/Frequency

    OLED_WriteCommand(0xA8); // Set Multiplex Ratio
    OLED_WriteCommand(0x3F); // 1/64 Duty (for 128x64 display)

    OLED_WriteCommand(0xD3); // Set Display Offset
    OLED_WriteCommand(0x00); // No offset

    OLED_WriteCommand(0x40); // Set Display Start Line (0)

    OLED_WriteCommand(0x8D); // Set Charge Pump Setting
    OLED_WriteCommand(0x14); // Enable Charge Pump

    OLED_WriteCommand(0x20); // Set Memory Addressing Mode
    OLED_WriteCommand(0x00); // Horizontal Addressing Mode

    OLED_WriteCommand(0xA1); // Set Segment Re-map (Column address 127 is mapped to SEG0) - Adjust if mirrored
    OLED_WriteCommand(0xC8); // Set COM Output Scan Direction (Remapped mode) - Adjust if mirrored

    OLED_WriteCommand(0xDA); // Set COM Pins Hardware Configuration
    OLED_WriteCommand(0x12); // Alternative COM pin configuration, Disable COM Left/Right remap

    OLED_WriteCommand(0x81); // Set Contrast Control
    OLED_WriteCommand(0xCF); // Default contrast

    OLED_WriteCommand(0xD9); // Set Pre-charge Period
    OLED_WriteCommand(0xF1); // Default pre-charge

    OLED_WriteCommand(0xDB); // Set VCOMH Deselect Level
    OLED_WriteCommand(0x40); // Default VCOMH level

    OLED_WriteCommand(0xA4); // Set Entire Display ON/OFF (A4=Output follows RAM, A5=Entire display ON)
    OLED_WriteCommand(0xA6); // Set Normal/Inverse Display (A6=Normal, A7=Inverse)

    OLED_Clear();            // Clear screen

    OLED_WriteCommand(0xAF); // Display ON

    return true; // Indicate successful initialization
}

//--------------------------------------------------------------------------------------------------
// Basic OLED Functions
//--------------------------------------------------------------------------------------------------

/**
 * @brief Sets the DDRAM writing cursor position.
 * @param x Horizontal position (0-127).
 * @param y Vertical page position (0-7).
 */
void OLED_SetCursor(uint8_t x, uint8_t y)
{
    x += 2;
    OLED_WriteCommand(0xB0 + y);             // Set Page Start Address (0-7)
    OLED_WriteCommand(((x & 0xF0) >> 4) | 0x10); // Set Higher Column Start Address
    OLED_WriteCommand((x & 0x0F) | 0x00);    // Set Lower Column Start Address
}

/**
 * @brief Clears the entire OLED screen (writes 0x00 to all GRAM).
 */
void OLED_Clear(void)
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        OLED_SetCursor(0, i);
        for(n = 0; n < 128; n++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/**
 * @brief Fills the entire OLED screen with a specified pattern.
 * @param data The byte pattern to fill (e.g., 0xFF for all white, 0x00 for all black).
 */
void OLED_Fill(uint8_t data)
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        OLED_SetCursor(0, i);
        for(n = 0; n < 128; n++)
        {
            OLED_WriteData(data);
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Character and String Functions (Requires Font.h)
//--------------------------------------------------------------------------------------------------

/**
 * @brief Displays a single character at the specified position.
 * @param x Starting horizontal position (0-127).
 * @param y Starting page position (0-7).
 * @param chr Character to display.
 * @param size Font size (currently supports 6 for 6x8, 8 for 8x16).
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size)
{
    uint8_t c = 0, i = 0;
    c = chr - ' '; // Get character index in font table (assuming ASCII starts from space)

    if (x > OLED_WIDTH - 1) { x = 0; y++; } // Basic wrap-around

    if (size == 8) // 8x16 Font
    {
        OLED_SetCursor(x, y);
        for (i = 0; i < 8; i++)
            OLED_WriteData(F8X16[c * 16 + i]); // Write top half
        OLED_SetCursor(x, y + 1);
        for (i = 0; i < 8; i++)
            OLED_WriteData(F8X16[c * 16 + i + 8]); // Write bottom half
    }
    else // Default to 6x8 Font
    {
        OLED_SetCursor(x, y);
        for (i = 0; i < 6; i++)
            OLED_WriteData(F6x8[c][i]);
    }
}

/**
 * @brief Displays a string at the specified position.
 * @param x Starting horizontal position (0-127).
 * @param y Starting page position (0-7).
 * @param str Pointer to the string.
 * @param size Font size (6 or 8).
 */
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size)
{
    uint8_t j = 0;
    uint8_t char_width = (size == 8) ? 8 : 6;

    while (str[j] != '\0')
    {
        OLED_ShowChar(x, y, str[j], size);
        x += char_width;
        if (x > (OLED_WIDTH - char_width)) // Wrap to next line
        {
            x = 0;
            y += (size == 8) ? 2 : 1; // Increment page(s)
        }
        if (y > (OLED_HEIGHT / 8 - ((size == 8) ? 2 : 1))) // Check if exceeding screen height
        {
           y = 0; // Wrap to top
           x = 0;
        }
        j++;
    }
}

// Helper function for OLED_ShowNum
uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while(n--) result *= m;
    return result;
}

/**
 * @brief Displays a decimal number.
 * @param x Starting horizontal position.
 * @param y Starting page position.
 * @param num Number to display.
 * @param len Number of digits to display.
 * @param size Font size (6 or 8).
 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    uint8_t char_width = (size == 8) ? 8 : 6;

    for(t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if(enshow == 0 && t < (len - 1))
        {
            if(temp == 0)
            {
                OLED_ShowChar(x + t * char_width, y, ' ', size); // Show space for leading zeros
                continue;
            }
            else enshow = 1;
        }
        OLED_ShowChar(x + t * char_width, y, temp + '0', size);
    }
}

/**
 * @brief Displays a hexadecimal number.
 * @param x Starting horizontal position.
 * @param y Starting page position.
 * @param num Number to display.
 * @param len Number of digits to display (e.g., 2 for byte, 4 for half-word, 8 for word).
 * @param size Font size (6 or 8).
 */
void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t char_width = (size == 8) ? 8 : 6;
    char hex_char;

    for(t = 0; t < len; t++)
    {
        temp = (num >> (4 * (len - t - 1))) & 0x0F; // Extract hex digit
        if(temp < 10)
        {
            hex_char = temp + '0';
        }
        else
        {
            hex_char = temp - 10 + 'A';
        }
        OLED_ShowChar(x + t * char_width, y, hex_char, size);
    }
}


// Placeholder for Bitmap function - requires specific implementation based on BMP format
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t BMP[])
{
    // Implementation depends heavily on how BMP data is stored.
    // Typically involves setting the drawing window and streaming pixel data.
    // Example for a simple monochrome format matching OLED pages:
    uint16_t i = 0;
    uint8_t x, y;
    // uint8_t width = x1 - x0 + 1; // Unused variable removed
    uint8_t height_pages = (y1 - y0 + 1) / 8; // Assuming height is multiple of 8

    for (y = y0 / 8; y < (y0 / 8) + height_pages; y++)
    {
        OLED_SetCursor(x0, y);
        for (x = x0; x <= x1; x++)
        {
            OLED_WriteData(BMP[i++]);
        }
    }
}

// Placeholder for buffer update function if using buffer
void OLED_UpdateScreen(void)
{
    #ifdef OLED_USE_BUFFER
    // Implementation to write OLED_GRAM buffer to the screen
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        OLED_SetCursor(0, i);
        for(n = 0; n < 128; n++)
        {
            OLED_WriteData(OLED_GRAM[i * 128 + n]);
        }
    }
    #endif
}

// Placeholder for DrawPixel if using buffer
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    #ifdef OLED_USE_BUFFER
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint16_t index = (y / 8) * OLED_WIDTH + x;
    uint8_t bit_pos = y % 8;
    if (color) // Set pixel (white)
    {
        OLED_GRAM[index] |= (1 << bit_pos);
    }
    else // Clear pixel (black)
    {
        OLED_GRAM[index] &= ~(1 << bit_pos);
    }
    #else
    // Direct drawing not implemented efficiently without buffer
    // Could read-modify-write, but that's slow over I2C
#endif
}

// Original buffer definition location removed
