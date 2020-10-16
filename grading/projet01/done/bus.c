/**
 * @file bus.c
 * @brief Game Boy Bus Emulator code
 *
 * @date 2019
 */

#include "bus.h"
#include "bit.h"
#include "error.h"

int bus_remap(bus_t bus, component_t *c, addr_t offset)
{
    // we start by checking the validity of the arguments
    int check = check_component(c);
    M_REQUIRE_NO_ERR(check);
    M_REQUIRE_NON_NULL(c->mem);
    if (c->end - c->start + offset >= c->mem->size)
    {
        return ERR_ADDRESS;
    }

    // we assign new pointers to the bus
    for (addr_t i = 0; i <= c->end - c->start; i++)
    {
        bus[c->start + i] = &c->mem->memory[offset + i];
    }
    return ERR_NONE;
}

int bus_forced_plug(bus_t bus, component_t *c, addr_t start, addr_t end, addr_t offset)
{
    // we start by checking the validity of the arguments
    int check = check_component(c);
    M_REQUIRE_NO_ERR(check);
    if (start > end)
    {
        return ERR_BAD_PARAMETER;
    }

    // change start and end adress values of the component for new plug
    c->start = start;
    c->end = end;
    int remap = bus_remap(bus, c, offset);
    // in case of failure, the component is disconnected
    if (ERR_NONE != remap)
    {
        c->start = 0;
        c->end = 0;
        return remap;
    }
    return ERR_NONE;
}

int bus_plug(bus_t bus, component_t *c, addr_t start, addr_t end)
{
    int check = check_component(c);
    M_REQUIRE_NO_ERR(check);
    if (start > end)
    {
        return ERR_BAD_PARAMETER;
    }

    /* returns the error code ERR_ADDRESS if at least part 
     * of the provided range is already occupied (and then makes no connection).
     */
    for (addr_t i = 0; i <= end - start; i++)
    {
        //M_REQUIRE_NON_NULL_CUSTOM_ERR(bus[start + i], ERR_ADDRESS);
        if (bus[start + i])
        {
            return ERR_ADDRESS;
        }
    }

    return bus_forced_plug(bus, c, start, end, 0);
}

int bus_unplug(bus_t bus, component_t *c)
{
    int check = check_component(c);
    M_REQUIRE_NO_ERR(check);

    for (addr_t i = 0; i < c->end - c->start; i++)
    {
        bus[c->start + i] = NULL;
    }
    c->start = 0;
    c->end = 0;
    return ERR_NONE;
}

int bus_read(const bus_t bus, addr_t address, data_t* data)
{
    M_REQUIRE_NON_NULL(data);

    if (bus[address] == NULL)
    {
        *data = 0xFF;
    }
    else
    {
        *data = *bus[address];
    }
    return ERR_NONE;
}

int bus_read16(const bus_t bus, addr_t address, addr_t* data16)
{
    M_REQUIRE_NON_NULL(data16);

    if (bus[address] == NULL)
    {
        *data16 = 0xFF;
    }
    else
    {
        *data16 = merge8(*bus[address], *bus[address + 1]);
    }
    return ERR_NONE;
}

int bus_write(bus_t bus, addr_t address, data_t data)
{
    M_REQUIRE_NON_NULL(bus[address]);
    
    *bus[address] = data;
    return ERR_NONE;
}

int bus_write16(bus_t bus, addr_t address, addr_t data16)
{
    M_REQUIRE_NON_NULL(bus[address]);
        
    data_t first_byte = lsb8(data16);
    *bus[address] = first_byte;
    data_t second_byte = msb8(data16);
    *bus[address + 1] = second_byte;
    return ERR_NONE;
}
