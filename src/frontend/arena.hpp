#pragma once

// Simple bump-pointer arena allocator.
//
// All nodes, strings, and child arrays produced during parsing are allocated
// from a single Arena, which is freed all at once after IR emission.
//
// Pure-C backport note: replace `class Arena` with `struct Arena` + free
// functions; remove the template method (use explicit casts).

#include <cstddef>
#include <string>

namespace c4c {

class Arena {
 public:
  // Default block size: 64 KiB per block.
  explicit Arena(std::size_t block_size = 65536);
  ~Arena();

  // Allocate n bytes, zero-initialised, 8-byte aligned.
  void* alloc(std::size_t n);

  // Typed allocation: allocate sizeof(T)*count bytes and return T*.
  // Pure-C backport: macro ARENA_ALLOC(arena, T, count)
  template <typename T>
  T* alloc_array(int count) {
    return static_cast<T*>(alloc(static_cast<std::size_t>(count) * sizeof(T)));
  }

  // Duplicate a null-terminated string into the arena.
  const char* strdup(const char* s);

  // Duplicate a std::string into the arena.
  const char* strdup(const std::string& s);

  // Release all blocks. After this call the arena is empty and can be reused.
  void free_all();

 private:
  struct Block {
    char* data;
    std::size_t cap;
    std::size_t used;
    Block* next;
  };

  void new_block(std::size_t min_size);

  Block* head_;
  std::size_t block_size_;
};

}  // namespace c4c
