#include "lookup_agreement.hpp"

#include "module.hpp"
#include "publication_plans.hpp"
#include "select_chain_lookups.hpp"

#include <string_view>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] const bir::Value* source_producer_result(
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

[[nodiscard]] bool source_producer_matches_instruction(
    const PreparedEdgePublicationSourceProducer& producer,
    const bir::Inst& inst) {
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

[[nodiscard]] const bir::Value* instruction_result_value_ref(
    const bir::Inst& inst) {
  if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&inst)) {
    return &load_local->result;
  }
  if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst)) {
    return &load_global->result;
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
    return &cast->result;
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    return &binary->result;
  }
  if (const auto* select = std::get_if<bir::SelectInst>(&inst)) {
    return &select->result;
  }
  if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
    return call->result.has_value() ? &*call->result : nullptr;
  }
  return nullptr;
}

}  // namespace

PreparedBirFunctionAgreement prepared_bir_function_agreement(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function) {
  if (function.function_name == kInvalidFunctionName) {
    return {};
  }
  const std::string_view function_name =
      prepared_function_name(prepared.names, function.function_name);
  if (function_name.empty()) {
    return {};
  }

  PreparedBirFunctionAgreement agreement;
  for (const auto& bir_function : prepared.module.functions) {
    if (std::string_view{bir_function.name} != function_name) {
      continue;
    }
    if (agreement.function != nullptr) {
      agreement.function = nullptr;
      agreement.available = false;
      agreement.conflicted = true;
      return agreement;
    }
    agreement.function = &bir_function;
    agreement.available = true;
  }
  return agreement;
}

PreparedBirBlockLabelAgreement prepared_bir_block_label_agreement(
    const PreparedBirModule& prepared,
    const bir::Block& block) {
  if (block.label_id == kInvalidBlockLabel) {
    return {};
  }

  const std::string_view structured_label =
      prepared.module.names.block_labels.spelling(block.label_id);
  const std::string_view fallback_structured_label =
      structured_label.empty() ? prepared.names.block_labels.spelling(block.label_id)
                               : structured_label;
  if (fallback_structured_label.empty()) {
    return PreparedBirBlockLabelAgreement{.conflicted = true};
  }

  const BlockLabelId prepared_label =
      prepared.names.block_labels.find(fallback_structured_label);
  if (prepared_label == kInvalidBlockLabel) {
    return PreparedBirBlockLabelAgreement{.conflicted = true};
  }

  return PreparedBirBlockLabelAgreement{
      .block_label = prepared_label,
      .available = true,
  };
}

PreparedBirBlockAgreement prepared_bir_block_agreement(
    const PreparedBirModule& prepared,
    const bir::Function& function,
    const PreparedControlFlowBlock& block) {
  if (block.block_label == kInvalidBlockLabel) {
    return {};
  }
  const std::string_view prepared_label =
      prepared_block_label(prepared.names, block.block_label);
  if (prepared_label.empty()) {
    return {};
  }

  PreparedBirBlockAgreement agreement;
  for (const auto& bir_block : function.blocks) {
    const auto label_agreement =
        prepared_bir_block_label_agreement(prepared, bir_block);
    if (label_agreement.available) {
      if (label_agreement.block_label != block.block_label) {
        if (std::string_view{bir_block.label} == prepared_label) {
          agreement.block = nullptr;
          agreement.available = false;
          agreement.conflicted = true;
        }
        continue;
      }
      if (agreement.block != nullptr) {
        agreement.block = nullptr;
        agreement.available = false;
        agreement.conflicted = true;
        return agreement;
      }
      agreement.block = &bir_block;
      agreement.available = true;
      continue;
    }
    if (label_agreement.conflicted &&
        std::string_view{bir_block.label} == prepared_label) {
      agreement.block = nullptr;
      agreement.available = false;
      agreement.conflicted = true;
    }
  }
  return agreement;
}

BlockLabelId compatible_prepared_bir_block_label_id(const PreparedBirModule& prepared,
                                                    const bir::Block& block) {
  const auto agreement = prepared_bir_block_label_agreement(prepared, block);
  if (agreement.available) {
    return agreement.block_label;
  }
  if (agreement.conflicted) {
    return kInvalidBlockLabel;
  }
  return prepared.names.block_labels.find(block.label);
}

PreparedBirValueNameAgreement prepared_bir_value_name_agreement(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (source_producers == nullptr ||
      block_label == kInvalidBlockLabel ||
      block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty() ||
      value_name == kInvalidValueName ||
      names.value_names.find(value.name) != value_name) {
    return {};
  }

  const auto* producer =
      find_indexed_prepared_edge_publication_source_producer(
          source_producers, value_name);
  if (producer == nullptr ||
      producer->block_label != block_label ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= block->insts.size()) {
    return {};
  }

  const auto& inst = block->insts[producer->instruction_index];
  if (!source_producer_matches_instruction(*producer, inst)) {
    return {};
  }

  const auto* producer_result = source_producer_result(*producer);
  const auto* instruction_result = instruction_result_value_ref(inst);
  if (producer_result == nullptr ||
      instruction_result == nullptr ||
      producer_result != instruction_result ||
      instruction_result->kind != bir::Value::Kind::Named ||
      instruction_result->name.empty() ||
      instruction_result->name != value.name ||
      instruction_result->type != value.type ||
      names.value_names.find(instruction_result->name) != value_name) {
    return {};
  }

  return PreparedBirValueNameAgreement{
      .source_producer = producer,
      .instruction = &inst,
      .produced_value = instruction_result,
      .value_name = value_name,
      .instruction_index = producer->instruction_index,
      .available = true,
  };
}

}  // namespace c4c::backend::prepare
