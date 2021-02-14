#ifndef UNS_H
#define UNS_H

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>

#define UNS_IGMP_ADDR   "224.0.0.70"
#define UNS_IGMP_PORT   5333

#ifndef UNS_RESPONSE_BUFFER_SIZE
#define UNS_RESPONSE_BUFFER_SIZE    0xff
#endif

#ifndef UNS_REQUEST_BUFFER_SIZE
#define UNS_REQUEST_BUFFER_SIZE     0xff
#endif

#ifndef UNS_HOSTNAME_MAXLEN
#define UNS_HOSTNAME_MAXLEN     66
#endif

#ifndef UNS_MAX_HOST_CACHE
#define UNS_MAX_HOST_CACHE      8
#endif

#ifndef UNS_MAX_PENDINGS
#define UNS_MAX_PENDINGS    8
#endif

#ifndef UNS_TIMEOUT
#define UNS_TIMEOUT         5000
#endif


typedef enum {
    UNS_VERB_DISCOVER = 1,
    UNS_VERB_ANSWER = 2
} unsverb_t;


struct unsrecord {
    char fullname[UNS_HOSTNAME_MAXLEN];
    struct ip_addr address;
};


typedef void (*unscallback)(struct unsrecord*);


struct unspending {
    char pattern[UNS_HOSTNAME_MAXLEN];
    uint8_t patternlen;
    uint32_t time;
    unscallback callback;
};


// TODO: Errors; err_t
err_t uns_init(const char *hostname);
err_t uns_discover(const char *hostname, unscallback);
err_t uns_deinit();
err_t uns_invalidate(const char *hostname);
void uns_cleanup();

#endif
