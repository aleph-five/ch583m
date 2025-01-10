#include "active_common.h"

void ActiveObj_ctor(ActiveObj * const me, State const * const TopState, char const * const name, Msg ** MsgList, uint16_t ListSize)
{
    HsmCtorByTopState(ACTIVE_OBJ_HSM_PTR(me), name, TopState);
}

bool ActiveObj_post_fifo_immutable(ActiveObj * const me, Msg const * const msg)
{
    bool status = EventQueue_push_to_head(ACTIVE_OBJ_QUEUE_PTR(me), msg);

    if(status)
    {
      ActiveProcessor_ImmutablePosted(me);
    }
    return status;
}

bool ActiveObj_post_lifo_immutable(ActiveObj * const me, Msg const * const msg)
{
    bool status = EventQueue_push_to_tail(ACTIVE_OBJ_QUEUE_PTR(me), msg);

    if(status)
    {
      ActiveProcessor_ImmutablePosted(me);
    }
    return status;
}
