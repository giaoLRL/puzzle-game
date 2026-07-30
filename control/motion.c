// ============================================================================
//  motion.c — 运动控制模块 (插补器 + 电磁铁状态机)
//
//  来源: C:\Users\PC\Documents\图像识别\HARDWARE\Motion\motion.c
//  修改: 适配 MSPM0 DriverLib,
//        __disable_irq→__disable_interrupts, MAGNET_ON→Mag_On,
//        UART_PutStr(USART2,→UART_PutStr(, printf→UART_PutStr
// ============================================================================

#include "motion.h"
#include "../drivers/servo.h"
#include "../drivers/magnet.h"
#include "../bsp/bsp_uart.h"
#include "../protocol/cmd_parser.h"
#include "ik.h"
#include "../config/tuning_params.h"
#include <stdlib.h>
#include <stdio.h>

/* 注意: magnet_task_lock 定义在 bsp_uart.c 中, 此处通过 bsp_uart.h 的 extern 声明访问 */

/* ================================================================
 *  插补器
 * ================================================================ */

/* 插补器内部数据结构 */
typedef struct {
    uint16 target[SERVO_NUM + 1];  /* 目标 PWM 值 */
    float  accum[SERVO_NUM + 1];   /* 浮点累加器 (避免截断误差) */
    float  inc[SERVO_NUM + 1];     /* 每步增量 */
    uint16 total_steps;            /* 总步数 */
    uint16 current_step;           /* 当前步号 */
    uint8  running;                /* 1 = 正在插补 */
} Planner;

static Planner planner;

/* 插补忙标志: 1 = 正在运动, 0 = 空闲 */
volatile uint8 planner_busy = 0;

/* ---- 前向声明 ---- */
static void Planner_Start(uint16 s1, uint16 s2, uint16 s3, uint16 s4, uint16 s5, uint16 duration_ms);
static uint8 Start_Pos_Move(float x, float y, float z);

/* ================================================================
 *  电磁铁状态机
 * ================================================================ */

typedef enum {
    MAG_IDLE = 0,

    MAG_POSS_MOVE,
    MAG_POSS_WAIT,
    MAG_POSS_ACTION,

    MAG_POSD_MOVE_PICK,
    MAG_POSD_WAIT_PICK,
    MAG_POSD_PICK,
    MAG_POSD_WAIT_AFTER_PICK,
    MAG_POSD_MOVE_PLACE,
    MAG_POSD_WAIT_PLACE,
    MAG_POSD_RELEASE,
    MAG_POSD_WAIT_RETURN,
    MAG_POSD_MOVE_HOME
} MagnetState;

static volatile MagnetState mag_state = MAG_IDLE;
static volatile uint16 mag_timer = 0;
static float mag_pick_x, mag_pick_y, mag_pick_z;
static float mag_place_x, mag_place_y, mag_place_z;
static uint8  mag_pending_magnet;

/* ================================================================
 *  Motion_Init — 初始化 (上电安全: 电磁铁默认释放)
 * ================================================================ */
void Motion_Init(void)
{
    mag_state = MAG_IDLE;
    mag_timer = 0;
    planner_busy = 0;
    magnet_task_lock = 0;
    planner.running = 0;
    Mag_Off();
}

/* ================================================================
 *  Planner_Start — 启动一次插补运动
 * ================================================================ */
static void Planner_Start(uint16 s1, uint16 s2, uint16 s3, uint16 s4, uint16 s5, uint16 duration_ms)
{
    int i;
    uint16 diff[SERVO_NUM + 1], max_diff = 0;
    uint16 target[SERVO_NUM + 1];

    target[1] = s1; target[2] = s2; target[3] = s3; target[4] = s4; target[5] = s5;

    for (i = 1; i <= SERVO_NUM; i++) {
        if (target[i] < SERVO_MIN) target[i] = SERVO_MIN;
        if (target[i] > SERVO_MAX) target[i] = SERVO_MAX;
    }

    for (i = 1; i <= SERVO_NUM; i++) {
        diff[i] = abs((int)target[i] - (int)CPWM[i]);
        if (diff[i] > max_diff) max_diff = diff[i];
    }

    if (max_diff == 0) {
        planner.running = 1;
        planner_busy = 1;
        __disable_irq();
        for (i = 1; i <= SERVO_NUM; i++) {
            CPWM[i] = target[i];
        }
        __enable_irq();
        planner.running = 0;
        planner_busy = 0;
        UART_PutStr("DONE\r\n");
        return;
    }

    if (duration_ms == 0) duration_ms = DEFAULT_DURATION_MS;
    planner.total_steps = duration_ms / INTERP_MS;
    if (planner.total_steps < 1) planner.total_steps = 1;

    for (i = 1; i <= SERVO_NUM; i++) {
        planner.target[i] = target[i];
        planner.inc[i]    = (float)((int)target[i] - (int)CPWM[i])
                          / (float)planner.total_steps;
        planner.accum[i]  = (float)CPWM[i];
    }

    planner.current_step = 0;
    planner.running = 1;
    planner_busy = 1;
}

/* ================================================================
 *  Motion_PlannerTick — 每 20ms 调用, 驱动一次插补步进
 * ================================================================ */
void Motion_PlannerTick(void)
{
    int i;

    planner.current_step++;

    if (planner.current_step >= planner.total_steps) {
        /* 最后一步: 精确写入目标值 */
        __disable_irq();
        for (i = 1; i <= SERVO_NUM; i++) {
            CPWM[i] = planner.target[i];
        }
        __enable_irq();
        planner.running = 0;
        planner_busy = 0;
        UART_PutStr("DONE\r\n");
        return;
    }

    /* 中间步: 累加增量 */
    __disable_irq();
    for (i = 1; i <= SERVO_NUM; i++) {
        planner.accum[i] += planner.inc[i];
        CPWM[i] = (uint16)planner.accum[i];
    }
    __enable_irq();
}

/* ================================================================
 *  Motion_IsRunning — 查询插补器状态
 * ================================================================ */
uint8 Motion_IsRunning(void)
{
    return planner.running;
}

/* ================================================================
 *  Start_Pos_Move — IK 逆解 + 启动插补
 * ================================================================ */
static uint8 Start_Pos_Move(float x, float y, float z)
{
    uint16 ik_s1, ik_s2, ik_s3, ik_s4;
    int ret = IK_Solve(x, y, z, &ik_s1, &ik_s2, &ik_s3, &ik_s4);

    if (ret != 0) {
        UART_PutStr("ERR IK FAIL\r\n");
        return 0;
    }

    /* 调试输出 IK 解算结果 */
    {
        char buf[48];
        snprintf(buf, sizeof(buf), "POS_PWM,%d,%d,%d,%d\r\n",
                 ik_s1, ik_s2, ik_s3, ik_s4);
        UART_PutStr(buf);
    }

    Planner_Start(ik_s1, ik_s2, ik_s3, ik_s4, CPWM[5], DEFAULT_DURATION_MS);
    return 1;
}

/* ================================================================
 *  Motion_ExecCmd — 消费待处理指令
 * ================================================================ */
void Motion_ExecCmd(void)
{
    uint8  type;
    float  x, y, z;
    uint16 s1, s2, s3, s4, s5;
    uint8  mag;
    float  x2, y2, z2;

    __disable_irq();
    type = cmd_type;
    x = target_x; y = target_y; z = target_z;
    s1 = target_s1; s2 = target_s2; s3 = target_s3; s4 = target_s4; s5 = target_s5;
    mag = target_magnet;
    x2 = target_x2; y2 = target_y2; z2 = target_z2;
    cmd_type = 0;
    __enable_irq();

    switch (type) {

    case CMD_PWM: /* #PWM,s1,s2,s3,s4,s5 */
        Planner_Start(s1, s2, s3, s4, s5, DEFAULT_DURATION_MS);
        break;

    case CMD_POS: /* #POS,x,y,z */
        Start_Pos_Move(x, y, z);
        break;

    case CMD_CAL: /* #CAL,b,s,e,w */
        IK_SetCalib(s1, s2, s3, s4);
        UART_PutStr("CAL OK\r\n");
        break;

    case CMD_HMSET: /* #HMSET */
        UART_PutStr("HMSET OK\r\n");
        break;

    case CMD_HOME: /* #HOME */
        Planner_Start(home_pwm[1], home_pwm[2], home_pwm[3], home_pwm[4], home_pwm[5], 1000);
        break;

    case CMD_POSS: /* #POSS,x,y,z,n */
        mag_pick_x = x; mag_pick_y = y; mag_pick_z = z;
        mag_pending_magnet = mag;
        magnet_task_lock = 1;
        if (Start_Pos_Move(x, y, z)) {
            mag_state = MAG_POSS_MOVE;
        } else {
            magnet_task_lock = 0;
            mag_state = MAG_IDLE;
        }
        break;

    case CMD_POSD: /* #POSD,x,y,z,l,m,n */
        mag_pick_x  = x;  mag_pick_y  = y;  mag_pick_z  = z;
        mag_place_x = x2; mag_place_y = y2; mag_place_z = z2;
        magnet_task_lock = 1;
        if (Start_Pos_Move(x, y, z)) {
            mag_state = MAG_POSD_MOVE_PICK;
        } else {
            magnet_task_lock = 0;
            mag_state = MAG_IDLE;
        }
        break;

    case CMD_ROT: /* #ROT,angle */
    {
        uint16 rot_pwm;
        int32_t tmp;
        tmp = (int32_t)((float)GRIPPER_ZERO_PWM + target_rot_angle * GRIPPER_ANGLE_SCALE);
        if (tmp < SERVO_MIN) tmp = SERVO_MIN;
        if (tmp > SERVO_MAX) tmp = SERVO_MAX;
        rot_pwm = (uint16)tmp;
        Planner_Start(CPWM[1], CPWM[2], CPWM[3], CPWM[4], rot_pwm, DEFAULT_DURATION_MS);
    }
        break;

    default:
        break;
    }
}

/* ================================================================
 *  Motion_MagnetTick — 电磁铁控制状态机 (每 20ms 调用)
 * ================================================================ */
void Motion_MagnetTick(void)
{
    if (mag_state == MAG_IDLE) return;

    switch (mag_state) {

    /* ---- #POSS 流程 ---- */

    case MAG_POSS_MOVE:
        if (!planner_busy) {
            UART_PutStr("MAG:POSS_WAIT\r\n");
            mag_timer = MAGNET_DWELL_MS / INTERP_MS;
            mag_state = MAG_POSS_WAIT;
        }
        break;

    case MAG_POSS_WAIT:
        if (mag_timer > 0) {
            mag_timer--;
        } else {
            UART_PutStr("MAG:POSS_ACTION\r\n");
            mag_state = MAG_POSS_ACTION;
        }
        break;

    case MAG_POSS_ACTION:
        if (mag_pending_magnet) {
            UART_PutStr("MAG:ON\r\n");
            Mag_On();
        } else {
            UART_PutStr("MAG:OFF\r\n");
            Mag_Off();
        }
        magnet_task_lock = 0;
        mag_state = MAG_IDLE;
        break;

    /* ---- #POSD 流程 ---- */

    case MAG_POSD_MOVE_PICK:
        if (!planner_busy) {
            UART_PutStr("MAG:D_WAIT_PICK\r\n");
            mag_timer = MAGNET_DWELL_MS / INTERP_MS;
            mag_state = MAG_POSD_WAIT_PICK;
        }
        break;

    case MAG_POSD_WAIT_PICK:
        if (mag_timer > 0) {
            mag_timer--;
        } else {
            UART_PutStr("MAG:D_PICK\r\n");
            mag_state = MAG_POSD_PICK;
        }
        break;

    case MAG_POSD_PICK:
        UART_PutStr("MAG:ON\r\n");
        Mag_On();
        UART_PutStr("MAG:D_WAIT_AFTER_PICK\r\n");
        mag_timer = MAGNET_DWELL_MS / INTERP_MS;
        mag_state = MAG_POSD_WAIT_AFTER_PICK;
        break;

    case MAG_POSD_WAIT_AFTER_PICK:
        if (mag_timer > 0) {
            mag_timer--;
        } else {
            if (Start_Pos_Move(mag_place_x, mag_place_y, mag_place_z)) {
                mag_state = MAG_POSD_MOVE_PLACE;
            } else {
                Mag_Off();
                magnet_task_lock = 0;
                mag_state = MAG_IDLE;
            }
        }
        break;

    case MAG_POSD_MOVE_PLACE:
        if (!planner_busy) {
            UART_PutStr("MAG:D_WAIT_PLACE\r\n");
            mag_timer = MAGNET_DWELL_MS / INTERP_MS;
            mag_state = MAG_POSD_WAIT_PLACE;
        }
        break;

    case MAG_POSD_WAIT_PLACE:
        if (mag_timer > 0) {
            mag_timer--;
        } else {
            UART_PutStr("MAG:D_RELEASE\r\n");
            mag_state = MAG_POSD_RELEASE;
        }
        break;

    case MAG_POSD_RELEASE:
        UART_PutStr("MAG:OFF\r\n");
        Mag_Off();
        UART_PutStr("MAG:D_WAIT_RETURN\r\n");
        mag_timer = MAGNET_RETURN_DWELL_MS / INTERP_MS;
        mag_state = MAG_POSD_WAIT_RETURN;
        break;

    case MAG_POSD_WAIT_RETURN:
        if (mag_timer > 0) {
            mag_timer--;
        } else {
            UART_PutStr("MAG:D_MOVE_HOME\r\n");
            Planner_Start(home_pwm[1], home_pwm[2], home_pwm[3], home_pwm[4], home_pwm[5], 1000);
            mag_state = MAG_POSD_MOVE_HOME;
        }
        break;

    case MAG_POSD_MOVE_HOME:
        if (!planner_busy) {
            magnet_task_lock = 0;
            mag_state = MAG_IDLE;
        }
        break;

    default:
        break;
    }
}
