#include "main.h"
#include "config.h"
#include "uart_driver.h"
#include "pwm_driver.h"
#include "adc_driver.h"   // Include the ADC driver header (used by submodules)
#include <stdio.h>
#include "system_cw32f003.h" // Include for SystemCoreClock variable
#include "cw32f003_iwdt.h"   // Include IWDT header
#include "cw32f003_systick.h" // Include SysTick header
#include "error_handler.h"   // Include the new error handler

// Include new charging gun modules
#include "charging_sm.h"
#include "ui_display.h"
#include "ac_measurement.h" // Include AC measurement header
#include "spi_oled_driver.h" // Include new SPI OLED driver header

static bool System_Init(void);

// --- Global Task Flags (set by SysTick_Handler) ---
volatile bool flag_run_state_machine = false; // Set every 10ms
volatile bool flag_update_display = false;    // Set every 100ms
extern volatile bool hlw8032_packet_ready;    // Flag defined in ac_measurement.c
// Add other flags here as needed


int32_t main(void)
{
    // Attempt system initialization
    if (!System_Init()) {
        // System_Init already called ErrorHandler_Handle for specific failures.
        // ErrorHandler_Handle might have already halted if the error was critical.
        // If execution reaches here, it means init failed but wasn't deemed
        // immediately fatal by the handler. We still should not proceed.
        printf("System Initialization failed. Halting.\r\n");
        // Optional: Add specific LED blink pattern here for init failure
        while(1) {} // Halt
    }

    // --- Initialization of Application Modules ---
    // These should ideally also report errors via ErrorHandler_Handle if they fail
    SM_Init();
    UI_Display_Init();     
    AC_Measurement_Init(); // Initialize HLW8032 communication
    OLED_Init();       

    // Start Watchdog *after* all initialization is complete
    IWDT_Cmd();                  // Start the watchdog counter
    while (!CW_IWDT->SR_f.RUN);  // Wait until the watchdog is running
    IWDT_Refresh();              // Perform an initial refresh immediately after starting

    UI_UpdateDisplay(); // Display initial state

    while(1) {
    
        // --- Time-Sliced Tasks ---

        // Run State Machine (every 10ms approx)
        if (flag_run_state_machine) {
            flag_run_state_machine = false; // Clear flag
            SM_RunStateMachine();
        }

        // Update Display (every 100ms approx)
        if (flag_update_display) {
            flag_update_display = false; // Clear flag
            // Decide if UI_UpdateDisplay() should be called here periodically,
            // or only when state actually changes (as currently done in SM_RunStateMachine).
            // For now, let's keep the update within SM_RunStateMachine on state change,
            // but this flag could be used for other periodic UI updates (e.g., blinking icons).
            // UI_UpdateDisplay(); // Example: Call here for periodic updates
        }

        // Process HLW8032 Packet when ready
        if (hlw8032_packet_ready) {
            // Flag is cleared within AC_Process_HLW8032_Packet after processing
             AC_Process_HLW8032_Packet();
             // Note: AC_Process_HLW8032_Packet clears the flag itself
        }

        // Add checks for other flags here...


        // --- Background Tasks ---

        // Refresh the watchdog periodically
        // Consider moving this into a timed task if precise timing is needed,
        // but refreshing it frequently in the main loop is usually safe.
        IWDT_Refresh();

        // Optional: Enter low-power sleep mode if no flags are pending
        // __WFI(); // Example: Wait For Interrupt instruction

    }
}

/**
 * @brief 系统初始化函数
 * @return true: 初始化成功, false: 初始化失败
 */
static bool System_Init(void)
{
    bool overall_status = true; // Track overall success

    // Initialize the correct OLED driver (SPI version)
    // Assuming OLED_Init returns bool and handles SPI init internally
    if (!OLED_Init()) {
        ErrorHandler_Handle(ERROR_OLED_INIT_FAILED, "System_Init", __LINE__);
        overall_status = false;
        // Decide whether to continue or return immediately based on severity
        // For now, let's continue to report all init errors
    }

    // Initialize Debug UART (UART1)
    if (!UART_Driver_Init(DEBUG_UART_BAUDRATE)) {
        // UART_Driver_Init might call ErrorHandler_Handle itself if modified,
        // but we call it here to ensure it's reported at this level.
        ErrorHandler_Handle(ERROR_UART1_INIT_FAILED, "System_Init", __LINE__);
        overall_status = false;
        // UART1 is critical for debugging, consider returning false immediately?
        // return false; // Example: Halt on critical UART failure
    }

    // Initialize PWM Driver
    if (!PWM_Driver_Init(INITIAL_PWM_FREQ_HZ, INITIAL_PWM_DUTY_PERCENT)) {
        ErrorHandler_Handle(ERROR_PWM_INIT_FAILED, "System_Init", __LINE__);
        overall_status = false;
    }

    // Initialize ADC Driver
    if (!ADC_Driver_Init()) {
        ErrorHandler_Handle(ERROR_ADC_INIT_FAILED, "System_Init", __LINE__);
        overall_status = false;
    }

    /* Initialize IWDT (Independent Watchdog Timer) */
    IWDT_InitTypeDef IWDT_InitStruct;
    RCC_APBPeriphClk_Enable1(RCC_APB1_PERIPH_IWDT, ENABLE); // Enable IWDT peripheral clock

    // Configure IWDT for a ~500ms timeout
    IWDT_InitStruct.IWDT_Prescaler = IWDT_Prescaler_DIV32;      // Prescaler = 32 (Clock = 10kHz / 32 = 312.5Hz)
    IWDT_InitStruct.IWDT_ReloadValue = 155;                     // Reload value (Timeout = (155+1) / 312.5Hz = 0.4992s)
    IWDT_InitStruct.IWDT_OverFlowAction = IWDT_OVERFLOW_ACTION_RESET; // Reset on overflow
    IWDT_InitStruct.IWDT_ITState = DISABLE;                     // Interrupt disabled
    IWDT_InitStruct.IWDT_WindowValue = 0xFFF;                   // Window disabled
    IWDT_InitStruct.IWDT_Pause = IWDT_SLEEP_CONTINUE;           // Continue in sleep modes (Adjust if needed)

    // Initialization sequence based on example (Configure only, don't start yet)
    // IWDT_Unlock(); // Unlock seems not needed before Init based on examples
    // IWDT_Init returns void, so we cannot check status directly. Assume success for now.
    IWDT_Init(&IWDT_InitStruct);
    // ErrorHandler_Handle(ERROR_IWDT_INIT_FAILED, "System_Init", __LINE__); // Removed as we can't check status
    // IWDT_Cmd();                  // Start the watchdog counter - MOVED TO main() after all init
    // while (!CW_IWDT->SR_f.RUN);  // Wait until the watchdog is running - MOVED TO main()
    // IWDT_Refresh();              // Perform an initial refresh immediately after starting - MOVED TO main()

    /* Initialize SysTick (1ms tick) */
    // Assuming InitTick doesn't return a status easily, but it's critical.
    // If SysTick fails, ErrorHandler_Handle might not even work if called later.
    InitTick(SystemCoreClock);
    // We might assume SysTick init works, or add a basic check if possible.
    // A simple check could be if uwTick starts incrementing, but that's complex here.
    // For now, assume it works, but note it's a potential point of silent failure.
    // ErrorHandler_Handle(ERROR_SYSTICK_INIT_FAILED, "System_Init", __LINE__); // Example if check added

    // Report overall success/failure
    if (overall_status) {
        printf("\r\nCW32F003 Core System Initialized Successfully\r\n");
        printf("IWDT Configured (Timeout ~500ms)\r\n");
        printf("SysTick Initialized (1ms tick)\r\n");
    } else {
        printf("\r\nCW32F003 Core System Initialization encountered errors!\r\n");
        // ErrorHandler_Handle was already called for specific errors.
    }

    return overall_status;
}

// Removed Display_Status and Update_OLED_Display as UI is handled by ui_display module

// Removed static void Error_Handler(void) as its functionality is replaced by ErrorHandler_Handle
// and the halt loop in main().








/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
