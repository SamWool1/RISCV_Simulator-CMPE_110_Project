# ifndef TLB_H
# define TLB_H

# include <stdint.h>
# include <stdio.h>
# include <string.h>
# include <math.h>


struct tlb_entry {
    uint8_t valid:1;
    uint32_t physical_page:18;
    uint16_t virtual_page:15;
};

struct tlb {
    struct tlb_entry entries[8];
    uint32_t cached_second_index;
};

uint8_t get_address(struct tlb* cache, uint32_t virtual_address, uint32_t* output, uint8_t status);

#endif