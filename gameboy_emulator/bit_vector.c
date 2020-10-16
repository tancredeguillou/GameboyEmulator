/**
 * @file bit_vector.c
 * @brief Bit vector code for PPS projects
 *
 * @author Tancrède Guillou, Pablo Stebler
 * @date 2020
 */

#include "bit_vector.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 32

static size_t get_chunk_count(size_t size)
{
    return (size % CHUNK_SIZE) ? size / CHUNK_SIZE + 1 : size / CHUNK_SIZE;
}

bit_vector_t *bit_vector_create(size_t size, bit_t value)
{
    if (size == 0)
    {
        return NULL;
    }

    size_t chunk_count = get_chunk_count(size);
    bit_vector_t *pbv = malloc(sizeof(bit_vector_t) + chunk_count * sizeof(uint32_t));
    if (!pbv)
    {
        return NULL;
    }

    pbv->size = size;
    uint32_t chunk_value = value ? ~0 : 0;
    for (size_t i = 0; i < chunk_count - 1; i++)
    {
        pbv->content[i] = chunk_value;
    }
    size_t last_chunk_idx = size % CHUNK_SIZE;
    uint32_t last_chunk_mask = last_chunk_idx == 0 ? ~0 : ((1 << last_chunk_idx) - 1);
    pbv->content[chunk_count - 1] = last_chunk_mask & chunk_value;

    return pbv;
}

bit_vector_t *bit_vector_cpy(const bit_vector_t *pbv)
{
    if (!pbv)
    {
        return NULL;
    }

    uint32_t chunk_count = get_chunk_count(pbv->size);
    bit_vector_t *new_pbv = malloc(sizeof(bit_vector_t) + chunk_count * sizeof(uint32_t));
    if (!new_pbv)
    {
        return NULL;
    }

    memcpy(new_pbv, pbv, sizeof(bit_vector_t) + chunk_count * sizeof(uint32_t));

    return new_pbv;
}

bit_t bit_vector_get(const bit_vector_t *pbv, size_t index)
{
    if (!pbv || index >= pbv->size)
    {
        return 0;
    }

    size_t chunk = index / CHUNK_SIZE;
    uint32_t chunk_index = index % CHUNK_SIZE;
    return (pbv->content[chunk] >> chunk_index) & 1;
}

bit_vector_t *bit_vector_not(bit_vector_t *pbv)
{
    if (!pbv)
    {
        return NULL;
    }

    size_t chunk_count = get_chunk_count(pbv->size);
    for (size_t i = 0; i < chunk_count - 1; i++)
    {
        pbv->content[i] = ~pbv->content[i];
    }
    size_t last_chunk_idx = pbv->size % CHUNK_SIZE;
    uint32_t last_chunk_mask = last_chunk_idx == 0 ? ~0 : ((1 << last_chunk_idx) - 1);
    pbv->content[chunk_count - 1] = last_chunk_mask & ~pbv->content[chunk_count - 1];

    return pbv;
}

bit_vector_t *bit_vector_and(bit_vector_t *pbv1, const bit_vector_t *pbv2)
{
    if (!pbv1 || !pbv2 || pbv1->size != pbv2->size)
    {
        return NULL;
    }

    size_t chunk_count = get_chunk_count(pbv1->size);
    for (size_t i = 0; i < chunk_count; i++)
    {
        pbv1->content[i] &= pbv2->content[i];
    }

    return pbv1;
}

bit_vector_t *bit_vector_or(bit_vector_t *pbv1, const bit_vector_t *pbv2)
{
    if (!pbv1 || !pbv2 || pbv1->size != pbv2->size)
    {
        return NULL;
    }

    size_t chunk_count = get_chunk_count(pbv1->size);
    for (size_t i = 0; i < chunk_count; i++)
    {
        pbv1->content[i] |= pbv2->content[i];
    }

    return pbv1;
}

bit_vector_t *bit_vector_xor(bit_vector_t *pbv1, const bit_vector_t *pbv2)
{
    if (!pbv1 || !pbv2 || pbv1->size != pbv2->size)
    {
        return NULL;
    }

    size_t chunk_count = get_chunk_count(pbv1->size);
    for (size_t i = 0; i < chunk_count; i++)
    {
        pbv1->content[i] ^= pbv2->content[i];
    }

    return pbv1;
}

uint32_t combine(uint32_t v1, uint32_t v2, size_t index)
{
    return v1 >> index || v2 << (CHUNK_SIZE - index);
}

static uint32_t bit_vector_get_chunk(const bit_vector_t *pbv, int64_t chunk, bool wrap) {
    if (wrap)
    {
        uint32_t val = 0;
        for (uint32_t bp = 32; bp > 0; bp--)
        {
            val = (val << 1) | bit_vector_get(pbv, (CHUNK_SIZE * chunk + bp - 1) % pbv->size);
        }
        return val;
    }

    int64_t chunk_count = get_chunk_count(pbv->size);
    if (chunk >= 0 && chunk < chunk_count)
    {
        return pbv->content[chunk];
    }
    return 0;
}

static uint32_t bit_vector_extract_chunk(const bit_vector_t *pbv, int64_t index, bool wrap) {
    if (!pbv)
    {
        return 0;
    }

    int64_t chunk = index >> 5;
    uint32_t chunk_index = (index % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
    if (chunk_index == 0)
    {
        return bit_vector_get_chunk(pbv, chunk, wrap);
    }

    return (bit_vector_get_chunk(pbv, chunk + 1, wrap) << (CHUNK_SIZE - chunk_index)) |
        (bit_vector_get_chunk(pbv, chunk, wrap) >> chunk_index);
}

static bit_vector_t *bit_vector_extract(const bit_vector_t *pbv, int64_t index, size_t size, bool wrap)
{
    if (size == 0)
    {
        return NULL;
    }

    size_t chunk_count = get_chunk_count(size);
    bit_vector_t *new_pbv = malloc(sizeof(bit_vector_t) + chunk_count * sizeof(uint32_t));
    if (!new_pbv)
    {
        return NULL;
    }

    new_pbv->size = size;
    for (size_t i = 0; i < chunk_count - 1; i++)
    {
        new_pbv->content[i] = bit_vector_extract_chunk(pbv, index + CHUNK_SIZE * i, wrap);
    }
    size_t last_chunk_idx = size % CHUNK_SIZE;
    uint32_t last_chunk_mask = last_chunk_idx == 0 ? ~0 : ((1 << last_chunk_idx) - 1);
    new_pbv->content[chunk_count - 1] = last_chunk_mask & bit_vector_extract_chunk(pbv, index + CHUNK_SIZE * (chunk_count - 1), wrap);

    return new_pbv;
}

bit_vector_t *bit_vector_extract_zero_ext(const bit_vector_t *pbv, int64_t index, size_t size)
{
    return bit_vector_extract(pbv, index, size, false);
}

bit_vector_t *bit_vector_extract_wrap_ext(const bit_vector_t *pbv, int64_t index, size_t size)
{
    if (!pbv)
    {
        return NULL;
    }

    return bit_vector_extract(pbv, index, size, true);
}

bit_vector_t *bit_vector_shift(const bit_vector_t* pbv, int64_t shift)
{
    if (!pbv)
    {
        return NULL;
    }

    return bit_vector_extract_zero_ext(pbv, -shift, pbv->size);
}

bit_vector_t* bit_vector_join(const bit_vector_t* pbv1, const bit_vector_t* pbv2, int64_t shift)
{
    if (!pbv1 || !pbv2 || pbv1->size != pbv2->size || shift < 0)
    {
        return NULL;
    }

    if (shift > (int64_t)pbv1->size)
    {
        return NULL;
    }

    bit_vector_t *ones = bit_vector_create(pbv1->size, 1);
    // décalage à droite de (total_bits - shift) pour obtenir un mask avec les shift LSB à 1 et les autres bits à 0
    bit_vector_t *mask1 = bit_vector_shift(ones, shift - pbv1->size);
    // décalage à gauche de shift pour obtenir un mask avec les (total_bits - shift) LSB à 0 et les autres bits à 1
    bit_vector_t *mask2 = bit_vector_shift(ones, shift);
    // on peut libérer la mémoire du vecteur que l'on a créé précedemment
    bit_vector_free(&ones);

    mask1 = bit_vector_and(mask1, pbv1);
    mask2 = bit_vector_and(mask2, pbv2);

    mask1 = bit_vector_or(mask1, mask2);

    bit_vector_free(&mask2);

    return mask1;
}

int bit_vector_print(const bit_vector_t *pbv)
{
    if (!pbv)
    {
        return 0;
    }

    for (size_t ip = pbv->size; ip > 0; ip--)
    {
        printf("%d", bit_vector_get(pbv, ip - 1));
    }

    return pbv->size;
}

int bit_vector_println(const char *prefix, const bit_vector_t *pbv)
{
    if (!pbv)
    {
        return 0;
    }

    printf("%s", prefix);
    int n = bit_vector_print(pbv);
    printf("\n");

    return strlen(prefix) + n + strlen("\n");
}

void bit_vector_free(bit_vector_t **pbv)
{
    free(*pbv);
    *pbv = NULL;
}
