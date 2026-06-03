#pragma once

#include "names.hpp"

#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedStackSliceFamily {
  SlotNameId family_name = kInvalidSlotName;
  std::size_t slice_offset = 0;
  bool legacy_slot_name_compatibility = false;
};

struct PreparedStackObject {
  PreparedObjectId object_id = 0;
  FunctionNameId function_name = kInvalidFunctionName;
  std::optional<SlotNameId> slot_name;
  std::optional<ValueNameId> value_name;
  std::string source_kind;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool address_exposed = false;
  bool requires_home_slot = false;
  bool permanent_home_slot = false;
  std::optional<PreparedStackSliceFamily> slice_family;
  bool aggregate_address_published = false;
  std::optional<ValueNameId> frame_address_value_name;
  bool legacy_frame_address_name_compatibility = false;
};

struct PreparedFrameSlot {
  PreparedFrameSlotId slot_id = 0;
  PreparedObjectId object_id = 0;
  FunctionNameId function_name = kInvalidFunctionName;
  std::size_t offset_bytes = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool fixed_location = false;
};

[[nodiscard]] inline std::string_view prepared_stack_object_slot_name(
    const PreparedNameTables& names,
    const PreparedStackObject& object) {
  return object.slot_name.has_value() ? prepared_slot_name(names, *object.slot_name)
                                      : std::string_view{};
}

[[nodiscard]] inline std::string_view prepared_stack_object_value_name(
    const PreparedNameTables& names,
    const PreparedStackObject& object) {
  return object.value_name.has_value() ? prepared_value_name(names, *object.value_name)
                                       : std::string_view{};
}

[[nodiscard]] inline std::string_view prepared_stack_object_name(
    const PreparedNameTables& names,
    const PreparedStackObject& object) {
  const std::string_view slot_name = prepared_stack_object_slot_name(names, object);
  return slot_name.empty() ? prepared_stack_object_value_name(names, object) : slot_name;
}

struct PreparedStackLayout {
  std::vector<PreparedStackObject> objects;
  std::vector<PreparedFrameSlot> frame_slots;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
};

enum class PreparedRegisterBank {
  None,
  Gpr,
  Fpr,
  Vreg,
  AggregateAddress,
};

[[nodiscard]] constexpr std::string_view prepared_register_bank_name(
    PreparedRegisterBank bank) {
  switch (bank) {
    case PreparedRegisterBank::None:
      return "none";
    case PreparedRegisterBank::Gpr:
      return "gpr";
    case PreparedRegisterBank::Fpr:
      return "fpr";
    case PreparedRegisterBank::Vreg:
      return "vreg";
    case PreparedRegisterBank::AggregateAddress:
      return "aggregate_address";
  }
  return "unknown";
}

enum class PreparedRegisterSlotPool {
  None,
  CallerSaved,
  CalleeSaved,
  CallArgument,
  CallResult,
  ReservedScratch,
};

[[nodiscard]] constexpr std::string_view prepared_register_slot_pool_name(
    PreparedRegisterSlotPool pool) {
  switch (pool) {
    case PreparedRegisterSlotPool::None:
      return "none";
    case PreparedRegisterSlotPool::CallerSaved:
      return "caller_saved";
    case PreparedRegisterSlotPool::CalleeSaved:
      return "callee_saved";
    case PreparedRegisterSlotPool::CallArgument:
      return "call_argument";
    case PreparedRegisterSlotPool::CallResult:
      return "call_result";
    case PreparedRegisterSlotPool::ReservedScratch:
      return "reserved_scratch";
  }
  return "unknown";
}

struct PreparedRegisterPlacement {
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  PreparedRegisterSlotPool pool = PreparedRegisterSlotPool::None;
  std::size_t slot_index = 0;
  std::size_t contiguous_width = 1;

  friend bool operator==(const PreparedRegisterPlacement& lhs,
                         const PreparedRegisterPlacement& rhs) {
    return lhs.bank == rhs.bank && lhs.pool == rhs.pool &&
           lhs.slot_index == rhs.slot_index &&
           lhs.contiguous_width == rhs.contiguous_width;
  }

  friend bool operator!=(const PreparedRegisterPlacement& lhs,
                         const PreparedRegisterPlacement& rhs) {
    return !(lhs == rhs);
  }
};

[[nodiscard]] inline bool has_prepared_register_placement(
    const PreparedRegisterPlacement& placement) {
  return placement.bank != PreparedRegisterBank::None &&
         placement.pool != PreparedRegisterSlotPool::None &&
         placement.contiguous_width > 0;
}

struct PreparedSpillSlotPlacement {
  PreparedFrameSlotId slot_id = 0;
  std::size_t offset_bytes = 0;

  friend bool operator==(const PreparedSpillSlotPlacement& lhs,
                         const PreparedSpillSlotPlacement& rhs) {
    return lhs.slot_id == rhs.slot_id && lhs.offset_bytes == rhs.offset_bytes;
  }

  friend bool operator!=(const PreparedSpillSlotPlacement& lhs,
                         const PreparedSpillSlotPlacement& rhs) {
    return !(lhs == rhs);
  }
};

struct PreparedSavedRegisterSlotPlacement {
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  std::string register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::size_t save_index = 0;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::size_t> size_bytes;
  std::optional<std::size_t> align_bytes;
  bool fixed_location = false;
};

[[nodiscard]] inline bool has_complete_prepared_saved_register_slot_placement(
    const PreparedSavedRegisterSlotPlacement& placement) {
  return placement.bank != PreparedRegisterBank::None &&
         !placement.register_name.empty() &&
         placement.contiguous_width > 0 &&
         !placement.occupied_register_names.empty() &&
         placement.register_placement.has_value() &&
         has_prepared_register_placement(*placement.register_placement) &&
         placement.slot_id.has_value() &&
         placement.stack_offset_bytes.has_value() &&
         placement.size_bytes.has_value() &&
         *placement.size_bytes > 0 &&
         placement.align_bytes.has_value() &&
         *placement.align_bytes > 0;
}

struct PreparedSavedRegister {
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  std::string register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::size_t save_index = 0;
  std::optional<PreparedRegisterPlacement> placement;
  std::optional<PreparedSavedRegisterSlotPlacement> slot_placement;
};

struct PreparedFramePlanFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  std::vector<PreparedSavedRegister> saved_callee_registers;
  std::vector<PreparedFrameSlotId> frame_slot_order;
  bool has_dynamic_stack = false;
  bool uses_frame_pointer_for_fixed_slots = false;
};

struct PreparedFramePlan {
  std::vector<PreparedFramePlanFunction> functions;
};

enum class PreparedDynamicStackOpKind {
  StackSave,
  DynamicAlloca,
  StackRestore,
};

[[nodiscard]] constexpr std::string_view prepared_dynamic_stack_op_kind_name(
    PreparedDynamicStackOpKind kind) {
  switch (kind) {
    case PreparedDynamicStackOpKind::StackSave:
      return "stack_save";
    case PreparedDynamicStackOpKind::DynamicAlloca:
      return "dynamic_alloca";
    case PreparedDynamicStackOpKind::StackRestore:
      return "stack_restore";
  }
  return "unknown";
}

struct PreparedDynamicStackOp {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  PreparedDynamicStackOpKind kind = PreparedDynamicStackOpKind::StackSave;
  std::optional<ValueNameId> result_value_name;
  std::optional<ValueNameId> operand_value_name;
  std::string allocation_type_text;
  std::size_t element_size_bytes = 0;
  std::size_t element_align_bytes = 0;
};

struct PreparedDynamicStackPlanFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  bool requires_stack_save_restore = false;
  bool uses_frame_pointer_for_fixed_slots = false;
  std::vector<PreparedDynamicStackOp> operations;
};

struct PreparedDynamicStackPlan {
  std::vector<PreparedDynamicStackPlanFunction> functions;
};

[[nodiscard]] inline const PreparedFrameSlot* find_prepared_frame_slot(
    const PreparedStackLayout& stack_layout,
    PreparedObjectId object_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.object_id == object_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::optional<std::pair<std::string_view, std::size_t>>
parse_prepared_slot_slice_name(std::string_view slot_name) {
  const auto dot = slot_name.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= slot_name.size()) {
    return std::nullopt;
  }

  std::size_t slice_offset = 0;
  const auto suffix = slot_name.substr(dot + 1);
  const auto* begin = suffix.data();
  const auto* end = begin + suffix.size();
  const auto [ptr, ec] = std::from_chars(begin, end, slice_offset);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return std::pair<std::string_view, std::size_t>{slot_name.substr(0, dot), slice_offset};
}

[[nodiscard]] inline std::optional<std::size_t> find_prepared_stack_frame_offset_by_name(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    FunctionNameId function_name,
    std::string_view requested_name) {
  if (requested_name.empty() || function_name == kInvalidFunctionName) {
    return std::nullopt;
  }

  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function_name ||
        prepared_stack_object_name(names, object) != requested_name) {
      continue;
    }
    const auto* frame_slot = find_prepared_frame_slot(stack_layout, object.object_id);
    if (frame_slot != nullptr) {
      return frame_slot->offset_bytes;
    }
  }

  const auto requested_slice = parse_prepared_slot_slice_name(requested_name);
  std::optional<std::size_t> best_slice_offset;
  std::optional<std::size_t> best_frame_offset;
  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function_name || !object.slice_family.has_value()) {
      continue;
    }
    const auto family_name = prepared_slot_name(names, object.slice_family->family_name);
    const std::size_t slice_offset = object.slice_family->slice_offset;

    std::optional<std::size_t> resolved_offset;
    if (requested_slice.has_value()) {
      if (family_name != requested_slice->first ||
          requested_slice->second < slice_offset ||
          requested_slice->second >= slice_offset + object.size_bytes) {
        continue;
      }
      const auto* frame_slot = find_prepared_frame_slot(stack_layout, object.object_id);
      if (frame_slot == nullptr) {
        continue;
      }
      resolved_offset =
          frame_slot->offset_bytes + (requested_slice->second - slice_offset);
    } else {
      if (family_name != requested_name || slice_offset != 0) {
        continue;
      }
      const auto* frame_slot = find_prepared_frame_slot(stack_layout, object.object_id);
      if (frame_slot == nullptr) {
        continue;
      }
      resolved_offset = frame_slot->offset_bytes;
    }

    if (!resolved_offset.has_value() ||
        (best_slice_offset.has_value() && slice_offset >= *best_slice_offset)) {
      continue;
    }
    best_slice_offset = slice_offset;
    best_frame_offset = *resolved_offset;
  }

  return best_frame_offset;
}

}  // namespace c4c::backend::prepare
