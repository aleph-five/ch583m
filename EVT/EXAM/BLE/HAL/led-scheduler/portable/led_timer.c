#include "LED_scheduler.h"
#include "config.h"
#include "HAL.h"

int32_t led_tick_count(void)
{
    return TMOS_GetSystemClock() / 16;
}

void led_set_timer(uint32_t delay_ticks)
{
    uint32_t delay = delay_ticks * 16;
    tmos_start_task(halTaskID, LED_BLINK_EVENT, delay);
}

void led_timer_stop(void)
{
    tmos_stop_task(halTaskID, LED_BLINK_EVENT);
}
