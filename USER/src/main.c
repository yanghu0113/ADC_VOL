#include "main.h"
#include "config.h"
#include "uart_driver.h"
#include "pwm_driver.h"
#include "oled_driver.h"
#include "adc_driver.h"   // Include the ADC driver header (used by submodules)
#include <stdio.h>
#include "system_cw32f003.h" // Include for SystemCoreClock variable
#include "cw32f003_iwdt.h"   // Include IWDT header
#include "cw32f003_systick.h" // Include SysTick header

// Include new charging gun modules
#include "charging_sm.h"
#include "ui_display.h"
#include "ac_measurement.h" // Include AC measurement header
#include "spi_oled_driver.h" // Include new SPI OLED driver header

static bool System_Init(void);
static void Error_Handler(void);

// --- Global Task Flags (set by SysTick_Handler) ---
volatile bool flag_run_state_machine = false; // Set every 10ms
volatile bool flag_update_display = false;    // Set every 100ms
extern volatile bool hlw8032_packet_ready;    // Flag defined in ac_measurement.c
// Add other flags here as needed


int32_t main(void)
{
    if (!System_Init()) {
        
        Error_Handler();
    }

    // Initialize the Charging State Machine, UI, AC Measurement, and OLED
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
    bool status = true;
    
    //Initialize the correct OLED driver (SPI version)
     if (!OLED_Init()) { // Remove call to old I2C init
         status = false;
         return status; 
     }
    
    if (!UART_Driver_Init(DEBUG_UART_BAUDRATE)) {
        status = false;
        return status; 
    }
    
    if (!PWM_Driver_Init(INITIAL_PWM_FREQ_HZ, INITIAL_PWM_DUTY_PERCENT)) {
        printf("Error: PWM Init Failed!\r\n");
        status = false;
        return status;
    }

    if (!ADC_Driver_Init()) { 
        printf("Error: ADC Driver Init Failed!\r\n");
        status = false;
        return status;
    }

    /* 初始化IWDT (Independent Watchdog Timer - Assuming RC10K source based on example) */
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
    IWDT_Unlock(); // Unlock needed before Init? Example doesn't show it here, but keep for safety? Let's remove based on example.
    IWDT_Init(&IWDT_InitStruct); // Initialize with settings
    // IWDT_Cmd();                  // Start the watchdog counter - MOVED TO main()
    // while (!CW_IWDT->SR_f.RUN);  // Wait until the watchdog is running - MOVED TO main()
    // IWDT_Refresh();              // Perform an initial refresh immediately after starting - MOVED TO main()

    /* 初始化SysTick (1ms tick) */
    InitTick(SystemCoreClock); // Configure SysTick for 1ms interrupts
    printf("SysTick Initialized (1ms tick)\r\n");

    /* 初始化成功 */
    printf("\r\nCW32F003 Drivers Initialized Successfully\r\n");
    printf("IWDT Initialized (Timeout ~500ms)\r\n"); // Updated comment
    return status;
}

// Removed Display_Status and Update_OLED_Display as UI is handled by ui_display module

static void Error_Handler(void)
{
    printf("Error: Peripheral Initialization Failed! Halting.\r\n");

    
    while(1) {
        /* 可以添加系统复位或安全模式切换代码 */
    }
}








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
