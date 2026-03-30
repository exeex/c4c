// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/types.rs
// x86-64 linker constants and GlobalSymbol behavior.

#include "mod.hpp"

#include <algorithm>

namespace c4c::backend::x86::linker {

void DynStrTab::add(const std::string& value) {
  if (std::find(entries_.begin(), entries_.end(), value) == entries_.end()) {
    entries_.push_back(value);
    dirty_ = true;
  }
}

std::size_t DynStrTab::get_offset(const std::string& value) const {
  std::size_t offset = 0;
  for (const auto& entry : entries_) {
    if (entry == value) return offset;
    offset += entry.size() + 1;
  }
  return offset;
}

const std::vector<std::uint8_t>& DynStrTab::as_bytes() const {
  if (!dirty_) return cached_bytes_;
  cached_bytes_.clear();
  cached_bytes_.push_back(0);
  for (const auto& entry : entries_) {
    cached_bytes_.insert(cached_bytes_.end(), entry.begin(), entry.end());
    cached_bytes_.push_back(0);
  }
  dirty_ = false;
  return cached_bytes_;
}

bool x86_should_replace_extra(const GlobalSymbol& existing) {
  // For x86, a dynamic definition should be replaced by a static definition.
  return existing.is_dynamic;
}

}  // namespace c4c::backend::x86::linker

