#include "contactor_control.h"
#include "config.h" // For pin/peripheral definitions
#include "cw32f003_gpio.h"
#include "cw32f003_rcc.h"

// Pin definitions are now in config.h

// Internal state variable for commanded state
static bool contactor_is_commanded_closed = false;

// --- Initialization ---

/**
 * @brief Initializes the GPIO pin used to control the contactor relay.
 */
void Contactor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // Enable clocks (ensure RCC is included if not done globally)
    // Assuming GPIOB clock is needed for both control and feedback
    CONTACTOR_CTRL_GPIO_CLK_ENABLE(); // Enable clock defined in config.h
    // CONTACTOR_FB_GPIO_CLK_ENABLE(); // Already enabled if same port as control

    // Configure Contactor Control Pin (PB0)
    GPIO_InitStruct.Pins = CONTACTOR_CTRL_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Push-pull output
    GPIO_Init(CONTACTOR_CTRL_GPIO_PORT, &GPIO_InitStruct);

    // Configure Contactor Feedback Pin (PB1)
    GPIO_InitStruct.Pins = CONTACTOR_FB_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLDOWN; // Input with pull-down (adjust if external pull-up/down exists)
    // Or GPIO_MODE_INPUT_PULLUP if logic requires it
    GPIO_Init(CONTACTOR_FB_GPIO_PORT, &GPIO_InitStruct);

    // Set initial commanded state to OPEN
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
        GPIO_WritePin(CONTACTOR_CTRL_GPIO_PORT, CONTACTOR_CTRL_GPIO_PIN, GPIO_Pin_RESET);
    } else { // Assuming HIGH means 1
        GPIO_WritePin(CONTACTOR_CTRL_GPIO_PORT, CONTACTOR_CTRL_GPIO_PIN, GPIO_Pin_SET);
    }
    contactor_is_commanded_closed = false;
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
        GPIO_WritePin(CONTACTOR_CTRL_GPIO_PORT, CONTACTOR_CTRL_GPIO_PIN, GPIO_Pin_RESET);
    } else { // Assuming HIGH means 1
        GPIO_WritePin(CONTACTOR_CTRL_GPIO_PORT, CONTACTOR_CTRL_GPIO_PIN, GPIO_Pin_SET);
    }
    contactor_is_commanded_closed = true;
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
    return contactor_is_commanded_closed;
}

/**
 * @brief Reads the physical state of the contactor via the feedback pin.
 * @return ContactorPhysicalState_t The detected physical state.
 */
ContactorPhysicalState_t Contactor_ReadFeedbackState(void)
{
    GPIO_PinState feedback_level;

    // Read the feedback pin state
    feedback_level = GPIO_ReadPin(CONTACTOR_FB_GPIO_PORT, CONTACTOR_FB_GPIO_PIN);

    // Determine state based on defined logic level
    // CONTACTOR_FEEDBACK_IS_CLOSED_STATE is 1 (HIGH means closed)
    if ((feedback_level == GPIO_Pin_SET && CONTACTOR_FEEDBACK_IS_CLOSED_STATE == 1) ||
        (feedback_level == GPIO_Pin_RESET && CONTACTOR_FEEDBACK_IS_CLOSED_STATE == 0))
    {
        return CONTACTOR_PHYS_CLOSED;
    }
    else
    {
        return CONTACTOR_PHYS_OPEN;
    }
    // Note: Doesn't handle floating input or unknown state currently.
    // Could add checks for pin configuration or return CONTACTOR_PHYS_UNKNOWN if needed.
}
