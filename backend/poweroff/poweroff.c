#include <pthread.h>
#include <unistd.h>

#include "poweroff_backend.h"

static void *poweroff_thd(void *p)
{
#ifdef BACKEND_BRIGHTNESS
	blank(1);
#endif

	/* Wait until the power slider is released, otherwise the
	 * device will reboot instead of powering down */
	while(power_button_is_pressed())
		usleep(10000);

	execlp("/sbin/poweroff", "/sbin/poweroff", (char*)NULL);
	return NULL;
}

void do_poweroff()
{
	pthread_t thd;
	pthread_create(&thd, NULL, poweroff_thd, NULL);
}
