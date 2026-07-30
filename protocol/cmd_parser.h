// ============================================================================
//  cmd_parser.h — 串口指令解析器
//
//  来源: 从 usart.c 拆分 ParseCmd / SetResp | 修改: 适配 MSPM0, 与 UART 驱动解耦
// ============================================================================

#ifndef __CMD_PARSER_H
#define __CMD_PARSER_H

#include "../config/common.h"

/* ---- 指令类型 ---- */
#define CMD_NONE    0
#define CMD_PWM     1   /* #PWM,s1,s2,s3,s4     直接 PWM */
#define CMD_POS     2   /* #POS,x,y,z           世界坐标 */
#define CMD_CAL     3   /* #CAL,b,s,e,w         IK 标定 */
#define CMD_HOME    4   /* #HOME               归中位 */
#define CMD_POSS    5   /* #POSS,x,y,z,n        到位+吸放 */
#define CMD_POSD    6   /* #POSD,x,y,z,l,m,n    取放复合 */
#define CMD_HMSET   7   /* #HMSET,s1,s2,s3,s4,s5 设置归位 */
#define CMD_ROT     8   /* #ROT,angle           舵机5旋转角度 */

/* ---- 解析一帧指令 (由 RingBuf_Poll 回调) ---- */
void CmdParser_Parse(const char *str);

/* ---- 设置响应消息 (ISR 安全, 仅设标志不阻塞发送) ---- */
void CmdParser_SetResp(const char *msg);

#endif /* __CMD_PARSER_H */
