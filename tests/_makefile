INCLUDES=-I../include -I./esp -I./include
LIBS=
CC=gcc
CFLAGS=-g -Wall


all: test_ringbuffer test_form_multipart test_form_multipart_big \
	test_querystring

.PHONY: test
test: 
	./test_ringbuffer
	./test_querystring
	./test_form_multipart
	./test_form_multipart_big


.PHONY: clean
clean:
	-rm ./*.o

testing.o: testing.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $^ $(LIBS)
	
ringbuffer.o: ../ringbuffer.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $^ $(LIBS)

multipart.o: ../multipart.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $^ $(LIBS)

querystring.o: ../querystring.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $^ $(LIBS)

test_ringbuffer: test_ringbuffer.c ringbuffer.o testing.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

test_form_multipart: test_form_multipart.c ringbuffer.o multipart.o testing.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

test_form_multipart_big: test_form_multipart_big.c ringbuffer.o multipart.o \
	testing.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

test_querystring: test_querystring.c ringbuffer.o querystring.o testing.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

