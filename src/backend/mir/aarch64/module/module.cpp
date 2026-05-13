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

[[nodiscard]] const c4c::backend::prepare::PreparedStackObject* find_stack_object(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    c4c::backend::prepare::PreparedObjectId object_id) {
  for (const auto& object : stack_layout.objects) {
    if (object.object_id == object_id) {
      return &object;
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
    AllocationSnapshotKind allocation_snapshot,
    c4c::backend::prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    c4c::backend::prepare::PreparedRegisterClass reg_class,
    c4c::backend::prepare::PreparedRegisterBank reg_bank,
    std::string_view physical_register,
    std::size_t contiguous_width,
    std::vector<std::string_view> occupied_registers) {
  const auto parsed = c4c::backend::aarch64::abi::parse_aarch64_register_name(physical_register);
  registers.push_back(TargetRegisterRecord{.reference_kind = reference_kind,
                                           .allocation_snapshot = allocation_snapshot,
                                           .value_id = value_id,
                                           .value_name = value_name,
                                           .register_class = reg_class,
                                           .register_bank = reg_bank,
                                           .allocation_pool =
                                               parsed.has_value()
                                                   ? std::optional{
                                                         c4c::backend::aarch64::abi::
                                                             allocation_register_pool(*parsed)}
                                                   : std::nullopt,
                                           .physical_register = physical_register,
                                           .contiguous_width = contiguous_width,
                                           .occupied_registers = std::move(occupied_registers),
                                           .may_be_long_lived_home =
                                               parsed.has_value() &&
                                               c4c::backend::aarch64::abi::
                                                   is_long_lived_allocatable_candidate(*parsed)});
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
    operand.stack_offset_is_prepared_snapshot = true;
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
                                                         AllocationSnapshotKind::PreparedSnapshot,
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
    operand.stack_offset_is_prepared_snapshot = true;
  }
  if (value.assigned_register.has_value()) {
    operand.assigned_register =
        append_register_record(registers,
                               TargetRegisterReferenceKind::RegallocAssignment,
                               AllocationSnapshotKind::AllocationResult,
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
                               AllocationSnapshotKind::SpillReloadScratch,
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
  operand.stack_offset_is_prepared_snapshot = value.stack_offset_bytes.has_value();
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
                               AllocationSnapshotKind::PreparedSnapshot,
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

[[nodiscard]] CalleeSaveRecord build_callee_save_record(
    const c4c::backend::prepare::PreparedSavedRegister& saved) {
  return CalleeSaveRecord{
      .bank = saved.bank,
      .register_name = saved.register_name,
      .contiguous_width = saved.contiguous_width,
      .occupied_registers = register_name_views(saved.occupied_register_names),
      .save_index = saved.save_index,
      .source_saved_register = &saved,
  };
}

[[nodiscard]] CalleeSaveRecord build_clobbered_register_record(
    const c4c::backend::prepare::PreparedClobberedRegister& clobbered) {
  return CalleeSaveRecord{
      .bank = clobbered.bank,
      .register_name = clobbered.register_name,
      .contiguous_width = clobbered.contiguous_width,
      .occupied_registers = register_name_views(clobbered.occupied_register_names),
  };
}

[[nodiscard]] FrameSlotRecord build_frame_slot_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedFrameSlot& slot) {
  const auto* object = find_stack_object(prepared.stack_layout, slot.object_id);
  FrameSlotRecord record{
      .slot_id = slot.slot_id,
      .object_id = slot.object_id,
      .function_name = slot.function_name,
      .offset_bytes = slot.offset_bytes,
      .size_bytes = slot.size_bytes,
      .align_bytes = slot.align_bytes,
      .fixed_location = slot.fixed_location,
      .source_slot = &slot,
      .source_object = object,
  };
  if (object != nullptr) {
    record.slot_name = object->slot_name;
    record.slot_label = c4c::backend::prepare::prepared_stack_object_slot_name(prepared.names,
                                                                                *object);
    record.value_name = object->value_name;
    record.value_label = c4c::backend::prepare::prepared_stack_object_value_name(prepared.names,
                                                                                  *object);
    record.type = object->type;
    record.address_exposed = object->address_exposed;
    record.requires_home_slot = object->requires_home_slot;
    record.permanent_home_slot = object->permanent_home_slot;
  }
  return record;
}

[[nodiscard]] DynamicStackRecord build_dynamic_stack_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedDynamicStackOp& op) {
  DynamicStackRecord record{
      .block_label = op.block_label,
      .block_label_text = c4c::backend::prepare::prepared_block_label(prepared.names,
                                                                       op.block_label),
      .instruction_index = op.instruction_index,
      .kind = op.kind,
      .result_value_name = op.result_value_name,
      .operand_value_name = op.operand_value_name,
      .allocation_type_text = op.allocation_type_text,
      .element_size_bytes = op.element_size_bytes,
      .element_align_bytes = op.element_align_bytes,
      .source_op = &op,
  };
  if (op.result_value_name.has_value()) {
    record.result_label =
        c4c::backend::prepare::prepared_value_name(prepared.names, *op.result_value_name);
  }
  if (op.operand_value_name.has_value()) {
    record.operand_label =
        c4c::backend::prepare::prepared_value_name(prepared.names, *op.operand_value_name);
  }
  return record;
}

[[nodiscard]] FrameRecord build_frame_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name) {
  const auto* frame_plan = c4c::backend::prepare::find_prepared_frame_plan(prepared,
                                                                           function_name);
  const auto* dynamic_stack = c4c::backend::prepare::find_prepared_dynamic_stack_plan(
      prepared, function_name);
  FrameRecord record{
      .source_frame_plan = frame_plan,
      .source_dynamic_stack = dynamic_stack,
  };
  if (frame_plan != nullptr) {
    record.frame_size_bytes = frame_plan->frame_size_bytes;
    record.frame_alignment_bytes = frame_plan->frame_alignment_bytes;
    record.has_dynamic_stack = frame_plan->has_dynamic_stack;
    record.uses_frame_pointer_for_fixed_slots = frame_plan->uses_frame_pointer_for_fixed_slots;
    record.frame_slot_order = frame_plan->frame_slot_order;
    record.callee_saves.reserve(frame_plan->saved_callee_registers.size());
    for (const auto& saved : frame_plan->saved_callee_registers) {
      record.callee_saves.push_back(build_callee_save_record(saved));
    }
  }
  for (const auto& slot : prepared.stack_layout.frame_slots) {
    if (slot.function_name == function_name) {
      record.slots.push_back(build_frame_slot_record(prepared, slot));
    }
  }
  if (dynamic_stack != nullptr) {
    record.requires_stack_save_restore = dynamic_stack->requires_stack_save_restore;
    if (dynamic_stack->uses_frame_pointer_for_fixed_slots) {
      record.uses_frame_pointer_for_fixed_slots = true;
    }
    record.dynamic_stack.reserve(dynamic_stack->operations.size());
    for (const auto& op : dynamic_stack->operations) {
      record.dynamic_stack.push_back(build_dynamic_stack_record(prepared, op));
    }
  }
  return record;
}

[[nodiscard]] std::vector<BranchRecord> build_branch_records(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& function) {
  std::vector<BranchRecord> branches;
  branches.reserve(function.branch_conditions.size());
  for (const auto& condition : function.branch_conditions) {
    branches.push_back(BranchRecord{
        .block_label = condition.block_label,
        .block_label_text = c4c::backend::prepare::prepared_block_label(prepared.names,
                                                                         condition.block_label),
        .condition_kind = condition.kind,
        .condition_value = condition.condition_value,
        .predicate = condition.predicate,
        .compare_type = condition.compare_type,
        .lhs = condition.lhs,
        .rhs = condition.rhs,
        .can_fuse_with_branch = condition.can_fuse_with_branch,
        .true_label = condition.true_label,
        .false_label = condition.false_label,
        .source_condition = &condition,
    });
  }
  return branches;
}

[[nodiscard]] CallArgumentRecord build_call_argument_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
  CallArgumentRecord record{
      .arg_index = argument.arg_index,
      .value_bank = argument.value_bank,
      .source_encoding = argument.source_encoding,
      .source_value_id = argument.source_value_id,
      .source_base_value_id = argument.source_base_value_id,
      .source_literal = argument.source_literal,
      .source_symbol_name_id = argument.source_symbol_name_id,
      .source_slot_id = argument.source_slot_id,
      .source_stack_offset_bytes = argument.source_stack_offset_bytes,
      .source_stack_offset_is_prepared_snapshot = argument.source_stack_offset_bytes.has_value(),
      .source_base_value_name = argument.source_base_value_name,
      .source_pointer_byte_delta = argument.source_pointer_byte_delta,
      .destination_register = argument.destination_register_name.has_value()
                                   ? std::string_view{*argument.destination_register_name}
                                   : std::string_view{},
      .destination_register_bank = argument.destination_register_bank,
      .destination_stack_offset_bytes = argument.destination_stack_offset_bytes,
      .destination_stack_offset_is_prepared_snapshot =
          argument.destination_stack_offset_bytes.has_value(),
      .source_argument = &argument,
  };
  if (argument.source_symbol_name_id.has_value()) {
    record.source_symbol_label = prepared.names.link_names.spelling(*argument.source_symbol_name_id);
  } else if (argument.source_symbol_name.has_value()) {
    record.source_symbol_label = *argument.source_symbol_name;
  }
  if (argument.source_base_value_name.has_value()) {
    record.source_base_label =
        c4c::backend::prepare::prepared_value_name(prepared.names,
                                                   *argument.source_base_value_name);
  }
  return record;
}

[[nodiscard]] CallResultRecord build_call_result_record(
    const c4c::backend::prepare::PreparedCallResultPlan& result) {
  return CallResultRecord{
      .value_bank = result.value_bank,
      .source_storage_kind = result.source_storage_kind,
      .destination_storage_kind = result.destination_storage_kind,
      .destination_value_id = result.destination_value_id,
      .source_register = result.source_register_name.has_value()
                              ? std::string_view{*result.source_register_name}
                              : std::string_view{},
      .source_register_bank = result.source_register_bank,
      .source_stack_offset_bytes = result.source_stack_offset_bytes,
      .source_stack_offset_is_prepared_snapshot = result.source_stack_offset_bytes.has_value(),
      .destination_register = result.destination_register_name.has_value()
                                   ? std::string_view{*result.destination_register_name}
                                   : std::string_view{},
      .destination_register_bank = result.destination_register_bank,
      .destination_slot_id = result.destination_slot_id,
      .destination_stack_offset_bytes = result.destination_stack_offset_bytes,
      .destination_stack_offset_is_prepared_snapshot =
          result.destination_stack_offset_bytes.has_value(),
      .source_result = &result,
  };
}

[[nodiscard]] CallPreservedValueRecord build_call_preserved_value_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedCallPreservedValue& preserved) {
  return CallPreservedValueRecord{
      .value_id = preserved.value_id,
      .value_name = preserved.value_name,
      .value_label = c4c::backend::prepare::prepared_value_name(prepared.names,
                                                                preserved.value_name),
      .route = preserved.route,
      .callee_saved_save_index = preserved.callee_saved_save_index,
      .register_name = preserved.register_name.has_value()
                           ? std::string_view{*preserved.register_name}
                           : std::string_view{},
      .register_bank = preserved.register_bank,
      .slot_id = preserved.slot_id,
      .stack_offset_bytes = preserved.stack_offset_bytes,
      .stack_offset_is_prepared_snapshot = preserved.stack_offset_bytes.has_value(),
      .source_preserved_value = &preserved,
  };
}

[[nodiscard]] std::vector<CallRecord> build_call_records(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name) {
  std::vector<CallRecord> calls;
  const auto* call_plans = c4c::backend::prepare::find_prepared_call_plans(prepared,
                                                                           function_name);
  if (call_plans == nullptr) {
    return calls;
  }
  calls.reserve(call_plans->calls.size());
  for (const auto& call : call_plans->calls) {
    CallRecord record{
        .block_index = call.block_index,
        .instruction_index = call.instruction_index,
        .wrapper_kind = call.wrapper_kind,
        .is_indirect = call.is_indirect,
        .direct_callee_name = call.direct_callee_name.has_value()
                                  ? std::string_view{*call.direct_callee_name}
                                  : std::string_view{},
        .source_call = &call,
    };
    if (call.indirect_callee.has_value()) {
      record.indirect_callee_value_name = call.indirect_callee->value_name;
      record.indirect_callee_value_id = call.indirect_callee->value_id;
    }
    if (call.memory_return.has_value()) {
      record.memory_return_slot_id = call.memory_return->slot_id;
      record.memory_return_stack_offset_bytes = call.memory_return->stack_offset_bytes;
    }
    record.arguments.reserve(call.arguments.size());
    for (const auto& argument : call.arguments) {
      record.arguments.push_back(build_call_argument_record(prepared, argument));
    }
    if (call.result.has_value()) {
      record.result = build_call_result_record(*call.result);
    }
    record.preserved_values.reserve(call.preserved_values.size());
    for (const auto& preserved : call.preserved_values) {
      record.preserved_values.push_back(build_call_preserved_value_record(prepared, preserved));
    }
    record.clobbered_registers.reserve(call.clobbered_registers.size());
    for (const auto& clobbered : call.clobbered_registers) {
      record.clobbered_registers.push_back(build_clobbered_register_record(clobbered));
    }
    calls.push_back(std::move(record));
  }
  return calls;
}

[[nodiscard]] MoveRecord build_move_record(
    const c4c::backend::prepare::PreparedMoveBundle& bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move) {
  return MoveRecord{
      .phase = bundle.phase,
      .authority_kind = move.authority_kind,
      .from_value_id = move.from_value_id,
      .to_value_id = move.to_value_id,
      .destination_kind = move.destination_kind,
      .destination_storage_kind = move.destination_storage_kind,
      .destination_abi_index = move.destination_abi_index,
      .destination_register = move.destination_register_name.has_value()
                                  ? std::string_view{*move.destination_register_name}
                                  : std::string_view{},
      .destination_stack_offset_bytes = move.destination_stack_offset_bytes,
      .destination_stack_offset_is_prepared_snapshot =
          move.destination_stack_offset_bytes.has_value(),
      .block_index = move.block_index,
      .instruction_index = move.instruction_index,
      .uses_cycle_temp_source = move.uses_cycle_temp_source,
      .coalesced_by_assigned_storage = move.coalesced_by_assigned_storage,
      .source_parallel_copy_step_index = move.source_parallel_copy_step_index,
      .source_immediate_i32 = move.source_immediate_i32,
      .op_kind = move.op_kind,
      .source_parallel_copy_predecessor_label = move.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = move.source_parallel_copy_successor_label,
      .reason = move.reason,
      .source_bundle = &bundle,
      .source_move = &move,
  };
}

[[nodiscard]] AbiBindingRecord build_abi_binding_record(
    const c4c::backend::prepare::PreparedMoveBundle& bundle,
    const c4c::backend::prepare::PreparedAbiBinding& binding) {
  return AbiBindingRecord{
      .phase = bundle.phase,
      .destination_kind = binding.destination_kind,
      .destination_storage_kind = binding.destination_storage_kind,
      .destination_abi_index = binding.destination_abi_index,
      .destination_register = binding.destination_register_name.has_value()
                                  ? std::string_view{*binding.destination_register_name}
                                  : std::string_view{},
      .destination_stack_offset_bytes = binding.destination_stack_offset_bytes,
      .destination_stack_offset_is_prepared_snapshot =
          binding.destination_stack_offset_bytes.has_value(),
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_bundle = &bundle,
      .source_binding = &binding,
  };
}

void append_value_location_moves(const c4c::backend::prepare::PreparedValueLocationFunction* locations,
                                 std::vector<MoveRecord>& moves,
                                 std::vector<AbiBindingRecord>& abi_bindings) {
  if (locations == nullptr) {
    return;
  }
  for (const auto& bundle : locations->move_bundles) {
    for (const auto& move : bundle.moves) {
      moves.push_back(build_move_record(bundle, move));
    }
    for (const auto& binding : bundle.abi_bindings) {
      abi_bindings.push_back(build_abi_binding_record(bundle, binding));
    }
  }
}

[[nodiscard]] std::vector<SpillReloadRecord> build_spill_reload_records(
    const c4c::backend::prepare::PreparedRegallocFunction* regalloc) {
  std::vector<SpillReloadRecord> records;
  if (regalloc == nullptr) {
    return records;
  }
  records.reserve(regalloc->spill_reload_ops.size());
  for (const auto& op : regalloc->spill_reload_ops) {
    records.push_back(SpillReloadRecord{
        .value_id = op.value_id,
        .op_kind = op.op_kind,
        .block_index = op.block_index,
        .instruction_index = op.instruction_index,
        .register_bank = op.register_bank,
        .register_name = op.register_name.has_value() ? std::string_view{*op.register_name}
                                                       : std::string_view{},
        .slot_id = op.slot_id,
        .stack_offset_bytes = op.stack_offset_bytes,
        .stack_offset_is_prepared_snapshot = op.stack_offset_bytes.has_value(),
        .source_spill_reload = &op,
    });
  }
  return records;
}

[[nodiscard]] std::vector<ParallelCopyRecord> build_parallel_copy_records(
    const c4c::backend::prepare::PreparedControlFlowFunction& function) {
  std::vector<ParallelCopyRecord> records;
  records.reserve(function.parallel_copy_bundles.size());
  for (const auto& bundle : function.parallel_copy_bundles) {
    records.push_back(ParallelCopyRecord{
        .predecessor_label = bundle.predecessor_label,
        .successor_label = bundle.successor_label,
        .execution_site = bundle.execution_site,
        .execution_block_label = bundle.execution_block_label,
        .has_cycle = bundle.has_cycle,
        .move_count = bundle.moves.size(),
        .step_count = bundle.steps.size(),
        .source_bundle = &bundle,
    });
  }
  return records;
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
  const auto* locations =
      c4c::backend::prepare::find_prepared_value_location_function(prepared,
                                                                   function.function_name);
  const auto* regalloc = find_prepared_regalloc_function(prepared.regalloc,
                                                         function.function_name);
  record.frame = build_frame_record(prepared, function.function_name);
  record.branches = build_branch_records(prepared, function);
  record.calls = build_call_records(prepared, function.function_name);
  append_value_location_moves(locations, record.moves, record.abi_bindings);
  if (regalloc != nullptr) {
    for (const auto& move : regalloc->move_resolution) {
      record.moves.push_back(MoveRecord{
          .authority_kind = move.authority_kind,
          .from_value_id = move.from_value_id,
          .to_value_id = move.to_value_id,
          .destination_kind = move.destination_kind,
          .destination_storage_kind = move.destination_storage_kind,
          .destination_abi_index = move.destination_abi_index,
          .destination_register = move.destination_register_name.has_value()
                                      ? std::string_view{*move.destination_register_name}
                                      : std::string_view{},
          .destination_stack_offset_bytes = move.destination_stack_offset_bytes,
          .destination_stack_offset_is_prepared_snapshot =
              move.destination_stack_offset_bytes.has_value(),
          .block_index = move.block_index,
          .instruction_index = move.instruction_index,
          .uses_cycle_temp_source = move.uses_cycle_temp_source,
          .coalesced_by_assigned_storage = move.coalesced_by_assigned_storage,
          .source_parallel_copy_step_index = move.source_parallel_copy_step_index,
          .source_immediate_i32 = move.source_immediate_i32,
          .op_kind = move.op_kind,
          .source_parallel_copy_predecessor_label = move.source_parallel_copy_predecessor_label,
          .source_parallel_copy_successor_label = move.source_parallel_copy_successor_label,
          .reason = move.reason,
          .source_move = &move,
      });
    }
  }
  record.spill_reloads = build_spill_reload_records(regalloc);
  record.parallel_copies = build_parallel_copy_records(function);
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

[[nodiscard]] std::string_view link_label(
    const c4c::backend::bir::NameTables& names,
    c4c::LinkNameId id,
    std::string_view fallback) {
  if (id != c4c::kInvalidLinkName) {
    const std::string_view label = names.link_names.spelling(id);
    if (!label.empty()) {
      return label;
    }
  }
  return fallback;
}

[[nodiscard]] std::string_view text_label(
    const c4c::backend::bir::NameTables& names,
    c4c::TextId id,
    std::string_view fallback) {
  if (id != c4c::kInvalidText) {
    const std::string_view label = names.texts.lookup(id);
    if (!label.empty()) {
      return label;
    }
  }
  return fallback;
}

[[nodiscard]] SymbolVisibilityRecordKind visibility_for_global(
    const c4c::backend::bir::Global& global) {
  return global.is_extern ? SymbolVisibilityRecordKind::ExternalDeclaration
                          : SymbolVisibilityRecordKind::LinkVisibleDefinition;
}

[[nodiscard]] std::optional<DataRelocationNeedRecord> relocation_need_for_value(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Global& owner,
    std::size_t global_index,
    DataRelocationNeedKind kind,
    const c4c::backend::bir::Value& value,
    std::optional<std::size_t> initializer_element_index) {
  if (value.pointer_symbol_link_name_id == c4c::kInvalidLinkName) {
    return std::nullopt;
  }
  return DataRelocationNeedRecord{
      .kind = kind,
      .global_index = global_index,
      .owner_link_name = owner.link_name_id,
      .owner_label = link_label(prepared.module.names, owner.link_name_id, owner.name),
      .target_link_name = value.pointer_symbol_link_name_id,
      .target_label = link_label(prepared.module.names, value.pointer_symbol_link_name_id,
                                 value.name),
      .initializer_element_index = initializer_element_index,
      .initializer_element = value,
      .source_global = &owner,
  };
}

[[nodiscard]] DataRelocationNeedRecord relocation_need_for_initializer_symbol(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Global& owner,
    std::size_t global_index) {
  const std::string_view fallback =
      owner.initializer_symbol_name.has_value() ? std::string_view{*owner.initializer_symbol_name}
                                                : std::string_view{};
  return DataRelocationNeedRecord{
      .kind = DataRelocationNeedKind::InitializerSymbol,
      .global_index = global_index,
      .owner_link_name = owner.link_name_id,
      .owner_label = link_label(prepared.module.names, owner.link_name_id, owner.name),
      .target_link_name = owner.initializer_symbol_name_id,
      .target_label = link_label(prepared.module.names, owner.initializer_symbol_name_id, fallback),
      .initializer_element_index = std::nullopt,
      .initializer_element = std::nullopt,
      .source_global = &owner,
  };
}

[[nodiscard]] GlobalDataRecord build_global_data_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::Global& global,
    std::size_t global_index) {
  GlobalDataRecord record{
      .global_index = global_index,
      .label = link_label(prepared.module.names, global.link_name_id, global.name),
      .link_name = global.link_name_id,
      .type = global.type,
      .visibility = visibility_for_global(global),
      .is_extern = global.is_extern,
      .is_thread_local = global.is_thread_local,
      .is_constant = global.is_constant,
      .size_bytes = global.size_bytes,
      .align_bytes = global.align_bytes,
      .initializer = global.initializer,
      .initializer_symbol_label =
          global.initializer_symbol_name.has_value()
              ? std::optional<std::string_view>{
                    link_label(prepared.module.names,
                               global.initializer_symbol_name_id,
                               *global.initializer_symbol_name)}
              : std::nullopt,
      .initializer_symbol_name_id = global.initializer_symbol_name_id,
      .initializer_elements = global.initializer_elements,
      .source_global = &global,
  };

  if (global.initializer_symbol_name.has_value() ||
      global.initializer_symbol_name_id != c4c::kInvalidLinkName) {
    record.relocation_needs.push_back(
        relocation_need_for_initializer_symbol(prepared, global, global_index));
  }
  if (global.initializer.has_value()) {
    if (auto need = relocation_need_for_value(prepared,
                                              global,
                                              global_index,
                                              DataRelocationNeedKind::InitializerSymbol,
                                              *global.initializer,
                                              std::nullopt)) {
      record.relocation_needs.push_back(*need);
    }
  }
  for (std::size_t index = 0; index < global.initializer_elements.size(); ++index) {
    if (auto need = relocation_need_for_value(prepared,
                                              global,
                                              global_index,
                                              DataRelocationNeedKind::InitializerElementSymbol,
                                              global.initializer_elements[index],
                                              index)) {
      record.relocation_needs.push_back(*need);
    }
  }
  return record;
}

[[nodiscard]] std::vector<GlobalDataRecord> build_global_data_records(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  std::vector<GlobalDataRecord> globals;
  globals.reserve(prepared.module.globals.size());
  for (std::size_t index = 0; index < prepared.module.globals.size(); ++index) {
    globals.push_back(build_global_data_record(prepared, prepared.module.globals[index], index));
  }
  return globals;
}

[[nodiscard]] StringDataRecord build_string_data_record(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::bir::StringConstant& string,
    std::size_t string_index) {
  return StringDataRecord{
      .string_index = string_index,
      .label = text_label(prepared.module.names, string.name_id, string.name),
      .name_id = string.name_id,
      .bytes = string.bytes,
      .align_bytes = string.align_bytes,
      .source_string = &string,
  };
}

[[nodiscard]] std::vector<StringDataRecord> build_string_data_records(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  std::vector<StringDataRecord> strings;
  strings.reserve(prepared.module.string_constants.size());
  for (std::size_t index = 0; index < prepared.module.string_constants.size(); ++index) {
    strings.push_back(
        build_string_data_record(prepared, prepared.module.string_constants[index], index));
  }
  return strings;
}

[[nodiscard]] std::vector<DataRelocationNeedRecord> collect_relocation_needs(
    const std::vector<GlobalDataRecord>& globals) {
  std::vector<DataRelocationNeedRecord> needs;
  for (const auto& global : globals) {
    needs.insert(needs.end(), global.relocation_needs.begin(), global.relocation_needs.end());
  }
  return needs;
}

}  // namespace

BuildResult build(const c4c::backend::prepare::PreparedBirModule& prepared) {
  const c4c::TargetProfile target_profile =
      c4c::backend::aarch64::abi::resolve_target_profile(prepared);
  if (auto error = c4c::backend::aarch64::abi::validate_prepared_module_handoff(prepared)) {
    return BuildResult{.module = std::nullopt, .error = std::move(error)};
  }
  auto globals = build_global_data_records(prepared);
  auto strings = build_string_data_records(prepared);
  auto relocation_needs = collect_relocation_needs(globals);
  return BuildResult{.module = Module{.prepared = &prepared,
                                      .target_profile = target_profile,
                                      .globals = std::move(globals),
                                      .strings = std::move(strings),
                                      .relocation_needs = std::move(relocation_needs),
                                      .functions = build_function_records(prepared)},
                     .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::module
