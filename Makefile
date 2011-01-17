
TARGET=pwswbd
OBJS=event_listener.o shortcut_handler.o main.o

OBJS += backend/brightness/brightness.o
OBJS += backend/volume/volume.o
OBJS += backend/poweroff/poweroff.o
OBJS += backend/reboot/reboot.o
OBJS += backend/screenshot/screenshot.o
OBJS += backend/tvout/tvout.o


CROSS=mipsel-linux-uclibc-
CC=$(CROSS)gcc
STRIP=$(CROSS)objcopy


CFLAGS=-Wall -O2
INCLUDES=-I/opt/mipsel-linux-uclibc/usr/include
LIBS=-lasound -lpng


all: $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(TARGET)
	$(STRIP) -S $(TARGET)


clean:
	-rm -rf $(TARGET) $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<
