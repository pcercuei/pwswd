#include "../../event_listener.h"

void do_change_ratiomode(void)
{
	inject(EV_KEY, KEY_SCALE, 1);
	inject(EV_KEY, KEY_SCALE, 0);
	inject(EV_SYN, SYN_REPORT, 0);
}
