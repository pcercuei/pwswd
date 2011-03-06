
#include <stdio.h>
#include <stdlib.h>

#include "tvout_backend.h"

static int activated = 0;

void tv_out(void)
{
	activated = !activated;

	if (activated)
	  system("/usr/sbin/tvtool --on");
	else
	  system("/usr/sbin/tvtool --off");
}
