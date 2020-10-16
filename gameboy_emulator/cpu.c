/**
 * @file cpu.c
 * @brief Game Boy CPU simulation
 *
 * @date 2019
 */

#include "error.h"
#include "cpu.h"
#include "cpu-alu.h"
#include "cpu-registers.h"
#include "cpu-storage.h"
#include "util.h"
#include "gameboy.h"
#include "bit.h"

#include <inttypes.h> // PRIX8
#include <stdio.h> // fprintf
#include <assert.h>

#define interrupt_address(interruption) \
    0x40 + ((addr_t)interruption << 3)

// ======================================================================
int cpu_init(cpu_t *cpu)
{
    M_REQUIRE_NON_NULL(cpu);

    *cpu = (cpu_t) { 0 };
    M_REQUIRE_NO_ERR(component_create(&cpu->high_ram, HIGH_RAM_SIZE));

    return ERR_NONE;
}

// ======================================================================
int cpu_plug(cpu_t *cpu, bus_t *bus)
{
    M_REQUIRE_NON_NULL(cpu);
    M_REQUIRE_NON_NULL(bus);

    cpu->bus = bus;
    M_REQUIRE_NO_ERR(bus_plug(*cpu->bus, &cpu->high_ram, HIGH_RAM_START, HIGH_RAM_END));

    (*cpu->bus)[REG_IE] = &cpu->IE;
    (*cpu->bus)[REG_IF] = &cpu->IF;

    return ERR_NONE;
}

// ======================================================================
void cpu_free(cpu_t *cpu)
{
    assert(cpu);
    component_free(&cpu->high_ram);
    if (cpu->bus)
    {
        (*cpu->bus)[REG_IE] = NULL;
        (*cpu->bus)[REG_IF] = NULL;
    }
    cpu->bus = NULL;
}

// ======================================================================
/**
 * @brief tool method for conditional instructions
 * 
 * @param lu : the instruction
 * @param cpu : the cpu which shall execute
 * 
 */
static bool check_cc(const instruction_t *lu, cpu_t *cpu)
{
    switch (extract_cc(lu->opcode))
    {
    // NZ : if Z is false
    case 0:
        return !get_Z(cpu->F);
    // Z : if Z is true
    case 1:
        return get_Z(cpu->F);
    // NC : if C is false
    case 2:
        return !get_C(cpu->F);
    // C : if C is true
    case 3:
        return get_C(cpu->F);
    default:
        assert(false);
        return false; // unreachable
    }
}

//=========================================================================
/**
 * @brief Executes an instruction
 * @param lu instruction
 * @param cpu, the CPU which shall execute
 * @return error code
 *
 * See opcode.h and cpu.h
 */
static int cpu_dispatch(const instruction_t *lu, cpu_t *cpu)
{
    M_REQUIRE_NON_NULL(lu);
    M_REQUIRE_NON_NULL(cpu);

    cpu->alu = (alu_output_t) { 0 };

    bool increment_pc = true;

    switch (lu->family)
    {

    // ALU
    case ADD_A_HLR:
    case ADD_A_N8:
    case ADD_A_R8:
    case INC_HLR:
    case INC_R8:
    case ADD_HL_R16SP:
    case INC_R16SP:
    case SUB_A_HLR:
    case SUB_A_N8:
    case SUB_A_R8:
    case DEC_HLR:
    case DEC_R8:
    case DEC_R16SP:
    case AND_A_HLR:
    case AND_A_N8:
    case AND_A_R8:
    case OR_A_HLR:
    case OR_A_N8:
    case OR_A_R8:
    case XOR_A_HLR:
    case XOR_A_N8:
    case XOR_A_R8:
    case CPL:
    case CP_A_HLR:
    case CP_A_N8:
    case CP_A_R8:
    case SLA_HLR:
    case SLA_R8:
    case SRA_HLR:
    case SRA_R8:
    case SRL_HLR:
    case SRL_R8:
    case ROTCA:
    case ROTA:
    case ROTC_HLR:
    case ROT_HLR:
    case ROTC_R8:
    case ROT_R8:
    case SWAP_HLR:
    case SWAP_R8:
    case BIT_U3_HLR:
    case BIT_U3_R8:
    case CHG_U3_HLR:
    case CHG_U3_R8:
    case LD_HLSP_S8:
    case DAA:
    case SCCF:
        M_EXIT_IF_ERR(cpu_dispatch_alu(lu, cpu));
        break;

    // STORAGE
    case LD_A_BCR:
    case LD_A_CR:
    case LD_A_DER:
    case LD_A_HLRU:
    case LD_A_N16R:
    case LD_A_N8R:
    case LD_BCR_A:
    case LD_CR_A:
    case LD_DER_A:
    case LD_HLRU_A:
    case LD_HLR_N8:
    case LD_HLR_R8:
    case LD_N16R_A:
    case LD_N16R_SP:
    case LD_N8R_A:
    case LD_R16SP_N16:
    case LD_R8_HLR:
    case LD_R8_N8:
    case LD_R8_R8:
    case LD_SP_HL:
    case POP_R16:
    case PUSH_R16:
        M_EXIT_IF_ERR(cpu_dispatch_storage(lu, cpu));
        break;


    // JUMP
    case JP_CC_N16:
        if (check_cc(lu, cpu))
        {
            cpu->PC = cpu_read_addr_after_opcode(cpu);
            cpu->idle_time += lu->xtra_cycles;
            increment_pc = false;
        }
        break;

    case JP_HL:
        cpu->PC = cpu_HL_get(cpu);
        increment_pc = false;
        break;

    case JP_N16:
        cpu->PC = cpu_read_addr_after_opcode(cpu);
        increment_pc = false;
        break;

    case JR_CC_E8:
        if (check_cc(lu, cpu))
        {
            cpu->PC += (int8_t)cpu_read_data_after_opcode(cpu);
            cpu->idle_time += lu->xtra_cycles;
        }
        break;

    case JR_E8:
        cpu->PC += (int8_t)cpu_read_data_after_opcode(cpu);
        break;


    // CALLS
    case CALL_CC_N16:
        if (check_cc(lu, cpu))
        {
            M_REQUIRE_NO_ERR(cpu_SP_push(cpu, cpu->PC + lu->bytes));
            cpu->PC = cpu_read_addr_after_opcode(cpu);
            cpu->idle_time += lu->xtra_cycles;
            increment_pc = false;
        }
        break;

    case CALL_N16:
        M_REQUIRE_NO_ERR(cpu_SP_push(cpu, cpu->PC + lu->bytes));
        cpu->PC = cpu_read_addr_after_opcode(cpu);
        increment_pc = false;
        break;


    // RETURN (from call)
    case RET:
        cpu->PC = cpu_SP_pop(cpu);
        increment_pc = false;
        break;

    case RET_CC:
        if (check_cc(lu, cpu))
        {
            cpu->PC = cpu_SP_pop(cpu);
            cpu->idle_time += lu->xtra_cycles;
            increment_pc = false;
        }
        break;

    case RST_U3:
        cpu_SP_push(cpu, cpu->PC + 1);
        cpu->PC = RST_ADDRESS(lu->opcode);
        increment_pc = false;
        break;


    // INTERRUPT & MISC.
    case EDI:
        cpu->IME = extract_ime(lu->opcode);
        break;

    case RETI:
        cpu->IME = 1;
        cpu->PC = cpu_SP_pop(cpu);
        increment_pc = false;
        break;

    case HALT:
        cpu->HALT = 1;
        break;

    case STOP:
    case NOP:
        // ne rien faire
        break;

    default: {
        fprintf(stderr, "Unknown instruction, Code: 0x%" PRIX8 "\n", cpu_read_at_idx(cpu, cpu->PC));
        return ERR_INSTR;
    } break;

    } // switch

    if (increment_pc)
    {
        cpu->PC += lu->bytes;
    }

    cpu->idle_time += lu->cycles;

    return ERR_NONE;
}

// ----------------------------------------------------------------------
static interrupt_t look_for_interrupt(cpu_t *cpu)
{
    interrupt_t interrupt = cpu->IF & cpu->IE;
    for (interrupt_t i = 0; i < INTERRUPT_COUNT; i++)
    {
        if ((interrupt >> i) & 1)
        {
            return i;
        }
    }
    return INTERRUPT_COUNT;
}

// ----------------------------------------------------------------------
static int cpu_do_cycle(cpu_t *cpu)
{
    M_REQUIRE_NON_NULL(cpu);
    interrupt_t interrupt = look_for_interrupt(cpu);
    // if there is an interruption to be handled.
    if (1 == cpu->IME && interrupt != INTERRUPT_COUNT)
    {
        cpu->IME = 0;
        cpu->IF &= ~(1 << interrupt);
        cpu_SP_push(cpu, cpu->PC);
        cpu->PC = interrupt_address(interrupt);
        cpu->idle_time += INTERRUPT_CYCLES;
    }
    else
    {
        data_t opcode = cpu_read_at_idx(cpu, cpu->PC);
        instruction_t instruction;
        // if we have a prefix, the instruction is defined by the second byte of the opcode.
        if (PREFIXED == opcode)
        {
            instruction = instruction_prefixed[cpu_read_data_after_opcode(cpu)];
        }
        else
        {
            instruction = instruction_direct[opcode];
        }
        M_REQUIRE_NO_ERR(cpu_dispatch(&instruction, cpu));
    }
    return ERR_NONE;
}

// ======================================================================
/**
 * See cpu.h
 */
int cpu_cycle(cpu_t *cpu)
{
    M_REQUIRE_NON_NULL(cpu);
    M_REQUIRE_NON_NULL(cpu->bus);

    // Set the listening address back to zero
    cpu->write_listener = 0;

    // if we are in halt mode...
    if (1 == cpu->HALT)
    {
        // we can only unlock the CPU if an interruption is waiting
        if (INTERRUPT_COUNT != look_for_interrupt(cpu))
        {
            cpu->HALT = 0;
            M_REQUIRE_NO_ERR(cpu_do_cycle(cpu));
        }
    }
    // if we are not in halt mode...
    else
    {
        // if the idle time is zero, then the previous instruction is finsihed and we can call a new instruction
        if (0 == cpu->idle_time)
        {
            M_REQUIRE_NO_ERR(cpu_do_cycle(cpu));
        }
        /* we always decrement idle_time because :
         *  - either idle_time was 0 and the cycles value of the new instruction was affected to idle_time
         *  - either is was non zero and therefore we need to decrement it
         */
        --cpu->idle_time;
    }
    return ERR_NONE;
}

void cpu_request_interrupt(cpu_t* cpu, interrupt_t i)
{
    if (NULL != cpu && i < INTERRUPT_COUNT)
    {
        cpu->IF |= (1 << i);
    }
}
