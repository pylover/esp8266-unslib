#ifndef _OSAPI_H
#define _OSAPI_H

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>

#define os_printf   printf
#define os_sprintf  sprintf
#define os_strstr    strstr
#define os_strcpy    strcpy
#define os_strncpy    strncpy
#define os_strncmp    strncmp
#define os_strlen    strlen

#define ICACHE_FLASH_ATTR
#endif

