#include "dispatch_producers.hpp"

#include "dispatch.hpp"

#include "dispatch_edge_copies.hpp"
#include "dispatch_publication.hpp"

#include "../../../prealloc/prepared_lookups.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <variant>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

[[nodiscard]] const bir::Value* instruction_result_value_ref(const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> const bir::Value* {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                      std::is_same_v<T, bir::CastInst> ||
                      std::is_same_v<T, bir::SelectInst> ||
                      std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          return &typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return typed_inst.result.has_value() ? &*typed_inst.result : nullptr;
        }
        return nullptr;
      },
      inst);
}

[[nodiscard]] std::optional<bir::Value> instruction_result_value(
    const bir::Inst& inst) {
  const auto* result = instruction_result_value_ref(inst);
  if (result == nullptr) {
    return std::nullopt;
  }
  return *result;
}

[[nodiscard]] std::optional<prepare::PreparedEdgePublicationSourceProducer>
prepared_source_producer_for_value(const module::BlockLoweringContext& context,
                                   const bir::Value& value) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  if (context.function.prepared_lookups == nullptr) {
    return std::nullopt;
  }
  const auto* producer =
      prepare::find_indexed_prepared_edge_publication_source_producer(
          &context.function.prepared_lookups->edge_publication_source_producers,
          *value_name);
  return producer != nullptr
             ? std::optional<prepare::PreparedEdgePublicationSourceProducer>{
                   *producer}
             : std::nullopt;
}

[[nodiscard]] SameBlockSelectProducer prepared_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr || context.control_flow_block == nullptr ||
      value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return {};
  }
  const auto producer = prepared_source_producer_for_value(context, value);
  if (!producer.has_value() ||
      producer->kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization ||
      producer->block_label != context.control_flow_block->block_label ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= context.bir_block->insts.size()) {
    return {};
  }
  const auto* select =
      std::get_if<bir::SelectInst>(&context.bir_block->insts[producer->instruction_index]);
  if (select == nullptr || producer->select != select ||
      select->result.kind != bir::Value::Kind::Named ||
      select->result.name != value.name ||
      select->result.type != value.type) {
    return {};
  }
  return SameBlockSelectProducer{.select = select,
                                 .instruction_index = producer->instruction_index};
}

}  // namespace

[[nodiscard]] SameBlockSelectProducer find_prepared_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  return prepared_same_block_select_producer(context, value, before_instruction_index);
}

[[nodiscard]] static bool dependency_is_load_global(
    const mir::DependencyTraversalRecord& record) {
  return record.kind == mir::SameBlockProducerKind::LoadGlobal;
}

[[nodiscard]] std::optional<bool>
prepared_select_chain_contains_direct_global_load(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth) {
  if (depth > 64U) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return false;
  }
  if (context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto producer = prepared_source_producer_for_value(context, value);
  if (!producer.has_value() ||
      producer->block_label != context.control_flow_block->block_label ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }

  const auto& inst = context.bir_block->insts[producer->instruction_index];
  switch (producer->kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer->load_global == std::get_if<bir::LoadGlobalInst>(&inst)
                 ? std::optional<bool>{true}
                 : std::nullopt;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer->load_local == std::get_if<bir::LoadLocalInst>(&inst)
                 ? std::optional<bool>{false}
                 : std::nullopt;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast: {
      const auto* cast = std::get_if<bir::CastInst>(&inst);
      if (producer->cast != cast || cast == nullptr) {
        return std::nullopt;
      }
      return prepared_select_chain_contains_direct_global_load(
          context, cast->operand, producer->instruction_index, depth + 1U);
    }
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary: {
      const auto* binary = std::get_if<bir::BinaryInst>(&inst);
      if (producer->binary != binary || binary == nullptr) {
        return std::nullopt;
      }
      const auto lhs = prepared_select_chain_contains_direct_global_load(
          context, binary->lhs, producer->instruction_index, depth + 1U);
      if (!lhs.has_value() || *lhs) {
        return lhs;
      }
      return prepared_select_chain_contains_direct_global_load(
          context, binary->rhs, producer->instruction_index, depth + 1U);
    }
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization: {
      const auto* select = std::get_if<bir::SelectInst>(&inst);
      if (producer->select != select || select == nullptr) {
        return std::nullopt;
      }
      const auto true_value =
          prepared_select_chain_contains_direct_global_load(
              context, select->true_value, producer->instruction_index, depth + 1U);
      if (!true_value.has_value() || *true_value) {
        return true_value;
      }
      return prepared_select_chain_contains_direct_global_load(
          context, select->false_value, producer->instruction_index, depth + 1U);
    }
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
      return false;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool select_chain_contains_direct_global_load(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth) {
  const auto prepared = prepared_select_chain_contains_direct_global_load(
      context, value, before_instruction_index, depth);
  if (prepared.has_value()) {
    return *prepared;
  }
  return mir::select_chain_contains_dependency(
      context.bir_block, value, before_instruction_index, dependency_is_load_global, depth);
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
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *result_value_name);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value) {
        continue;
      }
      if (prepare::prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
              move, result_home->value_id)) {
        return true;
      }
      if (move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
          move.source_immediate_i32.has_value() ||
          move.from_value_id != result_home->value_id ||
          move.from_value_id == move.to_value_id) {
        continue;
      }
      const auto* destination_home =
          prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                    context.function.value_locations,
                                                    move.to_value_id);
      if (destination_home != nullptr &&
          (prepare::prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
               move, *result_home, *destination_home) ||
           result_home->kind == prepare::PreparedValueHomeKind::StackSlot)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::vector<std::string_view> named_operands_of_instruction(
    const bir::Inst& inst) {
  std::vector<std::string_view> operands;
  auto append_named = [&operands](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Named && !value.name.empty()) {
      operands.push_back(value.name);
    }
  };
  std::visit(
      [&](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          append_named(typed_inst.lhs);
          append_named(typed_inst.rhs);
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          append_named(typed_inst.operand);
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          append_named(typed_inst.lhs);
          append_named(typed_inst.rhs);
          append_named(typed_inst.true_value);
          append_named(typed_inst.false_value);
        }
      },
      inst);
  return operands;
}

[[nodiscard]] bool is_join_parallel_copy_expression_instruction(
    const bir::Inst& inst) {
  return std::get_if<bir::BinaryInst>(&inst) != nullptr ||
         std::get_if<bir::CastInst>(&inst) != nullptr ||
         std::get_if<bir::SelectInst>(&inst) != nullptr;
}

[[nodiscard]] std::optional<std::size_t> find_same_block_result_index(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr || value_name.empty()) {
    return std::nullopt;
  }
  const auto limit =
      std::min(before_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = limit; index > 0; --index) {
    const auto* result = instruction_result_value_ref(context.bir_block->insts[index - 1]);
    if (result != nullptr && result->kind == bir::Value::Kind::Named &&
        result->name == value_name) {
      return index - 1;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool same_block_result_depends_on_value(
    const module::BlockLoweringContext& context,
    std::string_view source_name,
    std::string_view dependency_name,
    std::size_t before_instruction_index,
    std::size_t depth = 0) {
  if (source_name.empty() || dependency_name.empty() || depth > 16U) {
    return false;
  }
  if (source_name == dependency_name) {
    return true;
  }
  const auto producer_index =
      find_same_block_result_index(context, source_name, before_instruction_index);
  if (!producer_index.has_value()) {
    return false;
  }
  const auto& producer = context.bir_block->insts[*producer_index];
  if (!is_join_parallel_copy_expression_instruction(producer)) {
    return false;
  }
  auto operand_depends = [&](const bir::Value& operand) {
    return operand.kind == bir::Value::Kind::Named &&
           !operand.name.empty() &&
           (operand.name == dependency_name ||
            same_block_result_depends_on_value(context,
                                               operand.name,
                                               dependency_name,
                                               *producer_index,
                                               depth + 1U));
  };
  return std::visit(
      [&](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          return operand_depends(typed_inst.lhs) ||
                 operand_depends(typed_inst.rhs);
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          return operand_depends(typed_inst.operand);
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return operand_depends(typed_inst.lhs) ||
                 operand_depends(typed_inst.rhs) ||
                 operand_depends(typed_inst.true_value) ||
                 operand_depends(typed_inst.false_value);
        }
        return false;
      },
      producer);
}

[[nodiscard]] bool is_current_block_join_parallel_copy_incoming_expression(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      !is_join_parallel_copy_expression_instruction(inst)) {
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
      prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *result_value_name);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.source_immediate_i32.has_value()) {
        continue;
      }
      const auto* source_home =
          prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                    context.function.value_locations,
                                                    move.from_value_id);
      if (source_home == nullptr ||
          source_home->value_name == c4c::kInvalidValueName) {
        continue;
      }
      if (source_home->value_id == result_home->value_id) {
        return true;
      }
      const auto source_name =
          prepare::prepared_value_name(context.function.prepared->names,
                                       source_home->value_name);
      if (!source_name.empty() &&
          same_block_result_depends_on_value(context,
                                             source_name,
                                             result_value->name,
                                             context.bir_block->insts.size())) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] CurrentBlockJoinParallelCopyCache
build_current_block_join_parallel_copy_cache(
    const module::BlockLoweringContext& context) {
  CurrentBlockJoinParallelCopyCache cache{.context = &context};
  if (context.bir_block == nullptr) {
    return cache;
  }
  const bool has_relevant_bundle =
      context.function.value_locations != nullptr &&
      context.control_flow_block != nullptr &&
      std::any_of(context.function.value_locations->move_bundles.begin(),
                  context.function.value_locations->move_bundles.end(),
                  [&](const prepare::PreparedMoveBundle& bundle) {
                    return bundle.phase == prepare::PreparedMovePhase::BlockEntry &&
                           bundle.authority_kind ==
                               prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
                           bundle.source_parallel_copy_successor_label ==
                               std::optional<c4c::BlockLabelId>{
                                   context.control_flow_block->block_label};
                  });
  cache.incoming_expressions.reserve(context.bir_block->insts.size());
  cache.sources.reserve(context.bir_block->insts.size());
  if (!has_relevant_bundle) {
    cache.incoming_expressions.assign(context.bir_block->insts.size(), false);
    cache.sources.assign(context.bir_block->insts.size(), false);
    return cache;
  }
  std::unordered_map<std::string_view, std::size_t> result_indices;
  result_indices.reserve(context.bir_block->insts.size());
  for (std::size_t index = 0; index < context.bir_block->insts.size(); ++index) {
    const auto* result = instruction_result_value_ref(context.bir_block->insts[index]);
    if (result != nullptr && result->kind == bir::Value::Kind::Named &&
        !result->name.empty()) {
      result_indices.emplace(result->name, index);
    }
  }

  std::unordered_set<prepare::PreparedValueId> incoming_value_ids;
  std::unordered_set<prepare::PreparedValueId> source_value_ids;
  std::unordered_set<std::string_view> incoming_expression_names;
  std::vector<std::string_view> pending_expression_names;
  auto add_expression_dependency = [&](std::string_view name) {
    if (!name.empty()) {
      pending_expression_names.push_back(name);
    }
  };

  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value) {
        continue;
      }
      if (!move.source_immediate_i32.has_value()) {
        const auto* source_home =
            prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                      context.function.value_locations,
                                                      move.from_value_id);
        if (source_home != nullptr &&
            source_home->value_name != c4c::kInvalidValueName) {
          incoming_value_ids.insert(source_home->value_id);
          add_expression_dependency(prepare::prepared_value_name(
              context.function.prepared->names, source_home->value_name));
        }
      }
      if (!prepare::prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
              move, move.to_value_id)) {
        continue;
      }
      source_value_ids.insert(move.to_value_id);
      if (move.source_immediate_i32.has_value() ||
          move.from_value_id == move.to_value_id) {
        continue;
      }
      const auto* result_home =
          prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                    context.function.value_locations,
                                                    move.from_value_id);
      const auto* destination_home =
          prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                    context.function.value_locations,
                                                    move.to_value_id);
      if (result_home != nullptr && destination_home != nullptr &&
          (prepare::prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
               move, *result_home, *destination_home) ||
           result_home->kind == prepare::PreparedValueHomeKind::StackSlot)) {
        source_value_ids.insert(move.from_value_id);
      }
    }
  }

  while (!pending_expression_names.empty()) {
    const auto name = pending_expression_names.back();
    pending_expression_names.pop_back();
    if (name.empty() || !incoming_expression_names.insert(name).second) {
      continue;
    }
    const auto producer_it = result_indices.find(name);
    if (producer_it == result_indices.end()) {
      continue;
    }
    const auto& producer = context.bir_block->insts[producer_it->second];
    if (!is_join_parallel_copy_expression_instruction(producer)) {
      continue;
    }
    for (const auto operand_name : named_operands_of_instruction(producer)) {
      add_expression_dependency(operand_name);
    }
  }

  for (const auto& inst : context.bir_block->insts) {
    const auto* result = instruction_result_value_ref(inst);
    bool incoming_expression = false;
    bool source = false;
    if (result != nullptr && result->kind == bir::Value::Kind::Named &&
        !result->name.empty()) {
      const auto result_value_name = prepared_named_value_id(context, *result);
      const auto* result_home =
          result_value_name.has_value()
              ? prepare::find_indexed_prepared_value_home(context.function.value_home_lookups,
                                                          context.function.regalloc,
                                                          context.function.value_locations,
                                                          *result_value_name)
              : nullptr;
      if (result_home != nullptr && result_home->value_name != c4c::kInvalidValueName) {
        incoming_expression =
            incoming_value_ids.find(result_home->value_id) != incoming_value_ids.end() ||
            incoming_expression_names.find(result->name) != incoming_expression_names.end();
        source = source_value_ids.find(result_home->value_id) != source_value_ids.end();
      }
    }
    cache.incoming_expressions.push_back(incoming_expression);
    cache.sources.push_back(source);
  }
  return cache;
}

[[nodiscard]] bool cached_current_block_join_parallel_copy_incoming_expression(
    const CurrentBlockJoinParallelCopyCache& cache,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::Inst& inst) {
  if (cache.context == &context &&
      instruction_index < cache.incoming_expressions.size()) {
    return cache.incoming_expressions[instruction_index];
  }
  return is_current_block_join_parallel_copy_incoming_expression(context, inst);
}

[[nodiscard]] bool cached_current_block_join_parallel_copy_source(
    const CurrentBlockJoinParallelCopyCache& cache,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::Inst& inst) {
  if (cache.context == &context && instruction_index < cache.sources.size()) {
    return cache.sources[instruction_index];
  }
  return is_current_block_join_parallel_copy_source(context, inst);
}

}  // namespace c4c::backend::aarch64::codegen
