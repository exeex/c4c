#include "dispatch.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] const bir::BinaryInst* find_same_block_binary_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  return mir::find_same_block_binary_producer(context.bir_block, value).binary;
}

[[nodiscard]] SameBlockSelectProducer find_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  return mir::find_same_block_select_producer(
      context.bir_block, value, before_instruction_index);
}

[[nodiscard]] std::optional<std::int64_t> evaluate_same_block_integer_constant(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    unsigned depth) {
  const auto constant =
      mir::evaluate_same_block_integer_constant(context.bir_block, value, depth);
  if (!constant.has_value()) {
    return std::nullopt;
  }
  return constant->value;
}

[[nodiscard]] static bool dependency_is_load_global(
    const mir::DependencyTraversalRecord& record) {
  return record.kind == mir::SameBlockProducerKind::LoadGlobal;
}

[[nodiscard]] bool select_chain_contains_direct_global_load(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth) {
  return mir::select_chain_contains_dependency(
      context.bir_block, value, before_instruction_index, dependency_is_load_global, depth);
}

[[nodiscard]] const bir::Inst* find_same_block_named_producer(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  return mir::find_same_block_named_producer(
      context.bir_block, value_name, before_instruction_index);
}

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const module::BlockLoweringContext& context,
    const bir::Inst* producer) {
  return mir::producer_instruction_index(context.bir_block, producer);
}

[[nodiscard]] const bir::Global* find_load_global_target(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global) {
  if (context.function.prepared == nullptr) {
    return nullptr;
  }
  const auto& globals = context.function.prepared->module.globals;
  if (load_global.global_name_id != c4c::kInvalidLinkName) {
    const auto it = std::find_if(
        globals.begin(),
        globals.end(),
        [&](const bir::Global& global) {
          return global.link_name_id == load_global.global_name_id;
        });
    if (it != globals.end()) {
      return &*it;
    }
  }
  if (load_global.global_name.empty()) {
    return nullptr;
  }
  const auto it = std::find_if(
      globals.begin(),
      globals.end(),
      [&](const bir::Global& global) {
        return global.name == load_global.global_name;
      });
  return it == globals.end() ? nullptr : &*it;
}

[[nodiscard]] std::string load_global_symbol_label(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global,
    const bir::Global* target_global) {
  if (context.function.prepared != nullptr &&
      load_global.global_name_id != c4c::kInvalidLinkName) {
    const std::string_view semantic_name =
        context.function.prepared->module.names.link_names.spelling(
            load_global.global_name_id);
    if (!semantic_name.empty()) {
      return std::string{semantic_name};
    }
  }
  if (target_global != nullptr && !target_global->name.empty()) {
    return target_global->name;
  }
  return load_global.global_name;
}

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block) {
  if (function.bir_function == nullptr) {
    return nullptr;
  }

  if (function.prepared == nullptr || block.block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const std::string_view prepared_block_label =
      prepare::prepared_block_label(function.prepared->names, block.block_label);
  if (prepared_block_label.empty()) {
    return nullptr;
  }

  for (const auto& bir_block : function.bir_function->blocks) {
    if (bir_block.label_id != c4c::kInvalidBlockLabel &&
        function.prepared->module.names.block_labels.spelling(bir_block.label_id) ==
            prepared_block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == prepared_block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] bool is_current_block_join_parallel_copy_source(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto result_value = instruction_result_value(inst);
  if (!result_value.has_value() ||
      result_value->kind != bir::Value::Kind::Named ||
      result_value->name.empty()) {
    return false;
  }
  const auto result_value_name = prepared_named_value_id(context, *result_value);
  if (!result_value_name.has_value()) {
    return false;
  }
  const auto* result_home =
      find_value_home(context,
*result_value_name);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return false;
  }
  for (const auto& publication : prepare::collect_prepared_block_entry_publications(
           context.function.value_locations, context.control_flow_block->block_label)) {
    const auto* move = publication.move;
    if (move == nullptr ||
        move->op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        publication.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        publication.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
      continue;
    }
    if (publication.destination_value_id == result_home->value_id) {
      return true;
    }
    if (move->source_immediate_i32.has_value() ||
        move->from_value_id != result_home->value_id ||
        move->from_value_id == publication.destination_value_id) {
      continue;
    }
    const auto* destination_home = publication.home;
    if (destination_home != nullptr &&
        (prepared_edge_select_source_is_destination_register(*result_home,
                                                            *destination_home) ||
         result_home->kind == prepare::PreparedValueHomeKind::StackSlot)) {
      return true;
    }
  }
  return false;
}

}  // namespace c4c::backend::aarch64::codegen
