/**
 * @file component.c
 * @brief Game Boy Component simulation code
 *
 * @author Tancrède Guillou, Pablo Stebler
 * @date 2019
 */

#include <stdlib.h>

#include "component.h"
#include "error.h"

int check_component(component_t *c)
{
    if (c == NULL || c->end < c->start)
    {
        return ERR_BAD_PARAMETER;
    }
    return ERR_NONE;
}

int component_create(component_t *c, size_t mem_size)
{
    M_REQUIRE_NON_NULL(c);

    *c = (component_t) { 0 };

    if (mem_size != 0)
    {
        c->mem = calloc(1, sizeof(memory_t));
        M_REQUIRE_NON_NULL(c->mem);
        int create = mem_create(c->mem, mem_size);
        M_REQUIRE_NO_ERR(create);
    }

    return ERR_NONE;
}

void component_free(component_t *c)
{
    if (NULL != c)
    {
        if (c->mem != NULL)
        {
            mem_free(c->mem);
            free(c->mem);
        }
        *c = (component_t) { 0 };
    }
}

int component_shared(component_t *c, component_t *c_old)
{
    M_REQUIRE_NON_NULL(c);
    M_REQUIRE_NO_ERR(check_component(c_old));
    
    // STEP 1 : "disconnects" the c component (i.e sets the start and end to 0)
    c->start = 0;
    c->end = 0;
    // STEP 2 : makes c to use the same memory as c_old
    c->mem = c_old->mem;
    return ERR_NONE;
}
