#pragma once

#include "names.hpp"
#include "regalloc.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedValueHomeKind {
  None,
  Register,
  StackSlot,
  RematerializableImmediate,
  PointerBasePlusOffset,
};

[[nodiscard]] constexpr std::string_view prepared_value_home_kind_name(
    PreparedValueHomeKind kind) {
  switch (kind) {
    case PreparedValueHomeKind::None:
      return "none";
    case PreparedValueHomeKind::Register:
      return "register";
    case PreparedValueHomeKind::StackSlot:
      return "stack_slot";
    case PreparedValueHomeKind::RematerializableImmediate:
      return "rematerializable_immediate";
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return "pointer_base_plus_offset";
  }
  return "unknown";
}

enum class PreparedMovePhase {
  BlockEntry,
  BeforeInstruction,
  BeforeCall,
  AfterCall,
  BeforeReturn,
};

[[nodiscard]] constexpr std::string_view prepared_move_phase_name(PreparedMovePhase phase) {
  switch (phase) {
    case PreparedMovePhase::BlockEntry:
      return "block_entry";
    case PreparedMovePhase::BeforeInstruction:
      return "before_instruction";
    case PreparedMovePhase::BeforeCall:
      return "before_call";
    case PreparedMovePhase::AfterCall:
      return "after_call";
    case PreparedMovePhase::BeforeReturn:
      return "before_return";
  }
  return "unknown";
}

struct PreparedValueHome {
  PreparedValueId value_id = 0;
  FunctionNameId function_name = kInvalidFunctionName;
  ValueNameId value_name = kInvalidValueName;
  PreparedValueHomeKind kind = PreparedValueHomeKind::None;
  std::optional<std::string> register_name;
  std::optional<PreparedTargetRegisterIdentity> target_register_identity;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> offset_bytes;
  std::optional<std::size_t> size_bytes;
  std::optional<std::size_t> align_bytes;
  std::optional<std::int64_t> immediate_i32;
  std::optional<c4c::backend::bir::Value::F128Payload> immediate_f128;
  std::optional<ValueNameId> pointer_base_value_name;
  std::optional<std::int64_t> pointer_byte_delta;
};

// Step 5 fence: value homes and move bundles are backend-prepared route-local
// lookup state. Block indexes, instruction indexes, and physical register
// spellings here describe the prepared route only; semantic identity stays in
// PreparedValueId plus interned function/value/block IDs.
struct PreparedMoveBundle {
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedMovePhase phase = PreparedMovePhase::BeforeInstruction;
  PreparedMoveAuthorityKind authority_kind = PreparedMoveAuthorityKind::None;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::optional<BlockLabelId> source_parallel_copy_predecessor_label;
  std::optional<BlockLabelId> source_parallel_copy_successor_label;
  std::vector<PreparedMoveResolution> moves;
  std::vector<PreparedAbiBinding> abi_bindings;
};

[[nodiscard]] inline bool prepared_move_resolution_has_out_of_ssa_parallel_copy_authority(
    const PreparedMoveResolution& move) {
  return move.authority_kind == PreparedMoveAuthorityKind::OutOfSsaParallelCopy;
}

[[nodiscard]] inline bool prepared_move_bundle_has_out_of_ssa_parallel_copy_authority(
    const PreparedMoveBundle& bundle) {
  return bundle.authority_kind == PreparedMoveAuthorityKind::OutOfSsaParallelCopy;
}

struct PreparedValueLocationFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedValueHome> value_homes;
  std::vector<PreparedMoveBundle> move_bundles;
};

struct PreparedValueLocations {
  std::vector<PreparedValueLocationFunction> functions;
};

[[nodiscard]] inline const PreparedValueLocationFunction* find_prepared_value_location_function(
    const PreparedValueLocations& value_locations,
    FunctionNameId function_name) {
  for (const auto& function_locations : value_locations.functions) {
    if (function_locations.function_name == function_name) {
      return &function_locations;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedValueLocationFunction* find_prepared_value_location_function(
    const PreparedNameTables& names,
    const PreparedValueLocations& value_locations,
    std::string_view function_name) {
  const FunctionNameId function_name_id = names.function_names.find(function_name);
  if (function_name_id == kInvalidFunctionName) {
    return nullptr;
  }
  return find_prepared_value_location_function(value_locations, function_name_id);
}

[[nodiscard]] inline const PreparedValueHome* find_prepared_value_home(
    const PreparedValueLocationFunction& function_locations,
    PreparedValueId value_id) {
  for (const auto& value_home : function_locations.value_homes) {
    if (value_home.value_id == value_id) {
      return &value_home;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedValueHome* find_prepared_value_home(
    const PreparedValueLocationFunction& function_locations,
    ValueNameId value_name) {
  for (const auto& value_home : function_locations.value_homes) {
    if (value_home.value_name == value_name) {
      return &value_home;
    }
  }
  return nullptr;
}

// Step 5 fence: this spelling bridge is for prepared-route diagnostics and
// legacy callers that still name a value by text. It resolves through the
// prepared value table before lookup; callers with structured state should pass
// ValueNameId directly.
[[nodiscard]] inline const PreparedValueHome* find_prepared_value_home(
    const PreparedNameTables& names,
    const PreparedValueLocationFunction& function_locations,
    std::string_view value_name) {
  const ValueNameId value_name_id = names.value_names.find(value_name);
  if (value_name_id == kInvalidValueName) {
    return nullptr;
  }
  return find_prepared_value_home(function_locations, value_name_id);
}

[[nodiscard]] inline const PreparedMoveBundle* find_prepared_move_bundle(
    const PreparedValueLocationFunction& function_locations,
    PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (const auto& move_bundle : function_locations.move_bundles) {
    if (move_bundle.phase == phase && move_bundle.block_index == block_index &&
        move_bundle.instruction_index == instruction_index) {
      return &move_bundle;
    }
  }
  return nullptr;
}

// Step 5 fence: the step index is the prepared parallel-copy plan's local move
// resolution coordinate. It is valid only with the matched bundle/edge authority
// above.
[[nodiscard]] inline const PreparedMoveResolution*
find_prepared_out_of_ssa_parallel_copy_move_for_step(
    const PreparedMoveBundle& move_bundle,
    std::size_t parallel_copy_step_index) {
  if (move_bundle.authority_kind != PreparedMoveAuthorityKind::OutOfSsaParallelCopy) {
    return nullptr;
  }
  const PreparedMoveResolution* match = nullptr;
  for (const auto& move : move_bundle.moves) {
    if (move.source_parallel_copy_step_index != parallel_copy_step_index) {
      continue;
    }
    if (match != nullptr) {
      return nullptr;
    }
    match = &move;
  }
  return match;
}

// Step 5 fence: uniqueness is scoped to one prepared route phase/block index.
// This is a scheduling lookup for move insertion, not a semantic identity map.
[[nodiscard]] inline const PreparedMoveBundle* find_prepared_unique_move_bundle(
    const PreparedValueLocationFunction& function_locations,
    PreparedMovePhase phase,
    std::size_t block_index) {
  const PreparedMoveBundle* match = nullptr;
  for (const auto& move_bundle : function_locations.move_bundles) {
    if (move_bundle.phase != phase || move_bundle.block_index != block_index) {
      continue;
    }
    if (match != nullptr) {
      return nullptr;
    }
    match = &move_bundle;
  }
  return match;
}

}  // namespace c4c::backend::prepare
