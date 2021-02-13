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


typedef enum {
    UNS_VERB_DISCOVER = 1,
    UNS_VERB_ANSWER = 2
} unsverb_t;


typedef void (*uns_callback)(char* hostname, int hostnamelen, remot_info*);


// TODO: Errors; err_t
err_t uns_init(const char *zone, const char *name);
err_t uns_discover(const char*zone, const char *name, uns_callback);
err_t uns_deinit();

#endif
