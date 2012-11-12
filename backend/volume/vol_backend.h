
#ifndef VOL_BACKEND_H
#define VOL_BACKEND_H

int vol_init(const char *mixer, const char *dac);
void vol_down(int event_value);
void vol_up(int event_value);

#endif
