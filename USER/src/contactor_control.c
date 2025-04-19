#include "contactor_control.h"
#include "config.h" // For pin/peripheral definitions
#include "cw32f003_gpio.h"
#include "cw32f003_rcc.h"

// Define which peripherals/pins are used (Update these in config.h later)
#define CONTACTOR_GPIO_PORT     CW_GPIOB   // Example: Use GPIOB
#define CONTACTOR_GPIO_PIN      GPIO_PIN_0 // Example: Use PB0
#define CONTACTOR_OPEN_STATE    0          // Logic level (e.g., 0) to open contactor
#define CONTACTOR_CLOSED_STATE  1          // Logic level (e.g., 1) to close contactor

// Internal state variable
static bool contactor_is_closed = false;

// --- Initialization ---

/**
 * @brief Initializes the GPIO pin used to control the contactor relay.
 */
void Contactor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // Configure Contactor Control Pin (e.g., PB0)
    // Assuming RCC clock for GPIOB is enabled elsewhere
    // CONTACTOR_GPIO_CLK_ENABLE(); // Clock should be enabled centrally

    GPIO_InitStruct.Pins = CONTACTOR_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Push-pull output
    // GPIO_InitStruct.Speed = GPIO_SPEED_LOW; // Speed setting not available in this HAL
    GPIO_Init(CONTACTOR_GPIO_PORT, &GPIO_InitStruct);

    // Set initial state to OPEN
    Contactor_Open();
}

// --- Control Functions ---

/**
 * @brief Opens the contactor (stops power flow).
 */
void Contactor_Open(void)
{
    // Set GPIO pin to the state that opens the contactor
    // #define CONTACTOR_OPEN_STATE 0 (Assuming LOW means 0)
    if (CONTACTOR_OPEN_STATE == 0) {
        GPIO_WritePin(CONTACTOR_GPIO_PORT, CONTACTOR_GPIO_PIN, GPIO_Pin_RESET); // Use GPIO_WritePin
    } else { // Assuming HIGH means 1
        GPIO_WritePin(CONTACTOR_GPIO_PORT, CONTACTOR_GPIO_PIN, GPIO_Pin_SET); // Use GPIO_WritePin
    }
    contactor_is_closed = false;
    // Add delay if necessary for relay switching time?
}

/**
 * @brief Closes the contactor (allows power flow).
 */
void Contactor_Close(void)
{
    // Set GPIO pin to the state that closes the contactor
    // #define CONTACTOR_CLOSED_STATE 1 (Assuming HIGH means 1)
    if (CONTACTOR_CLOSED_STATE == 0) { // Assuming LOW means 0
        GPIO_WritePin(CONTACTOR_GPIO_PORT, CONTACTOR_GPIO_PIN, GPIO_Pin_RESET); // Use GPIO_WritePin
    } else { // Assuming HIGH means 1
        GPIO_WritePin(CONTACTOR_GPIO_PORT, CONTACTOR_GPIO_PIN, GPIO_Pin_SET); // Use GPIO_WritePin
    }
    contactor_is_closed = true;
    // Add delay if necessary for relay switching time?
}

/**
 * @brief Checks if the contactor is currently commanded to be closed.
 * @return bool true if the contactor state is set to closed, false otherwise.
 * @note This reflects the commanded state, not necessarily the physical state
 *       unless feedback is implemented.
 */
bool Contactor_IsClosed(void)
{
    return contactor_is_closed;
}
