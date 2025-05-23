/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    interrupts.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 CW.
  * All rights reserved.</center></h2>
  *
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "../inc/main.h"
#include "../inc/interrupts_cw32f003.h"
#include "../inc/uart_driver.h" // Include UART driver header
#include "cw32f003_systick.h"   // Include SysTick header
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "../inc/cw32f003_atim.h"
#include "../inc/hlw_uart_driver.h" // Include the HLW UART driver header
/* USER CODE END Includes */


/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
/* USER CODE END TD */


/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */ 
/* USER CODE END PD */


/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */


/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */
extern volatile bool flag_run_state_machine; // Flag defined in main.c
extern volatile bool flag_update_display;    // Flag defined in main.c
// Add extern declarations for other flags if needed
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M0P Processor Interruption and Exception Handlers          */ 
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn */

  /* USER CODE END NonMaskableInt_IRQn */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn */

  /* USER CODE END HardFault_IRQn */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn */
	  
    /* USER CODE END W1_HardFault_IRQn */
  }
}


/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn */

  /* USER CODE END SVCall_IRQn */
}


/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn */

  /* USER CODE END PendSV_IRQn */
}

void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn */
  uwTick++; // Increment the system tick counter (used by HAL_Delay, etc.)

  // Task Scheduling Counters (assuming 1ms SysTick interval)
  static uint32_t counter_10ms = 0;
  static uint32_t counter_100ms = 0;
  // Add other counters here

  // --- 10ms Tasks ---
  counter_10ms++;
  if (counter_10ms >= 10) // Every 10ms
  {
    counter_10ms = 0;
    flag_run_state_machine = true; // Set flag for state machine execution
    // Add other 10ms tasks flags here (e.g., button debouncing)
  }

  // --- 100ms Tasks ---
  counter_100ms++;
  if (counter_100ms >= 100) // Every 100ms
  {
    counter_100ms = 0;
    flag_update_display = true; // Set flag for display update
    // Add other 100ms tasks flags here
  }

  // Add other interval checks here (e.g., 1ms for AC sampling trigger)

  /* USER CODE END SysTick_IRQn */
}

/******************************************************************************/
/* CW32F003 Peripheral Interrupt Handlers                                     */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_cw32f003.s).                     */
/******************************************************************************/

/**
 * @brief This funcation handles WDT
 */
void WDT_IRQHandler(void)
{
  /* USER CODE BEGIN */
	
  /* USER CODE END */
}

/**
 * @brief This funcation handles LVD
 */
void LVD_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}


/**
 * @brief This funcation handles FLASHRAM
 */
void FLASHRAM_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles RCC
 */
void SYSCTRL_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles GPIOA
 */
void GPIOA_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles GPIOB
 */
void GPIOB_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles GPIOC
 */
void GPIOC_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}


/**
 * @brief This funcation handles ADC
 */
void ADC_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles ATIM
 */
void ATIM_IRQHandler(void)
{
	if(ATIM_GetITStatus(ATIM_IT_C2BF) != RESET){
		ATIM_ClearITPendingBit(ATIM_IT_C2BF);
	}
	 // ATIM_IRQHandlerCallBack();

  /* USER CODE END */
}

/**
 * @brief This funcation handles VC1
 */
void VC1_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles VC2
 */
void VC2_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles GTIM
 */
void GTIM_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles BTIM1
 */
void BTIM1_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles BTIM2
 */
void BTIM2_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles BTIM3
 */
void BTIM3_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles I2C
 */
void I2C_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/**
 * @brief This funcation handles SPI
 */
void SPI_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}


/**
 * @brief This funcation handles UART1
 */
void UART1_IRQHandler(void)
{
  /* USER CODE BEGIN UART1_IRQn */

  // Check for Receive Complete interrupt
  if (USART_GetITStatus(CW_UART1, USART_IT_RC) != RESET)
  {
    UART_Driver_Handle_RC(); // Call the receive handler function
    // The flag is cleared inside UART_Driver_Handle_RC after reading data
  }

  // Check for Transmit Empty interrupt
  if (USART_GetITStatus(CW_UART1, USART_IT_TXE) != RESET)
  {
    // Note: TXE flag is cleared automatically when data is written to DR
    UART_Driver_Handle_TXE(); // Call the transmit handler function
    // The TXE interrupt might be disabled inside UART_Driver_Handle_TXE if buffer is empty
  }

  /* USER CODE END UART1_IRQn */
}

/**
 * @brief This funcation handles UART2
 */
void UART2_IRQHandler(void)
{
  /* USER CODE BEGIN UART2_IRQn */

  // Check for Receive Complete interrupt and call handler
  // Note: The handler itself checks the flag again and clears it.
  HLW_UART_Handle_RC();

  /* USER CODE END UART2_IRQn */
}


/**
 * @brief This funcation handles AWT
 */
void AWT_IRQHandler(void)
{
  /* USER CODE BEGIN */

  /* USER CODE END */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/************************ (C) COPYRIGHT CW *****END OF FILE****/
