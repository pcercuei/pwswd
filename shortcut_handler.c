
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shortcut_handler.h"

static struct shortcut *shortcuts = NULL;

static void shortcut_free(struct shortcut *scuts) {
    struct shortcut *prev;
    while(scuts) {
        prev = scuts->prev;
        free(scuts);
        scuts = prev;
    }
}

int read_conf_file(const char *filename) {
    FILE *conf = fopen(filename, "r");
    if (!conf) return -1;

    char line[0x100];
    char *word;
    struct shortcut *newone=NULL, *prev=NULL;

    unsigned int i,j;
    for (i=0;;i++) {
        if (fgets(line, 0x100, conf) == NULL)
          break;

        word = strtok(line, ":");
        if (!word || !strlen(word)) {
            free(word);
            fclose(conf);
            return -2;
        }

        newone = malloc(sizeof(struct shortcut));
        newone->prev = prev;

        if (!strcmp(word, "poweroff"))
          newone->action = poweroff;
        else if (!strcmp(word, "reboot"))
          newone->action = reboot;
        else if (!strcmp(word, "suspend"))
          newone->action = suspend;
        else if (!strcmp(word, "hold"))
          newone->action = hold;
        else if (!strcmp(word, "volup"))
          newone->action = volup;
        else if (!strcmp(word, "voldown"))
          newone->action = voldown;
        else if (!strcmp(word, "brightup"))
          newone->action = brightup;
        else if (!strcmp(word, "brightdown"))
          newone->action = brightdown;
        else if (!strcmp(word, "mouse"))
          newone->action = mouse;
        else {
            fclose(conf);
            shortcut_free(newone);
            return -3;
        }

        memset(newone->keys,   0, NB_MAX_KEYS*sizeof(unsigned short));
        memset(newone->states, 0, NB_MAX_KEYS*sizeof(unsigned short));

        j=0;
        while( (word = strtok(NULL, ",")) != NULL) {
            if (j>= NB_MAX_KEYS) {
                fclose(conf);
                shortcut_free(newone);
                return -4;
            }
            newone->keys[j++] = atoi(word);
        }

        newone->nb_keys = j;

        prev = newone;
    }

    fclose(conf);
    shortcuts = newone;
    return i;
}


const struct shortcut * getShortcuts() {
    return (const struct shortcut*)shortcuts;
}


void deinit() {
    shortcut_free(shortcuts);
}
