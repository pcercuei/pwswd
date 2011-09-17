
#include <stdio.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include "event_listener.h"
#include "shortcut_handler.h"
#include "backend/backends.h"

#define EVENT_FILENAME  "/dev/event0"
#define UINPUT_FILENAME "/dev/uinput"

#ifdef DEBUG
#define DEBUGMSG(msg...) printf(msg)
#else
#define DEBUGMSG(msg...)
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

static int grabbed = 0;

static int power_button_pressed = 0;

static void switchmode(enum _mode new)
{
	if (new == mode) return;

	switch(mode) {
		case NORMAL:
			switch(new) {
				case MOUSE:
					// Enable non-blocking reads: we won't have to wait for an event
					// to process mouse emulation.
					assert(!fcntl(fileno(event0), F_SETFL, O_NONBLOCK));
				case HOLD:
					grabbed = 1;
				default:
					mode = new;
					return;
			}
			break;
		case MOUSE:
			// Disable non-blocking reads.
			assert(!fcntl(fileno(event0), F_SETFL, 0));
		case HOLD:
			switch(new) {
				case NORMAL:
					grabbed = 0;
				default:
					mode = new;
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
		case reboot:
			if (value == 2) return;
			str = "reboot";
			do_reboot();
			break;
		case poweroff:
			if (value == 2) return;
			str = "poweroff";
			do_poweroff();
			break;
		case suspend:
			if (value != 1) return;
			str = "suspend";
			do_suspend();
			break;
		case hold:
			if (value == 2) return;
			str = "hold";
			if (mode == HOLD)
			  switchmode(NORMAL);
			else
			  switchmode(HOLD);
			break;
		case volup:
			str = "volup";
			vol_up(value);
			break;
		case voldown:
			str = "voldown";
			vol_down(value);
			break;
		case brightup:
			str = "brightup";
			bright_up(value);
			break;
		case brightdown:
			str = "brightdown";
			bright_down(value);
			break;
		case mouse:
			if (value == 2) return;
			str = "mouse";
			if (mode == MOUSE)
			  switchmode(NORMAL);
			else
			  switchmode(MOUSE);
			break;
		case tvout:
			if (value == 2) return;
			str = "tvout";
			tv_out();
			break;
		case screenshot:
			if (value == 2) return;
			str = "screenshot";
			do_screenshot();
			break;
		case kill:
			if (value == 2) return;
			str = "kill";
			do_kill();
			break;
		default:
			return;
	}
	DEBUGMSG("Execute: %s.\n", str);
}


static void open_fds(const char *event0fn, const char *uinputfn)
{
	event0 = fopen(event0fn, "r");
	assert(event0);

	uinput = fopen(uinputfn, "r+");
	assert(uinput);

	write(fileno(uinput), &uud, sizeof(uud));
	assert(!ioctl(fileno(uinput), UI_SET_EVBIT, EV_KEY));
	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BTN_LEFT));
	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BTN_MIDDLE));
	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BTN_RIGHT));

	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BUTTON_X));
	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BUTTON_Y));
	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BUTTON_L));
	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BUTTON_R));
	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BUTTON_START));
	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BUTTON_SELECT));

	assert(!ioctl(fileno(uinput), UI_SET_EVBIT, EV_REL));
	assert(!ioctl(fileno(uinput), UI_SET_RELBIT, REL_X));
	assert(!ioctl(fileno(uinput), UI_SET_RELBIT, REL_Y));
	assert(!ioctl(fileno(uinput), UI_SET_RELBIT, REL_WHEEL));

	assert(!ioctl(fileno(uinput), UI_SET_KEYBIT, BTN_MOUSE));

	assert(!ioctl(fileno(uinput), UI_DEV_CREATE));
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


int power_button_is_pressed(void)
{
	return power_button_pressed;
}

int do_listen()
{
	open_fds(EVENT_FILENAME, UINPUT_FILENAME);

	struct shortcut *tmp;
	const struct shortcut *shortcuts = getShortcuts();
	struct input_event my_event;
	unsigned int i;

	unsigned short code = 0;
	int value = 0;

	// booleans
	int key_combo, changed;
	int read;

	for(;;) {

		// We wait for an event.
		// On mouse mode, this call does not block.
		read = fread(&my_event, sizeof(struct input_event), 1, event0);

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
				power_button_pressed = my_event.value;

				if (!grabbed) {
					if (power_button_pressed)
					  assert(!ioctl(fileno(event0), EVIOCGRAB, 1));
					else
					  assert(!ioctl(fileno(event0), EVIOCGRAB, 0));
				}
				continue;
			}


			// If the power button is currently pressed, we enable shortcuts.
			if (power_button_pressed) {

				tmp = (struct shortcut *)shortcuts;
				while(tmp) {
					key_combo = 1;
					changed = 0;

					for(i=0; i < tmp->nb_keys; i++)
					  if (my_event.code == tmp->keys[i]->id) {
						  changed = 1;
						  tmp->keys[i]->state = (my_event.value != 0);
					  }

					if (changed) {
						for (i=0; i < tmp->nb_keys; i++)
						  key_combo &= tmp->keys[i]->state;

						if (key_combo)
						  execute(tmp->action, my_event.value);
					}

					tmp = tmp->prev;
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

				// Toggle the "value" flag of the button object
				for (i=0; i<NB_BUTTONS; i++) {
					if (buttons[i].id == my_event.code)
					  buttons[i].state = my_event.value;
				}

				switch(my_event.code) {
					case BUTTON_LEFT:
					case BUTTON_RIGHT:
					case BUTTON_DOWN:
					case BUTTON_UP:
						// We don't care, it will be handled sooner or later.
						continue;

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
				for (i=0; i<NB_BUTTONS; i++) {
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
