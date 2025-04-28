#include "charging_sm.h"
#include "cp_signal.h"
#include "pp_signal.h"
#include "contactor_control.h"
#include "ac_measurement.h" // Included for potential future use in states
#include "ui_display.h"     // To update UI based on state changes
#include "config.h"         // May contain timing definitions etc.
#include "error_handler.h"  // Include the error handler
#include "system_cw32f003.h" // For FirmwareDelay
#include <stdio.h>          // Keep for printf

// Delay in approximate milliseconds to wait for contactor to physically switch
// Adjust based on relay specification and testing
#define CONTACTOR_SWITCH_DELAY_MS 100
// Calculate delay count based on SystemCoreClock (assuming FirmwareDelay uses simple loop)
// This is approximate! A SysTick-based delay would be more accurate.
// Assuming 48MHz clock, need rough estimate. Let's use the value from OLED init for ~1ms.
#define CONTACTOR_DELAY_COUNT (4800 * CONTACTOR_SWITCH_DELAY_MS / 1) // Approx count for delay

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
    CP_State_t cp_state;
    SM_State_t next_state = current_state; // Assume no change unless transition occurs
    ErrorCode_t last_error = ErrorHandler_GetLast(); // Check for persistent errors

    // --- Pre-State Machine Error Check ---
    // If a persistent error exists and we are not already handling it in the fault state, force transition to fault.
    if (last_error != ERROR_NONE && current_state != SM_STATE_FAULT) {
        printf("SM: Persistent Error Detected (%d). Forcing FAULT state.\n", last_error);
        next_state = SM_STATE_FAULT;
        // Note: The specific error was already logged by the module that detected it.
    } else {
        // Only read inputs and run state logic if no overriding error exists
        cp_state = CP_ReadState(); // Read current CP state

        // Handle CP Read Fault immediately
        if (cp_state == CP_STATE_FAULT && current_state != SM_STATE_FAULT) {
             printf("SM: CP Fault Detected. Forcing FAULT state.\n");
             next_state = SM_STATE_FAULT;
             // Error already reported by CP_ReadState or ADC_Read_Channel_Raw
        }
    }

    // --- State Logic (only if not already forced to FAULT by checks above) ---
    if (next_state == current_state) // Avoid re-evaluating if already transitioning to fault
    {
        switch (current_state)
    {
        case SM_STATE_IDLE: // State A: Waiting for vehicle connection
            if (cp_state == CP_STATE_B_9V || cp_state == CP_STATE_C_6V || cp_state == CP_STATE_D_3V) {
                // Vehicle detected (CP voltage dropped from +12V)
                cable_capacity_amps = PP_GetCableCapacity();
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
                        // State D handling might need refinement based on requirements
                        ErrorHandler_Handle(ERROR_STATE_INVALID, "SM_Idle", __LINE__); // Report unexpected state D
                        next_state = SM_STATE_FAULT;
                        printf("SM: State D detected - treating as Fault.\n");
                    }
                }
            }
            // CP Fault check moved above the switch statement
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
            }
            // CP Fault check moved above the switch statement
            // Ensure contactor remains open
            Contactor_Open();
            break;

        case SM_STATE_CHARGING_REQ: // State C: EV requests charging, prepare to close contactor
            // Add any pre-charge checks if necessary
            Contactor_Close(); // Command contactor closed
            FirmwareDelay(CONTACTOR_DELAY_COUNT); // Wait for relay to switch

            // Verify contactor closed
            if (Contactor_ReadFeedbackState() == CONTACTOR_PHYS_CLOSED) {
                next_state = SM_STATE_CHARGING;
                printf("SM: Contactor Closed Confirmed. Charging Active.\n");
            } else {
                // Contactor failed to close!
                ErrorHandler_Handle(ERROR_CONTACTOR_FAULT, "SM_ChargingReq", __LINE__);
                Contactor_Open(); // Attempt to ensure it's open
                next_state = SM_STATE_FAULT;
                printf("SM: Contactor Close FAILED! Entering Fault.\n");
            }
            break;

        case SM_STATE_CHARGING: // State C Active: Power flowing
            if (cp_state == CP_STATE_B_9V) {
                // EV stopped charging request (but still connected)
                Contactor_Open(); // Command contactor open
                FirmwareDelay(CONTACTOR_DELAY_COUNT); // Wait for relay to switch

                // Verify contactor opened
                if (Contactor_ReadFeedbackState() == CONTACTOR_PHYS_OPEN) {
                    next_state = SM_STATE_CONNECTED;
                    printf("SM: Charging Stopped by EV (State B). Contactor Opened Confirmed.\n");
                } else {
                    // Contactor failed to open (Welded?)!
                    ErrorHandler_Handle(ERROR_CONTACTOR_FAULT, "SM_Charging_Stop", __LINE__);
                    next_state = SM_STATE_FAULT;
                    printf("SM: Contactor Open FAILED! Entering Fault.\n");
                }
            } else if (cp_state == CP_STATE_A_12V) {
                // Vehicle disconnected during charging (should not happen ideally)
                Contactor_Open(); // Command contactor open
                FirmwareDelay(CONTACTOR_DELAY_COUNT); // Wait for relay to switch

                // Verify contactor opened
                if (Contactor_ReadFeedbackState() == CONTACTOR_PHYS_OPEN) {
                    next_state = SM_STATE_IDLE;
                    CP_SetMaxCurrentPWM(0); // Reset PWM (State A)
                    printf("SM: Vehicle Disconnected during Charging. Contactor Opened Confirmed.\n");
                } else {
                    // Contactor failed to open (Welded?)!
                    ErrorHandler_Handle(ERROR_CONTACTOR_FAULT, "SM_Charging_Disconnect", __LINE__);
                    next_state = SM_STATE_FAULT;
                    printf("SM: Contactor Open FAILED after disconnect! Entering Fault.\n");
                }
            }
            // CP Fault check moved above the switch statement

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

            // Log error (already done by the module that detected it, or by the checks above)
            // Wait for reset or specific recovery condition.
            // Example recovery: Check if CP state returns to A (vehicle disconnected)
            // We need to read CP state again here if we want to check for recovery within the fault state.
            cp_state = CP_ReadState(); // Re-read CP state

            // Check for recovery condition: CP is State A AND Contactor is confirmed Open
            if (cp_state == CP_STATE_A_12V && Contactor_ReadFeedbackState() == CONTACTOR_PHYS_OPEN) {
                 printf("SM: Fault condition cleared (CP State A & Contactor Open). Returning to IDLE.\n");
                 ErrorHandler_ClearLast(); // Clear the stored error code
                 next_state = SM_STATE_IDLE;
            } else if (cp_state == CP_STATE_FAULT) {
                 // If CP read itself faults again, stay in FAULT state (error already logged)
                 // Do nothing, remain in fault state
            }
            // Otherwise (e.g., CP is B/C/D or Contactor feedback still shows closed), remain in FAULT state.
            break;

        case SM_STATE_INIT: // Should not stay here
        default:
            // Should not happen, report error and force back to IDLE or FAULT
            ErrorHandler_Handle(ERROR_STATE_INVALID, "SM_Run", __LINE__);
            printf("SM: Invalid State (%d)! Forcing to IDLE.\n", current_state);
            Contactor_Open();
            CP_SetMaxCurrentPWM(0);
            next_state = SM_STATE_IDLE; // Or SM_STATE_FAULT? IDLE might be safer.
            break;
    }
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
