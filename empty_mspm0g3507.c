/**
 * 四自由度机械臂控制器（MSPM0G3507）
 * CCS watch: step, tg0_load, tg0_cc0, tg0_ctr, ta1_ctr, stk_ctrl, hb_count, cmd_type
 */

#include "ti_msp_dl_config.h"
#include "config/common.h"
#include "config/tuning_params.h"
#include "bsp/bsp_uart.h"
#include "bsp/bsp_timer.h"
#include "drivers/servo.h"
#include "drivers/magnet.h"
#include "control/motion.h"
#include "protocol/ringbuf.h"
#include "protocol/cmd_parser.h"
#include "app/test_grab.h"

int main(void)
{
    step = 10;
    SYSCFG_DL_init();

    /* 使能 UART2 RX 中断 + NVIC (SysConfig 未自动配置, 不调此句串口收不到数据) */
    BSP_UART_Init();

    step = 11;
    Mag_Init();

    step = 12;
    Servo_Init();   /* 内部 step 1~7 */

    step = 20;
    RingBuf_Init(CmdParser_Parse);

    step = 21;
    Motion_Init();

    step = 22;
    BSP_Timer_Init();
    BSP_Timer_Start();

    stk_load = SysTick->LOAD;
    stk_ctrl = SysTick->CTRL;
    stk_val  = SysTick->VAL;

    step = 30;
    UART_PutStr("Arm Ready\r\n");

    while (1) {
        RingBuf_Poll();

        if (resp_ready) {
            resp_ready = 0;
            UART_PutStr(resp_msg);
        }

        if (flag_planner) {
            flag_planner = 0;
            hb_count = g_tick_count;

            if (Motion_IsRunning()) {
                Motion_PlannerTick();
                Servo_UpdateAll();
            }
            Motion_MagnetTick();
            TestGrab_Tick();
        }

        if (!Motion_IsRunning() && cmd_type != 0) {
            Motion_ExecCmd();
        }
    }
}
