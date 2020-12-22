#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <osapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ringbuffer.h"
#include "multipart.h"
#include "testing.h"


char *formheader = 
    "-----------------------------9051914041544843365972754266\r\n"
    "Content-Disposition: form-data; name=\"text\"\r\n"
    "\r\n"
    "text default\r\n"
    "-----------------------------9051914041544843365972754266\r\n"
    "Content-Disposition: form-data; name=\"file1\"; filename=\"a.bin\"\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n";

char *formfooter = 
    "\r\n"
    "-----------------------------9051914041544843365972754266\r\n"
    "Content-Disposition: form-data; name=\"file2\"; filename=\"a.html\"\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<!DOCTYPE html><title>Content of a.html.</title>\r\n"
    "-----------------------------9051914041544843365972754266--\r\n";


#define CONTENTTYPE    "Content-Type: multipart/form-data; boundary=" \
    "---------------------------9051914041544843365972754266\r\n"


#define CHUNKSIZE   1024
#define BIGFILENAME "bigfile.bin"
#define TEMPFILENAME "tempfile.bin"
#define BUFFSIZE    2048


static char buff[BUFFSIZE];
static RingBuffer rb = {BUFFSIZE, 0, 0, buff};
static int tempfile;
static char recchunk[CHUNKSIZE * 2];
static int recchunklen;
static int cbc = 0;


void cb(MultipartField *f, char *body, Size bodylen, bool last) {
    pcolorln(YELLOW, "Chunk (last: %d) #%d - %d", last, cbc++, bodylen);
    if (os_strncmp(f->name, "file1", 5) != 0) {
        return;
    }
    memcpy(recchunk, body, bodylen);
    recchunklen = bodylen;
    int err = write(tempfile, (unsigned char*)body, bodylen);
    if (err < 0) {
        perrln("Cannot write to: %s", TEMPFILENAME);
        exit(1);
    }
}


int _feed_buffer(Multipart *mp, char *data, int offset, Size datalen) {
    char temp[2048];
    memcpy(temp, data + offset, datalen);
    rb_safepush(&rb, temp, datalen);
    return mp_feedbybuffer(mp, &rb);
}

void test_multipart_big() {
    Multipart mp;
    rb_reset(&rb);
    
    mp_init(&mp, CONTENTTYPE, cb);
    eqint(_feed_buffer(&mp, formheader, 0, strlen(formheader)), MP_MORE);

    // Open tempfile for write
    tempfile = open(TEMPFILENAME, O_CREAT | O_WRONLY, S_IRWXU);
    if (tempfile < 0) {
        perrln("Cannot open: %s, ERR: %d", TEMPFILENAME, tempfile);
        return;
    }
   
    // Open bigfile.
    int bigfile = open(BIGFILENAME, O_RDONLY);
    if (bigfile < 0) {
        perrln("Cannot open: %s", BIGFILENAME);
        return;
    }
    int c = 0;
    cbc = 0;
    int readbytes;
    char chunk[CHUNKSIZE];
    while (1) {
        readbytes = read(bigfile, chunk, CHUNKSIZE);    
        if (readbytes == 0) {
            break;
        }
        pcolorln(CYAN, "Chunk #%.3d", c);
        recchunklen = 0;
        eqint(MP_MORE, _feed_buffer(&mp, chunk, 0, readbytes));
        
        //eqint(recchunklen, readbytes);
        //eqint(rb_used(&rb), 0);
        //eqbin(recchunk, chunk, readbytes);
        c++;
    }
   
    eqint(MP_DONE, _feed_buffer(&mp, formfooter, 0, strlen(formfooter)));
    close(bigfile);
    close(tempfile);
    mp_close(&mp);
}


int main() {
    test_multipart_big();
}
