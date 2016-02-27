#include <stdio.h>

#define RATIOMODE_FILE "/sys/devices/platform/soc/13050000.jz4770-fb/keep_aspect_ratio"

void do_change_ratiomode(void)
{
	FILE *f = fopen(RATIOMODE_FILE, "r+");
	if (f) {
		char enabled = '0';
		fread(&enabled, 1, 1, f);
		rewind(f);

		if (enabled == 'Y' || enabled == '1')
			fputs("0\n", f);
		else
			fputs("1\n", f);
		fwrite(&enabled, 1, 1, f);
		fclose(f);
	}
}
