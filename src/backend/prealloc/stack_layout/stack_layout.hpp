#pragma once

#include "../frame.hpp"

#include "../../bir/bir.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare::stack_layout {

struct FunctionInlineAsmSummary {
  std::size_t instruction_count = 0;
  std::size_t structured_memory_address_operand_count = 0;
  bool has_conservative_side_effect_placement = false;
};

std::vector<PreparedStackObject> collect_function_stack_objects(PreparedNameTables& names,
                                                                const bir::NameTables& bir_names,
                                                                const bir::Function& function,
                                                                PreparedObjectId& next_object_id);

void apply_alloca_coalescing_hints(const PreparedNameTables& names,
                                   const bir::Function& function,
                                   std::vector<PreparedStackObject>& objects);

void apply_copy_coalescing_hints(const PreparedNameTables& names,
                                 const bir::Function& function,
                                 std::vector<PreparedStackObject>& objects);

void apply_aggregate_address_publication_hints(const PreparedNameTables& names,
                                               const bir::Function& function,
                                               std::vector<PreparedStackObject>& objects);

void apply_frame_address_publication_hints(PreparedNameTables& names,
                                           const bir::Function& function,
                                           std::vector<PreparedStackObject>& objects);

FunctionInlineAsmSummary summarize_inline_asm(const bir::Function& function);

void apply_regalloc_hints(PreparedNameTables& names,
                          const bir::Function& function,
                          const FunctionInlineAsmSummary& inline_asm_summary,
                          std::vector<PreparedStackObject>& objects);

std::vector<PreparedFrameSlot> assign_frame_slots(const PreparedNameTables& names,
                                                  const std::vector<PreparedStackObject>& objects,
                                                  PreparedFrameSlotId& next_slot_id,
                                                  std::size_t& frame_size_bytes,
                                                  std::size_t& frame_alignment_bytes);

[[nodiscard]] inline std::size_t fallback_type_size(c4c::backend::bir::TypeKind type) {
  using TypeKind = c4c::backend::bir::TypeKind;
  switch (type) {
    case TypeKind::Void:
      return 0;
    case TypeKind::I1:
    case TypeKind::I8:
      return 1;
    case TypeKind::I16:
      return 2;
    case TypeKind::I32:
    case TypeKind::F32:
      return 4;
    case TypeKind::I64:
    case TypeKind::Ptr:
    case TypeKind::F64:
      return 8;
    case TypeKind::I128:
    case TypeKind::F128:
      return 16;
  }
  return 0;
}

[[nodiscard]] inline std::size_t normalize_size(c4c::backend::bir::TypeKind type,
                                                std::size_t size_bytes) {
  return size_bytes == 0 ? fallback_type_size(type) : size_bytes;
}

[[nodiscard]] inline std::size_t normalize_alignment(c4c::backend::bir::TypeKind type,
                                                     std::size_t align_bytes,
                                                     std::size_t size_bytes) {
  const std::size_t normalized_size = normalize_size(type, size_bytes);
  if (align_bytes != 0) {
    return align_bytes;
  }
  if (normalized_size == 0) {
    return 1;
  }
  return std::min<std::size_t>(normalized_size, 16);
}

[[nodiscard]] inline std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment <= 1) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

}  // namespace c4c::backend::prepare::stack_layout
