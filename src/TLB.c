#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "riscv_sim_framework.h"
#include "mem.h"
#include "TLB.h"

// Notes on structure: the TLB is restricted to 8 entries - therefore addresses will be indexed on bits [4 : 2].

uint8_t update_tlb_entry(struct tlb* cache, uint32_t tlb_index, uint32_t virtual_page /* :20 */, uint32_t* ret_physical_page, uint8_t status) {
    uint32_t virtual_superpage = virtual_page >> 2;
    // split virtual address into upper and lower
    uint32_t first_level_index = (virtual_superpage >> 8 << 2) + get_ptbr();
    uint32_t second_level_index;
    bool memory_direct_status = false;

    if (status == 0 && !(memory_direct_status = memory_status(first_level_index, &second_level_index))) {
        return 0;
    } else if (status == 0xFF && !(memory_direct_status = memory_read(first_level_index, &second_level_index, 4))) {
        return 0;
    }
    if (memory_direct_status) {
        cache->cached_second_index = second_level_index;
        return 2; // only one read per stage
    } else {
        second_level_index = cache->cached_second_index;
    }


    if (second_level_index >> 31 != 1) {
        return 0x80;
    }

    second_level_index = second_level_index << 1 >> 1 << 12;

    uint32_t physical_page;

    second_level_index += (virtual_page & 0xFF) << 2;
    if (status != 1 && !memory_read(second_level_index, &physical_page, 4)) {
        return 1;
    } else if (status == 1 && !memory_status(second_level_index, &physical_page)) {
        return 1;
    }

    if (!(physical_page >> 31)) {
        return 0x80; // invalid entry
    }
    if (virtual_superpage >> 3 != tlb_index) {
        printf("superpage tlb_index mismatch!\n");
        exit(1);
    }
    cache->entries[tlb_index].virtual_page = (uint16_t) (virtual_superpage >> 3);
    cache->entries[tlb_index].physical_page = physical_page << 1 >> 3;
    cache->entries[tlb_index].valid = 1;
    *ret_physical_page = cache->entries[tlb_index].physical_page;
    return 0xFF;
}

uint8_t get_address(struct tlb* cache, uint32_t virtual_address, uint32_t* output, uint8_t status) {
    uint32_t index = (virtual_address >> 14) & 0b111;
    uint8_t new_status;
    uint32_t ret_physical_page;
    if (status == 0xFF) {
        if (virtual_address >> 17 == cache->entries[index].virtual_page && cache->entries[index].valid) { // found
            *output = (cache->entries[index].physical_page << 14) | (virtual_address << 18 >> 18);
            return 0xFE;
        } else { // not found
            if ((new_status = update_tlb_entry(cache, index, virtual_address >> 12, &ret_physical_page, 0xFF)) != 0xFF) {
                return new_status;
            }
        }
    } else {
        if ((new_status = update_tlb_entry(cache, index, virtual_address >> 12, &ret_physical_page, status)) != 0xFF) {
            return new_status;
        }
    }
    *output = (ret_physical_page << 14) | (virtual_address << 18 >> 18);
    return 0xFF;
}