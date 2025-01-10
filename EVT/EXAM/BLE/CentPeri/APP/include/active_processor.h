#ifndef active_processor_h
#define active_h

#include "active_common.h"

void ActiveObj_register(ActiveObj * const AObj);


// Used in active posts
void ActiveProcessor_ImmutablePosted(ActiveObj * const AObj);

#endif /* active_h */
