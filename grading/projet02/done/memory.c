/**
 * @file memory.c
 * @brief Memory for GameBoy code
 *
 * @date 2019
 */

#include <stdlib.h>
#include <assert.h>

#include "memory.h"
#include "error.h"

int mem_create(memory_t *mem, size_t size)
{
    M_REQUIRE_NON_NULL(mem);
    if (size == 0)
    {
        return ERR_BAD_PARAMETER;
    }

    mem->memory = calloc(size, sizeof(data_t));
    if (!mem->memory) {
        return ERR_MEM;
    }
    mem->size = size;
    return ERR_NONE;
}

void mem_free(memory_t *mem)
{
    assert(mem);
    free(mem->memory);
    mem->memory = NULL;
    mem->size = 0;
}
