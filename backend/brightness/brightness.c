
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bright_backend.h"

#define SYSFS_DIR "/sys/devices/platform/board/board:backlight/backlight/board:backlight"
#define BRIGHT_FILENAME SYSFS_DIR "/brightness"
#define MAX_BRIGHT_FILENAME SYSFS_DIR "/max_brightness"
#define BLANKING_FILENAME "/sys/class/graphics/fb0/blank"

#define MIN_BRIGHTNESS 1
#define STEP_VALUE 1

static int max_brightness = -1;
static int current_value = -1;
static FILE *dev_file = NULL;

static int dev_file_open(void)
{
	if (!dev_file) {
		dev_file = fopen(BRIGHT_FILENAME, "r+");
		if (!dev_file)
			return -1;
		// Make stream unbuffered.
		setbuf(dev_file, NULL);
	}
	return 0;
}

static void dev_file_close(void)
{
	if (dev_file) {
		fclose(dev_file);
		dev_file = NULL;
	}
}

static int get_brightness(void)
{
	int val;
	int num_read;

	if (dev_file_open() < 0)
		return -1;

	num_read = fscanf(dev_file, "%d\n", &val);
	if (num_read == 0)
		return -1;

	return val;
}

static void set_brightness(int value)
{
	if (dev_file_open() < 0)
		return ;

	fprintf(dev_file, "%d\n", value);
	fflush(dev_file);
}

static void max_brightness_init()
{
	// We retrieve the maximum brightness parameter.

	FILE *file;
	int val;
	int num_read;

	file = fopen(MAX_BRIGHT_FILENAME, "r");
	if (!file)
		return;

	num_read = fscanf(file, "%d\n", &val);
	if (num_read != 0)
		max_brightness = val;

	fclose(file);
}

static bright_adjust(int event_value, int delta)
{
	int new_value;

	if (max_brightness == -1) {
		max_brightness_init();
		if (max_brightness == -1)
			return;
	}

	if (event_value == 0) { // key release
		dev_file_close();
		return;
	}

	if (event_value == 1) { // new key press (not a repeat)
		// Make sure we fetch an up-to-date value in the next step.
		dev_file_close();
		current_value = -1;
	}

	if (current_value == -1) {
		current_value = get_brightness();
		if (current_value == -1)
			return;
	}

	new_value = current_value + delta;
	if (new_value < MIN_BRIGHTNESS)
		new_value = MIN_BRIGHTNESS;
	else if (new_value > max_brightness)
		new_value = max_brightness;

	if (new_value != current_value) {
		current_value = new_value;
		set_brightness(current_value);
	}
}

void bright_up(int event_value)
{
	bright_adjust(event_value, STEP_VALUE);
}

void bright_down(int event_value)
{
	bright_adjust(event_value, -STEP_VALUE);
}

void blank(int enable)
{
	FILE *file = fopen(BLANKING_FILENAME, "w");
	if (!file)
		return;

	fprintf(file, "%d\n", enable);
	fclose(file);
}
