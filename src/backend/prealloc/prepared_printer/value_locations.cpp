#include "private.hpp"

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace {

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_block_label(const PreparedNameTables& names, BlockLabelId id) {
  if (id == kInvalidBlockLabel) {
    return "<none>";
  }
  return prepared_block_label(names, id);
}

std::string_view maybe_value_name(const PreparedNameTables& names, ValueNameId id) {
  if (id == kInvalidValueName) {
    return "<none>";
  }
  return prepared_value_name(names, id);
}

std::string move_destination_kind_name(PreparedMoveDestinationKind kind) {
  switch (kind) {
    case PreparedMoveDestinationKind::Value:
      return "value";
    case PreparedMoveDestinationKind::CallArgumentAbi:
      return "call_argument_abi";
    case PreparedMoveDestinationKind::CallResultAbi:
      return "call_result_abi";
    case PreparedMoveDestinationKind::FunctionReturnAbi:
      return "function_return_abi";
  }
  return "unknown";
}

std::string move_storage_kind_name(PreparedMoveStorageKind kind) {
  switch (kind) {
    case PreparedMoveStorageKind::None:
      return "none";
    case PreparedMoveStorageKind::Register:
      return "register";
    case PreparedMoveStorageKind::StackSlot:
      return "stack_slot";
  }
  return "unknown";
}

std::string move_resolution_op_kind_name(PreparedMoveResolutionOpKind kind) {
  switch (kind) {
    case PreparedMoveResolutionOpKind::Move:
      return "move";
    case PreparedMoveResolutionOpKind::SaveDestinationToTemp:
      return "save_destination_to_temp";
  }
  return "unknown";
}

void append_register_placement(std::ostringstream& out,
                               std::string_view label,
                               const std::optional<PreparedRegisterPlacement>& placement) {
  if (!placement.has_value()) {
    return;
  }
  out << " " << label << "="
      << prepared_register_bank_name(placement->bank) << ":"
      << prepared_register_slot_pool_name(placement->pool)
      << "#" << placement->slot_index
      << "/w" << placement->contiguous_width;
}

}  // namespace

void append_value_locations(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-value-locations ---\n";
  for (const auto& function_locations : module.value_locations.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_locations.function_name)
        << "\n";
    for (const auto& home : function_locations.value_homes) {
      out << "  home " << maybe_value_name(module.names, home.value_name)
          << " value_id=" << home.value_id
          << " kind=" << prepared_value_home_kind_name(home.kind);
      if (home.register_name.has_value()) {
        out << " reg=" << *home.register_name;
      }
      if (home.slot_id.has_value()) {
        out << " slot_id=" << *home.slot_id;
      }
      if (home.offset_bytes.has_value()) {
        out << " offset=" << *home.offset_bytes;
      }
      if (home.immediate_i32.has_value()) {
        out << " imm_i32=" << *home.immediate_i32;
      }
      if (home.immediate_f128.has_value()) {
        out << " imm_f128=0x" << std::hex << std::uppercase << std::setfill('0')
            << std::setw(16) << home.immediate_f128->high_bits
            << std::setw(16) << home.immediate_f128->low_bits
            << std::dec;
      }
      out << "\n";
    }

    for (const auto& bundle : function_locations.move_bundles) {
      out << "  move_bundle phase=" << prepared_move_phase_name(bundle.phase)
          << " authority=" << prepared_move_authority_kind_name(bundle.authority_kind)
          << " block_index=" << bundle.block_index
          << " instruction_index=" << bundle.instruction_index;
      if (bundle.source_parallel_copy_predecessor_label.has_value() &&
          bundle.source_parallel_copy_successor_label.has_value()) {
        out << " source_parallel_copy="
            << maybe_block_label(module.names, *bundle.source_parallel_copy_predecessor_label)
            << " -> "
            << maybe_block_label(module.names, *bundle.source_parallel_copy_successor_label);
      }
      out << "\n";
      for (const auto& move : bundle.moves) {
        out << "    move from_value_id=" << move.from_value_id
            << " to_value_id=" << move.to_value_id
            << " destination_kind=" << move_destination_kind_name(move.destination_kind)
            << " destination_storage=" << move_storage_kind_name(move.destination_storage_kind)
            << " op_kind=" << move_resolution_op_kind_name(move.op_kind)
            << " uses_cycle_temp_source=" << (move.uses_cycle_temp_source ? "yes" : "no");
        if (move.source_parallel_copy_step_index.has_value()) {
          out << " parallel_copy_step_index=" << *move.source_parallel_copy_step_index;
        }
        if (move.source_immediate_i32.has_value()) {
          out << " source_imm_i32=" << *move.source_immediate_i32;
        }
        if (move.destination_abi_index.has_value()) {
          out << " abi_index=" << *move.destination_abi_index;
        }
        append_register_placement(out, "placement", move.destination_register_placement);
        if (move.destination_register_name.has_value()) {
          out << " reg=" << *move.destination_register_name;
        }
        if (move.destination_stack_offset_bytes.has_value()) {
          out << " stack_offset=" << *move.destination_stack_offset_bytes;
        }
        if (!move.reason.empty()) {
          out << " reason=" << move.reason;
        }
        out << "\n";
      }
      for (const auto& abi : bundle.abi_bindings) {
        out << "    abi_binding destination_kind="
            << move_destination_kind_name(abi.destination_kind)
            << " destination_storage="
            << move_storage_kind_name(abi.destination_storage_kind);
        if (abi.destination_abi_index.has_value()) {
          out << " abi_index=" << *abi.destination_abi_index;
        }
        append_register_placement(out, "placement", abi.destination_register_placement);
        if (abi.destination_register_name.has_value()) {
          out << " reg=" << *abi.destination_register_name;
        }
        if (abi.destination_stack_offset_bytes.has_value()) {
          out << " stack_offset=" << *abi.destination_stack_offset_bytes;
        }
        out << "\n";
      }
    }
  }

  out << "--- prepared-block-entry-publications ---\n";
  for (const auto& function_locations : module.value_locations.functions) {
    std::vector<BlockLabelId> successor_labels;
    for (const auto& bundle : function_locations.move_bundles) {
      if (bundle.phase != PreparedMovePhase::BlockEntry ||
          !bundle.source_parallel_copy_successor_label.has_value()) {
        continue;
      }
      const BlockLabelId successor_label = *bundle.source_parallel_copy_successor_label;
      bool already_recorded = false;
      for (const auto recorded_label : successor_labels) {
        if (recorded_label == successor_label) {
          already_recorded = true;
          break;
        }
      }
      if (!already_recorded) {
        successor_labels.push_back(successor_label);
      }
    }

    if (successor_labels.empty()) {
      continue;
    }

    out << "prepared.func @" << maybe_function_name(module.names, function_locations.function_name)
        << "\n";
    for (const auto successor_label : successor_labels) {
      const auto publications =
          collect_prepared_block_entry_publications(&function_locations, successor_label);
      for (const auto& publication : publications) {
        out << "  block_entry_publication successor="
            << maybe_block_label(module.names, successor_label)
            << " status="
            << prepared_block_entry_publication_status_name(publication.status)
            << " to_value_id=" << publication.destination_value_id
            << " to=" << maybe_value_name(module.names, publication.destination_value_name)
            << " home_kind="
            << (publication.home == nullptr
                    ? std::string_view{"<none>"}
                    : prepared_value_home_kind_name(publication.home->kind))
            << " destination_kind="
            << move_destination_kind_name(publication.destination_kind)
            << " destination_storage="
            << move_storage_kind_name(publication.destination_storage_kind);
        if (publication.destination_register_name.has_value()) {
          out << " reg=" << *publication.destination_register_name;
        }
        if (publication.bundle != nullptr) {
          out << " block_index=" << publication.bundle->block_index
              << " instruction_index=" << publication.bundle->instruction_index;
        }
        out << "\n";
      }
    }
  }
}

}  // namespace c4c::backend::prepare
