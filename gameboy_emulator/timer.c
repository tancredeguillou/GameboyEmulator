/**
 * @file timer.c
 * @brief Game Boy Timer simulation code
 *
 * @author Tancrede Guillou, Pablo Stebler
 * @date 2019
 */

#include "timer.h"
#include "error.h"
#include "cpu-storage.h"
#include "assert.h"

static bit_t timer_state(gbtimer_t* timer)
{
    data_t tac = cpu_read_at_idx(timer->cpu, REG_TAC);
    bit_t counter_bit;
    // we only keep the 2 LSB
    switch (tac & 0x03)
    {
    case 0:
        counter_bit = bit_get(msb8(timer->counter), TAC_0_BIT - 8);
        break;
    case 1:
        counter_bit = bit_get(timer->counter, TAC_1_BIT);
        break;
    case 2:
        counter_bit = bit_get(timer->counter, TAC_2_BIT);
        break;
    case 3:
        counter_bit = bit_get(timer->counter, TAC_3_BIT);
        break;
    
    default:
        counter_bit = 0;
        break;
    }
    bit_t activated = bit_get(tac, ACTIVATION_BIT);

    return activated & counter_bit;
}

static int timer_incr_if_state_change(gbtimer_t* timer, bit_t old_state)
{
    bit_t new_state = timer_state(timer);
    if (old_state && !new_state)
    {
        data_t tima = cpu_read_at_idx(timer->cpu, REG_TIMA);
        if (tima == TIMER_MAX)
        {
            cpu_request_interrupt(timer->cpu, TIMER);
            M_REQUIRE_NO_ERR(cpu_write_at_idx(timer->cpu, REG_TIMA, cpu_read_at_idx(timer->cpu, REG_TMA)));
        }
        else
        {
            M_REQUIRE_NO_ERR(cpu_write_at_idx(timer->cpu, REG_TIMA, tima + 1));
        }
    }
    return ERR_NONE;
}

int timer_init(gbtimer_t* timer, cpu_t* cpu)
{
    M_REQUIRE_NON_NULL(timer);
    M_REQUIRE_NON_NULL(cpu);
    timer->cpu = cpu;
    timer->counter = 0;
    return ERR_NONE;
}

int timer_cycle(gbtimer_t* timer)
{
    M_REQUIRE_NON_NULL(timer);
    M_REQUIRE_NON_NULL(timer->cpu);

    bit_t cur_state = timer_state(timer);
    timer->counter += INC_CYCLE;
    M_REQUIRE_NO_ERR(cpu_write_at_idx(timer->cpu, REG_DIV, msb8(timer->counter)));
    M_REQUIRE_NO_ERR(timer_incr_if_state_change(timer, cur_state));
    return ERR_NONE;
}

int timer_bus_listener(gbtimer_t* timer, addr_t addr)
{
    M_REQUIRE_NON_NULL(timer);
    M_REQUIRE_NON_NULL(timer->cpu);

    switch (addr)
    {
    case REG_DIV:
    {
        bit_t cur_state = timer_state(timer);
        timer->counter = 0;
        M_REQUIRE_NO_ERR(timer_incr_if_state_change(timer, cur_state));
        break;
    }

    case REG_TAC:
        M_REQUIRE_NO_ERR(timer_incr_if_state_change(timer, timer_state(timer)));
        break;

    default:
        break;
    }
    return ERR_NONE;
}
