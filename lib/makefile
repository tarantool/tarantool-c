CC      = gcc
AR      = ar
RM      = rm
CFLAGS  = -I. -Wall -g
LFLAGS  = 
OBJECTS = file.o crc32.o session.o
all: lib test
lib: $(OBJECTS)
	$(AR) crus libtarantool.a $(OBJECTS)
test: $(OBJECTS) test.o
	$(CC) $(LFLAGS) $(OBJECTS) test.o -o test
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	$(RM) -f $(OBJECTS) test libtarantool.a
