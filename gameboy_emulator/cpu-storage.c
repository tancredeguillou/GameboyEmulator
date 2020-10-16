/**
 * @file cpu-registers.c
 * @brief Game Boy CPU simulation, register part
 *
 * @date 2019
 */

#include "error.h"
#include "cpu-storage.h" // cpu_read_at_HL
#include "cpu-registers.h" // cpu_BC_get
#include "gameboy.h" // REGISTER_START
#include "util.h"
#include <inttypes.h> // PRIX8
#include <stdio.h> // fprintf
#include <assert.h>

// ==== see cpu-storage.h ========================================
data_t cpu_read_at_idx(const cpu_t* cpu, addr_t addr)
{
    assert(cpu);
    assert(cpu->bus);
    data_t result;
    assert(!bus_read(*cpu->bus, addr, &result));
    return result;
}

// ==== see cpu-storage.h ========================================
addr_t cpu_read16_at_idx(const cpu_t* cpu, addr_t addr)
{
    assert(cpu);
    assert(cpu->bus);
    addr_t result;
    assert(!bus_read16(*cpu->bus, addr, &result));
    return result;
}

// ==== see cpu-storage.h ========================================
int cpu_write_at_idx(cpu_t* cpu, addr_t addr, data_t data)
{
    M_REQUIRE_NON_NULL(cpu);
    M_REQUIRE_NON_NULL(cpu->bus);
    M_REQUIRE_NO_ERR(bus_write(*cpu->bus, addr, data));
    cpu->write_listener = addr;
    return ERR_NONE;
}

// ==== see cpu-storage.h ========================================
int cpu_write16_at_idx(cpu_t* cpu, addr_t addr, addr_t data16)
{
    M_REQUIRE_NON_NULL(cpu);
    M_REQUIRE_NON_NULL(cpu->bus);
    M_REQUIRE_NO_ERR(bus_write16(*cpu->bus, addr, data16));
    cpu->write_listener = addr;
    return ERR_NONE;
}

// ==== see cpu-storage.h ========================================
int cpu_SP_push(cpu_t* cpu, addr_t data16)
{
    M_REQUIRE_NON_NULL(cpu);
    // Step 1 : Decrements the address in the stack pointer (SP register) by 2 units
    cpu->SP -= SP_UNITS;
    // Step 2 : Writes the 16 bits given value to this new address
    int result = cpu_write16_at_idx(cpu, cpu->SP, data16);
    // if it didn't work, put the address of the stack pointer at its previous value
    if (ERR_NONE != result)
    {
        cpu->SP += SP_UNITS;
    }
    return result;
}

// ==== see cpu-storage.h ========================================
addr_t cpu_SP_pop(cpu_t* cpu)
{
    assert(cpu);
    // Step 1 : Reads the 16 bits given value from the bus at the address inside the stack pointer (SP register)
    int result = cpu_read16_at_idx(cpu, cpu->SP);
    // Step 2 : Increments the address in the stack pointer (SP register) by 2 units
    cpu->SP += SP_UNITS;
    return result;
}

static void load(cpu_t *cpu, reg_kind reg, addr_t addr) {
    cpu_reg_set(cpu, reg, cpu_read_at_idx(cpu, addr));
}

static int store(cpu_t *cpu, reg_kind reg, addr_t addr) {
    return cpu_write_at_idx(cpu, addr, cpu_reg_get(cpu, reg));
}

// ==== see cpu-storage.h ========================================
int cpu_dispatch_storage(const instruction_t* lu, cpu_t* cpu)
{
    M_REQUIRE_NON_NULL(cpu);

    switch (lu->family) {
    case LD_A_BCR:
        load(cpu, REG_A_CODE, cpu_BC_get(cpu));
        break;

    case LD_A_CR:
        load(cpu, REG_A_CODE, REGISTERS_START + cpu_reg_get(cpu, REG_C_CODE));
        break;

    case LD_A_DER:
        load(cpu, REG_A_CODE, cpu_DE_get(cpu));
        break;

    case LD_A_HLRU:
        load(cpu, REG_A_CODE, cpu_HL_get(cpu));
        cpu_HL_set(cpu, cpu_HL_get(cpu) + extract_HL_increment(lu->opcode));
        break;

    case LD_A_N16R:
        load(cpu, REG_A_CODE, cpu_read_addr_after_opcode(cpu));
        break;

    case LD_A_N8R:
        load(cpu, REG_A_CODE, REGISTERS_START + cpu_read_data_after_opcode(cpu));
        break;

    case LD_BCR_A:
        M_REQUIRE_NO_ERR(store(cpu, REG_A_CODE, cpu_BC_get(cpu)));
        break;

    case LD_CR_A:
        M_REQUIRE_NO_ERR(store(cpu, REG_A_CODE, REGISTERS_START + cpu_reg_get(cpu, REG_C_CODE)));
        break;

    case LD_DER_A:
        M_REQUIRE_NO_ERR(store(cpu, REG_A_CODE, cpu_DE_get(cpu)));
        break;

    case LD_HLRU_A:
        M_REQUIRE_NO_ERR(store(cpu, REG_A_CODE, cpu_HL_get(cpu)));
        cpu_HL_set(cpu, cpu_HL_get(cpu) + extract_HL_increment(lu->opcode));
        break;

    case LD_HLR_N8:
        M_REQUIRE_NO_ERR(cpu_write_at_idx(cpu, cpu_HL_get(cpu), cpu_read_data_after_opcode(cpu)));
        break;

    case LD_HLR_R8:
        M_REQUIRE_NO_ERR(store(cpu, extract_reg(lu->opcode, 0), cpu_HL_get(cpu)));
        break;

    case LD_N16R_A:
        M_REQUIRE_NO_ERR(store(cpu, REG_A_CODE, cpu_read_addr_after_opcode(cpu)));
        break;

    case LD_N16R_SP:
        M_REQUIRE_NO_ERR(cpu_write16_at_idx(cpu, cpu_read_addr_after_opcode(cpu), cpu->SP));
        break;

    case LD_N8R_A:
        M_REQUIRE_NO_ERR(store(cpu, REG_A_CODE, REGISTERS_START + cpu_read_data_after_opcode(cpu)));
        break;

    case LD_R16SP_N16:
        cpu_reg_pair_SP_set(cpu, extract_reg_pair(lu->opcode), cpu_read_addr_after_opcode(cpu));
        break;

    case LD_R8_HLR:
        load(cpu, extract_reg(lu->opcode, 3), cpu_HL_get(cpu));
        break;

    case LD_R8_N8:
        cpu_reg_set(cpu, extract_reg(lu->opcode, 3), cpu_read_data_after_opcode(cpu));
        break;

    case LD_R8_R8: {
        reg_kind dst = extract_reg(lu->opcode, 3);
        reg_kind src = extract_reg(lu->opcode, 0);
        if (dst == src)
        {
            return ERR_INSTR;
        }
        cpu_reg_set(cpu, dst, cpu_reg_get(cpu, src));
    } break;

    case LD_SP_HL:
        cpu->SP = cpu_HL_get(cpu);
        break;

    case POP_R16:
        cpu_reg_pair_set(cpu, extract_reg_pair(lu->opcode), cpu_SP_pop(cpu));
        break;

    case PUSH_R16:
        M_REQUIRE_NO_ERR(cpu_SP_push(cpu, cpu_reg_pair_get(cpu, extract_reg_pair(lu->opcode))));
        break;

    default:
        fprintf(stderr, "Unknown STORAGE instruction, Code: 0x%" PRIX8 "\n", cpu_read_at_idx(cpu, cpu->PC));
        return ERR_INSTR;
        break;
    } // switch

    return ERR_NONE;
}
