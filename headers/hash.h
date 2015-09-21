#ifndef LS_HASH_H
#define LS_HASH_H
#include <stdint.h>
#include <stdlib.h>
#include <array>

#define MULTIPLIER 22695477

class __attribute__ ((__packed__)) sha224_t: public std::array<uint8_t, 28> {};

class __attribute__ ((__packed__)) rollh_t: public std::array<uint8_t, 4> {};

struct __attribute__ ((__packed__)) hash_t {
    rollh_t weak_hash;
    sha224_t strong_hash;
};

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

class Hasher;

class RollHash {
    uint32_t hash;
    size_t window_length;
    size_t window_pos;
    uint8_t* window; 
    size_t mult;
    friend Hasher;
public:
    RollHash(size_t window_length);
    void update(uint8_t* begin, uint8_t* end);
    rollh_t get();
    ~RollHash();
};

class Hasher {
    RollHash weak_hash;
public:
    Hasher(size_t window_length): weak_hash(window_length) {};
    void update(uint8_t* begin, uint8_t* end) {
        weak_hash.update(begin, end);
    }
    rollh_t get_weak_hash() {
        return weak_hash.get();
    }
    hash_t get_strong_hash() {
        SHA224 strong_hash;
        strong_hash.update(weak_hash.window+weak_hash.window_pos, weak_hash.window+weak_hash.window_length);
        strong_hash.update(weak_hash.window, weak_hash.window+weak_hash.window_pos);
        hash_t full_hash;
        full_hash.weak_hash = weak_hash.get();
        full_hash.strong_hash = strong_hash.get();
        return full_hash;
    }
};
#endif
