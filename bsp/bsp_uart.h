// ============================================================================
//  bsp_uart.h — UART 板级支持包 (MSPM0G3507)
//
//  来源: C:\Users\PC\Documents\图像识别\SYSTEM\usart\usart.h
//  修改: 删除 STM32 USART 初始化，改用 SysConfig 生成的 UART2 驱动
//  外设: UART2 (PB15=TX, PB16=RX), 115200-8N1
// ============================================================================

#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "../config/common.h"

/*
 * 指令解析结果 — 由 cmd_parser 填充, main 循环消费
 * 来源: 原 usart.h 中的全局变量
 */
/* 指令类型: 1=PWM 2=POS 3=CAL 4=HOME 5=POSS 6=POSD */
extern volatile uint8  cmd_type;
extern volatile uint16 target_s1, target_s2, target_s3, target_s4, target_s5;
extern volatile float  target_x, target_y, target_z;
extern volatile uint8  target_magnet;
extern volatile float  target_x2, target_y2, target_z2;
extern volatile float  target_rot_angle;
extern volatile uint8  magnet_task_lock;

/* 响应机制 */
extern volatile uint8  resp_ready;
extern char            resp_msg[64];

/* ---- 初始化: SysConfig 已配置 UART2, 此处注册中断回调 ---- */
void BSP_UART_Init(void);

/* ---- 发送字符串 (阻塞) ---- */
void UART_PutStr(const char *str);

/* ---- 发送单字符 (阻塞) ---- */
void UART_PutChar(char ch);

#endif /* __BSP_UART_H */
