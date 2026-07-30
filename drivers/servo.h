// ============================================================================
//  servo.h
// ============================================================================

#ifndef __SERVO_H
#define __SERVO_H

#include "../config/common.h"

extern volatile uint16 CPWM[SERVO_NUM + 1];

void Servo_Init(void);
void Servo_UpdateAll(void);
void Servo_SetPWM(uint8 ch, uint16 pwm_us);

extern volatile uint32 step;
extern volatile uint32 tg0_load, tg0_cc0, tg0_cc1, tg0_ctr, tg0_ctrctl;
extern volatile uint32 ta1_load, ta1_cc0, ta1_cc1, ta1_ctr, ta1_ctrctl;
extern volatile uint32 tg8_load, tg8_cc0, tg8_ctr, tg8_ctrctl;
extern volatile uint32 stk_load, stk_ctrl, stk_val;
extern volatile uint32 hb_count;
extern volatile uint32 ccpd0, ccpd1, ccpd2, odis0, odis1, odis2;
extern volatile uint32 cps0, cps1, cps2, clkdiv0, clkdiv1, clkdiv2;

#endif

/* ---- 上电归位 PWM（运行时可通过 #HMSET 修改）---- */
extern volatile uint16 home_pwm[SERVO_NUM + 1];
