
CC      = gcc
RM      = rm
CFLAGS  = -I. -Wall -g
LFLAGS  = 
OBJECTS = tfile.o crc32.o test.o
TARGET  = test

$(TARGET): $(OBJECTS)
	$(CC) $(LFLAGS) $(OBJECTS) -o $(TARGET)
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	$(RM) -f $(OBJECTS) $(TARGET)
