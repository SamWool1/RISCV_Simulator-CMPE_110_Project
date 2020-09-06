
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "riscv.h"
#include "riscv_sim_framework.h"
#include "riscv_pipeline_registers.h"
#include "riscv_pipeline_registers_vars.h"
#include "limits.h"
#include "branch_predictor.h"
#include "cache.h"
#include "TLB.h"

uint8_t forwarded_register_read_single (uint64_t reg, uint64_t* value) {
    if (current_stage_x_register->rd == reg) {
        return 2;
    } else if (current_stage_m_register->readWrite == 3 && current_stage_m_register->reg == reg) {
        *value = current_stage_m_register->value;
        return 1;
    } else if (current_stage_w_register->reg == reg) {
        *value = current_stage_w_register->value;
        return 1;
    }
    return 0;
}

int forwarded_register_read (uint64_t register_a, uint64_t register_b, uint64_t * value_a, uint64_t * value_b) {
    uint8_t aForwarded = forwarded_register_read_single(register_a, value_a);
    uint8_t bForwarded = forwarded_register_read_single(register_b, value_b);
    if (aForwarded == 2 || bForwarded == 2) {
        return 1;
    }
    if (aForwarded && bForwarded) {
        return 0;
    } else if (aForwarded) {
        register_read(register_b, register_b, value_b, value_b);
    } else if (bForwarded) {
        register_read(register_a, register_a, value_a, value_a);
    } else {
        register_read(register_a, register_b, value_a, value_b);
    }
    return 0;
}

void riscv_illegal_instruction(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    printf("Illegal instruction @ 0x%016lX: 0x%08X\n", (*pc) - 4, *(uint32_t*) &instruction);
}

void riscv_nop(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    // does nothing intentionally
}

void prepare_register_write(uint64_t reg, uint64_t value, struct stage_reg_m* new_m_reg) {
    new_m_reg->readWrite = 3;
    new_m_reg->reg = reg;
    new_m_reg->value = value;
}

void riscv_auipc(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads PC + immediate into upper 52 bits of a register-
	uint64_t temp = *pc + (int64_t) (int32_t) (instruction.data.u.imm << 12);
    prepare_register_write(instruction.data.u.rd, temp, new_m_reg);
}

void riscv_lui(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads immediate into [31:12] of a register
	uint64_t temp = instruction.data.u.imm << 12;
    prepare_register_write(instruction.data.u.rd, (int64_t) (int32_t) temp, new_m_reg);
}

void riscv_jal(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Stores newPC to rd and sets currPC to the old PC + imm
    prepare_register_write(instruction.data.u.rd, *pc, new_m_reg);
    uint64_t immediate = instruction.data.u.imm;
    const uint64_t mask = 0xFFFFF;
    immediate = ((immediate >> 19) & 0b1) << 19 | (mask & (immediate << 12)) >> 1 | ((immediate >> 8) & 0b1) << 10 | (mask & (immediate << 1)) >> 10;
    if (immediate & (1 << 19)) {
        immediate |= 0b1111 << 20;
        immediate |= 0xFFFFFFFFFF000000;
    }
    int64_t offset = immediate << 1;
	*pc = *pc - 4 + offset; // necessary decrement to retrieve oldPC
}

void riscv_jalr(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (instruction.data.i.funct3 != 0) {
        riscv_illegal_instruction(pc, instruction, NULL);
    }

    // Stores newPC to rd and sets currPC to rs1+imm
    prepare_register_write(instruction.data.i.rd, *pc, new_m_reg);
    uint64_t immediate = instruction.data.i.imm;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;
    *pc = offset + current_stage_x_register->rs1_value;
}

void riscv_lb(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads the lower 8 bits of the value at imm(rs1) into rd w/ sign extension
    uint64_t immediate = instruction.data.i.imm;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->readWrite = 2;
    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->size = 1;
    new_m_reg->reg = instruction.data.i.rd;
    new_m_reg->signExtend = 1;
}

void riscv_lh(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads the lower 16 bits of the value at imm(rs1) into rd w/ sign extension
    uint64_t immediate = instruction.data.i.imm;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->readWrite = 2;
    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->size = 2;
    new_m_reg->reg = instruction.data.i.rd;
    new_m_reg->signExtend = 1;
}

void riscv_lw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads the lower 32 bits of the value at imm(rs1) into rd w/ sign extension
    uint64_t immediate = instruction.data.i.imm;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->readWrite = 2;
    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->size = 4;
    new_m_reg->reg = instruction.data.i.rd;
    new_m_reg->signExtend = 1;
}

void riscv_ld(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads the value at imm(rs1) into rd
	uint64_t immediate = instruction.data.i.imm;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->readWrite = 2;
    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->size = 8;
    new_m_reg->reg = instruction.data.i.rd;
    new_m_reg->signExtend = 0;
}

void riscv_lbu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads the lower 8 bits of the value at imm(rs1) into rd w/o sign extension
    uint64_t immediate = instruction.data.i.imm;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->readWrite = 2;
    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->size = 1;
    new_m_reg->reg = instruction.data.i.rd;
    new_m_reg->signExtend = 0;
}

void riscv_lhu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads the lower 16 bits of a value at imm(rs1) into rd w/o sign extension
    uint64_t immediate = instruction.data.i.imm;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->readWrite = 2;
    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->size = 2;
    new_m_reg->reg = instruction.data.i.rd;
    new_m_reg->signExtend = 0;
}

void riscv_lwu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
	// Loads the lower 32 bits of a value at imm(rs1) into rd w/o sign extension
    uint64_t immediate = instruction.data.i.imm;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->readWrite = 2;
    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->size = 4;
    new_m_reg->reg = instruction.data.i.rd;
    new_m_reg->signExtend = 0;
}

void riscv_load_dispatch(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    switch(instruction.data.i.funct3) {
        case 0b000:
            riscv_lb(pc, instruction, new_m_reg);
            break;
        case 0b001:
            riscv_lh(pc, instruction, new_m_reg);
            break;
        case 0b010:
            riscv_lw(pc, instruction, new_m_reg);
            break;
        case 0b011:
            riscv_ld(pc, instruction, new_m_reg);
            break;
        case 0b100:
            riscv_lbu(pc, instruction, new_m_reg);
            break;
        case 0b101:
            riscv_lhu(pc, instruction, new_m_reg);
            break;
        case 0b110:
            riscv_lwu(pc, instruction, new_m_reg);
            break;
        default:
            riscv_illegal_instruction(pc, instruction, NULL);
    }
}

void riscv_beq(uint64_t* pc, struct riscv_instruction instruction) {
    uint32_t raw_range = (uint32_t) instruction.data.s.imm >> 6 << 11 | (uint32_t) (instruction.data.s.imm2 & 0x01) | (uint32_t) (instruction.data.s.imm & 0b0111111) << 5 | (uint32_t) instruction.data.s.imm2 >> 1 << 1;
    uint16_t sized_range = (uint16_t) (raw_range & 0b111111111110);
    int16_t extended_range = (int16_t) (sized_range | ((sized_range & (11 << 1)) ? 0b1111 << 12 : 0));

    if (current_stage_x_register->rs1_value == current_stage_x_register->rs2_value) {
        *pc = *pc - 4 + extended_range;
    }
}

void riscv_bne(uint64_t* pc, struct riscv_instruction instruction) {
    uint32_t raw_range = (uint32_t) instruction.data.s.imm >> 6 << 11 | (uint32_t) (instruction.data.s.imm2 & 0x01) | (uint32_t) (instruction.data.s.imm & 0b0111111) << 5 | (uint32_t) instruction.data.s.imm2 >> 1 << 1;
    uint16_t sized_range = (uint16_t) (raw_range & 0b111111111110);
    int16_t extended_range = (int16_t) (sized_range | ((sized_range & (11 << 1)) ? 0b1111 << 12 : 0));

    if (current_stage_x_register->rs1_value != current_stage_x_register->rs2_value) {
        *pc = *pc - 4 + extended_range;
    }
}

void riscv_blt(uint64_t* pc, struct riscv_instruction instruction) {
    uint32_t raw_range = (uint32_t) instruction.data.s.imm >> 6 << 11 | (uint32_t) (instruction.data.s.imm2 & 0x01) | (uint32_t) (instruction.data.s.imm & 0b0111111) << 5 | (uint32_t) instruction.data.s.imm2 >> 1 << 1;
    uint16_t sized_range = (uint16_t) (raw_range & 0b111111111110);
    int16_t extended_range = (int16_t) (sized_range | ((sized_range & (11 << 1)) ? 0b1111 << 12 : 0));


    if ((int64_t) current_stage_x_register->rs1_value < (int64_t) current_stage_x_register->rs2_value) {
        *pc = *pc - 4 + extended_range;
    }
}

void riscv_bge(uint64_t* pc, struct riscv_instruction instruction) {
    uint32_t raw_range = (uint32_t) instruction.data.s.imm >> 6 << 11 | (uint32_t) (instruction.data.s.imm2 & 0x01) | (uint32_t) (instruction.data.s.imm & 0b0111111) << 5 | (uint32_t) instruction.data.s.imm2 >> 1 << 1;
    uint16_t sized_range = (uint16_t) (raw_range & 0b111111111110);
    int16_t extended_range = (int16_t) (sized_range | ((sized_range & (11 << 1)) ? 0b1111 << 12 : 0));


    if ((int64_t) current_stage_x_register->rs1_value >= (int64_t) current_stage_x_register->rs2_value) {
        *pc = *pc - 4 + extended_range;
    }
}

void riscv_bltu(uint64_t* pc, struct riscv_instruction instruction) {
    uint32_t raw_range = (uint32_t) instruction.data.s.imm >> 6 << 11 | (uint32_t) (instruction.data.s.imm2 & 0x01) | (uint32_t) (instruction.data.s.imm & 0b0111111) << 5 | (uint32_t) instruction.data.s.imm2 >> 1 << 1;
    uint16_t sized_range = (uint16_t) (raw_range & 0b111111111110);
    int16_t extended_range = (int16_t) (sized_range | ((sized_range & (11 << 1)) ? 0b1111 << 12 : 0));


    if (current_stage_x_register->rs1_value < current_stage_x_register->rs2_value) {
        *pc = *pc - 4 + extended_range;
    }}

void riscv_bgeu(uint64_t* pc, struct riscv_instruction instruction) {
    uint32_t raw_range = (uint32_t) instruction.data.s.imm >> 6 << 11 | (uint32_t) (instruction.data.s.imm2 & 0x01) | (uint32_t) (instruction.data.s.imm & 0b0111111) << 5 | (uint32_t) instruction.data.s.imm2 >> 1 << 1;
    uint16_t sized_range = (uint16_t) (raw_range & 0b111111111110);
    int16_t extended_range = (int16_t) (sized_range | ((sized_range & (11 << 1)) ? 0b1111 << 12 : 0));


    if (current_stage_x_register->rs1_value >= current_stage_x_register->rs2_value) {
        *pc = *pc - 4 + extended_range;
    }}

void riscv_branch_dispatch(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    switch(instruction.data.i.funct3) {
        case 0b000:
            riscv_beq(pc, instruction);
            break;
        case 0b001:
            riscv_bne(pc, instruction);
            break;
        case 0b100:
            riscv_blt(pc, instruction);
            break;
        case 0b101:
            riscv_bge(pc, instruction);
            break;
        case 0b110:
            riscv_bltu(pc, instruction);
            break;
        case 0b111:
            riscv_bgeu(pc, instruction);
            break;
        default:
            riscv_illegal_instruction(pc, instruction, NULL);
    }
}

void riscv_sb(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    // stores the lower 8 bits of the value of rs2 into memory
    uint64_t immediate = instruction.data.s.imm2 | instruction.data.s.imm << 5;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->signExtend = 0;
    new_m_reg->value = (uint8_t) current_stage_x_register->rs2_value;
    new_m_reg->size = 1;
    new_m_reg->readWrite = 1;
}

void riscv_sh(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    // stores the lower 16 bits of the value of rs2 into memory
    uint64_t immediate = instruction.data.s.imm2 | instruction.data.s.imm << 5;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->signExtend = 0;
    new_m_reg->value = (uint16_t) current_stage_x_register->rs2_value;
    new_m_reg->size = 2;
    new_m_reg->readWrite = 1;
}

void riscv_sw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    // stores the lower 32 bits of the value of rs2 into memory
    uint64_t immediate = instruction.data.s.imm2 | instruction.data.s.imm << 5;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->signExtend = 0;
    new_m_reg->value = (uint32_t) current_stage_x_register->rs2_value;
    new_m_reg->size = 4;
    new_m_reg->readWrite = 1;
}

void riscv_sd(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    // stores the value of rs2 into memory
    uint64_t immediate = instruction.data.s.imm2 | instruction.data.s.imm << 5;
    if (immediate & (1 << 11)) {
        immediate |= 0b1111 << 12;
        immediate |= 0xFFFFFFFFFFFF0000;
    }
    int64_t offset = immediate;

    new_m_reg->address = current_stage_x_register->rs1_value + offset;
    new_m_reg->signExtend = 0;
    new_m_reg->value = current_stage_x_register->rs2_value;
    new_m_reg->size = 8;
    new_m_reg->readWrite = 1;
}

void riscv_store_dispatch(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    switch(instruction.data.s.funct3) {
        case 0b000:
            riscv_sb(pc, instruction, new_m_reg);
            break;
        case 0b001:
            riscv_sh(pc, instruction, new_m_reg);
            break;
        case 0b010:
            riscv_sw(pc, instruction, new_m_reg);
            break;
        case 0b011:
            riscv_sd(pc, instruction, new_m_reg);
            break;
        default:
            riscv_illegal_instruction(pc, instruction, NULL);
    }
}

void riscv_addiw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    //add the value of current_stage_x_register->rs1_value and immediate and save into rd
    uint64_t temp = instruction.data.i.imm;
    if (temp & (0b1 << 11)) {
        temp |= 0b1111 << 12;
        temp = (int32_t) (int16_t) temp;
    }

    prepare_register_write(instruction.data.i.rd, (int32_t) current_stage_x_register->rs1_value + (int32_t) temp, new_m_reg);
}

void riscv_addi(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;
    if (temp & (0b1 << 11)) {
        temp |= 0b1111 << 12;
        temp = (int64_t) (int16_t) temp;
    }

    prepare_register_write(instruction.data.i.rd, (int64_t) current_stage_x_register->rs1_value + (int64_t) temp, new_m_reg);
}

void riscv_slliw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;

    prepare_register_write(instruction.data.i.rd, (uint32_t) current_stage_x_register->rs1_value << (uint32_t) temp, new_m_reg);

}

void riscv_slli(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;

    prepare_register_write(instruction.data.i.rd, current_stage_x_register->rs1_value << temp, new_m_reg);
}

void riscv_slti(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;
    if (temp & (0b1 << 11)) {
        temp |= 0b1111 << 12;
        temp = (int64_t) (int16_t) temp;
    }

    if ((int64_t) current_stage_x_register->rs1_value < (int64_t) temp) {
        prepare_register_write(instruction.data.i.rd, 1, new_m_reg);
    } else {
        prepare_register_write(instruction.data.i.rd, 0, new_m_reg);
    }
}

void riscv_sltiu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;

    if ((uint64_t) current_stage_x_register->rs1_value < (uint64_t) temp) {
        prepare_register_write(instruction.data.i.rd, 1, new_m_reg);
    } else {
        prepare_register_write(instruction.data.i.rd, 0, new_m_reg);
    }
}

void riscv_xori(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;
    if (temp & (0b1 << 11)) {
        temp |= 0b1111 << 12;
        temp = (int64_t) (int16_t) temp;
    }

    prepare_register_write(instruction.data.i.rd, current_stage_x_register->rs1_value ^ temp, new_m_reg);
}

void riscv_srliw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;

    prepare_register_write(instruction.data.i.rd, (uint32_t)current_stage_x_register->rs1_value >> (uint32_t) temp, new_m_reg);

}

void riscv_srli(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;

    prepare_register_write(instruction.data.i.rd, current_stage_x_register->rs1_value >> temp, new_m_reg);
}

void riscv_sraiw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;

    prepare_register_write(instruction.data.r.rd, (uint32_t)(((int32_t) current_stage_x_register->rs1_value) >> ((int32_t) temp)), new_m_reg);
}

void riscv_srai(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;

    prepare_register_write(instruction.data.r.rd, (uint64_t)(((int64_t) current_stage_x_register->rs1_value) >> ((int64_t) temp)), new_m_reg);
}

void riscv_ori(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;
    if (temp & (0b1 << 11)) {
        temp |= 0b1111 << 12;
        temp = (int64_t) (int16_t) temp;
    }

    prepare_register_write(instruction.data.i.rd, current_stage_x_register->rs1_value | temp, new_m_reg);
}

void riscv_andi(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t temp = instruction.data.i.imm;
    if (temp & (0b1 << 11)) {
        temp |= 0b1111 << 12;
        temp = (int64_t) (int16_t) temp;
    }

    prepare_register_write(instruction.data.i.rd, current_stage_x_register->rs1_value & temp, new_m_reg);
}

void riscv_arithmetic1_64_dispatch(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    switch(instruction.data.i.funct3) {
        case 0b000:
            riscv_addiw(pc, instruction, new_m_reg);
            break;
        case 0b001:
            if (instruction.data.r.funct7 != 0) {
                riscv_illegal_instruction(pc, instruction, NULL);
                break;
            }
            riscv_slliw(pc, instruction, new_m_reg);
            break;
        case 0b101:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_srliw(pc, instruction, new_m_reg);
                    break;
                case 0b0100000:
                    riscv_sraiw(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        default:
            riscv_illegal_instruction(pc, instruction, NULL);
    }
}

void riscv_arithmetic1_dispatch(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    switch(instruction.data.i.funct3) {
        case 0b000:
            riscv_addi(pc, instruction, new_m_reg);
            break;
        case 0b001:
            if (instruction.data.r.funct7 != 0) {
                riscv_illegal_instruction(pc, instruction, NULL);
                break;
            }
            riscv_slli(pc, instruction, new_m_reg);
            break;
        case 0b010:
            riscv_slti(pc, instruction, new_m_reg);
            break;
        case 0b011:
            riscv_sltiu(pc, instruction, new_m_reg);
            break;
        case 0b100:
            riscv_xori(pc, instruction, new_m_reg);
            break;
        case 0b101:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_srli(pc, instruction, new_m_reg);
                    break;
                case 0b0100000:
                    riscv_srai(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b110:
            riscv_ori(pc, instruction, new_m_reg);
            break;
        case 0b111:
            riscv_andi(pc, instruction, new_m_reg);
            break;
        default:
            riscv_illegal_instruction(pc, instruction, NULL);
    }
}

void riscv_addw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (int32_t) current_stage_x_register->rs1_value + (int32_t) current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_add(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (int32_t) current_stage_x_register->rs1_value + (int32_t) current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_subw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (uint64_t) ((int32_t) current_stage_x_register->rs1_value - (int32_t) current_stage_x_register->rs2_value), new_m_reg);
}

void riscv_sub(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (uint64_t) ((int64_t) current_stage_x_register->rs1_value - (int64_t) current_stage_x_register->rs2_value), new_m_reg);
}

void riscv_mulw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t reg1 = current_stage_x_register->rs1_value & 0xFFFFFFFF;
    uint64_t reg2 = current_stage_x_register->rs2_value & 0xFFFFFFFF;
    int64_t result = (int64_t) reg1 * (int64_t) reg2;
    result &= 0xFFFFFFFF;
    prepare_register_write(instruction.data.r.rd, (uint64_t) (int64_t) (int32_t) result, new_m_reg); // remove upper 32 bits, sign extend, and unsign
}

void riscv_mul(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (uint64_t) ((int64_t) current_stage_x_register->rs1_value * (int64_t) current_stage_x_register->rs2_value), new_m_reg);
}

void riscv_sllw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (uint32_t) current_stage_x_register->rs1_value << (uint32_t) current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_sll(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, current_stage_x_register->rs1_value << current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_mulh(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t product = (uint64_t) ((int64_t) current_stage_x_register->rs1_value * (int64_t) current_stage_x_register->rs2_value);
    product >>= 32;
    prepare_register_write(instruction.data.r.rd, product, new_m_reg);
}

void riscv_slt(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if ((int64_t) current_stage_x_register->rs1_value < (int64_t) current_stage_x_register->rs2_value) {
        prepare_register_write(instruction.data.r.rd, 1, new_m_reg);
    } else {
        prepare_register_write(instruction.data.r.rd, 0, new_m_reg);
    }
}

void riscv_mulhsu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t product = (uint64_t) ((int64_t) current_stage_x_register->rs1_value * (uint64_t) current_stage_x_register->rs2_value);
    product >>= 32;
    prepare_register_write(instruction.data.r.rd, product, new_m_reg);
}

void riscv_sltu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs1_value < current_stage_x_register->rs2_value) {
        prepare_register_write(instruction.data.r.rd, 1, new_m_reg);
    } else {
        prepare_register_write(instruction.data.r.rd, 0, new_m_reg);
    }
}

void riscv_mulhu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    uint64_t product = (uint64_t) ((uint64_t) current_stage_x_register->rs1_value * (uint64_t) current_stage_x_register->rs2_value);
    product >>= 32;
    prepare_register_write(instruction.data.r.rd, product, new_m_reg);
}

void riscv_xor(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (uint32_t) current_stage_x_register->rs1_value ^ (uint32_t) current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_divw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs2_value == 0 || (current_stage_x_register->rs2_value == -1 && current_stage_x_register->rs1_value == LONG_MIN)) {
        prepare_register_write(instruction.data.r.rd, (uint64_t) -1, new_m_reg); // intentional overflow
    } else {
        uint64_t reg1 = current_stage_x_register->rs1_value & 0xFFFFFFFF;
        uint64_t reg2 = current_stage_x_register->rs2_value & 0xFFFFFFFF;
        int32_t result = (int32_t) reg1 / (int32_t) reg2;
        result &= 0xFFFFFFFF;
        prepare_register_write(instruction.data.r.rd, (uint64_t) (int64_t) result, new_m_reg); // sign extend, and unsign
    }
}

void riscv_div(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs2_value == 0 || (current_stage_x_register->rs2_value == -1 && current_stage_x_register->rs1_value == LLONG_MIN)) {
        prepare_register_write(instruction.data.r.rd, (uint64_t) -1, new_m_reg); // intentional overflow
    } else {
        prepare_register_write(instruction.data.r.rd, (uint64_t)((int64_t) current_stage_x_register->rs1_value / (int64_t) current_stage_x_register->rs2_value), new_m_reg);
    }
}

void riscv_srlw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, current_stage_x_register->rs1_value >> current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_srl(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (uint32_t) current_stage_x_register->rs1_value >> (uint32_t) current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_sraw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (uint32_t)(((int32_t) current_stage_x_register->rs1_value) >> ((int32_t) current_stage_x_register->rs2_value)), new_m_reg);
}

void riscv_sra(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, (uint64_t)(((int64_t) current_stage_x_register->rs1_value) >> ((int64_t) current_stage_x_register->rs2_value)), new_m_reg);
}

void riscv_divuw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs2_value == 0) {
        prepare_register_write(instruction.data.r.rd, (uint64_t) -1, new_m_reg); // intentional overflow
    } else {
        uint64_t reg1 = current_stage_x_register->rs1_value & 0xFFFFFFFF;
        uint64_t reg2 = current_stage_x_register->rs2_value & 0xFFFFFFFF;
        uint32_t result = (uint32_t) reg1 / (uint32_t) reg2;
        prepare_register_write(instruction.data.r.rd, (uint64_t) (int64_t) result, new_m_reg); // sign extend
    }
}

void riscv_divu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs2_value == 0) {
        prepare_register_write(instruction.data.r.rd, (uint64_t) -1, new_m_reg); // intentional overflow
    } else {
        prepare_register_write(instruction.data.r.rd, (uint64_t)(current_stage_x_register->rs1_value / current_stage_x_register->rs2_value), new_m_reg);
    }
}

void riscv_or(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, current_stage_x_register->rs1_value | current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_remw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs2_value == 0 || (current_stage_x_register->rs2_value == -1 && current_stage_x_register->rs1_value == LONG_MIN)) {
        prepare_register_write(instruction.data.r.rd, (uint64_t) -1, new_m_reg); // intentional overflow
    } else {
        uint64_t reg1 = current_stage_x_register->rs1_value & 0xFFFFFFFF;
        uint64_t reg2 = current_stage_x_register->rs2_value & 0xFFFFFFFF;
        int32_t result = (int32_t) reg1 % (int32_t) reg2;
        result &= 0xFFFFFFFF;
        prepare_register_write(instruction.data.r.rd, (uint64_t) (int64_t) result, new_m_reg); // sign extend, and unsign
    }
}

void riscv_rem(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs2_value == 0 || (current_stage_x_register->rs2_value == -1 && current_stage_x_register->rs1_value == LLONG_MIN)) {
        prepare_register_write(instruction.data.r.rd, (uint64_t) -1, new_m_reg); // intentional overflow
    } else {
        prepare_register_write(instruction.data.r.rd, (uint64_t)((int64_t) current_stage_x_register->rs1_value % (int64_t) current_stage_x_register->rs2_value), new_m_reg);
    }
}

void riscv_and(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    prepare_register_write(instruction.data.r.rd, current_stage_x_register->rs1_value & current_stage_x_register->rs2_value, new_m_reg);
}

void riscv_remuw(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs2_value == 0) {
        prepare_register_write(instruction.data.r.rd, (uint64_t) -1, new_m_reg); // intentional overflow
    } else {
        uint64_t reg1 = current_stage_x_register->rs1_value & 0xFFFFFFFF;
        uint64_t reg2 = current_stage_x_register->rs2_value & 0xFFFFFFFF;
        uint32_t result = (uint32_t) reg1 % (uint32_t) reg2;
        prepare_register_write(instruction.data.r.rd, (uint64_t) (int64_t) result, new_m_reg); // sign extend
    }
}

void riscv_remu(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    if (current_stage_x_register->rs2_value == 0) {
        prepare_register_write(instruction.data.r.rd, (uint64_t) -1, new_m_reg); // intentional overflow
    } else {
        prepare_register_write(instruction.data.r.rd, (uint64_t)(current_stage_x_register->rs1_value % current_stage_x_register->rs2_value), new_m_reg);
    }
}

void riscv_arithmetic2_dispatch(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    switch(instruction.data.r.funct3) {
        case 0b000:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_add(pc, instruction, new_m_reg);
                    break;
                case 0b0100000:
                    riscv_sub(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_mul(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b001:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_sll(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_mulh(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b010:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_slt(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_mulhsu(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b011:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_sltu(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_mulhu(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b100:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_xor(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_div(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b101:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_srl(pc, instruction, new_m_reg);
                    break;
                case 0b0100000:
                    riscv_sra(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_divu(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b110:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_or(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_rem(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b111:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_and(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_remu(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        default:
            riscv_illegal_instruction(pc, instruction, NULL);
    }
}


void riscv_arithmetic2_64_dispatch(uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg) {
    switch(instruction.data.r.funct3) {
        case 0b000:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_addw(pc, instruction, new_m_reg);
                    break;
                case 0b0100000:
                    riscv_subw(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_mulw(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b001:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_sllw(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b100:
            switch(instruction.data.r.funct7) {
                case 0b0000001:
                    riscv_divw(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b101:
            switch(instruction.data.r.funct7) {
                case 0b0000000:
                    riscv_srlw(pc, instruction, new_m_reg);
                    break;
                case 0b0100000:
                    riscv_sraw(pc, instruction, new_m_reg);
                    break;
                case 0b0000001:
                    riscv_divuw(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b110:
            switch(instruction.data.r.funct7) {
                case 0b0000001:
                    riscv_remw(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        case 0b111:
            switch(instruction.data.r.funct7) {
                case 0b0000001:
                    riscv_remuw(pc, instruction, new_m_reg);
                    break;
                default:
                    riscv_illegal_instruction(pc, instruction, NULL);
            }
            break;
        default:
            riscv_illegal_instruction(pc, instruction, NULL);
    }
}

void (*major_dispatch_table[128]) (uint64_t* pc, struct riscv_instruction instruction, struct stage_reg_m* new_m_reg);

// we cannot edit main method....
int has_initialised = 0;

struct cache_table instruction_cache;
struct cache_table data_cache;

void initialise() {
    if (has_initialised) {
        return;
    }
    has_initialised = 1;
    construct_cache(&instruction_cache, 512, CACHE_INSTRUCTION);
    construct_cache(&data_cache, 2048, CACHE_DATA);
    for (int i = 0; i < 128; i++) {
        major_dispatch_table[i] = riscv_illegal_instruction;
    }
    // all U/UJ decoder types here (they have no subtables)
    major_dispatch_table[0b0010111] = riscv_auipc;
    major_dispatch_table[0b0110111] = riscv_lui;
    major_dispatch_table[0b1101111] = riscv_jal;
    // JALR singular
    major_dispatch_table[0b1100111] = riscv_jalr;
    // funct3 decoding tables
    major_dispatch_table[0b0000011] = riscv_load_dispatch;
    major_dispatch_table[0b1100011] = riscv_branch_dispatch;
    major_dispatch_table[0b0100011] = riscv_store_dispatch;
    major_dispatch_table[0b0010011] = riscv_arithmetic1_dispatch;
    major_dispatch_table[0b0011011] = riscv_arithmetic1_64_dispatch;
    major_dispatch_table[0b0110011] = riscv_arithmetic2_dispatch;
    major_dispatch_table[0b0111011] = riscv_arithmetic2_64_dispatch;
    major_dispatch_table[0b0001111] = riscv_nop; // fence instructions
    major_dispatch_table[0b1110011] = riscv_nop; // CSR/ECALL/EBREAK
}

struct tlb itlb;
struct tlb dtlb;

// API

void stage_fetch (struct stage_reg_d* new_d_reg) {
    initialise();
    uint64_t pc = get_pc();
    uint32_t physical_pc = 0;
    uint8_t status = get_address(&itlb, (uint32_t) pc, &physical_pc, current_stage_d_register->will_be_stalled == 2 ? current_stage_d_register->tlb_stall_status : (uint8_t) 0xFF);
    uint8_t memory_read_available = 0;
    if (status == 0xFE) {
        memory_read_available = 1;
    } else if (status != 0xFF) {
        new_d_reg->will_be_stalled = 2;
        new_d_reg->tlb_stall_status = status;
        return;
    }
    uint8_t read_status;
    if (read_status = read_access(&instruction_cache, physical_pc, 4, (void*) &new_d_reg->instruction, current_stage_d_register->will_be_stalled == 1, memory_read_available)) {
        // stall
        memset(new_d_reg, 0, sizeof(struct stage_reg_d));
        new_d_reg->tlb_stall_status = 0xFF;
        new_d_reg->will_be_stalled = (uint8_t) (read_status == 2 ? 3 : 1);
        return;
    }
    new_d_reg->pc = pc;
    pc = predict_address(pc);
    new_d_reg->new_pc = pc;
    set_pc(pc);
    new_d_reg->not_stalled = 1;
    new_d_reg->will_be_stalled = 0;
}

void stage_decode (struct stage_reg_x* new_x_reg) {
    if (!current_stage_d_register->not_stalled || current_stage_w_register->global_memory_stall) {
        new_x_reg->not_stalled = 0;
        return;
    }
    initialise();
    new_x_reg->pc = current_stage_d_register->pc;
    new_x_reg->new_pc = current_stage_d_register->new_pc;
    new_x_reg->instruction = *(struct riscv_instruction*) &current_stage_d_register->instruction;
    new_x_reg->not_stalled = 1;
    uint8_t opcode = new_x_reg->instruction.data.i.opcode;
    uint8_t decoding_type = 0; // 0 = I, 1 = U, 2 = S, 3 = R
    switch (opcode) {
        case 0b0010111:
        case 0b0110111:
        case 0b1101111:
            decoding_type = 1;
            break;
        case 0b0100011:
        case 0b1100011:
            decoding_type = 2;
            break;
        case 0b0110011:
        case 0b0111011:
            decoding_type = 3;
    }
    int16_t rs1 = -1;
    int16_t rs2 = -1;
    int16_t rd = -1;
    if (decoding_type == 0) {
        rs1 = new_x_reg->instruction.data.i.rs1;
        rd = new_x_reg->instruction.data.i.rd;
    } else if (decoding_type == 1) {
        rd = new_x_reg->instruction.data.u.rd;
    } else if (decoding_type == 2) {
        rs1 = new_x_reg->instruction.data.s.rs1;
        rs2 = new_x_reg->instruction.data.s.rs2;
    } else if (decoding_type == 3) {
        rs1 = new_x_reg->instruction.data.r.rs1;
        rs2 = new_x_reg->instruction.data.r.rs2;
        rd = new_x_reg->instruction.data.r.rd;
    }
    if (current_stage_m_register->readWrite == 2 && (current_stage_m_register->reg == rs1 || current_stage_m_register->reg == rs2 || current_stage_m_register->reg == rd)) {
        set_pc(current_stage_d_register->pc);
        new_x_reg->rs2_value = (uint64_t) -1;
        new_x_reg->rs1_value = (uint64_t) -1;
        new_x_reg->rs1 = -1;
        new_x_reg->rs2 = -1;
        new_x_reg->rd = -1;
        new_x_reg->not_stalled = 0;
        return;
    }
    new_x_reg->rs1 = rs1;
    new_x_reg->rs2 = rs2;
    new_x_reg->rd = rd;
    int register_stall = 0;
    if (rs1 == -1 && rs2 == -1) {
        new_x_reg->rs2_value = (uint64_t) -1;
        new_x_reg->rs1_value = (uint64_t) -1;
    } else if (rs1 != -1 && rs2 == -1) {
        register_stall = forwarded_register_read(new_x_reg->instruction.data.r.rs1, 0, &new_x_reg->rs1_value, &new_x_reg->rs2_value);
        new_x_reg->rs2_value = (uint64_t) -1;
    } else {
        register_stall = forwarded_register_read(new_x_reg->instruction.data.r.rs1, new_x_reg->instruction.data.r.rs2, &new_x_reg->rs1_value, &new_x_reg->rs2_value);
    }
    if (register_stall) {
        set_pc(current_stage_d_register->pc);
        new_x_reg->rs2_value = (uint64_t) -1;
        new_x_reg->rs1_value = (uint64_t) -1;
        new_x_reg->rs1 = -1;
        new_x_reg->rs2 = -1;
        new_x_reg->rd = -1;
        new_x_reg->not_stalled = 0;
    }

}


void stage_execute (struct stage_reg_m* new_m_reg) {
    if (current_stage_w_register->global_memory_stall) {
        memcpy(new_m_reg, &current_stage_w_register->memory_register, sizeof(struct stage_reg_m));
        if (current_stage_m_register->tainted_executions > 0) {
            new_m_reg->tainted_executions = (uint8_t) (current_stage_m_register->tainted_executions - 1);
            //return;
        }
        set_pc(new_m_reg->pc);
        new_m_reg->tainted_executions = 1; // flush decode
        return;
    }
    new_m_reg->wasStalled = 0;
    new_m_reg->tainted_executions = 0;
    new_m_reg->readWrite = 0;
    new_m_reg->pc = current_stage_x_register->pc;
    if (!current_stage_x_register->not_stalled) {
        return;
    }
    if (current_stage_m_register->tainted_executions > 0) {
        new_m_reg->tainted_executions = (uint8_t) (current_stage_m_register->tainted_executions - 1);
        return;
    }
    initialise();
    if (*(uint32_t*) &current_stage_x_register->instruction == 0) {
        return; // artificially make 0x00000000 a nop for over-execution
    }
    // printf("%08X\n", current_stage_x_register->pc);
    uint64_t pc = current_stage_x_register->pc + 4;
    major_dispatch_table[current_stage_x_register->instruction.data.u.opcode](&pc, current_stage_x_register->instruction, new_m_reg);
    if (pc != current_stage_x_register->new_pc) { // mispredict
        set_pc(pc);
        new_m_reg->tainted_executions = 1;
    }
    if (pc != current_stage_x_register->pc + 4) {
        update_entry(current_stage_x_register->pc, pc, pc == current_stage_x_register->pc + 4);
    }
}

void stage_memory (struct stage_reg_w *new_w_reg) {
    initialise();
    // printf("mem %08X\n", current_stage_m_register->address);
    if (current_stage_w_register->tainted_executions > 0) {
        new_w_reg->value = 0;
        new_w_reg->reg = 0;
        new_w_reg->op = 0;
        new_w_reg->global_memory_stall = 0;
        new_w_reg->tainted_executions = (uint8_t) (current_stage_w_register->tainted_executions - 1);
        return;
    }
    new_w_reg->tainted_executions = 0;
    if (current_stage_m_register->readWrite == 3) { // register write data forward
        new_w_reg->reg = current_stage_m_register->reg;
        new_w_reg->value = current_stage_m_register->value;
        new_w_reg->op = 1;
        new_w_reg->global_memory_stall = 0;
        return;
    } else if (current_stage_m_register->readWrite == 2) { // memory read, write to register
        uint32_t physical_address = 0;
        uint8_t status = get_address(&dtlb, (uint32_t) current_stage_m_register->address, &physical_address, current_stage_m_register->wasStalled ? current_stage_m_register->stallStatus : (uint8_t) 0xFF);

        uint8_t memory_read_available = 0;
        if (status == 0xFE) {
            memory_read_available = 1;
        } else if (status != 0xFF) {
            new_w_reg->global_memory_stall = 2;
            new_w_reg->tlb_stall_status = status;
            new_w_reg->value = 0;
            new_w_reg->reg = 0;
            new_w_reg->op = 0;
            new_w_reg->tainted_executions = 1;
            memcpy(&new_w_reg->memory_register, current_stage_m_register, sizeof(struct stage_reg_m));
            new_w_reg->memory_register.stallStatus = status;
            new_w_reg->memory_register.wasStalled = 1;
            return;
        }

        uint8_t missed = read_access(&data_cache, physical_address, current_stage_m_register->size, &new_w_reg->value, current_stage_m_register->wasStalled == 1, memory_read_available);
        if (missed) {
            new_w_reg->global_memory_stall = (uint8_t) (missed == 2 ? 4 : 1);
            new_w_reg->value = 0;
            new_w_reg->reg = 0;
            new_w_reg->op = 0;
            new_w_reg->tainted_executions = 1;
            memcpy(&new_w_reg->memory_register, current_stage_m_register, sizeof(struct stage_reg_m));
            new_w_reg->memory_register.wasStalled = missed == 2 ? 2 : 1;
            new_w_reg->memory_register.stallStatus = 0xFF;
            return;
        }
        if (current_stage_m_register->signExtend && (new_w_reg->value & (0b1 << (current_stage_m_register->size * 8 - 1)))) {
            for (int i = current_stage_m_register->size; i < 8; i++) {
                new_w_reg->value = new_w_reg->value | ((uint64_t) 0xFF << (uint64_t) (8 * i));
            }
        }
        new_w_reg->reg = current_stage_m_register->reg;
        new_w_reg->op = 1;
        new_w_reg->global_memory_stall = 0;
        return;
    } else if (current_stage_m_register->readWrite == 1) { // memory write value
        uint32_t physical_address = 0;
        uint8_t status = get_address(&dtlb, (uint32_t) current_stage_m_register->address, &physical_address, current_stage_m_register->wasStalled ? current_stage_m_register->stallStatus : (uint8_t) 0xFF);

        uint8_t memory_read_available = 0;
        if (status == 0xFE) {
            memory_read_available = 1;
        } else if (status != 0xFF) {
            new_w_reg->global_memory_stall = 3;
            new_w_reg->tlb_stall_status = status;
            new_w_reg->value = 0;
            new_w_reg->reg = 0;
            new_w_reg->op = 0;
            new_w_reg->tainted_executions = 1;
            memcpy(&new_w_reg->memory_register, current_stage_m_register, sizeof(struct stage_reg_m));
            new_w_reg->memory_register.stallStatus = status;
            new_w_reg->memory_register.wasStalled = 1;
            return;
        }

        uint8_t missed = write_access(&data_cache, physical_address, current_stage_m_register->value, current_stage_m_register->size, current_stage_m_register->wasStalled == 1, memory_read_available);
        if (missed) {
            new_w_reg->global_memory_stall = (uint8_t) (missed == 2 ? 5 : 1);
            new_w_reg->value = 0;
            new_w_reg->reg = 0;
            new_w_reg->op = 0;
            new_w_reg->tainted_executions = 1;
            new_w_reg->tainted_executions = 0;
            memcpy(&new_w_reg->memory_register, current_stage_m_register, sizeof(struct stage_reg_m));
            new_w_reg->memory_register.wasStalled = missed == 2 ? 2 : 1;
            new_w_reg->memory_register.stallStatus = 0xFF;
            return;
        }
    } // else nop
    new_w_reg->value = 0;
    new_w_reg->reg = 0;
    new_w_reg->op = 0;
    new_w_reg->global_memory_stall = 0;
}

void stage_writeback () {
    if (!current_stage_w_register->op) {
        return;
    }
    initialise();
    register_write(current_stage_w_register->reg, current_stage_w_register->value);
}

