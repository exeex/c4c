#pragma once

#include "../../prealloc/prealloc.hpp"
#include "abi/abi.hpp"
#include "debug/debug.hpp"
#include "lowering/lowering.hpp"

#include <optional>
#include <string_view>

namespace c4c::backend::x86 {

struct ConsumedPlans {
  const c4c::backend::prepare::PreparedFramePlanFunction* frame = nullptr;
  const c4c::backend::prepare::PreparedDynamicStackPlanFunction* dynamic_stack = nullptr;
  const c4c::backend::prepare::PreparedCallPlansFunction* calls = nullptr;
  const c4c::backend::prepare::PreparedRegallocFunction* regalloc = nullptr;
  const c4c::backend::prepare::PreparedStoragePlanFunction* storage = nullptr;
};

[[nodiscard]] inline const c4c::backend::prepare::PreparedCallPlan* find_consumed_call_plan(
    const ConsumedPlans& consumed,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (consumed.calls == nullptr) {
    return nullptr;
  }
  for (const auto& call : consumed.calls->calls) {
    if (call.block_index == block_index && call.instruction_index == instruction_index) {
      return &call;
    }
  }
  return nullptr;
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
