#include "traversal.hpp"

#include "dispatch.hpp"
#include "prologue.hpp"

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

[[nodiscard]] const c4c::backend::bir::Function* find_bir_function(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedControlFlowFunction& function) {
  if (function.function_name == c4c::kInvalidFunctionName) {
    return nullptr;
  }

  const std::string_view prepared_function_name =
      prepare::prepared_function_name(prepared.names, function.function_name);
  if (prepared_function_name.empty()) {
    return nullptr;
  }

  for (const auto& bir_function : prepared.module.functions) {
    if (std::string_view{bir_function.name} == prepared_function_name) {
      return &bir_function;
    }
  }
  return nullptr;
}

void emit_unplaced_parallel_copy_obligation_diagnostics(
    const prepare::PreparedControlFlowFunction& prepared_function,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto obligations =
      prepare::collect_unplaced_prepared_object_parallel_copy_obligations(
          prepared_function);
  for (const auto& obligation : obligations) {
    auto diagnostic = prepare::diagnose_prepared_object_consumer(obligation);
    if (!diagnostic.has_value()) {
      continue;
    }
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::
            PreparedObjectConsumerContractViolation,
        .function_name = prepared_function.function_name,
        .block_label = obligation.predecessor_label,
        .prepared_consumer_category = diagnostic->category,
        .message = std::move(diagnostic->message),
    });
  }
}

}  // namespace

module::FunctionLoweringContext make_function_lowering_context(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    const prepare::PreparedControlFlowFunction& function) {
  return module::FunctionLoweringContext{
      .prepared = &prepared,
      .target_profile = &target_profile,
      .control_flow = &function,
      .bir_function = find_bir_function(prepared, function),
      .value_locations =
          prepare::find_prepared_value_location_function(prepared, function.function_name),
      .storage_plan = prepare::find_prepared_storage_plan(prepared, function.function_name),
      .regalloc = [&prepared, function_name = function.function_name]()
          -> const prepare::PreparedRegallocFunction* {
        for (const auto& regalloc_function : prepared.regalloc.functions) {
          if (regalloc_function.function_name == function_name) {
            return &regalloc_function;
          }
        }
        return nullptr;
      }(),
      .frame_plan = prepare::find_prepared_frame_plan(prepared, function.function_name),
      .dynamic_stack_plan =
          prepare::find_prepared_dynamic_stack_plan(prepared, function.function_name),
      .call_plans = prepare::find_prepared_call_plans(prepared, function.function_name),
  };
}

std::vector<module::MachineFunction> lower_prepared_functions(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineFunction> functions;
  functions.reserve(prepared.control_flow.functions.size());

  for (const auto& prepared_function : prepared.control_flow.functions) {
    emit_unplaced_parallel_copy_obligation_diagnostics(prepared_function,
                                                       diagnostics);

    auto function_context =
        make_function_lowering_context(prepared, target_profile, prepared_function);
    const auto prepared_lookups =
        prepare::make_prepared_function_lookups(prepared, prepared_function);
    const auto prepared_call_plan_lookups = prepare::make_prepared_call_plan_lookups(
        prepared, function_context.call_plans, prepared_function);
    const auto prepared_address_materialization_lookups =
        prepare::make_prepared_address_materialization_lookups(
            prepared, prepared_function.function_name);
    const auto prepared_value_home_lookups =
        prepare::make_prepared_value_home_lookups(function_context.value_locations);
    const auto prepared_move_bundle_lookups =
        prepare::make_prepared_move_bundle_lookups(prepared, prepared_function);
    function_context.prepared_lookups = &prepared_lookups;
    function_context.call_plan_lookups = &prepared_call_plan_lookups;
    function_context.address_materialization_lookups =
        &prepared_address_materialization_lookups;
    function_context.move_bundle_lookups = &prepared_move_bundle_lookups;
    function_context.value_home_lookups = &prepared_value_home_lookups;

    module::MachineFunction function{
        .function_name = prepared_function.function_name,
        .blocks = {},
    };
    function.blocks.reserve(prepared_function.blocks.size());

    for (std::size_t block_index = 0; block_index < prepared_function.blocks.size();
         ++block_index) {
      const auto& prepared_block = prepared_function.blocks[block_index];
      function.blocks.push_back(module::MachineBlock{
          .block_label = prepared_block.block_label,
          .index = block_index,
          .instructions = {},
      });
      const auto block_context =
          make_block_lowering_context(function_context, prepared_block, block_index);
      if (block_context.bir_block != nullptr) {
        function.blocks.back().instructions.reserve(
            block_context.bir_block->insts.size() * 8U + 8U);
      }
      (void)dispatch_prepared_block(block_context, function.blocks.back(), diagnostics);
    }

    if (prepared_function.function_name == c4c::kInvalidFunctionName) {
      diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
          .kind = module::ModuleLoweringDiagnosticKind::MissingFunctionContext,
          .function_name = prepared_function.function_name,
          .message = "prepared control-flow function is missing durable function identity",
      });
    }

    insert_prepared_frame_boundary_nodes(function_context, prepared_function, function);

    functions.push_back(std::move(function));
  }

  return functions;
}

}  // namespace c4c::backend::aarch64::codegen
