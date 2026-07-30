// ============================================================================
//  cmd_parser.c — 串口指令解析器 (MSPM0G3507)
// ============================================================================

#include "cmd_parser.h"
#include "../bsp/bsp_uart.h"
#include "../app/test_grab.h"
#include "../drivers/servo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern volatile uint8 magnet_task_lock;
extern uint8 g_test_active;

void CmdParser_SetResp(const char *msg)
{
    uint8 i = 0;
    while (msg[i] && i < sizeof(resp_msg) - 1) {
        resp_msg[i] = msg[i];
        i++;
    }
    resp_msg[i] = '\0';
    resp_ready = 1;
}

void CmdParser_Parse(const char *str)
{
    int a, b, c, d;
    int n;

    char buf[64];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *p = strchr(buf, '\r');
    if (p) *p = 0;
    p = strchr(buf, '\n');
    if (p) *p = 0;

    /* ---- #DBG — CCS 变量监控用 (无串口输出, 仅用于设置断点) ---- */
    if (strncmp(buf, "#DBG", 4) == 0) {
        CmdParser_SetResp("OK\r\n");
        return;
    }

    /* 电磁铁任务运行中, 拒绝新运动指令 (含 #T 测试指令) */
    if (magnet_task_lock &&
        (strncmp(buf, "#T", 2) == 0 ||
         strncmp(buf, "#PWM", 4) == 0 ||
         strncmp(buf, "#POS", 4) == 0 ||
         strncmp(buf, "#POSS", 5) == 0 ||
         strncmp(buf, "#POSD", 5) == 0 ||
         strncmp(buf, "#HOME", 5) == 0 ||
         strncmp(buf, "#CAL", 4) == 0 ||
         strncmp(buf, "#HMSET", 6) == 0 ||
         strncmp(buf, "#ROT", 4) == 0)) {
        CmdParser_SetResp("ERR BUSY\r\n");
        return;
    }

    if (g_test_active &&
        strncmp(buf, "#T", 2) != 0 &&
        (strncmp(buf, "#PWM", 4) == 0 ||
         strncmp(buf, "#POS", 4) == 0 ||
         strncmp(buf, "#POSS", 5) == 0 ||
         strncmp(buf, "#POSD", 5) == 0 ||
         strncmp(buf, "#HOME", 5) == 0 ||
         strncmp(buf, "#CAL", 4) == 0 ||
         strncmp(buf, "#HMSET", 6) == 0 ||
         strncmp(buf, "#ROT", 4) == 0)) {
        CmdParser_SetResp("ERR BUSY\r\n");
        return;
    }

    /* --- #PWM,s1,s2,s3,s4,s5 --- */
    if (strncmp(buf, "#PWM,", 5) == 0) {
        int e = 1500;
        n = sscanf(buf, "#PWM,%d,%d,%d,%d,%d", &a, &b, &c, &d, &e);
        if (n < 4) {
            CmdParser_SetResp("ERR FORMAT\r\n");
            return;
        }
        if (a < SERVO_MIN || a > SERVO_MAX || b < SERVO_MIN || b > SERVO_MAX ||
            c < SERVO_MIN || c > SERVO_MAX || d < SERVO_MIN || d > SERVO_MAX) {
            CmdParser_SetResp("ERR RANGE\r\n");
            return;
        }
        target_s1 = (uint16)a;
        target_s2 = (uint16)b;
        target_s3 = (uint16)c;
        target_s4 = (uint16)d;
        target_s5 = (uint16)e;
        cmd_type = CMD_PWM;
        CmdParser_SetResp("OK PWM\r\n");
        return;
    }

    /* --- #POS,x,y,z --- */
    if (strncmp(buf, "#POS,", 5) == 0) {
        n = sscanf(buf, "#POS,%f,%f,%f", &target_x, &target_y, &target_z);
        if (n == 3) {
            cmd_type = CMD_POS;
            CmdParser_SetResp("OK POS\r\n");
        } else {
            CmdParser_SetResp("ERR FORMAT\r\n");
        }
        return;
    }

    /* --- #POSS,x,y,z,n --- */
    if (strncmp(buf, "#POSS,", 6) == 0) {
        int m;
        n = sscanf(buf, "#POSS,%f,%f,%f,%d",
                   &target_x, &target_y, &target_z, &m);
        if (n != 4 || (m != 0 && m != 1)) {
            CmdParser_SetResp("ERR FORMAT\r\n");
            return;
        }
        target_magnet = (uint8)m;
        cmd_type = CMD_POSS;
        CmdParser_SetResp("OK POSS\r\n");
        return;
    }

    /* --- #POSD,x,y,z,l,m,n --- */
    if (strncmp(buf, "#POSD,", 6) == 0) {
        n = sscanf(buf, "#POSD,%f,%f,%f,%f,%f,%f",
                   &target_x, &target_y, &target_z,
                   &target_x2, &target_y2, &target_z2);
        if (n != 6) {
            CmdParser_SetResp("ERR FORMAT\r\n");
            return;
        }
        cmd_type = CMD_POSD;
        CmdParser_SetResp("OK POSD\r\n");
        return;
    }

    /* --- #CAL,off1,off2,off3,off4,off5 --- */
    if (strncmp(buf, "#CAL,", 5) == 0) {
        int e = 1500;
        n = sscanf(buf, "#CAL,%d,%d,%d,%d,%d", &a, &b, &c, &d, &e);
        if (n < 4) {
            CmdParser_SetResp("ERR FORMAT\r\n");
            return;
        }
        if (a < SERVO_MIN || a > SERVO_MAX || b < SERVO_MIN || b > SERVO_MAX ||
            c < SERVO_MIN || c > SERVO_MAX || d < SERVO_MIN || d > SERVO_MAX) {
            CmdParser_SetResp("ERR RANGE\r\n");
            return;
        }
        target_s1 = (uint16)a;
        target_s2 = (uint16)b;
        target_s3 = (uint16)c;
        target_s4 = (uint16)d;
        target_s5 = (uint16)e;
        cmd_type = CMD_CAL;
        CmdParser_SetResp("OK CAL\r\n");
        return;
    }

    /* --- #HMSET,s1,s2,s3,s4,s5 --- */
    if (strncmp(buf, "#HMSET,", 7) == 0) {
        int e = 1500;
        n = sscanf(buf, "#HMSET,%d,%d,%d,%d,%d", &a, &b, &c, &d, &e);
        if (n < 4) {
            CmdParser_SetResp("ERR FORMAT\r\n");
            return;
        }
        if (a < SERVO_MIN || a > SERVO_MAX || b < SERVO_MIN || b > SERVO_MAX ||
            c < SERVO_MIN || c > SERVO_MAX || d < SERVO_MIN || d > SERVO_MAX) {
            CmdParser_SetResp("ERR RANGE\r\n");
            return;
        }
        home_pwm[1] = (uint16)a;
        home_pwm[2] = (uint16)b;
        home_pwm[3] = (uint16)c;
        home_pwm[4] = (uint16)d;
        home_pwm[5] = (uint16)e;
        cmd_type = CMD_HMSET;
        CmdParser_SetResp("OK HMSET\r\n");
        return;
    }

    /* --- #ROT,angle --- */
    if (strncmp(buf, "#ROT,", 5) == 0) {
        float angle;
        n = sscanf(buf, "#ROT,%f", &angle);
        if (n != 1) {
            CmdParser_SetResp("ERR FORMAT\r\n");
            return;
        }
        target_rot_angle = angle;
        cmd_type = CMD_ROT;
        CmdParser_SetResp("OK ROT\r\n");
        return;
    }

    /* --- #HOME --- */
    if (strncmp(buf, "#HOME", 5) == 0) {
        cmd_type = CMD_HOME;
        CmdParser_SetResp("OK HOME\r\n");
        return;
    }

    /* 测试指令 */
    if (strncmp(buf, "#T", 2) == 0) {
        TestGrab_HandleCmd(buf);
        return;
    }

    CmdParser_SetResp("ERR UNKNOWN\r\n");
}
