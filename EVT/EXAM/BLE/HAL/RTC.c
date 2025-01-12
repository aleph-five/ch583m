/********************************** (C) COPYRIGHT *******************************
 * File Name          : RTC.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : RTC���ü����ʼ��
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* ͷ�ļ����� */
#include "HAL.h"

/*********************************************************************
 * CONSTANTS
 */
#define RTC_INIT_TIME_HOUR      0
#define RTC_INIT_TIME_MINUTE    0
#define RTC_INIT_TIME_SECEND    0

/***************************************************
 * Global variables
 */
volatile uint32_t RTCTigFlag;

/*******************************************************************************
 * @fn      RTC_SetTignTime
 *
 * @brief   ����RTC����ʱ��
 *
 * @param   time    - ����ʱ��.
 *
 * @return  None.
 */
void RTC_SetTignTime(uint32_t time)
{
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();
    sys_safe_access_enter();
    R32_RTC_TRIG = time;
    sys_safe_access_exit();
    RTCTigFlag = 0;
    irq_restore_ctx(irq_ctx);
}

/*******************************************************************************
 * @fn      RTC_IRQHandler
 *
 * @brief   RTC�жϴ���
 *
 * @param   None.
 *
 * @return  None.
 */
__INTERRUPT
__HIGH_CODE
void RTC_IRQHandler(void)
{
    R8_RTC_FLAG_CTRL = (RB_RTC_TMR_CLR | RB_RTC_TRIG_CLR);
    RTCTigFlag = 1;
    HAL_SLEEP_ISR_POST_PROCESS();
}

/*******************************************************************************
 * @fn      HAL_Time0Init
 *
 * @brief   ϵͳ��ʱ����ʼ��
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_TimeInit(void)
{
#if(CLK_OSC32K)
    sys_safe_access_enable();
    R8_CK32K_CONFIG &= ~(RB_CLK_OSC32K_XT | RB_CLK_XT32K_PON);
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_CK32K_CONFIG |= RB_CLK_INT32K_PON;
    sys_safe_access_disable();
    LSECFG_Current(LSE_RCur_100);
    Lib_Calibration_LSI();
#else
    LSECFG_Capacitance(LSECap_12p);
    uint8_t status = LSE_Init();
    if(!status)
    {
        while(1);
    }
#endif
    RTC_InitTime(2020, 1, 1, 0, 0, 0); //RTCʱ�ӳ�ʼ����ǰʱ��
    TMOS_TimerInit(0);
}

/******************************** endfile @ time ******************************/
