#include "uns.h"

#include <mem.h>
#include <osapi.h>
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <user_interface.h>


static struct espconn conn;
static char hostname[UNS_HOSTNAME_MAXLEN];
static struct ip_info ipconfig;
static ip_addr_t igmpaddr;
static uns_callback callback;

struct uns_record {
    char fullname[UNS_HOSTNAME_MAXLEN];
    struct ip_info address;
};


static ICACHE_FLASH_ATTR 
void _answer(char *pattern, uint16_t len, remot_info *remoteinfo) {
    char *fullname = pattern;
    char *temp;
    int hostnamelen;

    temp = os_strstr(pattern, " ");
    if (temp == NULL){
        // Invalid Packet
        os_printf("Invalid packet, ignoring\n");
        return;
    }

    os_printf("Answer: %s\n", pattern);
    hostnamelen = temp - fullname;
    fullname[hostnamelen] = 0;

    if (callback) {
        callback(fullname, hostnamelen, remoteinfo);
        callback = NULL;
    }
}


ICACHE_FLASH_ATTR 
err_t uns_discover(const char*zone, const char *name, uns_callback cb) {
    err_t err;
    char req[UNS_REQUEST_BUFFER_SIZE];
    req[0] = UNS_VERB_DISCOVER;
    os_sprintf(req + 1, "%s.%s", zone, name);

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
    if (os_strncmp(hostname, pattern, len)) {
        /* Ignore, It's not me */
        return;
    }

    os_printf("%s\n", pattern);
    /* Create Response */
    char resp[UNS_RESPONSE_BUFFER_SIZE];
    os_memset(resp, 0, UNS_RESPONSE_BUFFER_SIZE);
    os_sprintf(resp, "%s", hostname);
    
    /* Send Back */
    os_memcpy(conn.proto.udp->remote_ip, remoteinfo->remote_ip, 4);
    conn.proto.udp->remote_port = remoteinfo->remote_port;
    espconn_sent(&conn, resp, strlen(resp));
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
err_t uns_init(const char *zone, const char *name) {
    err_t err;
    os_sprintf(hostname, "%s.%s", zone, name);
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
 
