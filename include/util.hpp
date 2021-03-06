#pragma once

namespace mrs {

struct search_result {
    uint64_t position;
    uint64_t sum;  // sum of values up to (excluding) position
};

template <typename SummaryType, typename KeyType>
void build_node_prefix_sums(KeyType const* input, uint8_t* out,
                            uint64_t segment_size, uint64_t num_segments,
                            uint64_t bytes) {
    std::fill(out, out + bytes, 0);
    SummaryType* summary = reinterpret_cast<SummaryType*>(out);
    uint64_t summary_bytes = num_segments * sizeof(SummaryType);
    KeyType* keys = reinterpret_cast<KeyType*>(out + summary_bytes);
    summary[0] = 0;
    for (uint64_t i = 0; i != num_segments; ++i) {
        keys[0] = input[0];
        for (uint64_t j = 1; j != segment_size; ++j) {
            keys[j] = keys[j - 1] + input[j];
        }
        if (i + 1 < num_segments) {
            summary[i + 1] = summary[i] + keys[segment_size - 1];
        }
        input += segment_size;
        keys += segment_size;
    }
}

// From http://xoroshiro.di.unimi.it/splitmix64.c
struct splitmix64 {
    splitmix64(uint64_t seed) : x(seed){};

    uint64_t next() {
        uint64_t z = (x += uint64_t(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30)) * uint64_t(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * uint64_t(0x94D049BB133111EB);
        return z ^ (z >> 31);
    }

private:
    uint64_t x;
};

uint64_t create_random_bits(std::vector<uint64_t>& bits, uint64_t threshold,
                            uint64_t seed = 13) {
    uint64_t num_ones = 0;
    splitmix64 hasher(seed);
    for (uint64_t i = 0; i < bits.size() * 64; i++) {
        if (hasher.next() < threshold) {
            bits[i / 64] |= 1ULL << (i % 64);
            num_ones++;
        }
    }

    // If we do not have at least one bit set in the first 256 bits,
    // we cannot create queries for Select256.
    // Therefore, we always set the first bit to 1.
    bool allzero = true;
    for (size_t i = 0; i < 4; i++) {
        if (bits[i] != 0) {
            allzero = false;
            break;
        }
    }
    if (allzero) {
        bits[0] = 1ULL;
        num_ones++;
    }

    return num_ones;
}

}  // namespace mrs