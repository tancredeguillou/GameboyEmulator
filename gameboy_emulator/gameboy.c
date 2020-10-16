/**
 * @file gameboy.c
 * @brief Gameboy Code for GameBoy Emulator
 *
 * @author Tancrède Guillou, Pablo Stebler
 * @date 2019
 */

#include "gameboy.h"
#include "error.h"
#include "bootrom.h"
#include "assert.h"

static int component_connect(gameboy_t *gameboy, component_type type, size_t size, addr_t start, addr_t end)
{
    M_REQUIRE_NO_ERR(component_create(&gameboy->components[type], size));
    M_REQUIRE_NO_ERR(bus_plug(gameboy->bus, &gameboy->components[type], start, end));
    return ERR_NONE;
}

int gameboy_create(gameboy_t *gameboy, const char *filename)
{
    M_REQUIRE_NON_NULL(gameboy);

    gameboy->boot = 1;
    gameboy->cycles = 0;
    gameboy->nb_components = GB_NB_COMPONENTS;

    // STEP 1 : create the work RAM component and connect it to the bus
    M_REQUIRE_NO_ERR(component_connect(gameboy, WORK_RAM, MEM_SIZE(WORK_RAM), START(WORK_RAM), END(WORK_RAM)));

    // STEP 2 : share its memory with the echo RAM
    M_REQUIRE_NO_ERR(component_shared(&gameboy->echo, &gameboy->components[WORK_RAM]));
    
    // STEP 3 : connect echo RAM to the bus
    M_REQUIRE_NO_ERR(bus_plug(gameboy->bus, &gameboy->echo, ECHO_RAM_START, ECHO_RAM_END));

    // STEP 4-5-6-7-8 : create and connect the five other components present on the bus
    M_REQUIRE_NO_ERR(component_connect(gameboy, VIDEO_RAM, MEM_SIZE(VIDEO_RAM), START(VIDEO_RAM), END(VIDEO_RAM)));
    M_REQUIRE_NO_ERR(component_connect(gameboy, EXTERN_RAM, MEM_SIZE(EXTERN_RAM), START(EXTERN_RAM), END(EXTERN_RAM)));
    M_REQUIRE_NO_ERR(component_connect(gameboy, GRAPH_RAM, MEM_SIZE(GRAPH_RAM), START(GRAPH_RAM), END(GRAPH_RAM)));
    M_REQUIRE_NO_ERR(component_connect(gameboy, USELESS, MEM_SIZE(USELESS), START(USELESS), END(USELESS)));
    M_REQUIRE_NO_ERR(component_connect(gameboy, REGISTERS, MEM_SIZE(REGISTERS), START(REGISTERS), END(REGISTERS)));

    // STEP 9 : create and connect the cpu
    M_REQUIRE_NO_ERR(cpu_init(&gameboy->cpu));
    M_REQUIRE_NO_ERR(cpu_plug(&gameboy->cpu, &gameboy->bus));

    // STEP 10 : create and connect the cartridge
    M_REQUIRE_NO_ERR(cartridge_init(&gameboy->cartridge, filename));
    M_REQUIRE_NO_ERR(cartridge_plug(&gameboy->cartridge, gameboy->bus));

    // STEP 11 : create and connect the bootrom
    M_REQUIRE_NO_ERR(bootrom_init(&gameboy->bootrom));
    M_REQUIRE_NO_ERR(bootrom_plug(&gameboy->bootrom, gameboy->bus));

    // STEP 12 : initialize the timer
    M_REQUIRE_NO_ERR(timer_init(&gameboy->timer, &gameboy->cpu));

    M_REQUIRE_NO_ERR(lcdc_init(gameboy));
    M_REQUIRE_NO_ERR(lcdc_plug(&gameboy->screen, gameboy->bus));

    M_REQUIRE_NO_ERR(joypad_init_and_plug(&gameboy->pad, &gameboy->cpu));

    return ERR_NONE;
}

void gameboy_free(gameboy_t *gameboy)
{
    assert(gameboy);
    for (size_t i = 0; i < gameboy->nb_components; i++)
    {
        if (NULL != &gameboy->components[i])
        {
            assert(!bus_unplug(gameboy->bus, &gameboy->components[i]));
            component_free(&gameboy->components[i]);
        }
    }
    assert(!bus_unplug(gameboy->bus, &gameboy->echo));
    assert(!bus_unplug(gameboy->bus, &gameboy->bootrom));
    component_free(&gameboy->bootrom);
    assert(!bus_unplug(gameboy->bus, &gameboy->cartridge.c));
    cartridge_free(&gameboy->cartridge);
    assert(!bus_unplug(gameboy->bus, &gameboy->cpu.high_ram));
    lcdc_free(&gameboy->screen);
    cpu_free(&gameboy->cpu);
}

#ifdef BLARGG
static int blargg_bus_listener(gameboy_t *gameboy, addr_t addr)
{
    M_REQUIRE_NON_NULL(gameboy);
    if (BLARGG_REG == addr)
    {
        data_t data;
        M_REQUIRE_NO_ERR(bus_read(gameboy->bus, addr, &data));
        printf("%c", data);
    }
    return ERR_NONE;
}
#endif

int gameboy_run_until(gameboy_t *gameboy, uint64_t cycle)
{
    M_REQUIRE_NON_NULL(gameboy);
    if (cycle < gameboy->cycles)
    {
        return ERR_BAD_PARAMETER;
    }

    while (gameboy->cycles < cycle)
    {
        M_REQUIRE_NO_ERR(timer_cycle(&gameboy->timer));
        M_REQUIRE_NO_ERR(lcdc_cycle(&gameboy->screen, gameboy->cycles));
        M_REQUIRE_NO_ERR(cpu_cycle(&gameboy->cpu));

        M_REQUIRE_NO_ERR(timer_bus_listener(&gameboy->timer, gameboy->cpu.write_listener));
        M_REQUIRE_NO_ERR(bootrom_bus_listener(gameboy, gameboy->cpu.write_listener));
        M_REQUIRE_NO_ERR(lcdc_bus_listener(&gameboy->screen, gameboy->cpu.write_listener));
        M_REQUIRE_NO_ERR(joypad_bus_listener(&gameboy->pad, gameboy->cpu.write_listener));
        #ifdef BLARGG
        M_EXIT_IF_ERR(blargg_bus_listener(gameboy, gameboy->cpu.write_listener));
        #endif

        gameboy->cycles++;
    }

    return ERR_NONE;
}
