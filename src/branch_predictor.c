#include "branch_predictor.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


struct branch_information_entry {
    uint64_t branch_address; // Address of the branch instruction
    uint64_t target_address; // Target destination of the address instruction
    uint8_t bht; // 2 bit branch history table
};

struct branch_information_entry branch_table[32];

void update_bht(uint8_t t_bht, uint8_t t_index) {
    if ( (t_bht) && (branch_table[t_index].bht < 3) ) {
        branch_table[t_index].bht++;
    }
    else if ( (!t_bht) && (branch_table[t_index].bht > 0) ) {
        branch_table[t_index].bht--;
    }
}

uint64_t predict_address(uint64_t branch_address) {
    uint8_t t_index = (uint8_t) branch_address & 31;
    if ( (branch_address == branch_table[t_index].branch_address) && (branch_table[t_index].bht > 1) ) {
        return branch_table[t_index].target_address;
    }
    return branch_address + 4;
}

uint64_t predict_address_BTFNT(uint64_t branch_address) {
    uint8_t t_index = (uint8_t) branch_address & 31;
    if (branch_table[t_index].branch_address == branch_address && branch_table[t_index].target_address > branch_address + 4) {
        return branch_table[t_index].target_address;
    }
    return branch_address + 4;
}

void update_entry(uint64_t new_address, uint64_t new_target, uint8_t t_bht) {
    uint8_t t_index = (uint8_t) new_target & 31;
    // Updates when entry is already present
    if (branch_table[t_index].branch_address == new_target) {
        update_bht(t_bht, t_index);
        return;
    }

    // Creates new entry
    branch_table[t_index].branch_address = new_address;
    branch_table[t_index].target_address = new_target;
    branch_table[t_index].bht = (new_address+4 > new_target)*3;
}