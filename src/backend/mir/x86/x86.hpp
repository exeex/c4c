#pragma once

#include "abi/abi.hpp"
#include "debug/debug.hpp"
#include "lowering/lowering.hpp"

namespace c4c::backend::x86 {

struct ConsumedPlans {
  const c4c::backend::prepare::PreparedFramePlanFunction* frame = nullptr;
  const c4c::backend::prepare::PreparedDynamicStackPlanFunction* dynamic_stack = nullptr;
  const c4c::backend::prepare::PreparedCallPlansFunction* calls = nullptr;
  const c4c::backend::prepare::PreparedStoragePlanFunction* storage = nullptr;
};

[[nodiscard]] inline ConsumedPlans consume_plans(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name) {
  return ConsumedPlans{
      .frame = c4c::backend::prepare::find_prepared_frame_plan(module, function_name),
      .dynamic_stack =
          c4c::backend::prepare::find_prepared_dynamic_stack_plan(module, function_name),
      .calls = c4c::backend::prepare::find_prepared_call_plans(module, function_name),
      .storage = c4c::backend::prepare::find_prepared_storage_plan(module, function_name),
  };
}

inline std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt) {
  return c4c::backend::x86::debug::summarize_prepared_module_routes(
      module, focus_function, focus_block);
}

inline std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt) {
  return c4c::backend::x86::debug::trace_prepared_module_routes(
      module, focus_function, focus_block);
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
