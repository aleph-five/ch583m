#include "config.h"
#include "active_common.h"

#define TMOS_SYS_MSG_ACTIVE_OBJECT                10
#define TMOS_EVENT_POST_IMMUTABLE                 (0x0001)
#define TMOS_ACTIVE_OBJ_MSG_SIZE                  sizeof(TMOSActiveObjMsg)
#define TMOS_ACTIVE_OBJ_HANDLER_TASK_ID           ActiveObjProcessorTaskID



tmosTaskID ActiveObjProcessorTaskID = INVALID_TASK_ID;

typedef struct
{
    tmos_event_hdr_t hdr;
    ActiveObj * pObj;
}
TMOSActiveObjMsg;

static ActiveObj * AObjList[ACTIVE_OBJECT_MAX];
static uint16_t NumOfAObjs;

bool ActiveProcessor_IsInList(ActiveObj * const AObj)
{
    bool in_list = false;
    for(int i = 0; i < NumOfAObjs; i++)
    {
        if(AObj == AObjList[i])
        {
            in_list = true;
            break;
        }
    }
    return in_list;
}

static inline Msg * const RealMsgPtr(TMOSActiveObjMsg const * const TMOSMsg)
{
    uint32_t real_msg_ptr = (uint32_t)TMOSMsg + (uint32_t)TMOS_ACTIVE_OBJ_MSG_SIZE;
    return (Msg *)real_msg_ptr;
}

static inline TMOSActiveObjMsg * const RealTMOSMsgPtr(Msg const *const msg)
{
    uint32_t real_tmos_msg_ptr = (uint32_t)msg - (uint32_t)TMOS_ACTIVE_OBJ_MSG_SIZE;
    return (TMOSActiveObjMsg *)real_tmos_msg_ptr;
}


Msg * Event_allocate(uint16_t size)
{
    ACTIVE_STATIC_ASSERT(TMOS_ACTIVE_OBJ_MSG_SIZE % 4 == 0);
    TMOSActiveObjMsg *tmos_msg = (TMOSActiveObjMsg *)tmos_msg_allocate(TMOS_ACTIVE_OBJ_MSG_SIZE + size);
    if(tmos_msg)
    {
        uint32_t real_msg_ptr = (uint32_t)tmos_msg + (uint32_t)TMOS_ACTIVE_OBJ_MSG_SIZE;
        return (Msg *)real_msg_ptr;
    }
    else
    {
      return (Msg *)0;
    }
}

void ActiveObj_register(ActiveObj * const AObj)
{
    ACTIVE_OBJ_ASSERT(NumOfAObjs < ARR_DIMENSION(AObjList));
    AObjList[NumOfAObjs] = AObj;
    NumOfAObjs++;
}

bool ActiveObj_post_mutable(ActiveObj * const me, Msg const * const msg)
{
    TMOSActiveObjMsg * const tmos_msg = RealTMOSMsgPtr(msg);
    tmos_msg->hdr.event = TMOS_SYS_MSG_ACTIVE_OBJECT;
    tmos_msg->pObj = me;
    bStatus_t status = tmos_msg_send(TMOS_ACTIVE_OBJ_HANDLER_TASK_ID, (uint8_t *)tmos_msg);

    if(status == SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ActiveProcessor_DispatchMutable(TMOSActiveObjMsg const * const tmos_msg)
{
    Msg const * const msg = RealMsgPtr(tmos_msg);
    ActiveObj *AObj = tmos_msg->pObj;
    ACTIVE_OBJ_ASSERT(ActiveProcessor_IsInList(AObj));
    HsmOnEvent(ACTIVE_OBJ_HSM_PTR(AObj), msg);
}

void ActiveProcessor_DispatchImmutable(ActiveObj * const AObj)
{
    Msg const * const msg = EventQueue_pop_from_head(ACTIVE_OBJ_QUEUE_PTR(AObj));
    if(msg != (Msg *)0)
    {
        HsmOnEvent(ACTIVE_OBJ_HSM_PTR(AObj), msg);
    }
}

void ActiveProcessor_Process(void)
{
    for(int i = 0; i < NumOfAObjs; i++)
    {
        ActiveObj * const AObj = AObjList[i];
        ActiveProcessor_DispatchImmutable(AObj);
    }
}

void ActiveProcessor_Start(void)
{
    for(int i = 0; i < NumOfAObjs; i++)
    {
        ActiveObj * const AObj = AObjList[i];
        HsmOnStart(ACTIVE_OBJ_HSM_PTR(AObj));
    }
}

void ActiveProcessor_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        case TMOS_SYS_MSG_ACTIVE_OBJECT:
        {
          ActiveProcessor_DispatchMutable((TMOSActiveObjMsg *)pMsg);
        }break;
        default:
            break;
    }
}

void ActiveProcessor_ImmutablePosted(ActiveObj * const AObj)
{
    (void)AObj;
    tmos_set_event(TMOS_ACTIVE_OBJ_HANDLER_TASK_ID, TMOS_EVENT_POST_IMMUTABLE);
}
