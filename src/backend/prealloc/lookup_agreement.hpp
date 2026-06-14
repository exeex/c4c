#pragma once

#include "../../shared/text_id_table.hpp"
#include "../bir/bir.hpp"

#include <cstddef>

namespace c4c::backend::bir {
struct Block;
struct Function;
struct Value;
}  // namespace c4c::backend::bir

namespace c4c::backend::prepare {

struct PreparedBirModule;
struct PreparedControlFlowBlock;
struct PreparedControlFlowFunction;
struct PreparedEdgePublicationSourceProducer;
struct PreparedEdgePublicationSourceProducerLookups;
struct PreparedNameTables;

struct PreparedBirFunctionAgreement {
  const bir::Function* function = nullptr;
  bool available = false;
  bool conflicted = false;
};

struct PreparedBirBlockAgreement {
  const bir::Block* block = nullptr;
  bool available = false;
  bool conflicted = false;
};

struct PreparedBirBlockLabelAgreement {
  BlockLabelId block_label = kInvalidBlockLabel;
  bool available = false;
  bool conflicted = false;
};

struct PreparedBirValueNameAgreement {
  const PreparedEdgePublicationSourceProducer* source_producer = nullptr;
  const bir::Inst* instruction = nullptr;
  const bir::Value* produced_value = nullptr;
  ValueNameId value_name = kInvalidValueName;
  std::size_t instruction_index = 0;
  bool available = false;
};

[[nodiscard]] PreparedBirFunctionAgreement prepared_bir_function_agreement(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

[[nodiscard]] PreparedBirBlockAgreement prepared_bir_block_agreement(
    const PreparedBirModule& prepared,
    const bir::Function& function,
    const PreparedControlFlowBlock& block);

[[nodiscard]] PreparedBirBlockLabelAgreement prepared_bir_block_label_agreement(
    const PreparedBirModule& prepared,
    const bir::Block& block);

[[nodiscard]] BlockLabelId compatible_prepared_bir_block_label_id(
    const PreparedBirModule& prepared,
    const bir::Block& block);

[[nodiscard]] PreparedBirValueNameAgreement prepared_bir_value_name_agreement(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    ValueNameId value_name,
    std::size_t before_instruction_index);

}  // namespace c4c::backend::prepare
