#ifndef active_common_h
#define active_common_h
#include <stdint.h>
#include <stdbool.h>
#include "hsm.h"
#include "active.h"
#include "evt_queue.h"
#include "active_processor.h"


#define ACTIVE_OBJECT_MAX                         10

#define ARR_DIMENSION(Arr)                        (sizeof(Arr) / sizeof(Arr[0]))

#define ACTIVE_OBJ_HSM_PTR(ActiveObjPtr)            (&(ActiveObjPtr->super))
#define ACTIVE_OBJ_QUEUE_PTR(ActiveObjPtr)            (&(ActiveObjPtr->queue))

#define ACTIVE_OBJ_ASSERT(expr)                   do { if((expr) == 0) while(1); } while(0)

#define ACTIVE_STATIC_ASSERT(x)

#endif /* active_common_h */
