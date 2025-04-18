#include "main.h"
#include "config.h"
#include "uart_driver.h"
#include "pwm_driver.h"
#include "oled_driver.h"
#include "adc_driver.h"   // Include the new ADC driver header
#include <stdio.h>
#include "system_cw32f003.h" // Include for SystemCoreClock variable

static bool System_Init(void);
static void Display_Status(void);
static void Update_OLED_Display(uint8_t line, const char* str); // Modified to accept line number
static void Error_Handler(void);


int32_t main(void)
{
    if (!System_Init()) {
        
        Error_Handler();
    }

    // Display initial status including clock speed
    Display_Status();

    // Display Clock Speed on Line 0
    char clkStr[20];
    sprintf(clkStr, "Clk: %luMHz", SystemCoreClock / 1000000); // Format clock in MHz
    Update_OLED_Display(4, clkStr);


    while(1) {
        /* Read ADC Voltage and update display */
        uint16_t voltage_mV = ADC_Read_Voltage_mV(); // Read voltage in mV
        char voltStr[20];
        // Format as V: X.XXX V (integer division for volts, modulo for millivolts)
        sprintf(voltStr, "V: %u.%03uV", voltage_mV / 1000, voltage_mV % 1000);
        Update_OLED_Display(2, voltStr); // Display voltage on line 2

        /* Read Internal Temperature and update display */
        float temperature = ADC_Read_Internal_Temperature();
        char tempStr[20];
        // Format as T: XX.XC (requires float support in sprintf)
        sprintf(tempStr, "T: %.1fC", temperature);
        Update_OLED_Display(3, tempStr); // Display temperature on line 3

        FirmwareDelay(3000000); // Delay adjusted for 48MHz clock (was 500000 for 8MHz)
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
    
    /* 显示欢迎信息 */
    OLED_ShowString(0, 0, "Hello CW32!", 6);
    
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

    /* 初始化成功 */
    printf("\r\nCW32F003 Drivers Initialized Successfully\r\n");
    return status;
} // <-- Restore missing closing brace for System_Init

// Restore missing function definitions
/**
 * @brief 显示系统状态信息
 */
static void Display_Status(void)
{
    unsigned long pwmFreq = PWM_Get_Frequency();
    uint8_t pwmDuty = PWM_Get_DutyCycle();
    char strTemp[30];

    printf("PWM Freq: %lu Hz, Duty: %u%%\r\n", pwmFreq, pwmDuty);

    sprintf(strTemp, "Freq:%luHz Duty:%u%%", pwmFreq, pwmDuty);

    Update_OLED_Display(1, strTemp); // Display PWM info on line 1
}

/**
 * @brief 在中断保护的临界区内更新OLED指定行显示
 * @param line 要显示的行 (0-based)
 * @param str 要显示的字符串
 */
static void Update_OLED_Display(uint8_t line, const char* str)
{
    /* 禁用PWM中断 (如果ADC读取或OLED操作与PWM中断冲突) */
    /* 注意: 频繁开关中断可能影响PWM精度，根据实际情况调整 */
    // NVIC_DisableIRQ(PWM_TIMER_IRQn);

    /* 执行OLED显示操作 */
    OLED_ShowString(0, line, (char*)str, 6); // Use specified line

    /* 重新启用PWM中断 */
    // NVIC_EnableIRQ(PWM_TIMER_IRQn);
}


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
