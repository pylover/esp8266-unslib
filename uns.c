#include "uns.h"

#include <mem.h>
#include <osapi.h>
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <user_interface.h>


#define cache_prev(i) abs((cache_last - (i)) % UNS_MAX_HOST_CACHE)
#define cache_next(i) abs((cache_last + (i)) % UNS_MAX_HOST_CACHE)

#define pendings_prev(i) abs((pendings_last - (i)) % UNS_MAX_PENDINGS)
#define pendings_next(i) abs((pendings_last + (i)) % UNS_MAX_PENDINGS)

#define gettime() (system_get_time() / 1000)
#define MIN(x, y) (x) < (y)? (x): (y)


static struct espconn conn;
static char myhostname[UNS_HOSTNAME_MAXLEN];
static struct ip_info ipconfig;
static ip_addr_t igmpaddr;
static struct unsrecord hostcache[UNS_MAX_HOST_CACHE];
static uint8_t cache_last;
static struct unspending pendings[UNS_MAX_PENDINGS];
static uint8_t pendings_last;


ICACHE_FLASH_ATTR 
void uns_cleanup() {
    uint8_t i;
    struct unspending *p;
    uint32_t ct = gettime();
    //os_printf("Cleaning up pending records: %09d.\n", gettime());
    for (i = 0; i < UNS_MAX_PENDINGS; i++) {
        p = &pendings[pendings_prev(i)];
        if ((ct - p->time) > UNS_TIMEOUT) {
            os_memset(p, 0, sizeof(struct unspending));
        }
    }
}


static ICACHE_FLASH_ATTR 
struct unspending * _pendingfind(const char *name, uint16_t len) {
    uint8_t i;
    struct unspending *p;

    for (i = 0; i < UNS_MAX_PENDINGS; i++) {
        p = &pendings[pendings_prev(i)];
        if (p->callback == NULL) {
            continue;
        }
        if (os_strncmp(p->pattern, name, MIN(p->patternlen, len)) == 0) {
            return p;
        }
    }
    return NULL;
}


static ICACHE_FLASH_ATTR 
struct unsrecord * _cachefind(const char *name, uint16_t len) {
    uint8_t i;
    uint8_t index;

    for (i = 0; i < UNS_MAX_HOST_CACHE; i++) {
        index = cache_prev(i);
        if (os_strncmp(hostcache[i].fullname, name, len)) {
            continue;
        }
        return &hostcache[i];
    }
    return NULL;
}


static ICACHE_FLASH_ATTR 
struct unsrecord * _cachestore(char *name, uint16_t len, 
        struct ip_addr* addr) {
    uint8_t i = cache_next(1);
    struct unsrecord *rec = &hostcache[i];
    os_sprintf(rec->fullname, name, len);
    rec->address.addr = addr->addr;
    cache_last = i;
    return rec; 
}


static ICACHE_FLASH_ATTR 
void _answer(char *hostname, uint16_t len, remot_info *remoteinfo) {
    hostname[len] = 0;

    struct unsrecord *r = _cachefind(hostname, len);
    if (r) {
        os_memcpy(&r->address, remoteinfo->remote_ip, 4);
    }
    else {
        r = _cachestore(hostname, len, (ip_addr_t *)remoteinfo->remote_ip);
    }
   
    struct unspending *p; 
    while (true) {
        p = _pendingfind(hostname, len); 
        if (p == NULL) {
            break;
        }
        if (p->callback) {
            p->callback(r);
        }
        os_memset(p, 0, sizeof(struct unspending));
    }
}


ICACHE_FLASH_ATTR 
err_t uns_discover(const char *pattern, unscallback cb) {
    err_t err;
    char req[UNS_REQUEST_BUFFER_SIZE];
    req[0] = UNS_VERB_DISCOVER;
    int hostnamelen = os_strlen(pattern);
    os_strcpy(req + 1, pattern);
    
    /* Try current cache items. */
    struct unsrecord *r = _cachefind(pattern, hostnamelen);
    if (r) {
        cb(r);
        return;
    }

    /* Item not found in cache, Add a pending item */
    uint8_t pindex = pendings_next(1);
    struct unspending *p = &pendings[pindex];
    os_strncpy(p->pattern, pattern, hostnamelen);
    p->patternlen = hostnamelen;
    p->callback = cb;
    p->time = gettime();
    pendings_last = pindex;

    /* Send discover packet */
    os_memcpy(conn.proto.udp->remote_ip, &igmpaddr, 4);
    conn.proto.udp->remote_port = UNS_IGMP_PORT;
    err = espconn_sent(&conn, req, strlen(req));
    if(err) {
        return err;
    }
    return OK; 
}


static ICACHE_FLASH_ATTR
void _req_discover(char *pattern, uint16_t len, remot_info *remoteinfo) {
    if (os_strncmp(myhostname, pattern, len)) {
        /* Ignore, It's not me */
        return;
    }

    /* Create Response */
    char resp[UNS_RESPONSE_BUFFER_SIZE];
    os_memset(resp, 0, UNS_RESPONSE_BUFFER_SIZE);
    resp[0] = UNS_VERB_ANSWER;
    int resplen = os_sprintf(resp + 1, "%s", myhostname);
    
    /* Send Back */
    os_memcpy(conn.proto.udp->remote_ip, remoteinfo->remote_ip, 4);
    conn.proto.udp->remote_port = remoteinfo->remote_port;
    espconn_sent(&conn, resp, resplen + 1);
}


static ICACHE_FLASH_ATTR
void _recv(void *arg, char *data, uint16_t length) {
    remot_info *remoteinfo = NULL;

    struct espconn *c = (struct espconn*) arg;
    if (espconn_get_connection_info(c, &remoteinfo, 0)) {
        /* Remote info problem */
        return;
    }
    os_printf("Packet from: "IPSTR":%d len: %d\r\n", 
            IP2STR(remoteinfo->remote_ip),
            remoteinfo->remote_port,
            length
        );

    if (length < 1) {
        /* Malformed request, Just ignoring */
        os_printf("Malformed IGMP packet: %d Bytes\r\n", length);
        return;
    }

    if (data[0] == UNS_VERB_DISCOVER) {
        _req_discover(data + 1, length - 1, remoteinfo);
    }
    else if (data[0] == UNS_VERB_ANSWER) {
        _answer(data + 1, length - 1, remoteinfo);
    }
}


ICACHE_FLASH_ATTR 
err_t uns_init(const char *hostname) {
    err_t err;
    os_strcpy(myhostname, hostname);
    conn.type = ESPCONN_UDP;
    conn.state = ESPCONN_NONE;
    conn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    conn.proto.udp->local_port = UNS_IGMP_PORT;

    err = espconn_regist_recvcb(&conn, _recv);
    if (err) {
        return err;
    }
    
    err = espconn_create(&conn);
    if (err) {
        return err;
    }
    
    wifi_get_ip_info(STATION_IF, &ipconfig);
    ipaddr_aton(UNS_IGMP_ADDR, &igmpaddr);

    err = espconn_igmp_join(&ipconfig.ip, &igmpaddr);
    if (err) {
        return err;
    }
    return ESPCONN_OK;
}


ICACHE_FLASH_ATTR 
err_t uns_deinit() {
    espconn_igmp_leave(&ipconfig.ip, &igmpaddr);
    os_free(conn.proto.udp);
    return espconn_delete(&conn); 
}
 
