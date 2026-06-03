#include "i128_runtime_helpers.hpp"
#include "target_register_profile.hpp"

#include <algorithm>
#include <array>
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

[[nodiscard]] std::vector<PreparedClobberedRegister> build_call_clobber_set(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function);

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    std::size_t program_point,
    bool require_call_crossing_value);

[[nodiscard]] bool preserved_value_has_complete_route(
    const PreparedCallPreservedValue& preserved);

void append_i128_runtime_helper_fact(PreparedI128RuntimeHelper& helper,
                                     std::string fact) {
  if (std::find(helper.missing_required_facts.begin(),
                helper.missing_required_facts.end(),
                fact) == helper.missing_required_facts.end()) {
    helper.missing_required_facts.push_back(std::move(fact));
  }
}

[[nodiscard]] PreparedI128RuntimeHelper::LaneBinding make_i128_helper_lane_binding(
    const PreparedI128Carrier& carrier,
    const PreparedI128LaneCarrier& lane) {
  return PreparedI128RuntimeHelper::LaneBinding{
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .carrier_kind = carrier.kind,
      .role = lane.role,
      .lane_index = lane.lane_index,
      .width_bytes = lane.width_bytes,
      .register_name = lane.register_name,
      .slot_id = lane.slot_id,
      .stack_offset_bytes = lane.stack_offset_bytes,
  };
}

[[nodiscard]] std::optional<std::string> i128_helper_abi_register_name(
    const c4c::TargetProfile& target_profile,
    bool result,
    std::size_t abi_register_index) {
  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      if (result) {
        constexpr std::array<std::string_view, 2> kResultRegisters = {"rax", "rdx"};
        if (abi_register_index < kResultRegisters.size()) {
          return std::string(kResultRegisters[abi_register_index]);
        }
        return std::nullopt;
      }
      break;
    case c4c::TargetArch::Aarch64:
    case c4c::TargetArch::Riscv64:
      if (result) {
        const bir::CallResultAbiInfo result_abi{
            .type = bir::TypeKind::I64,
            .primary_class = bir::AbiValueClass::Integer,
        };
        const auto low = call_result_destination_register_name(target_profile, result_abi);
        if (!low.has_value()) {
          return std::nullopt;
        }
        if (abi_register_index == 0) {
          return low;
        }
        if (abi_register_index == 1) {
          if (target_profile.arch == c4c::TargetArch::Aarch64) {
            return std::string("x1");
          }
          return std::string("a1");
        }
        return std::nullopt;
      }
      break;
    case c4c::TargetArch::I686:
    case c4c::TargetArch::Unknown:
      return std::nullopt;
  }

  const bir::CallArgAbiInfo arg_abi{
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .primary_class = bir::AbiValueClass::Integer,
      .passed_in_register = true,
  };
  return call_arg_destination_register_name(target_profile, arg_abi, abi_register_index);
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>
make_i128_helper_abi_register_binding(
    const c4c::TargetProfile& target_profile,
    PreparedValueId value_id,
    ValueNameId value_name,
    PreparedI128LaneRole role,
    std::size_t lane_index,
    std::optional<std::size_t> helper_argument_index,
    std::size_t abi_register_index) {
  const auto register_name =
      i128_helper_abi_register_name(target_profile,
                                    !helper_argument_index.has_value(),
                                    abi_register_index);
  if (!register_name.has_value()) {
    return std::nullopt;
  }
  return PreparedI128RuntimeHelper::AbiRegisterBinding{
      .value_id = value_id,
      .value_name = value_name,
      .role = role,
      .lane_index = lane_index,
      .width_bytes = 8,
      .helper_argument_index = helper_argument_index,
      .abi_register_index = abi_register_index,
      .register_bank = PreparedRegisterBank::Gpr,
      .register_class = PreparedRegisterClass::General,
      .register_name = *register_name,
      .contiguous_width = 1,
      .occupied_register_names = {*register_name},
      .register_placement =
          PreparedRegisterPlacement{
              .bank = PreparedRegisterBank::Gpr,
              .pool = helper_argument_index.has_value()
                          ? PreparedRegisterSlotPool::CallArgument
                          : PreparedRegisterSlotPool::CallResult,
              .slot_index = abi_register_index,
              .contiguous_width = 1,
          },
  };
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>
make_i128_helper_scalar_abi_register_binding(
    const c4c::TargetProfile& target_profile,
    const PreparedI128RuntimeHelper::ScalarValueOwnership& scalar,
    std::optional<std::size_t> helper_argument_index,
    std::size_t abi_register_index) {
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterPlacement> placement;
  PreparedRegisterBank bank = PreparedRegisterBank::None;

  if (helper_argument_index.has_value()) {
    const auto arg_abi = infer_call_arg_abi(target_profile, scalar.type);
    if (!arg_abi.has_value() || !arg_abi->passed_in_register) {
      return std::nullopt;
    }
    register_name =
        call_arg_destination_register_name(target_profile, *arg_abi, abi_register_index);
    placement =
        call_arg_destination_register_placement(target_profile, *arg_abi, abi_register_index);
    bank = register_bank_from_arg_abi(*arg_abi);
  } else {
    const bir::CallResultAbiInfo result_abi{
        .type = scalar.type,
        .primary_class = scalar.register_bank == PreparedRegisterBank::Fpr
                             ? bir::AbiValueClass::Sse
                             : bir::AbiValueClass::Integer,
    };
    register_name = call_result_destination_register_name(target_profile, result_abi);
    placement = call_result_destination_register_placement(target_profile, result_abi);
    bank = register_bank_from_result_abi(result_abi);
  }

  if (!register_name.has_value() || register_name->empty() ||
      !placement.has_value() || bank == PreparedRegisterBank::None ||
      bank != scalar.register_bank) {
    return std::nullopt;
  }

  return PreparedI128RuntimeHelper::AbiRegisterBinding{
      .value_id = scalar.value_id,
      .value_name = scalar.value_name,
      .role = PreparedI128LaneRole::Low,
      .lane_index = 0,
      .width_bytes = scalar.width_bytes,
      .helper_argument_index = helper_argument_index,
      .abi_register_index = abi_register_index,
      .register_bank = bank,
      .register_class = register_class_from_bank(bank),
      .register_name = *register_name,
      .contiguous_width = 1,
      .occupied_register_names = {*register_name},
      .register_placement = placement,
  };
}

void populate_i128_helper_lanes_from_carrier(
    const PreparedI128CarrierFunction* function_carriers,
    PreparedI128RuntimeHelper& helper,
    PreparedValueId value_id,
    std::optional<PreparedI128RuntimeHelper::LaneBinding>& low,
    std::optional<PreparedI128RuntimeHelper::LaneBinding>& high,
    std::string_view fact_prefix) {
  low.reset();
  high.reset();
  if (function_carriers == nullptr) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_missing_prepared_i128_carriers");
    return;
  }

  const auto* carrier = find_prepared_i128_carrier(*function_carriers, value_id);
  if (carrier == nullptr) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_missing_prepared_i128_carrier");
    return;
  }

  low = make_i128_helper_lane_binding(*carrier, carrier->low_lane);
  high = make_i128_helper_lane_binding(*carrier, carrier->high_lane);

  if (carrier->kind != PreparedI128CarrierKind::RegisterPair) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_requires_register_pair_carrier");
  }
  for (const auto& fact : carrier->missing_required_facts) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_carrier_fact:" + fact);
  }
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::ScalarValueOwnership>
make_i128_helper_scalar_ownership(
    PreparedI128RuntimeHelper& helper,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    PreparedValueId value_id,
    ValueNameId value_name,
    bir::TypeKind type,
    std::size_t width_bytes,
    PreparedRegisterBank expected_bank,
    std::string_view fact_prefix) {
  if (value_name == kInvalidValueName) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_identity");
    return std::nullopt;
  }
  if (value_locations == nullptr) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_locations");
    return std::nullopt;
  }
  const auto* home = find_prepared_value_home(*value_locations, value_name);
  if (home == nullptr || home->value_id != value_id) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_value_home");
    return std::nullopt;
  }
  const PreparedRegisterBank bank =
      published_bank_for_value(regalloc_function, value_name, type);
  if (bank != expected_bank) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_expected_bank");
  }
  if (width_bytes == 0) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_width");
  }
  if (home->kind != PreparedValueHomeKind::Register &&
      home->kind != PreparedValueHomeKind::StackSlot) {
    append_i128_runtime_helper_fact(
        helper, std::string(fact_prefix) + "_scalar_ownership_requires_register_or_stack_home");
  }
  return PreparedI128RuntimeHelper::ScalarValueOwnership{
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

void populate_i128_runtime_helper_abi_bindings(
    const c4c::TargetProfile& target_profile,
    PreparedI128RuntimeHelper& helper) {
  helper.lhs_low_abi_argument.reset();
  helper.lhs_high_abi_argument.reset();
  helper.rhs_low_abi_argument.reset();
  helper.rhs_high_abi_argument.reset();
  helper.result_low_abi_result.reset();
  helper.result_high_abi_result.reset();
  helper.scalar_operand_abi_argument.reset();
  helper.scalar_result_abi_result.reset();

  if (helper.callee_name.empty()) {
    append_i128_runtime_helper_fact(helper, "i128_helper_abi_bindings_require_callee_identity");
    return;
  }

  if (helper.helper_family == PreparedI128RuntimeHelperFamily::FloatIntegerConversion) {
    const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
    const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
    if (operand_is_i128 == result_is_i128) {
      append_i128_runtime_helper_fact(
          helper, "i128_conversion_abi_bindings_require_one_i128_side");
      return;
    }

    if (operand_is_i128) {
      helper.lhs_low_abi_argument =
          make_i128_helper_abi_register_binding(target_profile,
                                                helper.operand_value_id,
                                                helper.operand_value_name,
                                                PreparedI128LaneRole::Low,
                                                0,
                                                std::size_t{0},
                                                0);
      helper.lhs_high_abi_argument =
          make_i128_helper_abi_register_binding(target_profile,
                                                helper.operand_value_id,
                                                helper.operand_value_name,
                                                PreparedI128LaneRole::High,
                                                1,
                                                std::size_t{0},
                                                1);
      if (!helper.scalar_result.has_value()) {
        append_i128_runtime_helper_fact(
            helper, "i128_conversion_abi_bindings_require_scalar_result_ownership");
      } else {
        helper.scalar_result_abi_result =
            make_i128_helper_scalar_abi_register_binding(
                target_profile, *helper.scalar_result, std::nullopt, 0);
      }
      helper.abi_policy = PreparedI128RuntimeHelper::AbiPolicy{
          .transition =
              PreparedI128RuntimeHelperAbiTransition::RegisterPairArgumentAndScalarResult,
          .argument_bank = PreparedRegisterBank::Gpr,
          .result_bank = PreparedRegisterBank::Fpr,
          .argument_count = 1,
          .lanes_per_argument = 2,
          .result_lane_count = 1,
          .lane_width_bytes = 8,
      };
      if (!helper.lhs_low_abi_argument.has_value() ||
          !helper.lhs_high_abi_argument.has_value() ||
          !helper.scalar_result_abi_result.has_value()) {
        append_i128_runtime_helper_fact(
            helper, "i128_conversion_abi_bindings_require_supported_target_registers");
      }
      return;
    }

    if (!helper.scalar_operand.has_value()) {
      append_i128_runtime_helper_fact(
          helper, "i128_conversion_abi_bindings_require_scalar_operand_ownership");
    } else {
      helper.scalar_operand_abi_argument =
          make_i128_helper_scalar_abi_register_binding(
              target_profile, *helper.scalar_operand, std::size_t{0}, 0);
    }
    helper.result_low_abi_result =
        make_i128_helper_abi_register_binding(target_profile,
                                              helper.result_value_id,
                                              helper.result_value_name,
                                              PreparedI128LaneRole::Low,
                                              0,
                                              std::nullopt,
                                              0);
    helper.result_high_abi_result =
        make_i128_helper_abi_register_binding(target_profile,
                                              helper.result_value_id,
                                              helper.result_value_name,
                                              PreparedI128LaneRole::High,
                                              1,
                                              std::nullopt,
                                              1);
    helper.abi_policy = PreparedI128RuntimeHelper::AbiPolicy{
        .transition =
            PreparedI128RuntimeHelperAbiTransition::ScalarArgumentAndRegisterPairResult,
        .argument_bank = PreparedRegisterBank::Fpr,
        .result_bank = PreparedRegisterBank::Gpr,
        .argument_count = 1,
        .lanes_per_argument = 1,
        .result_lane_count = 2,
        .lane_width_bytes = 8,
    };
    if (!helper.scalar_operand_abi_argument.has_value() ||
        !helper.result_low_abi_result.has_value() ||
        !helper.result_high_abi_result.has_value()) {
      append_i128_runtime_helper_fact(
          helper, "i128_conversion_abi_bindings_require_supported_target_registers");
    }
    return;
  }

  if (helper.helper_family != PreparedI128RuntimeHelperFamily::DivRem) {
    append_i128_runtime_helper_fact(helper, "i128_helper_abi_bindings_deferred_for_family");
    return;
  }
  if (helper.result_ownership !=
      PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes) {
    append_i128_runtime_helper_fact(
        helper, "i128_helper_abi_bindings_require_direct_low_high_result");
    return;
  }

  helper.lhs_low_abi_argument =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.lhs_value_id,
                                            helper.lhs_value_name,
                                            PreparedI128LaneRole::Low,
                                            0,
                                            std::size_t{0},
                                            0);
  helper.lhs_high_abi_argument =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.lhs_value_id,
                                            helper.lhs_value_name,
                                            PreparedI128LaneRole::High,
                                            1,
                                            std::size_t{0},
                                            1);
  helper.rhs_low_abi_argument =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.rhs_value_id,
                                            helper.rhs_value_name,
                                            PreparedI128LaneRole::Low,
                                            0,
                                            std::size_t{1},
                                            2);
  helper.rhs_high_abi_argument =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.rhs_value_id,
                                            helper.rhs_value_name,
                                            PreparedI128LaneRole::High,
                                            1,
                                            std::size_t{1},
                                            3);
  helper.result_low_abi_result =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.result_value_id,
                                            helper.result_value_name,
                                            PreparedI128LaneRole::Low,
                                            0,
                                            std::nullopt,
                                            0);
  helper.result_high_abi_result =
      make_i128_helper_abi_register_binding(target_profile,
                                            helper.result_value_id,
                                            helper.result_value_name,
                                            PreparedI128LaneRole::High,
                                            1,
                                            std::nullopt,
                                            1);

  if (!helper.lhs_low_abi_argument.has_value() || !helper.lhs_high_abi_argument.has_value() ||
      !helper.rhs_low_abi_argument.has_value() || !helper.rhs_high_abi_argument.has_value() ||
      !helper.result_low_abi_result.has_value() ||
      !helper.result_high_abi_result.has_value()) {
    append_i128_runtime_helper_fact(
        helper, "i128_helper_abi_bindings_require_supported_target_registers");
  }
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::MarshalingMove>
make_i128_helper_marshaling_move(
    const PreparedI128RuntimeHelper::LaneBinding& carrier_lane,
    const PreparedI128RuntimeHelper::AbiRegisterBinding& abi_register,
    PreparedI128RuntimeHelperMarshalDirection direction) {
  return PreparedI128RuntimeHelper::MarshalingMove{
      .direction = direction,
      .phase = direction ==
                       PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument
                   ? PreparedMovePhase::BeforeCall
                   : PreparedMovePhase::AfterCall,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .carrier_lane = carrier_lane,
      .abi_register = abi_register,
  };
}

[[nodiscard]] std::optional<PreparedI128RuntimeHelper::ScalarMarshalingMove>
make_i128_helper_scalar_marshaling_move(
    const PreparedI128RuntimeHelper::ScalarValueOwnership& scalar_value,
    const PreparedI128RuntimeHelper::AbiRegisterBinding& abi_register,
    PreparedI128RuntimeHelperMarshalDirection direction) {
  return PreparedI128RuntimeHelper::ScalarMarshalingMove{
      .direction = direction,
      .phase = direction ==
                       PreparedI128RuntimeHelperMarshalDirection::ScalarValueToAbiArgument
                   ? PreparedMovePhase::BeforeCall
                   : PreparedMovePhase::AfterCall,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .scalar_value = scalar_value,
      .abi_register = abi_register,
  };
}

[[nodiscard]] std::vector<PreparedCallPreservedValue> build_call_preserved_values(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    std::size_t program_point,
    bool require_call_crossing_value);

[[nodiscard]] bool preserved_value_has_complete_route(
    const PreparedCallPreservedValue& preserved);

void populate_i128_runtime_helper_marshaling(
    PreparedI128RuntimeHelper& helper) {
  helper.lhs_low_argument_move.reset();
  helper.lhs_high_argument_move.reset();
  helper.rhs_low_argument_move.reset();
  helper.rhs_high_argument_move.reset();
  helper.result_low_unmarshal_move.reset();
  helper.result_high_unmarshal_move.reset();
  helper.scalar_operand_argument_move.reset();
  helper.scalar_result_unmarshal_move.reset();

  auto bind_move =
      [&](std::string_view fact_prefix,
          const std::optional<PreparedI128RuntimeHelper::LaneBinding>& carrier_lane,
          const std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>& abi_register,
          PreparedI128RuntimeHelperMarshalDirection direction,
          std::optional<PreparedI128RuntimeHelper::MarshalingMove>& output) {
        output.reset();
        if (!carrier_lane.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_carrier_lane");
          return;
        }
        if (!abi_register.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_abi_binding");
          return;
        }
        const bool has_register_lane = carrier_lane->register_name.has_value();
        const bool has_memory_lane =
            carrier_lane->slot_id.has_value() &&
            carrier_lane->stack_offset_bytes.has_value();
        if (carrier_lane->carrier_kind == PreparedI128CarrierKind::Missing ||
            (!has_register_lane && !has_memory_lane)) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_complete_carrier_lane");
          return;
        }
        if (abi_register->register_bank != PreparedRegisterBank::Gpr ||
            abi_register->register_class != PreparedRegisterClass::General ||
            abi_register->register_name.empty() ||
            abi_register->occupied_register_names.empty() ||
            abi_register->contiguous_width == 0 ||
            !abi_register->register_placement.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_register_binding");
          return;
        }
        if (carrier_lane->value_id != abi_register->value_id ||
            carrier_lane->value_name != abi_register->value_name ||
            carrier_lane->role != abi_register->role ||
            carrier_lane->lane_index != abi_register->lane_index ||
            carrier_lane->width_bytes != abi_register->width_bytes) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_matching_lane_identity");
          return;
        }
        output = make_i128_helper_marshaling_move(*carrier_lane, *abi_register, direction);
      };

  auto bind_scalar_move =
      [&](std::string_view fact_prefix,
          const std::optional<PreparedI128RuntimeHelper::ScalarValueOwnership>& scalar_value,
          const std::optional<PreparedI128RuntimeHelper::AbiRegisterBinding>& abi_register,
          PreparedI128RuntimeHelperMarshalDirection direction,
          std::optional<PreparedI128RuntimeHelper::ScalarMarshalingMove>& output) {
        output.reset();
        if (!scalar_value.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_scalar_ownership");
          return;
        }
        if (!abi_register.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_abi_binding");
          return;
        }
        const bool has_register_home = scalar_value->register_name.has_value();
        const bool has_memory_home =
            scalar_value->slot_id.has_value() &&
            scalar_value->stack_offset_bytes.has_value();
        if (scalar_value->home_kind == PreparedValueHomeKind::None ||
            (!has_register_home && !has_memory_home)) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_complete_scalar_home");
          return;
        }
        if (abi_register->register_bank != scalar_value->register_bank ||
            abi_register->register_class == PreparedRegisterClass::None ||
            abi_register->register_name.empty() ||
            abi_register->occupied_register_names.empty() ||
            abi_register->contiguous_width == 0 ||
            !abi_register->register_placement.has_value()) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_register_binding");
          return;
        }
        if (scalar_value->value_id != abi_register->value_id ||
            scalar_value->value_name != abi_register->value_name ||
            scalar_value->width_bytes != abi_register->width_bytes) {
          append_i128_runtime_helper_fact(
              helper, std::string(fact_prefix) + "_marshaling_requires_matching_scalar_identity");
          return;
        }
        output =
            make_i128_helper_scalar_marshaling_move(*scalar_value, *abi_register, direction);
      };

  if (helper.helper_family == PreparedI128RuntimeHelperFamily::FloatIntegerConversion) {
    const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
    const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
    if (operand_is_i128) {
      bind_move("operand_low",
                helper.lhs_low_lane,
                helper.lhs_low_abi_argument,
                PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
                helper.lhs_low_argument_move);
      bind_move("operand_high",
                helper.lhs_high_lane,
                helper.lhs_high_abi_argument,
                PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
                helper.lhs_high_argument_move);
    } else {
      bind_scalar_move("operand",
                       helper.scalar_operand,
                       helper.scalar_operand_abi_argument,
                       PreparedI128RuntimeHelperMarshalDirection::ScalarValueToAbiArgument,
                       helper.scalar_operand_argument_move);
    }
    if (result_is_i128) {
      bind_move("result_low",
                helper.result_low_lane,
                helper.result_low_abi_result,
                PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane,
                helper.result_low_unmarshal_move);
      bind_move("result_high",
                helper.result_high_lane,
                helper.result_high_abi_result,
                PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane,
                helper.result_high_unmarshal_move);
    } else {
      bind_scalar_move("result",
                       helper.scalar_result,
                       helper.scalar_result_abi_result,
                       PreparedI128RuntimeHelperMarshalDirection::AbiResultToScalarValue,
                       helper.scalar_result_unmarshal_move);
    }
    return;
  }

  bind_move("lhs_low",
            helper.lhs_low_lane,
            helper.lhs_low_abi_argument,
            PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
            helper.lhs_low_argument_move);
  bind_move("lhs_high",
            helper.lhs_high_lane,
            helper.lhs_high_abi_argument,
            PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
            helper.lhs_high_argument_move);
  bind_move("rhs_low",
            helper.rhs_low_lane,
            helper.rhs_low_abi_argument,
            PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
            helper.rhs_low_argument_move);
  bind_move("rhs_high",
            helper.rhs_high_lane,
            helper.rhs_high_abi_argument,
            PreparedI128RuntimeHelperMarshalDirection::CarrierLaneToAbiArgument,
            helper.rhs_high_argument_move);
  bind_move("result_low",
            helper.result_low_lane,
            helper.result_low_abi_result,
            PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane,
            helper.result_low_unmarshal_move);
  bind_move("result_high",
            helper.result_high_lane,
            helper.result_high_abi_result,
            PreparedI128RuntimeHelperMarshalDirection::AbiResultToCarrierLane,
            helper.result_high_unmarshal_move);
}

void populate_i128_runtime_helper_call_ownership(
    const PreparedBirModule& prepared,
    const PreparedFramePlanFunction* frame_plan,
    const PreparedLivenessFunction* liveness_function,
    const PreparedRegallocFunction* regalloc_function,
    const PreparedValueLocationFunction* value_locations,
    PreparedI128RuntimeHelper& helper) {
  helper.live_preservation_policy = PreparedI128RuntimeHelper::LivePreservationPolicy{};
  helper.selected_call_ownership = PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy{};

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
    append_i128_runtime_helper_fact(
        helper, "live_preservation_requires_structured_live_across_helper_facts");
  } else if (!preserved_values_complete) {
    append_i128_runtime_helper_fact(
        helper, "live_preservation_requires_complete_preserved_value_routes");
  }
  helper.live_preservation_policy = PreparedI128RuntimeHelper::LivePreservationPolicy{
      .evaluated = true,
      .caller_saved_clobbers_modeled = has_clobber_policy,
      .no_additional_live_preservation_required =
          helper_point.has_value() && preserved_values_complete,
      .preserved_values = std::move(preserved_values),
  };

  const bool has_resource_policy =
      helper.resource_policy.call_boundary &&
      helper.resource_policy.runtime_helper_callee &&
      helper.resource_policy.caller_saved_clobbers &&
      helper.resource_policy.preserves_source_operation_identity;
  bool has_abi_bindings = false;
  bool has_marshaling = false;
  const bool has_prepared_abi_contract =
      prepared_i128_runtime_helper_has_abi_contract(helper);
  if (helper.helper_family == PreparedI128RuntimeHelperFamily::FloatIntegerConversion) {
    const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
    const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
    if (operand_is_i128 && !result_is_i128) {
      has_abi_bindings =
          helper.lhs_low_abi_argument.has_value() &&
          helper.lhs_high_abi_argument.has_value() &&
          helper.scalar_result_abi_result.has_value();
      has_marshaling =
          helper.lhs_low_argument_move.has_value() &&
          helper.lhs_high_argument_move.has_value() &&
          helper.scalar_result_unmarshal_move.has_value();
    } else if (!operand_is_i128 && result_is_i128) {
      has_abi_bindings =
          helper.scalar_operand_abi_argument.has_value() &&
          helper.result_low_abi_result.has_value() &&
          helper.result_high_abi_result.has_value();
      has_marshaling =
          helper.scalar_operand_argument_move.has_value() &&
          helper.result_low_unmarshal_move.has_value() &&
          helper.result_high_unmarshal_move.has_value();
    }
  } else if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem) {
    has_abi_bindings =
        helper.lhs_low_abi_argument.has_value() &&
        helper.lhs_high_abi_argument.has_value() &&
        helper.rhs_low_abi_argument.has_value() &&
        helper.rhs_high_abi_argument.has_value() &&
        helper.result_low_abi_result.has_value() &&
        helper.result_high_abi_result.has_value();
    has_marshaling =
        helper.lhs_low_argument_move.has_value() &&
        helper.lhs_high_argument_move.has_value() &&
        helper.rhs_low_argument_move.has_value() &&
        helper.rhs_high_argument_move.has_value() &&
        helper.result_low_unmarshal_move.has_value() &&
        helper.result_high_unmarshal_move.has_value();
  }
  has_abi_bindings = has_abi_bindings && has_prepared_abi_contract;
  const bool has_live_preservation =
      helper.live_preservation_policy.evaluated &&
      helper.live_preservation_policy.caller_saved_clobbers_modeled &&
      helper.live_preservation_policy.no_additional_live_preservation_required;

  helper.selected_call_ownership =
      PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy{
          .owns_terminal_call =
              !helper.callee_name.empty() && has_resource_policy &&
              has_clobber_policy && has_abi_bindings && has_marshaling &&
              has_live_preservation,
          .has_callee_identity = !helper.callee_name.empty(),
          .has_resource_policy = has_resource_policy,
          .has_clobber_policy = has_clobber_policy,
          .has_abi_bindings = has_abi_bindings,
          .has_marshaling = has_marshaling,
          .has_live_preservation = has_live_preservation,
      };

  if (!helper.selected_call_ownership.has_callee_identity) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_callee_identity");
  }
  if (!helper.selected_call_ownership.has_resource_policy) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_resource_policy");
  }
  if (!helper.selected_call_ownership.has_clobber_policy) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_clobber_policy");
  }
  if (!helper.selected_call_ownership.has_abi_bindings) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_abi_bindings");
  }
  if (!has_prepared_abi_contract) {
    append_i128_runtime_helper_fact(
        helper, "selected_call_ownership_requires_prepared_abi_contract");
  }
  if (!helper.selected_call_ownership.has_marshaling) {
    append_i128_runtime_helper_fact(helper, "selected_call_ownership_requires_marshaling");
  }
  if (!helper.selected_call_ownership.has_live_preservation) {
    append_i128_runtime_helper_fact(
        helper, "selected_call_ownership_requires_live_preservation_policy");
  }
}

void populate_i128_runtime_helper_boundary_policy(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocFunction* regalloc_function,
    PreparedI128RuntimeHelper& helper) {
  helper.resource_policy = PreparedI128RuntimeHelper::ResourcePolicy{};
  helper.abi_policy = PreparedI128RuntimeHelper::AbiPolicy{};
  helper.clobbered_registers.clear();

  const bool supported_family =
      helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem ||
      helper.helper_family == PreparedI128RuntimeHelperFamily::FloatIntegerConversion;
  if (!supported_family) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_policy_deferred_for_family");
    return;
  }

  helper.resource_policy = PreparedI128RuntimeHelper::ResourcePolicy{
      .call_boundary = true,
      .runtime_helper_callee = true,
      .caller_saved_clobbers = true,
      .preserves_source_operation_identity = true,
  };
  helper.clobbered_registers = build_call_clobber_set(target_profile, regalloc_function);

  if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem) {
    helper.abi_policy = PreparedI128RuntimeHelper::AbiPolicy{
        .transition = PreparedI128RuntimeHelperAbiTransition::DirectRegisterPairArgumentsAndResult,
        .argument_bank = PreparedRegisterBank::Gpr,
        .result_bank = PreparedRegisterBank::Gpr,
        .argument_count = 2,
        .lanes_per_argument = 2,
        .result_lane_count = 2,
        .lane_width_bytes = 8,
    };
  }

  if (helper.callee_name.empty()) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_requires_callee_identity");
  }
  if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem &&
      (helper.abi_policy.argument_bank == PreparedRegisterBank::None ||
       helper.abi_policy.result_bank == PreparedRegisterBank::None ||
       helper.abi_policy.argument_count != 2 ||
       helper.abi_policy.lanes_per_argument != 2 ||
       helper.abi_policy.result_lane_count != 2 ||
       helper.abi_policy.lane_width_bytes != 8)) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_requires_direct_gpr_pair_abi");
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_requires_resource_policy");
  }
  if (helper.clobbered_registers.empty()) {
    append_i128_runtime_helper_fact(helper, "i128_helper_boundary_requires_caller_saved_clobbers");
  }
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

}  // namespace

bool prepared_i128_runtime_helper_has_abi_contract(
    const PreparedI128RuntimeHelper& helper) {
  if (helper.callee_name.empty()) {
    return false;
  }

  if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem) {
    const bool div_rem_opcode =
        helper.source_binary_opcode == bir::BinaryOpcode::SDiv ||
        helper.source_binary_opcode == bir::BinaryOpcode::UDiv ||
        helper.source_binary_opcode == bir::BinaryOpcode::SRem ||
        helper.source_binary_opcode == bir::BinaryOpcode::URem;
    return div_rem_opcode &&
           helper.source_type == bir::TypeKind::I128 &&
           helper.result_type == bir::TypeKind::I128 &&
           helper.result_ownership ==
               PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes &&
           helper.abi_policy.transition ==
               PreparedI128RuntimeHelperAbiTransition::
                   DirectRegisterPairArgumentsAndResult &&
           helper.abi_policy.argument_bank == PreparedRegisterBank::Gpr &&
           helper.abi_policy.result_bank == PreparedRegisterBank::Gpr &&
           helper.abi_policy.argument_count == 2 &&
           helper.abi_policy.lanes_per_argument == 2 &&
           helper.abi_policy.result_lane_count == 2 &&
           helper.abi_policy.lane_width_bytes == 8;
  }

  if (helper.helper_family != PreparedI128RuntimeHelperFamily::FloatIntegerConversion ||
      !helper.source_cast_opcode.has_value()) {
    return false;
  }

  const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
  const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
  const bool source_is_supported_scalar =
      helper.source_type == bir::TypeKind::F32 ||
      helper.source_type == bir::TypeKind::F64;
  const bool result_is_supported_scalar =
      helper.result_type == bir::TypeKind::F32 ||
      helper.result_type == bir::TypeKind::F64;
  if (operand_is_i128 == result_is_i128 ||
      (!operand_is_i128 && !source_is_supported_scalar) ||
      (!result_is_i128 && !result_is_supported_scalar)) {
    return false;
  }

  if (operand_is_i128) {
    return helper.result_ownership ==
               PreparedI128RuntimeHelperResultOwnership::ScalarValue &&
           helper.abi_policy.transition ==
               PreparedI128RuntimeHelperAbiTransition::
                   RegisterPairArgumentAndScalarResult &&
           helper.abi_policy.argument_bank == PreparedRegisterBank::Gpr &&
           helper.abi_policy.result_bank == PreparedRegisterBank::Fpr &&
           helper.abi_policy.argument_count == 1 &&
           helper.abi_policy.lanes_per_argument == 2 &&
           helper.abi_policy.result_lane_count == 1 &&
           helper.abi_policy.lane_width_bytes == 8 &&
           helper.source_width_bytes == 16 &&
           (helper.result_width_bytes == 4 || helper.result_width_bytes == 8);
  }

  return helper.result_ownership ==
             PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes &&
         helper.abi_policy.transition ==
             PreparedI128RuntimeHelperAbiTransition::
                 ScalarArgumentAndRegisterPairResult &&
         helper.abi_policy.argument_bank == PreparedRegisterBank::Fpr &&
         helper.abi_policy.result_bank == PreparedRegisterBank::Gpr &&
         helper.abi_policy.argument_count == 1 &&
         helper.abi_policy.lanes_per_argument == 1 &&
         helper.abi_policy.result_lane_count == 2 &&
         helper.abi_policy.lane_width_bytes == 8 &&
         (helper.source_width_bytes == 4 || helper.source_width_bytes == 8) &&
         helper.result_width_bytes == 16;
}

void populate_i128_runtime_helper_lanes(PreparedBirModule& prepared) {
  for (auto& function_helpers : prepared.i128_runtime_helpers.functions) {
    const auto* function_carriers =
        find_prepared_i128_carriers(prepared.i128_carriers, function_helpers.function_name);
    const auto* regalloc_function =
        find_regalloc_function(prepared.regalloc, function_helpers.function_name);
    const auto* frame_plan = find_prepared_frame_plan(prepared, function_helpers.function_name);
    const auto* liveness_function =
        find_liveness_function(prepared.liveness, function_helpers.function_name);
    const auto* value_locations =
        find_prepared_value_location_function(prepared, function_helpers.function_name);
    for (auto& helper : function_helpers.helpers) {
      helper.lhs_low_lane.reset();
      helper.lhs_high_lane.reset();
      helper.rhs_low_lane.reset();
      helper.rhs_high_lane.reset();
      helper.result_low_lane.reset();
      helper.result_high_lane.reset();
      helper.lhs_low_argument_move.reset();
      helper.lhs_high_argument_move.reset();
      helper.rhs_low_argument_move.reset();
      helper.rhs_high_argument_move.reset();
      helper.result_low_unmarshal_move.reset();
      helper.result_high_unmarshal_move.reset();
      helper.scalar_operand_argument_move.reset();
      helper.scalar_result_unmarshal_move.reset();
      helper.memory_return.reset();
      helper.scalar_operand.reset();
      helper.scalar_result.reset();
      helper.missing_required_facts.clear();
      populate_i128_runtime_helper_boundary_policy(
          prepared.target_profile, regalloc_function, helper);

      if (helper.helper_family == PreparedI128RuntimeHelperFamily::DivRem) {
        populate_i128_helper_lanes_from_carrier(function_carriers,
                                               helper,
                                               helper.lhs_value_id,
                                               helper.lhs_low_lane,
                                               helper.lhs_high_lane,
                                               "lhs");
        populate_i128_helper_lanes_from_carrier(function_carriers,
                                               helper,
                                               helper.rhs_value_id,
                                               helper.rhs_low_lane,
                                               helper.rhs_high_lane,
                                               "rhs");
        populate_i128_helper_lanes_from_carrier(function_carriers,
                                               helper,
                                               helper.result_value_id,
                                               helper.result_low_lane,
                                               helper.result_high_lane,
                                               "result");
      } else if (helper.helper_family ==
                 PreparedI128RuntimeHelperFamily::FloatIntegerConversion) {
        const bool operand_is_i128 = helper.source_type == bir::TypeKind::I128;
        const bool result_is_i128 = helper.result_type == bir::TypeKind::I128;
        if (operand_is_i128) {
          populate_i128_helper_lanes_from_carrier(function_carriers,
                                                 helper,
                                                 helper.operand_value_id,
                                                 helper.lhs_low_lane,
                                                 helper.lhs_high_lane,
                                                 "operand");
        } else {
          helper.scalar_operand =
              make_i128_helper_scalar_ownership(helper,
                                                regalloc_function,
                                                value_locations,
                                                helper.operand_value_id,
                                                helper.operand_value_name,
                                                helper.source_type,
                                                helper.source_width_bytes,
                                                PreparedRegisterBank::Fpr,
                                                "operand");
        }
        if (result_is_i128) {
          populate_i128_helper_lanes_from_carrier(function_carriers,
                                                 helper,
                                                 helper.result_value_id,
                                                 helper.result_low_lane,
                                                 helper.result_high_lane,
                                                 "result");
        } else {
          helper.scalar_result =
              make_i128_helper_scalar_ownership(helper,
                                                regalloc_function,
                                                value_locations,
                                                helper.result_value_id,
                                                helper.result_value_name,
                                                helper.result_type,
                                                helper.result_width_bytes,
                                                PreparedRegisterBank::Fpr,
                                                "result");
        }
      }
      populate_i128_runtime_helper_abi_bindings(prepared.target_profile, helper);
      switch (helper.result_ownership) {
        case PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes:
          if (!helper.result_low_lane.has_value() || !helper.result_high_lane.has_value()) {
            append_i128_runtime_helper_fact(
                helper, "direct_result_requires_low_high_result_lanes");
          }
          break;
        case PreparedI128RuntimeHelperResultOwnership::ScalarValue:
          if (!helper.scalar_result.has_value()) {
            append_i128_runtime_helper_fact(
                helper, "scalar_result_requires_scalar_ownership");
          }
          break;
        case PreparedI128RuntimeHelperResultOwnership::MemoryReturn:
          append_i128_runtime_helper_fact(
              helper, "memory_return_ownership_requires_explicit_destination");
          break;
        case PreparedI128RuntimeHelperResultOwnership::Missing:
          append_i128_runtime_helper_fact(
              helper, "missing_i128_helper_result_ownership_policy");
          break;
      }
      populate_i128_runtime_helper_marshaling(helper);
      populate_i128_runtime_helper_call_ownership(prepared,
                                                  frame_plan,
                                                  liveness_function,
                                                  regalloc_function,
                                                  value_locations,
                                                  helper);
    }
  }
}

}  // namespace c4c::backend::prepare
