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
#include <unordered_set>
#include <vector>
#include <variant>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
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

[[nodiscard]] std::optional<prepare::PreparedValueId>
instruction_result_prepared_value_id(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  const auto* result = instruction_result_value_ref(inst);
  if (result == nullptr ||
      result->kind != bir::Value::Kind::Named ||
      result->name.empty()) {
    return std::nullopt;
  }
  const auto result_value_name = prepared_named_value_id(context, *result);
  if (!result_value_name.has_value()) {
    return std::nullopt;
  }
  return prepare::find_indexed_prepared_value_id(context.function.value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *result_value_name);
}

[[nodiscard]] prepare::PreparedCurrentBlockJoinParallelCopySourceFacts
prepare_current_block_join_parallel_copy_source_facts(
    const module::BlockLoweringContext& context) {
  std::optional<prepare::PreparedValueHomeLookups> local_value_home_lookups;
  const auto* value_home_lookups = context.function.value_home_lookups;
  if (value_home_lookups == nullptr && context.function.value_locations != nullptr) {
    local_value_home_lookups =
        prepare::make_prepared_value_home_lookups(context.function.value_locations);
    value_home_lookups = &*local_value_home_lookups;
  }

  std::optional<prepare::PreparedEdgePublicationLookups> local_edge_publications;
  const auto* edge_publications =
      context.function.prepared_lookups != nullptr
          ? &context.function.prepared_lookups->edge_publications
          : nullptr;
  if (edge_publications == nullptr &&
      context.function.prepared != nullptr &&
      context.function.control_flow != nullptr) {
    local_edge_publications =
        prepare::make_prepared_edge_publication_lookups(
            context.function.prepared->names,
            *context.function.control_flow,
            context.function.value_locations,
            value_home_lookups);
    edge_publications = &*local_edge_publications;
  }

  return prepare::prepare_current_block_join_parallel_copy_source_facts(
      prepare::PreparedCurrentBlockJoinParallelCopySourceQueryInputs{
          .names = context.function.prepared != nullptr
                       ? &context.function.prepared->names
                       : nullptr,
          .value_locations = context.function.value_locations,
          .value_home_lookups = value_home_lookups,
          .edge_publications = edge_publications,
          .block = context.bir_block,
          .successor_label =
              context.control_flow_block != nullptr
                  ? context.control_flow_block->block_label
                  : c4c::kInvalidBlockLabel,
      });
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
  return false;
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

[[nodiscard]] bool prepared_query_current_block_join_parallel_copy_source(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  const auto result_value_id = instruction_result_prepared_value_id(context, inst);
  if (!result_value_id.has_value()) {
    return false;
  }
  const auto query = prepare_current_block_join_parallel_copy_source_facts(context);
  if (query.status !=
      prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::Available) {
    return false;
  }
  return std::find(query.source_value_ids.begin(),
                   query.source_value_ids.end(),
                   *result_value_id) != query.source_value_ids.end();
}

[[nodiscard]] CurrentBlockJoinPreparedQueryRouting
build_current_block_join_prepared_query_routing(
    const module::BlockLoweringContext& context) {
  CurrentBlockJoinPreparedQueryRouting routing{.context = &context};
  if (context.bir_block == nullptr) {
    return routing;
  }
  const auto query = prepare_current_block_join_parallel_copy_source_facts(context);
  const std::unordered_set<prepare::PreparedValueId> incoming_value_ids{
      query.incoming_expression_value_ids.begin(),
      query.incoming_expression_value_ids.end()};
  const std::unordered_set<prepare::PreparedValueId> source_value_ids{
      query.source_value_ids.begin(),
      query.source_value_ids.end()};

  for (const auto& inst : context.bir_block->insts) {
    const auto result_value_id = instruction_result_prepared_value_id(context, inst);
    const bool incoming_expression =
        result_value_id.has_value() &&
        incoming_value_ids.find(*result_value_id) != incoming_value_ids.end();
    const bool source =
        result_value_id.has_value() &&
        source_value_ids.find(*result_value_id) != source_value_ids.end();
    routing.incoming_expressions.push_back(incoming_expression);
    routing.sources.push_back(source);
  }
  return routing;
}

[[nodiscard]] bool current_block_join_prepared_query_incoming_expression(
    const CurrentBlockJoinPreparedQueryRouting& routing,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::Inst& inst) {
  if (routing.context == &context &&
      instruction_index < routing.incoming_expressions.size()) {
    return routing.incoming_expressions[instruction_index];
  }
  (void)inst;
  return false;
}

[[nodiscard]] bool current_block_join_prepared_query_source(
    const CurrentBlockJoinPreparedQueryRouting& routing,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::Inst& inst) {
  if (routing.context == &context && instruction_index < routing.sources.size()) {
    return routing.sources[instruction_index];
  }
  (void)inst;
  return false;
}

}  // namespace c4c::backend::aarch64::codegen
