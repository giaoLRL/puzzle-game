// 来源: C:\Users\PC\Documents\图像识别\HARDWARE/Arm/ik.h | 修改: 适配 MSPM0G3507, 移除重复宏定义
#ifndef _IK_H_
#define _IK_H_

#include "../config/common.h"

/*
 * 四自由度机械臂逆运动学（含腕部俯仰补偿）
 *
 * 几何模型:
 *         z ↑
 *           │
 *    shoulder●──── L1(大臂) ────● elbow
 *           │                    \
 *           │                     L2(小臂)
 *           │                       \
 *           │                        ● wrist ── 末端吸盘(垂直向下)
 *    ───────┼────────────────────────● end-effector (x,y,z)
 *           │ base
 *   ←────────────────── r = sqrt(x²+y²) ──────→
 *
 *  俯视图:
 *         y ↑
 *           │
 *           │    target(x,y)
 *           │    /
 *           │   /
 *           │  / θ1 = atan2(y,x)
 *           │ /
 *     base  ●──────────→ x
 *
 *  腕关节 θ4 由 IK 自动求解:
 *     实际公式: θ4 = -π/2 - (θ2 + θ3)
 *     (-π/2 垂直向下偏移已通过 wrist_offset 标定值吸收)
 */

/* 注意: 标定偏移和机械参数统一在 config/tuning_params.h 中定义 */

typedef struct {
    uint16 base_offset;
    uint16 shoulder_offset;
    uint16 elbow_offset;
    uint16 wrist_offset;
} IK_Calib;

int IK_Solve(float x, float y, float z,
             uint16 *s1, uint16 *s2, uint16 *s3, uint16 *s4);

void IK_SetCalib(uint16 base, uint16 shoulder, uint16 elbow, uint16 wrist);

uint16 IK_RadToPWM(float rad);

#endif
