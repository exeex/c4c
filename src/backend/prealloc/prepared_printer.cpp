#include "prepared_printer.hpp"

#include "prepared_printer/private.hpp"

#include "../bir/bir.hpp"

#include <iomanip>
#include <sstream>

namespace c4c::backend::prepare {

namespace {

std::string render_value(const c4c::backend::bir::Value& value) {
  if (value.kind == c4c::backend::bir::Value::Kind::Named) {
    return value.name;
  }
  if (value.type == c4c::backend::bir::TypeKind::F128 && value.f128_payload.has_value()) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setfill('0')
        << std::setw(16) << value.f128_payload->high_bits
        << std::setw(16) << value.f128_payload->low_bits;
    return out.str();
  }
  if (value.type == c4c::backend::bir::TypeKind::F32 ||
      value.type == c4c::backend::bir::TypeKind::F64) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setfill('0');
    if (value.type == c4c::backend::bir::TypeKind::F32) {
      out << std::setw(8) << static_cast<std::uint32_t>(value.immediate_bits);
    } else {
      out << std::setw(16) << value.immediate_bits;
    }
    return out.str();
  }
  return std::to_string(value.immediate);
}

std::string escape_quoted_text(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (const char ch : text) {
    if (ch == '\\' || ch == '"') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

// Step 5 fence: prepared-printer helpers expand interned IDs and route-local
// spellings only for debug/display output. They do not make raw text an
// authority for prepared or semantic identity.
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

std::string_view maybe_slot_name(const PreparedNameTables& names, std::optional<SlotNameId> id) {
  if (!id.has_value()) {
    return "<none>";
  }
  return prepared_slot_name(names, *id);
}

std::string_view maybe_link_name(const PreparedNameTables& names, std::optional<LinkNameId> id) {
  if (!id.has_value()) {
    return "<none>";
  }
  return prepared_link_name(names, *id);
}

std::string source_symbol_name(const PreparedNameTables& names,
                               const PreparedCallArgumentPlan& arg) {
  if (arg.source_symbol_name_id.has_value()) {
    const std::string_view semantic_name = prepared_link_name(names, *arg.source_symbol_name_id);
    if (!semantic_name.empty() && semantic_name.front() == '@') {
      return std::string(semantic_name);
    }
    return "@" + std::string(semantic_name);
  }
  // Step 5 fence: the legacy raw symbol spelling is printed only as prepared
  // route-debug text. Metadata-bearing call plans should prefer LinkNameId.
  if (arg.source_symbol_name.has_value()) {
    return *arg.source_symbol_name;
  }
  return {};
}

const PreparedRegallocFunction* find_regalloc_function(const PreparedBirModule& module,
                                                       FunctionNameId function_name) {
  for (const auto& function : module.regalloc.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

const PreparedRegallocValue* find_regalloc_value(const PreparedRegallocFunction& function,
                                                 PreparedValueId value_id) {
  for (const auto& value : function.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
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

std::string storage_encoding_kind_name(PreparedStorageEncodingKind kind) {
  switch (kind) {
    case PreparedStorageEncodingKind::None:
      return "none";
    case PreparedStorageEncodingKind::Register:
      return "register";
    case PreparedStorageEncodingKind::FrameSlot:
      return "frame_slot";
    case PreparedStorageEncodingKind::Immediate:
      return "immediate";
    case PreparedStorageEncodingKind::ComputedAddress:
      return "computed_address";
    case PreparedStorageEncodingKind::SymbolAddress:
      return "symbol_address";
  }
  return "unknown";
}

std::string type_kind_name(c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::Void:
      return "void";
    case c4c::backend::bir::TypeKind::I1:
      return "i1";
    case c4c::backend::bir::TypeKind::I8:
      return "i8";
    case c4c::backend::bir::TypeKind::I16:
      return "i16";
    case c4c::backend::bir::TypeKind::I32:
      return "i32";
    case c4c::backend::bir::TypeKind::I64:
      return "i64";
    case c4c::backend::bir::TypeKind::I128:
      return "i128";
    case c4c::backend::bir::TypeKind::Ptr:
      return "ptr";
    case c4c::backend::bir::TypeKind::F32:
      return "f32";
    case c4c::backend::bir::TypeKind::F64:
      return "f64";
    case c4c::backend::bir::TypeKind::F128:
      return "f128";
  }
  return "unknown";
}

std::string optional_size_text(const std::optional<std::size_t>& value) {
  return value.has_value() ? std::to_string(*value) : std::string("<unknown>");
}

void append_call_arg_source_summary(std::ostringstream& out,
                                    const PreparedNameTables& names,
                                    const PreparedCallArgumentPlan& arg) {
  out << storage_encoding_kind_name(arg.source_encoding);
  const std::string symbol_name = source_symbol_name(names, arg);
  if (!symbol_name.empty()) {
    out << ":" << symbol_name;
  } else if (arg.source_literal.has_value()) {
    out << ":" << render_value(*arg.source_literal);
  } else if (arg.source_base_value_name.has_value()) {
    out << ":" << maybe_value_name(names, *arg.source_base_value_name);
    if (arg.source_base_value_id.has_value()) {
      out << "#" << *arg.source_base_value_id;
    }
    if (arg.source_pointer_byte_delta.has_value()) {
      out << "+" << *arg.source_pointer_byte_delta;
    }
  } else if (arg.source_register_name.has_value()) {
    out << ":" << *arg.source_register_name;
  } else if (arg.source_stack_offset_bytes.has_value()) {
    out << ":stack+" << *arg.source_stack_offset_bytes;
  }
}

void append_indirect_callee_summary(std::ostringstream& out,
                                    const PreparedNameTables& names,
                                    const std::optional<PreparedIndirectCalleePlan>& callee) {
  if (!callee.has_value()) {
    return;
  }

  out << " indirect_callee=" << maybe_value_name(names, callee->value_name)
      << " indirect_home=" << storage_encoding_kind_name(callee->encoding)
      << " indirect_bank=" << prepared_register_bank_name(callee->bank);
  if (callee->value_id.has_value()) {
    out << " indirect_value_id=" << *callee->value_id;
  }
  if (callee->register_name.has_value()) {
    out << " indirect_reg=" << *callee->register_name;
  }
  if (callee->slot_id.has_value()) {
    out << " indirect_slot=#" << *callee->slot_id;
  }
  if (callee->stack_offset_bytes.has_value()) {
    out << " indirect_stack_offset=" << *callee->stack_offset_bytes;
  }
}

void append_indirect_callee_detail(std::ostringstream& out,
                                   const PreparedNameTables& names,
                                   const PreparedIndirectCalleePlan& callee) {
  out << " indirect_callee=" << maybe_value_name(names, callee.value_name)
      << " indirect_encoding=" << storage_encoding_kind_name(callee.encoding)
      << " indirect_bank=" << prepared_register_bank_name(callee.bank);
  if (callee.value_id.has_value()) {
    out << " indirect_value_id=" << *callee.value_id;
  }
  if (callee.register_name.has_value()) {
    out << " indirect_reg=" << *callee.register_name;
  }
  if (callee.slot_id.has_value()) {
    out << " indirect_slot=#" << *callee.slot_id;
  }
  if (callee.stack_offset_bytes.has_value()) {
    out << " indirect_stack_offset=" << *callee.stack_offset_bytes;
  }
  if (callee.immediate_i32.has_value()) {
    out << " indirect_imm_i32=" << *callee.immediate_i32;
  }
  if (callee.pointer_base_value_name.has_value()) {
    out << " indirect_base="
        << maybe_value_name(names, *callee.pointer_base_value_name);
  }
  if (callee.pointer_byte_delta.has_value()) {
    out << " indirect_delta=" << *callee.pointer_byte_delta;
  }
}

void append_memory_return_summary(std::ostringstream& out,
                                  const PreparedNameTables& names,
                                  const std::optional<PreparedMemoryReturnPlan>& memory_return) {
  if (!memory_return.has_value()) {
    return;
  }

  out << " memory_return=";
  if (memory_return->storage_slot_name != kInvalidSlotName) {
    out << prepared_slot_name(names, memory_return->storage_slot_name);
  } else {
    out << "<none>";
  }
  out << " memory_home=" << storage_encoding_kind_name(memory_return->encoding);
  if (memory_return->sret_arg_index.has_value()) {
    out << " sret_arg=" << *memory_return->sret_arg_index;
  }
  if (memory_return->stack_offset_bytes.has_value()) {
    out << " memory_stack_offset=" << *memory_return->stack_offset_bytes;
  }
}

void append_memory_return_detail(std::ostringstream& out,
                                 const PreparedNameTables& names,
                                 const PreparedMemoryReturnPlan& memory_return) {
  out << " memory_return=";
  if (memory_return.storage_slot_name != kInvalidSlotName) {
    out << prepared_slot_name(names, memory_return.storage_slot_name);
  } else {
    out << "<none>";
  }
  out << " memory_encoding=" << storage_encoding_kind_name(memory_return.encoding);
  if (memory_return.sret_arg_index.has_value()) {
    out << " sret_arg_index=" << *memory_return.sret_arg_index;
  }
  if (memory_return.slot_id.has_value()) {
    out << " memory_slot=#" << *memory_return.slot_id;
  }
  if (memory_return.stack_offset_bytes.has_value()) {
    out << " memory_stack_offset=" << *memory_return.stack_offset_bytes;
  }
  if (memory_return.size_bytes != 0) {
    out << " memory_size=" << memory_return.size_bytes;
  }
  if (memory_return.align_bytes != 0) {
    out << " memory_align=" << memory_return.align_bytes;
  }
}

void append_preserved_value_summary(std::ostringstream& out,
                                    const PreparedNameTables& names,
                                    const PreparedCallPreservedValue& preserved) {
  out << maybe_value_name(names, preserved.value_name) << "#" << preserved.value_id << ":"
      << prepared_call_preservation_route_name(preserved.route);
  if (preserved.route == PreparedCallPreservationRoute::StackSlot && preserved.slot_id.has_value()) {
    out << ":slot#" << *preserved.slot_id;
  }
  if (preserved.register_name.has_value()) {
    out << ":" << *preserved.register_name;
    if (preserved.contiguous_width > 1) {
      out << "/w" << preserved.contiguous_width;
    }
    if (!preserved.occupied_register_names.empty()) {
      out << "[";
      for (std::size_t index = 0; index < preserved.occupied_register_names.size(); ++index) {
        if (index != 0) {
          out << ",";
        }
        out << preserved.occupied_register_names[index];
      }
      out << "]";
    }
  } else if (preserved.stack_offset_bytes.has_value()) {
    out << ":stack+" << *preserved.stack_offset_bytes;
  }
  if (preserved.route == PreparedCallPreservationRoute::StackSlot) {
    if (preserved.stack_size_bytes.has_value()) {
      out << ":size=" << *preserved.stack_size_bytes;
    }
    if (preserved.stack_align_bytes.has_value()) {
      out << ":align=" << *preserved.stack_align_bytes;
    }
  }
  if (preserved.callee_saved_save_index.has_value()) {
    out << ":save" << *preserved.callee_saved_save_index;
  }
}

std::string maybe_register_bank(std::optional<PreparedRegisterBank> bank) {
  if (!bank.has_value()) {
    return "none";
  }
  return std::string(prepared_register_bank_name(*bank));
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

void append_spill_slot_placement(std::ostringstream& out,
                                 std::string_view label,
                                 const std::optional<PreparedSpillSlotPlacement>& placement) {
  if (!placement.has_value()) {
    return;
  }
  out << " " << label << "=slot#" << placement->slot_id
      << "+stack" << placement->offset_bytes;
}

void append_saved_register_slot_placement(
    std::ostringstream& out,
    const std::optional<PreparedSavedRegisterSlotPlacement>& placement) {
  if (!placement.has_value()) {
    return;
  }
  out << " slot_placement=";
  if (placement->slot_id.has_value()) {
    out << "slot#" << *placement->slot_id;
  } else {
    out << "<none>";
  }
  out << "+stack" << optional_size_text(placement->stack_offset_bytes)
      << " slot_size=" << optional_size_text(placement->size_bytes)
      << " slot_align=" << optional_size_text(placement->align_bytes)
      << " fixed_location=" << (placement->fixed_location ? "yes" : "no")
      << " slot_reg=" << prepared_register_bank_name(placement->bank)
      << ":" << placement->register_name
      << " slot_save_index=" << placement->save_index
      << " slot_width=" << placement->contiguous_width;
  if (!placement->occupied_register_names.empty()) {
    out << " slot_units=";
    for (std::size_t index = 0; index < placement->occupied_register_names.size(); ++index) {
      if (index != 0) {
        out << ",";
      }
      out << placement->occupied_register_names[index];
    }
  }
  append_register_placement(out, "slot_register_placement", placement->register_placement);
}

void append_register_occupancy(std::ostringstream& out,
                               std::size_t contiguous_width,
                               const std::vector<std::string>& occupied_register_names) {
  out << " width=" << contiguous_width;
  if (!occupied_register_names.empty()) {
    out << " units=";
    for (std::size_t index = 0; index < occupied_register_names.size(); ++index) {
      if (index != 0) {
        out << ",";
      }
      out << occupied_register_names[index];
    }
  }
}

void append_register_span_summary(std::ostringstream& out,
                                  std::string_view register_name,
                                  std::size_t contiguous_width,
                                  const std::vector<std::string>& occupied_register_names) {
  out << register_name;
  if (contiguous_width > 1 || occupied_register_names.size() > 1) {
    out << "/w" << contiguous_width;
    if (!occupied_register_names.empty()) {
      out << "[";
      for (std::size_t index = 0; index < occupied_register_names.size(); ++index) {
        if (index != 0) {
          out << ",";
        }
        out << occupied_register_names[index];
      }
      out << "]";
    }
  }
}

void append_clobbered_register_summary(std::ostringstream& out,
                                       const PreparedClobberedRegister& clobbered) {
  out << prepared_register_bank_name(clobbered.bank) << ":" << clobbered.register_name
      << "/w" << clobbered.contiguous_width;
  if (!clobbered.occupied_register_names.empty()) {
    out << "[";
    for (std::size_t index = 0; index < clobbered.occupied_register_names.size(); ++index) {
      if (index != 0) {
        out << ",";
      }
      out << clobbered.occupied_register_names[index];
    }
    out << "]";
  }
}

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
}

void append_regalloc(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-regalloc ---\n";
  for (const auto& function : module.regalloc.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function.function_name) << "\n";
    for (const auto& op : function.spill_reload_ops) {
      out << "  spill_reload kind=" << prepared_spill_reload_op_kind_name(op.op_kind)
          << " value_id=" << op.value_id;
      if (const auto* value = find_regalloc_value(function, op.value_id); value != nullptr) {
        out << " value=" << maybe_value_name(module.names, value->value_name);
      }
      out << " block_index=" << op.block_index
          << " inst_index=" << op.instruction_index
          << " bank=" << prepared_register_bank_name(op.register_bank);
      append_register_placement(out, "placement", op.register_placement);
      append_spill_slot_placement(out, "spill_slot", op.spill_slot_placement);
      if (op.register_name.has_value()) {
        out << " reg=" << *op.register_name;
      }
      append_register_occupancy(out, op.contiguous_width, op.occupied_register_names);
      if (op.slot_id.has_value()) {
        out << " slot_id=#" << *op.slot_id;
      }
      if (op.stack_offset_bytes.has_value()) {
        out << " stack_offset=" << *op.stack_offset_bytes;
      }
      out << "\n";
    }
  }
}

}  // namespace

std::string print(const PreparedBirModule& module) {
  std::ostringstream out;
  out << "prepared.module target=" << module.target_profile.triple
      << " route=" << prepare_route_name(module.route) << "\n";

  if (!module.completed_phases.empty()) {
    out << "completed_phases:";
    for (const auto& phase : module.completed_phases) {
      out << " " << phase;
    }
    out << "\n";
  }

  if (!module.invariants.empty()) {
    out << "invariants:\n";
    for (const auto& invariant : module.invariants) {
      out << "  - " << prepared_bir_invariant_name(invariant) << "\n";
    }
  }

  if (!module.notes.empty()) {
    out << "notes:\n";
    for (const auto& note : module.notes) {
      out << "  - [" << note.phase << "] " << note.message << "\n";
    }
  }

  out << "--- prepared-bir ---\n";
  out << c4c::backend::bir::print(module.module);
  if (!module.module.functions.empty() || !module.module.globals.empty() ||
      !module.module.string_constants.empty()) {
    out << "\n";
  }

  append_function_summaries(out, module);
  append_prepared_control_flow(out, module);
  append_value_locations(out, module);
  append_stack_layout(out, module);
  append_frame_plan(out, module);
  append_dynamic_stack_plan(out, module);
  append_call_plans(out, module);
  append_variadic_entry_plans(out, module);
  append_regalloc(out, module);
  append_storage_plans(out, module);
  append_i128_carriers(out, module);
  append_f128_carriers(out, module);
  append_atomic_operations(out, module);
  append_intrinsic_carriers(out, module);
  append_inline_asm_carriers(out, module);
  append_f128_runtime_helpers(out, module);
  append_i128_runtime_helpers(out, module);
  append_addressing(out, module);
  return out.str();
}

}  // namespace c4c::backend::prepare
