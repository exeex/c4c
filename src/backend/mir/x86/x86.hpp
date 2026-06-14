#pragma once

#include "../../prealloc/prealloc.hpp"
#include "../../prealloc/prepared_lookups.hpp"
#include "abi/abi.hpp"
#include "debug/debug.hpp"
#include "lowering/lowering.hpp"

#include <optional>
#include <string_view>

namespace c4c::backend::x86 {

struct ConsumedPlans {
  const c4c::backend::prepare::PreparedFramePlanFunction* frame = nullptr;
  const c4c::backend::prepare::PreparedDynamicStackPlanFunction* dynamic_stack = nullptr;
  const c4c::backend::prepare::PreparedControlFlowFunction* control_flow = nullptr;
  const c4c::backend::prepare::PreparedCallPlansFunction* calls = nullptr;
  const c4c::backend::prepare::PreparedRegallocFunction* regalloc = nullptr;
  const c4c::backend::prepare::PreparedStoragePlanFunction* storage = nullptr;
  std::optional<c4c::backend::prepare::PreparedFunctionLookups> prepared_lookups;
  std::optional<c4c::backend::bir::Route6CallUseSourceIndex> route6_call_use_sources;

  [[nodiscard]] const c4c::backend::prepare::PreparedFunctionLookups*
  shared_function_lookups() const {
    return prepared_lookups.has_value() ? &*prepared_lookups : nullptr;
  }

  [[nodiscard]] const c4c::backend::prepare::PreparedCallPlanLookups*
  shared_call_plan_lookups() const {
    const auto* lookups = shared_function_lookups();
    return lookups != nullptr ? &lookups->call_plans : nullptr;
  }

  [[nodiscard]] const c4c::backend::bir::Route6CallUseSourceIndex*
  shared_route6_call_use_source_index() const {
    return route6_call_use_sources.has_value() ? &*route6_call_use_sources : nullptr;
  }
};

struct ConsumedScalarI32CallArgumentSourceAuthority {
  c4c::backend::bir::Route6CallArgumentSourceRecord route6_source;
  std::string_view source_name;
};

[[nodiscard]] inline const c4c::backend::bir::Function* find_consumed_bir_function(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name) {
  const auto function_spelling =
      c4c::backend::prepare::prepared_function_name(module.names, function_name);
  for (const auto& function : module.module.functions) {
    if (function.name == function_spelling) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallPlan* find_consumed_call_plan(
    const ConsumedPlans& consumed,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (consumed.calls == nullptr) {
    return nullptr;
  }
  return c4c::backend::prepare::find_indexed_prepared_call_plan(
      consumed.shared_call_plan_lookups(), consumed.calls, block_index, instruction_index);
}

[[nodiscard]] inline std::optional<c4c::backend::prepare::PreparedFunctionLookups>
consume_prepared_function_lookups(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name) {
  const auto* control_flow =
      c4c::backend::prepare::find_prepared_control_flow_function(module.control_flow,
                                                                 function_name);
  if (control_flow == nullptr) {
    return std::nullopt;
  }
  return c4c::backend::prepare::make_prepared_function_lookups(module, *control_flow);
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallArgumentPlan*
find_consumed_call_argument_plan(const ConsumedPlans& consumed,
                                 std::size_t block_index,
                                 std::size_t instruction_index,
                                 std::size_t arg_index) {
  const auto* call = find_consumed_call_plan(consumed, block_index, instruction_index);
  if (call == nullptr) {
    return nullptr;
  }
  for (const auto& argument : call->arguments) {
    if (argument.arg_index == arg_index) {
      return &argument;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::optional<ConsumedScalarI32CallArgumentSourceAuthority>
find_consumed_scalar_i32_call_argument_source_authority(
    const ConsumedPlans& consumed,
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index,
    const c4c::backend::bir::Value& argument) {
  const auto* prepared_argument =
      find_consumed_call_argument_plan(consumed, block_index, instruction_index, arg_index);
  const auto* route6_sources = consumed.shared_route6_call_use_source_index();
  if (prepared_argument == nullptr || route6_sources == nullptr || !*route6_sources ||
      argument.kind != c4c::backend::bir::Value::Kind::Named ||
      argument.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  const auto record = c4c::backend::bir::route6_find_call_argument_source(
      *route6_sources, block, instruction_index, call.callee, arg_index);
  if (!c4c::backend::bir::route6_call_argument_source_matches_argument_value_record(
          record, argument) ||
      !record.source_value_id.has_value() || !prepared_argument->source_value_id.has_value() ||
      *record.source_value_id != *prepared_argument->source_value_id) {
    return std::nullopt;
  }
  if (!record.source_value_name.has_value()) {
    return std::nullopt;
  }
  return ConsumedScalarI32CallArgumentSourceAuthority{
      .route6_source = record,
      .source_name = *record.source_value_name,
  };
}

[[nodiscard]] inline std::optional<c4c::backend::bir::Route6CallArgumentSourceRecord>
find_consumed_scalar_i32_call_argument_source(
    const ConsumedPlans& consumed,
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index,
    const c4c::backend::bir::Value& argument) {
  const auto authority = find_consumed_scalar_i32_call_argument_source_authority(
      consumed, block, call, block_index, instruction_index, arg_index, argument);
  if (!authority.has_value()) {
    return std::nullopt;
  }
  return authority->route6_source;
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallResultPlan*
find_consumed_call_result_plan(const ConsumedPlans& consumed,
                               std::size_t block_index,
                               std::size_t instruction_index) {
  const auto* call = find_consumed_call_plan(consumed, block_index, instruction_index);
  if (call == nullptr || !call->result.has_value()) {
    return nullptr;
  }
  return &*call->result;
}

[[nodiscard]] inline ConsumedPlans consume_plans(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name) {
  return ConsumedPlans{
      .frame = c4c::backend::prepare::find_prepared_frame_plan(module, function_name),
      .dynamic_stack =
          c4c::backend::prepare::find_prepared_dynamic_stack_plan(module, function_name),
      .control_flow =
          c4c::backend::prepare::find_prepared_control_flow_function(module.control_flow,
                                                                     function_name),
      .calls = c4c::backend::prepare::find_prepared_call_plans(module, function_name),
      .regalloc = [&]() -> const c4c::backend::prepare::PreparedRegallocFunction* {
        for (const auto& function_regalloc : module.regalloc.functions) {
          if (function_regalloc.function_name == function_name) {
            return &function_regalloc;
          }
        }
        return nullptr;
      }(),
      .storage = c4c::backend::prepare::find_prepared_storage_plan(module, function_name),
      .prepared_lookups = consume_prepared_function_lookups(module, function_name),
      .route6_call_use_sources = [&]() -> std::optional<c4c::backend::bir::Route6CallUseSourceIndex> {
        const auto* function = find_consumed_bir_function(module, function_name);
        if (function == nullptr) {
          return std::nullopt;
        }
        return c4c::backend::bir::route6_build_call_use_source_index(*function);
      }(),
  };
}

[[nodiscard]] inline ConsumedPlans consume_plans(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name) {
  const auto function_name_id =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function_name);
  if (!function_name_id.has_value()) {
    return {};
  }
  return consume_plans(module, *function_name_id);
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallPlan* find_consumed_call_plan(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index) {
  return find_consumed_call_plan(consume_plans(module, function_name), block_index, instruction_index);
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallPlan* find_consumed_call_plan(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name,
    std::size_t block_index,
    std::size_t instruction_index) {
  return find_consumed_call_plan(consume_plans(module, function_name), block_index, instruction_index);
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallArgumentPlan*
find_consumed_call_argument_plan(const c4c::backend::prepare::PreparedBirModule& module,
                                 c4c::FunctionNameId function_name,
                                 std::size_t block_index,
                                 std::size_t instruction_index,
                                 std::size_t arg_index) {
  return find_consumed_call_argument_plan(
      consume_plans(module, function_name), block_index, instruction_index, arg_index);
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallArgumentPlan*
find_consumed_call_argument_plan(const c4c::backend::prepare::PreparedBirModule& module,
                                 std::string_view function_name,
                                 std::size_t block_index,
                                 std::size_t instruction_index,
                                 std::size_t arg_index) {
  return find_consumed_call_argument_plan(
      consume_plans(module, function_name), block_index, instruction_index, arg_index);
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallResultPlan* find_consumed_call_result_plan(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index) {
  return find_consumed_call_result_plan(
      consume_plans(module, function_name), block_index, instruction_index);
}

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallResultPlan* find_consumed_call_result_plan(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view function_name,
    std::size_t block_index,
    std::size_t instruction_index) {
  return find_consumed_call_result_plan(
      consume_plans(module, function_name), block_index, instruction_index);
}

inline std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt,
    std::optional<std::string_view> focus_value = std::nullopt) {
  return c4c::backend::x86::debug::summarize_prepared_module_routes(
      module, focus_function, focus_block, focus_value);
}

inline std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt,
    std::optional<std::string_view> focus_value = std::nullopt) {
  return c4c::backend::x86::debug::trace_prepared_module_routes(
      module, focus_function, focus_block, focus_value);
}

inline std::string render_asm_symbol_name(std::string_view logical_name) {
  return c4c::backend::x86::abi::render_asm_symbol_name(
      c4c::default_host_target_triple(), logical_name);
}

inline std::string render_private_data_label(std::string_view pool_name) {
  return c4c::backend::x86::abi::render_private_data_label(
      c4c::default_host_target_triple(), pool_name);
}

// Root x86 contract: consume prepared plans published by `prealloc/` and only
// encode them into x86 syntax. GPR, FPR, and VREG homes must already be split
// in the prepared plans; x86 only maps each published bank to concrete x86
// spellings and instruction forms. VLA and dynamic `alloca` handling must come
// from `dynamic_stack_plan`; x86 may emit stack-save/alloca/restore sequences,
// but it must not rediscover dynamic-stack lifetime or fixed-slot anchoring
// policy locally.

}  // namespace c4c::backend::x86
