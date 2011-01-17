
#include <stdio.h>
#include <unistd.h>

#include "tvout_backend.h"

#define TVTOOL_BINARY "tvtool"
#define ACTIVATE "--on"
#define DISACTIVATE "--off"

static int activated = 0;

void tv_out(void)
{
	activated = !activated;

	if ( fork() ) {
		// if to be activated
		if (activated)
			execlp(TVTOOL_BINARY, TVTOOL_BINARY, ACTIVATE, (char*)NULL);
		else
			execlp(TVTOOL_BINARY, TVTOOL_BINARY, DISACTIVATE, (char*)NULL);
	}
}
