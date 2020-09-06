
#ifndef RISCVSIM_RISCV_H
#define RISCVSIM_RISCV_H

#include <stdint.h>

struct __attribute__((packed)) riscv_r_instruction_data {
    uint8_t opcode:7;
    uint8_t rd:5;
    uint8_t funct3:3;
    uint8_t rs1:5;
    uint8_t rs2:5;
    uint8_t funct7:7;
};

struct __attribute__((packed)) riscv_i_instruction_data {
    uint8_t opcode:7;
    uint8_t rd:5;
    uint8_t funct3:3;
    uint8_t rs1:5;
    uint16_t imm:12;
};

// same as sb, but with different immediate layout.
struct __attribute__((packed)) riscv_s_instruction_data {
    uint8_t opcode:7;
    uint16_t imm2:5;
    uint8_t funct3:3;
    uint8_t rs1:5;
    uint8_t rs2:5;
    uint8_t imm:7;
};

// same as uj, but with different immediate layout.
struct __attribute__((packed)) riscv_u_instruction_data {
    uint8_t opcode:7;
    uint8_t rd:5;
    uint32_t imm:20;
};

struct __attribute__((packed)) riscv_instruction {
    union {
        struct riscv_r_instruction_data r;
        struct riscv_i_instruction_data i;
        struct riscv_s_instruction_data s;
        struct riscv_u_instruction_data u;
    } data;
};

#endif //RISCVSIM_RISCV_H
