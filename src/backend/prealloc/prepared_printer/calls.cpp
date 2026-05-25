#include "private.hpp"

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_value_name(const PreparedNameTables& names, ValueNameId id) {
  if (id == kInvalidValueName) {
    return "<none>";
  }
  return prepared_value_name(names, id);
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

}  // namespace

void append_call_plans(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-call-plans ---\n";
  for (const auto& function_plan : module.call_plans.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_plan.function_name)
        << "\n";
    for (const auto& call : function_plan.calls) {
      out << "  call block_index=" << call.block_index
          << " inst_index=" << call.instruction_index
          << " wrapper_kind=" << prepared_call_wrapper_kind_name(call.wrapper_kind)
          << " variadic_fpr_arg_register_count=" << call.variadic_fpr_arg_register_count
          << " indirect=" << (call.is_indirect ? "yes" : "no");
      if (call.direct_callee_name.has_value()) {
        out << " callee=" << *call.direct_callee_name;
      }
      if (call.indirect_callee.has_value()) {
        append_indirect_callee_detail(out, module.names, *call.indirect_callee);
      }
      if (call.memory_return.has_value()) {
        append_memory_return_detail(out, module.names, *call.memory_return);
      }
      out << "\n";
      for (const auto& arg : call.arguments) {
        out << "    arg index=" << arg.arg_index
            << " value_bank=" << prepared_register_bank_name(arg.value_bank)
            << " source_encoding=" << storage_encoding_kind_name(arg.source_encoding);
        if (arg.source_value_id.has_value()) {
          out << " source_value_id=" << *arg.source_value_id;
        }
        if (arg.source_base_value_id.has_value()) {
          out << " source_base_value_id=" << *arg.source_base_value_id;
        }
        if (arg.source_literal.has_value()) {
          out << " source_literal=" << render_value(*arg.source_literal);
        }
        const std::string symbol_name = source_symbol_name(module.names, arg);
        if (!symbol_name.empty()) {
          out << " source_symbol=" << symbol_name;
          if (arg.source_symbol_name_id.has_value()) {
            out << " source_symbol_id=" << *arg.source_symbol_name_id;
          }
        }
        append_register_placement(out, "source_placement", arg.source_register_placement);
        if (arg.source_register_name.has_value()) {
          out << " source_reg=" << *arg.source_register_name;
        }
        if (arg.source_slot_id.has_value()) {
          out << " source_slot=#" << *arg.source_slot_id;
        }
        if (arg.source_stack_offset_bytes.has_value()) {
          out << " source_stack_offset=" << *arg.source_stack_offset_bytes;
        }
        out << " source_bank=" << maybe_register_bank(arg.source_register_bank);
        if (arg.source_base_value_name.has_value()) {
          out << " source_base="
              << maybe_value_name(module.names, *arg.source_base_value_name);
        }
        if (arg.source_pointer_byte_delta.has_value()) {
          out << " source_delta=" << *arg.source_pointer_byte_delta;
        }
        append_register_placement(out, "dest_placement", arg.destination_register_placement);
        if (arg.destination_register_name.has_value()) {
          out << " dest_reg=" << *arg.destination_register_name;
        }
        if (arg.destination_contiguous_width > 1 ||
            arg.destination_occupied_register_names.size() > 1) {
          append_register_occupancy(out,
                                    arg.destination_contiguous_width,
                                    arg.destination_occupied_register_names);
        }
        out << " dest_bank=" << maybe_register_bank(arg.destination_register_bank);
        if (arg.destination_stack_offset_bytes.has_value()) {
          out << " dest_stack_offset=" << *arg.destination_stack_offset_bytes;
        }
        if (arg.destination_stack_size_bytes.has_value()) {
          out << " dest_stack_size=" << *arg.destination_stack_size_bytes;
        }
        out << "\n";
      }
      if (call.result.has_value()) {
        const auto& result = *call.result;
        out << "    result value_bank=" << prepared_register_bank_name(result.value_bank)
            << " source_storage=" << move_storage_kind_name(result.source_storage_kind)
            << " destination_storage="
            << move_storage_kind_name(result.destination_storage_kind);
        if (result.destination_value_id.has_value()) {
          out << " destination_value_id=" << *result.destination_value_id;
        }
        append_register_placement(out, "source_placement", result.source_register_placement);
        if (result.source_register_name.has_value()) {
          out << " source_reg=" << *result.source_register_name;
        }
        if (result.source_stack_offset_bytes.has_value()) {
          out << " source_stack_offset=" << *result.source_stack_offset_bytes;
        }
        if (result.source_contiguous_width > 1 ||
            result.source_occupied_register_names.size() > 1) {
          append_register_occupancy(out,
                                    result.source_contiguous_width,
                                    result.source_occupied_register_names);
        }
        out << " source_bank=" << maybe_register_bank(result.source_register_bank);
        append_register_placement(out, "dest_placement", result.destination_register_placement);
        append_spill_slot_placement(out,
                                    "dest_spill_slot",
                                    result.destination_spill_slot_placement);
        if (result.destination_register_name.has_value()) {
          out << " dest_reg=" << *result.destination_register_name;
        }
        if (result.destination_contiguous_width > 1 ||
            result.destination_occupied_register_names.size() > 1) {
          append_register_occupancy(out,
                                    result.destination_contiguous_width,
                                    result.destination_occupied_register_names);
        }
        out << " dest_bank=" << maybe_register_bank(result.destination_register_bank);
        if (result.destination_slot_id.has_value()) {
          out << " destination_slot=#" << *result.destination_slot_id;
        }
        if (result.destination_stack_offset_bytes.has_value()) {
          out << " dest_stack_offset=" << *result.destination_stack_offset_bytes;
        }
        out << "\n";
      }
      for (const auto& preserved : call.preserved_values) {
        out << "    preserve value=" << maybe_value_name(module.names, preserved.value_name)
            << " value_id=" << preserved.value_id
            << " route=" << prepared_call_preservation_route_name(preserved.route);
        if (preserved.callee_saved_save_index.has_value()) {
          out << " save_index=" << *preserved.callee_saved_save_index;
        }
        append_register_placement(out, "placement", preserved.register_placement);
        append_spill_slot_placement(out, "spill_slot", preserved.spill_slot_placement);
        if (preserved.register_name.has_value()) {
          out << " reg=" << *preserved.register_name;
        }
        out << " bank=" << maybe_register_bank(preserved.register_bank);
        if (preserved.slot_id.has_value()) {
          out << " slot=#" << *preserved.slot_id;
        }
        if (preserved.stack_offset_bytes.has_value()) {
          out << " stack_offset=" << *preserved.stack_offset_bytes;
        }
        if (preserved.stack_size_bytes.has_value()) {
          out << " stack_size=" << *preserved.stack_size_bytes;
        }
        if (preserved.stack_align_bytes.has_value()) {
          out << " stack_align=" << *preserved.stack_align_bytes;
        }
        out << "\n";
      }
      for (const auto& clobbered : call.clobbered_registers) {
        out << "    clobber bank=" << prepared_register_bank_name(clobbered.bank);
        append_register_placement(out, "placement", clobbered.placement);
        out << " reg=" << clobbered.register_name;
        append_register_occupancy(out,
                                  clobbered.contiguous_width,
                                  clobbered.occupied_register_names);
        out << "\n";
      }
    }
  }
}

}  // namespace c4c::backend::prepare
