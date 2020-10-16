/**
 * @file bootrom.c
 * @brief Gameboy Boot ROM
 *
 * @author Tancrède Guillou, Pablo Stebler
 * @date 2019
 */

#include "bootrom.h"
#include "error.h"
#include "cartridge.h"

int bootrom_init(component_t* c)
{
    M_REQUIRE_NON_NULL(c);
    M_REQUIRE_NO_ERR(component_create(c, MEM_SIZE(BOOT_ROM)));
    data_t content[MEM_SIZE(BOOT_ROM)] = GAMEBOY_BOOT_ROM_CONTENT;
    memcpy(c->mem->memory, content, MEM_SIZE(BOOT_ROM));
    return ERR_NONE;
}

int bootrom_bus_listener(gameboy_t* gameboy, addr_t addr)
{
    M_REQUIRE_NON_NULL(gameboy);

    if (REG_BOOT_ROM_DISABLE == addr && 1 == gameboy->boot)
    {
        M_REQUIRE_NO_ERR(bus_unplug(gameboy->bus, &gameboy->bootrom));
        M_REQUIRE_NO_ERR(cartridge_plug(&gameboy->cartridge, gameboy->bus));
        gameboy->boot = 0;
    }

    return ERR_NONE;
}
