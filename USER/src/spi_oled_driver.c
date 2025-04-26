#include "spi_oled_driver.h"
#include "cw32f003.h"       // Include main device header first
#include "cw32f003_gpio.h"  // Explicitly include GPIO header
#include "config.h"         // For pin/peripheral definitions
#include "cw32f003_spi.h"   // SPI library
#include "cw32f003_rcc.h"   // RCC library
#include "error_handler.h"  // Include the error handler
#include "Font.h"           // Font data
#include <string.h>         // For memset if using buffer


extern void FirmwareDelay(uint32_t Cnt);

//--------------------------------------------------------------------------------------------------
// SPI Low-Level Communication Functions for OLED
// Define a reasonable timeout for SPI flag waits
// Adjust based on SPI clock speed and expected transaction time
#define SPI_TIMEOUT_COUNT 10000

/**
 * @brief Sends a single byte via SPI with timeout.
 * @param data The byte to send.
 * @return true if successful, false on timeout.
 */
static bool SPI_WriteByte(uint8_t data)
{
    volatile uint32_t timeout;

    // Wait until TX buffer is empty with timeout
    timeout = SPI_TIMEOUT_COUNT;
    while (SPI_GetFlagStatus(SPI_FLAG_TXE) == RESET)
    {
        if (timeout-- == 0) {
            ErrorHandler_Handle(ERROR_TIMEOUT, "SPI_Write_TXE", __LINE__);
            return false;
        }
    }

    // Send data
    SPI_SendData(data); // Removed OLED_SPI_PERIPH argument

    // Wait until transmission is complete (BUSY flag goes low) with timeout
    timeout = SPI_TIMEOUT_COUNT;
    while (SPI_GetFlagStatus(SPI_FLAG_BUSY) == SET)
    {
         if (timeout-- == 0) {
            ErrorHandler_Handle(ERROR_TIMEOUT, "SPI_Write_BUSY", __LINE__);
            return false;
        }
    }
    return true;
}

/**
 * @brief Writes a single command byte to the OLED via SPI.
 * @param command The command byte to write.
 * @return true if successful, false on SPI communication failure.
 */
bool OLED_WriteCommand(uint8_t command)
{
    bool status;
    OLED_DC_LOW();  // Select command mode
    OLED_CS_LOW();  // Select OLED
    status = SPI_WriteByte(command);
    OLED_CS_HIGH(); // Deselect OLED
    return status;
}

/**
 * @brief Writes a single data byte to the OLED via SPI.
 * @param data The data byte to write.
 * @return true if successful, false on SPI communication failure.
 */
bool OLED_WriteData(uint8_t data)
{
    bool status;
    OLED_DC_HIGH(); // Select data mode
    OLED_CS_LOW();  // Select OLED
    status = SPI_WriteByte(data);
    OLED_CS_HIGH(); // Deselect OLED
    return status;
}

//--------------------------------------------------------------------------------------------------
// OLED Initialization
//--------------------------------------------------------------------------------------------------
bool OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStruct;

    // 1. Enable Clocks
    RCC_AHBPeriphClk_Enable(RCC_AHB_PERIPH_GPIOB | RCC_AHB_PERIPH_GPIOC, ENABLE); // Use correct constants
    RCC_APBPeriphClk_Enable2(RCC_APB2_PERIPH_SPI, ENABLE); // Use APB2 enable function and constant

    // 2. Configure GPIO Pins
    // Configure RES(PC4), DC(PC3) as Output Push-Pull
    GPIO_InitStructure.Pins = OLED_DC_PIN | OLED_RES_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    // GPIO_InitStructure.Speed = GPIO_SPEED_HIGH; // Removed Speed setting
    GPIO_Init(OLED_DC_PORT, &GPIO_InitStructure); // Initialize PC3, PC4 (Use correct port)

    // Configure CS(PB0) as Output Push-Pull
    GPIO_InitStructure.Pins = OLED_CS_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(OLED_CS_PORT, &GPIO_InitStructure); // Initialize PB0

    // Configure SCK(PC0), MOSI(PC2) as SPI Alternate Function Push-Pull
    // Use direct AF macros instead of GPIO_MODE_AF_PP and GPIO_Init
    //PC00_AFx_SPISCK();
    PB07_AFx_SPISCK();  
    PC02_AFx_SPIMOSI();
    //PC00_DIR_OUTPUT(); // Set direction to output for AF
    PB07_DIR_OUTPUT();
    PC02_DIR_OUTPUT(); // Set direction to output for AF
    //PC00_PUSHPULL_ENABLE(); // Ensure push-pull for AF
    PB07_PUSHPULL_ENABLE();
    PC02_PUSHPULL_ENABLE(); // Ensure push-pull for AF
    //PC00_DIGTAL_ENABLE(); // Ensure digital mode
    PB07_DIGTAL_ENABLE();
    PC02_DIGTAL_ENABLE(); // Ensure digital mode

    // Ensure control pins are digital outputs
    PB00_DIR_OUTPUT();
    PB00_PUSHPULL_ENABLE();
    PB00_DIGTAL_ENABLE();
    PC03_DIR_OUTPUT();
    PC03_PUSHPULL_ENABLE();
    PC03_DIGTAL_ENABLE();
    PC04_DIR_OUTPUT();
    PC04_PUSHPULL_ENABLE();
    PC04_DIGTAL_ENABLE();

    // Set initial pin states
    OLED_CS_HIGH(); // Deselect OLED initially
    OLED_DC_HIGH(); // Default to data mode (can be either)
    OLED_RES_HIGH(); // Keep RES high initially

    // 3. Configure SPI Peripheral
    SPI_StructInit(&SPI_InitStruct); // Initialize with default values
    SPI_InitStruct.SPI_Direction = SPI_Direction_1Line_TxOnly; // Use correct constant
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;    // Clock low when idle (Mode 0 or 3) - Check SSD1309 datasheet
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;  // Data captured on first clock edge (Mode 0) - Check SSD1309 datasheet
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;     // Use software control for CS pin
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // SPI Clock = PCLK / 8 (Adjust as needed)
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    // SPI_InitStruct.SPI_Speed = SPI_Speed_High; // Add speed setting if needed/available
    SPI_Init(&SPI_InitStruct); // Removed OLED_SPI_PERIPH argument

    // Enable SPI
    SPI_Cmd(ENABLE); // Removed OLED_SPI_PERIPH argument

    // 4. Hardware Reset Sequence
    OLED_RES_LOW();
    //FirmwareDelay(10000); // Delay ~1ms
    FirmwareDelay(4800); // Delay ~1ms
    
    OLED_RES_HIGH();
    //FirmwareDelay(100000); // Delay > 10ms after reset
    FirmwareDelay(48000); // Delay ~1ms

    // 5. SSD1309 Initialization Sequence (Refer to SSD1309 Datasheet)
    // Check status of each command write. If any fail, report error and return false.
    if (!OLED_WriteCommand(0xAE)) return false; // Display OFF
    if (!OLED_WriteCommand(0xD5)) return false; // Set Display Clock Divide Ratio/Oscillator Frequency
    if (!OLED_WriteCommand(0xF0)) return false; // Set Ratio
    if (!OLED_WriteCommand(0xA8)) return false; // Set Multiplex Ratio
    if (!OLED_WriteCommand(0x3F)) return false; // 1/64 Duty
    if (!OLED_WriteCommand(0xD3)) return false; // Set Display Offset
    if (!OLED_WriteCommand(0x00)) return false; // No offset
    if (!OLED_WriteCommand(0x40)) return false; // Set Display Start Line (0)
    if (!OLED_WriteCommand(0x8D)) return false; // Set Charge Pump Setting
    if (!OLED_WriteCommand(0x14)) return false; // Enable Charge Pump
    if (!OLED_WriteCommand(0x20)) return false; // Set Memory Addressing Mode
    if (!OLED_WriteCommand(0x00)) return false; // Horizontal Addressing Mode
    if (!OLED_WriteCommand(0xA1)) return false; // Set Segment Re-map
    if (!OLED_WriteCommand(0xC8)) return false; // Set COM Output Scan Direction
    if (!OLED_WriteCommand(0xDA)) return false; // Set COM Pins Hardware Configuration
    if (!OLED_WriteCommand(0x12)) return false; // Sequential COM pin config
    if (!OLED_WriteCommand(0x81)) return false; // Set Contrast Control
    if (!OLED_WriteCommand(0xFF)) return false; // Max contrast
    if (!OLED_WriteCommand(0xD9)) return false; // Set Pre-charge Period
    if (!OLED_WriteCommand(0x22)) return false; // Phase 1: 2 DCLKs, Phase 2: 2 DCLKs
    if (!OLED_WriteCommand(0xDB)) return false; // Set VCOMH Deselect Level
    if (!OLED_WriteCommand(0x20)) return false; // ~0.77 x VCC
    if (!OLED_WriteCommand(0xA4)) return false; // Set Entire Display ON/OFF
    if (!OLED_WriteCommand(0xA6)) return false; // Set Normal Display

    // Clear screen RAM, check status
    if (!OLED_Clear()) {
        // Error already handled by ErrorHandler_Handle in OLED_Clear->OLED_WriteData->SPI_WriteByte
        // We still need to report the overall OLED init failure though.
        ErrorHandler_Handle(ERROR_OLED_INIT_FAILED, "OLED_Init_Clear", __LINE__);
        return false;
    }

    // Turn display ON
    if (!OLED_WriteCommand(0xAF)) return false; // Display ON

    // If all commands succeeded
    return true; // Indicate successful initialization
}

//--------------------------------------------------------------------------------------------------
// Basic OLED Functions
//--------------------------------------------------------------------------------------------------

/**
 * @brief Sets the DDRAM writing cursor position.
 * @param x Horizontal position (0-127).
 * @param y Vertical page position (0-7).
 * @return true if successful, false otherwise.
 */
bool OLED_SetCursor(uint8_t x, uint8_t y)
{
    // SSD1309 uses different commands for setting column/page
    // Check status of each command write
    if (!OLED_WriteCommand(0xB0 + y)) return false;         // Set Page Start Address
    if (!OLED_WriteCommand(0x00 | (x & 0x0F))) return false; // Set Lower Column Start Address
    if (!OLED_WriteCommand(0x10 | (x >> 4))) return false;  // Set Higher Column Start Address
    return true;
}

/**
 * @brief Clears the entire OLED screen (writes 0x00 to all GRAM).
 * @return true if successful, false if any SPI write fails.
 */
bool OLED_Clear(void)
{
    uint8_t i, n;
    for(i = 0; i < 8; i++)
    {
        // OLED_SetCursor calls OLED_WriteCommand which now returns bool, but we might ignore its status here
        // as failure is more likely during the bulk data write. Or check it too for robustness.
        OLED_SetCursor(0, i); // Assuming SetCursor failure is less critical or handled internally if needed
        for(n = 0; n < 128; n++)
        {
            if (!OLED_WriteData(0x00)) {
                // Error already handled by ErrorHandler_Handle in OLED_WriteData->SPI_WriteByte
                return false; // Propagate failure
            }
        }
    }
    return true; // Success
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
 * @return true if successful, false if any SPI write fails.
 */
bool OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size)
{
    uint8_t c = 0, i = 0;
    c = chr - ' '; // Get character index in font table (assuming ASCII starts from space)

    if (x > OLED_WIDTH - 1) { x = 0; y++; } // Basic wrap-around

    if (size == 8) // 8x16 Font
    {
        if (!OLED_SetCursor(x, y)) return false;
        for (i = 0; i < 8; i++) {
            if (!OLED_WriteData(F8X16[c * 16 + i])) return false; // Write top half
        }
        if (!OLED_SetCursor(x, y + 1)) return false;
        for (i = 0; i < 8; i++) {
            if (!OLED_WriteData(F8X16[c * 16 + i + 8])) return false; // Write bottom half
        }
    }
    else // Default to 6x8 Font
    {
        if (!OLED_SetCursor(x, y)) return false;
        for (i = 0; i < 6; i++) {
            if (!OLED_WriteData(F6x8[c][i])) return false;
        }
    }
    return true;
}

/**
 * @brief Displays a string at the specified position.
 * @param x Starting horizontal position (0-127).
 * @param y Starting page position (0-7).
 * @param str Pointer to the string.
 * @param size Font size (6 or 8).
 * @return true if successful, false if any character fails to display.
 */
bool OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size)
{
    uint8_t j = 0;
    uint8_t char_width = (size == 8) ? 8 : 6;

    while (str[j] != '\0')
    {
        if (!OLED_ShowChar(x, y, str[j], size)) {
            return false; // Propagate error from ShowChar
        }
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
    return true;
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
 * @return true if successful, false if any character fails to display.
 */
bool OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    uint8_t char_width = (size == 8) ? 8 : 6;
    bool status = true;

    for(t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if(enshow == 0 && t < (len - 1))
        {
            if(temp == 0)
            {
                if (!OLED_ShowChar(x + t * char_width, y, ' ', size)) status = false; // Show space for leading zeros
                continue;
            }
            else enshow = 1;
        }
        if (!OLED_ShowChar(x + t * char_width, y, temp + '0', size)) status = false;
    }
    return status;
}

/**
 * @brief Displays a hexadecimal number.
 * @param x Starting horizontal position.
 * @param y Starting page position.
 * @param num Number to display.
 * @param len Number of digits to display (e.g., 2 for byte, 4 for half-word, 8 for word).
 * @param size Font size (6 or 8).
 * @return true if successful, false if any character fails to display.
 */
bool OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t char_width = (size == 8) ? 8 : 6;
    char hex_char;
    bool status = true;

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
        if (!OLED_ShowChar(x + t * char_width, y, hex_char, size)) status = false;
    }
    return status;
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
    bool status = true;

    for (y = y0 / 8; y < (y0 / 8) + height_pages; y++)
    {
        if (!OLED_SetCursor(x0, y)) { status = false; break; } // Check cursor set status
        for (x = x0; x <= x1; x++)
        {
            if (!OLED_WriteData(BMP[i++])) { status = false; break; } // Check data write status
        }
        if (!status) break; // Exit outer loop if inner loop failed
    }
    // Return status? Function is void currently. Consider changing if needed.
}

// Placeholder for buffer update function if using buffer
void OLED_UpdateScreen(void)
{
    #ifdef OLED_USE_BUFFER
    // Implementation to write OLED_GRAM buffer to the screen
    uint8_t i, n;
    bool status = true;
    for(i = 0; i < 8; i++)
    {
        if (!OLED_SetCursor(0, i)) { status = false; break; }
        for(n = 0; n < 128; n++)
        {
            if (!OLED_WriteData(OLED_GRAM[i * 128 + n])) { status = false; break; }
        }
        if (!status) break;
    }
    // Return status? Function is void currently. Consider changing if needed.
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

#ifdef OLED_USE_BUFFER
// Define the screen buffer if used
uint8_t OLED_GRAM[OLED_WIDTH * OLED_HEIGHT / 8];
#endif
