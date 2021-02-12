#include "unc.h"

#include <mem.h>
#include <osapi.h>
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <user_interface.h>


static struct espconn conn;
static struct ip_info ipconfig;
static ip_addr_t igmpaddr;
static unc_callback callback;


static ICACHE_FLASH_ATTR
void unc_answer(char *pattern, uint16_t len, remot_info *remoteinfo) {
    char *hostname = pattern;
    char *services;
    char *temp;
    int hostnamelen;
    int serviceslen;

    temp = os_strstr(pattern, " ");
    if (temp == NULL){
        // Invalid Packet
        os_printf("Invalid packet, ignoring\n");
        return;
    }
    
    os_printf("Answer: %s\n", pattern);
    hostnamelen = temp - hostname;
    hostname[hostnamelen] = 0;
    services = temp + 1;
    
    serviceslen = len - (services - hostname);
    services[serviceslen] = 0;
    if (callback) {
        callback(hostname, hostnamelen, services, serviceslen, remoteinfo);
        callback = NULL;
    }
}

#define IP_FMT    "%d.%d.%d.%d"
#define IPPORT_FMT    IP_FMT":%d"
#define unpack_ip(ip) ip[0], ip[1], ip[2], ip[3]
#define lclinfo(t) unpack_ip((t)->local_ip), (t)->local_port
#define rmtinfo(t) unpack_ip((t)->remote_ip), (t)->remote_port


ICACHE_FLASH_ATTR 
int8_t unc_discover(const char*zone, const char *name, unc_callback cb) {
    err_t err;
    char req[UNC_REQUEST_BUFFER_SIZE];
    req[0] = UNC_VERB_DISCOVER;
    os_sprintf(req + 1, "%s.%s", zone, name);

    /* Update callback pointer, and ignore previous data */
    callback = cb;

    /* Send discover packet */
    os_memcpy(conn.proto.udp->local_ip, &ipconfig, 4);
    //conn.proto.udp->local_port = UNC_IGMP_PORT;
    os_memcpy(conn.proto.udp->remote_ip, &igmpaddr, 4);
    conn.proto.udp->remote_port = UNC_IGMP_PORT;
    os_printf("Sending: %s, to: "IPPORT_FMT"\n", req, rmtinfo(conn.proto.udp));
    os_printf("From: "IPPORT_FMT"\n", lclinfo(conn.proto.udp));
    err = espconn_sent(&conn, req, strlen(req));
    if(err) {
        os_printf("Cannot send UDP: %d\n", err);
    }
    return OK; 
}


static ICACHE_FLASH_ATTR
void unc_recv(void *arg, char *data, uint16_t length) {
    remot_info *remoteinfo = NULL;

    struct espconn *c = (struct espconn*) arg;
    if (espconn_get_connection_info(c, &remoteinfo, 0)) {
        /* Remote info problem */
        return;
    }

    if (data[0] == UNC_VERB_ANSWER) {
        unc_answer(data + 1, length - 1, remoteinfo);
    }
}


ICACHE_FLASH_ATTR 
int8_t unc_init() {
    err_t err;
    conn.type = ESPCONN_UDP;
    conn.state = ESPCONN_NONE;
    conn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    conn.proto.udp->local_port = 5334;

    err = espconn_regist_recvcb(&conn, unc_recv);
    if (err) {
        return err;
    }
    
    err = espconn_create(&conn);
    if (err) {
        return err;
    }
    
    wifi_get_ip_info(STATION_IF, &ipconfig);
    ipaddr_aton(UNC_IGMP_ADDR, &igmpaddr);

    //err = espconn_igmp_join(&ipconfig.ip, &igmpaddr);
    //if (err) {
    //    return err;
    //}
    return ESPCONN_OK;
}


ICACHE_FLASH_ATTR 
int8_t unc_deinit() {
    os_free(conn.proto.udp);
    return espconn_delete(&conn); 
}
 
