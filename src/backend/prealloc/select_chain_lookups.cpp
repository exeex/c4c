#include "select_chain_lookups.hpp"

#include "module.hpp"

#include <optional>
#include <variant>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] std::optional<ValueNameId> existing_prepared_value_name_id(
    const PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == kInvalidValueName) {
    return std::nullopt;
  }
  return value_name;
}

[[nodiscard]] const bir::Function* prepared_bir_function(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function) {
  if (function.function_name == kInvalidFunctionName) {
    return nullptr;
  }
  const std::string_view function_name =
      prepared_function_name(prepared.names, function.function_name);
  if (function_name.empty()) {
    return nullptr;
  }
  for (const auto& bir_function : prepared.module.functions) {
    if (std::string_view{bir_function.name} == function_name) {
      return &bir_function;
    }
  }
  return nullptr;
}

[[nodiscard]] BlockLabelId prepared_bir_block_label_id(const PreparedBirModule& prepared,
                                                       const bir::Block& block) {
  if (block.label_id != kInvalidBlockLabel) {
    const std::string_view label =
        prepared.module.names.block_labels.spelling(block.label_id);
    if (!label.empty()) {
      const BlockLabelId prepared_label = prepared.names.block_labels.find(label);
      if (prepared_label != kInvalidBlockLabel) {
        return prepared_label;
      }
    }
  }
  return prepared.names.block_labels.find(block.label);
}

void publish_source_producer(
    std::unordered_map<ValueNameId, PreparedEdgePublicationSourceProducer>& producers,
    ValueNameId value_name,
    PreparedEdgePublicationSourceProducer producer) {
  if (value_name == kInvalidValueName) {
    return;
  }
  const auto [it, inserted] = producers.emplace(value_name, producer);
  if (!inserted) {
    it->second = PreparedEdgePublicationSourceProducer{};
  }
}

[[nodiscard]] const bir::Value* prepared_source_producer_result(
    const PreparedEdgePublicationSourceProducer& producer) {
  switch (producer.kind) {
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local != nullptr ? &producer.load_local->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global != nullptr ? &producer.load_global->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast != nullptr ? &producer.cast->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary != nullptr ? &producer.binary->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select != nullptr ? &producer.select->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::Immediate:
    case PreparedEdgePublicationSourceProducerKind::Unknown:
      return nullptr;
  }
  return nullptr;
}

[[nodiscard]] bool prepared_source_producer_matches_indexed_instruction(
    const PreparedEdgePublicationSourceProducer& producer,
    const bir::Inst& inst,
    const bir::Value& value) {
  const auto* result = prepared_source_producer_result(producer);
  if (result == nullptr ||
      result->kind != bir::Value::Kind::Named ||
      value.kind != bir::Value::Kind::Named ||
      result->name != value.name ||
      result->type != value.type) {
    return false;
  }
  switch (producer.kind) {
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local == std::get_if<bir::LoadLocalInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global == std::get_if<bir::LoadGlobalInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast == std::get_if<bir::CastInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary == std::get_if<bir::BinaryInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select == std::get_if<bir::SelectInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::Immediate:
    case PreparedEdgePublicationSourceProducerKind::Unknown:
      return false;
  }
  return false;
}

[[nodiscard]] std::optional<bool>
prepared_select_chain_contains_direct_global_load(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0) {
  if (depth > 64U) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return false;
  }
  const auto* producer = find_prepared_select_chain_source_producer(
      names, source_producers, block_label, block, value, before_instruction_index);
  if (producer == nullptr) {
    return std::nullopt;
  }
  switch (producer->kind) {
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return true;
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
    case PreparedEdgePublicationSourceProducerKind::Immediate:
      return false;
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return producer->cast != nullptr
                 ? prepared_select_chain_contains_direct_global_load(
                       names,
                       source_producers,
                       block_label,
                       block,
                       producer->cast->operand,
                       producer->instruction_index,
                       depth + 1U)
                 : std::nullopt;
    case PreparedEdgePublicationSourceProducerKind::Binary: {
      if (producer->binary == nullptr) {
        return std::nullopt;
      }
      const auto lhs = prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          producer->binary->lhs,
          producer->instruction_index,
          depth + 1U);
      if (!lhs.has_value() || *lhs) {
        return lhs;
      }
      return prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          producer->binary->rhs,
          producer->instruction_index,
          depth + 1U);
    }
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization: {
      if (producer->select == nullptr) {
        return std::nullopt;
      }
      const auto true_value = prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          producer->select->true_value,
          producer->instruction_index,
          depth + 1U);
      if (!true_value.has_value() || *true_value) {
        return true_value;
      }
      return prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          producer->select->false_value,
          producer->instruction_index,
          depth + 1U);
    }
    case PreparedEdgePublicationSourceProducerKind::Unknown:
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace

PreparedEdgePublicationSourceProducerLookups
make_prepared_edge_publication_source_producer_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function) {
  PreparedEdgePublicationSourceProducerLookups lookups;
  const auto* bir_function = prepared_bir_function(prepared, function);
  if (bir_function == nullptr) {
    return lookups;
  }

  for (const auto& block : bir_function->blocks) {
    const BlockLabelId block_label = prepared_bir_block_label_id(prepared, block);
    for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
      const auto& inst = block.insts[inst_index];
      if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&inst);
          load_local != nullptr) {
        if (const auto result_name =
                existing_prepared_value_name_id(prepared.names, load_local->result);
            result_name.has_value()) {
          publish_source_producer(
              lookups.producers_by_value_name,
              *result_name,
              PreparedEdgePublicationSourceProducer{
                  .kind = PreparedEdgePublicationSourceProducerKind::LoadLocal,
                  .block_label = block_label,
                  .instruction_index = inst_index,
                  .load_local = load_local,
              });
        }
        continue;
      }
      if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst);
          load_global != nullptr) {
        if (const auto result_name =
                existing_prepared_value_name_id(prepared.names, load_global->result);
            result_name.has_value()) {
          publish_source_producer(
              lookups.producers_by_value_name,
              *result_name,
              PreparedEdgePublicationSourceProducer{
                  .kind = PreparedEdgePublicationSourceProducerKind::LoadGlobal,
                  .block_label = block_label,
                  .instruction_index = inst_index,
                  .load_global = load_global,
              });
        }
        continue;
      }
      if (const auto* cast = std::get_if<bir::CastInst>(&inst); cast != nullptr) {
        if (const auto result_name =
                existing_prepared_value_name_id(prepared.names, cast->result);
            result_name.has_value()) {
          publish_source_producer(
              lookups.producers_by_value_name,
              *result_name,
              PreparedEdgePublicationSourceProducer{
                  .kind = PreparedEdgePublicationSourceProducerKind::Cast,
                  .block_label = block_label,
                  .instruction_index = inst_index,
                  .cast = cast,
              });
        }
        continue;
      }
      if (const auto* binary = std::get_if<bir::BinaryInst>(&inst); binary != nullptr) {
        if (const auto result_name =
                existing_prepared_value_name_id(prepared.names, binary->result);
            result_name.has_value()) {
          publish_source_producer(
              lookups.producers_by_value_name,
              *result_name,
              PreparedEdgePublicationSourceProducer{
                  .kind = PreparedEdgePublicationSourceProducerKind::Binary,
                  .block_label = block_label,
                  .instruction_index = inst_index,
                  .binary = binary,
              });
        }
        continue;
      }
      if (const auto* select = std::get_if<bir::SelectInst>(&inst); select != nullptr) {
        if (const auto result_name =
                existing_prepared_value_name_id(prepared.names, select->result);
            result_name.has_value()) {
          publish_source_producer(
              lookups.producers_by_value_name,
              *result_name,
              PreparedEdgePublicationSourceProducer{
                  .kind =
                      PreparedEdgePublicationSourceProducerKind::SelectMaterialization,
                  .block_label = block_label,
                  .instruction_index = inst_index,
                  .select = select,
              });
        }
      }
    }
  }
  return lookups;
}

const PreparedEdgePublicationSourceProducer*
find_indexed_prepared_edge_publication_source_producer(
    const PreparedEdgePublicationSourceProducerLookups* lookups,
    ValueNameId value_name) {
  if (lookups == nullptr || value_name == kInvalidValueName) {
    return nullptr;
  }
  const auto it = lookups->producers_by_value_name.find(value_name);
  if (it == lookups->producers_by_value_name.end()) {
    return nullptr;
  }
  return &it->second;
}

const PreparedEdgePublicationSourceProducer*
find_prepared_select_chain_source_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (source_producers == nullptr ||
      block_label == kInvalidBlockLabel ||
      block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  const auto value_name = resolve_prepared_value_name_id(names, value.name);
  if (!value_name.has_value()) {
    return nullptr;
  }
  const auto* producer =
      find_indexed_prepared_edge_publication_source_producer(
          source_producers, *value_name);
  if (producer == nullptr ||
      producer->block_label != block_label ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= block->insts.size()) {
    return nullptr;
  }
  const auto& inst = block->insts[producer->instruction_index];
  return prepared_source_producer_matches_indexed_instruction(*producer, inst, value)
             ? producer
             : nullptr;
}

PreparedDirectGlobalSelectChainDependency
find_prepared_direct_global_select_chain_dependency(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  PreparedDirectGlobalSelectChainDependency dependency;
  const auto* root = find_prepared_select_chain_source_producer(
      names, source_producers, block_label, block, value, before_instruction_index);
  if (root == nullptr) {
    return dependency;
  }
  const auto contains_direct_global_load =
      prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          value,
          before_instruction_index);
  if (!contains_direct_global_load.has_value() || !*contains_direct_global_load) {
    return dependency;
  }
  dependency.contains_direct_global_load = true;
  dependency.root_is_select =
      root->kind == PreparedEdgePublicationSourceProducerKind::SelectMaterialization;
  dependency.root_instruction_index = root->instruction_index;
  return dependency;
}

PreparedDirectGlobalSelectChainDependency
find_prepared_direct_global_select_chain_dependency(
    const PreparedSelectChainDependencyQuery& query,
    const bir::Value& value) {
  if (query.names == nullptr) {
    return {};
  }
  return find_prepared_direct_global_select_chain_dependency(
      *query.names,
      query.source_producers,
      query.block_label,
      query.block,
      value,
      query.before_instruction_index);
}

PreparedStoreSourceDirectGlobalSelectChainDependency
find_prepared_store_source_direct_global_select_chain_dependency(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  return find_prepared_direct_global_select_chain_dependency(
      names,
      source_producers,
      block_label,
      block,
      value,
      before_instruction_index);
}

PreparedScalarSelectChainMaterialization
find_prepared_scalar_select_chain_materialization(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  PreparedScalarSelectChainMaterialization materialization;
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return materialization;
  }
  const auto value_name = resolve_prepared_value_name_id(names, value.name);
  if (!value_name.has_value()) {
    return materialization;
  }
  const auto* root = find_prepared_select_chain_source_producer(
      names, source_producers, block_label, block, value, before_instruction_index);
  if (root == nullptr) {
    return materialization;
  }
  auto dependency =
      find_prepared_direct_global_select_chain_dependency(
          names,
          source_producers,
          block_label,
          block,
          value,
          before_instruction_index);
  materialization.available = true;
  materialization.root_value_name = *value_name;
  materialization.root_is_select =
      root->kind == PreparedEdgePublicationSourceProducerKind::SelectMaterialization;
  materialization.root_instruction_index = root->instruction_index;
  materialization.direct_global_dependency = dependency;
  return materialization;
}

PreparedScalarSelectChainMaterialization
find_prepared_scalar_select_chain_materialization(
    const PreparedSelectChainDependencyQuery& query,
    const bir::Value& value) {
  if (query.names == nullptr) {
    return {};
  }
  return find_prepared_scalar_select_chain_materialization(
      *query.names,
      query.source_producers,
      query.block_label,
      query.block,
      value,
      query.before_instruction_index);
}

}  // namespace c4c::backend::prepare
