#pragma once

/**
 * @file cpu.h
 * @brief CPU model for PPS-GBemul project, high level interface
 *
 * @date 2019
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "alu.h"
#include "bus.h"
#include "opcode.h"

//=========================================================================
/**
 * @brief Type to represent CPU interupts
 */
typedef enum {
    VBLANK, LCD_STAT, TIMER, SERIAL, JOYPAD
} interrupt_t ;

#define INTERRUPT_BOUND    (interrupt_t) 5
#define NO_INTERRUPT    1 << INTERRUPT_BOUND

#define REG_IF          0xFF0F
#define REG_IE          0xFFFF
#define HIGH_RAM_START   0xFF80
#define HIGH_RAM_END     0xFFFE
#define HIGH_RAM_SIZE ((HIGH_RAM_END - HIGH_RAM_START)+1)

#define INTERRUPT_CYCLES    5
#define RST_ADDRESS(op) extract_n3(op) << 3

//=========================================================================
/**
 * @brief Type to represent CPU
 */
typedef struct
{
    // for each register, we can see them as two separate ones or as a single pair
    union
    {
        struct 
        {
            uint8_t F;
            uint8_t A;
        };
        uint16_t AF;
    };
    union
    {
        struct 
        {
            uint8_t C;
            uint8_t B;
        };
        uint16_t BC;
    };
    union
    {
        struct 
        {
            uint8_t E;
            uint8_t D;
        };
        uint16_t DE;
    };
    union
    {
        struct 
        {
            uint8_t L;
            uint8_t H;
        };
        uint16_t HL;
    };
    uint16_t PC;
    uint16_t SP;
    alu_output_t alu;
    bus_t *bus;
    bit_t IME;
    uint8_t IE;
    uint8_t IF;
    bit_t HALT;
    component_t high_ram;
    addr_t write_listener;
    uint8_t idle_time;
} cpu_t;

//=========================================================================
/**
 * @brief Run one CPU cycle
 * @param cpu (modified), the CPU which shall run
 * @param cycle, the cycle number to run, starting from 0
 * @return error code
 */
int cpu_cycle(cpu_t* cpu);


/**
 * @brief Plugs a bus into the cpu
 *
 * @param cpu cpu to plug into
 * @param bus bus to plug
 *
 * @return error code
 */
int cpu_plug(cpu_t* cpu, bus_t* bus);


/**
 * @brief Starts the cpu by initializing all registers at zero
 *
 * @param cpu cpu to start
 *
 * @return error code
 */
int cpu_init(cpu_t* cpu);


/**
 * @brief Frees a cpu
 *
 * @param cpu cpu to free
 */
void cpu_free(cpu_t* cpu);


/**
 * @brief Set an interruption
 */
void cpu_request_interrupt(cpu_t* cpu, interrupt_t i);


#ifdef __cplusplus
}
#endif
