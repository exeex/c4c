#pragma once

#include <cstddef>
#include <cstdint>

#include "text_id_table.hpp"

namespace c4c {

constexpr uint64_t kIdHashSeed = 1469598103934665603ull;

[[nodiscard]] inline uint64_t pack_u32_pair(uint32_t lo, uint32_t hi) {
  return static_cast<uint64_t>(lo) | (static_cast<uint64_t>(hi) << 32);
}

[[nodiscard]] inline uint64_t pack_text_ids(TextId lo, TextId hi) {
  return pack_u32_pair(lo, hi);
}

[[nodiscard]] inline uint64_t mix_id_hash(uint64_t hash, uint64_t word) {
  return hash ^ (word + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2));
}

template <typename... Words>
[[nodiscard]] inline uint64_t hash_id_words(uint64_t seed, Words... words) {
  ((seed = mix_id_hash(seed, static_cast<uint64_t>(words))), ...);
  return seed;
}

[[nodiscard]] inline size_t hash_u32_sequence(const uint32_t* segments, size_t count) {
  uint64_t hash = kIdHashSeed;
  switch (count) {
    case 0:
      hash = mix_id_hash(hash, 0u);
      break;
    case 1:
      hash = hash_id_words(hash, segments[0], 1u);
      break;
    case 2:
      hash = hash_id_words(hash, pack_u32_pair(segments[0], segments[1]), 2u);
      break;
    case 3:
      hash = hash_id_words(hash, pack_u32_pair(segments[0], segments[1]), segments[2], 3u);
      break;
    case 4:
      hash = hash_id_words(hash, pack_u32_pair(segments[0], segments[1]),
                           pack_u32_pair(segments[2], segments[3]), 4u);
      break;
    case 5:
      hash = hash_id_words(hash, pack_u32_pair(segments[0], segments[1]),
                           pack_u32_pair(segments[2], segments[3]), segments[4], 5u);
      break;
    default:
      for (size_t i = 0; i < count; ++i) {
        hash = mix_id_hash(hash, segments[i]);
      }
      hash = mix_id_hash(hash, count);
      break;
  }
  return static_cast<size_t>(hash);
}

[[nodiscard]] inline size_t hash_text_id_sequence(const TextId* segments, size_t count) {
  return hash_u32_sequence(segments, count);
}

}  // namespace c4c
