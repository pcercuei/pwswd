
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shortcut_handler.h"
#include "event_listener.h"
#include "backend/backends.h"

#ifndef PROGNAME
#define PROGNAME "pwswd"
#endif

#ifndef EVENT_FILENAME
#define EVENT_FILENAME  "/dev/event0"
#endif

#ifndef UINPUT_FILENAME
#define UINPUT_FILENAME "/dev/uinput"
#endif

#ifndef SOFTVOL_NAME
#define SOFTVOL_NAME "SoftMaster"
#endif

#ifndef DAC_NAME
#define DAC_NAME "Output Mixer DAC"
#endif

#define USAGE() printf("Usage:\n\t" PROGNAME " [-f config file] [-e event interface] [-u uinput interface]\n\n")


int main(int argc, char **argv)
{
	const char *filename = NULL,
		  *event = NULL, *uinput = NULL;
	size_t i;

	for (i = 1; i < argc; i++) {
		if (argc > i + 1) {
			if (!strcmp(argv[i], "-f"))
				filename = argv[i+1];
			else if (!strcmp(argv[i], "-e"))
				event = argv[i+1];
			else if (!strcmp(argv[i], "-u"))
				uinput = argv[i+1];
			else {
				USAGE();
				return 1;
			}

			i++;
		} else {
			USAGE();
			return 1;
		}
	}
	
	if (!filename) {
		struct stat st;
		filename = "/etc/local/" PROGNAME ".conf";

		if (stat(filename, &st) == -1) {
			filename = "/etc/" PROGNAME ".conf";

			if (stat(filename, &st) == -1) {
				printf("pwswd: Unable to find a configuration file.\n");
				exit(EXIT_FAILURE);
			}
		}

		if (!S_ISREG(st.st_mode)) {
			printf("pwswd: The configuration file is not a regular file.\n");
			exit(EXIT_FAILURE);
		}
	}

	if (!event)
		event = EVENT_FILENAME;

	if (!uinput)
		uinput = UINPUT_FILENAME;

	int nb_shortcuts;
	switch(nb_shortcuts = read_conf_file(filename)) {
		case -1:
			fprintf(stderr, "Error: unable to open file.\n");
			return 2;
		case -2:
			fprintf(stderr, "Error: empty event name in conf file.\n");
			return 3;
		case -3:
			fprintf(stderr, "Error: unknown event name in conf file.\n");
			return 4;
		case -4:
			fprintf(stderr, "Error: the number of keys is limited to %i (power switch excepted). Please check your conf file.\n", NB_MAX_KEYS);
			return 5;
		default:
			break;
	}

	if (vol_init(SOFTVOL_NAME, DAC_NAME))
		fprintf(stderr, "Unable to init volume backend\n");

	do_listen(event, uinput);
	deinit();
	return 0;
}
