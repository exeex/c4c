#include "call_plans.hpp"
#include "regalloc/call_return_abi.hpp"
#include "target_register_profile.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] PreparedRegisterBank register_bank_from_class(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return PreparedRegisterBank::Gpr;
    case PreparedRegisterClass::Float:
      return PreparedRegisterBank::Fpr;
    case PreparedRegisterClass::Vector:
      return PreparedRegisterBank::Vreg;
    case PreparedRegisterClass::AggregateAddress:
      return PreparedRegisterBank::AggregateAddress;
    case PreparedRegisterClass::None:
      return PreparedRegisterBank::None;
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] PreparedRegisterBank register_bank_from_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return PreparedRegisterBank::Gpr;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      return PreparedRegisterBank::Fpr;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return PreparedRegisterBank::Vreg;
    case bir::TypeKind::Void:
      return PreparedRegisterBank::None;
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] PreparedRegisterBank register_bank_from_arg_abi(const bir::CallArgAbiInfo& abi) {
  switch (abi.primary_class) {
    case bir::AbiValueClass::Sse:
      if (abi.type == bir::TypeKind::F128) {
        return PreparedRegisterBank::Vreg;
      }
      return PreparedRegisterBank::Fpr;
    case bir::AbiValueClass::Memory:
      return abi.type == bir::TypeKind::Ptr ? PreparedRegisterBank::AggregateAddress
                                            : register_bank_from_type(abi.type);
    case bir::AbiValueClass::Integer:
      return register_bank_from_type(abi.type);
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] PreparedRegisterBank register_bank_from_result_abi(
    const bir::CallResultAbiInfo& abi) {
  switch (abi.primary_class) {
    case bir::AbiValueClass::Sse:
      return PreparedRegisterBank::Fpr;
    case bir::AbiValueClass::Memory:
      return abi.type == bir::TypeKind::Ptr ? PreparedRegisterBank::AggregateAddress
                                            : register_bank_from_type(abi.type);
    case bir::AbiValueClass::Integer:
      return register_bank_from_type(abi.type);
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] std::optional<PreparedSpillSlotPlacement> make_spill_slot_placement(
    std::optional<PreparedFrameSlotId> slot_id,
    std::optional<std::size_t> offset_bytes) {
  if (!slot_id.has_value() || !offset_bytes.has_value()) {
    return std::nullopt;
  }
  return PreparedSpillSlotPlacement{
      .slot_id = *slot_id,
      .offset_bytes = *offset_bytes,
  };
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> find_register_pool_placement(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names,
    bool caller_pool) {
  if (occupied_register_names.empty()) {
    return std::nullopt;
  }
  const auto spans = caller_pool ? caller_saved_register_spans(target_profile, reg_class, contiguous_width)
                                 : callee_saved_register_spans(target_profile, reg_class, contiguous_width);
  for (const auto& span : spans) {
    if (span.occupied_register_names == occupied_register_names) {
      return span.placement;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> find_register_placement(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names) {
  if (const auto callee_saved =
          find_register_pool_placement(target_profile, reg_class, contiguous_width, occupied_register_names, false);
      callee_saved.has_value()) {
    return callee_saved;
  }
  return find_register_pool_placement(target_profile, reg_class, contiguous_width, occupied_register_names, true);
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> assignment_register_placement(
    const c4c::TargetProfile& target_profile,
    const PreparedPhysicalRegisterAssignment& assignment) {
  if (assignment.placement.has_value()) {
    return assignment.placement;
  }
  return find_register_placement(target_profile,
                                 assignment.reg_class,
                                 assignment.contiguous_width,
                                 assignment.occupied_register_names);
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> as_reserved_scratch_placement(
    std::optional<PreparedRegisterPlacement> placement) {
  if (!placement.has_value()) {
    return std::nullopt;
  }
  placement->pool = PreparedRegisterSlotPool::ReservedScratch;
  return placement;
}

[[nodiscard]] std::optional<ValueNameId> maybe_named_value_id(const PreparedNameTables& names,
                                                              const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const ValueNameId id = names.value_names.find(value.name);
  if (id == kInvalidValueName) {
    return std::nullopt;
  }
  return id;
}

[[nodiscard]] std::size_t aarch64_byval_register_lane_width(
    const c4c::TargetProfile& target_profile,
    const bir::CallArgAbiInfo& abi) {
  if (target_profile.arch == c4c::TargetArch::Aarch64 &&
      abi.type == bir::TypeKind::Ptr &&
      abi.byval_copy &&
      abi.passed_in_register &&
      !abi.passed_on_stack &&
      abi.primary_class == bir::AbiValueClass::Integer &&
      abi.size_bytes > 0 &&
      abi.size_bytes <= 16) {
    return std::max<std::size_t>((abi.size_bytes + 7) / 8, 1);
  }
  return 1;
}

[[nodiscard]] const PreparedRegallocFunction* find_regalloc_function(
    const PreparedRegalloc& regalloc,
    FunctionNameId function_name) {
  for (const auto& function : regalloc.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] const bir::Function* find_module_function(const bir::Module& module,
                                                        std::string_view function_name) {
  for (const auto& function : module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] const bir::Function* find_module_function_by_link_name_id(
    const bir::Module& module,
    LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) {
    return nullptr;
  }
  for (const auto& function : module.functions) {
    if (function.link_name_id == link_name_id) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] bool module_has_symbol_link_name_id(const bir::Module& module,
                                                  LinkNameId link_name_id) {
  if (link_name_id == kInvalidLinkName) {
    return false;
  }
  for (const auto& global : module.globals) {
    if (global.link_name_id == link_name_id) {
      return true;
    }
  }
  return find_module_function_by_link_name_id(module, link_name_id) != nullptr;
}

[[nodiscard]] std::optional<std::string_view> resolve_symbol_pointer_name(
    const bir::Module& module,
    const bir::Value& value) {
  if (value.pointer_symbol_link_name_id == kInvalidLinkName) {
    if (value.kind == bir::Value::Kind::Named && !value.name.empty() && value.name.front() == '@') {
      return value.name;
    }
    return std::nullopt;
  }

  const std::string_view semantic_name =
      module.names.link_names.spelling(value.pointer_symbol_link_name_id);
  if (semantic_name.empty() || !module_has_symbol_link_name_id(module, value.pointer_symbol_link_name_id)) {
    return std::nullopt;
  }
  std::string_view raw_name = value.name;
  if (!raw_name.empty() && raw_name.front() == '@') {
    raw_name.remove_prefix(1);
  }
  if (!raw_name.empty() && raw_name != semantic_name) {
    return std::nullopt;
  }
  return semantic_name;
}

[[nodiscard]] std::string display_symbol_pointer_name(std::string_view semantic_name) {
  if (!semantic_name.empty() && semantic_name.front() == '@') {
    return std::string(semantic_name);
  }
  return "@" + std::string(semantic_name);
}

struct DirectCalleeResolution {
  std::optional<std::string_view> name;
  const bir::Function* function = nullptr;
};

[[nodiscard]] DirectCalleeResolution resolve_direct_callee(const bir::Module& module,
                                                           const bir::CallInst& call) {
  if (call.is_indirect) {
    return {};
  }

  if (call.callee_link_name_id != kInvalidLinkName) {
    const std::string_view semantic_name =
        module.names.link_names.spelling(call.callee_link_name_id);
    if (semantic_name.empty()) {
      return {};
    }
    if (!call.callee.empty() && call.callee != semantic_name) {
      return {};
    }
    return DirectCalleeResolution{
        .name = semantic_name,
        .function = find_module_function_by_link_name_id(module, call.callee_link_name_id),
    };
  }

  if (call.callee.empty()) {
    return {};
  }
  return DirectCalleeResolution{
      .name = call.callee,
      .function = find_module_function(module, call.callee),
  };
}

[[nodiscard]] PreparedCallWrapperKind classify_call_wrapper_kind(
    const bir::CallInst& call,
    const DirectCalleeResolution& direct_callee) {
  if (call.is_indirect) {
    return PreparedCallWrapperKind::Indirect;
  }
  if (!direct_callee.name.has_value()) {
    return PreparedCallWrapperKind::Indirect;
  }

  if (const auto* callee = direct_callee.function; callee != nullptr) {
    if (!callee->is_declaration) {
      return PreparedCallWrapperKind::SameModule;
    }
    if (callee->is_variadic || call.is_variadic) {
      return PreparedCallWrapperKind::DirectExternVariadic;
    }
    return PreparedCallWrapperKind::DirectExternFixedArity;
  }

  return call.is_variadic ? PreparedCallWrapperKind::DirectExternVariadic
                          : PreparedCallWrapperKind::DirectExternFixedArity;
}

[[nodiscard]] std::size_t variadic_fpr_arg_register_count(const bir::CallInst& call) {
  if (!call.is_variadic) {
    return 0;
  }

  std::size_t count = 0;
  for (const auto& arg_abi : call.arg_abi) {
    if (arg_abi.passed_in_register && arg_abi.primary_class == bir::AbiValueClass::Sse) {
      ++count;
    }
  }
  return count;
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value_by_name(
    const PreparedRegallocFunction& function,
    ValueNameId value_name) {
  for (const auto& value : function.values) {
    if (value.value_name == value_name) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedRegallocValue* find_f128_constant_regalloc_value(
    const PreparedRegallocFunction& function,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate ||
      value.type != bir::TypeKind::F128 ||
      !value.f128_payload.has_value()) {
    return nullptr;
  }
  for (const auto& regalloc_value : function.values) {
    if (regalloc_value.type == bir::TypeKind::F128 &&
        regalloc_value.constant_f128_payload == value.f128_payload) {
      return &regalloc_value;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedLivenessFunction* find_liveness_function(
    const PreparedLiveness& liveness,
    FunctionNameId function_name) {
  for (const auto& function : liveness.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::size_t> find_call_program_point(
    const PreparedLivenessFunction& function,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (const auto& block : function.blocks) {
    if (block.block_index != block_index) {
      continue;
    }
    return block.start_point + instruction_index;
  }
  return std::nullopt;
}

[[nodiscard]] PreparedMoveStorageKind move_storage_kind_from_home(
    const PreparedValueHome& home) {
  switch (home.kind) {
    case PreparedValueHomeKind::Register:
      return PreparedMoveStorageKind::Register;
    case PreparedValueHomeKind::StackSlot:
      return PreparedMoveStorageKind::StackSlot;
    case PreparedValueHomeKind::None:
    case PreparedValueHomeKind::RematerializableImmediate:
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedMoveStorageKind::None;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] PreparedStorageEncodingKind storage_encoding_from_home(
    const PreparedValueHome& home) {
  switch (home.kind) {
    case PreparedValueHomeKind::Register:
      return PreparedStorageEncodingKind::Register;
    case PreparedValueHomeKind::StackSlot:
      return PreparedStorageEncodingKind::FrameSlot;
    case PreparedValueHomeKind::RematerializableImmediate:
      return PreparedStorageEncodingKind::Immediate;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedStorageEncodingKind::ComputedAddress;
    case PreparedValueHomeKind::None:
      return PreparedStorageEncodingKind::None;
  }
  return PreparedStorageEncodingKind::None;
}

[[nodiscard]] PreparedRegisterBank published_bank_for_value(
    const PreparedRegallocFunction* regalloc_function,
    ValueNameId value_name,
    bir::TypeKind type) {
  if (regalloc_function != nullptr) {
    if (const auto* regalloc_value = find_regalloc_value_by_name(*regalloc_function, value_name);
        regalloc_value != nullptr) {
      return register_bank_from_class(regalloc_value->register_class);
    }
  }
  return register_bank_from_type(type);
}

[[nodiscard]] std::optional<PreparedIndirectCalleePlan> build_indirect_callee_plan(
    const PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    const bir::CallInst& call) {
  if (!call.is_indirect || !call.callee_value.has_value()) {
    return std::nullopt;
  }

  const auto value_name_id = maybe_named_value_id(names, *call.callee_value);
  if (!value_name_id.has_value()) {
    return std::nullopt;
  }

  PreparedIndirectCalleePlan plan{
      .value_name = *value_name_id,
      .value_id = std::nullopt,
      .encoding = PreparedStorageEncodingKind::None,
      .bank = published_bank_for_value(regalloc_function, *value_name_id, call.callee_value->type),
      .register_name = std::nullopt,
      .slot_id = std::nullopt,
      .stack_offset_bytes = std::nullopt,
      .immediate_i32 = std::nullopt,
      .pointer_base_value_name = std::nullopt,
      .pointer_byte_delta = std::nullopt,
      .register_placement = std::nullopt,
  };

  if (value_locations == nullptr) {
    return plan;
  }

  if (const auto* home = find_prepared_value_home(*value_locations, *value_name_id); home != nullptr) {
    plan.value_id = home->value_id;
    plan.encoding = storage_encoding_from_home(*home);
    plan.register_name = home->register_name;
    plan.slot_id = home->slot_id;
    plan.stack_offset_bytes = home->offset_bytes;
    plan.immediate_i32 = home->immediate_i32;
    plan.pointer_base_value_name = home->pointer_base_value_name;
    plan.pointer_byte_delta = home->pointer_byte_delta;
    if (home->register_name.has_value() && regalloc_function != nullptr) {
      if (const auto* regalloc_value =
              find_regalloc_value_by_name(*regalloc_function, *value_name_id);
          regalloc_value != nullptr && regalloc_value->assigned_register.has_value() &&
          home->register_name ==
              std::optional<std::string>{regalloc_value->assigned_register->register_name}) {
        plan.register_placement =
            assignment_register_placement(target_profile, *regalloc_value->assigned_register);
      }
    }
  }

  return plan;
}

[[nodiscard]] std::optional<PreparedMemoryReturnPlan> build_memory_return_plan(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedStackLayout& stack_layout,
    FunctionNameId function_name,
    const bir::CallInst& call) {
  if (!call.result_abi.has_value() || !call.result_abi->returned_in_memory ||
      !call.sret_storage_name.has_value()) {
    return std::nullopt;
  }

  PreparedMemoryReturnPlan plan{
      .sret_arg_index = std::nullopt,
      .storage_slot_name = kInvalidSlotName,
      .encoding = PreparedStorageEncodingKind::None,
      .slot_id = std::nullopt,
      .stack_offset_bytes = std::nullopt,
      .size_bytes = 0,
      .align_bytes = 0,
  };

  for (std::size_t arg_index = 0; arg_index < call.arg_abi.size(); ++arg_index) {
    if (call.arg_abi[arg_index].sret_pointer) {
      plan.sret_arg_index = arg_index;
      plan.size_bytes = call.arg_abi[arg_index].size_bytes;
      plan.align_bytes = call.arg_abi[arg_index].align_bytes;
      break;
    }
  }

  const std::string_view sret_storage_spelling =
      call.sret_storage_name_id == kInvalidSlotName
          ? std::string_view(*call.sret_storage_name)
          : bir_names.slot_names.spelling(call.sret_storage_name_id);
  const SlotNameId slot_name_id = names.slot_names.find(sret_storage_spelling);
  if (slot_name_id == kInvalidSlotName) {
    return plan;
  }
  plan.storage_slot_name = slot_name_id;

  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function_name || object.slot_name != slot_name_id) {
      continue;
    }
    if (plan.size_bytes == 0) {
      plan.size_bytes = object.size_bytes;
    }
    if (plan.align_bytes == 0) {
      plan.align_bytes = object.align_bytes;
    }
    if (const auto* frame_slot = find_prepared_frame_slot(stack_layout, object.object_id);
        frame_slot != nullptr) {
      plan.encoding = PreparedStorageEncodingKind::FrameSlot;
      plan.slot_id = frame_slot->slot_id;
      plan.stack_offset_bytes = frame_slot->offset_bytes;
    }
    break;
  }

  return plan;
}

[[nodiscard]] const PreparedAbiBinding* find_call_abi_binding(
    const PreparedMoveBundle* move_bundle,
    PreparedMoveDestinationKind destination_kind,
    std::optional<std::size_t> destination_abi_index) {
  if (move_bundle == nullptr) {
    return nullptr;
  }
  const PreparedAbiBinding* preferred = nullptr;
  for (const auto& binding : move_bundle->abi_bindings) {
    if (binding.destination_kind != destination_kind ||
        binding.destination_abi_index != destination_abi_index) {
      continue;
    }
    if (binding.destination_storage_kind == PreparedMoveStorageKind::StackSlot) {
      return &binding;
    }
    if (preferred == nullptr) {
      preferred = &binding;
    }
  }
  return preferred;
}

[[nodiscard]] const PreparedAbiBinding* find_call_register_abi_binding(
    const PreparedMoveBundle* move_bundle,
    PreparedMoveDestinationKind destination_kind,
    std::optional<std::size_t> destination_abi_index) {
  if (move_bundle == nullptr) {
    return nullptr;
  }
  for (const auto& binding : move_bundle->abi_bindings) {
    if (binding.destination_kind == destination_kind &&
        binding.destination_storage_kind == PreparedMoveStorageKind::Register &&
        binding.destination_abi_index == destination_abi_index &&
        binding.destination_register_name.has_value()) {
      return &binding;
    }
  }
  return nullptr;
}

[[nodiscard]] std::vector<PreparedClobberedRegister> build_call_clobber_set(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function) {
  std::vector<PreparedClobberedRegister> clobbers;
  auto append_spans_for_width = [&](PreparedRegisterClass reg_class, std::size_t contiguous_width) {
    const PreparedRegisterBank bank = register_bank_from_class(reg_class);
    for (const auto& register_span :
         caller_saved_register_spans(target_profile, reg_class, contiguous_width)) {
      const auto duplicate = std::find_if(
          clobbers.begin(),
          clobbers.end(),
          [&](const PreparedClobberedRegister& clobber) {
            return clobber.bank == bank &&
                   clobber.occupied_register_names == register_span.occupied_register_names;
          });
      if (duplicate != clobbers.end()) {
        continue;
      }
      clobbers.push_back(PreparedClobberedRegister{
          .bank = bank,
          .register_name = register_span.register_name,
          .contiguous_width = register_span.contiguous_width,
          .occupied_register_names = register_span.occupied_register_names,
          .placement = as_reserved_scratch_placement(register_span.placement),
      });
    }
  };
  for (PreparedRegisterClass reg_class :
       {PreparedRegisterClass::General, PreparedRegisterClass::Float, PreparedRegisterClass::Vector}) {
    append_spans_for_width(reg_class, 1);
    if (regalloc_function == nullptr) {
      continue;
    }
    for (const auto& value : regalloc_function->values) {
      if (value.register_class != reg_class || value.register_group_width <= 1) {
        continue;
      }
      append_spans_for_width(reg_class, value.register_group_width);
    }
  }
  return clobbers;
}

[[nodiscard]] bool is_callee_saved_register_assignment(const c4c::TargetProfile& target_profile,
                                                       const PreparedRegallocValue& value) {
  if (!value.assigned_register.has_value()) {
    return false;
  }
  for (const auto& span :
       callee_saved_register_spans(target_profile, value.register_class, value.register_group_width)) {
    if (span.occupied_register_names == value.assigned_register->occupied_register_names) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<std::size_t> find_saved_callee_save_index(
    const PreparedFramePlanFunction* frame_plan,
    const PreparedRegallocValue& value) {
  if (frame_plan == nullptr || !value.assigned_register.has_value()) {
    return std::nullopt;
  }
  for (const auto& saved : frame_plan->saved_callee_registers) {
    if (saved.occupied_register_names == value.assigned_register->occupied_register_names &&
        !saved.occupied_register_names.empty()) {
      return saved.save_index;
    }
    if (saved.register_name == value.assigned_register->register_name) {
      return saved.save_index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    std::size_t program_point,
    bool require_call_crossing_value) {
  std::vector<PreparedCallPreservedValue> preserved_values;
  if (liveness_function == nullptr || regalloc_function == nullptr) {
    return preserved_values;
  }

  for (const auto& value : regalloc_function->values) {
    if ((require_call_crossing_value && !value.crosses_call) ||
        !value.live_interval.has_value()) {
      continue;
    }
    if (!(program_point > value.live_interval->start_point &&
          program_point < value.live_interval->end_point)) {
      continue;
    }

    PreparedCallPreservedValue preserved{
        .value_id = value.value_id,
        .value_name = value.value_name,
        .route = PreparedCallPreservationRoute::Unknown,
        .callee_saved_save_index = std::nullopt,
        .contiguous_width = std::max<std::size_t>(value.register_group_width, 1),
        .register_name = std::nullopt,
        .register_bank = std::nullopt,
        .occupied_register_names = {},
        .slot_id = std::nullopt,
        .stack_offset_bytes = std::nullopt,
        .stack_size_bytes = std::nullopt,
        .stack_align_bytes = std::nullopt,
        .register_placement = std::nullopt,
        .spill_slot_placement = std::nullopt,
    };

    const PreparedValueHome* value_home =
        value_locations == nullptr ? nullptr : find_prepared_value_home(*value_locations, value.value_id);
    if (value_home != nullptr && value_home->kind == PreparedValueHomeKind::StackSlot) {
      preserved.route = PreparedCallPreservationRoute::StackSlot;
      preserved.slot_id = value_home->slot_id;
      preserved.stack_offset_bytes = value_home->offset_bytes;
      preserved.stack_size_bytes = value_home->size_bytes;
      preserved.stack_align_bytes = value_home->align_bytes;
      preserved.spill_slot_placement =
          make_spill_slot_placement(value_home->slot_id, value_home->offset_bytes);
    } else if (value.stack_object_id.has_value()) {
      if (const auto* frame_slot =
              find_prepared_frame_slot(prepared.stack_layout, *value.stack_object_id);
          frame_slot != nullptr) {
        preserved.route = PreparedCallPreservationRoute::StackSlot;
        preserved.slot_id = frame_slot->slot_id;
        preserved.stack_offset_bytes = frame_slot->offset_bytes;
        preserved.stack_size_bytes = frame_slot->size_bytes;
        preserved.stack_align_bytes = frame_slot->align_bytes;
        preserved.spill_slot_placement = PreparedSpillSlotPlacement{
            .slot_id = frame_slot->slot_id,
            .offset_bytes = frame_slot->offset_bytes,
        };
      }
    } else if (value.assigned_stack_slot.has_value()) {
      preserved.route = PreparedCallPreservationRoute::StackSlot;
      preserved.slot_id = value.assigned_stack_slot->slot_id;
      preserved.stack_offset_bytes = value.assigned_stack_slot->offset_bytes;
      preserved.stack_size_bytes = value.assigned_stack_slot->size_bytes;
      preserved.stack_align_bytes = value.assigned_stack_slot->align_bytes;
      preserved.spill_slot_placement =
          make_spill_slot_placement(preserved.slot_id, preserved.stack_offset_bytes);
    } else if (value.assigned_register.has_value()) {
      preserved.register_name = value.assigned_register->register_name;
      preserved.register_bank = register_bank_from_class(value.register_class);
      preserved.contiguous_width = value.assigned_register->contiguous_width;
      preserved.occupied_register_names = value.assigned_register->occupied_register_names;
      preserved.register_placement =
          assignment_register_placement(prepared.target_profile, *value.assigned_register);
      if (is_callee_saved_register_assignment(prepared.target_profile, value)) {
        preserved.route = PreparedCallPreservationRoute::CalleeSavedRegister;
        preserved.callee_saved_save_index = find_saved_callee_save_index(frame_plan, value);
      }
    }

    preserved_values.push_back(std::move(preserved));
  }

  std::sort(preserved_values.begin(),
            preserved_values.end(),
            [](const PreparedCallPreservedValue& lhs, const PreparedCallPreservedValue& rhs) {
              return lhs.value_id < rhs.value_id;
            });
  return preserved_values;
}

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (liveness_function == nullptr) {
    return {};
  }
  const auto call_point =
      find_call_program_point(*liveness_function, block_index, instruction_index);
  if (!call_point.has_value()) {
    return {};
  }
  return build_call_preserved_values(prepared,
                                     frame_plan,
                                     liveness_function,
                                     regalloc_function,
                                     value_locations,
                                     *call_point,
                                     true);
}

}  // namespace

void populate_call_plans(PreparedBirModule& prepared) {
  prepared.call_plans.functions.clear();

  for (const auto& function : prepared.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    const FunctionNameId function_name_id = prepared.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    PreparedCallPlansFunction function_plan{
        .function_name = function_name_id,
        .calls = {},
    };
    const auto* frame_plan = find_prepared_frame_plan(prepared, function_name_id);
    const auto* regalloc_function = find_regalloc_function(prepared.regalloc, function_name_id);
    const auto* liveness_function = find_liveness_function(prepared.liveness, function_name_id);
    const auto* value_locations = find_prepared_value_location_function(prepared, function_name_id);
    const std::vector<PreparedClobberedRegister> call_clobbers =
        build_call_clobber_set(prepared.target_profile, regalloc_function);

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
        const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
        if (call == nullptr) {
          continue;
        }

        const DirectCalleeResolution direct_callee = resolve_direct_callee(prepared.module, *call);
        PreparedCallPlan call_plan{
            .block_index = block_index,
            .instruction_index = instruction_index,
            .wrapper_kind = classify_call_wrapper_kind(*call, direct_callee),
            .variadic_fpr_arg_register_count = variadic_fpr_arg_register_count(*call),
            .is_indirect = call->is_indirect,
            .direct_callee_name =
                direct_callee.name.has_value()
                    ? std::optional<std::string>{std::string{*direct_callee.name}}
                    : std::nullopt,
            .indirect_callee =
                build_indirect_callee_plan(prepared.names,
                                           prepared.target_profile,
                                           regalloc_function,
                                           value_locations,
                                           *call),
            .memory_return =
                build_memory_return_plan(prepared.names,
                                         prepared.module.names,
                                         prepared.stack_layout,
                                         function_name_id,
                                         *call),
            .arguments = {},
            .result = std::nullopt,
            .preserved_values = build_call_preserved_values(
                prepared,
                frame_plan,
                liveness_function,
                regalloc_function,
                value_locations,
                block_index,
                instruction_index),
            .clobbered_registers = call_clobbers,
        };

        const PreparedMoveBundle* before_call_bundle =
            value_locations == nullptr
                ? nullptr
                : find_prepared_move_bundle(
                      *value_locations, PreparedMovePhase::BeforeCall, block_index, instruction_index);
        const PreparedMoveBundle* after_call_bundle =
            value_locations == nullptr
                ? nullptr
                : find_prepared_move_bundle(
                      *value_locations, PreparedMovePhase::AfterCall, block_index, instruction_index);

        for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
          PreparedCallArgumentPlan arg_plan{
              .instruction_index = instruction_index,
              .arg_index = arg_index,
              .value_bank = arg_index < call->arg_abi.size()
                                ? register_bank_from_arg_abi(call->arg_abi[arg_index])
                                : register_bank_from_type(call->arg_types[arg_index]),
              .source_encoding = PreparedStorageEncodingKind::None,
              .source_value_id = std::nullopt,
              .source_base_value_id = std::nullopt,
              .source_literal = std::nullopt,
              .source_symbol_name = std::nullopt,
              .source_symbol_name_id = std::nullopt,
              .source_register_name = std::nullopt,
              .source_slot_id = std::nullopt,
              .source_stack_offset_bytes = std::nullopt,
              .source_register_bank = std::nullopt,
              .source_base_value_name = std::nullopt,
              .source_pointer_byte_delta = std::nullopt,
              .destination_register_name = std::nullopt,
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {},
              .destination_register_bank = std::nullopt,
              .destination_stack_offset_bytes = std::nullopt,
              .source_register_placement = std::nullopt,
              .destination_register_placement = std::nullopt,
          };
          const auto abi_register_index =
              regalloc_detail::call_arg_abi_register_index(
                  prepared.target_profile, *call, arg_index);

          if (const auto* binding = find_call_abi_binding(before_call_bundle,
                                                          PreparedMoveDestinationKind::CallArgumentAbi,
                                                          arg_index);
              binding != nullptr) {
            arg_plan.destination_register_name = binding->destination_register_name;
            arg_plan.destination_contiguous_width = binding->destination_contiguous_width;
            arg_plan.destination_occupied_register_names =
                binding->destination_occupied_register_names;
            arg_plan.destination_stack_offset_bytes = binding->destination_stack_offset_bytes;
            if (binding->destination_register_name.has_value()) {
              arg_plan.destination_register_bank = arg_plan.value_bank;
              arg_plan.destination_register_placement =
                  binding->destination_register_placement;
              if (!arg_plan.destination_register_placement.has_value() &&
                  arg_index < call->arg_abi.size() && abi_register_index.has_value()) {
                arg_plan.destination_register_placement =
                    call_arg_destination_register_placement(prepared.target_profile,
                                                            call->arg_abi[arg_index],
                                                            *abi_register_index,
                                                            arg_plan.destination_contiguous_width);
              }
            }
          }
          if (!arg_plan.destination_register_name.has_value()) {
            if (const auto* register_binding =
                    find_call_register_abi_binding(before_call_bundle,
                                                   PreparedMoveDestinationKind::CallArgumentAbi,
                                                   arg_index);
                register_binding != nullptr) {
              arg_plan.destination_register_name = register_binding->destination_register_name;
              arg_plan.destination_contiguous_width =
                  register_binding->destination_contiguous_width;
              arg_plan.destination_occupied_register_names =
                  register_binding->destination_occupied_register_names;
              arg_plan.destination_register_bank = arg_plan.value_bank;
              arg_plan.destination_register_placement =
                  register_binding->destination_register_placement;
              if (!arg_plan.destination_register_placement.has_value() &&
                  arg_index < call->arg_abi.size() && abi_register_index.has_value()) {
                arg_plan.destination_register_placement =
                    call_arg_destination_register_placement(prepared.target_profile,
                                                            call->arg_abi[arg_index],
                                                            *abi_register_index,
                                                            arg_plan.destination_contiguous_width);
                }
              }
            }

            if (arg_index < call->arg_abi.size() &&
                arg_plan.destination_register_name.has_value() && abi_register_index.has_value()) {
              const std::size_t byval_lane_width =
                  aarch64_byval_register_lane_width(prepared.target_profile,
                                                     call->arg_abi[arg_index]);
              if (byval_lane_width > arg_plan.destination_contiguous_width) {
                arg_plan.destination_contiguous_width = byval_lane_width;
                arg_plan.destination_occupied_register_names =
                    regalloc_detail::call_arg_destination_register_names(
                        prepared.target_profile,
                        PreparedRegisterClass::General,
                        *abi_register_index,
                        *arg_plan.destination_register_name,
                        byval_lane_width);
                arg_plan.destination_register_placement =
                    call_arg_destination_register_placement(prepared.target_profile,
                                                            call->arg_abi[arg_index],
                                                            *abi_register_index,
                                                            byval_lane_width);
              }
            }

            if (arg_index < call->args.size()) {
            if (const auto value_name_id = maybe_named_value_id(prepared.names, call->args[arg_index]);
                value_name_id.has_value() && value_locations != nullptr) {
              if (const auto* home = find_prepared_value_home(*value_locations, *value_name_id);
                  home != nullptr) {
                arg_plan.source_encoding = storage_encoding_from_home(*home);
                arg_plan.source_register_name = home->register_name;
                arg_plan.source_slot_id = home->slot_id;
                arg_plan.source_stack_offset_bytes = home->offset_bytes;
                if (home->immediate_i32.has_value()) {
                  arg_plan.source_literal = bir::Value::immediate_i32(*home->immediate_i32);
                }
                arg_plan.source_base_value_name = home->pointer_base_value_name;
                arg_plan.source_pointer_byte_delta = home->pointer_byte_delta;
                if (home->pointer_base_value_name.has_value()) {
                  if (const auto* base_home =
                          find_prepared_value_home(*value_locations, *home->pointer_base_value_name);
                      base_home != nullptr) {
                    arg_plan.source_base_value_id = base_home->value_id;
                  } else if (regalloc_function != nullptr) {
                    if (const auto* base_regalloc_value =
                            find_regalloc_value_by_name(*regalloc_function,
                                                        *home->pointer_base_value_name);
                        base_regalloc_value != nullptr) {
                      arg_plan.source_base_value_id = base_regalloc_value->value_id;
                    }
                  }
                }
              }
              if (regalloc_function != nullptr) {
                if (const auto* regalloc_value =
                        find_regalloc_value_by_name(*regalloc_function, *value_name_id);
                  regalloc_value != nullptr) {
                  arg_plan.source_value_id = regalloc_value->value_id;
                  arg_plan.source_register_bank =
                      register_bank_from_class(regalloc_value->register_class);
                  if (regalloc_value->assigned_register.has_value() &&
                      arg_plan.source_register_name ==
                          std::optional<std::string>{
                              regalloc_value->assigned_register->register_name}) {
                    arg_plan.source_register_placement =
                        assignment_register_placement(prepared.target_profile,
                                                      *regalloc_value->assigned_register);
                  }
                }
              }
              const auto symbol_name =
                  resolve_symbol_pointer_name(prepared.module, call->args[arg_index]);
              if (symbol_name.has_value()) {
                arg_plan.source_encoding = PreparedStorageEncodingKind::SymbolAddress;
                if (call->args[arg_index].pointer_symbol_link_name_id != kInvalidLinkName) {
                  arg_plan.source_symbol_name_id = prepared.names.link_names.intern(*symbol_name);
                  arg_plan.source_symbol_name = display_symbol_pointer_name(*symbol_name);
                } else {
                  arg_plan.source_symbol_name = std::string(*symbol_name);
                }
              }
            } else if (const auto symbol_name =
                           resolve_symbol_pointer_name(prepared.module, call->args[arg_index]);
                       symbol_name.has_value()) {
              arg_plan.source_encoding = PreparedStorageEncodingKind::SymbolAddress;
              if (call->args[arg_index].pointer_symbol_link_name_id != kInvalidLinkName) {
                arg_plan.source_symbol_name_id = prepared.names.link_names.intern(*symbol_name);
                arg_plan.source_symbol_name = display_symbol_pointer_name(*symbol_name);
              } else {
                arg_plan.source_symbol_name = std::string(*symbol_name);
              }
            } else if (call->args[arg_index].kind != bir::Value::Kind::Named) {
              arg_plan.source_encoding = PreparedStorageEncodingKind::Immediate;
              arg_plan.source_literal = call->args[arg_index];
              if (regalloc_function != nullptr) {
                if (const auto* constant_value =
                        find_f128_constant_regalloc_value(*regalloc_function, call->args[arg_index]);
                    constant_value != nullptr) {
                  arg_plan.source_value_id = constant_value->value_id;
                  arg_plan.source_register_bank =
                      register_bank_from_class(constant_value->register_class);
                }
              }
            }
          }

          call_plan.arguments.push_back(std::move(arg_plan));
        }

        if (call->result.has_value() && call->result->kind == bir::Value::Kind::Named &&
            value_locations != nullptr) {
          PreparedCallResultPlan result_plan{
              .instruction_index = instruction_index,
              .value_bank = call->result_abi.has_value()
                                ? register_bank_from_result_abi(*call->result_abi)
                                : register_bank_from_type(call->result->type),
              .source_storage_kind = PreparedMoveStorageKind::None,
              .destination_storage_kind = PreparedMoveStorageKind::None,
              .destination_value_id = std::nullopt,
              .source_register_name = std::nullopt,
              .source_contiguous_width = 1,
              .source_occupied_register_names = {},
              .source_register_bank = std::nullopt,
              .source_stack_offset_bytes = std::nullopt,
              .destination_register_name = std::nullopt,
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {},
              .destination_register_bank = std::nullopt,
              .destination_slot_id = std::nullopt,
              .destination_stack_offset_bytes = std::nullopt,
              .source_register_placement = std::nullopt,
              .destination_register_placement = std::nullopt,
              .destination_spill_slot_placement = std::nullopt,
          };

          if (const auto* binding = find_call_abi_binding(after_call_bundle,
                                                          PreparedMoveDestinationKind::CallResultAbi,
                                                          std::nullopt);
              binding != nullptr) {
            result_plan.source_storage_kind = binding->destination_storage_kind;
            result_plan.source_register_name = binding->destination_register_name;
            result_plan.source_contiguous_width = binding->destination_contiguous_width;
            result_plan.source_occupied_register_names =
                binding->destination_occupied_register_names;
            result_plan.source_stack_offset_bytes = binding->destination_stack_offset_bytes;
            if (binding->destination_register_name.has_value()) {
              result_plan.source_register_bank = result_plan.value_bank;
              result_plan.source_register_placement = binding->destination_register_placement;
              if (!result_plan.source_register_placement.has_value() &&
                  call->result_abi.has_value()) {
                result_plan.source_register_placement =
                    call_result_destination_register_placement(prepared.target_profile,
                                                               *call->result_abi,
                                                               result_plan.source_contiguous_width);
              }
            }
          }

          if (const auto value_name_id = maybe_named_value_id(prepared.names, *call->result);
              value_name_id.has_value()) {
            if (const auto* home = find_prepared_value_home(*value_locations, *value_name_id);
                home != nullptr) {
              result_plan.destination_storage_kind = move_storage_kind_from_home(*home);
              result_plan.destination_register_name = home->register_name;
              result_plan.destination_slot_id = home->slot_id;
              result_plan.destination_stack_offset_bytes = home->offset_bytes;
              if (home->register_name.has_value()) {
                result_plan.destination_register_bank = result_plan.value_bank;
              }
              result_plan.destination_spill_slot_placement =
                  make_spill_slot_placement(home->slot_id, home->offset_bytes);
            }
            if (regalloc_function != nullptr) {
              if (const auto* regalloc_value =
                      find_regalloc_value_by_name(*regalloc_function, *value_name_id);
                  regalloc_value != nullptr) {
                result_plan.destination_value_id = regalloc_value->value_id;
                if (regalloc_value->assigned_register.has_value() &&
                    result_plan.destination_register_name ==
                        std::optional<std::string>{
                            regalloc_value->assigned_register->register_name}) {
                  result_plan.destination_register_placement =
                      assignment_register_placement(prepared.target_profile,
                                                    *regalloc_value->assigned_register);
                }
              }
            }
          }
          call_plan.result = std::move(result_plan);
        }

        function_plan.calls.push_back(std::move(call_plan));
      }
    }

    if (!function_plan.calls.empty()) {
      prepared.call_plans.functions.push_back(std::move(function_plan));
    }
  }
}

}  // namespace c4c::backend::prepare
