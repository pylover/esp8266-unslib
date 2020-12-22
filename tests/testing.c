#include <stdlib.h>
#include <string.h>

#include "testing.h"

#define SUCCESS(c) if (c) {pokln("%s Ok", __func__); return; }
#define FAILED() perrln("%s Failed", __func__)
#define EXPECTED() pcolor(BLUE, "Expected: ")
#define GIVEN() pcolor(YELLOW, "Given: ")


static void printbinary(const unsigned char *buf, int buflen) {
    int i;
    for (i = 0; i < buflen; i++){
        printf("\\%02X", buf[i]);
    }
    printf("\n");
}


void equalbin(const unsigned char *given, const unsigned char *expected, 
        size_t len) {
    SUCCESS(memcmp(given, expected, len) == 0);

    /* Error */
    FAILED();
    EXPECTED();
    printbinary(expected, len);

    GIVEN();
    printbinary(given, len);

    exit(EXIT_FAILURE);
}



void equalstr(const char *given, const char *expected) {
    SUCCESS(strcmp(given, expected) == 0);

    /* Error */
    FAILED();
    EXPECTED();
    pdataln("%s", expected);

    GIVEN();
    pdataln("%s", given);

    exit(EXIT_FAILURE);
}


void equalint(int given, int expected) {
    SUCCESS(given == expected);

    /* Error */
    FAILED();
    EXPECTED();
    pdataln("%d", expected);

    GIVEN();
    pdataln("%d", given);

    exit(EXIT_FAILURE);
}
