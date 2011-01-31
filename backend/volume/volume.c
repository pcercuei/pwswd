
#include <stdio.h>
#include <ctype.h>
#include <alsa/asoundlib.h>

#include "vol_backend.h"

#define CONTROL_NAME "SoftMaster"

#define STEP_VALUE 1

static char card[] = "default";

static snd_mixer_t *handle = NULL;
static snd_mixer_elem_t *elem;
static snd_mixer_selem_id_t *sid;
static long min, max;
static int inited = 0;

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


static int init() {
    snd_mixer_selem_id_alloca(&sid);

    if (parse_simple_id(CONTROL_NAME, sid)) {
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
        handle = NULL;
        return 1;
    }

    if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {
        fprintf(stderr, "Mixer register error.\n");
        snd_mixer_close(handle);
        handle = NULL;
        return 1;
    }

    if (snd_mixer_load(handle) < 0) {
        fprintf(stderr, "Mixer load error.\n");
        snd_mixer_close(handle);
        handle = NULL;
        return 1;
    }

    elem = snd_mixer_find_selem(handle, sid);
    if (!elem) {
        fprintf(stderr, "Unable to find control.\n");
        snd_mixer_close(handle);
        handle = NULL;
        return 1;
    }

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    inited=1;
    return 0;
}


static void setVolume(long vol) {
    if (!inited) init();

    long value = vol;
    if (snd_mixer_selem_set_playback_volume_all(elem, value)) {
        fprintf(stderr, "Unable to set volume.\n");
    }
}


static long getVolume() {
    if (!inited) init();

    long value;
    if (snd_mixer_selem_get_playback_volume(elem, 0, &value)) {
        fprintf(stderr, "Unable to read volume.\n");
        return -1;
    }

    return value;
}


void vol_down(int event_value) {
    long cur = getVolume();
    if (cur > STEP_VALUE-1)
      setVolume(cur - STEP_VALUE);
}

void vol_up(int event_value) {
    long cur = getVolume();
    if (cur < max-min - STEP_VALUE+1)
      setVolume(cur + STEP_VALUE);
}
