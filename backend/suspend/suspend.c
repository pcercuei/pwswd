
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../../event_listener.h"

#define PW_FILE "/sys/power/state"

static pthread_t thd;

// It is necessary to run this on a thread.
// The do_suspend() function is called as soon as
// the user presses the button; the power switch
// is probably still pressed, and will wake up
// the dingoo as soon as it's released.
// Here, we create a thread that will suspend the
// dingoo only when the power switch is released.

static void * suspend_thd(void* p)
{
	FILE *f = fopen(PW_FILE, "r+");
	char *mem_word = "mem";

	if (!f) {
		fprintf(stderr, "Unable to open file %s for writing.\n", PW_FILE);
		return NULL;
	}

	while(power_button_is_pressed())
	  usleep(10000);

	fwrite(mem_word, sizeof(char), strlen(mem_word), f);
	fclose(f);
	return NULL;
}

void do_suspend(void)
{
	pthread_create(&thd, NULL, suspend_thd, NULL);
}
