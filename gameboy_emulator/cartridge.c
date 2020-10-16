/**
 * @file cartridge.c
 * @brief Game Boy Cartridge simulation code
 *
 * @author Tancrède Guillou, Pablo Stebler
 * @date 2019
 */

#include "cartridge.h"
#include "error.h"

int cartridge_init_from_file(component_t* c, const char* filename)
{
    M_REQUIRE_NON_NULL(c);
    M_REQUIRE_NON_NULL(filename);

    FILE *file = fopen(filename, "rb"); // b is a no-op on Linux
    if (!file)
    {
        return ERR_IO;
    }
    if (BANK_ROM_SIZE != fread(c->mem->memory, 1, BANK_ROM_SIZE, file))
    {
        fclose(file);
        return ERR_IO;
    }
    fclose(file);

    if (0 != c->mem->memory[CARTRIDGE_TYPE_ADDR])
    {
        return ERR_NOT_IMPLEMENTED;
    }

    return ERR_NONE;
}

int cartridge_init(cartridge_t* ct, const char* filename)
{
    M_REQUIRE_NON_NULL(ct);
    M_REQUIRE_NO_ERR(component_create(&ct->c, BANK_ROM_SIZE));
    M_REQUIRE_NO_ERR(cartridge_init_from_file(&ct->c, filename));
    return ERR_NONE;
}

int cartridge_plug(cartridge_t* ct, bus_t bus)
{
    M_REQUIRE_NON_NULL(ct);
    M_REQUIRE_NON_NULL(bus);
    M_REQUIRE_NO_ERR(bus_forced_plug(bus, &ct->c, BANK_ROM0_START, BANK_ROM1_END, 0));
    return ERR_NONE;
}

void cartridge_free(cartridge_t* ct)
{
    if (NULL != ct)
    {
        component_free(&ct->c);
    }
}
