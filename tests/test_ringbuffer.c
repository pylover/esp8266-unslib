#include <stdlib.h>
#include <stdio.h>

#include "ringbuffer.h"
#include "testing.h"


#define SIZE    10


int main() {
    char t[9];
    char *buffer = (char*)malloc(SIZE);
    RingBuffer rb = {SIZE, 0, 0, buffer};
    rb_push(&rb, "ABCD", 4);
    eqint(rb_used(&rb), 4);
    eqint(rb_available(&rb), 5);
    eqint(rb.head, 0);
    eqint(rb.tail, 4);
    
    rb_push(&rb, "EFGHI", 5);
    rb_push(&rb, "JKLMN", 5);
    eqint(rb_used(&rb), 9);
    eqint(rb_available(&rb), 0);
    eqint(rb.head, 5);
    eqint(rb.tail, 4);

    rb_pop(&rb, t, 9);
    eqstr(t, "FGHIJKLMN");
    eqint(rb_used(&rb), 0);
    eqint(rb_available(&rb), 9);
    eqint(rb.head, 4);
    eqint(rb.tail, 4);

    free(buffer);
}
