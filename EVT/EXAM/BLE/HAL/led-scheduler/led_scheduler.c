#include "LED_scheduler_config.h"
#include "LED_scheduler.h"

#define LED_STATE(state)                                        (bool)(((uint8_t)(state->initial_state) + state->pattern_idx) % 2)
#define LED_TIMER_EXPIRED(led_state)                            (led_state->deadline - led_tick_count() <= 0)

void LED_control_block_init(led_control_block_t *block, led_state_t *state_list, const led_itf_t *itf_list, uint8_t num_of_leds)
{
    // TODO: assertion
    LED_ASSERT(block);
    LED_ASSERT(state_list);
    LED_ASSERT(itf_list);
    LED_ASSERT(num_of_leds);

    block->itf_list = itf_list;
    block->state_list = state_list;
    block->num_of_leds = num_of_leds;

    for(int i = 0; i < num_of_leds; i++)
    {
        LED_ASSERT(itf_list[i].init_deinit);
        LED_ASSERT(itf_list[i].on_off);
        itf_list[i].init_deinit(true);
    }

    memset(state_list, 0, sizeof(led_state_t)* num_of_leds);
}

static void LED_update_state(led_state_t *state, const led_itf_t *itf)
{
    // TODO: assert state pattern
    LED_ASSERT(state->pattern);

    // §°§Ò§ß§à§Ó§Ý§ñ§Ö§Þ §Ú§ß§Õ§Ö§Ü§ã §ê§Ñ§Ò§Ý§à§ß§Ñ
    uint8_t new_idx = state->pattern_idx + 1;
    uint8_t pattern_size = state->pattern[0];
    state->pattern_idx = new_idx % pattern_size;


    // §¦§ã§Ý§Ú §ã§é§Ö§ä§é§Ú§Ü §á§à§Ó§ä§à§â§Ö§ß§Ú§Û §ß§Ö§ß§å§Ý§Ö§Ó§à§Û, §ä§à §à§Ò§ß§à§Ó§Ý§ñ§Ö§Þ §ã§à§ã§ä§à§ñ§ß§Ú§Ö
    if(state->repeat_count)
    {
        // §Ö§ã§Ý§Ú §á§Ñ§ä§ä§Ö§â§ß §Õ§à§Ý§Ø§Ö§ß §Ó§à§ã§á§â§à§Ú§Ù§Ó§à§Õ§Ú§ä§î§ã§ñ §Ó§ã§Ö§Ô§Õ§Ñ, §ä§à §ß§Ö
        // §Õ§Ö§Ü§â§Ö§Þ§Ö§ß§ä§Ú§â§å§Ö§Þ §á§Ö§â§Ö§Þ§Ö§ß§ß§å§ð
        if(state->repeat_count != LED_REPEAT_INFINITE)
        {
            state->repeat_count -= new_idx / pattern_size;
        }

        uint8_t new_state = LED_STATE(state);
        itf->on_off(new_state);
        uint16_t delay = state->pattern[state->pattern_idx + 1];
        state->deadline = led_tick_count() + delay;
    }
    else
    {
        state->deadline = led_tick_count() + LED_MAX_WIDTH;
    }
}

static void LED_schedule(led_control_block_t *block)
{
    uint8_t num_of_leds = block->num_of_leds;

    int32_t nearest_deadline = led_tick_count() + LED_MAX_WIDTH;
    bool any_active = false;

    for(int i = 0; i < num_of_leds; i++)
    {
        led_state_t *led_state = &(block->state_list[i]);
        const led_itf_t *led_itf = &(block->itf_list[i]);
        if(led_state->repeat_count)
        {
            any_active = true;
            bool timer_expired = led_state->deadline - led_tick_count() <= 0;
            if(timer_expired)
            {
                LED_update_state(led_state, led_itf);
            }
            if(led_state->deadline - nearest_deadline <= 0)
            {
                nearest_deadline = led_state->deadline;
            }
        }
    }

    if(any_active)
    {
        int32_t time_delay = nearest_deadline - led_tick_count();

        if(time_delay <= 0)
        {
            time_delay = 1;
        }

        led_set_timer(time_delay);
    }
    else
    {
        led_timer_stop();
    }
}

void LED_on_event(led_control_block_t *block, const led_event_t *event)
{
    switch(event->type)
    {
        case LED_EVENT_NEW_STATE:
        {
            const led_event_new_state_t *new_state = &(event->new_state);
            uint8_t led_num = new_state->led_num;
            uint8_t num_of_leds = block->num_of_leds;
            if(led_num < num_of_leds)
            {
                led_state_t *led_state = &(block->state_list[led_num]);
                const led_itf_t *led_itf = &(block->itf_list[led_num]);
                led_state->pattern = new_state->pattern;
                led_state->repeat_count = new_state->repeat_count;
                led_state->initial_state = new_state->initial_state;
                led_state->pattern_idx = 0;
                led_itf->on_off(led_state->initial_state);
                uint16_t delay = led_state->pattern[0 + 1];
                led_state->deadline = led_tick_count() + delay;
                LED_schedule(block);
            }
        }break;
        case LED_EVENT_TIMER:
        {
            LED_schedule(block);
        }break;
        default:
        {
            break;
        }
    }
}



/***************************************************************************************************
 *                                           GLOBAL VARIABLES
 ***************************************************************************************************/


/***************************************************************************************************
 *                                            LOCAL FUNCTION
 ***************************************************************************************************/


/***************************************************************************************************
 *                                            FUNCTIONS - API
 ***************************************************************************************************/



