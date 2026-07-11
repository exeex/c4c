#pragma once

#include "bir_producer_view.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

namespace c4c::backend::bir {

struct Block;
struct Value;

enum class BirMemoryAccessKind : unsigned char {
  Unknown,
  LoadLocal,
  LoadGlobal,
  StoreLocal,
  StoreGlobal,
};

enum class BirMemoryBaseKind : unsigned char {
  None,
  LocalSlot,
  GlobalSymbol,
  PointerValue,
  StringConstant,
};

struct BirMemoryAccessResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  BirMemoryAccessKind kind = BirMemoryAccessKind::Unknown;
  BirMemoryBaseKind base_kind = BirMemoryBaseKind::None;
  std::size_t instruction_index = 0;
  std::string_view block_label;
  std::string_view base_name;
  const Value* pointer_base = nullptr;
  const Value* result_value = nullptr;
  const Value* stored_value = nullptr;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available;
  }
};

class BirMemoryAccessView {
 public:
  BirMemoryAccessView() = default;
  [[nodiscard]] explicit operator bool() const { return implementation_ != nullptr; }

 private:
  struct Implementation;
  explicit BirMemoryAccessView(std::shared_ptr<const Implementation> implementation)
      : implementation_(std::move(implementation)) {}
  std::shared_ptr<const Implementation> implementation_;

  friend BirMemoryAccessView make_bir_memory_access_view(const Block& block);
  friend BirMemoryAccessResult find_memory_access(
      const BirMemoryAccessView&, std::size_t);
  friend BirMemoryAccessResult find_memory_access_source(
      const BirMemoryAccessView&, const Value&, std::size_t);
};

[[nodiscard]] BirMemoryAccessView make_bir_memory_access_view(const Block& block);
[[nodiscard]] BirMemoryAccessResult find_memory_access(
    const BirMemoryAccessView& view, std::size_t instruction_index);
[[nodiscard]] BirMemoryAccessResult find_memory_access_source(
    const BirMemoryAccessView& view,
    const Value& value,
    std::size_t before_instruction_index);

}  // namespace c4c::backend::bir
