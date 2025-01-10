/** event.h -- Event
 */
#ifndef evt_queue_h
#define evt_queue_h
#include "active_common.h"

typedef struct
{
    Msg **pMsgList;
    uint16_t list_size;
    uint16_t head;
    uint16_t tail;
}
EventQueue;

void EventQueue_ctor(EventQueue * const me, Msg **MsgList, uint16_t MsgSize);

bool EventQueue_push_to_head(EventQueue * const me, Msg const * const msg);

bool EventQueue_push_to_tail(EventQueue * const me, Msg const * const msg);

Msg const * const EventQueue_pop_from_head(EventQueue * const me);

#endif /* evt_queue_h */
