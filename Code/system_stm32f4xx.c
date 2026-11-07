/*
 * CMSIS system support for the STM32F401.
 *
 * Derived from ST's STM32CubeF4 CMSIS template at
 * Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c.
 */

#include "stm32f4xx.h"

#ifndef HSE_VALUE
#define HSE_VALUE ((uint32_t)25000000U)
#endif

#ifndef HSI_VALUE
#define HSI_VALUE ((uint32_t)16000000U)
#endif

uint32_t SystemCoreClock = HSI_VALUE;

const uint8_t AHBPrescTable[16] = {
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
    1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U
};

void SystemInit(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= (3UL << (10U * 2U)) | (3UL << (11U * 2U));
#endif
    SCB->VTOR = FLASH_BASE;
}

void SystemCoreClockUpdate(void)
{
    uint32_t sysclk_source = RCC->CFGR & RCC_CFGR_SWS;
    uint32_t system_clock;

    switch (sysclk_source) {
    case RCC_CFGR_SWS_HSI:
        system_clock = HSI_VALUE;
        break;

    case RCC_CFGR_SWS_HSE:
        system_clock = HSE_VALUE;
        break;

    case RCC_CFGR_SWS_PLL: {
        uint32_t pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
        uint32_t plln = (RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6U;
        uint32_t pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16U) + 1U) * 2U;
        uint32_t pll_input = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) != 0U)
                                 ? HSE_VALUE
                                 : HSI_VALUE;

        /* A zero PLLM is invalid in hardware, but avoid a divide by zero if
           this function is called while RCC is being configured. */
        system_clock = (pllm == 0U) ? HSI_VALUE : ((pll_input / pllm) * plln) / pllp;
        break;
    }

    default:
        system_clock = HSI_VALUE;
        break;
    }

    SystemCoreClock = system_clock >> AHBPrescTable[(RCC->CFGR & RCC_CFGR_HPRE) >> 4U];
}
