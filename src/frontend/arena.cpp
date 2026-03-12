#include "arena.hpp"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace tinyc2ll::frontend_cxx {

Arena::Arena(std::size_t block_size)
    : head_(nullptr), block_size_(block_size) {}

Arena::~Arena() {
  free_all();
}

void Arena::new_block(std::size_t min_size) {
  std::size_t cap = (min_size > block_size_) ? min_size : block_size_;
  Block* b = static_cast<Block*>(std::malloc(sizeof(Block) + cap));
  if (!b) throw std::bad_alloc();
  b->data = reinterpret_cast<char*>(b + 1);
  b->cap = cap;
  b->used = 0;
  b->next = head_;
  head_ = b;
}

void* Arena::alloc(std::size_t n) {
  if (n == 0) n = 1;
  std::size_t aligned = (n + 7u) & ~7u;

  if (!head_ || head_->used + aligned > head_->cap) {
    new_block(aligned);
  }

  void* ptr = head_->data + head_->used;
  head_->used += aligned;
  std::memset(ptr, 0, aligned);
  return ptr;
}

const char* Arena::strdup(const char* s) {
  if (!s) return nullptr;
  std::size_t len = std::strlen(s);
  char* dst = static_cast<char*>(alloc(len + 1));
  std::memcpy(dst, s, len);
  dst[len] = '\0';
  return dst;
}

const char* Arena::strdup(const std::string& s) {
  char* dst = static_cast<char*>(alloc(s.size() + 1));
  std::memcpy(dst, s.data(), s.size());
  dst[s.size()] = '\0';
  return dst;
}

void Arena::free_all() {
  Block* b = head_;
  while (b) {
    Block* next = b->next;
    std::free(b);
    b = next;
  }
  head_ = nullptr;
}

}  // namespace tinyc2ll::frontend_cxx
