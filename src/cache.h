# ifndef CACHE_H
# define CACHE_H

# include <stdint.h>
# include <stdio.h>
# include <string.h>
# include <math.h>

#define CACHE_INSTRUCTION 0
#define CACHE_DATA 1

#define WRITEBACK
#define TWOWAY

struct cache_row_i {
    uint64_t tag:51;
    uint32_t data[4];
    uint8_t valid:1;
};

struct cache_row_d {
    uint64_t tag:50;
    uint32_t data[2];
#ifdef WRITEBACK
    uint8_t dirty:1;
#endif
    uint8_t valid:1;
};

union cache_row {
    struct cache_row_i i;
    struct cache_row_d d;
};

struct cache_table {
    size_t num_blocks;
    uint8_t index_length;
    uint8_t cache_type;
    uint8_t block_size;
    union cache_row* rows;
};

void construct_cache(struct cache_table* cache, size_t num_blocks, uint8_t cache_type);
uint8_t read_access(struct cache_table* cache, uint64_t address, uint64_t size, uint64_t* value, uint8_t is_followup, uint8_t memory_read_available);
uint8_t write_access(struct cache_table* cache, uint64_t address, uint64_t data, uint8_t size, uint8_t is_followup, uint8_t memory_read_available);

# endif