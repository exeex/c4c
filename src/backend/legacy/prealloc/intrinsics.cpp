#include "intrinsics.hpp"

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] std::optional<ValueNameId> maybe_intrinsic_named_value_id(
    const PreparedNameTables& names,
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

[[nodiscard]] std::optional<PreparedValueHome> prepared_intrinsic_home_for_named_value(
    PreparedNameTables& names,
    const PreparedValueLocationFunction* value_locations,
    const bir::Value& value) {
  if (value_locations == nullptr) {
    return std::nullopt;
  }
  const auto value_name = maybe_intrinsic_named_value_id(names, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home = find_prepared_value_home(*value_locations, *value_name);
  if (home == nullptr) {
    return std::nullopt;
  }
  return *home;
}

void append_intrinsic_missing_fact(PreparedIntrinsicCarrierFunction& function_carriers,
                                   PreparedIntrinsicCarrier& carrier,
                                   std::string fact) {
  carrier.missing_required_facts.push_back(fact);
  function_carriers.missing_required_facts.push_back(
      "inst#" + std::to_string(carrier.inst_index) + ":" + std::move(fact));
}

[[nodiscard]] std::optional<ValueNameId> prepared_intrinsic_named_value_id(
    PreparedNameTables& names,
    const std::optional<bir::Value>& value) {
  if (!value.has_value()) {
    return std::nullopt;
  }
  return prepared_named_value_id(names, *value);
}

[[nodiscard]] std::vector<ValueNameId> prepared_intrinsic_named_value_ids(
    PreparedNameTables& names,
    const std::vector<bir::Value>& values) {
  std::vector<ValueNameId> ids;
  ids.reserve(values.size());
  for (const auto& value : values) {
    const auto id = prepared_named_value_id(names, value);
    ids.push_back(id.value_or(kInvalidValueName));
  }
  return ids;
}

[[nodiscard]] std::vector<std::optional<PreparedValueHome>>
prepared_intrinsic_operand_homes(
    PreparedNameTables& names,
    const PreparedValueLocationFunction* value_locations,
    const std::vector<bir::Value>& values) {
  std::vector<std::optional<PreparedValueHome>> homes;
  homes.reserve(values.size());
  for (const auto& value : values) {
    homes.push_back(prepared_intrinsic_home_for_named_value(names, value_locations, value));
  }
  return homes;
}

[[nodiscard]] const PreparedCallPlan* find_call_plan_for_instruction(
    const PreparedCallPlansFunction* function_call_plans,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (function_call_plans == nullptr) {
    return nullptr;
  }
  for (const auto& call_plan : function_call_plans->calls) {
    if (call_plan.block_index == block_index &&
        call_plan.instruction_index == instruction_index) {
      return &call_plan;
    }
  }
  return nullptr;
}

[[nodiscard]] bool intrinsic_roles_are(
    const bir::IntrinsicOperation& intrinsic,
    std::initializer_list<bir::IntrinsicOperandRole> expected) {
  return intrinsic.operand_roles.size() == expected.size() &&
         std::equal(intrinsic.operand_roles.begin(),
                    intrinsic.operand_roles.end(),
                    expected.begin());
}

[[nodiscard]] bool has_intrinsic_operand_home(
    const PreparedIntrinsicCarrier& carrier,
    std::size_t index) {
  return index < carrier.operand_homes.size() &&
         carrier.operand_homes[index].has_value();
}

void require_intrinsic_value_homes(PreparedIntrinsicCarrierFunction& function_carriers,
                                   PreparedIntrinsicCarrier& carrier,
                                   std::size_t operand_count,
                                   bool requires_result) {
  for (std::size_t index = 0; index < operand_count; ++index) {
    if (!has_intrinsic_operand_home(carrier, index)) {
      append_intrinsic_missing_fact(
          function_carriers,
          carrier,
          "missing_operand" + std::to_string(index) + "_home");
    }
  }
  if (requires_result && !carrier.result_home.has_value()) {
    append_intrinsic_missing_fact(function_carriers, carrier, "missing_result_home");
  }
}

void require_intrinsic_call_plan_shape(PreparedIntrinsicCarrierFunction& function_carriers,
                                       PreparedIntrinsicCarrier& carrier,
                                       const PreparedCallPlan* call_plan,
                                       std::size_t operand_count,
                                       bool requires_result) {
  if (!carrier.has_prepared_call_plan) {
    append_intrinsic_missing_fact(function_carriers, carrier, "missing_prepared_call_plan");
    return;
  }
  if (call_plan->arguments.size() != operand_count) {
    append_intrinsic_missing_fact(
        function_carriers,
        carrier,
        "prepared_call_plan_requires_" + std::to_string(operand_count) + "_arguments");
  }
  if (requires_result && !call_plan->result.has_value()) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "prepared_call_plan_requires_result");
  }
  if (!requires_result && call_plan->result.has_value()) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "prepared_call_plan_requires_void_result");
  }
}

void validate_scalar_fp_unary_intrinsic(PreparedIntrinsicCarrierFunction& function_carriers,
                                        PreparedIntrinsicCarrier& carrier,
                                        const bir::CallInst& call,
                                        const PreparedCallPlan* call_plan) {
  if (carrier.operation != bir::IntrinsicOperationKind::FAbs) {
    append_intrinsic_missing_fact(function_carriers, carrier, "unsupported_intrinsic_operation");
  }
  if (call.args.size() != 1 || call.arg_types.size() != 1) {
    append_intrinsic_missing_fact(function_carriers, carrier, "scalar_fp_unary_requires_one_operand");
  }
  if (!carrier.result.has_value()) {
    append_intrinsic_missing_fact(function_carriers, carrier, "scalar_fp_unary_requires_result");
  }
  if (carrier.operand_type != carrier.result_type) {
    append_intrinsic_missing_fact(function_carriers, carrier, "scalar_fp_unary_requires_matching_types");
  }
  if (carrier.operand_type != bir::TypeKind::F32 &&
      carrier.operand_type != bir::TypeKind::F64) {
    append_intrinsic_missing_fact(function_carriers, carrier, "unsupported_scalar_fp_unary_type");
  }
  require_intrinsic_call_plan_shape(function_carriers, carrier, call_plan, 1, true);
  require_intrinsic_value_homes(function_carriers, carrier, 1, true);
}

void validate_crc_intrinsic(PreparedIntrinsicCarrierFunction& function_carriers,
                            PreparedIntrinsicCarrier& carrier,
                            const bir::IntrinsicOperation& intrinsic,
                            const bir::CallInst& call,
                            const PreparedCallPlan* call_plan) {
  if (carrier.operation != bir::IntrinsicOperationKind::Crc32W) {
    append_intrinsic_missing_fact(function_carriers, carrier, "unsupported_crc_operation");
  }
  if (intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Crc) {
    append_intrinsic_missing_fact(function_carriers, carrier, "crc_requires_aarch64_crc_feature");
  }
  if (call.args.size() != 2 || call.arg_types.size() != 2 ||
      !intrinsic_roles_are(intrinsic,
                           {bir::IntrinsicOperandRole::Accumulator,
                            bir::IntrinsicOperandRole::Data})) {
    append_intrinsic_missing_fact(function_carriers, carrier, "crc32w_requires_accumulator_and_data");
  }
  if (!carrier.result.has_value()) {
    append_intrinsic_missing_fact(function_carriers, carrier, "crc32w_requires_result");
  }
  if (carrier.operand_type != bir::TypeKind::I32 ||
      carrier.result_type != bir::TypeKind::I32) {
    append_intrinsic_missing_fact(function_carriers, carrier, "crc32w_requires_i32_types");
  }
  if (intrinsic.signedness != bir::IntrinsicSignedness::Unsigned) {
    append_intrinsic_missing_fact(function_carriers, carrier, "crc32w_requires_unsigned_semantics");
  }
  if (intrinsic.memory_operand.has_value() ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      carrier.has_side_effects) {
    append_intrinsic_missing_fact(function_carriers, carrier, "crc32w_requires_pure_register_semantics");
  }
  require_intrinsic_call_plan_shape(function_carriers, carrier, call_plan, 2, true);
  require_intrinsic_value_homes(function_carriers, carrier, 2, true);
}

void validate_vector_shape(PreparedIntrinsicCarrierFunction& function_carriers,
                           PreparedIntrinsicCarrier& carrier,
                           const bir::IntrinsicOperation& intrinsic,
                           std::string_view fact_prefix) {
  if (intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Neon) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, std::string(fact_prefix) + "_requires_aarch64_neon_feature");
  }
  if (intrinsic.vector_element_type != bir::TypeKind::I8 ||
      intrinsic.vector_element_width_bytes != 1 ||
      intrinsic.vector_lane_count != 16 ||
      intrinsic.vector_total_width_bytes != 16) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, std::string(fact_prefix) + "_requires_v16i8_shape");
  }
  if (intrinsic.signedness != bir::IntrinsicSignedness::Unsigned) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, std::string(fact_prefix) + "_requires_unsigned_semantics");
  }
}

void validate_vector_memory_intrinsic(PreparedIntrinsicCarrierFunction& function_carriers,
                                      PreparedIntrinsicCarrier& carrier,
                                      const bir::IntrinsicOperation& intrinsic,
                                      const bir::CallInst& call,
                                      const PreparedCallPlan* call_plan) {
  if (carrier.operation != bir::IntrinsicOperationKind::VectorLoad) {
    append_intrinsic_missing_fact(function_carriers, carrier, "unsupported_vector_memory_operation");
  }
  validate_vector_shape(function_carriers, carrier, intrinsic, "vector_load");
  if (call.args.size() != 1 || call.arg_types.size() != 1 ||
      !intrinsic_roles_are(intrinsic, {bir::IntrinsicOperandRole::Pointer})) {
    append_intrinsic_missing_fact(function_carriers, carrier, "vector_load_requires_pointer_operand");
  }
  if (!carrier.result.has_value()) {
    append_intrinsic_missing_fact(function_carriers, carrier, "vector_load_requires_result");
  }
  if (carrier.operand_type != bir::TypeKind::Ptr ||
      carrier.result_type != bir::TypeKind::I128) {
    append_intrinsic_missing_fact(function_carriers, carrier, "vector_load_requires_ptr_to_i128_types");
  }
  if (!intrinsic.memory_operand.has_value() ||
      intrinsic.memory_operand->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      intrinsic.memory_operand->size_bytes != 16 ||
      intrinsic.memory_operand->align_bytes != 16 ||
      intrinsic.memory_operand->is_volatile ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::Read ||
      carrier.has_side_effects) {
    append_intrinsic_missing_fact(function_carriers, carrier, "vector_load_requires_read_memory_facts");
  }
  require_intrinsic_call_plan_shape(function_carriers, carrier, call_plan, 1, true);
  require_intrinsic_value_homes(function_carriers, carrier, 1, true);
}

void validate_vector_operation_intrinsic(PreparedIntrinsicCarrierFunction& function_carriers,
                                         PreparedIntrinsicCarrier& carrier,
                                         const bir::IntrinsicOperation& intrinsic,
                                         const bir::CallInst& call,
                                         const PreparedCallPlan* call_plan) {
  if (carrier.operation != bir::IntrinsicOperationKind::VectorAdd) {
    append_intrinsic_missing_fact(function_carriers, carrier, "unsupported_vector_operation");
  }
  validate_vector_shape(function_carriers, carrier, intrinsic, "vector_add");
  if (call.args.size() != 2 || call.arg_types.size() != 2 ||
      !intrinsic_roles_are(intrinsic,
                           {bir::IntrinsicOperandRole::VectorLhs,
                            bir::IntrinsicOperandRole::VectorRhs})) {
    append_intrinsic_missing_fact(function_carriers, carrier, "vector_add_requires_lhs_and_rhs");
  }
  if (!carrier.result.has_value()) {
    append_intrinsic_missing_fact(function_carriers, carrier, "vector_add_requires_result");
  }
  if (carrier.operand_type != bir::TypeKind::I128 ||
      carrier.result_type != bir::TypeKind::I128) {
    append_intrinsic_missing_fact(function_carriers, carrier, "vector_add_requires_i128_types");
  }
  if (intrinsic.memory_operand.has_value() ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      carrier.has_side_effects) {
    append_intrinsic_missing_fact(function_carriers, carrier, "vector_add_requires_pure_vector_semantics");
  }
  require_intrinsic_call_plan_shape(function_carriers, carrier, call_plan, 2, true);
  require_intrinsic_value_homes(function_carriers, carrier, 2, true);
}

void validate_barrier_intrinsic(PreparedIntrinsicCarrierFunction& function_carriers,
                                PreparedIntrinsicCarrier& carrier,
                                const bir::IntrinsicOperation& intrinsic,
                                const bir::CallInst& call,
                                const PreparedCallPlan* call_plan) {
  if (carrier.operation != bir::IntrinsicOperationKind::BarrierDmb) {
    append_intrinsic_missing_fact(function_carriers, carrier, "unsupported_barrier_operation");
  }
  if (intrinsic.required_feature != bir::IntrinsicFeatureKind::None) {
    append_intrinsic_missing_fact(function_carriers, carrier, "barrier_requires_no_target_feature");
  }
  if (call.args.size() != 1 || call.arg_types.size() != 1 ||
      !intrinsic_roles_are(intrinsic, {bir::IntrinsicOperandRole::BarrierDomain})) {
    append_intrinsic_missing_fact(function_carriers, carrier, "barrier_dmb_requires_domain_operand");
  }
  if (carrier.result.has_value() || call.return_type != bir::TypeKind::Void) {
    append_intrinsic_missing_fact(function_carriers, carrier, "barrier_dmb_requires_void_result");
  }
  if (carrier.operand_type != bir::TypeKind::I32 ||
      carrier.result_type != bir::TypeKind::Void) {
    append_intrinsic_missing_fact(function_carriers, carrier, "barrier_dmb_requires_i32_to_void_types");
  }
  if (intrinsic.barrier_domain != bir::IntrinsicBarrierDomainKind::Sy) {
    append_intrinsic_missing_fact(function_carriers, carrier, "barrier_dmb_requires_sy_domain");
  }
  if (!intrinsic.has_immediate_operand || !intrinsic.requires_immediate_operand ||
      !intrinsic.immediate_value.has_value() || *intrinsic.immediate_value != 15) {
    append_intrinsic_missing_fact(function_carriers, carrier, "barrier_dmb_requires_immediate_15");
  }
  if (intrinsic.memory_operand.has_value() ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      !carrier.has_side_effects) {
    append_intrinsic_missing_fact(function_carriers, carrier, "barrier_dmb_requires_side_effect_only_semantics");
  }
  require_intrinsic_call_plan_shape(function_carriers, carrier, call_plan, 1, false);
}

void validate_cache_maintenance_intrinsic(
    PreparedIntrinsicCarrierFunction& function_carriers,
    PreparedIntrinsicCarrier& carrier,
    const bir::IntrinsicOperation& intrinsic,
    const bir::CallInst& call,
    const PreparedCallPlan* call_plan) {
  if (carrier.operation != bir::IntrinsicOperationKind::CacheDcCvau) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "unsupported_cache_maintenance_operation");
  }
  if (intrinsic.required_feature != bir::IntrinsicFeatureKind::None) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "cache_maintenance_requires_no_target_feature");
  }
  if (call.args.size() != 1 || call.arg_types.size() != 1 ||
      !intrinsic_roles_are(intrinsic, {bir::IntrinsicOperandRole::CacheAddress})) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "cache_dc_cvau_requires_address_operand");
  }
  if (carrier.result.has_value() || call.return_type != bir::TypeKind::Void) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "cache_dc_cvau_requires_void_result");
  }
  if (carrier.operand_type != bir::TypeKind::Ptr ||
      carrier.result_type != bir::TypeKind::Void) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "cache_dc_cvau_requires_ptr_to_void_types");
  }
  if (!intrinsic.memory_operand.has_value() ||
      intrinsic.memory_operand->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      intrinsic.memory_operand->address_space != bir::AddressSpace::Default ||
      intrinsic.memory_operand->size_bytes != 0 ||
      intrinsic.memory_operand->align_bytes != 1 ||
      intrinsic.memory_operand->is_volatile ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      !carrier.has_side_effects) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "cache_dc_cvau_requires_address_side_effect_facts");
  }
  if (intrinsic.has_immediate_operand || intrinsic.requires_immediate_operand ||
      intrinsic.immediate_value.has_value()) {
    append_intrinsic_missing_fact(
        function_carriers, carrier, "cache_dc_cvau_requires_no_immediate");
  }
  require_intrinsic_call_plan_shape(function_carriers, carrier, call_plan, 1, false);
  require_intrinsic_value_homes(function_carriers, carrier, 1, false);
}

void validate_pause_hint_intrinsic(
    PreparedIntrinsicCarrierFunction& function_carriers,
    PreparedIntrinsicCarrier& carrier,
    const bir::IntrinsicOperation& intrinsic,
    const bir::CallInst& call,
    const PreparedCallPlan* call_plan) {
  if (carrier.operation != bir::IntrinsicOperationKind::HintYield) {
    append_intrinsic_missing_fact(function_carriers, carrier, "unsupported_pause_hint_operation");
  }
  if (intrinsic.required_feature != bir::IntrinsicFeatureKind::None) {
    append_intrinsic_missing_fact(function_carriers, carrier, "pause_hint_requires_no_target_feature");
  }
  if (call.args.size() != 1 || call.arg_types.size() != 1 ||
      !intrinsic_roles_are(intrinsic, {bir::IntrinsicOperandRole::HintImmediate})) {
    append_intrinsic_missing_fact(function_carriers, carrier, "hint_yield_requires_immediate_operand");
  }
  if (carrier.result.has_value() || call.return_type != bir::TypeKind::Void) {
    append_intrinsic_missing_fact(function_carriers, carrier, "hint_yield_requires_void_result");
  }
  if (carrier.operand_type != bir::TypeKind::I32 ||
      carrier.result_type != bir::TypeKind::Void) {
    append_intrinsic_missing_fact(function_carriers, carrier, "hint_yield_requires_i32_to_void_types");
  }
  if (!intrinsic.has_immediate_operand || !intrinsic.requires_immediate_operand ||
      !intrinsic.immediate_value.has_value() || *intrinsic.immediate_value != 1) {
    append_intrinsic_missing_fact(function_carriers, carrier, "hint_yield_requires_immediate_1");
  }
  if (intrinsic.memory_operand.has_value() ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      intrinsic.barrier_domain != bir::IntrinsicBarrierDomainKind::None ||
      !carrier.has_side_effects) {
    append_intrinsic_missing_fact(function_carriers, carrier, "hint_yield_requires_side_effect_only_semantics");
  }
  require_intrinsic_call_plan_shape(function_carriers, carrier, call_plan, 1, false);
}

[[nodiscard]] PreparedIntrinsicCarrier build_intrinsic_carrier(
    PreparedNameTables& names,
    PreparedIntrinsicCarrierFunction& function_carriers,
    const bir::CallInst& call,
    std::size_t block_index,
    std::size_t instruction_index,
    const PreparedValueLocationFunction* value_locations,
    const PreparedCallPlan* call_plan) {
  const bir::IntrinsicOperation intrinsic =
      call.intrinsic.value_or(bir::IntrinsicOperation{});
  PreparedIntrinsicCarrier carrier{
      .function_name = function_carriers.function_name,
      .carrier_kind = PreparedIntrinsicCarrierKind::Missing,
      .family = intrinsic.family,
      .operation = intrinsic.operation,
      .required_feature = intrinsic.required_feature,
      .block_index = block_index,
      .inst_index = instruction_index,
      .operand_type = intrinsic.operand_type,
      .result_type = intrinsic.result_type,
      .operand_roles = intrinsic.operand_roles,
      .vector_element_type = intrinsic.vector_element_type,
      .vector_element_width_bytes = intrinsic.vector_element_width_bytes,
      .vector_lane_count = intrinsic.vector_lane_count,
      .vector_total_width_bytes = intrinsic.vector_total_width_bytes,
      .signedness = intrinsic.signedness,
      .memory_operand = intrinsic.memory_operand,
      .memory_access = intrinsic.memory_access,
      .barrier_domain = intrinsic.barrier_domain,
      .has_immediate_operand = intrinsic.has_immediate_operand,
      .requires_immediate_operand = intrinsic.requires_immediate_operand,
      .immediate_value = intrinsic.immediate_value,
      .operand = call.args.empty() ? std::nullopt
                                   : std::optional<bir::Value>{call.args.front()},
      .operands = call.args,
      .result = call.result,
      .operand_value_name = prepared_intrinsic_named_value_id(
          names, call.args.empty() ? std::nullopt
                                   : std::optional<bir::Value>{call.args.front()}),
      .operand_value_names = prepared_intrinsic_named_value_ids(names, call.args),
      .result_value_name = prepared_intrinsic_named_value_id(names, call.result),
      .operand_homes = prepared_intrinsic_operand_homes(names, value_locations, call.args),
      .result_home = call.result.has_value()
                         ? prepared_intrinsic_home_for_named_value(names, value_locations, *call.result)
                         : std::nullopt,
      .has_side_effects = intrinsic.has_side_effects,
      .requires_feature = intrinsic.required_feature != bir::IntrinsicFeatureKind::None,
      .source_callee_name = call.callee.empty()
                                ? std::nullopt
                                : std::optional<std::string>{call.callee},
      .has_prepared_call_plan = call_plan != nullptr,
      .missing_required_facts = {},
  };

  if (!call.intrinsic.has_value()) {
    append_intrinsic_missing_fact(function_carriers, carrier, "missing_intrinsic_operation");
  }

  switch (carrier.family) {
    case bir::IntrinsicFamilyKind::ScalarFpUnary:
      validate_scalar_fp_unary_intrinsic(function_carriers, carrier, call, call_plan);
      break;
    case bir::IntrinsicFamilyKind::Crc:
      validate_crc_intrinsic(function_carriers, carrier, intrinsic, call, call_plan);
      break;
    case bir::IntrinsicFamilyKind::VectorMemory:
      validate_vector_memory_intrinsic(function_carriers, carrier, intrinsic, call, call_plan);
      break;
    case bir::IntrinsicFamilyKind::VectorOperation:
      validate_vector_operation_intrinsic(function_carriers, carrier, intrinsic, call, call_plan);
      break;
    case bir::IntrinsicFamilyKind::Barrier:
      validate_barrier_intrinsic(function_carriers, carrier, intrinsic, call, call_plan);
      break;
    case bir::IntrinsicFamilyKind::CacheMaintenance:
      validate_cache_maintenance_intrinsic(
          function_carriers, carrier, intrinsic, call, call_plan);
      break;
    case bir::IntrinsicFamilyKind::PauseHint:
      validate_pause_hint_intrinsic(function_carriers, carrier, intrinsic, call, call_plan);
      break;
    case bir::IntrinsicFamilyKind::None:
      append_intrinsic_missing_fact(function_carriers, carrier, "unsupported_intrinsic_family");
      break;
  }
  if (carrier.missing_required_facts.empty()) {
    carrier.carrier_kind = PreparedIntrinsicCarrierKind::Complete;
  }
  return carrier;
}

}  // namespace

void populate_intrinsic_carriers(PreparedBirModule& prepared) {
  prepared.intrinsic_carriers.functions.clear();

  for (const auto& function : prepared.module.functions) {
    const FunctionNameId function_name =
        prepared.names.function_names.find(function.name);
    if (function_name == kInvalidFunctionName) {
      continue;
    }
    const auto* function_call_plans =
        find_prepared_call_plans(prepared.call_plans, function_name);
    const auto* value_locations =
        find_prepared_value_location_function(prepared, function_name);
    PreparedIntrinsicCarrierFunction function_carriers{
        .function_name = function_name,
        .carriers = {},
        .missing_required_facts = {},
    };

    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
           ++instruction_index) {
        const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
        if (call == nullptr || !call->intrinsic.has_value()) {
          continue;
        }
        const auto* call_plan =
            find_call_plan_for_instruction(function_call_plans, block_index, instruction_index);
        function_carriers.carriers.push_back(
            build_intrinsic_carrier(prepared.names,
                                    function_carriers,
                                    *call,
                                    block_index,
                                    instruction_index,
                                    value_locations,
                                    call_plan));
      }
    }
    if (!function_carriers.carriers.empty() ||
        !function_carriers.missing_required_facts.empty()) {
      prepared.intrinsic_carriers.functions.push_back(std::move(function_carriers));
    }
  }
}

}  // namespace c4c::backend::prepare
