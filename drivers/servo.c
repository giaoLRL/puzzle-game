// ============================================================================
//  servo.c — 用 initPWMMode(EDGE_ALIGN_UP) 正确重建 PWM, period=20000=50Hz
//            TIMG0(CC0/CC1)=舵机1/2, TIMA1(CC0/CC1)=舵机3/4, TIMG8(CC0)=舵机5
// ============================================================================

#include "servo.h"

volatile uint16 CPWM[SERVO_NUM + 1];

volatile uint16 home_pwm[SERVO_NUM + 1] = { 0, HOME_PWM_S1, HOME_PWM_S2, HOME_PWM_S3, HOME_PWM_S4, HOME_PWM_S5 };

volatile uint32 step;
volatile uint32 tg0_load, tg0_cc0, tg0_cc1, tg0_ctr, tg0_ctrctl;
volatile uint32 ta1_load, ta1_cc0, ta1_cc1, ta1_ctr, ta1_ctrctl;
volatile uint32 tg8_load, tg8_cc0, tg8_ctr, tg8_ctrctl;
volatile uint32 stk_load, stk_ctrl, stk_val;
volatile uint32 hb_count;
volatile uint32 ccpd0, ccpd1, ccpd2, odis0, odis1, odis2;
volatile uint32 cps0, cps1, cps2, clkdiv0, clkdiv1, clkdiv2;

static void cap_regs(void)
{
    tg0_load   = TIMG0->COUNTERREGS.LOAD;
    tg0_cc0    = TIMG0->COUNTERREGS.CC_01[0];
    tg0_cc1    = TIMG0->COUNTERREGS.CC_01[1];
    tg0_ctr    = TIMG0->COUNTERREGS.CTR;
    tg0_ctrctl = TIMG0->COUNTERREGS.CTRCTL;
    ta1_load   = TIMA1->COUNTERREGS.LOAD;
    ta1_cc0    = TIMA1->COUNTERREGS.CC_01[0];
    ta1_cc1    = TIMA1->COUNTERREGS.CC_01[1];
    ta1_ctr    = TIMA1->COUNTERREGS.CTR;
    ta1_ctrctl = TIMA1->COUNTERREGS.CTRCTL;
    tg8_load   = TIMG8->COUNTERREGS.LOAD;
    tg8_cc0    = TIMG8->COUNTERREGS.CC_01[0];
    tg8_ctr    = TIMG8->COUNTERREGS.CTR;
    tg8_ctrctl = TIMG8->COUNTERREGS.CTRCTL;
    ccpd0 = TIMG0->COMMONREGS.CCPD;  ccpd1 = TIMA1->COMMONREGS.CCPD;  ccpd2 = TIMG8->COMMONREGS.CCPD;
    odis0 = TIMG0->COMMONREGS.ODIS;  odis1 = TIMA1->COMMONREGS.ODIS;  odis2 = TIMG8->COMMONREGS.ODIS;
    cps0  = TIMG0->COMMONREGS.CPS;   cps1  = TIMA1->COMMONREGS.CPS;   cps2  = TIMG8->COMMONREGS.CPS;
    clkdiv0 = TIMG0->CLKDIV;         clkdiv1 = TIMA1->CLKDIV;         clkdiv2 = TIMG8->CLKDIV;
}

static void write_pwm_g0(uint16 pwm_us, DL_TIMER_CC_INDEX cc)
{
    if (pwm_us < SERVO_MIN) pwm_us = SERVO_MIN;
    if (pwm_us > SERVO_MAX) pwm_us = SERVO_MAX;
    DL_TimerG_setCaptureCompareValue(TIMG0, (uint32_t)pwm_us, cc);
}

static void write_pwm_a1(uint16 pwm_us, DL_TIMER_CC_INDEX cc)
{
    if (pwm_us < SERVO_MIN) pwm_us = SERVO_MIN;
    if (pwm_us > SERVO_MAX) pwm_us = SERVO_MAX;
    DL_TimerA_setCaptureCompareValue(TIMA1, (uint32_t)pwm_us, cc);
}

static void write_pwm_g8(uint16 pwm_us, DL_TIMER_CC_INDEX cc)
{
    if (pwm_us < SERVO_MIN) pwm_us = SERVO_MIN;
    if (pwm_us > SERVO_MAX) pwm_us = SERVO_MAX;
    DL_TimerG_setCaptureCompareValue(TIMG8, (uint32_t)pwm_us, cc);
}

void Servo_Init(void)
{
    DL_TimerG_PWMConfig pwm_cfg;
    DL_TimerG_ClockConfig clk_cfg;

    /* 通用时钟配置 */
    clk_cfg.clockSel    = DL_TIMER_CLOCK_BUSCLK;
    clk_cfg.divideRatio = DL_TIMER_CLOCK_DIVIDE_1;
    clk_cfg.prescale    = 31U;

    step = 1;

    /* === TIMG0: UP 边沿对齐, 0→LOAD, 周期=20000=20ms=50Hz === */
    DL_TimerG_setClockConfig(TIMG0, &clk_cfg);
    pwm_cfg.pwmMode           = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP;
    pwm_cfg.period            = 20000;
    pwm_cfg.isTimerWithFourCC = true;
    pwm_cfg.startTimer        = DL_TIMER_STOP;
    DL_TimerG_initPWMMode(TIMG0, &pwm_cfg);

    step = 2;

    /* === TIMA1: 同上 === */
    DL_TimerA_setClockConfig(TIMA1, &clk_cfg);
    pwm_cfg.isTimerWithFourCC = false;  /* TIMA1 only 2 CC */
    DL_TimerA_initPWMMode(TIMA1, &pwm_cfg);

    step = 3;

    /* === TIMG8: 舵机5 (PA7) === */
    DL_TimerG_enablePower(TIMG8);
    DL_TimerG_setClockConfig(TIMG8, &clk_cfg);
    DL_TimerG_initPWMMode(TIMG8, &pwm_cfg);
    DL_TimerG_enableClock(TIMG8);

    /* PA7 → TIMG8 CCP0 */
    DL_GPIO_initPeripheralOutputFunction(IOMUX_PINCM14, IOMUX_PINCM14_PF_TIMG8_CCP0);
    DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_7);

    step = 4;

    /* HOME PWM */
    CPWM[1] = home_pwm[1];
    CPWM[2] = home_pwm[2];
    CPWM[3] = home_pwm[3];
    CPWM[4] = home_pwm[4];
    CPWM[5] = home_pwm[5];
    Servo_UpdateAll();

    step = 5;

    DL_TimerG_startCounter(TIMG0);
    DL_TimerA_startCounter(TIMA1);
    DL_TimerG_startCounter(TIMG8);
    cap_regs();

    step = 6;
}

void Servo_UpdateAll(void)
{
    write_pwm_g0(CPWM[1], DL_TIMER_CC_0_INDEX);
    write_pwm_g0(CPWM[2], DL_TIMER_CC_1_INDEX);
    write_pwm_a1(CPWM[3], DL_TIMER_CC_0_INDEX);
    write_pwm_a1(CPWM[4], DL_TIMER_CC_1_INDEX);
    write_pwm_g8(CPWM[5], DL_TIMER_CC_0_INDEX);
}

void Servo_SetPWM(uint8 ch, uint16 pwm_us)
{
    if (ch < 1 || ch > SERVO_NUM) return;
    if (pwm_us < SERVO_MIN) pwm_us = SERVO_MIN;
    if (pwm_us > SERVO_MAX) pwm_us = SERVO_MAX;
    CPWM[ch] = pwm_us;
    switch (ch) {
    case 1: write_pwm_g0(pwm_us, DL_TIMER_CC_0_INDEX); break;
    case 2: write_pwm_g0(pwm_us, DL_TIMER_CC_1_INDEX); break;
    case 3: write_pwm_a1(pwm_us, DL_TIMER_CC_0_INDEX); break;
    case 4: write_pwm_a1(pwm_us, DL_TIMER_CC_1_INDEX); break;
    case 5: write_pwm_g8(pwm_us, DL_TIMER_CC_0_INDEX); break;
    default: break;
    }
}
