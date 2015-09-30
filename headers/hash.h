#ifndef CN_HASH_H
#define CN_HASH_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <array>
#include "common.h"
#include <boost/utility.hpp>

static const uint32_t MULTIPLIER = 22695477;

class SHA224 {
    uint32_t hash[8];
    uint8_t buff[64];
    size_t buff_used;
    size_t total_len;
    void process(uint8_t* begin, uint8_t* end);
public:
    SHA224();
    void update(uint8_t* begin, uint8_t* end);
    sha224_t get();
};

class Hasher {
    uint32_t hash;
    size_t window_length;
    size_t window_start;
    size_t window_end;
    uint8_t* window;
    size_t mult;
public:
    Hasher(size_t window_length);
    void update(const uint8_t* begin, const uint8_t* end);
    uint32_t get_weak_hash() const;
    hash_t get_strong_hash() const;
    Chunk get_chunk() const;
    ~Hasher();
};
#endif
