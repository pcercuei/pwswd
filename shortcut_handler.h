
#ifndef SHORTCUT_HANDLER_H
#define SHORTCUT_HANDLER_H

#define NB_MAX_KEYS 4

#define BUTTON_A KEY_LEFTCTRL
#define BUTTON_B KEY_LEFTALT
#define BUTTON_L KEY_TAB
#define BUTTON_R KEY_BACKSPACE
#define BUTTON_UP KEY_UP
#define BUTTON_DOWN KEY_DOWN
#define BUTTON_LEFT KEY_LEFT
#define BUTTON_RIGHT KEY_RIGHT
#define BUTTON_START KEY_ENTER
#define BUTTON_SELECT KEY_ESC
#define BUTTON_POWER KEY_HOME
#define BUTTON_HOLD KEY_PAUSE

#define BUTTON_JS_A BTN_EAST
#define BUTTON_JS_B BTN_SOUTH
#define BUTTON_JS_L BTN_THUMBL
#define BUTTON_JS_R BTN_THUMBR
#define BUTTON_JS_START BTN_START
#define BUTTON_JS_SELECT BTN_SELECT

#define BUTTON_JS_HAT0X ABS_HAT0X
#define BUTTON_JS_HAT0Y ABS_HAT0Y

#ifdef WITH_REVERSED_X_Y
#define BUTTON_X KEY_SPACE
#define BUTTON_Y KEY_LEFTSHIFT
#define BUTTON_JS_X BTN_NORTH
#define BUTTON_JS_Y BTN_WEST
#else
#define BUTTON_X KEY_LEFTSHIFT
#define BUTTON_Y KEY_SPACE
#define BUTTON_JS_X BTN_WEST
#define BUTTON_JS_Y BTN_NORTH
#endif

enum event_type {
	reboot, poweroff, suspend, hold,
	volup, voldown,
	brightup, brightdown,
	mouse, tvout, screenshot,
	kill,
};

struct button {
	const char *name;
	size_t name_len;
	unsigned short type;
	unsigned short id;
	unsigned short state;
	int hat_value;
};

extern struct button buttons[];
extern unsigned int nb_buttons;


struct shortcut {
	enum event_type action;
	struct button * keys[NB_MAX_KEYS];
	int nb_keys;
	struct shortcut *prev;
};


int read_conf_file(const char *filename);
const struct shortcut * getShortcuts();
void deinit();

#endif // SHORTCUT_HANDLER_H
