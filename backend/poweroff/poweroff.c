
#include <unistd.h>

#include "poweroff_backend.h"

void do_poweroff()
{
	execlp("/sbin/poweroff", "/sbin/poweroff", (char*)NULL);
}
