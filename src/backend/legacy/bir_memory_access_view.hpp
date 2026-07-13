#pragma once

#include "bir_producer_view.hpp"
#include "../../shared/text_id_table.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <variant>

namespace c4c::backend::bir {

struct Block;
struct Value;
struct BinaryInst;
struct SelectInst;
struct CastInst;
struct PhiInst;
struct CallInst;
struct LoadLocalInst;
struct LoadGlobalInst;
struct StoreGlobalInst;
struct StoreLocalInst;
using Inst = std::variant<BinaryInst, SelectInst, CastInst, PhiInst, CallInst,
                          LoadLocalInst, LoadGlobalInst, StoreGlobalInst,
                          StoreLocalInst>;
enum class AddressSpace : unsigned char;

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
  const Inst* instruction = nullptr;
  std::size_t instruction_index = 0;
  std::string_view block_label;
  std::string_view base_name;
  AddressSpace address_space = static_cast<AddressSpace>(0);
  bool is_volatile = false;
  std::size_t align_bytes = 0;
  std::string_view local_slot_name;
  c4c::SlotNameId local_slot_id = c4c::kInvalidSlotName;
  std::string_view global_name;
  c4c::LinkNameId global_name_id = c4c::kInvalidLinkName;
  std::string_view string_constant_name;
  c4c::LinkNameId string_constant_name_id = c4c::kInvalidLinkName;
  const Value* pointer_base = nullptr;
  std::string_view pointer_base_name;
  const Value* result_value = nullptr;
  std::string_view result_value_name;
  const Value* stored_value = nullptr;
  std::string_view stored_value_name;
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
