/******************************************************************************
* Include files                                                              *
******************************************************************************/
#include "base_types.h"
#include "cw32f003.h"
#include "system_cw32f003.h"
#include "cw32f003_rcc.h"
#include "cw32f003_flash.h" // Include Flash header for latency setting


/******************************************************************************
 ** System Clock Frequency (Core Clock) Variable according CMSIS
 ******************************************************************************/

uint32_t SystemCoreClock = 48000000; // Set default to max expected speed

void SystemCoreClockUpdate(void) // Update SystemCoreClock variable
{
    uint32_t Hclk = 0;
    uint8_t HclkDiv = 0;
    // uint8_t PclkDiv = 0; // Removed unused variable
    uint8_t SysClkSource = 0;

    // Read the System Clock Source bits (assuming bits 0 and 1 based on RCC_SYSCLKSRC_xxx values)
    SysClkSource = (CW_SYSCTRL->CR0 & 0x03U); // Mask for bits 0 and 1

    switch (SysClkSource)
    {
        case RCC_SYSCLKSRC_HSI: // HSI Used as System Clock
            // HSI frequency depends on HSIDIV setting, assume 48MHz if DIV1 is selected
            // This simplified version assumes HSI is always 48MHz when selected.
            // A more robust version would read HSIDIV.
            Hclk = HSIOSC_VALUE;
            break;
        case RCC_SYSCLKSRC_HEX: // HEX Used as System Clock
            Hclk = HEX_VALUE;
            break;
        case RCC_SYSCLKSRC_LSI: // LSI Used as System Clock
            Hclk = LSI_VALUE;
            break;
        default: // HSI Used as System Clock
            Hclk = HSIOSC_VALUE;
            break;
    }

    // Get HCLK prescaler
    // Assuming HCLKPRS bits are bits 4-6 based on RCC header needing 3 bits (0-7)
    // SYSCTRL_CR0_HCLKPRS_Pos = 4 (Assumed)
    // SYSCTRL_CR0_HCLKPRS_Msk = (0x7UL << 4) (Assumed)
    HclkDiv = (CW_SYSCTRL->CR0 & (0x7UL << 4)) >> 4; // Read bits 4-6
    // Calculate SystemCoreClock
    SystemCoreClock = Hclk >> HclkDiv; // HCLK prescaler is 2^HclkDiv (except for 0 -> div1)
    if (HclkDiv == 0) {
         SystemCoreClock = Hclk; // Div by 1
    } else {
         SystemCoreClock = Hclk >> HclkDiv; // Div by 2, 4, 8 etc.
    }
}

/**
 ******************************************************************************
 ** \brief  Setup the microcontroller system. Initialize the System and update
 ** the SystemCoreClock variable.
 **
 ** \param  none
 ** \return none
 ******************************************************************************/
void SystemInit(void)
{
    // 1. Set Flash Latency for 48MHz operation
    FLASH_SetLatency(FLASH_Latency_2);

    // 2. Enable HSI at 48MHz (DIV1)
    RCC_HSI_Enable(RCC_HSIOSC_DIV1);

    // 3. Wait for HSI stable
    while(RCC_GetStableFlag(RCC_FLAG_HSISTABLE) == RESET);

    // 4. Switch System Clock source to HSI (now 48MHz)
    RCC_SysClk_Switch(RCC_SYSCLKSRC_HSI);

    // 5. Update SystemCoreClock variable
    SystemCoreClockUpdate();

    // Load Trim Codes (original code)
    CW_SYSCTRL->HSI_f.TRIM = *((volatile uint16_t *)RCC_HSI_TRIMCODEADDR);
    CW_SYSCTRL->LSI_f.TRIM = *((volatile uint16_t *)RCC_LSI_TRIMCODEADDR);

    // Init Hide thing (original code)
    // ...
}

#if 0
#if defined (__CC_ARM)
extern int32_t $Super$$main(void);
/* re-define main function */
int $Sub$$main(void)
{
    SystemInit();
    $Super$$main();
    return 0;
}
#elif defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
extern int32_t $Super$$main(void);
/* re-define main function */
int $Sub$$main(void)
{
    SystemInit();
    $Super$$main();
    return 0;
}
#elif defined(__ICCARM__)
extern int32_t main(void);
/* __low_level_init will auto called by IAR cstartup */
extern void __iar_data_init3(void);
int __low_level_init(void)
{
    // call IAR table copy function.
    __iar_data_init3();
    SystemInit();
    main();
    return 0;
}
#endif

#endif


/******************************************************************************
* Internal Funcation                                                          *
******************************************************************************/

///**
// * \brief   delay1ms
// *          delay approximately 1ms.
// * \param   [in]  u32Cnt
// * \retval  void
// */
//void delay1ms(uint32_t u32Cnt)
//{
//    uint32_t u32end;
//    
//    SysTick->LOAD = 0xFFFFFF;
//    SysTick->VAL  = 0;
//    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
//    
//    while(u32Cnt-- > 0)
//    {
//        SysTick->VAL  = 0;
//        u32end = 0x1000000 - SystemCoreClock/1000;
//        while(SysTick->VAL > u32end)
//        {
//            ;
//        }
//    }
//    
//    SysTick->CTRL = (SysTick->CTRL & (~SysTick_CTRL_ENABLE_Msk));
//}


///**
// * \brief   delay100us
// *          delay approximately 100us.
// * \param   [in]  u32Cnt
// * \retval  void
// */
//void delay100us(uint32_t u32Cnt)
//{
//    uint32_t u32end;
//    
//    SysTick->LOAD = 0xFFFFFF;
//    SysTick->VAL  = 0;
//    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
//    
//    while(u32Cnt-- > 0)
//    {
//        SysTick->VAL = 0;

//        u32end = 0x1000000 - SystemCoreClock/10000;
//        while(SysTick->VAL > u32end)
//        {
//            ;
//        }
//    }
//    
//    SysTick->CTRL = (SysTick->CTRL & (~SysTick_CTRL_ENABLE_Msk));
//}


///**
// * \brief   delay10us
// *          delay approximately 10us.
// * \param   [in]  u32Cnt
// * \retval  void
// */
//void delay10us(uint32_t u32Cnt)
//{
//    uint32_t u32end;
//    
//    SysTick->LOAD = 0xFFFFFF;
//    SysTick->VAL  = 0;
//    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
//    
//    while(u32Cnt-- > 0)
//    {
//        SysTick->VAL = 0;

//        u32end = 0x1000000 - SystemCoreClock/100000;
//        while(SysTick->VAL > u32end)
//        {
//            ;
//        }
//    }
//    
//    SysTick->CTRL = (SysTick->CTRL & (~SysTick_CTRL_ENABLE_Msk));
//}


void FirmwareDelay(uint32_t DlyCnt)
{
	volatile uint32_t thisCnt = DlyCnt;
	while( thisCnt-- )
	{
		;
	}
}

/**
 * \brief   clear memory
 *          
 * \param   [in]  start addr
 * \param   [in]  memory size(byte)
 * \retval  void
 */
void MemClr(void *pu8Address, uint32_t u32Count)
{
    uint8_t *pu8Addr = (uint8_t *)pu8Address;
    
    if(NULL == pu8Addr)
    {
        return;
    }
    
    while (u32Count--)
    {
        *pu8Addr++ = 0;
    }
}
