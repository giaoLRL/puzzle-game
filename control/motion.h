// ============================================================================
//  motion.h — 运动控制模块 (插补器 + 电磁铁状态机)
//
//  来源: C:\Users\PC\Documents\图像识别\HARDWARE\Motion\motion.h
//  修改: 适配 MSPM0, 更新 include 路径, MAGNET_ON/OFF 宏改为函数调用
// ============================================================================

#ifndef __MOTION_H
#define __MOTION_H

#include "../config/common.h"

/* ---- 插补忙标志 (供 test_grab 外部读取) ---- */
extern volatile uint8 planner_busy;

/* ---- 电磁铁任务忙锁 (供 cmd_parser 读取) ---- */
extern volatile uint8 magnet_task_lock;

/* ---- 初始化 ---- */
void Motion_Init(void);

/* ---- 查询插补器状态: 1=正在运行, 0=空闲 ---- */
uint8 Motion_IsRunning(void);

/* ---- 20ms 插补 tick (由主循环每 20ms 调用) ---- */
void Motion_PlannerTick(void);

/* ---- 20ms 电磁铁状态机 tick (由主循环每 20ms 调用) ---- */
void Motion_MagnetTick(void);

/* ---- 消费待处理指令 (主循环空闲时调用) ---- */
void Motion_ExecCmd(void);

#endif /* __MOTION_H */
