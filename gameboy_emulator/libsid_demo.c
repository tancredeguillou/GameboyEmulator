#include "sidlib.h"

#include <stdint.h>

// Key press bits
#define MY_KEY_UP_BIT    0x01
#define MY_KEY_DOWN_BIT  0x02
#define MY_KEY_RIGHT_BIT 0x04
#define MY_KEY_LEFT_BIT  0x08
#define MY_KEY_A_BIT     0x10

// ======================================================================
static void set_grey(guchar* pixels, int row, int col, int width, guchar grey)
{
    const size_t i = (size_t) (3 * (row * width + col)); // 3 = RGB
    pixels[i+2] = pixels[i+1] = pixels[i] = grey;
}

// ======================================================================
static void generate_image(guchar* pixels, int height, int width)
{
    static int N = 0;
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
    }
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
        do_key(UP);
        return TRUE;

    case GDK_KEY_Down:
        do_key(DOWN);
        return TRUE;

    case GDK_KEY_Right:
        do_key(RIGHT);
        return TRUE;

    case GDK_KEY_Left:
        do_key(LEFT);
        return TRUE;

    case 'A':
    case 'a':
        do_key(A);
        return TRUE;
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
        do_key(UP);
        return TRUE;

    case GDK_KEY_Down:
        do_key(DOWN);
        return TRUE;

    case GDK_KEY_Right:
        do_key(RIGHT);
        return TRUE;

    case GDK_KEY_Left:
        do_key(LEFT);
        return TRUE;

    case 'A':
    case 'a':
        do_key(A);
        return TRUE;
    }

    return FALSE;
}
#undef do_key

// ======================================================================
int main(int argc, char *argv[])
{
    if (argv[1] == NULL) {
        sd_launch(&argc, &argv,
                  sd_init("demo1 (no update)", 144 * 3, 160 * 3, 0,
                          generate_image, NULL, NULL));
    } else if (argv[1][0] == '2') {
        sd_launch(&argc, &argv,
                  sd_init("demo2 (default key handler)", 144 * 3, 160 * 3, 250,
                          generate_image, NULL, NULL));
    } else {
        sd_launch(&argc, &argv,
                  sd_init("demo3 (provided key handler)", 144 * 3, 160 * 3, 250,
                          generate_image, keypress_handler, keyrelease_handler));
    }
    return 0;
}
