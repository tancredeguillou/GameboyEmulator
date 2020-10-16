/**
 * @file test-image.c
 * @brief ad-hoc end-to-end testing of image.c
 *
 * @author J.-C. Chappelier & C.Hölzl, EPFL
 * @date 2019
 */

#include "error.h"
#include "util.h"
#include "image.h"

#include "sidlib.h"

#include <assert.h>
#include <stdio.h>
#include <inttypes.h> // for SCNx macro

// ======================================================================
// image display scale factor
#define SCALE 2

// ======================================================================
image_t image; // needs to be global to be shared with libsid (via functions)

// ======================================================================
static int error(const char* msg)
{
    assert(msg != NULL);
    fputs("ERROR: ", stderr);
    fputs(msg, stderr);
    fputs(".\n", stderr);
    return 1;
}

// ======================================================================
#define CHECK(call, msg_err) \
    do { \
        if (call != ERR_NONE) { \
            fclose(image_file); \
            image_free(&image); \
            return error(msg_err); } \
    } while(0)

// ======================================================================
static int image_line_read_word(FILE* file, image_line_t* pil, size_t index)
{
    uint32_t msb = 0, lsb = 0;
    for(size_t i = 0; i < sizeof(uint32_t); ++i) {
        if (1 != fread((uint8_t*)&msb + i, 1, 1, file)) {
            error("cannot read MSB");
            fprintf(stderr, "  (at index %zu)\n", i);
            return ERR_IO;
        }
        if (1 != fread((uint8_t*)&lsb + i, 1, 1, file)) {
            error("cannot read LSB");
            fprintf(stderr, "  (at index %zu)\n", i);
            return ERR_IO;
        }
    }
    return image_line_set_word(pil, index, msb, lsb);
}
#undef READ

// ======================================================================
static void set_grey(guchar* pixels, int row, int col, int width, guchar grey)
{
    const size_t i = (size_t) (3 * (row * width + col)); // 3 = RGB
    pixels[i+2] = pixels[i+1] = pixels[i] = grey;
}

// ======================================================================
static void generate_image(guchar* pixels, int height, int width)
{
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            uint8_t pixel = 0;
            if (image_get_pixel(&pixel, &image, (size_t) y / SCALE, (size_t) x / SCALE) != ERR_NONE)
                pixel = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
            set_grey(pixels, x, y,  width, 255 - 85 * pixel);
#pragma GCC diagnostic pop
        }
    }
}

// ======================================================================
int main(int argc, char *argv[])
{
    if (argc <= 1)
        return error("please provide an input file (binary image)");

    zero_init_var(image);

    const size_t width = 256, height = 256;
    M_EXIT_IF_ERR(image_create(&image, width, height));

    FILE* image_file = fopen(argv[1], "rb");
    if (image_file == NULL)
        return error("cannot open provided image file for reading");

    const size_t nb_line_words = width / IMAGE_LINE_WORD_BITS;
    for (size_t y = 0; y < height; ++y) {
        image_line_t il;
        zero_init_var(il);
        CHECK(image_line_create(&il, width), "cannot create line"); // prepare for next line

        for (size_t x = 0; x < nb_line_words; ++x) {
            CHECK(image_line_read_word(image_file, &il, x), "cannot read line word");
        }
        CHECK(image_own_line_content(&image, y, il), "cannot acquire ownership on line");
    }

    // display image
    sd_launch(&argc, &argv,
              sd_init(argv[1], (int) width * SCALE, (int) height * SCALE, 0,
                      generate_image, NULL, NULL));

    fclose(image_file);
    image_free(&image);

    return 0;
}
