# ifndef BRANCH_PREDICTOR_H
# define BRANCH_PREDICTOR_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint64_t predict_address(uint64_t branch_address);
uint64_t predict_address_BTFNT(uint64_t branch_address);
void update_entry(uint64_t new_address, uint64_t new_target, uint8_t t_bht);

#endif