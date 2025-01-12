/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#ifndef __SLEEP_H
#define __SLEEP_H

#ifdef __cplusplus
extern "C" {
#endif
/*********************************************************************
 * MACROS
 */



/*********************************************************************
 * USE API MACRO INSTEAD OF API FUNCTIONS
 */
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)

#define HAL_SLEEP_INIT()                                    HAL_SleepInit()
#define HAL_SLEEP_SET_IDLE_MODE(idle_bitmask)               HAL_SleepSetIdleMode(idle_bitmask)
#define HAL_SLEEP_RESET_IDLE_MODE(idle_bitmask)             HAL_SleepResetIdleMode(idle_bitmask)
#define HAL_SLEEP_ISR_POST_PROCESS()                        HAL_SleepISRPostProcess()

#else

#define HAL_SLEEP_INIT()
#define HAL_SLEEP_SET_IDLE_MODE(idle_bitmask)
#define HAL_SLEEP_RESET_IDLE_MODE(idle_bitmask)
#define HAL_SLEEP_ISR_POST_PROCESS()

#endif


/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */
typedef uint32_t sleep_idle_bitmask_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern volatile sleep_idle_bitmask_t _m_sleep_idle_bitmask;

/*********************************************************************
 * FUNCTIONS
 */

/*******************************************************************************
 * @fn          HAL_SleepInit
 *
 * @brief       Initializes sleep module
 *
 * @param       none
 *
 * @return      none.
 */
extern void HAL_SleepInit(void);

/*******************************************************************************
 * @fn          HAL_SleepSetIdleMode
 *
 * @brief       Sets idle mode for specified resource
 *
 * @param       idle_bitmask   resource bitmask. Defined by application.
 *              Up to 32 resources available for application.
 *              System will go into Sleep mode only when all resources
 *              reset Idle mode. If at least one resource sets Idle mode
 *              then system always will go into Idle mode.
 *
 * @return      none.
 */
static inline void HAL_SleepSetIdleMode(sleep_idle_bitmask_t idle_bitmask)
{
    __AMOOR_W(&_m_sleep_idle_bitmask, idle_bitmask);
}

/*******************************************************************************
 * @fn          HAL_SleepResetIdleMode
 *
 * @brief       Resets idle mode for specified resource
 *
 * @param       idle_bitmask   resource bitmask. Defined by application.
 *
 * @return      none.
 */
static inline void HAL_SleepResetIdleMode(sleep_idle_bitmask_t idle_bitmask)
{
    __AMOAND_W(&_m_sleep_idle_bitmask, ~(idle_bitmask));
}


/*******************************************************************************
 * @fn          HAL_SleepISRPostProcess
 *
 * @brief       Notify sleep module that interrupt service routine has been executed.
 *              This procedure must be placed in the end of every ISR in use.
 *
 *
 * @param       none
 *
 * @return      none
 */
extern void HAL_SleepISRPostProcess(void);


/*********************************************************************
 * PROTECTED FUNCTIONS
 */
/*******************************************************************************
 * @fn          CH58X_LowPower
 *
 * @brief       Low power preparation procedure. Do not call this function
 *              manually. This function invoked by BLE stack if no active
 *              task is present.
 *
 * @param       time RTC trigger absolute value
 *
 * @return      state
 */
extern uint32_t CH58X_LowPower(uint32_t time);


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
