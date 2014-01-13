#include <stdio.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "event_listener.h"
#include "shortcut_handler.h"
#include "backend/backends.h"

#ifdef DEBUG
#define DEBUGMSG(msg...) printf(msg)
#else
#define DEBUGMSG(msg...)
#endif

/* Time in seconds before poweroff when holding the power switch.
 * Set to 0 to disable this feature. */
#ifndef POWEROFF_TIMEOUT
#define POWEROFF_TIMEOUT 3
#endif

#if (POWEROFF_TIMEOUT > 0)
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#endif

static struct uinput_user_dev uud = {
	.name = "OpenDingux mouse daemon",
	.id = { BUS_USB, 1,1,1 },
};

enum _mode {
	NORMAL, MOUSE, HOLD
};

static enum _mode mode = NORMAL;

struct button buttons[NB_BUTTONS] = {
	_BUTTON(UP),
	_BUTTON(DOWN),
	_BUTTON(LEFT),
	_BUTTON(RIGHT),
	_BUTTON(A),
	_BUTTON(B),
	_BUTTON(X),
	_BUTTON(Y),
	_BUTTON(L),
	_BUTTON(R),
	_BUTTON(SELECT),
	_BUTTON(START),
	_BUTTON(POWER),
	_BUTTON(HOLD),
};


static FILE *event0 = NULL;
static FILE *uinput = NULL;

static bool grabbed, power_button_pressed;

static void switchmode(enum _mode new)
{
	if (new == mode) return;

	switch(mode) {
		case NORMAL:
			switch(new) {
				case MOUSE:
					// Enable non-blocking reads: we won't have to wait for an
					// event to process mouse emulation.
					if (fcntl(fileno(event0), F_SETFL, O_NONBLOCK) == -1)
						perror(__func__);
				case HOLD:
					grabbed = true;
					blank(1);
				default:
					mode = new;
					return;
			}
			break;
		case MOUSE:
			// Disable non-blocking reads.
			if (fcntl(fileno(event0), F_SETFL, 0) == -1)
				perror(__func__);
		case HOLD:
			switch(new) {
				case NORMAL:
					grabbed = false;
				default:
					mode = new;
					blank(0);
					return;
			}
		default:
			break;
	}
}


static void execute(enum event_type event, int value)
{
	char *str;
	switch(event) {
#ifdef BACKEND_REBOOT
		case reboot:
			if (value != 1) return;
			str = "reboot";
			do_reboot();
			break;
#endif
#ifdef BACKEND_POWEROFF
		case poweroff:
			if (value == 2) return;
			str = "poweroff";
			do_poweroff();
			break;
#endif
#ifdef BACKEND_SUSPEND
		case suspend:
			if (value != 1) return;
			str = "suspend";
			do_suspend();
			break;
#endif
		case hold:
			if (value != 1) return;
			str = "hold";
			if (mode == HOLD)
			  switchmode(NORMAL);
			else
			  switchmode(HOLD);
			break;
#ifdef BACKEND_VOLUME
		case volup:
			str = "volup";
			vol_up(value);
			break;
		case voldown:
			str = "voldown";
			vol_down(value);
			break;
#endif
#ifdef BACKEND_BRIGHTNESS
		case brightup:
			str = "brightup";
			bright_up(value);
			break;
		case brightdown:
			str = "brightdown";
			bright_down(value);
			break;
#endif
		case mouse:
			if (value != 1) return;
			str = "mouse";
			if (mode == MOUSE)
			  switchmode(NORMAL);
			else
			  switchmode(MOUSE);
			break;
#ifdef BACKEND_TVOUT
		case tvout:
			if (value != 1) return;
			str = "tvout";
			tv_out();
			break;
#endif
#ifdef BACKEND_SCREENSHOT
		case screenshot:
			if (value != 1) return;
			str = "screenshot";
			do_screenshot();
			break;
#endif
#ifdef BACKEND_KILL
		case kill:
			if (value != 1) return;
			str = "kill";
			do_kill();
			break;
#endif
		default:
			return;
	}
	DEBUGMSG("Execute: %s.\n", str);
}


static int open_fds(const char *event0fn, const char *uinputfn)
{
	event0 = fopen(event0fn, "r");
	if (!event0) {
		perror("opening event0");
		return -1;
	}

	uinput = fopen(uinputfn, "r+");
	if (!uinput) {
		perror("opening uinput");
		return -1;
	}

	int fd = fileno(uinput);
	write(fd, &uud, sizeof(uud));

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT) == -1) goto filter_fail;

	if (ioctl(fd, UI_SET_KEYBIT, BUTTON_X) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_KEYBIT, BUTTON_Y) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_KEYBIT, BUTTON_L) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_KEYBIT, BUTTON_R) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_KEYBIT, BUTTON_START) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_KEYBIT, BUTTON_SELECT) == -1) goto filter_fail;

	if (ioctl(fd, UI_SET_EVBIT, EV_REL) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_RELBIT, REL_X) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_RELBIT, REL_Y) == -1) goto filter_fail;
	if (ioctl(fd, UI_SET_RELBIT, REL_WHEEL) == -1) goto filter_fail;

	if (ioctl(fd, UI_SET_KEYBIT, BTN_MOUSE) == -1) goto filter_fail;

	if (ioctl(fd, UI_DEV_CREATE)) {
		perror("creating device");
		return -1;
	}

	return 0;

filter_fail:
	perror("setting event filter bits");
	return -1;
}


static int inject(unsigned short type, unsigned short code, int value)
{
	struct input_event inject_event;

	DEBUGMSG("Injecting event.\n");
	inject_event.value = value;
	inject_event.type = type;
	inject_event.code = code;
	inject_event.time.tv_sec = time(0);
	inject_event.time.tv_usec = 0;
	return write(fileno(uinput), &inject_event, sizeof(struct input_event));
}


#if (POWEROFF_TIMEOUT > 0)
struct poweroff_thd_args {
	pthread_mutex_t mutex, mutex2;
	unsigned long timeout;
	unsigned char canceled;
};

static void * poweroff_thd(void *p)
{
	struct poweroff_thd_args *args = p;
	unsigned long *args_timeout = &args->timeout;
	unsigned char *canceled = &args->canceled;

	while (1) {
		unsigned long time_us;
		long delay;

		pthread_mutex_unlock(&args->mutex2);
		pthread_mutex_lock(&args->mutex);

		do {
			struct timeval tv;

			gettimeofday(&tv, NULL);
			time_us = tv.tv_sec * 1000000 + tv.tv_usec;
			delay = *args_timeout - time_us;

			DEBUGMSG("poweroff thread: sleeping for %i micro-seconds\n", delay);
			if (delay <= 0)
				break;

			usleep(delay);

		} while (*args_timeout > time_us + delay);

		if (!*canceled)
			execute(poweroff, 0);

		DEBUGMSG("Poweroff canceled\n");
	}

	return NULL;
}
#endif


bool power_button_is_pressed(void)
{
	return power_button_pressed;
}

int do_listen(const char *event, const char *uinput)
{
	if (open_fds(event, uinput))
		return -1;

	const struct shortcut *tmp;
	const struct shortcut *shortcuts = getShortcuts();

#if (POWEROFF_TIMEOUT > 0)
	bool with_poweroff_timeout = true;
	struct poweroff_thd_args *args;

	/* If a poweroff combo exist, don't activate the shutdown
	 * on timeout feature */
	for (tmp = shortcuts; tmp; tmp = tmp->prev)
		if (tmp->action == poweroff) {
			with_poweroff_timeout = false;
			break;
		}

	if (with_poweroff_timeout) {
		args = malloc(sizeof(*args));

		if (!args) {
			fprintf(stderr, "Unable to allocate memory\n");
			with_poweroff_timeout = false;
		} else {
			pthread_t thd;

			pthread_mutex_init(&args->mutex, NULL);
			pthread_mutex_init(&args->mutex2, NULL);
			pthread_mutex_lock(&args->mutex);

			pthread_create(&thd, NULL, poweroff_thd, args);
		}
	}
#endif

	while(1) {
		// We wait for an event.
		// On mouse mode, this call does not block.
		struct input_event my_event;
		int read = fread(&my_event, sizeof(struct input_event), 1, event0);

		// If we are on "mouse" mode and nothing has been read, let's wait for a bit.
		if (mode == MOUSE && !read)
		  usleep(10000);

		if (read) {
			// If the power button is pressed, block inputs (if it wasn't already blocked)
			if (my_event.code == KEY_POWER) {

				// We don't want key repeats on the power button.
				if (my_event.value == 2)
				  continue;

				DEBUGMSG("(un)grabbing.\n");
				power_button_pressed = !!my_event.value;

				if (!power_button_pressed) {
					// Send release events to all active combos.
					for (tmp = shortcuts; tmp; tmp = tmp->prev) {
						bool was_combo = true;
						unsigned int i;
						for (i = 0; i < tmp->nb_keys; i++) {
							struct button *button = tmp->keys[i];
							was_combo &= !!button->state;
							button->state = 0;
						}
						if (was_combo)
							execute(tmp->action, 0);
					}
				}

#if (POWEROFF_TIMEOUT > 0)
				if (with_poweroff_timeout) {
					if (power_button_pressed) {
						struct timeval tv;

						gettimeofday(&tv, NULL);
						args->timeout = (tv.tv_sec + POWEROFF_TIMEOUT)
									* 1000000 + tv.tv_usec;
						args->canceled = 0;
						if (!pthread_mutex_trylock(&args->mutex2))
							pthread_mutex_unlock(&args->mutex);
					} else {
						DEBUGMSG("Power button released: canceling poweroff\n");
						args->canceled = 1;
					}
				}
#endif

				if (!grabbed) {
					if (ioctl(fileno(event0), EVIOCGRAB, power_button_pressed)
							== -1)
						perror(__func__);
				}
				continue;
			}


			// If the power button is currently pressed, we enable shortcuts.
			if (power_button_pressed) {
				for (tmp = shortcuts; tmp; tmp = tmp->prev) {
					bool was_combo = true, is_combo = true, match = false;
					unsigned int i;
					for (i = 0; i < tmp->nb_keys; i++) {
						struct button *button = tmp->keys[i];
						was_combo &= !!button->state;
						if (my_event.code == button->id) {
							match = 1;
							button->state = (my_event.value != 0);
						}
						is_combo &= button->state;
					}

					if (match && (was_combo || is_combo)) {
#if (POWEROFF_TIMEOUT > 0)
						if (with_poweroff_timeout) {
							DEBUGMSG("Combo: canceling poweroff\n");
							args->canceled = 1;
						}
#endif
						execute(tmp->action, my_event.value);
					}
				}
				continue;
			}
		}

		// In case we are in the "mouse" mode, handle mouse emulation.
		if (mode == MOUSE) {

			// We don't want to move the mouse if the power button is pressed.
			if (power_button_pressed)
			  continue;

			// An event occured
			if (read) {
				unsigned int i;

				// Toggle the "value" flag of the button object
				for (i=0; i<NB_BUTTONS; i++) {
					if (buttons[i].id == my_event.code)
					  buttons[i].state = my_event.value;
				}

				switch(my_event.code) {
					case BUTTON_A:
						if (my_event.value == 2)    // We don't want key repeats on mouse buttons.
						  continue;

						inject(EV_KEY, BTN_LEFT, my_event.value);
						inject(EV_SYN, SYN_REPORT, 0);
						continue;

					case BUTTON_B:
						if (my_event.value == 2)    // We don't want key repeats on mouse buttons.
						  continue;

						inject(EV_KEY, BTN_RIGHT, my_event.value);
						inject(EV_SYN, SYN_REPORT, 0);
						continue;

					case BUTTON_X:
					case BUTTON_Y:
					case BUTTON_L:
					case BUTTON_R:
					case BUTTON_START:
					case BUTTON_SELECT:

						// If the event is not mouse-related, we reinject it.
						inject(EV_KEY, my_event.code, my_event.value);
						continue;

					default:
						continue;
				}
			}

			// No event this time
			else {
				// For each direction of the D-pad, we check the state of the corresponding button.
				// If it is pressed, we inject an event with the corresponding mouse movement.
				unsigned int i;
				for (i=0; i<NB_BUTTONS; i++) {
					unsigned short code;
					int value;

					if (!buttons[i].state)
					  continue;

					switch(buttons[i].id) {
						case BUTTON_LEFT:
							code = REL_X;
							value = -5;
							break;
						case BUTTON_RIGHT:
							code = REL_X;
							value = 5;
							break;
						case BUTTON_DOWN:
							code = REL_Y;
							value = 5;
							break;
						case BUTTON_UP:
							code = REL_Y;
							value = -5;
							break;
						default:
							continue;
					}

					inject(EV_REL, code, value);
					inject(EV_SYN, SYN_REPORT, 0);
				}
			}
		}
	}

	return -1;
}
