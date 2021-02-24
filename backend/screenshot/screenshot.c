
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <features.h>
#include <pthread.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "../backends.h"

static unsigned int number = 1;


static void * screenshot_thd(void* p)
{
	struct sched_param params;
	int policy;
	pthread_t my_thread;
	const char *png_fn = p;
	char buf[1024];

	/* Fall back to a lower priority. */
	my_thread = pthread_self();
	pthread_getschedparam(my_thread, &policy, &params);
	params.sched_priority = sched_get_priority_min(policy);
	pthread_setschedparam(my_thread, policy, &params);

	snprintf(buf, sizeof(buf), "kmsgrab %s", png_fn);
	system(buf);

	return NULL;
}


void do_screenshot(void)
{
	pthread_t thd = 0;
	char *screenshot_fn, *png_fn;
	FILE *pngfile;
	const char *home = getenv("HOME");
	int found;

	if (!home) {
		fprintf(stderr, "$HOME is not defined.\n");
		return;
	}

	/* 32 = strlen("/screenshots") + strlen("/screenshot%03d.png") +1
	 * 31 = strlen("/screenshots") + strlen("/screenshotXXX.png") +1 */
	screenshot_fn = malloc(strlen(home) + 32);
	if (!screenshot_fn) {
		fprintf(stderr, "Unable to allocate memory for screenshot filename string.\n");
		return;
	}

	png_fn = malloc(strlen(home) + 31);
	if (!png_fn) {
		fprintf(stderr, "Unable to allocate memory for screenshot filename string.\n");
		goto free_screenshot_fn;
	}

	strcpy(screenshot_fn, home);
	strcat(screenshot_fn, "/screenshots");
	mkdir(screenshot_fn, 0755);
	strcat(screenshot_fn, "/screenshot\%03d.png");

	// Try to find a file on which the data is to be saved.
	for (;;) {
		sprintf(png_fn, screenshot_fn, number++);
		pngfile = fopen(png_fn, "a+");
		if (!pngfile) {
			fprintf(stderr, "Unable to open file for write: %s.\n", png_fn);
			goto free_png_fn;
		}

		fseek(pngfile, 0, SEEK_END);
		found = ftell(pngfile) == 0;
		fclose(pngfile);

		if (found)
			break;
	}

	pthread_create(&thd, NULL, screenshot_thd, png_fn);
	pthread_join(thd, NULL);
	printf("Wrote to %s\n", png_fn);

free_png_fn:
	free(png_fn);
free_screenshot_fn:
	free(screenshot_fn);
	return;
}
