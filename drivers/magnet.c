// ============================================================================
//  magnet.c — 电磁铁控制驱动 (MSPM0G3507)
//
//  来源: 从 motion.c 拆出电磁铁 GPIO 操作 | 修改: 适配 MSPM0 DriverLib
//  引脚: PA2 (已在 SysConfig 中配置为 GPIO 推挽输出)
//  安全: 初始化强制输出低电平，防止上电瞬间误吸合
// ============================================================================

#include "magnet.h"

/* ---- 电磁铁 GPIO 引脚常量 (须与 SysConfig 一致) ---- */
#define MAGNET_PORT   GPIOA
#define MAGNET_PIN    DL_GPIO_PIN_2

/* ---- 初始化: 确保电磁铁默认释放 ---- */
void Mag_Init(void)
{
    /*
     * SysConfig 已将 PA2 配置为输出, 此处显式置低
     * 防止上电/复位瞬间电磁铁误吸合.
     */
    DL_GPIO_clearPins(MAGNET_PORT, MAGNET_PIN);
}

/* ---- 吸合 ---- */
void Mag_On(void)
{
    DL_GPIO_setPins(MAGNET_PORT, MAGNET_PIN);
}

/* ---- 释放 ---- */
void Mag_Off(void)
{
    DL_GPIO_clearPins(MAGNET_PORT, MAGNET_PIN);
}
