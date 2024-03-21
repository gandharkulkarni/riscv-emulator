#include <stdint.h>
#include <stdbool.h>
#include "bits.h"

// Get the specified bits from a number
uint32_t get_bits(uint64_t num, uint32_t start, uint32_t count) {
    uint64_t mask = (1ULL << count) - 1;
    return (num >> start) & mask;
}

// Sign extend a number from a specified bit position
int64_t sign_extend(uint64_t num, uint32_t start) {
    uint32_t distance = 64 - start;
    int64_t imm;
    imm = num << distance;
    imm = imm >> distance;
    return imm;
}

// Get a specific bit from a number
bool get_bit(uint64_t num, uint32_t bit) {
    return (num >> bit) & 1U;
}
