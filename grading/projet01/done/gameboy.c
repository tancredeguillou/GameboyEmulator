/**
 * @file gameboy.c
 * @brief Gameboy Code for GameBoy Emulator
 *
 * @date 2019
 */

#include "gameboy.h"
#include "error.h"

int gameboy_create(gameboy_t *gameboy, const char *filename)
{
    M_REQUIRE_NON_NULL(gameboy);

    // STEP 1.1 : create the work RAM component
    int create_component = component_create(&gameboy->components[WORK], MEM_SIZE(WORK_RAM));
    M_REQUIRE_NO_ERR(create_component);

    // STEP 1.2 : connect it to the bus
    int plug_work = bus_plug(gameboy->bus, &gameboy->components[WORK], WORK_RAM_START, WORK_RAM_END);
    M_REQUIRE_NO_ERR(plug_work);

    // STEP 2 : share its memory with the echo RAM
    component_t echo;
    int share = component_shared(&echo, &gameboy->components[WORK]);
    M_REQUIRE_NO_ERR(share);
    
    // STEP 3 : connect echo RAM to the bus
    int plug_echo = bus_plug(gameboy->bus, &echo, ECHO_RAM_START, ECHO_RAM_END);
    M_REQUIRE_NO_ERR(plug_echo);

    return ERR_NONE;
}

void gameboy_free(gameboy_t *gameboy)
{
    for (size_t i = 0; i < GB_NB_COMPONENTS; i++)
    {
        bus_unplug(gameboy->bus, &gameboy->components[i]);
        component_free(&gameboy->components[i]);
    }
}
