#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <osapi.h>

#include "ringbuffer.h"
#include "multipart.h"
#include "testing.h"


char *sample = 
    "-----------------------------9051914041544843365972754266\r\n"
    "Content-Disposition: form-data; name=\"text\"\r\n"
    "\r\n"
    "text default\r\n"
    "-----------------------------9051914041544843365972754266\r\n"
    "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    "Content of a.txt.\r\n"
    "-----------------------------9051914041544843365972754266\r\n"
    "Content-Disposition: form-data; name=\"file2\"; filename=\"a.html\"\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<!DOCTYPE html><title>Content of a.html.</title>\r\n"
    "-----------------------------9051914041544843365972754266--\r\n";


#define CONTENTTYPE    "Content-Type: multipart/form-data; boundary=" \
    "---------------------------9051914041544843365972754266\r\n"

#define BUFFSIZE    2048

static char buff[BUFFSIZE];
static RingBuffer rb = {BUFFSIZE, 0, 0, buff};
static char result[BUFFSIZE] = {0};


void cb(MultipartField *f, char *body, Size bodylen, bool last) {
    char value[bodylen + 1];

    memset(value, 0, bodylen + 1);
    strncpy(value, body, bodylen);
    
    os_sprintf(result + strlen(result), "%s\r\n%s\r\n", f->name, value);
}


int _feed_buffer(Multipart *mp, char *data, int offset, Size datalen) {
    char temp[2048];
    memset(temp, 0, 2048);
    strncpy(temp, data + offset, datalen);
    rb_safepush(&rb, temp, datalen);
    return mp_feedbybuffer(mp, &rb);
}


void test_multipart_whole() {
    Multipart mp;
    rb_reset(&rb);
    memset(&result, 0, BUFFSIZE);
    
    mp_init(&mp, CONTENTTYPE, cb);
    eqint(MP_DONE, _feed_buffer(&mp, sample, 0, strlen(sample)));

    eqstr(
        result, 
        "text\r\n"
        "text default\r\n"
        "file1\r\n"
        "Content of a.txt.\r\n"
        "file2\r\n"
        "<!DOCTYPE html><title>Content of a.html.</title>\r\n"
    );
    mp_close(&mp);
}


void test_multipart_chunked() {
    Multipart mp;
    rb_reset(&rb);
    memset(&result, 0, BUFFSIZE);
    
    eqint(MP_OK, mp_init(&mp, CONTENTTYPE, cb));
    eqint(MP_MORE, _feed_buffer(&mp, sample, 0, 50));
    eqint(MP_MORE, _feed_buffer(&mp, sample, 50, 70));
    eqint(MP_DONE, _feed_buffer(&mp, sample, 120, strlen(sample) - 120));
    
    eqstr(
        result, 
        "text\r\n"
        "text default\r\n"
        "file1\r\n"
        "Content of a.txt.\r\n"
        "file2\r\n"
        "<!DOCTYPE html><title>Content of a.html.</title>\r\n"
    );
    mp_close(&mp);
}

char *shortform = 
    "-----------------------------9051914041544843365972754266\r\n"
    "Content-Disposition: form-data; name=\"a\"\r\n"
    "\r\n"
    "b\r\n"
    "-----------------------------9051914041544843365972754266--\r\n";


void test_multipart_short() {
    Multipart mp;
    rb_reset(&rb);
    memset(&result, 0, BUFFSIZE);
    
    eqint(MP_OK, mp_init(&mp, CONTENTTYPE, cb));
    eqint(MP_DONE, _feed_buffer(&mp, shortform, 0, strlen(shortform)));
    
    eqstr(
        result, 
        "a\r\n"
        "b\r\n"
    );
    mp_close(&mp);

}


int main() {
    test_multipart_whole();
    test_multipart_chunked();
    test_multipart_short();
}

