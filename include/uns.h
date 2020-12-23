#ifndef UNS_H
#define UNS_H

#include <c_types.h>

#define UNS_IGMP_ADDR   "224.0.0.77"
#define UNS_IGMP_PORT   5333

#ifndef UNS_SERVICES
#define UNS_SERVICES "u5333"
#endif

#ifndef UNS_RESPONSE_BUFFER_SIZE
#define UNS_RESPONSE_BUFFER_SIZE    0xff
#endif

typedef enum {
    UNS_VERB_DISCOVER = 1
} unsverb_t;


int8_t uns_init(const char *zone, const char *name);
int8_t uns_deinit();

#endif
