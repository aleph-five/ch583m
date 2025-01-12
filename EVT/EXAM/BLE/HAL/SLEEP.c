/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : 睡眠配置及其初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */

#include <stdbool.h>
#include <stdint.h>
#include "HAL.h"

#define RTC_TIMER_GAP           60
#define WAKEUP_PERIPHERAL       RB_SLP_GPIO_WAKE

#define RTC_TIME_NORMALIZE(time)      time = time % RTC_TIMER_MAX_VALUE; if(!time){ time = 1;}

#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)

volatile sleep_idle_bitmask_t _m_sleep_idle_bitmask = 0;
static volatile bool _m_bypass = false;
static volatile uint32_t _m_nextAfterWFI = 0;


/*********************************************************************
 * @fn      LowPower_Idle_safe
 *
 * @brief   safe implementation of LowPower_Idle() function
 *          to avoid softlock
 *
 * @param   wfi if 0, wfe instruction is used, otherwise wfi
 *
 * @return  none
 */
__HIGH_CODE
static void LowPower_Idle_safe(uint32_t wfi)
{
    // Global interrupts must be already disabled before executing this function
    FLASH_ROM_SW_RESET();
    R8_FLASH_CTRL = 0x04; //flash关闭
    PFIC->SCTLR &= ~(1 << 2); // sleep

    if(wfi)
    {
      // This assembly inline is software patch for hardware bug in processor.
      // It allows to avoid soft lock in case when interrupt service routine is executed
      // between enabling interrupts (csrsi instruction) - and going to sleep (wfi instruction).
      // It saves next after wfi instruction address into variable _m_nextAfterWFI. This variable is
      // used in HAL_SleepISRPostProcess() - see a comment in this function.
      //
      // Bug description: According to Volume II: RISC-V Privileged Architectures V20211203, page 47
      // wfi instruction should resume execution if any local interrupt pending even if global
      // interrupt disabled. But this won't work for this processor.
      PFIC->SCTLR &= ~(1 << 3); // wfi
      __asm__ volatile(
                       "la t6, next_after_wfi\n\t"
                       "sw t6, %0\n\t"
                       "csrsi 0x800, 0x8\n\t"
                       "wfi\n\t"
                       "next_after_wfi: sw zero, %0"
                       :
                       : "m"(_m_nextAfterWFI)
                       : "t6");
        __nop();
    }
    else
    {
        __WFE();
        __nop();
        __nop();
    }
}


#endif

__attribute__((noinline))
__HIGH_CODE
void HAL_SleepISRPostProcess(void)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    // This function does two main things:
    // 1. It notifies sleep module that some interrupts were executed,
    //    so going to sleep won't be executed and main program
    //    remains responsive to interrupt events;
    // 2. It allows bypass wfi instruction if it was reached. Detailed description
    //    of hardware bug is in LowPower_Idle_safe() procedure.
    //    This patch based on mepc csr register. First it checks that interrupt nesting
    //    level is 1, because interrupt may be preempted by another interrupt and it returns
    //    in main program, but not in the preempted interrupt. Second, it atomically checks
    //    and swaps with 0 _m_nextAfterWFI variable. The writing in mepc occurs only if variable
    //    is not zero, because we don't always need to return to next after wfi instruction:
    //    interrupt may occur in any place of main program.
    _m_bypass = true;
    if((PFIC->GISR & 0xFF) == 1)
    {
        uint32_t naw = __AMOSWAP_W(&_m_nextAfterWFI, 0);
        if(naw)
        {
            write_csr(mepc, naw);
        }
    }
#endif
}

/*******************************************************************************
 * @fn          CH58X_LowPower
 *
 * @brief       启动睡眠
 *
 * @param   time    - 唤醒的时间点（RTC绝对值）
 *
 * @return      state.
 */

uint32_t CH58X_LowPower(uint32_t time)
{
    uint32_t ret_val = 0;
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    uint32_t time_sleep, time_curr;

    // LOW POWER-sleep模式
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();

    if(_m_bypass)
    {
        ret_val = 2;
        goto _no_low_power;
    }

    time_curr = RTC_GetCycle32k();
    // Normalize time to RTC TRIG register (even if we go out of sleep earlier,
    // TMOS scheduler will see that no active tasks and reschedule the next wakeup)
    RTC_TIME_NORMALIZE(time);

    // Calculate sleep time
    if (time < time_curr)
    {
        time_sleep = time + (RTC_TIMER_MAX_VALUE - time_curr);
    }
    else
    {
        time_sleep = time - time_curr;
    }
    bool in_gap = ((RTC_TIMER_MAX_VALUE - RTC_TIMER_GAP) < time_sleep) && (time_sleep <= RTC_TIMER_MAX_VALUE);

    if ((time_sleep < SLEEP_RTC_MIN_TIME) || in_gap) {
        ret_val = 2;
        goto _no_low_power;
    }

    RTC_SetTignTime(time);

    if(_m_sleep_idle_bitmask)
    {
        // go to idle mode
        LowPower_Idle_safe(1);
    }
    else
    {
#if(DEBUG == Debug_UART1) // 使用其他串口输出打印信息需要修改这行代码
        while((R8_UART1_LSR & RB_LSR_TX_ALL_EMP) == 0)
        {
            __nop();
        }
#endif
        // go to sleep mode
        LowPower_Sleep(RB_PWR_RAM2K | RB_PWR_RAM30K | RB_PWR_EXTEND);
        irq_restore_ctx(irq_ctx);
        irq_ctx = irq_save_ctx_and_disable();

        if(RTCTigFlag) // 注意如果使用了RTC以外的唤醒方式，需要注意此时32M晶振未稳定
        {
            time += WAKE_UP_RTC_MAX_TIME;
        }
        else
        {
            time = RTC_GetCycle32k();
            time += WAKE_UP_RTC_MAX_TIME;
        }

        RTC_TIME_NORMALIZE(time);

        RTC_SetTignTime(time);

        // This piece of code requires some explanation
        // The 32M oscillator requires some time for stabilization (defined by WAKE_UP_RTC_MAX_TIME).
        // If wakeup by some peripheral is allowed, like GPIO, it may occur in the middle of stabilization.
        // As a consequence, it may break stabilization process, and microcontroller goes out of CH58X_LowPower()
        // with unstabilized oscillator.
        // To prevent this, we disable wakeup by other peripheral except RTC, and
        // use LowPower_Idle_safe() with wfi as wfe instruction to go to sleep.
        // Please note that global interrupts are disabled during this procedure.
        // After returning from LowPower_Idle_safe(), we restore wakeup by other peripheral.
        PWR_PeriphWakeUpCfg(DISABLE, WAKEUP_PERIPHERAL, Long_Delay); // Disable other wakeup methods, RTC only, to stabilize 32M oscillator
        LowPower_Idle_safe(0);
        PWR_PeriphWakeUpCfg(ENABLE, WAKEUP_PERIPHERAL, Long_Delay); // Restore other wakeup methods after stabilizing 32M oscillator

        HSECFG_Current(HSE_RCur_100); // 降为额定电流(低功耗函数中提升了HSE偏置电流)
    }

    _no_low_power:
    _m_bypass = false;
    irq_restore_ctx(irq_ctx);

#endif
    return ret_val;
}

/*******************************************************************************
 * @fn      HAL_SleepInit
 *
 * @brief   配置睡眠唤醒的方式   - RTC唤醒，触发模式
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_SleepInit(void)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    PWR_PeriphWakeUpCfg(ENABLE, WAKEUP_PERIPHERAL | RB_SLP_RTC_WAKE, Long_Delay);
    irq_ctx_t irq_ctx = irq_save_ctx_and_disable();
    sys_safe_access_enter();
    R8_RTC_MODE_CTRL |= RB_RTC_TRIG_EN;  // 触发模式
    sys_safe_access_exit();              //
    PFIC_EnableIRQ(RTC_IRQn);
    _m_bypass = false;
    _m_sleep_idle_bitmask = 0;
    irq_restore_ctx(irq_ctx);
#endif
}
