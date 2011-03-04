
#include <unistd.h>

#include "reboot_backend.h"

void do_reboot()
{
	execlp("/sbin/reboot", "/sbin/reboot", (char*)NULL);
}
