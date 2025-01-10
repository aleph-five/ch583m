#ifndef __LED_SCHEDULER_H
#define __LED_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * CONSTANTS
 */
#define LED_REPEAT_INFINITE                 0xFFFFFFFF
#define LED_MAX_WIDTH                       0xFFFF
#define LED_MAX_PATTERN_LEN                 0xFF

/*********************************************************************
 * TYPEDEFS
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*led_switch_fn_t)(bool);

typedef struct
{
    const uint16_t * const delay_list;
    uint8_t size;
}
led_pattern_t;

typedef struct
{
    const led_pattern_t *pattern;
    uint8_t pattern_idx;
    int32_t deadline;
    uint32_t repeat_count;
    bool initial_state;
    bool is_active;
}
led_state_t;

typedef struct
{
    uint8_t led_num;
    const led_pattern_t *pattern;
    uint32_t repeat_count;
    bool initial_state;
}
led_event_new_state_t;

typedef struct
{
    led_switch_fn_t init_deinit;
    led_switch_fn_t on_off;
}
led_itf_t;

enum
{
    LED_EVENT_NEW_STATE,
    LED_EVENT_TIMER,
};

typedef struct
{
    uint8_t type;
    union
    {
        led_event_new_state_t new_state;
    };
}
led_event_t;

typedef struct
{
    led_state_t *state_list;
    uint8_t led_num;
}
led_state_table_t;

typedef struct
{
    led_itf_t *itf_list;
    uint8_t list_num;
}
led_itf_table_t;

typedef struct
{
    led_state_t *state_list;
    const led_itf_t *itf_list;
    uint8_t num_of_leds;
}
led_control_block_t;

void LED_control_block_init(led_control_block_t *block, led_state_t *state_list, const led_itf_t *itf_list, uint8_t num_of_leds);

void LED_on_event(led_control_block_t *block, const led_event_t *event);

// for internal use
int32_t led_tick_count(void);

void led_set_timer(uint32_t delay);

void led_timer_stop(void);

#ifdef __cplusplus
}
#endif

#endif
