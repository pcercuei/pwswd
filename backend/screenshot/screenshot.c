
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <features.h>
#include <pthread.h>
#include <sched.h>
#include <png.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../backends.h"

#define XRES 320
#define YRES 240

typedef struct {
	uint8_t r, g, b;
} uint24_t;


static unsigned int number = 1;
static pthread_mutex_t number_mutex = PTHREAD_MUTEX_INITIALIZER;


static inline unsigned int getNumber(void)
{
	unsigned int res;
	pthread_mutex_lock(&number_mutex);
	res = number++;
	pthread_mutex_unlock(&number_mutex);
	return res;
}


static inline uint24_t rgb16_to_24(uint16_t px)
{
	uint24_t pixel;

	pixel.b = (px & 0x1f)   << 3;
	pixel.g = (px & 0x7e0)  >> 3;
	pixel.r = (px & 0xf800) >> 8;

	return pixel;
}


static inline void convert_to_24(uint24_t *to, uint16_t *from, unsigned int len)
{
	while (len--)
	  *to++ = rgb16_to_24(*from++);
}


static void * screenshot_thd(void* p)
{
	unsigned int i;
	png_bytep row_pointers[YRES];
	png_structp png;
	png_infop info;
	void *picture, *buffer;
	FILE *fbdev, *pngfile = (FILE *) p;

	picture = malloc(XRES * YRES * sizeof(uint24_t));
	if (!picture) {
		fprintf(stderr, "Unable to allocate memory.\n");
		return NULL;
	}

	// Pointer to an area corresponding to the end of the "picture" buffer.
	buffer = picture + (sizeof(uint24_t) - sizeof(uint16_t)) * XRES * YRES;

	fbdev = fopen(FRAMEBUFFER, "rb");
	if (!fbdev) {
		fprintf(stderr, "Unable to open framebuffer device.\n");
		goto err_free_picture;
	}

	// Read the framebuffer data.
	read(fileno(fbdev), buffer, XRES * YRES * 2);
	fclose(fbdev);

	// The framebuffer has been read, now we can fall back to a lower priority.
	  {
		struct sched_param params;
		int policy;
		pthread_t my_thread;

		my_thread = pthread_self();
		pthread_getschedparam(my_thread, &policy, &params);
		params.sched_priority = sched_get_priority_min(policy);
		pthread_setschedparam(my_thread, policy, &params);
	  }

	// Init the PNG stuff, and write the header
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
				NULL, NULL, NULL);
	if (!png) {
		fprintf(stderr, "Unable to write PNG header.\n");
		goto err_free_picture;
	}

	info = png_create_info_struct(png);
	if (!info) {
		fprintf(stderr, "Unable to write PNG header.\n");
		goto err_free_png;
	}

	png_init_io(png, pngfile);
	png_set_IHDR(png, info, XRES, YRES, 8,
				PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE,
				PNG_FILTER_TYPE_BASE);
	png_write_info(png, info);

	// Convert the picture to a format that can be written into the PNG file (rgb888)
	convert_to_24(picture, buffer, XRES * YRES);

	// And save the final image
	for (i=0; i<YRES; i++)
		row_pointers[i] = picture + i * XRES * sizeof(uint24_t);

	png_write_image(png, row_pointers);
	png_write_end(png, info);

	png_destroy_write_struct(NULL, &info);
err_free_png:
	png_destroy_write_struct(&png, NULL);
err_free_picture:
	free(picture);
	return NULL;
}


void do_screenshot(void)
{
	pthread_t thd = 0;
	char *screenshot_fn, *png_fn;
	FILE *pngfile;
	const char *home = getenv("HOME");

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
		sprintf(png_fn, screenshot_fn, getNumber());
		pngfile = fopen(png_fn, "ab+");
		if (!pngfile) {
			fprintf(stderr, "Unable to open file for write.\n");
			goto free_png_fn;
		}

		fseek(pngfile, 0, SEEK_END);
		if (ftell(pngfile) == 0)
			break;

		fclose(pngfile);
	}

	rewind(pngfile);
	pthread_create(&thd, NULL, screenshot_thd, pngfile);
	pthread_join(thd, NULL);
	printf("Wrote to %s\n", png_fn);

	fclose(pngfile);
free_png_fn:
	free(png_fn);
free_screenshot_fn:
	free(screenshot_fn);
	return;
}
