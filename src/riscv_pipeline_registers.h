/*
 * External routines for use in CMPE 110, Fall 2018
 *
 * (c) 2018 Ethan L. Miller
 * Redistribution not permitted without explicit permission from the copyright holder.
 *
 */

/*
 * IMPORTANT: rename this file to riscv_pipeline_registers.h
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "riscv.h"

/*
 * These pipeline stage registers are loaded at the end of the cycle with whatever
 * values were filled in by the relevant pipeline stage.
 *
 * Add fields to these stage registers for whatever values you'd like to have passed from
 * one stage to the next.  You may have as many values as you like in each stage.  However,
 * this is the ONLY information that may be passed from one stage to stages in the next
 * cycle.
 */


struct stage_reg_d {
    uint64_t    pc;
    uint64_t    new_pc;
    uint32_t    instruction;
    uint8_t     not_stalled;
    uint8_t     will_be_stalled;
    uint8_t     tlb_stall_status;
};

struct stage_reg_x {
    uint64_t                    pc;
    uint64_t                    new_pc;
    struct riscv_instruction    instruction;
    uint8_t                     not_stalled;
    uint64_t                    rs1_value;
    uint64_t                    rs2_value;
    int16_t                     rs1;
    int16_t                     rs2;
    int16_t                     rd;
};

struct stage_reg_m {
    uint64_t    address; // address of memory
    uint8_t     size; // size of operation
    uint8_t     readWrite; // 3 for register write, 2 for memory read, 1 for memory write, 0 for nop
    uint8_t     signExtend; // 1 for doing sign extensions to 64-bit, 0 otherwise
    uint64_t    reg; // register to read into for reads, or write into for register writes
    uint64_t    value; // value to write
    uint8_t     tainted_executions;
    uint8_t     wasStalled;
    uint8_t     stallStatus;
    uint64_t    pc;
};

struct stage_reg_w {
    uint64_t reg; // (uint64_t) -1 for nop
    uint64_t value;
    uint8_t  op; // 0 is nop, 1 is do writing
    uint8_t global_memory_stall;
    uint8_t tlb_stall_status;
    uint8_t tainted_executions;
    struct stage_reg_m memory_register;
};

