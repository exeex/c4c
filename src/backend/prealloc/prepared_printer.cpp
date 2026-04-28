#include "prepared_printer.hpp"

#include "../bir/bir.hpp"

#include <iomanip>
#include <sstream>

namespace c4c::backend::prepare {

namespace {

std::string render_value(const c4c::backend::bir::Value& value) {
  if (value.kind == c4c::backend::bir::Value::Kind::Named) {
    return value.name;
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

const PreparedFrameSlot* find_frame_slot(const PreparedBirModule& module,
                                         PreparedFrameSlotId slot_id) {
  for (const auto& slot : module.stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
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

const bir::Function* find_prepared_function(const PreparedBirModule& module,
                                            FunctionNameId function_name) {
  if (function_name == kInvalidFunctionName) {
    return nullptr;
  }
  const auto name = prepared_function_name(module.names, function_name);
  for (const auto& function : module.module.functions) {
    if (function.name == name) {
      return &function;
    }
  }
  return nullptr;
}

std::string terminator_kind_name(c4c::backend::bir::TerminatorKind kind) {
  switch (kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      return "return";
    case c4c::backend::bir::TerminatorKind::Branch:
      return "branch";
    case c4c::backend::bir::TerminatorKind::CondBranch:
      return "cond_branch";
  }
  return "unknown";
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

void append_call_arg_source_summary(std::ostringstream& out,
                                    const PreparedNameTables& names,
                                    const PreparedCallArgumentPlan& arg) {
  out << storage_encoding_kind_name(arg.source_encoding);
  if (arg.source_symbol_name.has_value()) {
    out << ":" << *arg.source_symbol_name;
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

std::string prepared_join_transfer_ownership_name(const PreparedJoinTransfer& transfer) {
  if (transfer.source_branch_block_label.has_value()) {
    return "authoritative_branch_pair";
  }
  if (!transfer.edge_transfers.empty()) {
    return "per_edge";
  }
  return "none";
}

std::string prepared_parallel_copy_resolution_name(const PreparedParallelCopyBundle& bundle) {
  return bundle.has_cycle ? "cycle_break" : "acyclic";
}

void append_parallel_copy_step_detail(std::ostringstream& out,
                                      const PreparedNameTables& names,
                                      const PreparedParallelCopyBundle& bundle,
                                      const PreparedParallelCopyStep& step) {
  const auto* move = find_prepared_parallel_copy_move_for_step(bundle, step);
  if (move == nullptr) {
    out << " invalid_move_index";
    return;
  }

  if (step.kind == PreparedParallelCopyStepKind::SaveDestinationToTemp) {
    out << " save_destination=" << render_value(move->destination_value)
        << " blocked_source=" << render_value(move->source_value)
        << " temp_source=cycle_temp(" << render_value(move->destination_value) << ")"
        << " carrier=" << prepared_join_transfer_carrier_kind_name(move->carrier_kind);
    if (move->storage_name.has_value()) {
      out << " storage=" << prepared_slot_name(names, *move->storage_name);
    }
    return;
  }

  out << " source=";
  if (step.uses_cycle_temp_source) {
    out << "cycle_temp(" << render_value(move->source_value) << ")";
  } else {
    out << render_value(move->source_value);
  }
  out << " destination=" << render_value(move->destination_value)
      << " carrier=" << prepared_join_transfer_carrier_kind_name(move->carrier_kind);
  if (move->storage_name.has_value()) {
    out << " storage=" << prepared_slot_name(names, *move->storage_name);
  }
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

void append_function_summaries(std::ostringstream& out, const PreparedBirModule& module) {
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
        << " storage_values=" << (storage_plan != nullptr ? storage_plan->values.size() : 0)
        << "\n";

    if (frame_plan != nullptr) {
      for (const auto& saved : frame_plan->saved_callee_registers) {
        out << "  saved " << prepared_register_bank_name(saved.bank)
            << ":" << saved.register_name
            << " order=" << saved.save_index;
        append_register_occupancy(out, saved.contiguous_width, saved.occupied_register_names);
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
          out << "\n";
        }
      }
    }

    if (storage_plan != nullptr) {
      for (const auto& value : storage_plan->values) {
        out << "  storage " << maybe_value_name(module.names, value.value_name)
            << " " << storage_encoding_kind_name(value.encoding)
            << " bank=" << prepared_register_bank_name(value.bank);
        if (value.register_name.has_value()) {
          out << " reg=" << *value.register_name;
        }
        if (value.stack_offset_bytes.has_value()) {
          out << " offset=" << *value.stack_offset_bytes;
        }
        if (value.immediate_i32.has_value()) {
          out << " imm=" << *value.immediate_i32;
        }
        out << "\n";
      }
    }
  }
}

void append_prepared_control_flow(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-control-flow ---\n";
  for (const auto& function_cf : module.control_flow.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_cf.function_name) << "\n";
    const auto* function = find_prepared_function(module, function_cf.function_name);

    for (const auto& block : function_cf.blocks) {
      out << "  block " << maybe_block_label(module.names, block.block_label)
          << " terminator=" << terminator_kind_name(block.terminator_kind);
      if (block.branch_target_label != kInvalidBlockLabel) {
        out << " target=" << maybe_block_label(module.names, block.branch_target_label);
      }
      if (block.true_label != kInvalidBlockLabel || block.false_label != kInvalidBlockLabel) {
        out << " true=" << maybe_block_label(module.names, block.true_label)
            << " false=" << maybe_block_label(module.names, block.false_label);
      }
      out << "\n";
    }

    for (const auto& condition : function_cf.branch_conditions) {
      out << "  branch_condition " << maybe_block_label(module.names, condition.block_label)
          << " kind=" << prepared_branch_condition_kind_name(condition.kind)
          << " condition=" << render_value(condition.condition_value);
      if (condition.predicate.has_value()) {
        out << " compare=" << c4c::backend::bir::render_binary_opcode(*condition.predicate)
            << " " << c4c::backend::bir::render_type(*condition.compare_type)
            << " " << render_value(*condition.lhs)
            << ", " << render_value(*condition.rhs);
      }
      out << " can_fuse_with_branch=" << (condition.can_fuse_with_branch ? "yes" : "no")
          << " true=" << maybe_block_label(module.names, condition.true_label)
          << " false=" << maybe_block_label(module.names, condition.false_label)
          << "\n";
    }

    for (const auto& transfer : function_cf.join_transfers) {
      out << "  join_transfer " << maybe_block_label(module.names, transfer.join_block_label)
          << " result=" << render_value(transfer.result)
          << " kind=" << prepared_join_transfer_kind_name(transfer.kind)
          << " carrier="
          << prepared_join_transfer_carrier_kind_name(
                 effective_prepared_join_transfer_carrier_kind(transfer))
          << " ownership=" << prepared_join_transfer_ownership_name(transfer)
          << " incomings=" << transfer.incomings.size()
          << " edge_transfers=" << transfer.edge_transfers.size();
      if (transfer.storage_name.has_value()) {
        out << " storage=" << prepared_slot_name(module.names, *transfer.storage_name);
      }
      if (transfer.source_branch_block_label.has_value()) {
        out << " source_branch="
            << maybe_block_label(module.names, *transfer.source_branch_block_label);
      }
      if (transfer.source_true_incoming_label.has_value() ||
          transfer.source_false_incoming_label.has_value()) {
        out << " source_incomings=("
            << maybe_block_label(module.names,
                                 transfer.source_true_incoming_label.value_or(kInvalidBlockLabel))
            << ", "
            << maybe_block_label(module.names,
                                 transfer.source_false_incoming_label.value_or(kInvalidBlockLabel))
            << ")";
      }
      if (transfer.source_true_transfer_index.has_value() ||
          transfer.source_false_transfer_index.has_value()) {
        out << " source_transfer_indexes=("
            << (transfer.source_true_transfer_index.has_value()
                    ? std::to_string(*transfer.source_true_transfer_index)
                    : std::string("<none>"))
            << ", "
            << (transfer.source_false_transfer_index.has_value()
                    ? std::to_string(*transfer.source_false_transfer_index)
                    : std::string("<none>"))
            << ")";
      }
      const auto continuation_targets =
          published_prepared_compare_join_continuation_targets(transfer);
      if (continuation_targets.has_value()) {
        out << " continuation_targets=("
            << maybe_block_label(module.names, continuation_targets->true_label)
            << ", "
            << maybe_block_label(module.names, continuation_targets->false_label)
            << ")";
      }
      out << "\n";
      for (const auto& incoming : transfer.incomings) {
        out << "    incoming [" << incoming.label << "] -> " << render_value(incoming.value)
            << "\n";
      }
      for (const auto& edge : transfer.edge_transfers) {
        out << "    edge_transfer "
            << maybe_block_label(module.names, edge.predecessor_label)
            << " -> " << maybe_block_label(module.names, edge.successor_label)
            << " incoming=" << render_value(edge.incoming_value)
            << " destination=" << render_value(edge.destination_value);
        if (edge.storage_name.has_value()) {
          out << " storage=" << prepared_slot_name(module.names, *edge.storage_name);
        }
        out << "\n";
      }
    }

    for (const auto& bundle : function_cf.parallel_copy_bundles) {
      const auto execution_block =
          published_prepared_parallel_copy_execution_block_label(bundle);
      out << "  parallel_copy "
          << maybe_block_label(module.names, bundle.predecessor_label)
          << " -> " << maybe_block_label(module.names, bundle.successor_label)
          << " execution_site="
          << prepared_parallel_copy_execution_site_name(bundle.execution_site)
          << " execution_block="
          << maybe_block_label(module.names, execution_block.value_or(kInvalidBlockLabel))
          << " has_cycle=" << (bundle.has_cycle ? "yes" : "no")
          << " resolution=" << prepared_parallel_copy_resolution_name(bundle)
          << " moves=" << bundle.moves.size()
          << " steps=" << bundle.steps.size() << "\n";
      out << "    authority execution_site="
          << prepared_parallel_copy_execution_site_name(bundle.execution_site)
          << " execution_block="
          << maybe_block_label(module.names, execution_block.value_or(kInvalidBlockLabel))
          << "\n";
      for (std::size_t move_index = 0; move_index < bundle.moves.size(); ++move_index) {
        const auto& move = bundle.moves[move_index];
        out << "    move[" << move_index << "] "
            << render_value(move.source_value) << " -> " << render_value(move.destination_value)
            << " join_transfer_index=" << move.join_transfer_index
            << " edge_transfer_index=" << move.edge_transfer_index
            << " carrier="
            << prepared_join_transfer_carrier_kind_name(move.carrier_kind);
        if (move.storage_name.has_value()) {
          out << " storage=" << prepared_slot_name(module.names, *move.storage_name);
        }
        out << "\n";
      }
      for (std::size_t step_index = 0; step_index < bundle.steps.size(); ++step_index) {
        const auto& step = bundle.steps[step_index];
        out << "    step[" << step_index << "] "
            << prepared_parallel_copy_step_kind_name(step.kind)
            << " move_index=" << step.move_index;
        append_parallel_copy_step_detail(out, module.names, bundle, step);
        out << " uses_cycle_temp_source="
            << (step.uses_cycle_temp_source ? "yes" : "no") << "\n";
      }
    }
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

void append_stack_layout(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-stack-layout ---\n";
  out << "frame_size=" << module.stack_layout.frame_size_bytes
      << " frame_alignment=" << module.stack_layout.frame_alignment_bytes << "\n";
  for (const auto& object : module.stack_layout.objects) {
    out << "object #" << object.object_id
        << " func=" << maybe_function_name(module.names, object.function_name)
        << " name=" << prepared_stack_object_name(module.names, object)
        << " source_kind=" << object.source_kind
        << " type=" << c4c::backend::bir::render_type(object.type)
        << " size=" << object.size_bytes
        << " align=" << object.align_bytes
        << " address_exposed=" << (object.address_exposed ? "yes" : "no")
        << " requires_home_slot=" << (object.requires_home_slot ? "yes" : "no")
        << " permanent_home_slot=" << (object.permanent_home_slot ? "yes" : "no")
        << "\n";
  }
  for (const auto& slot : module.stack_layout.frame_slots) {
    out << "slot #" << slot.slot_id
        << " object_id=" << slot.object_id
        << " func=" << maybe_function_name(module.names, slot.function_name)
        << " offset=" << slot.offset_bytes
        << " size=" << slot.size_bytes
        << " align=" << slot.align_bytes
        << " fixed_location=" << (slot.fixed_location ? "yes" : "no")
        << "\n";
  }
}

void append_frame_plan(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-frame-plan ---\n";
  for (const auto& function_plan : module.frame_plan.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_plan.function_name)
        << " frame_size=" << function_plan.frame_size_bytes
        << " frame_alignment=" << function_plan.frame_alignment_bytes
        << " has_dynamic_stack=" << (function_plan.has_dynamic_stack ? "yes" : "no")
        << " fixed_slots_use_fp="
        << (function_plan.uses_frame_pointer_for_fixed_slots ? "yes" : "no")
        << "\n";
    for (const auto& saved : function_plan.saved_callee_registers) {
      out << "  saved_register bank=" << prepared_register_bank_name(saved.bank)
          << " reg=" << saved.register_name
          << " save_index=" << saved.save_index;
      append_register_occupancy(out, saved.contiguous_width, saved.occupied_register_names);
      out << "\n";
    }
    for (const auto slot_id : function_plan.frame_slot_order) {
      out << "  frame_slot_order slot_id=#" << slot_id;
      if (const auto* slot = find_frame_slot(module, slot_id); slot != nullptr) {
        out << " offset=" << slot->offset_bytes
            << " size=" << slot->size_bytes
            << " align=" << slot->align_bytes;
      }
      out << "\n";
    }
  }
}

void append_dynamic_stack_plan(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-dynamic-stack-plan ---\n";
  for (const auto& function_plan : module.dynamic_stack_plan.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_plan.function_name)
        << " requires_stack_save_restore="
        << (function_plan.requires_stack_save_restore ? "yes" : "no")
        << " fixed_slots_use_fp="
        << (function_plan.uses_frame_pointer_for_fixed_slots ? "yes" : "no")
        << "\n";
    for (const auto& op : function_plan.operations) {
      out << "  dynamic_stack_op block=" << maybe_block_label(module.names, op.block_label)
          << " inst_index=" << op.instruction_index
          << " kind=" << prepared_dynamic_stack_op_kind_name(op.kind);
      if (op.result_value_name.has_value()) {
        out << " result=" << prepared_value_name(module.names, *op.result_value_name);
      }
      if (op.operand_value_name.has_value()) {
        out << " operand=" << prepared_value_name(module.names, *op.operand_value_name);
      }
      if (!op.allocation_type_text.empty()) {
        out << " alloc_type=" << op.allocation_type_text;
      }
      if (op.element_size_bytes != 0) {
        out << " elem_size=" << op.element_size_bytes;
      }
      if (op.element_align_bytes != 0) {
        out << " elem_align=" << op.element_align_bytes;
      }
      out << "\n";
    }
  }
}

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
        if (arg.source_symbol_name.has_value()) {
          out << " source_symbol=" << *arg.source_symbol_name;
        }
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
        out << "\n";
      }
      for (const auto& clobbered : call.clobbered_registers) {
        out << "    clobber bank=" << prepared_register_bank_name(clobbered.bank)
            << " reg=" << clobbered.register_name;
        append_register_occupancy(out,
                                  clobbered.contiguous_width,
                                  clobbered.occupied_register_names);
        out << "\n";
      }
    }
  }
}

void append_storage_plans(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-storage-plans ---\n";
  for (const auto& function_plan : module.storage_plans.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_plan.function_name)
        << "\n";
    for (const auto& value : function_plan.values) {
      out << "  storage " << maybe_value_name(module.names, value.value_name)
          << " value_id=" << value.value_id
          << " encoding=" << storage_encoding_kind_name(value.encoding)
          << " bank=" << prepared_register_bank_name(value.bank);
      if (value.register_name.has_value()) {
        out << " reg=" << *value.register_name;
      }
      append_register_occupancy(out,
                                value.contiguous_width,
                                value.occupied_register_names);
      if (value.slot_id.has_value()) {
        out << " slot_id=#" << *value.slot_id;
      }
      if (value.stack_offset_bytes.has_value()) {
        out << " stack_offset=" << *value.stack_offset_bytes;
      }
      if (value.immediate_i32.has_value()) {
        out << " imm_i32=" << *value.immediate_i32;
      }
      if (value.symbol_name.has_value()) {
        out << " symbol=" << prepared_link_name(module.names, *value.symbol_name);
      }
      out << "\n";
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

void append_addressing(std::ostringstream& out, const PreparedBirModule& module) {
  out << "--- prepared-addressing ---\n";
  for (const auto& function_addressing : module.addressing.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_addressing.function_name)
        << " frame_size=" << function_addressing.frame_size_bytes
        << " frame_alignment=" << function_addressing.frame_alignment_bytes << "\n";
    for (const auto& access : function_addressing.accesses) {
      out << "  access block=" << maybe_block_label(module.names, access.block_label)
          << " inst_index=" << access.inst_index
          << " base=" << prepared_address_base_kind_name(access.address.base_kind);
      if (access.result_value_name.has_value()) {
        out << " result=" << prepared_value_name(module.names, *access.result_value_name);
      }
      if (access.stored_value_name.has_value()) {
        out << " stored=" << prepared_value_name(module.names, *access.stored_value_name);
      }
      if (access.address.frame_slot_id.has_value()) {
        out << " frame_slot=#" << *access.address.frame_slot_id;
      }
      if (access.address.symbol_name.has_value()) {
        out << " symbol=" << prepared_link_name(module.names, *access.address.symbol_name);
      }
      if (access.address.pointer_value_name.has_value()) {
        out << " pointer=" << prepared_value_name(module.names, *access.address.pointer_value_name);
      }
      out << " offset=" << access.address.byte_offset
          << " size=" << access.address.size_bytes
          << " align=" << access.address.align_bytes
          << " base_plus_offset="
          << (access.address.can_use_base_plus_offset ? "yes" : "no")
          << "\n";
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
  append_regalloc(out, module);
  append_storage_plans(out, module);
  append_addressing(out, module);
  return out.str();
}

}  // namespace c4c::backend::prepare
