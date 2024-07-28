/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH58x_clk.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2021/11/17
 * Description
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH58x_common.h"

#define CLK_ATRIBUTE_OPTIMIZE_FAST                          __attribute__((optimize("-Ofast")))
#define CLK_ATRIBUTE_OPTIMIZE_SIZE                          __attribute__((optimize("-Os")))
#define CLK_ATTRIBUTE_FORCE_INLINE                          __attribute__((always_inline))

typedef struct
{
    uint32_t days;
    uint32_t ticks;
}
rtc_datetime_t;


#define RTC_GetDateTime() ({ \
    uint32_t t, ticks;\
    uint16_t days, days_again;\
\
  /* This procedure is needed because we can read between day change */ \
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();\
    days = R32_RTC_CNT_DAY;\
    ticks = R32_RTC_CNT_32K;\
    days_again = R32_RTC_CNT_DAY;\
    irq_restore_ctx(irq_ctx);\
\
\
    days &= RTC_CNT_DAY_MSK;\
    days_again &= RTC_CNT_DAY_MSK;\
\
    /* if days and days_again are not equal, then we read between day change,
    so we need read number of ticks again*/ \
    if(days != days_again)\
    {\
        ticks = R32_RTC_CNT_32K;\
        days = days_again;\
    }\
\
    rtc_datetime_t datetime = {\
        .days = days,\
        .ticks = ticks\
    };\
\
    datetime;\
})


/*********************************************************************
 * @fn      LClk32K_Select
 *
 * @brief   32K 低频时钟来源
 *
 * @param   hc  - 选择32K使用内部还是外部
 *
 * @return  none
 */
void LClk32K_Select(LClk32KTypeDef hc)
{
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    uint8_t cfg = R8_CK32K_CONFIG;

    if(hc == Clk32K_LSI)
    {
        cfg &= ~RB_CLK_OSC32K_XT;
        LSECFG_Current(LSE_RCur_100);
    }
    else
    {
        cfg |= RB_CLK_OSC32K_XT;
    }

    sys_safe_access_enter();
    R8_CK32K_CONFIG = cfg;
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

CLK_ATRIBUTE_OPTIMIZE_FAST
static int32_t _WaitForHalfCycleAtLeast(void)
{
    int32_t status = 0;
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    uint32_t times = GetSysClock() / 50;
    uint32_t i = times;
    uint8_t clk_pin = (R8_CK32K_CONFIG & RB_32K_CLK_PIN);
    while(--i)
    {
        uint8_t clk_pin_temp;
        if((clk_pin_temp = (R8_CK32K_CONFIG & RB_32K_CLK_PIN)) != clk_pin)
        {
            clk_pin = clk_pin_temp;
            status = 1;
            break;
        }
    }
    if(status)
    {
        status = 0;
        i = times;
        while(--times)
        {
            if(clk_pin != (R8_CK32K_CONFIG & RB_32K_CLK_PIN))
            {
                status = 1;
                break;
            }
        }
    }

    irq_restore_ctx(irq_ctx);
    return status;
}

// This procedure of LSE initialization is according to ch.6.4.1 of datasheet v1.8
uint8_t LSE_Init(void)
{
    // Disable digital pins for LSE crystal oscillator
    R16_PIN_ANALOG_IE |= RB_PIN_XT32K_IE;
    GPIOA_ModeCfg(bX32KI | bX32KO, GPIO_ModeIN_Floating);

    // Enable external oscillator
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();
    sys_safe_access_enter();
    R8_CK32K_CONFIG |= RB_CLK_XT32K_PON;

    // Change to max current first, wait for stabilizing, change to rated current
    LSECFG_Current(LSE_RCur_200);
    mDelaymS(200);
    LSECFG_Current(LSE_RCur_100);

    R8_CK32K_CONFIG |= RB_CLK_OSC32K_XT;

    // Switch off internal oscillator for power save
    sys_safe_access_enter();
    R8_CK32K_CONFIG &= ~RB_CLK_INT32K_PON;
    sys_safe_access_exit();

    // Switch to LSE
    LClk32K_Select(Clk32K_LSE);

    // Wait for half cycle at least
    if(_WaitForHalfCycleAtLeast())
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*********************************************************************
 * @fn      HSECFG_Current
 *
 * @brief   HSE晶体 偏置电流配置
 *
 * @param   c   - 75%,100%,125%,150%
 *
 * @return  none
 */
void HSECFG_Current(HSECurrentTypeDef c)
{
    uint8_t x32M_c;
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();
    x32M_c = R8_XT32M_TUNE;
    x32M_c = (x32M_c & 0xfc) | (c & 0x03);

    sys_safe_access_enter();
    R8_XT32M_TUNE = x32M_c;
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      HSECFG_Capacitance
 *
 * @brief   HSE晶体 负载电容配置
 *
 * @param   c   - refer to HSECapTypeDef
 *
 * @return  none
 */
void HSECFG_Capacitance(HSECapTypeDef c)
{
    uint8_t x32M_c;
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    x32M_c = R8_XT32M_TUNE;
    x32M_c = (x32M_c & 0x8f) | (c << 4);

    sys_safe_access_enter();
    R8_XT32M_TUNE = x32M_c;
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      LSECFG_Current
 *
 * @brief   LSE晶体 偏置电流配置
 *
 * @param   c   - 70%,100%,140%,200%
 *
 * @return  none
 */
void LSECFG_Current(LSECurrentTypeDef c)
{
    uint8_t x32K_c;

    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    x32K_c = R8_XT32K_TUNE;
    x32K_c = (x32K_c & 0xfc) | (c & 0x03);

    sys_safe_access_enter();
    R8_XT32K_TUNE = x32K_c;
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      LSECFG_Capacitance
 *
 * @brief   LSE晶体 负载电容配置
 *
 * @param   c   - refer to LSECapTypeDef
 *
 * @return  none
 */
void LSECFG_Capacitance(LSECapTypeDef c)
{
    uint8_t x32K_c;

    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    x32K_c = R8_XT32K_TUNE;
    x32K_c = (x32K_c & 0x0f) | (c << 4);

    sys_safe_access_enter();
    R8_XT32K_TUNE = x32K_c;
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      Calibration_LSI
 *
 * @brief   校准内部32K时钟
 *
 * @param   cali_Lv - 校准等级选择    Level_32 ：2.4ms   1000ppm (32M 主频)  1100ppm (60M 主频)
 *                                   Level_64 ：4.4ms   800ppm  (32M 主频)  1000ppm (60M 主频)
 *                                   Level_128 ：8.4ms  600ppm  (32M 主频)  800ppm  (60M 主频)
 *
 * @return  none
 */
void Calibration_LSI(Cali_LevelTypeDef cali_Lv)
{
    UINT32 i;
    INT32 cnt_offset;
    UINT8  retry = 0;
    UINT8  retry_all = 0;
    INT32  freq_sys;
    UINT32 cnt_32k = 0;
    UINT32 irqv = 0;
    freq_sys = GetSysClock();

    sys_safe_access_enable();
    R8_CK32K_CONFIG &= ~RB_CLK_OSC32K_FILT;
    R8_CK32K_CONFIG |= RB_CLK_OSC32K_FILT;
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_XT32K_TUNE &= ~3;
    R8_XT32K_TUNE |= 1;
    sys_safe_access_disable();

    while(1)
    {
        // 粗调
        sys_safe_access_enable();
        R8_OSC_CAL_CTRL &= ~RB_OSC_CNT_TOTAL;
        R8_OSC_CAL_CTRL |= 1;
        sys_safe_access_disable();

        while(1)
        {
            sys_safe_access_enable();
            R8_OSC_CAL_CTRL |= RB_OSC_CNT_EN;
            R16_OSC_CAL_CNT |= RB_OSC_CAL_OV_CLR;
            R16_OSC_CAL_CNT |= RB_OSC_CAL_IF;
            sys_safe_access_disable();
            while( (R8_OSC_CAL_CTRL & RB_OSC_CNT_EN) == 0 )
            {
                sys_safe_access_enable();
                R8_OSC_CAL_CTRL |= RB_OSC_CNT_EN;
                sys_safe_access_disable();
            }

            while(!(R8_OSC_CAL_CTRL & RB_OSC_CNT_HALT)); // 用于丢弃

            SYS_DisableAllIrq(&irqv);
            sys_safe_access_enable();
            R8_OSC_CAL_CTRL &= ~RB_OSC_CNT_EN;
            R8_OSC_CAL_CTRL |= RB_OSC_CNT_EN;
            R16_OSC_CAL_CNT |= RB_OSC_CAL_OV_CLR;
            R16_OSC_CAL_CNT |= RB_OSC_CAL_IF;
            sys_safe_access_disable();
            while( (R8_OSC_CAL_CTRL & RB_OSC_CNT_EN) == 0 )
            {
                sys_safe_access_enable();
                R8_OSC_CAL_CTRL |= RB_OSC_CNT_EN;
                sys_safe_access_disable();
            }

            while(R8_OSC_CAL_CTRL & RB_OSC_CNT_HALT);
            cnt_32k = RTC_GetCycle32k();
            while(RTC_GetCycle32k() == cnt_32k);
            R16_OSC_CAL_CNT |= RB_OSC_CAL_OV_CLR;
            SYS_RecoverIrq(irqv);
            while(!(R8_OSC_CAL_CTRL & RB_OSC_CNT_HALT));
            i = R16_OSC_CAL_CNT; // 实时校准后采样值
            cnt_offset = (i & 0x3FFF) + R8_OSC_CAL_OV_CNT * 0x3FFF - 2000 * (freq_sys / 1000) / CAB_LSIFQ;
            if(((cnt_offset > -37 * (freq_sys / 1000) / 60000) && (cnt_offset < 37 * (freq_sys / 1000) / 60000)) || retry > 2)
            {
                if(retry)
                    break;
            }
            retry++;
            cnt_offset = (cnt_offset > 0) ? (((cnt_offset * 2) / (74 * (freq_sys/1000) / 60000)) + 1) / 2 : (((cnt_offset * 2) / (74 * (freq_sys/1000) / 60000 )) - 1) / 2;
            sys_safe_access_enable();
            R16_INT32K_TUNE += cnt_offset;
            sys_safe_access_disable();
        }

        // 细调
        // 配置细调参数后，丢弃2次捕获值（软件行为）上判断已有一次，这里只留一次
        sys_safe_access_enable();
        R8_OSC_CAL_CTRL &= ~RB_OSC_CNT_TOTAL;
        R8_OSC_CAL_CTRL |= cali_Lv;
        sys_safe_access_disable();
        while( (R8_OSC_CAL_CTRL & RB_OSC_CNT_TOTAL) != cali_Lv )
        {
            sys_safe_access_enable();
            R8_OSC_CAL_CTRL |= cali_Lv;
            sys_safe_access_disable();
        }

        sys_safe_access_enable();
        R8_OSC_CAL_CTRL &= ~RB_OSC_CNT_EN;
        R8_OSC_CAL_CTRL |= RB_OSC_CNT_EN;
        R16_OSC_CAL_CNT |= RB_OSC_CAL_OV_CLR;
        R16_OSC_CAL_CNT |= RB_OSC_CAL_IF;
        sys_safe_access_disable();
        while( (R8_OSC_CAL_CTRL & RB_OSC_CNT_EN) == 0 )
        {
            sys_safe_access_enable();
            R8_OSC_CAL_CTRL |= RB_OSC_CNT_EN;
            sys_safe_access_disable();
        }

        while(!(R8_OSC_CAL_CTRL & RB_OSC_CNT_HALT)); // 用于丢弃

        SYS_DisableAllIrq(&irqv);
        sys_safe_access_enable();
        R8_OSC_CAL_CTRL &= ~RB_OSC_CNT_EN;
        R8_OSC_CAL_CTRL |= RB_OSC_CNT_EN;
        R16_OSC_CAL_CNT |= RB_OSC_CAL_OV_CLR;
        R16_OSC_CAL_CNT |= RB_OSC_CAL_IF;
        sys_safe_access_disable();
        while( (R8_OSC_CAL_CTRL & RB_OSC_CNT_EN) == 0 )
        {
            sys_safe_access_enable();
            R8_OSC_CAL_CTRL |= RB_OSC_CNT_EN;
            sys_safe_access_disable();
        }

        while(R8_OSC_CAL_CTRL & RB_OSC_CNT_HALT);
        cnt_32k = RTC_GetCycle32k();
        while(RTC_GetCycle32k() == cnt_32k);
        R16_OSC_CAL_CNT |= RB_OSC_CAL_OV_CLR;
        SYS_RecoverIrq(irqv);
        while(!(R8_OSC_CAL_CTRL & RB_OSC_CNT_HALT));
        sys_safe_access_enable();
        R8_OSC_CAL_CTRL &= ~RB_OSC_CNT_EN;
        sys_safe_access_disable();
        i = R16_OSC_CAL_CNT; // 实时校准后采样值
        cnt_offset = (i & 0x3FFF) + R8_OSC_CAL_OV_CNT * 0x3FFF -  4000 * (1 << cali_Lv) * (freq_sys / 1000000) / 256 * 1000/(CAB_LSIFQ/256);
        cnt_offset = (cnt_offset > 0) ? ((((cnt_offset * 2*(100 )) / (1366 * ((1 << cali_Lv)/8) * (freq_sys/1000) / 60000)) + 1) / 2) : ((((cnt_offset * 2*(100)) / (1366 * ((1 << cali_Lv)/8) * (freq_sys/1000) / 60000)) - 1) / 2);
        if((cnt_offset > 0)&&(((R16_INT32K_TUNE>>5)+cnt_offset)>0xFF))
        {
            if(retry_all>2)
            {
                sys_safe_access_enable();
                R16_INT32K_TUNE |= (0xFF<<5);
                sys_safe_access_disable();
                return;
            }
            else
            {
                sys_safe_access_enable();
                R16_INT32K_TUNE = (R16_INT32K_TUNE&0x1F)|(0x7F<<5);
                sys_safe_access_disable();
            }
        }
        else if((cnt_offset < 0)&&((R16_INT32K_TUNE>>5)<(-cnt_offset)))
        {
            if(retry_all>2)
            {
                sys_safe_access_enable();
                R16_INT32K_TUNE &= 0x1F;
                sys_safe_access_disable();
                return;
            }
            else
            {
                sys_safe_access_enable();
                R16_INT32K_TUNE = (R16_INT32K_TUNE&0x1F)|(0x7F<<5);
                sys_safe_access_disable();
            }
        }
        else
        {
            sys_safe_access_enable();
            R16_INT32K_TUNE += (cnt_offset<<5);
            sys_safe_access_disable();
            return;
        }
        retry_all++;

    }
}

/*********************************************************************
 * @fn      RTCInitTime
 *
 * @brief   RTC时钟初始化当前时间
 *
 * @param   y       - 配置年，MAX_Y = BEGYEAR + 44
 * @param   mon     - 配置月，MAX_MON = 12
 * @param   d       - 配置日，MAX_D = 31
 * @param   h       - 配置小时，MAX_H = 23
 * @param   m       - 配置分钟，MAX_M = 59
 * @param   s       - 配置秒，MAX_S = 59
 *
 * @return  none
 */
void RTC_InitTime(uint16_t y, uint16_t mon, uint16_t d, uint16_t h, uint16_t m, uint16_t s)
{
    uint32_t         t;
    uint16_t         year, month, day, sec2, t32k;
    volatile uint8_t clk_pin;

    year = y;
    month = mon;
    day = 0;
    while(year > BEGYEAR)
    {
        day += YearLength(year - 1);
        year--;
    }
    while(month > 1)
    {
        day += monthLength(IsLeapYear(y), month - 2);
        month--;
    }

    day += d - 1;
    sec2 = (h % 24) * 1800 + m * 30 + s / 2;
    t32k = (s & 1) ? (0x8000) : (0);
    t = sec2;
    t = t << 16 | t32k;

    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    do
    {
        clk_pin = (R8_CK32K_CONFIG & RB_32K_CLK_PIN);
    } while(clk_pin != (R8_CK32K_CONFIG & RB_32K_CLK_PIN));
    if(!clk_pin)
    {
        while(!clk_pin)
        {
            do
            {
                clk_pin = (R8_CK32K_CONFIG & RB_32K_CLK_PIN);
            } while(clk_pin != (R8_CK32K_CONFIG & RB_32K_CLK_PIN));
        }
    }

    sys_safe_access_enter();
    R32_RTC_TRIG = day;
    R8_RTC_MODE_CTRL |= RB_RTC_LOAD_HI;
    sys_safe_access_exit();
    while((R32_RTC_TRIG & 0x3FFF) != (R32_RTC_CNT_DAY & 0x3FFF));
    sys_safe_access_enter();
    R32_RTC_TRIG = t;
    R8_RTC_MODE_CTRL |= RB_RTC_LOAD_LO;
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      RTC_GetTime
 *
 * @brief   获取当前时间
 *
 * @param   py      - 获取到的年，MAX_Y = BEGYEAR + 44
 * @param   pmon    - 获取到的月，MAX_MON = 12
 * @param   pd      - 获取到的日，MAX_D = 31
 * @param   ph      - 获取到的小时，MAX_H = 23
 * @param   pm      - 获取到的分钟，MAX_M = 59
 * @param   ps      - 获取到的秒，MAX_S = 59
 *
 * @return  none
 */
CLK_ATRIBUTE_OPTIMIZE_FAST
void RTC_GetTime(uint16_t *py, uint16_t *pmon, uint16_t *pd, uint16_t *ph, uint16_t *pm, uint16_t *ps)
{
    uint32_t t;
    uint16_t day, sec2, t32k;

    rtc_datetime_t datetime = RTC_GetDateTime();
    day = datetime.days;
    t = RTC_CYCLES_TO_SEC(datetime.ticks);

    *py = BEGYEAR;
    while(day >= YearLength(*py))
    {
        day -= YearLength(*py);
        (*py)++;
    }

    *pmon = 0;
    while(day >= monthLength(IsLeapYear(*py), *pmon))
    {
        day -= monthLength(IsLeapYear(*py), *pmon);
        (*pmon)++;
    }
    (*pmon)++;
    *pd = day + 1;
    *ph = t / 3600;
    *pm = t % 3600 / 60;
    *ps = t % 60;
}

/*********************************************************************
 * @fn      RTC_SetCycle32k
 *
 * @brief   基于LSE/LSI时钟，配置当前RTC 周期数
 *
 * @param   cyc     - 配置周期计数初值，MAX_CYC = 0xA8BFFFFF = 2831155199
 *
 * @return  none
 */
void RTC_SetCycle32k(uint32_t cyc)
{
    uint8_t clk_pin;
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    // don't understand why this code snippet is here
    do
    {
        clk_pin = (R8_CK32K_CONFIG & RB_32K_CLK_PIN);
    } while((clk_pin != (R8_CK32K_CONFIG & RB_32K_CLK_PIN)) || (!clk_pin));

    sys_safe_access_enter();
    R32_RTC_TRIG = cyc;
    R8_RTC_MODE_CTRL |= RB_RTC_LOAD_LO;
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      RTC_TMRFunCfg
 *
 * @brief   RTC定时模式配置（注意定时基准固定为32768Hz）
 *
 * @param   t   - refer to RTC_TMRCycTypeDef
 *
 * @return  none
 */
void RTC_TMRFunCfg(RTC_TMRCycTypeDef t)
{
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();
    sys_safe_access_enter();
    R8_RTC_MODE_CTRL &= ~(RB_RTC_TMR_EN | RB_RTC_TMR_MODE);
    sys_safe_access_enter();
    R8_RTC_MODE_CTRL |= RB_RTC_TMR_EN | (t);
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      RTC_TRIGFunCfg
 *
 * @brief   RTC时间触发模式配置
 *
 * @param   cyc - 相对当前时间的触发间隔时间，基于LSE/LSI时钟周期数
 *
 * @return  none
 */
CLK_ATRIBUTE_OPTIMIZE_FAST
void RTC_TRIGFunCfg(uint32_t cyc)
{
    // TODO: need some clarification about cycle trigger value
    uint32_t cyc_h = cyc >> 1;
    uint32_t cyc_l = cyc & 1;
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    uint32_t cur_cyc = RTC_GetCycle32k();
    uint32_t cur_cyc_h = cur_cyc >> 1;
    uint32_t cur_cyc_l = cur_cyc & 1;

    uint32_t t = cur_cyc_h + cyc_h;
    if(t >= ((uint32_t)0xA8C00000 >> 1))
    {
        t -= ((uint32_t)0xA8C00000 >> 1);
    }

    t = (t << 1) + cyc_l + cur_cyc_l;

    sys_safe_access_enter();
    R32_RTC_TRIG = t;
    R8_RTC_MODE_CTRL |= RB_RTC_TRIG_EN;
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      RTC_ModeFunDisable
 *
 * @brief   RTC 模式功能关闭
 *
 * @param   m   - 需要关闭的当前模式
 *
 * @return  none
 */
void RTC_ModeFunDisable(RTC_MODETypeDef m)
{
    uint8_t i = 0;

    if(m == RTC_TRIG_MODE)
    {
        i |= RB_RTC_TRIG_EN;
    }
    else if(m == RTC_TMR_MODE)
    {
        i |= RB_RTC_TMR_EN;
    }

    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();
    sys_safe_access_enter();
    R8_RTC_MODE_CTRL &= ~(i);
    sys_safe_access_exit();
    irq_restore_ctx(irq_ctx);
}

/*********************************************************************
 * @fn      RTC_GetITFlag
 *
 * @brief   获取RTC中断标志
 *
 * @param   f   - refer to RTC_EVENTTypeDef
 *
 * @return  中断标志状态
 */
uint8_t RTC_GetITFlag(RTC_EVENTTypeDef f)
{
    if(f == RTC_TRIG_EVENT)
    {
        return (R8_RTC_FLAG_CTRL & RB_RTC_TRIG_FLAG);
    }
    else
    {
        return (R8_RTC_FLAG_CTRL & RB_RTC_TMR_FLAG);
    }
}

/*********************************************************************
 * @fn      RTC_ClearITFlag
 *
 * @brief   清除RTC中断标志
 *
 * @param   f   - refer to RTC_EVENTTypeDef
 *
 * @return  none
 */
void RTC_ClearITFlag(RTC_EVENTTypeDef f)
{
    switch(f)
    {
        case RTC_TRIG_EVENT:
            R8_RTC_FLAG_CTRL = RB_RTC_TRIG_CLR;
            break;
        case RTC_TMR_EVENT:
            R8_RTC_FLAG_CTRL = RB_RTC_TMR_CLR;
            break;
        default:
            break;
    }
}
