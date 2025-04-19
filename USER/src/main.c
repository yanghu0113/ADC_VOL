#include "main.h"
#include "config.h"
#include "uart_driver.h"
#include "pwm_driver.h"
#include "oled_driver.h"
#include "adc_driver.h"   // Include the ADC driver header (used by submodules)
#include <stdio.h>
#include "system_cw32f003.h" // Include for SystemCoreClock variable
#include "cw32f003_iwdt.h"   // Include IWDT header

// Include new charging gun modules
#include "charging_sm.h"
#include "ui_display.h"

static bool System_Init(void);
static void Error_Handler(void);


int32_t main(void)
{
    if (!System_Init()) {
        
        Error_Handler();
    }

    // Initialize the Charging State Machine and UI
    SM_Init();
    UI_Display_Init(); // Initialize the UI display

    // Start Watchdog *after* all initialization is complete
    IWDT_Cmd();                  // Start the watchdog counter
    while (!CW_IWDT->SR_f.RUN);  // Wait until the watchdog is running
    IWDT_Refresh();              // Perform an initial refresh immediately after starting

    while(1) {
    
        // Run the charging state machine logic
        SM_RunStateMachine();

        // Update the display based on the current state
        UI_UpdateDisplay();

        // Refresh the watchdog
        IWDT_Refresh();

        // Add a small delay to prevent excessive polling/CPU usage
        // Adjust as needed for responsiveness vs power consumption
        FirmwareDelay(100000); // Approx 2ms delay at 48MHz
    }
}

/**
 * @brief 系统初始化函数
 * @return true: 初始化成功, false: 初始化失败
 */
static bool System_Init(void)
{
    bool status = true;
    
    /* 初始化OLED显示屏 */
    if (!OLED_Init()) {
        status = false;
        return status; /* 提前返回，因为OLED无法显示欢迎信息 */
    }
    
    /* 初始化UART通信 */
    if (!UART_Driver_Init(DEBUG_UART_BAUDRATE)) {
        status = false;
        return status; 
    }
    
    /* 初始化PWM */
    if (!PWM_Driver_Init(INITIAL_PWM_FREQ_HZ, INITIAL_PWM_DUTY_PERCENT)) {
        printf("Error: PWM Init Failed!\r\n");
        status = false;
        return status;
    }

    /* 初始化ADC (using the new driver) */
    if (!ADC_Driver_Init()) { // Call the driver's init function
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
