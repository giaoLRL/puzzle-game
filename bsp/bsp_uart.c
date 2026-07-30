// ============================================================================
//  bsp_uart.c — UART 板级支持包 (MSPM0G3507)
//
//  来源: C:\Users\PC\Documents\图像识别\SYSTEM\usart\usart.c
//  修改: 删除 STM32 USART GPIO/NVIC 初始化 (改用 SysConfig),
//        移除 fputc 重定向, 中断回调改为 RingBuf_FeedByte
//  外设: UART2 (PB15=TX, PB16=RX), 115200-8N1, 中断接收
// ============================================================================

#include "bsp_uart.h"
#include "../protocol/ringbuf.h"

/* ---- 全局变量定义 (ISR 与主循环共享, 全部 volatile) ---- */
volatile uint8  cmd_type    = 0;
volatile uint16 target_s1   = 0, target_s2 = 0, target_s3 = 0, target_s4 = 0;
volatile float  target_x    = 0.0f, target_y = 0.0f, target_z = 0.0f;
volatile uint8  target_magnet     = 0;
volatile float  target_x2 = 0.0f, target_y2 = 0.0f, target_z2 = 0.0f;
volatile uint8  magnet_task_lock  = 0;

volatile uint8  resp_ready = 0;
char            resp_msg[64] = {0};

/* ---- UART2 中断服务函数 ---- */
/*
 * SysConfig 自动生成此 ISR 声明在 ti_msp_dl_config.c 中.
 * 此处在项目文件中提供实现, 覆盖弱定义.
 *
 * ISR 职责: 仅读取接收字节并喂入环形队列, 不做解析.
 */
void UART_2_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_2_INST)) {
    case DL_UART_MAIN_IIDX_RX:
        /* 读取接收到的字节, 喂入协议引擎环形队列 */
        {
            uint8_t data = (uint8_t)DL_UART_Main_receiveData(UART_2_INST);
            RingBuf_FeedByte(data);
        }
        break;
    default:
        break;
    }
}

/* ---- 初始化: SysConfig 已配置外设, 此处仅做额外初始化 ---- */
void BSP_UART_Init(void)
{
    /* RX 引脚上拉: 悬空时保持高电平, 防止噪声触发中断风暴 */
    DL_GPIO_setDigitalInternalResistor(GPIO_UART_2_IOMUX_RX, DL_GPIO_RESISTOR_PULL_UP);
    /* 使能 UART2 RX 中断 (SysConfig 未自动配置此步骤) */
    DL_UART_Main_enableInterrupt(UART_2_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);
}

/* ---- 发送字符串 (阻塞) ---- */
void UART_PutStr(const char *str)
{
    if (str == 0) return;
    while (*str) {
        DL_UART_Main_transmitDataBlocking(UART_2_INST, (uint8_t)(*str));
        str++;
    }
}

/* ---- 发送单字符 (阻塞) ---- */
void UART_PutChar(char ch)
{
    DL_UART_Main_transmitDataBlocking(UART_2_INST, (uint8_t)ch);
}
