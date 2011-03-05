
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shortcut_handler.h"
#include "event_listener.h"

#ifndef PROGNAME
#define PROGNAME "pwswd"
#endif


#define USAGE() printf("Usage:\n\t" PROGNAME " [-f conf_file]\n\n")


int main(int argc, char **argv)
{
	const char *filename = NULL;

	if (argc > 1) {
		if (strcmp(argv[1], "-f") || (argc < 3)) {
			USAGE();
			return 1;
		}
		filename = argv[2];
	}
	
	else {
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

	do_listen();
	deinit();
	return 0;
}
