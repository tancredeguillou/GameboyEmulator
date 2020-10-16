#include <assert.h>

#include "alu.h"
#include "error.h"

bit_t get_flag(flags_t flags, flag_bit_t flag) {
    return ((flags & 0x0F) == 0) ? flags & flag : 0;
}

void set_flag(flags_t *flags, flag_bit_t flag) {
    assert(flags);
    if ((*flags & 0x0F) == 0)
    {
        *flags |= flag;
    }
}

static void set_output(alu_output_t *output, uint16_t value, int z, int n, int h, int c) {
    output->value = value;
    output->flags = ((!!z) << 7) | ((!!n) << 6) | ((!!h) << 5) | ((!!c) << 4);
}

int alu_add8(alu_output_t *result, uint8_t x, uint8_t y, bit_t c0) {
    if (!result) {
        return ERR_BAD_PARAMETER;
    }
    uint8_t low = lsb4(x) + lsb4(y) + c0;
    uint8_t high = msb4(x) + msb4(y) + msb4(low);
    uint8_t sum = merge4(low, high);
    set_output(result, sum, !sum, 0, msb4(low), msb4(high));
    return 0;
}

int alu_sub8(alu_output_t *result, uint8_t x, uint8_t y, bit_t b0) {
    if (!result) {
        return ERR_BAD_PARAMETER;
    }
    uint8_t low = lsb4(x) - lsb4(y) - b0;
    uint8_t high = msb4(x) - msb4(y) - !!msb4(low);
    uint8_t sum = merge4(low, high);
    set_output(result, sum, !sum, 1, msb4(low), msb4(high));
    return 0;
}

static int alu_add16_common(alu_output_t *result, uint16_t x, uint16_t y, int flags_from_high) {
    if (!result) {
        return ERR_BAD_PARAMETER;
    }
    alu_output_t low, high;
    alu_add8(&low, lsb8(x), lsb8(y), 0);
    alu_add8(&high, msb8(x), msb8(y), !!get_C(low.flags));
    uint16_t sum = merge8(low.value, high.value);
    if (flags_from_high) {
        set_output(result, sum, !sum, 0, get_H(high.flags), get_C(high.flags));
    } else {
        set_output(result, sum, !sum, 0, get_H(low.flags), get_C(low.flags));
    }
    return 0;
}

int alu_add16_low(alu_output_t *result, uint16_t x, uint16_t y) {
    return alu_add16_common(result, x, y, 0);
}

int alu_add16_high(alu_output_t *result, uint16_t x, uint16_t y) {
    return alu_add16_common(result, x, y, 1);
}

int alu_shift(alu_output_t *result, uint8_t x, rot_dir_t dir) {
    if (!result || (dir != LEFT && dir != RIGHT)) {
        return ERR_BAD_PARAMETER;
    }
    bit_t carry = dir == LEFT ? bit_get(x, 7) : bit_get(x, 0);
    x = dir == LEFT ? x << 1 : x >> 1;
    set_output(result, x, !x, 0, 0, carry);
    return 0;
}

int alu_shiftR_A(alu_output_t *result, uint8_t x) {
    if (!result) {
        return ERR_BAD_PARAMETER;
    }
    bit_t carry = bit_get(x, 0);
    x = x >> 1;
    bit_edit(&x, 7, bit_get(x, 6));
    set_output(result, x, !x, 0, 0, carry);
    return 0;
}

int alu_rotate(alu_output_t *result, uint8_t x, rot_dir_t dir) {
    if (!result || (dir != LEFT && dir != RIGHT)) {
        return ERR_BAD_PARAMETER;
    }
    bit_t carry = dir == LEFT ? bit_get(x, 7) : bit_get(x, 0);
    bit_rotate(&x, dir, 1);
    set_output(result, x, !x, 0, 0, carry);
    return 0;
}

int alu_carry_rotate(alu_output_t* result, uint8_t x, rot_dir_t dir, flags_t flags) {
    if (!result || (dir != LEFT && dir != RIGHT)) {
        return ERR_BAD_PARAMETER;
    }
    bit_t carry = dir == LEFT ? bit_get(x, 7) : bit_get(x, 0);
    bit_rotate(&x, dir, 1);
    bit_edit(&x, dir == LEFT ? 0 : 7, get_C(flags));
    set_output(result, x, !x, 0, 0, carry);
    return 0;
}
