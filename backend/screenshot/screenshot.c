
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <features.h>
#include <pthread.h>
#include <sched.h>
#include <png.h>
#include <sys/types.h>

#define XRES 320
#define YRES 240
#define PIXELTYPE uint16_t

#define FRAMEBUFFER "/dev/fb0"
#define SCREENSHOTS "/card/screenshot%i.png"

typedef PIXELTYPE pixel_t;

typedef struct {
    uint8_t r, g, b;
} uint24_t;

struct screenshot_data {
    FILE *pngfile;
    png_structp png;
    png_infop info;
    uint24_t *picture;
};


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


#define _TO_24(type, px) type##_to_24(px)
#define  TO_24(type, px) _TO_24(type, px)

static inline uint24_t uint16_t_to_24(uint16_t px)
{
    uint24_t pixel;

    pixel.b = (px & 0x1f)   << 3;
    pixel.g = (px & 0x7e0)  >> 3;
    pixel.r = (px & 0xf800) >> 8;

    return pixel;
}

static inline void convert_to_24(uint24_t *to, pixel_t *from, unsigned int len)
{
    while (len--)
        *to++ = TO_24(PIXELTYPE, *from++);
}


static int init_png(struct screenshot_data * data)
{
    data->png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                NULL, NULL, NULL);
    if (!data->png) {
        fprintf(stderr, "Unable to write PNG header.\n");
        return 1;
    }

    data->info = png_create_info_struct(data->png);
    if (!data->info) {
        fprintf(stderr, "Unable to write PNG header.\n");
        png_destroy_write_struct(&data->png, NULL);
        return 2;
    }

    png_init_io(data->png, data->pngfile);
//    png_set_compression_level(data->png, Z_BEST_COMPRESSION);
    png_set_IHDR(data->png, data->info, XRES, YRES, 8,
                PNG_COLOR_TYPE_RGB, 
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);
    png_write_info(data->png, data->info);
    return 0;
}


static void * screenshot_thd(void* p)
{
    unsigned int i, n;
    char pngfilename[0x20];
    png_bytep row_pointers[YRES];
    pixel_t  *buffer;
    FILE *fbdev;

    struct screenshot_data *data = (struct screenshot_data*)p;

    data->picture = (uint24_t*) malloc(XRES * YRES * sizeof(uint24_t));
    if (!data->picture) {
        fprintf(stderr, "Unable to allocate memory.\n");
        free(data);
        return NULL;
    }

    // Pointer to an area corresponding to the end of the "picture" buffer.
    buffer = (pixel_t*)( (uint8_t*)data->picture + (sizeof(uint24_t) - sizeof(pixel_t)) * XRES * YRES);

    fbdev = fopen(FRAMEBUFFER, "rb");
    if (!fbdev) {
        fprintf(stderr, "Unable to open framebuffer device.\n");
        free(data->picture);
        free(data);
        return NULL;
    }

    // Read the framebuffer data.
    read(fileno(fbdev), buffer, XRES*YRES*sizeof(pixel_t));
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

    // Try to find a file on which the data is to be saved.
    for (;;) {
        n = getNumber();
        sprintf(pngfilename, SCREENSHOTS, n);
        data->pngfile = fopen(pngfilename, "ab+");
        if (!data->pngfile) {
            fprintf(stderr, "Unable to open file for write.\n");
            free(data->picture);
            free(data);
            return NULL;
        }

        fseek(data->pngfile, 0, SEEK_END);
        if (ftell(data->pngfile) == 0)
          break;
        
        fclose(data->pngfile);
    }

    rewind(data->pngfile);

    // Init the PNG stuff, and write the header
    if (init_png(data)) {
        fprintf(stderr, "Unable to init PNG context.\n");
        fclose(data->pngfile);
        free(data->picture);
        free(data);
        return NULL;
    }

    // Convert the picture to a format that can be written into the PNG file (rgb888)
    convert_to_24(data->picture, buffer, XRES*YRES);

    // And save the final image
    for (i=0; i<YRES; i++)
        row_pointers[i] = (png_bytep)data->picture + i * XRES * sizeof(uint24_t);

    png_write_image(data->png, row_pointers);

    png_write_end(data->png, data->info);
    png_destroy_write_struct(&data->png, &data->info);

    printf("Wrote to " SCREENSHOTS "\n", n);

    if (data->pngfile)
      fclose(data->pngfile);

    free(data->picture);
    free(data);
    return NULL;
}


void do_screenshot(void)
{
    pthread_t thd;
    struct screenshot_data * data;
    
    data = malloc(sizeof(struct screenshot_data));
    if (!data) {
        fprintf(stderr, "Unable to allocate memory.\n");
        return;
    }

    pthread_create(&thd, NULL, screenshot_thd, data);
}

