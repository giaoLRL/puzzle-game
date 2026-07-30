// 来源: C:\Users\PC\Documents\图像识别\HARDWARE\Common\common.h | 修改: 适配 MSPM0G3507 DriverLib
// ============================================================================
//  common.h — 基础类型定义 + 全局常量
// ============================================================================

#ifndef __COMMON_H
#define __COMMON_H

#include "ti_msp_dl_config.h"
#include "tuning_params.h"

/* ========== 基础类型 ========== */
typedef signed char     int8;
typedef signed short    int16;
typedef signed int      int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

/* ========== 舵机数量 ========== */
#define SERVO_NUM    4

/* ========== 电磁铁 GPIO 宏 — 改为调用 drivers/magnet.h 中的函数 ========== */
/* 注意: 不再直接用 GPIO 宏，统一通过 Mag_On() / Mag_Off() 操作 */

#endif /* __COMMON_H */
