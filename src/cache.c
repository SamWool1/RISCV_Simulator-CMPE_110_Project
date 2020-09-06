# include "cache.h"
# include <stdint.h>
# include <stdio.h>
# include <string.h>
# include <math.h>
# include "riscv_sim_framework.h"
#include "mem.h"

/*
 * Usage
 * read_access will return the index of the block with the data, OR 0xFFFF_FFFF_FFFF_FFFF if a stall is needed
 * write_access will return 0 if data written successfully, OR 1 if a stall is needed
 * Note: address lengths are 64 bits, offset is 2/3 bits, [1/2:0]
 */


// Constructor for the struct -  this stuff should probably just be done in riscv_virt in the end, useful as reference
// Can also just run this once on first(0th) cycle for I-cache, D-cache, and L2-cache
void construct_cache(struct cache_table* cache, size_t num_blocks, uint8_t cache_type) {
    cache->num_blocks = num_blocks;
    cache->index_length = (uint8_t) (log(num_blocks) / log(2));
    cache->cache_type = cache_type;
    cache->block_size = (uint8_t) (cache_type == CACHE_INSTRUCTION ? 4 : 3);
#ifdef TWOWAY
    cache->rows = scalloc(num_blocks * sizeof(union cache_row) * (cache->cache_type == CACHE_DATA ? 2 : 1));
#else
    cache->rows = scalloc(num_blocks * sizeof(union cache_row));
#endif
}

union cache_row* evict_read(struct cache_table* cache, uint64_t address, uint64_t index, uint64_t tag, uint8_t is_followup) {
    // Select index to be evicted (no choice if direct mapped)
#ifdef TWOWAY
    if (cache->cache_type == CACHE_DATA && cache->rows[index].d.valid &&
        (!cache->rows[index + cache->num_blocks].d.valid || (cache->rows[index].d.dirty && !cache->rows[index + cache->num_blocks].d.dirty))) {
        index += cache->num_blocks;
    }
#endif

    uint8_t needs_stall = 0;

    // Eviction for writeback
#ifdef WRITEBACK
    if (cache->cache_type == CACHE_DATA && cache->rows[index].d.dirty) {
        uint64_t old_address = (cache->rows[index].d.tag << (cache->index_length + cache->block_size) ) | (index << cache->block_size);
        if (is_followup && !memory_status(old_address, NULL)) {
            return NULL;
        } else if (!is_followup && !memory_write(old_address, *(uint64_t*) (uint32_t*) cache->rows[index].d.data, 8)) {
            needs_stall = 1; // old_address != address as then it would be a hit and evict_read is not called
        } else {
            cache->rows[index].d.dirty = 0;
        }
    }
#endif

    // Input new values
    if (cache->cache_type == CACHE_DATA) {
        if (is_followup && !memory_status(address, cache->rows[index].d.data)) {
            return NULL;
        } else if (!is_followup && (!memory_read(address, cache->rows[index].d.data, 8) || needs_stall)) {
            return NULL;
        }
        cache->rows[index].d.tag = tag;
        cache->rows[index].d.valid = 1;
#ifdef WRITEBACK
        cache->rows[index].d.dirty = 0;
#endif
    } else {
        if (is_followup && !memory_status(address, cache->rows[index].i.data)) {
            return NULL;
        } else if (!is_followup && (!memory_read(address, cache->rows[index].i.data, 16) || needs_stall)) {
            return NULL;
        }
        cache->rows[index].i.tag = tag;
        cache->rows[index].i.valid = 1;
    }
    return &cache->rows[index];
}

uint8_t read_access(struct cache_table* cache, uint64_t address, uint64_t size, uint64_t* value, uint8_t is_followup, uint8_t memory_read_available) {
    // extracts index
    uint64_t index = (address << ((64 - cache->block_size) - cache->index_length)) >> (64 - cache->index_length);
    // extracts tag
    uint64_t tag = address >> (cache->block_size + cache->index_length);
    uint64_t subindex = address << (64 - cache->block_size) >> (64 - cache->block_size);
    if ((subindex & 0b11) != 0) {
        printf("misaligned cache access\n");
        exit(1);
    }
    subindex >>= 2;
    uint64_t cache_hit = 0;
    uint8_t was_hit = 0;
    if (cache->cache_type == CACHE_INSTRUCTION) {
        if (cache->rows[index].i.valid && cache->rows[index].i.tag == tag && (size != 8 || subindex != 3)) {
            was_hit = 1;
            cache_hit = cache->rows[index].i.data[subindex];
            if (size == 8) {
                cache_hit = cache_hit | (uint64_t) cache->rows[index].i.data[subindex + 1] << 32;
            }
        }
    } else {
        if (cache->rows[index].d.valid && cache->rows[index].d.tag == tag) {
            was_hit = 1;
            cache_hit = cache->rows[index].d.data[subindex];
            if (size == 8) {
                cache_hit = cache_hit | (uint64_t) cache->rows[index].d.data[subindex + 1] << 32;
            }
        }
#ifdef TWOWAY
        else if (cache->rows[index + cache->num_blocks].d.valid && cache->rows[index + cache->num_blocks].d.tag == tag) {
            was_hit = 1;
            cache_hit = cache->rows[index + cache->num_blocks].d.data[subindex];
            if (size == 8) {
                cache_hit = cache_hit | (uint64_t) cache->rows[index + cache->num_blocks].d.data[subindex + 1] << 32;
            }
        }
#endif
    }
    if (!was_hit) {
        if (!memory_read_available) { // TLB took up our memory bandwidth, stall
            return 2;
        }
        union cache_row* row = evict_read(cache, address, index, tag, is_followup);
        if (row == NULL) { // stall
            return 1;
        }
        if (cache->cache_type == CACHE_INSTRUCTION) {
            cache_hit = row->i.data[subindex];
            if (size == 8) {
                cache_hit = cache_hit | (uint64_t) row->i.data[subindex + 1] << 32;
            }
        } else {
            cache_hit = row->d.data[subindex];
            if (size == 8) {
                cache_hit = cache_hit | (uint64_t) row->d.data[subindex + 1] << 32;
            }
        }
    }
    if (size == 1) {
        *((uint8_t*) value) = (uint8_t) cache_hit;
    } else if (size == 2) {
        *((uint16_t*) value) = (uint16_t) cache_hit;
    } else if (size == 4) {
        *((uint32_t*) value) = (uint32_t) cache_hit;
    } else if (size == 8) {
        *value = cache_hit;
    } else {
        printf("illegal cache access size %lu\n", size);
        exit(1);
    }
    return 0;
}

uint8_t evict_write(struct cache_table* cache, uint64_t address, uint64_t index, uint64_t tag, uint64_t data, uint64_t subindex, uint8_t size, uint8_t is_followup, uint8_t memory_read_available) {
    if (cache->cache_type != CACHE_DATA) { // we never write to the instruction cache
        return 2;
    }
#ifndef WRITEBACK
    if (is_followup) {
      return !memory_status(address, NULL);
    }
#endif

#ifdef TWOWAY
    if (cache->rows[index].d.valid &&
            (!cache->rows[index + cache->num_blocks].d.valid || (cache->rows[index].d.dirty && !cache->rows[index + cache->num_blocks].d.dirty))) {
        index += cache->num_blocks;
    }
#endif

    union cache_row *row = NULL;
    if (size != 8) {
        if (!memory_read_available) { // TLB took up our memory bandwidth, stall
            return 2;
        }
        row = evict_read(cache, address, index, tag, is_followup);
        if (row == NULL) { // stall for memory read
            return 1;
        }
    } else { // size == 8
        row = &cache->rows[index];

#ifdef WRITEBACK
        // Eviction for writeback (handled by evict_read in other cases)
        if (row->d.dirty) {
            if (!memory_read_available) { // TLB took up our memory bandwidth, stall
                return 2;
            }
            uint64_t old_address = (row->d.tag << (cache->index_length + cache->block_size) ) | (index << cache->block_size);
            if (is_followup && !memory_status(old_address, NULL)) {
                return 1;
            } else if (!is_followup && !memory_write(old_address, *(uint64_t*) (uint32_t*) row->d.data, 8)) {
                return 1;
            } else {
                row->d.dirty = 0;
            }
        }
#endif
    }

    // Input new values
    memcpy((uint8_t*) row->d.data + subindex, &data, size);
    row->d.tag = tag;
    row->d.valid = 1;
#ifdef WRITEBACK
    row->d.dirty = 1;
    return 0;
#else
    return (uint8_t) !memory_write(address, data, size);
#endif
}

uint8_t write_access(struct cache_table* cache, uint64_t address, uint64_t data, uint8_t size, uint8_t is_followup, uint8_t memory_read_available) {
    if (cache->cache_type != CACHE_DATA) { // we never write to the instruction cache
        return 1;
    }
    // extracts index
    uint64_t index = (address << ((64 - cache->block_size) - cache->index_length)) >> (64 - cache->index_length);
    // extracts tag
    uint64_t tag = address >> (cache->block_size + cache->index_length);
    uint64_t subindex = address << (64 - cache->block_size) >> (64 - cache->block_size);

    // Checks cache
    if (cache->rows[index].d.valid && cache->rows[index].d.tag == tag) {
        memcpy((uint8_t*) cache->rows[index].d.data + subindex, &data, size);
        return 0;
    }
#ifdef TWOWAY
    else if (cache->rows[index + cache->num_blocks].d.valid && cache->rows[index + cache->num_blocks].d.tag == tag) {
        memcpy((uint8_t*) cache->rows[index].d.data + subindex, &data, size);
        return 0;
    }
#endif

    return evict_write(cache, address, index, tag, data, subindex, size, is_followup, memory_read_available);
}