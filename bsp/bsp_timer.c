// ============================================================================
//  bsp_timer.c — 20ms 节拍定时器 (MSPM0G3507)
//  手动配置 Cortex-M0+ SysTick, 不依赖 SysConfig
// ============================================================================

#include "bsp_timer.h"
#include "ti_msp_dl_config.h"

volatile uint8  flag_planner = 0;
volatile uint32 g_tick_count = 0;

void SysTick_Handler(void)
{
    flag_planner = 1;
    g_tick_count++;
}

void BSP_Timer_Init(void)
{
    flag_planner = 0;
    g_tick_count = 0;

    /* CPUCLK = 32MHz, 20ms = 32M / 50 = 640000, LOAD = 640000 - 1 */
    SysTick->LOAD = 639999;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;
}

void BSP_Timer_Start(void)
{
    /* already started in BSP_Timer_Init */
}
