/** active.h -- Active object
 */
#ifndef active_h
#define active_h

#include "active_common.h"

#include "evt_queue.h"


/*struct ActiveObjVTable_tag
{
    uint8_t dummy;
};*/

typedef struct
{
    /*struct ActiveObjVTable_tag const * vtable;*/
    Hsm super;
    EventQueue queue;
}
ActiveObj;


void ActiveObj_ctor(ActiveObj * const me, State const * const TopState, char const * const name, Msg ** MsgList, uint16_t ListSize);

bool ActiveObj_post_fifo_immutable(ActiveObj * const me, Msg const * const msg);

bool ActiveObj_post_lifo_immutable(ActiveObj * const me, Msg const * const msg);

#endif /* active_h */
