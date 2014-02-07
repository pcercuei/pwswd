
#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include <stdbool.h>
#include <linux/input.h>

#ifdef DEBUG
#define DEBUGMSG(msg...) printf(msg)
#else
#define DEBUGMSG(msg...)
#endif

#define EVENT_SWITCH_POWER KEY_POWER

int do_listen();
bool power_button_is_pressed(void);

#endif // EVENT_LISTENER_H
