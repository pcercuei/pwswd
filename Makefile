
TARGET = pwswd

CROSS_COMPILE ?= mipsel-linux-
CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -O2
LDFLAGS = -s
LIBS = -lasound -lpng

OBJS = event_listener.o shortcut_handler.o main.o \
	backend/brightness/brightness.o \
	backend/volume/volume.o \
	backend/poweroff/poweroff.o \
	backend/reboot/reboot.o \
	backend/screenshot/screenshot.o \
	backend/tvout/tvout.o \
	backend/suspend/suspend.o \
	backend/kill/kill.o


.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(addprefix -Wl,,$(LDFLAGS)) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)
