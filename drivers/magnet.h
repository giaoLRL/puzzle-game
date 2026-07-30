// ============================================================================
//  magnet.h — 电磁铁控制驱动 (MSPM0G3507)
//
//  来源: 从 motion.c 拆出电磁铁 GPIO 操作，适配 MSPM0 DriverLib
// ============================================================================

#ifndef __MAGNET_H
#define __MAGNET_H

#include "../config/common.h"

/* ---- 初始化电磁铁 GPIO (输出低电平, 上电安全) ---- */
void Mag_Init(void);

/* ---- 吸合电磁铁 ---- */
void Mag_On(void);

/* ---- 释放电磁铁 ---- */
void Mag_Off(void);

#endif /* __MAGNET_H */
