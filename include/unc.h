#ifndef UNC_H
#define UNC_H

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>

#define UNC_IGMP_ADDR   "224.0.0.70"
//#define UNC_IGMP_ADDR   "192.168.8.157"
#define UNC_IGMP_PORT   5333

#ifndef UNC_RESPONSE_BUFFER_SIZE
#define UNC_RESPONSE_BUFFER_SIZE    0xff
#endif

#ifndef UNC_REQUEST_BUFFER_SIZE
#define UNC_REQUEST_BUFFER_SIZE     0xff
#endif

typedef enum {
    UNC_VERB_DISCOVER = 1,
    UNC_VERB_ANSWER = 2
} uncverb_t;


typedef void (*unc_callback)(char* hostname, int hostnamelen, 
        char* services, int serviceslen, remot_info*);

int8_t unc_init();
int8_t unc_discover(const char*zone, const char *name, unc_callback);
int8_t unc_deinit();

#endif
