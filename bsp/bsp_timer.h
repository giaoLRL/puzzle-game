// ============================================================================
//  bsp_timer.h — 20ms 节拍定时器 (MSPM0G3507)
// ============================================================================

#ifndef __BSP_TIMER_H
#define __BSP_TIMER_H

#include "../config/common.h"

extern volatile uint8  flag_planner;
extern volatile uint32 g_tick_count;

void BSP_Timer_Init(void);
void BSP_Timer_Start(void);

#endif
