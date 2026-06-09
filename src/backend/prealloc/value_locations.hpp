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
  std::optional<LinkNameId> pointer_base_symbol_name;
  std::optional<std::int64_t> pointer_byte_delta;
};

// Value homes and move bundles publish backend-prepared lookup state. Block
// indexes, instruction indexes, and physical register spellings describe the
// prepared route only; semantic identity stays in PreparedValueId plus interned
// function/value/block IDs.
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

enum class PreparedBlockEntryPublicationStatus {
  Available,
  UnsupportedOperation,
  UnsupportedDestinationKind,
  UnsupportedDestinationStorage,
  MissingValueHome,
  MissingRegisterName,
};

[[nodiscard]] constexpr std::string_view prepared_block_entry_publication_status_name(
    PreparedBlockEntryPublicationStatus status) {
  switch (status) {
    case PreparedBlockEntryPublicationStatus::Available:
      return "available";
    case PreparedBlockEntryPublicationStatus::UnsupportedOperation:
      return "unsupported_operation";
    case PreparedBlockEntryPublicationStatus::UnsupportedDestinationKind:
      return "unsupported_destination_kind";
    case PreparedBlockEntryPublicationStatus::UnsupportedDestinationStorage:
      return "unsupported_destination_storage";
    case PreparedBlockEntryPublicationStatus::MissingValueHome:
      return "missing_value_home";
    case PreparedBlockEntryPublicationStatus::MissingRegisterName:
      return "missing_register_name";
  }
  return "unknown";
}

struct PreparedBlockEntryPublication {
  PreparedBlockEntryPublicationStatus status =
      PreparedBlockEntryPublicationStatus::MissingValueHome;
  const PreparedMoveBundle* bundle = nullptr;
  const PreparedMoveResolution* move = nullptr;
  const PreparedValueHome* home = nullptr;
  PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  PreparedMoveDestinationKind destination_kind = PreparedMoveDestinationKind::Value;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::string> destination_register_name;
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

// Text-name access resolves through the prepared value table before lookup.
// Callers with structured state should pass ValueNameId directly.
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

template <typename PreparedValueHomeIndexes>
[[nodiscard]] inline const PreparedValueHome* find_indexed_prepared_value_home(
    const PreparedValueHomeIndexes* value_home_indexes,
    const PreparedValueLocationFunction* function_locations,
    PreparedValueId value_id) {
  if (value_home_indexes != nullptr) {
    const auto it = value_home_indexes->homes_by_id.find(value_id);
    if (it != value_home_indexes->homes_by_id.end()) {
      return it->second;
    }
    return nullptr;
  }
  return function_locations == nullptr ? nullptr
                                       : find_prepared_value_home(*function_locations,
                                                                  value_id);
}

[[nodiscard]] inline std::optional<PreparedValueId> find_prepared_value_id(
    const PreparedRegallocFunction* regalloc,
    const PreparedValueLocationFunction* function_locations,
    ValueNameId value_name) {
  if (regalloc != nullptr) {
    for (const auto& value : regalloc->values) {
      if (value.value_name == value_name) {
        return value.value_id;
      }
    }
  }
  if (function_locations != nullptr) {
    for (const auto& home : function_locations->value_homes) {
      if (home.value_name == value_name) {
        return home.value_id;
      }
    }
  }
  return std::nullopt;
}

template <typename PreparedValueHomeIndexes>
[[nodiscard]] inline std::optional<PreparedValueId> find_indexed_prepared_value_id(
    const PreparedValueHomeIndexes* value_home_indexes,
    const PreparedRegallocFunction* regalloc,
    const PreparedValueLocationFunction* function_locations,
    ValueNameId value_name) {
  if (value_home_indexes != nullptr) {
    const auto it = value_home_indexes->value_ids.find(value_name);
    if (it != value_home_indexes->value_ids.end()) {
      return it->second;
    }
    return std::nullopt;
  }
  return find_prepared_value_id(regalloc, function_locations, value_name);
}

template <typename PreparedValueHomeIndexes>
[[nodiscard]] inline const PreparedValueHome* find_indexed_prepared_value_home(
    const PreparedValueHomeIndexes* value_home_indexes,
    const PreparedRegallocFunction* regalloc,
    const PreparedValueLocationFunction* function_locations,
    ValueNameId value_name) {
  const auto value_id =
      find_indexed_prepared_value_id(value_home_indexes,
                                     regalloc,
                                     function_locations,
                                     value_name);
  return value_id.has_value()
             ? find_indexed_prepared_value_home(value_home_indexes, function_locations, *value_id)
             : nullptr;
}

template <typename PreparedValueHomeIndexes>
[[nodiscard]] inline const PreparedValueHome* find_prepared_value_home_for_bir_value(
    const PreparedNameTables& names,
    const PreparedValueHomeIndexes* value_home_indexes,
    const PreparedRegallocFunction* regalloc,
    const PreparedValueLocationFunction* function_locations,
    const bir::Value& value) {
  if (function_locations == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  const ValueNameId value_name = names.value_names.find(value.name);
  if (value_name == kInvalidValueName) {
    return nullptr;
  }
  return find_indexed_prepared_value_home(value_home_indexes,
                                          regalloc,
                                          function_locations,
                                          value_name);
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

// The step index is the prepared parallel-copy plan's local move resolution
// coordinate. It is valid only with the matched bundle/edge authority above.
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

// Uniqueness is scoped to one prepared route phase/block index. This is a
// scheduling lookup for move insertion, not a semantic identity map.
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

[[nodiscard]] inline bool prepared_block_entry_publication_available(
    const PreparedBlockEntryPublication& publication) {
  return publication.status == PreparedBlockEntryPublicationStatus::Available;
}

[[nodiscard]] inline std::vector<PreparedBlockEntryPublication>
collect_prepared_block_entry_publications(
    const PreparedValueLocationFunction* function_locations,
    BlockLabelId successor_label) {
  std::vector<PreparedBlockEntryPublication> publications;
  if (function_locations == nullptr || successor_label == kInvalidBlockLabel) {
    return publications;
  }

  const std::optional<BlockLabelId> expected_successor{successor_label};
  for (const auto& bundle : function_locations->move_bundles) {
    if (bundle.phase != PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label != expected_successor) {
      continue;
    }

    for (const auto& move : bundle.moves) {
      PreparedBlockEntryPublication publication{
          .bundle = &bundle,
          .move = &move,
          .destination_value_id = move.to_value_id,
          .destination_kind = move.destination_kind,
          .destination_storage_kind = move.destination_storage_kind,
          .destination_register_name = move.destination_register_name,
      };

      const auto* home = find_prepared_value_home(*function_locations, move.to_value_id);
      publication.home = home;
      if (home != nullptr) {
        publication.destination_value_name = home->value_name;
      }

      if (move.op_kind != PreparedMoveResolutionOpKind::Move) {
        publication.status = PreparedBlockEntryPublicationStatus::UnsupportedOperation;
      } else if (move.destination_kind != PreparedMoveDestinationKind::Value) {
        publication.status =
            PreparedBlockEntryPublicationStatus::UnsupportedDestinationKind;
      } else if (move.destination_storage_kind != PreparedMoveStorageKind::Register) {
        publication.status =
            PreparedBlockEntryPublicationStatus::UnsupportedDestinationStorage;
      } else if (home == nullptr) {
        publication.status = PreparedBlockEntryPublicationStatus::MissingValueHome;
      } else {
        if (!publication.destination_register_name.has_value() &&
            home->kind == PreparedValueHomeKind::Register) {
          publication.destination_register_name = home->register_name;
        }
        publication.status = publication.destination_register_name.has_value()
                                 ? PreparedBlockEntryPublicationStatus::Available
                                 : PreparedBlockEntryPublicationStatus::MissingRegisterName;
      }
      publications.push_back(publication);
    }
  }
  return publications;
}

}  // namespace c4c::backend::prepare
