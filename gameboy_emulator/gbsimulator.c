#include "sidlib.h"

#include <stdint.h>
#include <sys/time.h>
#include "gameboy.h"
#include "lcdc.h"
#include "util.h"
#include "error.h"

// Key press bits
#define MY_KEY_UP_BIT     0x01
#define MY_KEY_DOWN_BIT   0x02
#define MY_KEY_RIGHT_BIT  0x04
#define MY_KEY_LEFT_BIT   0x08
#define MY_KEY_A_BIT      0x10
#define MY_KEY_B_BIT      0x20
#define MY_KEY_START_BIT  0x40
#define MY_KEY_SELECT_BIT 0x80

#define SCALE_FACTOR 5
#define REFRESH_TIME 40

gameboy_t gameboy;
struct timeval start;
struct timeval paused;

static uint64_t get_time_in_GB_cycles_since(struct timeval *from)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    if (!timercmp(&now, from, >))
    {
        return 0;
    }
    struct timeval delta;
    timersub(&now, from, &delta);
    return delta.tv_sec * GB_CYCLES_PER_S + (delta.tv_usec * GB_CYCLES_PER_S) / 1000000;
}

// ======================================================================
static void set_grey(guchar* pixels, int row, int col, int width, guchar grey)
{
    const size_t i = (size_t) (3 * (row * width + col)); // 3 = RGB
    pixels[i+2] = pixels[i+1] = pixels[i] = grey;
}

// ======================================================================
static void generate_image(guchar* pixels, int height, int width)
{
    uint64_t cycles = get_time_in_GB_cycles_since(&start);

    gameboy_run_until(&gameboy, cycles);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            uint8_t pixel_gameboy;
            image_get_pixel(&pixel_gameboy, &gameboy.screen.display, x / SCALE_FACTOR, y / SCALE_FACTOR);
            set_grey(pixels, y, x, width, 255 - 85 * pixel_gameboy);
        }
    }
    
    
    /*static int N = 0;
    if (++N % 2) {
        // draw a pattern
        guchar color = 0;
        for (int i = 20; i < height; i += 20) {
            color = (color == 0) ? 255 : 0;
            for (int r = i; r < height - i; r++)
                for (int c = i; c < width - i; c++) {
                    set_grey(pixels, r, c,  width, color);
                }
        }
    } else {
        // draw another pattern
        guchar color = 192;
        for (int i = 20; i < height; i += 20) {
            color = (color == 192) ? 64 : 192;
            for (int r = i; r < height - i; r++)
                for (int c = i; c < width - i; c++) {
                    set_grey(pixels, r, c,  width, color);
                }
        }
    }*/
}

// ======================================================================
#define do_key(X) \
    do { \
        if (! (psd->key_status & MY_KEY_ ## X ##_BIT)) { \
            psd->key_status |= MY_KEY_ ## X ##_BIT; \
            puts(#X " key pressed"); \
        } \
    } while(0)

static gboolean keypress_handler(guint keyval, gpointer data)
{
    simple_image_displayer_t* const psd = data;
    if (psd == NULL) return FALSE;

    switch(keyval) {
    case GDK_KEY_Up:
        joypad_key_pressed(&gameboy.pad, UP_KEY);
        return TRUE;

    case GDK_KEY_Down:
        joypad_key_pressed(&gameboy.pad, DOWN_KEY);
        return TRUE;

    case GDK_KEY_Right:
        joypad_key_pressed(&gameboy.pad, RIGHT_KEY);
        return TRUE;

    case GDK_KEY_Left:
        joypad_key_pressed(&gameboy.pad, LEFT_KEY);
        return TRUE;

    case 'A':
    case 'a':
        joypad_key_pressed(&gameboy.pad, A_KEY);
        return TRUE;

    case 'S':
    case 's':
        joypad_key_pressed(&gameboy.pad, B_KEY);
        return TRUE;

    case GDK_KEY_Page_Up:
        joypad_key_pressed(&gameboy.pad, SELECT_KEY);
        return TRUE;

    case GDK_KEY_Page_Down:
        joypad_key_pressed(&gameboy.pad, START_KEY);
        return TRUE;

    case GDK_KEY_space:
        if (psd->timeout_id > 0)
        {
            gettimeofday(&paused, NULL);
        }
        else
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            timersub(&now, &paused, &paused);
            timeradd(&start, &paused, &start);
            timerclear(&paused);
        }
    }

    return ds_simple_key_handler(keyval, data);
}
#undef do_key

// ======================================================================
#define do_key(X) \
    do { \
        if (psd->key_status & MY_KEY_ ## X ##_BIT) { \
          psd->key_status &= (unsigned char) ~MY_KEY_ ## X ##_BIT; \
            puts(#X " key released"); \
        } \
    } while(0)

static gboolean keyrelease_handler(guint keyval, gpointer data)
{
    simple_image_displayer_t* const psd = data;
    if (psd == NULL) return FALSE;

    switch(keyval) {
    case GDK_KEY_Up:
        joypad_key_released(&gameboy.pad, UP_KEY);
        return TRUE;

    case GDK_KEY_Down:
        joypad_key_released(&gameboy.pad, DOWN_KEY);
        return TRUE;

    case GDK_KEY_Right:
        joypad_key_released(&gameboy.pad, RIGHT_KEY);
        return TRUE;

    case GDK_KEY_Left:
        joypad_key_released(&gameboy.pad, LEFT_KEY);
        return TRUE;

    case 'A':
    case 'a':
        joypad_key_released(&gameboy.pad, A_KEY);
        return TRUE;

    case 'S':
    case 's':
        joypad_key_released(&gameboy.pad, B_KEY);
        return TRUE;

    case GDK_KEY_Page_Up:
        joypad_key_released(&gameboy.pad, SELECT_KEY);
        return TRUE;

    case GDK_KEY_Page_Down:
        joypad_key_released(&gameboy.pad, START_KEY);
        return TRUE;
    }

    return FALSE;
}
#undef do_key

// ======================================================================
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("please provide input_file");
        return 1;
    }

    const char* const filename = argv[1];

    zero_init_var(gameboy);
    int err = gameboy_create(&gameboy, filename);
    if (err != ERR_NONE)
    {
        gameboy_free(&gameboy);
        return err;
    }

    gettimeofday(&start, NULL);
    timerclear(&paused);

    sd_launch(&argc, &argv,
              sd_init("provided key handler", LCD_WIDTH * SCALE_FACTOR, LCD_HEIGHT * SCALE_FACTOR, REFRESH_TIME,
                      generate_image, keypress_handler, keyrelease_handler));

    
    gameboy_free(&gameboy);

    return 0;
}
