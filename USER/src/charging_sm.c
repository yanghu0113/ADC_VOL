#include "charging_sm.h"
#include "cp_signal.h"
#include "pp_signal.h"
#include "contactor_control.h"
#include "ac_measurement.h" // Included for potential future use in states
#include "ui_display.h"     // To update UI based on state changes
#include "config.h"         // May contain timing definitions etc.

// --- State Machine Variables ---
static SM_State_t current_state = SM_STATE_INIT;
static uint16_t cable_capacity_amps = 0;
static uint8_t max_charging_current_amps = 0;

// --- Initialization ---

/**
 * @brief Initializes the charging state machine and dependent modules.
 */
void SM_Init(void)
{
    // Initialize all related hardware/logic modules
    CP_Signal_Init();
    PP_Signal_Init();
    Contactor_Init();
    AC_Measurement_Init();
    // UI_Display_Init(); // Assuming UI has its own init if needed

    // Set initial state
    current_state = SM_STATE_IDLE; // Start in Idle (State A) after init

    // Set initial CP PWM for State A (+12V, no PWM or 100% duty depending on hw)
    // Assuming 100% duty = +12V for now.
    CP_SetMaxCurrentPWM(0); // Or a specific function CP_SetStateA()?

    // Ensure contactor is open
    Contactor_Open();

    printf("Charging State Machine Initialized. State: IDLE\n");
}

// --- State Machine Execution ---

/**
 * @brief Runs one iteration of the charging state machine logic.
 *        Reads inputs (CP, PP), determines state transitions, and sets outputs (PWM, Contactor).
 */
void SM_RunStateMachine(void)
{
    CP_State_t cp_state = CP_ReadState(); // Read current CP state (simulated for now)
    SM_State_t next_state = current_state; // Assume no change unless transition occurs

    // --- State Logic ---
    switch (current_state)
    {
        case SM_STATE_IDLE: // State A: Waiting for vehicle connection
            if (cp_state == CP_STATE_B_9V || cp_state == CP_STATE_C_6V || cp_state == CP_STATE_D_3V) {
                // Vehicle detected (CP voltage dropped from +12V)
                cable_capacity_amps = PP_GetCableCapacity(); // Read cable capacity (simulated)
                if (cable_capacity_amps == PP_CAPACITY_UNKNOWN) {
                    next_state = SM_STATE_FAULT; // PP Error
                    printf("SM: PP Fault Detected!\n");
                } else {
                    // Determine max current based on cable and EVSE limits (e.g., EVSE limit is 32A)
                    uint8_t evse_limit = 32; // Example EVSE limit
                    max_charging_current_amps = (cable_capacity_amps < evse_limit) ? cable_capacity_amps : evse_limit;

                    // Set PWM according to max allowed current
                    CP_SetMaxCurrentPWM(max_charging_current_amps);
                    printf("SM: Vehicle Connected. Cable: %uA, Max Charge: %uA\n", cable_capacity_amps, max_charging_current_amps);

                    if (cp_state == CP_STATE_B_9V) {
                        next_state = SM_STATE_CONNECTED; // Transition to State B
                    } else if (cp_state == CP_STATE_C_6V) {
                        next_state = SM_STATE_CHARGING_REQ; // Transition directly to State C request
                    } else { // CP_STATE_D_3V
                        // Handle State D if needed, otherwise treat as C or Fault?
                        next_state = SM_STATE_FAULT; // Example: Fault if D is not expected/supported
                        printf("SM: State D detected - treating as Fault.\n");
                    }
                }
            } else if (cp_state == CP_STATE_E_0V || cp_state == CP_STATE_F_NEG_12V) {
                 next_state = SM_STATE_FAULT; // CP Error
                 printf("SM: CP Fault Detected (State E/F)!\n");
            }
            // Ensure contactor remains open in IDLE
            Contactor_Open();
            break;

        case SM_STATE_CONNECTED: // State B: Vehicle connected, waiting for charging request
            if (cp_state == CP_STATE_C_6V) {
                next_state = SM_STATE_CHARGING_REQ; // EV requests charging
                printf("SM: Charging Requested (State C).\n");
            } else if (cp_state == CP_STATE_A_12V) {
                next_state = SM_STATE_IDLE; // Vehicle disconnected
                CP_SetMaxCurrentPWM(0); // Reset PWM (State A)
                printf("SM: Vehicle Disconnected.\n");
            } else if (cp_state == CP_STATE_E_0V || cp_state == CP_STATE_F_NEG_12V) {
                 next_state = SM_STATE_FAULT; // CP Error
                 printf("SM: CP Fault Detected (State E/F)!\n");
            }
            // Ensure contactor remains open
            Contactor_Open();
            break;

        case SM_STATE_CHARGING_REQ: // State C: EV requests charging, prepare to close contactor
            // Add any pre-charge checks if necessary
            Contactor_Close(); // Close the contactor
            next_state = SM_STATE_CHARGING;
            printf("SM: Contactor Closed. Charging Active.\n");
            break;

        case SM_STATE_CHARGING: // State C Active: Power flowing
            if (cp_state == CP_STATE_B_9V) {
                // EV stopped charging request (but still connected)
                Contactor_Open();
                next_state = SM_STATE_CONNECTED;
                printf("SM: Charging Stopped by EV (State B). Contactor Opened.\n");
            } else if (cp_state == CP_STATE_A_12V) {
                // Vehicle disconnected during charging (should not happen ideally)
                Contactor_Open();
                next_state = SM_STATE_IDLE;
                CP_SetMaxCurrentPWM(0); // Reset PWM (State A)
                printf("SM: Vehicle Disconnected during Charging! Contactor Opened.\n");
            } else if (cp_state == CP_STATE_E_0V || cp_state == CP_STATE_F_NEG_12V) {
                 // CP Error during charging
                 Contactor_Open();
                 next_state = SM_STATE_FAULT;
                 printf("SM: CP Fault during Charging (State E/F)! Contactor Opened.\n");
            }
            // Add checks for overcurrent, etc. using AC_GetCurrent() here if needed
            // float current = AC_GetCurrent();
            // if (current > (max_charging_current_amps * 1.1)) { /* Handle overcurrent */ }
            break;

        case SM_STATE_VENTILATION: // State D: Optional state
            // Handle similarly to State C, potentially adjusting current or requiring external action
            // If not supported, transition to FAULT or back to B/C?
             Contactor_Open(); // Example: Open contactor if ventilation not supported
             next_state = SM_STATE_FAULT;
             printf("SM: State D (Ventilation) not supported. Entering Fault.\n");
            break;

        case SM_STATE_FAULT:
            // Ensure contactor is open
            Contactor_Open();
            // Stop PWM or set to specific error state?
            CP_SetMaxCurrentPWM(0); // Example: Set PWM to State A equivalent
            // Log error, wait for reset or specific recovery condition
            // Maybe check if CP goes back to A (12V) to allow transition back to IDLE?
            if (cp_state == CP_STATE_A_12V) {
                 printf("SM: Fault cleared (CP State A detected). Returning to IDLE.\n");
                 next_state = SM_STATE_IDLE;
            }
            break;

        case SM_STATE_INIT: // Should not stay here
        default:
            // Should not happen, force back to IDLE or FAULT
            printf("SM: Invalid State! Forcing to IDLE.\n");
            Contactor_Open();
            CP_SetMaxCurrentPWM(0);
            next_state = SM_STATE_IDLE;
            break;
    }

    // --- Update State ---
    if (next_state != current_state) {
        printf("SM: State Change %d -> %d\n", current_state, next_state);
        current_state = next_state;
        // Update UI display based on the new state
        UI_UpdateDisplay(); // Call the UI update function
    }
}

/**
 * @brief Gets the current state of the state machine.
 * @return SM_State_t The current state.
 */
SM_State_t SM_GetCurrentState(void)
{
    return current_state;
}
