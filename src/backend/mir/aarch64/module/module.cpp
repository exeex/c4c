#include "module.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace c4c::backend::aarch64::module {
namespace {

[[nodiscard]] std::optional<c4c::BlockLabelId> prepared_label_for_bir_block(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      const auto prepared_label =
          c4c::backend::prepare::resolve_prepared_block_label_id(names, structured_label);
      if (prepared_label.has_value()) {
        return prepared_label;
      }
    }
  }
  return c4c::backend::prepare::resolve_prepared_block_label_id(names, block.label);
}

[[nodiscard]] const c4c::backend::bir::Function* find_source_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name) {
  if (function_name == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  for (const auto& function : prepared.module.functions) {
    std::optional<c4c::FunctionNameId> candidate;
    if (function.link_name_id != c4c::kInvalidLinkName) {
      const std::string_view structured_name =
          prepared.module.names.link_names.spelling(function.link_name_id);
      candidate = c4c::backend::prepare::resolve_prepared_function_name_id(prepared.names,
                                                                           structured_name);
    } else {
      candidate =
          c4c::backend::prepare::resolve_prepared_function_name_id(prepared.names, function.name);
    }
    if (candidate.has_value() && *candidate == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] const c4c::backend::bir::Block* find_source_block(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Function* function,
    c4c::BlockLabelId block_label) {
  if (function == nullptr || block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  for (const auto& block : function->blocks) {
    const auto candidate =
        prepared_label_for_bir_block(prepared.names, prepared.module.names, block);
    if (candidate.has_value() && *candidate == block_label) {
      return &block;
    }
  }
  return nullptr;
}

[[nodiscard]] c4c::backend::prepare::PreparedRegisterClass register_class_from_bank(
    c4c::backend::prepare::PreparedRegisterBank bank) {
  using c4c::backend::prepare::PreparedRegisterBank;
  using c4c::backend::prepare::PreparedRegisterClass;
  switch (bank) {
    case PreparedRegisterBank::Gpr:
      return PreparedRegisterClass::General;
    case PreparedRegisterBank::Fpr:
      return PreparedRegisterClass::Float;
    case PreparedRegisterBank::Vreg:
      return PreparedRegisterClass::Vector;
    case PreparedRegisterBank::AggregateAddress:
      return PreparedRegisterClass::AggregateAddress;
    case PreparedRegisterBank::None:
      return PreparedRegisterClass::None;
  }
  return PreparedRegisterClass::None;
}

[[nodiscard]] std::vector<std::string_view> register_name_views(
    const std::vector<std::string>& names) {
  std::vector<std::string_view> views;
  views.reserve(names.size());
  for (const auto& name : names) {
    views.push_back(name);
  }
  return views;
}

[[nodiscard]] const c4c::backend::prepare::PreparedRegallocFunction*
find_prepared_regalloc_function(const c4c::backend::prepare::PreparedRegalloc& regalloc,
                                c4c::FunctionNameId function_name) {
  for (const auto& function : regalloc.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] OperandRecord& ensure_operand(std::vector<OperandRecord>& operands,
                                            c4c::backend::prepare::PreparedValueId value_id,
                                            c4c::FunctionNameId function_name) {
  const auto found = std::find_if(operands.begin(),
                                  operands.end(),
                                  [value_id](const OperandRecord& operand) {
                                    return operand.value_id == value_id;
                                  });
  if (found != operands.end()) {
    return *found;
  }
  operands.push_back(OperandRecord{.value_id = value_id, .function_name = function_name});
  return operands.back();
}

[[nodiscard]] std::size_t append_register_record(
    std::vector<TargetRegisterRecord>& registers,
    TargetRegisterReferenceKind reference_kind,
    c4c::backend::prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    c4c::backend::prepare::PreparedRegisterClass reg_class,
    c4c::backend::prepare::PreparedRegisterBank reg_bank,
    std::string_view physical_register,
    std::size_t contiguous_width,
    std::vector<std::string_view> occupied_registers) {
  registers.push_back(TargetRegisterRecord{.reference_kind = reference_kind,
                                           .value_id = value_id,
                                           .value_name = value_name,
                                           .register_class = reg_class,
                                           .register_bank = reg_bank,
                                           .physical_register = physical_register,
                                           .contiguous_width = contiguous_width,
                                           .occupied_registers = std::move(occupied_registers)});
  return registers.size() - 1U;
}

void merge_value_home_operand(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedValueHome& home,
    std::vector<OperandRecord>& operands,
    std::vector<TargetRegisterRecord>& registers) {
  auto& operand = ensure_operand(operands, home.value_id, home.function_name);
  operand.value_name = home.value_name;
  operand.label = c4c::backend::prepare::prepared_value_name(prepared.names, home.value_name);
  operand.home_kind = home.kind;
  operand.frame_slot_id = home.slot_id;
  if (home.offset_bytes.has_value()) {
    operand.stack_offset_bytes = home.offset_bytes;
  }
  operand.immediate_i32 = home.immediate_i32;
  operand.pointer_base_value_name = home.pointer_base_value_name;
  if (home.pointer_base_value_name.has_value()) {
    operand.pointer_base_label =
        c4c::backend::prepare::prepared_value_name(prepared.names, *home.pointer_base_value_name);
  }
  operand.pointer_byte_delta = home.pointer_byte_delta;
  operand.source_value_home = &home;
  if (home.register_name.has_value()) {
    operand.value_home_register = append_register_record(registers,
                                                         TargetRegisterReferenceKind::ValueHome,
                                                         home.value_id,
                                                         home.value_name,
                                                         c4c::backend::prepare::
                                                             PreparedRegisterClass::None,
                                                         c4c::backend::prepare::
                                                             PreparedRegisterBank::None,
                                                         *home.register_name,
                                                         1,
                                                         {});
  }
}

void merge_regalloc_operand(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedRegallocValue& value,
    std::vector<OperandRecord>& operands,
    std::vector<TargetRegisterRecord>& registers) {
  auto& operand = ensure_operand(operands, value.value_id, value.function_name);
  operand.value_name = value.value_name;
  operand.label = c4c::backend::prepare::prepared_value_name(prepared.names, value.value_name);
  operand.type = value.type;
  operand.value_kind = value.value_kind;
  operand.stack_object_id = value.stack_object_id;
  operand.register_class = value.register_class;
  operand.register_group_width = value.register_group_width;
  operand.source_regalloc = &value;
  if (value.assigned_stack_slot.has_value()) {
    operand.frame_slot_id = value.assigned_stack_slot->slot_id;
    operand.stack_offset_bytes = value.assigned_stack_slot->offset_bytes;
  }
  if (value.assigned_register.has_value()) {
    operand.assigned_register =
        append_register_record(registers,
                               TargetRegisterReferenceKind::RegallocAssignment,
                               value.value_id,
                               value.value_name,
                               value.assigned_register->reg_class,
                               c4c::backend::prepare::PreparedRegisterBank::None,
                               value.assigned_register->register_name,
                               value.assigned_register->contiguous_width,
                               register_name_views(
                                   value.assigned_register->occupied_register_names));
  }
  if (value.spill_register_authority.has_value()) {
    operand.spill_register_authority =
        append_register_record(registers,
                               TargetRegisterReferenceKind::SpillAuthority,
                               value.value_id,
                               value.value_name,
                               value.spill_register_authority->reg_class,
                               c4c::backend::prepare::PreparedRegisterBank::None,
                               value.spill_register_authority->register_name,
                               value.spill_register_authority->contiguous_width,
                               register_name_views(
                                   value.spill_register_authority->occupied_register_names));
  }
}

void merge_storage_operand(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedStoragePlanValue& value,
    c4c::FunctionNameId function_name,
    std::vector<OperandRecord>& operands,
    std::vector<TargetRegisterRecord>& registers) {
  auto& operand = ensure_operand(operands, value.value_id, function_name);
  operand.value_name = value.value_name;
  operand.label = c4c::backend::prepare::prepared_value_name(prepared.names, value.value_name);
  operand.storage_encoding = value.encoding;
  operand.storage_bank = value.bank;
  operand.frame_slot_id = value.slot_id;
  operand.stack_offset_bytes = value.stack_offset_bytes;
  operand.immediate_i32 = value.immediate_i32;
  operand.symbol_name = value.symbol_name;
  if (value.symbol_name.has_value()) {
    operand.symbol_label = prepared.names.link_names.spelling(*value.symbol_name);
  }
  operand.source_storage = &value;
  if (value.register_name.has_value()) {
    operand.storage_register =
        append_register_record(registers,
                               TargetRegisterReferenceKind::StoragePlan,
                               value.value_id,
                               value.value_name,
                               register_class_from_bank(value.bank),
                               value.bank,
                               *value.register_name,
                               value.contiguous_width,
                               register_name_views(value.occupied_register_names));
  }
}

[[nodiscard]] std::vector<OperandRecord> build_operands(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::vector<TargetRegisterRecord>& registers) {
  std::vector<OperandRecord> operands;
  if (const auto* locations =
          c4c::backend::prepare::find_prepared_value_location_function(prepared, function_name);
      locations != nullptr) {
    operands.reserve(locations->value_homes.size());
    for (const auto& home : locations->value_homes) {
      merge_value_home_operand(prepared, home, operands, registers);
    }
  }
  if (const auto* regalloc = find_prepared_regalloc_function(prepared.regalloc, function_name);
      regalloc != nullptr) {
    operands.reserve(std::max(operands.size(), regalloc->values.size()));
    for (const auto& value : regalloc->values) {
      merge_regalloc_operand(prepared, value, operands, registers);
    }
  }
  if (const auto* storage = c4c::backend::prepare::find_prepared_storage_plan(prepared,
                                                                              function_name);
      storage != nullptr) {
    operands.reserve(std::max(operands.size(), storage->values.size()));
    for (const auto& value : storage->values) {
      merge_storage_operand(prepared, value, function_name, operands, registers);
    }
  }
  return operands;
}

[[nodiscard]] BlockRecord build_block_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Function* source_function,
    const c4c::backend::prepare::PreparedControlFlowBlock& block) {
  return BlockRecord{
      .block_label = block.block_label,
      .label = c4c::backend::prepare::prepared_block_label(prepared.names, block.block_label),
      .terminator_kind = block.terminator_kind,
      .branch_target_label = block.branch_target_label,
      .true_label = block.true_label,
      .false_label = block.false_label,
      .source_block = find_source_block(prepared, source_function, block.block_label),
      .control_flow = &block,
  };
}

[[nodiscard]] FunctionRecord build_function_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& function) {
  const auto* source_function = find_source_function(prepared, function.function_name);
  FunctionRecord record{
      .function_name = function.function_name,
      .label = c4c::backend::prepare::prepared_function_name(prepared.names,
                                                             function.function_name),
      .source_function = source_function,
      .control_flow = &function,
  };
  record.operands = build_operands(prepared, function.function_name, record.target_registers);
  record.blocks.reserve(function.blocks.size());
  for (const auto& block : function.blocks) {
    record.blocks.push_back(build_block_record(prepared, source_function, block));
  }
  return record;
}

[[nodiscard]] std::vector<FunctionRecord> build_function_records(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  std::vector<FunctionRecord> functions;
  functions.reserve(prepared.control_flow.functions.size());
  for (const auto& function : prepared.control_flow.functions) {
    functions.push_back(build_function_record(prepared, function));
  }
  return functions;
}

}  // namespace

BuildResult build(const c4c::backend::prepare::PreparedBirModule& prepared) {
  const c4c::TargetProfile target_profile =
      c4c::backend::aarch64::abi::resolve_target_profile(prepared);
  if (auto error = c4c::backend::aarch64::abi::validate_prepared_module_handoff(prepared)) {
    return BuildResult{.module = std::nullopt, .error = std::move(error)};
  }
  return BuildResult{.module = Module{.prepared = &prepared,
                                      .target_profile = target_profile,
                                      .functions = build_function_records(prepared)},
                     .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::module
