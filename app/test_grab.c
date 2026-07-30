// 来源: C:\Users\PC\Documents\图像识别\USER/test_grab.c | 修改: 适配 MSPM0G3507, 无逻辑变更
/**
 *  多点巡回测试
 *
 *  流程:
 *    1. 移动臂到目标坐标
 *    2. 停留 2 秒
 *    3. 移动到下一个目标点
 *
 *  通信协议:
 *    通过设置 cmd_type/target_x/y/z 与主循环 Exec_Cmd() 交互
 */

#include "../app/test_grab.h"
#include "../bsp/bsp_uart.h"
#include "../protocol/cmd_parser.h"
#include <string.h>
#include <stdlib.h>

/* planner_busy 定义已移至 HARDWARE/Motion/motion.c */
/* 仍可通过 test_grab.h 中的 extern 访问 */

/* ================================================================
 *  棋盘坐标预设 (9 个格子)
 *
 *  世界坐标 (原点 = 底座转轴桌面投影, x=前方, y=左侧):
 *    索引对应棋盘位置:
 *      6    7    8      y = +32 (行1, 上)
 *      3    4    5      y = 0   (行2, 中)
 *      0    1    2      y = -32 (行3, 下)
 *    x: 165 / 133 / 101
 * ================================================================ */
static const TestPoint board[9] = {
    /* idx 0 */ {165.0f, -38.0f, 30.0f},
    /* idx 1 */ {133.0f, -38.0f, 30.0f},
    /* idx 2 */ {101.0f, -38.0f, 30.0f},
    /* idx 3 */ {165.0f,   0.0f, 30.0f},
    /* idx 4 */ {133.0f,   0.0f, 30.0f},   /* 中心 */
    /* idx 5 */ {101.0f,   0.0f, 30.0f},
    /* idx 6 */ {165.0f,  38.0f, 30.0f},
    /* idx 7 */ {133.0f,  38.0f, 30.0f},
    /* idx 8 */ {101.0f,  38.0f, 30.0f},
};

/* ================================================================
 *  测试序列配置
 *  可通过 #TSEQ,idx1,idx2,... 自定义
 * ================================================================ */
static uint8    test_seq[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
static uint8    seq_len = 9;
static uint8    seq_idx = 0;

/* 状态机变量 */
static TestState g_state   = TEST_IDLE;
uint8            g_test_active = 0;
static uint32    g_wait_ms = 0;

/* ---- 延时 ---- */
#define TICK_MS        20
#define DWELL_MS       2000   /* 每点停留 2 秒 */

/* ================================================================
 *  内部函数: 发送世界坐标移动指令
 * ================================================================ */
static void SendPosCmd(float x, float y, float z)
{
    __disable_irq();
    target_x = x;
    target_y = y;
    target_z = z;
    cmd_type = 2;   /* #POS 指令类型 */
    planner_busy = 1;  /* 预标记忙, 防止 Tick 在 Exec_Cmd 前误判完成 */
    __enable_irq();
}

/* ================================================================
 *  TestGrab_Init: 初始化测试模块
 * ================================================================ */
void TestGrab_Init(void)
{
    g_state   = TEST_IDLE;
    g_test_active = 0;
    seq_idx   = 0;
    g_wait_ms = 0;
}

/* ================================================================
 *  TestGrab_Start: 启动巡回测试
 *  seq: 棋盘索引数组 (NULL 则使用默认全序列)
 *  len: 数组长度
 * ================================================================ */
void TestGrab_Start(uint8 *seq, uint8 len)
{
    if (seq != 0 && len > 0 && len <= 9) {
        uint8 i;
        for (i = 0; i < len; i++) {
            test_seq[i] = seq[i];
        }
        seq_len = len;
    }

    seq_idx   = 0;
    g_test_active = 1;
    g_state   = TEST_MOVE;

    SendPosCmd(board[test_seq[0]].x,
               board[test_seq[0]].y,
               board[test_seq[0]].z);

    CmdParser_SetResp("TEST START\r\n");
}

/* ================================================================
 *  TestGrab_Tick: 主循环节拍 (每 20ms 调用一次)
 * ================================================================ */
void TestGrab_Tick(void)
{
    if (!g_test_active) return;

    switch (g_state) {

    case TEST_MOVE:
        if (!planner_busy) {
            g_state   = TEST_WAIT;
            g_wait_ms = 0;
        }
        break;

    case TEST_WAIT:
        g_wait_ms += TICK_MS;
        if (g_wait_ms >= DWELL_MS) {
            seq_idx++;
            if (seq_idx >= seq_len) {
                g_state  = TEST_DONE;
                g_test_active = 0;
                CmdParser_SetResp("TEST DONE\r\n");
            } else {
                g_state = TEST_MOVE;
                SendPosCmd(board[test_seq[seq_idx]].x,
                           board[test_seq[seq_idx]].y,
                           board[test_seq[seq_idx]].z);
            }
        }
        break;

    case TEST_DONE:
    case TEST_IDLE:
    default:
        break;
    }
}

/* ================================================================
 *  TestGrab_HandleCmd: 解析测试相关指令 (由 usart.c ISR 调用)
 *
 *  支持的测试指令:
 *    #TEST            → 运行默认 9 点序列
 *    #TSEQ,idx1,...   → 自定义序列 (棋盘索引, 逗号分隔)
 *    #TSTOP           → 停止测试
 * ================================================================ */
void TestGrab_HandleCmd(const char *str)
{
    int idx_list[9];
    int n;
    char  buf[64];                     /* 可变拷贝 (strtok 会修改) */
    char *p;

    /* 拷贝到可变缓冲区 */
    for (n = 0; str[n] && n < (int)sizeof(buf) - 1; n++) {
        buf[n] = str[n];
    }
    buf[n] = '\0';

    /* 去除 \r\n */
    p = strchr(buf, '\r'); if (p) *p = 0;
    p = strchr(buf, '\n'); if (p) *p = 0;

    /* ---- #TEST: 默认全序列 ---- */
    if (strncmp(buf, "#TEST", 5) == 0 && (buf[5] == '\0' || buf[5] == '\r' || buf[5] == '\n')) {
        TestGrab_Start(0, 0);
        return;
    }

    /* ---- #TSEQ,idx1,idx2,... ---- */
    if (strncmp(buf, "#TSEQ,", 6) == 0) {
        n = 0;
        char *tok = strtok(buf + 6, ",");
        while (tok && n < 9) {
            idx_list[n] = atoi(tok);
            if (idx_list[n] < 0) idx_list[n] = 0;
            if (idx_list[n] > 8) idx_list[n] = 8;
            n++;
            tok = strtok(0, ",");
        }
        if (n > 0) {
            TestGrab_Start((uint8 *)idx_list, (uint8)n);
        }
        return;
    }

    /* ---- #TSTOP: 停止测试 ---- */
    if (strncmp(buf, "#TSTOP", 6) == 0) {
        g_test_active = 0;
        g_state  = TEST_IDLE;
        CmdParser_SetResp("TEST STOP\r\n");
        return;
    }

    /* 未知 #T 子命令 */
    CmdParser_SetResp("ERR UNKNOWN\r\n");
}
