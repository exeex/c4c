#include "private.hpp"

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace function_summary_printer {

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

std::string source_symbol_name(const PreparedNameTables& names,
                               const PreparedCallArgumentPlan& arg) {
  if (arg.source_symbol_name_id.has_value()) {
    const std::string_view semantic_name = prepared_link_name(names, *arg.source_symbol_name_id);
    if (!semantic_name.empty() && semantic_name.front() == '@') {
      return std::string(semantic_name);
    }
    return "@" + std::string(semantic_name);
  }
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

std::string move_storage_kind_text(PreparedMoveStorageKind kind) {
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

void append_preservation_endpoint(std::ostringstream& out,
                                  std::string_view label,
                                  const PreparedCallBoundaryEffectEndpoint& endpoint) {
  out << " " << label << "=" << move_storage_kind_text(endpoint.storage_kind);
  if (endpoint.register_name.has_value()) {
    out << ":" << *endpoint.register_name;
  } else if (endpoint.slot_id.has_value()) {
    out << ":slot#" << *endpoint.slot_id;
  } else if (endpoint.stack_offset_bytes.has_value()) {
    out << ":stack+" << *endpoint.stack_offset_bytes;
  }
  if (endpoint.value_id.has_value()) {
    out << ":value#" << *endpoint.value_id;
  }
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

}  // namespace function_summary_printer

void append_function_summaries(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace function_summary_printer;

  out << "--- prepared-function-summaries ---\n";
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    const auto function_name_id = module.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }
    const auto* frame_plan = find_prepared_frame_plan(module, function_name_id);
    const auto* dynamic_plan = find_prepared_dynamic_stack_plan(module, function_name_id);
    const auto* call_plans = find_prepared_call_plans(module, function_name_id);
    const auto* variadic_entry_plan =
        find_prepared_variadic_entry_plan(module, function_name_id);
    const auto* storage_plan = find_prepared_storage_plan(module, function_name_id);

    out << "prepared.summary @" << function.name
        << " stable_base="
        << ((frame_plan != nullptr && frame_plan->uses_frame_pointer_for_fixed_slots) ? "fp"
                                                                                       : "rsp")
        << " frame_size=" << (frame_plan != nullptr ? frame_plan->frame_size_bytes : 0)
        << " frame_alignment=" << (frame_plan != nullptr ? frame_plan->frame_alignment_bytes : 1)
        << " has_dynamic_stack="
        << ((frame_plan != nullptr && frame_plan->has_dynamic_stack) ? "yes" : "no")
        << " saved_regs="
        << (frame_plan != nullptr ? frame_plan->saved_callee_registers.size() : 0)
        << " calls=" << (call_plans != nullptr ? call_plans->calls.size() : 0)
        << " dynamic_stack_ops=" << (dynamic_plan != nullptr ? dynamic_plan->operations.size() : 0)
        << " variadic_entry=" << (variadic_entry_plan != nullptr ? "yes" : "no")
        << " storage_values=" << (storage_plan != nullptr ? storage_plan->values.size() : 0)
        << "\n";

    if (frame_plan != nullptr) {
      for (const auto& saved : frame_plan->saved_callee_registers) {
        out << "  saved " << prepared_register_bank_name(saved.bank)
            << ":" << saved.register_name
            << " order=" << saved.save_index;
        append_register_placement(out, "placement", saved.placement);
        append_register_occupancy(out, saved.contiguous_width, saved.occupied_register_names);
        append_saved_register_slot_placement(out, saved.slot_placement);
        out << "\n";
      }
    }

    if (dynamic_plan != nullptr) {
      for (const auto& op : dynamic_plan->operations) {
        out << "  dynamic_stack block=" << maybe_block_label(module.names, op.block_label)
            << " inst=" << op.instruction_index
            << " kind=" << prepared_dynamic_stack_op_kind_name(op.kind);
        if (op.result_value_name.has_value()) {
          out << " result=" << prepared_value_name(module.names, *op.result_value_name);
        }
        if (op.operand_value_name.has_value()) {
          out << " operand=" << prepared_value_name(module.names, *op.operand_value_name);
        }
        out << "\n";
      }
    }

    if (call_plans != nullptr) {
      for (const auto& call : call_plans->calls) {
        out << "  callsite block=" << call.block_index
            << " inst=" << call.instruction_index
            << " wrapper=" << prepared_call_wrapper_kind_name(call.wrapper_kind);
        if (call.direct_callee_name.has_value()) {
          out << " callee=" << *call.direct_callee_name;
        }
        out << " variadic_fpr_args=" << call.variadic_fpr_arg_register_count;
        out << " args=" << call.arguments.size();
        append_indirect_callee_summary(out, module.names, call.indirect_callee);
        append_memory_return_summary(out, module.names, call.memory_return);
        if (call.result.has_value()) {
          out << " result_bank=" << prepared_register_bank_name(call.result->value_bank);
        }
        if (!call.preserved_values.empty()) {
          out << " preserves=";
          for (std::size_t index = 0; index < call.preserved_values.size(); ++index) {
            if (index != 0) {
              out << ",";
            }
            append_preserved_value_summary(out, module.names, call.preserved_values[index]);
          }
        }
        if (!call.clobbered_registers.empty()) {
          out << " clobbers=";
          for (std::size_t index = 0; index < call.clobbered_registers.size(); ++index) {
            if (index != 0) {
              out << ",";
            }
            append_clobbered_register_summary(out, call.clobbered_registers[index]);
          }
        }
        out << "\n";
        for (const auto& arg : call.arguments) {
          out << "    arg" << arg.arg_index
              << " bank=" << prepared_register_bank_name(arg.value_bank)
              << " from=";
          append_call_arg_source_summary(out, module.names, arg);
          out << " to=";
          if (arg.destination_register_name.has_value()) {
            append_register_span_summary(out,
                                         *arg.destination_register_name,
                                         arg.destination_contiguous_width,
                                         arg.destination_occupied_register_names);
          } else if (arg.destination_stack_offset_bytes.has_value()) {
            out << "stack+" << *arg.destination_stack_offset_bytes;
            if (arg.destination_stack_size_bytes.has_value()) {
              out << ":size=" << *arg.destination_stack_size_bytes;
            }
          } else {
            out << "none";
          }
          out << "\n";
        }
        if (call.result.has_value()) {
          const auto& result = *call.result;
          out << "    result bank=" << prepared_register_bank_name(result.value_bank)
              << " from=" << move_storage_kind_name(result.source_storage_kind);
          if (result.source_register_name.has_value()) {
            out << ":";
            append_register_span_summary(out,
                                         *result.source_register_name,
                                         result.source_contiguous_width,
                                         result.source_occupied_register_names);
          } else if (result.source_stack_offset_bytes.has_value()) {
            out << ":stack+" << *result.source_stack_offset_bytes;
          } else {
            out << ":none";
          }
          out << " to=" << move_storage_kind_name(result.destination_storage_kind);
          if (result.destination_register_name.has_value()) {
            out << ":";
            append_register_span_summary(out,
                                         *result.destination_register_name,
                                         result.destination_contiguous_width,
                                         result.destination_occupied_register_names);
          } else if (result.destination_stack_offset_bytes.has_value()) {
            out << ":stack+" << *result.destination_stack_offset_bytes;
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
          if (preserved.contiguous_width > 1) {
            out << " width=" << preserved.contiguous_width;
          }
          out << " bank=" << maybe_register_bank(preserved.register_bank);
          if (!preserved.occupied_register_names.empty()) {
            out << " units=";
            for (std::size_t index = 0; index < preserved.occupied_register_names.size(); ++index) {
              if (index != 0) {
                out << ",";
              }
              out << preserved.occupied_register_names[index];
            }
          }
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
          append_preservation_endpoint(out, "preservation_source", preserved.preservation_source);
          append_preservation_endpoint(
              out, "preservation_destination", preserved.preservation_destination);
          if (!preserved.preservation_reason.empty()) {
            out << " preservation_reason=" << preserved.preservation_reason;
          }
          out << "\n";
        }
      }
    }

    if (storage_plan != nullptr) {
      for (const auto& value : storage_plan->values) {
        out << "  storage " << maybe_value_name(module.names, value.value_name)
            << " " << storage_encoding_kind_name(value.encoding)
            << " bank=" << prepared_register_bank_name(value.bank);
        append_register_placement(out, "placement", value.register_placement);
        append_spill_slot_placement(out, "spill_slot", value.spill_slot_placement);
        if (value.register_name.has_value()) {
          out << " reg=" << *value.register_name;
        }
        if (value.stack_offset_bytes.has_value()) {
          out << " offset=" << *value.stack_offset_bytes;
        }
        if (value.immediate_i32.has_value()) {
          out << " imm=" << *value.immediate_i32;
        }
        if (value.immediate_f128.has_value()) {
          out << " imm_f128=0x" << std::hex << std::uppercase << std::setfill('0')
              << std::setw(16) << value.immediate_f128->high_bits
              << std::setw(16) << value.immediate_f128->low_bits
              << std::dec;
        }
        out << "\n";
      }
    }
  }
}

}  // namespace c4c::backend::prepare
