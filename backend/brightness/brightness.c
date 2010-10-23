
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bright_backend.h"

#define BRIGHT_FILENAME "/sys/class/backlight/pwm-backlight/brightness"
#define MAX_BRIGHT_FILENAME "/sys/class/backlight/pwm-backlight/max_brightness"
#define BLANKING_FILENAME "/sys/class/graphics/fb0/blank"

static FILE *dev_file = NULL;
static int max_brightness;
static int current_value;
static char tmp[0x10];

static void bright_init() {
    // First: we retrieve the maximum brightness parameter.
    dev_file = fopen(MAX_BRIGHT_FILENAME, "r");
    fread(tmp, sizeof(char), 0x10, dev_file);
    max_brightness = atoi(tmp);
    fclose(dev_file);

    // Second: we retrieve the current backlight power value.
    dev_file = fopen(BRIGHT_FILENAME, "r+");
    fread(tmp, sizeof(char), 0x10, dev_file);
    current_value = atoi(tmp);
}

void bright_up() {
    if (!dev_file) bright_init();

    if (current_value == max_brightness)
      return;

    sprintf(tmp, "%i", ++current_value);
//    printf("Value: %s.\n", tmp);
    write(fileno(dev_file), tmp, strlen(tmp)+1);
}

void bright_down() {
    if (!dev_file) bright_init();

    if (current_value == 5)
      return;

    sprintf(tmp, "%i", --current_value);
//    printf("Value: %s.\n", tmp);
    write(fileno(dev_file), tmp, strlen(tmp)+1);
}
