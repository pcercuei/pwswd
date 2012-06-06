
#include <unistd.h>

#include "tvout_backend.h"

void tv_out(void)
{
	if(!fork())
		execlp("tvtool", "tvtool", "--toggle", NULL);
}
