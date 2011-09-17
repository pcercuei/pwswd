
#include <unistd.h>

#include "../backends.h"

void do_kill(void)
{
	if (!fork())
		execlp("fuser", "fuser", "-k", "-HUP", FRAMEBUFFER, NULL);
}
