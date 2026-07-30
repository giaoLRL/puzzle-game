// 来源: C:\Users\PC\Documents\图像识别\HARDWARE/RingBuf/ringbuf.h | 修改: 适配 MSPM0G3507, 无逻辑变更
#ifndef _RINGBUF_H_
#define _RINGBUF_H_

#include "../config/common.h"

/**
 *  串口协议引擎（环形队列 + 帧切分 + 回调解析）
 *
 *  设计:
 *    - ISR 只管喂字节: RingBuf_FeedByte(ch)
 *    - 帧边界由 RINGBUF_DELIMITER 判定
 *    - 完整帧自动入队, 主循环消费时回调用户解析函数
 *    - 解析逻辑与缓冲区完全解耦, 换协议只改回调
 *
 *  配置:
 *    修改下方 RINGBUF_* 宏即可, 无需动 .c
 */

/* ========== 可配置参数 ========== */

#define RINGBUF_FRAME_LEN   64      /* 单帧最大字节数 (含 '\0') */
#define RINGBUF_CAPACITY     8      /* 环形队列缓存帧数 */
#define RINGBUF_DELIMITER   '\n'    /* 帧结束符; '\r' 自动丢弃以兼容 Windows/Linux */

/* ========== 回调类型 ========== */

/**
 *  帧处理回调（主循环上下文调用, 不在中断里）
 *  @param frame  以 '\0' 结尾的完整帧字符串
 */
typedef void (*RingBuf_FrameHandler)(const char *frame);

/* ========== 对外接口 ========== */

void RingBuf_Init(RingBuf_FrameHandler handler); /* 注册回调 */
void RingBuf_FeedByte(uint8 ch);                 /* ISR 调用: 喂入一个字节   */
void RingBuf_Poll(void);                         /* 主循环调用: 消费一帧      */
uint8 RingBuf_Count(void);                       /* 当前缓存的帧数            */
uint8 RingBuf_IsFull(void);                      /* 队列满返回 1             */

#endif
