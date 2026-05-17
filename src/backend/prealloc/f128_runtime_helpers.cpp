#include "f128_runtime_helpers.hpp"
#include "target_register_profile.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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

[[nodiscard]] PreparedRegisterClass register_class_from_bank(PreparedRegisterBank bank) {
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

[[nodiscard]] std::size_t type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I16:
      return 2;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::F64:
    case bir::TypeKind::Ptr:
      return 8;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return 16;
    case bir::TypeKind::Void:
      return 0;
  }
  return 0;
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

[[nodiscard]] bool preserved_value_has_complete_route(
    const PreparedCallPreservedValue& preserved) {
  switch (preserved.route) {
    case PreparedCallPreservationRoute::Unknown:
      return false;
    case PreparedCallPreservationRoute::CalleeSavedRegister:
      return preserved.register_name.has_value() &&
             preserved.register_bank.has_value() &&
             !preserved.occupied_register_names.empty() &&
             preserved.register_placement.has_value() &&
             preserved.callee_saved_save_index.has_value();
    case PreparedCallPreservationRoute::StackSlot:
      return preserved.slot_id.has_value() &&
             preserved.stack_offset_bytes.has_value() &&
             preserved.stack_size_bytes.has_value() &&
             *preserved.stack_size_bytes > 0 &&
             preserved.stack_align_bytes.has_value() &&
             *preserved.stack_align_bytes > 0 &&
             preserved.spill_slot_placement.has_value();
  }
  return false;
}

void append_f128_runtime_helper_fact(PreparedF128RuntimeHelper& helper,
                                     std::string fact) {
  if (std::find(helper.missing_required_facts.begin(),
                helper.missing_required_facts.end(),
                fact) == helper.missing_required_facts.end()) {
    helper.missing_required_facts.push_back(std::move(fact));
  }
}

[[nodiscard]] PreparedF128RuntimeHelper::CarrierBinding make_f128_helper_carrier_binding(
    const PreparedF128Carrier& carrier) {
  return PreparedF128RuntimeHelper::CarrierBinding{
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .carrier_kind = carrier.kind,
      .width_bytes = carrier.total_size_bytes,
      .align_bytes = carrier.total_align_bytes,
      .register_bank = carrier.register_bank,
      .register_class = carrier.register_class,
      .register_name = carrier.register_name,
      .slot_id = carrier.slot_id,
      .stack_offset_bytes = carrier.stack_offset_bytes,
  };
}

void populate_f128_helper_carrier_from_fact(
    const PreparedF128CarrierFunction* function_carriers,
    PreparedF128RuntimeHelper& helper,
    PreparedValueId value_id,
    std::optional<PreparedF128RuntimeHelper::CarrierBinding>& output,
    std::string_view fact_prefix) {
  output.reset();
  if (function_carriers == nullptr) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_missing_prepared_f128_carriers");
    return;
  }
  const auto* carrier = find_prepared_f128_carrier(*function_carriers, value_id);
  if (carrier == nullptr) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_missing_prepared_f128_carrier");
    return;
  }
  output = make_f128_helper_carrier_binding(*carrier);
  if (carrier->kind == PreparedF128CarrierKind::Missing) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_requires_full_width_f128_carrier");
  }
  if (carrier->total_size_bytes != 16 || carrier->total_align_bytes != 16) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_requires_16_byte_f128_carrier");
  }
  for (const auto& fact : carrier->missing_required_facts) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_carrier_fact:" + fact);
  }
}

[[nodiscard]] PreparedRegisterBank f128_helper_abi_bank_for_register(
    const std::string& register_name) {
  return !register_name.empty() && register_name.front() == 'q'
             ? PreparedRegisterBank::Vreg
             : PreparedRegisterBank::Fpr;
}

[[nodiscard]] std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>
make_f128_helper_abi_register_binding(
    const c4c::TargetProfile& target_profile,
    PreparedValueId value_id,
    ValueNameId value_name,
    std::optional<std::size_t> helper_argument_index,
    std::size_t abi_register_index) {
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterPlacement> placement;

  if (helper_argument_index.has_value()) {
    const auto arg_abi = infer_call_arg_abi(target_profile, bir::TypeKind::F128);
    if (!arg_abi.has_value() || !arg_abi->passed_in_register) {
      return std::nullopt;
    }
    register_name =
        call_arg_destination_register_name(target_profile, *arg_abi, abi_register_index);
    placement =
        call_arg_destination_register_placement(target_profile, *arg_abi, abi_register_index);
  } else {
    const bir::CallResultAbiInfo result_abi{
        .type = bir::TypeKind::F128,
        .primary_class = target_profile.has_float_return_registers
                             ? bir::AbiValueClass::Sse
                             : bir::AbiValueClass::Integer,
    };
    register_name = call_result_destination_register_name(target_profile, result_abi);
    placement = call_result_destination_register_placement(target_profile, result_abi);
  }

  if (!register_name.has_value() || register_name->empty() || !placement.has_value()) {
    return std::nullopt;
  }

  const PreparedRegisterBank bank = f128_helper_abi_bank_for_register(*register_name);
  PreparedRegisterPlacement normalized_placement = *placement;
  normalized_placement.bank = bank;
  normalized_placement.contiguous_width = 1;

  return PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .helper_argument_index = helper_argument_index,
      .abi_register_index = abi_register_index,
      .width_bytes = 16,
      .register_bank = bank,
      .register_class = register_class_from_bank(bank),
      .register_name = *register_name,
      .contiguous_width = 1,
      .occupied_register_names = {*register_name},
      .register_placement = normalized_placement,
  };
}

[[nodiscard]] std::optional<PreparedF128RuntimeHelper::ScalarResultOwnership>
make_f128_helper_scalar_ownership(
    PreparedF128RuntimeHelper& helper,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    PreparedValueId value_id,
    ValueNameId value_name,
    bir::TypeKind type,
    PreparedRegisterBank expected_bank,
    std::string_view fact_prefix) {
  const std::size_t width_bytes = type_size_bytes(type);
  if (value_name == kInvalidValueName) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_identity");
    return std::nullopt;
  }
  if (value_locations == nullptr) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_locations");
    return std::nullopt;
  }
  const auto* home = find_prepared_value_home(*value_locations, value_name);
  if (home == nullptr || home->value_id != value_id) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_home");
    return std::nullopt;
  }
  const PreparedRegisterBank bank =
      published_bank_for_value(regalloc_function, value_name, type);
  if (bank != expected_bank) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_expected_bank");
  }
  if (width_bytes == 0) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_width");
  }
  if (home->kind != PreparedValueHomeKind::Register &&
      home->kind != PreparedValueHomeKind::StackSlot) {
    append_f128_runtime_helper_fact(
        helper,
        std::string(fact_prefix) + "_scalar_ownership_requires_register_or_stack_home");
  }
  return PreparedF128RuntimeHelper::ScalarResultOwnership{
      .value_id = value_id,
      .value_name = value_name,
      .type = type,
      .width_bytes = width_bytes,
      .register_bank = bank,
      .home_kind = home->kind,
      .register_name = home->register_name,
      .slot_id = home->slot_id,
      .stack_offset_bytes = home->offset_bytes,
  };
}

[[nodiscard]] std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>
make_f128_helper_scalar_abi_register_binding(
    const c4c::TargetProfile& target_profile,
    const PreparedF128RuntimeHelper::ScalarResultOwnership& scalar,
    std::optional<std::size_t> helper_argument_index,
    std::size_t abi_register_index) {
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterPlacement> placement;
  if (helper_argument_index.has_value()) {
    const auto arg_abi = infer_call_arg_abi(target_profile, scalar.type);
    if (!arg_abi.has_value() || !arg_abi->passed_in_register) {
      return std::nullopt;
    }
    register_name =
        call_arg_destination_register_name(target_profile, *arg_abi, abi_register_index);
    placement =
        call_arg_destination_register_placement(target_profile, *arg_abi, abi_register_index);
  } else {
    const bir::CallResultAbiInfo result_abi{
        .type = scalar.type,
        .primary_class = scalar.register_bank == PreparedRegisterBank::Gpr
                             ? bir::AbiValueClass::Integer
                             : bir::AbiValueClass::Sse,
    };
    register_name = call_result_destination_register_name(target_profile, result_abi);
    placement = call_result_destination_register_placement(target_profile, result_abi);
  }
  if (!register_name.has_value() || register_name->empty() || !placement.has_value()) {
    return std::nullopt;
  }
  return PreparedF128RuntimeHelper::AbiRegisterBinding{
      .value_id = scalar.value_id,
      .value_name = scalar.value_name,
      .helper_argument_index = helper_argument_index,
      .abi_register_index = abi_register_index,
      .width_bytes = scalar.width_bytes,
      .register_bank = scalar.register_bank,
      .register_class = register_class_from_bank(scalar.register_bank),
      .register_name = *register_name,
      .contiguous_width = 1,
      .occupied_register_names = {*register_name},
      .register_placement = *placement,
  };
}

void populate_f128_runtime_helper_abi_bindings(
    const c4c::TargetProfile& target_profile,
    PreparedF128RuntimeHelper& helper) {
  helper.lhs_abi_argument.reset();
  helper.rhs_abi_argument.reset();
  helper.result_abi_result.reset();
  helper.scalar_operand_abi_argument.reset();

  if (helper.callee_name.empty()) {
    append_f128_runtime_helper_fact(helper, "f128_helper_abi_bindings_require_callee_identity");
    return;
  }

  if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast) {
    const bool scalar_to_f128 = helper.result_type == bir::TypeKind::F128;
    if (scalar_to_f128) {
      helper.scalar_operand_abi_argument =
          helper.scalar_operand.has_value()
              ? make_f128_helper_scalar_abi_register_binding(
                    target_profile, *helper.scalar_operand, std::size_t{0}, 0)
              : std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>{};
      helper.result_abi_result = make_f128_helper_abi_register_binding(
          target_profile, helper.result_value_id, helper.result_value_name, std::nullopt, 0);
    } else {
      helper.lhs_abi_argument = make_f128_helper_abi_register_binding(
          target_profile, helper.operand_value_id, helper.operand_value_name, std::size_t{0}, 0);
      helper.result_abi_result =
          helper.scalar_result.has_value()
              ? make_f128_helper_scalar_abi_register_binding(
                    target_profile, *helper.scalar_result, std::nullopt, 0)
              : std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>{};
    }
    helper.abi_policy = PreparedF128RuntimeHelper::AbiPolicy{
        .transition = scalar_to_f128
                          ? PreparedF128RuntimeHelperAbiTransition::
                                DirectScalarArgumentAndF128Result
                          : PreparedF128RuntimeHelperAbiTransition::
                                DirectF128ArgumentAndScalarResult,
        .argument_bank = scalar_to_f128 ? PreparedRegisterBank::Fpr : PreparedRegisterBank::Vreg,
        .result_bank = scalar_to_f128 ? PreparedRegisterBank::Vreg : PreparedRegisterBank::Fpr,
        .argument_count = 1,
        .result_count = 1,
        .width_bytes = scalar_to_f128 ? std::size_t{16} : type_size_bytes(helper.result_type),
    };
  } else {
    helper.lhs_abi_argument =
        make_f128_helper_abi_register_binding(target_profile,
                                              helper.lhs_value_id,
                                              helper.lhs_value_name,
                                              std::size_t{0},
                                              0);
    helper.rhs_abi_argument =
        make_f128_helper_abi_register_binding(target_profile,
                                              helper.rhs_value_id,
                                              helper.rhs_value_name,
                                              std::size_t{1},
                                              1);
    helper.result_abi_result =
        helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison
            ? (helper.scalar_result.has_value()
                   ? make_f128_helper_scalar_abi_register_binding(
                         target_profile, *helper.scalar_result, std::nullopt, 0)
                   : std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>{})
            : make_f128_helper_abi_register_binding(target_profile,
                                                    helper.result_value_id,
                                                    helper.result_value_name,
                                                    std::nullopt,
                                                    0);
    helper.abi_policy = PreparedF128RuntimeHelper::AbiPolicy{
        .transition =
            helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison
                ? PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult
                : PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult,
        .argument_bank = PreparedRegisterBank::Vreg,
        .result_bank = helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison
                           ? PreparedRegisterBank::Gpr
                           : PreparedRegisterBank::Vreg,
        .argument_count = 2,
        .result_count = 1,
        .width_bytes =
            helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison
                ? std::size_t{4}
                : std::size_t{16},
    };
  }

  if (!helper.result_abi_result.has_value() ||
      (helper.helper_family != PreparedF128RuntimeHelperFamily::Cast &&
       (!helper.lhs_abi_argument.has_value() || !helper.rhs_abi_argument.has_value())) ||
      (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
       helper.result_type == bir::TypeKind::F128 && !helper.scalar_operand_abi_argument.has_value()) ||
      (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
       helper.source_type == bir::TypeKind::F128 && !helper.lhs_abi_argument.has_value())) {
    append_f128_runtime_helper_fact(
        helper, "f128_helper_abi_bindings_require_supported_registers");
  }
}

[[nodiscard]] std::optional<PreparedF128RuntimeHelper::MarshalingMove>
make_f128_helper_marshaling_move(
    PreparedF128RuntimeHelper& helper,
    const PreparedF128RuntimeHelper::CarrierBinding& carrier,
    const PreparedF128RuntimeHelper::AbiRegisterBinding& abi_register,
    PreparedF128RuntimeHelperMarshalDirection direction,
    std::string_view fact_prefix) {
  if (carrier.carrier_kind != PreparedF128CarrierKind::FullWidthRegister) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_marshaling_requires_full_width_register_carrier");
  }
  if (carrier.width_bytes != 16 || abi_register.width_bytes != 16) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_marshaling_requires_16_byte_f128_values");
  }
  if (carrier.value_id != abi_register.value_id ||
      carrier.value_name != abi_register.value_name) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_marshaling_requires_matching_value_identity");
  }
  if (abi_register.register_bank != PreparedRegisterBank::Vreg ||
      abi_register.register_class != PreparedRegisterClass::Vector ||
      abi_register.register_name.empty() ||
      abi_register.occupied_register_names.empty() ||
      abi_register.contiguous_width != 1 ||
      !abi_register.register_placement.has_value()) {
    append_f128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_marshaling_requires_q_register_binding");
  }
  return PreparedF128RuntimeHelper::MarshalingMove{
      .direction = direction,
      .carrier = carrier,
      .abi_register = abi_register,
  };
}

void populate_f128_runtime_helper_marshaling(PreparedF128RuntimeHelper& helper) {
  helper.lhs_argument_move.reset();
  helper.rhs_argument_move.reset();
  helper.result_unmarshal_move.reset();
  helper.scalar_result_unmarshal_move.reset();
  helper.scalar_operand_argument_move.reset();

  auto populate_move =
      [&](const std::optional<PreparedF128RuntimeHelper::CarrierBinding>& carrier,
          const std::optional<PreparedF128RuntimeHelper::AbiRegisterBinding>& abi_register,
          PreparedF128RuntimeHelperMarshalDirection direction,
          std::optional<PreparedF128RuntimeHelper::MarshalingMove>& output,
          std::string_view fact_prefix) {
        if (!carrier.has_value()) {
          append_f128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_carrier");
          return;
        }
        if (!abi_register.has_value()) {
          append_f128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_abi_binding");
          return;
        }
        output = make_f128_helper_marshaling_move(helper,
                                                  *carrier,
                                                  *abi_register,
                                                  direction,
                                                  fact_prefix);
      };

  if (helper.helper_family != PreparedF128RuntimeHelperFamily::Cast ||
      helper.source_type == bir::TypeKind::F128) {
    populate_move(helper.lhs_carrier,
                  helper.lhs_abi_argument,
                  PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                  helper.lhs_argument_move,
                  "lhs");
  }
  if (helper.helper_family != PreparedF128RuntimeHelperFamily::Cast) {
    populate_move(helper.rhs_carrier,
                  helper.rhs_abi_argument,
                  PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                  helper.rhs_argument_move,
                  "rhs");
  }
  if (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison) {
    if (!helper.scalar_result.has_value()) {
      append_f128_runtime_helper_fact(
          helper, "cmp_result_marshaling_requires_scalar_result_ownership");
    } else if (!helper.result_abi_result.has_value()) {
      append_f128_runtime_helper_fact(
          helper, "cmp_result_marshaling_requires_abi_binding");
    } else if (helper.result_abi_result->register_bank != PreparedRegisterBank::Gpr ||
               helper.result_abi_result->width_bytes != 4) {
      append_f128_runtime_helper_fact(
          helper, "cmp_result_marshaling_requires_gpr_cmp_result_binding");
    } else {
      helper.scalar_result_unmarshal_move =
          PreparedF128RuntimeHelper::ScalarMarshalingMove{
              .direction =
                  PreparedF128RuntimeHelperMarshalDirection::AbiCmpResultToScalar,
              .scalar_result = *helper.scalar_result,
              .abi_register = *helper.result_abi_result,
          };
    }
  } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast) {
    const bool scalar_to_f128 = helper.result_type == bir::TypeKind::F128;
    if (scalar_to_f128) {
      if (!helper.scalar_operand.has_value()) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_argument_marshaling_requires_scalar_operand_ownership");
      } else if (!helper.scalar_operand_abi_argument.has_value()) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_argument_marshaling_requires_abi_binding");
      } else if (helper.scalar_operand_abi_argument->register_bank != PreparedRegisterBank::Fpr) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_argument_marshaling_requires_fpr_binding");
      } else {
        helper.scalar_operand_argument_move =
            PreparedF128RuntimeHelper::ScalarMarshalingMove{
                .direction =
                    PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument,
                .scalar_result = *helper.scalar_operand,
                .abi_register = *helper.scalar_operand_abi_argument,
            };
      }
      populate_move(helper.result_carrier,
                    helper.result_abi_result,
                    PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier,
                    helper.result_unmarshal_move,
                    "result");
    } else {
      if (!helper.scalar_result.has_value()) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_result_marshaling_requires_scalar_result_ownership");
      } else if (!helper.result_abi_result.has_value()) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_result_marshaling_requires_abi_binding");
      } else if (helper.result_abi_result->register_bank != PreparedRegisterBank::Fpr) {
        append_f128_runtime_helper_fact(
            helper, "cast_scalar_result_marshaling_requires_fpr_binding");
      } else {
        helper.scalar_result_unmarshal_move =
            PreparedF128RuntimeHelper::ScalarMarshalingMove{
                .direction =
                    PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar,
                .scalar_result = *helper.scalar_result,
                .abi_register = *helper.result_abi_result,
            };
      }
    }
  } else {
    populate_move(helper.result_carrier,
                  helper.result_abi_result,
                  PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier,
                  helper.result_unmarshal_move,
                  "result");
  }
}

void populate_f128_runtime_helper_boundary_policy(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function,
    PreparedF128RuntimeHelper& helper) {
  helper.resource_policy = PreparedF128RuntimeHelper::ResourcePolicy{
      .call_boundary = true,
      .runtime_helper_callee = !helper.callee_name.empty(),
      .caller_saved_clobbers = true,
      .preserves_source_operation_identity = true,
  };
  helper.abi_policy = PreparedF128RuntimeHelper::AbiPolicy{
      .transition = PreparedF128RuntimeHelperAbiTransition::Missing,
      .argument_bank = PreparedRegisterBank::None,
      .result_bank = PreparedRegisterBank::None,
      .argument_count = 2,
      .result_count = 1,
      .width_bytes = 16,
  };
  helper.live_preservation_policy = PreparedF128RuntimeHelper::LivePreservationPolicy{
      .evaluated = false,
      .caller_saved_clobbers_modeled = false,
      .no_additional_live_preservation_required = false,
      .preserved_values = {},
  };
  helper.clobbered_registers = build_call_clobber_set(target_profile, regalloc_function);

  if (helper.callee_name.empty()) {
    append_f128_runtime_helper_fact(helper, "f128_helper_boundary_requires_callee_identity");
  }
  if (helper.clobbered_registers.empty()) {
    append_f128_runtime_helper_fact(
        helper, "f128_helper_boundary_requires_caller_saved_clobbers");
  }
}

void populate_f128_runtime_helper_call_ownership(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    PreparedF128RuntimeHelper& helper) {
  const bool has_clobber_policy =
      helper.resource_policy.caller_saved_clobbers && !helper.clobbered_registers.empty();
  const auto helper_point =
      liveness_function == nullptr
          ? std::optional<std::size_t>{}
          : find_call_program_point(*liveness_function,
                                    helper.block_index,
                                    helper.instruction_index);
  std::vector<PreparedCallPreservedValue> preserved_values =
      helper_point.has_value()
          ? build_call_preserved_values(prepared,
                                        frame_plan,
                                        liveness_function,
                                        regalloc_function,
                                        value_locations,
                                        *helper_point,
                                        false)
          : std::vector<PreparedCallPreservedValue>{};
  const bool preserved_values_complete =
      std::all_of(preserved_values.begin(),
                  preserved_values.end(),
                  preserved_value_has_complete_route);
  if (liveness_function == nullptr || regalloc_function == nullptr ||
      !helper_point.has_value()) {
    append_f128_runtime_helper_fact(
        helper, "live_preservation_requires_structured_live_across_helper_facts");
  } else if (!preserved_values_complete) {
    append_f128_runtime_helper_fact(
        helper, "live_preservation_requires_complete_preserved_value_routes");
  }
  helper.live_preservation_policy = PreparedF128RuntimeHelper::LivePreservationPolicy{
      .evaluated = true,
      .caller_saved_clobbers_modeled = has_clobber_policy,
      .no_additional_live_preservation_required =
          helper_point.has_value() && preserved_values_complete,
      .preserved_values = std::move(preserved_values),
  };
}

void populate_f128_runtime_helper_ownership(PreparedF128RuntimeHelper& helper) {
  const bool scalar_to_f128_cast =
      helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
      helper.result_type == bir::TypeKind::F128;
  const bool f128_to_scalar_cast =
      helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
      helper.source_type == bir::TypeKind::F128;
  const bool has_carriers =
      scalar_to_f128_cast
          ? (helper.scalar_operand.has_value() && helper.result_carrier.has_value())
          : (f128_to_scalar_cast
                 ? (helper.lhs_carrier.has_value() && helper.scalar_result.has_value())
                 : (helper.lhs_carrier.has_value() &&
                    helper.rhs_carrier.has_value() &&
                    (helper.result_carrier.has_value() ||
                     (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison &&
                      helper.scalar_result.has_value()))));
  const bool has_resource_policy =
      helper.resource_policy.call_boundary &&
      helper.resource_policy.runtime_helper_callee &&
      helper.resource_policy.preserves_source_operation_identity;
  const bool has_clobber_policy =
      helper.resource_policy.caller_saved_clobbers && !helper.clobbered_registers.empty();
  const bool has_abi_bindings =
      (((helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison &&
         helper.abi_policy.transition ==
             PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Gpr &&
         helper.abi_policy.argument_count == 2 &&
         helper.abi_policy.width_bytes == 4) ||
        (helper.helper_family == PreparedF128RuntimeHelperFamily::Arithmetic &&
         helper.abi_policy.transition ==
             PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.argument_count == 2 &&
         helper.abi_policy.width_bytes == 16) ||
        (scalar_to_f128_cast &&
         helper.abi_policy.transition ==
             PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Fpr &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.argument_count == 1 &&
         helper.abi_policy.width_bytes == 16) ||
        (f128_to_scalar_cast &&
         helper.abi_policy.transition ==
             PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Vreg &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Fpr &&
         helper.abi_policy.argument_count == 1 &&
         (helper.abi_policy.width_bytes == 4 || helper.abi_policy.width_bytes == 8))) &&
       helper.abi_policy.result_count == 1);
  const bool has_marshaling =
      scalar_to_f128_cast
          ? (helper.scalar_operand_argument_move.has_value() &&
             helper.result_unmarshal_move.has_value())
          : (f128_to_scalar_cast
                 ? (helper.lhs_argument_move.has_value() &&
                    helper.scalar_result_unmarshal_move.has_value())
                 : (helper.lhs_argument_move.has_value() &&
                    helper.rhs_argument_move.has_value() &&
                    (helper.result_unmarshal_move.has_value() ||
                     (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison &&
                      helper.scalar_result_unmarshal_move.has_value()))));
  const bool has_cmp_result_consumption =
      helper.helper_family != PreparedF128RuntimeHelperFamily::Comparison ||
      (helper.scalar_cmp_result_consumption.has_value() &&
       helper.scalar_cmp_result_consumption->cmp_type == bir::TypeKind::I32 &&
       helper.scalar_cmp_result_consumption->bir_result_type == bir::TypeKind::I1 &&
       helper.scalar_cmp_result_consumption->zero_test !=
           PreparedF128CmpResultZeroTest::Missing &&
       helper.scalar_cmp_result_consumption->consumes_helper_cmp_result &&
       helper.scalar_cmp_result_consumption->owns_bir_i1_result);
  const bool has_live_preservation =
      helper.live_preservation_policy.evaluated &&
      helper.live_preservation_policy.caller_saved_clobbers_modeled &&
      helper.live_preservation_policy.no_additional_live_preservation_required;

  helper.selected_call_ownership =
      PreparedF128RuntimeHelper::SelectedCallOwnershipPolicy{
          .owns_terminal_call =
              !helper.callee_name.empty() && has_carriers && has_resource_policy &&
              has_clobber_policy && has_abi_bindings && has_marshaling &&
              has_cmp_result_consumption && has_live_preservation,
          .has_callee_identity = !helper.callee_name.empty(),
          .has_resource_policy = has_resource_policy,
          .has_clobber_policy = has_clobber_policy,
          .has_abi_bindings = has_abi_bindings,
          .has_marshaling = has_marshaling && has_cmp_result_consumption,
          .has_live_preservation = has_live_preservation,
      };

  if (!has_carriers) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_f128_carriers");
  }
  if (!helper.selected_call_ownership.has_resource_policy) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_resource_policy");
  }
  if (!helper.selected_call_ownership.has_clobber_policy) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_clobber_policy");
  }
  if (!helper.selected_call_ownership.has_abi_bindings) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_abi_bindings");
  }
  if (!helper.selected_call_ownership.has_marshaling) {
    append_f128_runtime_helper_fact(helper, "selected_call_ownership_requires_marshaling");
  }
  if (!helper.selected_call_ownership.has_live_preservation) {
    append_f128_runtime_helper_fact(
        helper, "selected_call_ownership_requires_live_preservation_policy");
  }
}

}  // namespace

void populate_f128_runtime_helper_facts(PreparedBirModule& prepared) {
  for (auto& function_helpers : prepared.f128_runtime_helpers.functions) {
    const auto* function_carriers =
        find_prepared_f128_carriers(prepared.f128_carriers, function_helpers.function_name);
    const auto* regalloc_function =
        find_regalloc_function(prepared.regalloc, function_helpers.function_name);
    const auto* frame_plan = find_prepared_frame_plan(prepared, function_helpers.function_name);
    const auto* liveness_function =
        find_liveness_function(prepared.liveness, function_helpers.function_name);
    const auto* value_locations =
        find_prepared_value_location_function(prepared, function_helpers.function_name);
    for (auto& helper : function_helpers.helpers) {
      helper.lhs_carrier.reset();
      helper.rhs_carrier.reset();
      helper.result_carrier.reset();
      helper.scalar_operand.reset();
      helper.scalar_result.reset();
      helper.missing_required_facts.clear();
      populate_f128_runtime_helper_boundary_policy(
          prepared.target_profile, regalloc_function, helper);
      if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast) {
        if (helper.result_type == bir::TypeKind::F128) {
          helper.scalar_operand =
              make_f128_helper_scalar_ownership(helper,
                                                regalloc_function,
                                                value_locations,
                                                helper.operand_value_id,
                                                helper.operand_value_name,
                                                helper.source_type,
                                                PreparedRegisterBank::Fpr,
                                                "cast_operand");
          populate_f128_helper_carrier_from_fact(
              function_carriers, helper, helper.result_value_id, helper.result_carrier, "result");
        } else {
          populate_f128_helper_carrier_from_fact(
              function_carriers, helper, helper.operand_value_id, helper.lhs_carrier, "lhs");
          helper.scalar_result =
              make_f128_helper_scalar_ownership(helper,
                                                regalloc_function,
                                                value_locations,
                                                helper.result_value_id,
                                                helper.result_value_name,
                                                helper.result_type,
                                                PreparedRegisterBank::Fpr,
                                                "cast_result");
        }
      } else {
        populate_f128_helper_carrier_from_fact(
            function_carriers, helper, helper.lhs_value_id, helper.lhs_carrier, "lhs");
        populate_f128_helper_carrier_from_fact(
            function_carriers, helper, helper.rhs_value_id, helper.rhs_carrier, "rhs");
      }
      if (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison) {
        helper.scalar_result =
            make_f128_helper_scalar_ownership(helper,
                                              regalloc_function,
                                              value_locations,
                                              helper.result_value_id,
                                              helper.result_value_name,
                                              bir::TypeKind::I32,
                                              PreparedRegisterBank::Gpr,
                                              "cmp_result");
      } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Arithmetic) {
        populate_f128_helper_carrier_from_fact(
            function_carriers, helper, helper.result_value_id, helper.result_carrier, "result");
      }
      populate_f128_runtime_helper_abi_bindings(prepared.target_profile, helper);
      populate_f128_runtime_helper_marshaling(helper);
      if (helper.helper_family == PreparedF128RuntimeHelperFamily::Comparison &&
          helper.result_ownership !=
              PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult) {
        append_f128_runtime_helper_fact(
            helper, "f128_compare_helper_result_requires_scalar_cmp_ownership");
      } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Arithmetic &&
                 helper.result_ownership !=
                     PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) {
        append_f128_runtime_helper_fact(
            helper, "f128_helper_result_requires_full_width_carrier_ownership");
      } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
                 helper.result_type == bir::TypeKind::F128 &&
                 helper.result_ownership !=
                     PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) {
        append_f128_runtime_helper_fact(
            helper, "f128_cast_helper_result_requires_full_width_carrier_ownership");
      } else if (helper.helper_family == PreparedF128RuntimeHelperFamily::Cast &&
                 helper.source_type == bir::TypeKind::F128 &&
                 helper.result_ownership !=
                     PreparedF128RuntimeHelperResultOwnership::ScalarValue) {
        append_f128_runtime_helper_fact(
            helper, "f128_cast_helper_result_requires_scalar_ownership");
      }
      populate_f128_runtime_helper_call_ownership(prepared,
                                                  frame_plan,
                                                  liveness_function,
                                                  regalloc_function,
                                                  value_locations,
                                                  helper);
      populate_f128_runtime_helper_ownership(helper);
    }
  }
}

}  // namespace c4c::backend::prepare
