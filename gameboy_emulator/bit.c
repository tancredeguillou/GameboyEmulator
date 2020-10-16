#include <assert.h>

#include "bit.h"
#include "error.h"

uint8_t lsb4(uint8_t value) {
    return value & 0xf;
}

uint8_t msb4(uint8_t value) {
    return value >> 4;
}

void bit_rotate(uint8_t *value, rot_dir_t dir, int d) {
    assert(value);
    d = CLAMP07(d);
    if (dir == RIGHT)
    {
        d = 8 - d;
    }
    *value = (*value << d) | (*value >> (8 - d));
}

uint8_t lsb8(uint16_t value) {
    return value;
}

uint8_t msb8(uint16_t value) {
    return value >> 8;
}

uint16_t merge8(uint8_t v1, uint8_t v2) {
    return (v2 << 8) | v1;
}

uint8_t merge4(uint8_t v1, uint8_t v2) {
    return (v2 << 4) | (v1 & 0xf);
}

bit_t bit_get(uint8_t value, int index) {
    return (value >> CLAMP07(index)) & 1;
}

void bit_set(uint8_t *value, int index) {
    assert(value);
    *value |= 1 << CLAMP07(index);
}

void bit_unset(uint8_t *value, int index) {
    assert(value);
    *value &= ~(1 << CLAMP07(index));
}

void bit_edit(uint8_t *value, int index, uint8_t v) {
    v ? bit_set(value, index) : bit_unset(value, index);
}
