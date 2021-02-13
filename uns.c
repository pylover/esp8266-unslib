#include "uns.h"

#include <mem.h>
#include <osapi.h>
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <user_interface.h>

static struct espconn conn;
static char myhostname[UNS_HOSTNAME_MAXLEN];
static struct ip_info ipconfig;
static ip_addr_t igmpaddr;
static uns_callback callback;
static struct unsrecord hostcache[UNS_MAX_HOST_CACHE];
static uint8_t cache_lastindex;


#define cache_previtem(i) abs((cache_lastindex - i) % UNS_MAX_HOST_CACHE)
#define cache_nextitem(i) abs((cache_lastindex + i) % UNS_MAX_HOST_CACHE)


static ICACHE_FLASH_ATTR 
struct unsrecord * _cachefind(const char *name, uint16_t len) {
    uint8_t i;
    uint8_t index;

    for (i = 0; i < UNS_MAX_HOST_CACHE; i++) {
        index = cache_previtem(i);
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
    struct unsrecord *rec = &hostcache[cache_nextitem(1)];
    os_sprintf(rec->fullname, name, len);
    rec->address.addr = addr->addr;
    os_printf("Cache stored: %s\n", name);
    return rec; 
}


static ICACHE_FLASH_ATTR 
void _answer(char *hostname, uint16_t len, remot_info *remoteinfo) {
    hostname[len] = 0;

    os_printf("Answer: %s\n", hostname);
    
    struct unsrecord *r = _cachefind(hostname, len);
    if (r) {
        os_memcpy(&r->address, remoteinfo->remote_ip, 4);
        //r->address.addr = ((ip_addr_t *)remoteinfo->remote_ip)->addr;
    }
    else {
        r = _cachestore(hostname, len, (ip_addr_t *)remoteinfo->remote_ip);
    }

    if (callback) {
        callback(r);
        callback = NULL;
    }
}


ICACHE_FLASH_ATTR 
err_t uns_discover(const char *hostname, uns_callback cb) {
    err_t err;
    char req[UNS_REQUEST_BUFFER_SIZE];
    req[0] = UNS_VERB_DISCOVER;
    int hostnamelen = os_strlen(hostname);
    os_strcpy(req + 1, hostname);
    
    /* Try current cache items. */
    struct unsrecord *r = _cachefind(hostname, hostnamelen);
    if (r) {
        cb(r);
        return;
    }

    /* Update callback pointer, and ignore previous data */
    callback = cb;

    /* Send discover packet */
    os_memcpy(conn.proto.udp->remote_ip, &igmpaddr, 4);
    conn.proto.udp->remote_port = UNS_IGMP_PORT;
    err = espconn_sent(&conn, req, strlen(req));
    if(err) {
        os_printf("Cannot send UDP: %d\n", err);
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

    os_printf("%s\n", pattern);
    /* Create Response */
    char resp[UNS_RESPONSE_BUFFER_SIZE];
    os_memset(resp, 0, UNS_RESPONSE_BUFFER_SIZE);
    resp[0] = UNS_VERB_ANSWER;
    int resplen = os_sprintf(resp + 1, "%s", myhostname);
    
    /* Send Back */
    os_memcpy(conn.proto.udp->remote_ip, remoteinfo->remote_ip, 4);
    conn.proto.udp->remote_port = remoteinfo->remote_port;
    os_printf("Sending: %s to "IPSTR"\n", resp, IP2STR(remoteinfo->remote_ip));
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
 
