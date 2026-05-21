#include "traversal.hpp"

#include "dispatch.hpp"
#include "prologue.hpp"

#include <cstddef>
#include <algorithm>
#include <optional>
#include <string_view>
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

}  // namespace

[[nodiscard]] std::size_t call_position_key(std::size_t block_index,
                                            std::size_t instruction_index) {
  return (block_index << 32U) ^ instruction_index;
}

[[nodiscard]] std::size_t move_bundle_position_key(prepare::PreparedMovePhase phase,
                                                   std::size_t block_index,
                                                   std::size_t instruction_index) {
  return (static_cast<std::size_t>(phase) << 56U) ^
         (block_index << 32U) ^
         instruction_index;
}

[[nodiscard]] bool prior_preserved_entry_position_less(
    const module::PriorPreservedValueEntry& lhs,
    const module::PriorPreservedValueEntry& rhs) {
  if (lhs.block_index != rhs.block_index) {
    return lhs.block_index < rhs.block_index;
  }
  return lhs.instruction_index < rhs.instruction_index;
}

[[nodiscard]] std::optional<std::size_t> prepared_block_index_by_label(
    const prepare::PreparedControlFlowFunction& function,
    c4c::BlockLabelId label) {
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    if (function.blocks[index].block_label == label) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::size_t> prepared_block_successors(
    const prepare::PreparedControlFlowFunction& function,
    const prepare::PreparedControlFlowBlock& block) {
  std::vector<std::size_t> successors;
  auto append_label = [&](c4c::BlockLabelId label) {
    if (label == c4c::kInvalidBlockLabel) {
      return;
    }
    const auto index = prepared_block_index_by_label(function, label);
    if (index.has_value() &&
        std::find(successors.begin(), successors.end(), *index) == successors.end()) {
      successors.push_back(*index);
    }
  };

  if (block.terminator_kind == bir::TerminatorKind::Branch) {
    append_label(block.branch_target_label);
  } else if (block.terminator_kind == bir::TerminatorKind::CondBranch) {
    append_label(block.true_label);
    append_label(block.false_label);
  }
  return successors;
}

[[nodiscard]] std::vector<std::vector<bool>> make_prepared_dominance_matrix(
    const prepare::PreparedControlFlowFunction& function) {
  const std::size_t count = function.blocks.size();
  std::vector<std::vector<std::size_t>> predecessors(count);
  for (std::size_t index = 0; index < count; ++index) {
    for (const auto successor : prepared_block_successors(function, function.blocks[index])) {
      if (successor < count) {
        predecessors[successor].push_back(index);
      }
    }
  }

  std::vector<std::vector<bool>> dominates(count, std::vector<bool>(count, true));
  if (count != 0) {
    std::fill(dominates.front().begin(), dominates.front().end(), false);
    dominates.front().front() = true;
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t index = 1; index < count; ++index) {
      std::vector<bool> next(count, !predecessors[index].empty());
      if (predecessors[index].empty()) {
        std::fill(next.begin(), next.end(), false);
      } else {
        for (const auto predecessor : predecessors[index]) {
          for (std::size_t candidate = 0; candidate < count; ++candidate) {
            next[candidate] = next[candidate] && dominates[predecessor][candidate];
          }
        }
      }
      next[index] = true;
      if (next != dominates[index]) {
        dominates[index] = std::move(next);
        changed = true;
      }
    }
  }
  return dominates;
}

[[nodiscard]] bool prior_stack_preserved_reaches_call(
    const std::vector<module::PriorPreservedValueEntry>& prior_entries,
    const std::vector<std::vector<bool>>& dominates,
    const prepare::PreparedCallPlan& call) {
  for (const auto& prior : prior_entries) {
    if (prior.preserved == nullptr ||
        prior.preserved->route != prepare::PreparedCallPreservationRoute::StackSlot) {
      continue;
    }
    if (prior.block_index == call.block_index) {
      if (prior.instruction_index < call.instruction_index) {
        return true;
      }
      continue;
    }
    if (call.block_index < dominates.size() &&
        prior.block_index < dominates[call.block_index].size() &&
        dominates[call.block_index][prior.block_index]) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] module::PreparedCallPlanIndexes make_prepared_call_plan_indexes(
    const prepare::PreparedCallPlansFunction* call_plans,
    const prepare::PreparedControlFlowFunction& function) {
  module::PreparedCallPlanIndexes indexes;
  if (call_plans == nullptr) {
    return indexes;
  }
  indexes.calls_by_position.reserve(call_plans->calls.size());
  indexes.first_stack_preserved_by_call_index.resize(call_plans->calls.size());
  const auto dominates = make_prepared_dominance_matrix(function);
  for (std::size_t call_index = 0; call_index < call_plans->calls.size(); ++call_index) {
    const auto& call = call_plans->calls[call_index];
    indexes.calls_by_position.emplace(
        call_position_key(call.block_index, call.instruction_index), &call);
    for (const auto& preserved : call.preserved_values) {
      const bool stack_preserved =
          preserved.route == prepare::PreparedCallPreservationRoute::StackSlot &&
          preserved.value_name != c4c::kInvalidValueName &&
          preserved.slot_id.has_value() &&
          preserved.stack_offset_bytes.has_value() &&
          preserved.stack_size_bytes.has_value() &&
          *preserved.stack_size_bytes != 0;
      if (stack_preserved) {
        const auto has_reaching_prior =
            preserved.value_id < indexes.prior_preserved_by_value.size() &&
            prior_stack_preserved_reaches_call(
                indexes.prior_preserved_by_value[preserved.value_id], dominates, call);
        if (!has_reaching_prior) {
          indexes.first_stack_preserved_by_call_index[call_index].push_back(&preserved);
        }
      }
      if (preserved.route != prepare::PreparedCallPreservationRoute::Unknown) {
        if (preserved.value_id >= indexes.prior_preserved_by_value.size()) {
          indexes.prior_preserved_by_value.resize(preserved.value_id + 1U);
        }
        indexes.prior_preserved_by_value[preserved.value_id].push_back(
            module::PriorPreservedValueEntry{
                .block_index = call.block_index,
                .instruction_index = call.instruction_index,
                .preserved = &preserved,
            });
      }
    }
  }
  for (auto& entries : indexes.prior_preserved_by_value) {
    std::sort(entries.begin(), entries.end(), prior_preserved_entry_position_less);
  }
  return indexes;
}

[[nodiscard]] module::PreparedAddressMaterializationIndexes
make_prepared_address_materialization_indexes(
    const prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name) {
  module::PreparedAddressMaterializationIndexes indexes;
  const auto* addressing = prepare::find_prepared_addressing(prepared, function_name);
  if (addressing == nullptr) {
    return indexes;
  }
  indexes.materializations_by_block.reserve(addressing->address_materializations.size());
  for (const auto& materialization : addressing->address_materializations) {
    indexes.materializations_by_block[materialization.block_label].push_back(
        &materialization);
  }
  return indexes;
}

[[nodiscard]] module::PreparedMoveBundleIndexes make_prepared_move_bundle_indexes(
    const prepare::PreparedValueLocationFunction* value_locations) {
  module::PreparedMoveBundleIndexes indexes;
  if (value_locations == nullptr) {
    return indexes;
  }
  indexes.bundles_by_position.reserve(value_locations->move_bundles.size());
  for (const auto& bundle : value_locations->move_bundles) {
    indexes.bundles_by_position.emplace(
        move_bundle_position_key(bundle.phase, bundle.block_index, bundle.instruction_index),
        &bundle);
  }
  return indexes;
}

[[nodiscard]] module::PreparedValueHomeIndexes make_prepared_value_home_indexes(
    const prepare::PreparedValueLocationFunction* value_locations) {
  module::PreparedValueHomeIndexes indexes;
  if (value_locations == nullptr) {
    return indexes;
  }
  indexes.homes_by_id.reserve(value_locations->value_homes.size());
  indexes.homes_by_name.reserve(value_locations->value_homes.size());
  for (const auto& home : value_locations->value_homes) {
    indexes.homes_by_id.emplace(home.value_id, &home);
    if (home.value_name != c4c::kInvalidValueName) {
      indexes.homes_by_name.emplace(home.value_name, &home);
    }
  }
  return indexes;
}

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
    auto function_context =
        make_function_lowering_context(prepared, target_profile, prepared_function);
    const auto call_plan_indexes =
        make_prepared_call_plan_indexes(function_context.call_plans, prepared_function);
    const auto address_materialization_indexes =
        make_prepared_address_materialization_indexes(prepared,
                                                      prepared_function.function_name);
    const auto move_bundle_indexes =
        make_prepared_move_bundle_indexes(function_context.value_locations);
    const auto value_home_indexes =
        make_prepared_value_home_indexes(function_context.value_locations);
    function_context.call_plan_indexes = &call_plan_indexes;
    function_context.address_materialization_indexes = &address_materialization_indexes;
    function_context.move_bundle_indexes = &move_bundle_indexes;
    function_context.value_home_indexes = &value_home_indexes;

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
