
#ifndef SHORTCUT_HANDLER_H
#define SHORTCUT_HANDLER_H

#define NB_MAX_KEYS 4


enum event_type {
    reboot, poweroff, suspend, hold,
    volup, voldown,
    brightup, brightdown,
    mouse,
};


struct button {
    const char *name;
    unsigned short id;
    unsigned short state;
};


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
