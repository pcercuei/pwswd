
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bright_backend.h"

#define BRIGHT_FILENAME "/sys/class/backlight/pwm-backlight/brightness"
#define MAX_BRIGHT_FILENAME "/sys/class/backlight/pwm-backlight/max_brightness"
#define BLANKING_FILENAME "/sys/class/graphics/fb0/blank"

#define MIN_BRIGHTNESS 5
#define STEP_VALUE 10

static int max_brightness = 0;
static int current_value = 0;
static FILE *dev_file = NULL;

static int get_current_value(FILE *dev)
{
	char tmp[0x10];

	fread(tmp, sizeof(char), 0x10, dev);
	return atoi(tmp);
}

static void set_brightness(FILE *dev, int value)
{
	char tmp[0x10];

	sprintf(tmp, "%i", value);
	write(fileno(dev), tmp, strlen(tmp)+1);
}

static void bright_init() {
	// We retrieve the maximum brightness parameter.

	FILE *file;
	char tmp[0x10];

	file = fopen(MAX_BRIGHT_FILENAME, "r");
	fread(tmp, sizeof(char), 0x10, file);
	max_brightness = atoi(tmp);
	fclose(file);
}

void bright_up(int event_value) {
	if (!max_brightness) bright_init();

	// If the button has been released, we simply close the BRIGHT_FILENAME file.
	if (event_value == 0) {
		fclose(dev_file);
		return;
	}

	// If the button has been pressed, update the current_value variable.
	else if (event_value == 1) {
		dev_file = fopen(BRIGHT_FILENAME, "r+");
		current_value = get_current_value(dev_file);
	}

	if (current_value == max_brightness)
	  return;

	else if (current_value + STEP_VALUE >= max_brightness)
	  current_value = max_brightness;

	else
	  current_value += STEP_VALUE;

	set_brightness(dev_file, current_value);
}

void bright_down(int event_value) {
	if (!max_brightness) bright_init();

	// If the button has been released, we simply close the BRIGHT_FILENAME file.
	if (event_value == 0) {
		fclose(dev_file);
		return;
	}

	// If the button has been pressed, update the current_value variable.
	else if (event_value == 1) {
		dev_file = fopen(BRIGHT_FILENAME, "r+");
		current_value = get_current_value(dev_file);
	}

	if (current_value == MIN_BRIGHTNESS)
	  return;

	else if (current_value - STEP_VALUE <= MIN_BRIGHTNESS)
	  current_value = MIN_BRIGHTNESS;

	else
	  current_value -= STEP_VALUE;

	set_brightness(dev_file, current_value);
}
