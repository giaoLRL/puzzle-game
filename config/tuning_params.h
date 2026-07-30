// ============================================================================
//  tuning_params.h — 机械臂全部可调参数集中定义
//
//  注意: IK 标定偏移和机械尺寸使用原始 STM32 项目实测值
// ============================================================================

#ifndef __TUNING_PARAMS_H
#define __TUNING_PARAMS_H

/* ========== 舵机 PWM 信号范围 (us) ========== */
#define SERVO_MIN            500
#define SERVO_MAX            2490
#define SERVO_MID            1500

/* ========== 上电默认 PWM 值 (1~4: 底座/大臂/小臂/腕部) ========== */
#define HOME_PWM_S1          1600
#define HOME_PWM_S2          2000
#define HOME_PWM_S3          600
#define HOME_PWM_S4          1500
#define HOME_PWM_S5          1500

/* ========== 舵机5 (末端旋转) 角度-PWM 转换 ========== */
#define GRIPPER_ZERO_PWM     1500
#define GRIPPER_ANGLE_SCALE   (800.0f / 90.0f)

/* ========== 机械臂连杆长度 (mm, 实测值) ========== */
#define L1                   105.0f
#define L2                   140.0f
#define SHOULDER_HEIGHT      95.0f

/* ========== IK 标定默认偏移 (PWM us, 原始项目实测) ========== */
#define DEFAULT_BASE_OFFSET      1600
#define DEFAULT_SHOULDER_OFFSET  600
#define DEFAULT_ELBOW_OFFSET     1400
#define DEFAULT_WRIST_OFFSET     1600

/* ========== IK 角度-PWM 转换系数 ========== */
#define BASE_SCALE          413.80f
#define SHOULDER_SCALE      636.6f
#define ELBOW_SCALE         413.9f
#define WRIST_SCALE         636.62f

/* ========== 插补运动参数 ========== */
#define INTERP_MS           20
#define DEFAULT_DURATION_MS 500

/* ========== 电磁铁控制参数 ========== */
#define MAGNET_DWELL_MS         2000
#define MAGNET_RETURN_DWELL_MS  500
#define MAGNET_PICK_RETURN_MS   500

/* ========== 测试序列参数 ========== */
#define TEST_TICK_MS        20
#define TEST_DWELL_MS       2000

/* ========== 通信参数 ========== */
#define UART_BAUDRATE       115200
#define RESP_MSG_MAX        64

#endif
