
#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include <stdbool.h>
#include <linux/input.h>

#define EVENT_SWITCH_POWER KEY_POWER

int do_listen();
bool power_button_is_pressed(void);
int inject(unsigned short type, unsigned short code, int value);

#endif // EVENT_LISTENER_H
