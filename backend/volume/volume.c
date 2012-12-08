
#include <stdio.h>
#include <ctype.h>
#include <alsa/asoundlib.h>

#include "vol_backend.h"

static char card[] = "default";

static snd_mixer_elem_t *elem, *dac_elem;
static long min, max, step;
static long current;
static int disactivated = 1;

static int parse_simple_id(const char *str, snd_mixer_selem_id_t *sid)
{
	int c, size;
	char buf[128];
	char *ptr = buf;

	while (*str == ' ' || *str == '\t')
		str++;
	if (!(*str))
		return -EINVAL;
	size = 1;	/* for '\0' */
	if (*str != '"' && *str != '\'') {
		while (*str && *str != ',') {
			if (size < (int)sizeof(buf)) {
				*ptr++ = *str;
				size++;
			}
			str++;
		}
	} else {
		c = *str++;
		while (*str && *str != c) {
			if (size < (int)sizeof(buf)) {
				*ptr++ = *str;
				size++;
			}
			str++;
		}
		if (*str == c)
			str++;
	}
	if (*str == '\0') {
		snd_mixer_selem_id_set_index(sid, 0);
		*ptr = 0;
		goto _set;
	}
	if (*str != ',')
		return -EINVAL;
	*ptr = 0;	/* terminate the string */
	str++;
	if (!isdigit(*str))
		return -EINVAL;
	snd_mixer_selem_id_set_index(sid, atoi(str));
_set:
	snd_mixer_selem_id_set_name(sid, buf);
	return 0;
}


int vol_init(const char *mixer, const char *dac) {
	snd_mixer_selem_id_t *sid;
	snd_mixer_t *handle = NULL;

	snd_mixer_selem_id_alloca(&sid);

	if (parse_simple_id(mixer, sid)) {
		fprintf(stderr, "Wrong control identifier.\n");
		return 1;
	}

	if (snd_mixer_open(&handle, 0) < 0) {
		fprintf(stderr, "Mixer open error.\n");
		return 1;
	}

	if (snd_mixer_attach(handle, card) < 0) {
		fprintf(stderr, "Mixer attach error.\n");
		snd_mixer_close(handle);
		return 1;
	}

	if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {
		fprintf(stderr, "Mixer register error.\n");
		snd_mixer_close(handle);
		return 1;
	}

	if (snd_mixer_load(handle) < 0) {
		fprintf(stderr, "Mixer load error.\n");
		snd_mixer_close(handle);
		return 1;
	}

	elem = snd_mixer_find_selem(handle, sid);
	if (!elem) {
		fprintf(stderr, "Unable to find control.\n");
		snd_mixer_close(handle);
		return 1;
	}

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	/* 32 steps */
	step = (max - min) / 32;
	if (!step)
		step = 1;

	if (dac) {
		if (parse_simple_id(dac, sid)) {
			fprintf(stderr, "Wrong control identifier.\n");
			snd_mixer_close(handle);
			return 1;
		}

		dac_elem = snd_mixer_find_selem(handle, sid);
		if (!dac_elem)
			fprintf(stderr, "Unable to find DAC switch.\n");
	}

	disactivated = 0;
	return 0;
}


static void setVolume(long vol) {
	long value = vol;

	if (snd_mixer_selem_set_playback_volume_all(elem, value)) {
		fprintf(stderr, "Unable to set volume.\n");
	}
}


static long getVolume() {
	long value;

	if (snd_mixer_selem_get_playback_volume(elem, 0, &value)) {
		fprintf(stderr, "Unable to read volume.\n");
		return -1;
	}

	return value;
}


void vol_down(int event_value) {
	if (disactivated)
		return;

	// If the button has been released, there's nothing to do.
	if (event_value == 0)
		return;

	// If the button has just been pressed, we retrieve
	// the latest volume from ALSA
	if (event_value == 1)
		current = getVolume();

	if (current == min)
	  return;

	else if (current - step <= min) {
		current = min;
		if (dac_elem)
			snd_mixer_selem_set_playback_switch(dac_elem,
						SND_MIXER_SCHN_MONO, 0);
	}

	else
	  current -= step;

	setVolume(current);
}

void vol_up(int event_value) {
	if (disactivated)
		return;

	// If the button has been released, there's nothing to do.
	if (event_value == 0)
		return;

	// If the button has just been pressed, we retrieve
	// the latest volume from ALSA
	if (event_value == 1) {
		current = getVolume();
		if (dac_elem && current == min)
			snd_mixer_selem_set_playback_switch(dac_elem,
						SND_MIXER_SCHN_MONO, 1);
	}

	if (current == max)
	  return;

	else if (current + step >= max)
	  current = max;

	else
	  current += step;

	setVolume(current);
}
