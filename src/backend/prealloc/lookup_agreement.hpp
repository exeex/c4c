#pragma once

#include "names.hpp"

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
struct PreparedRegallocFunction;
struct PreparedValueHome;
struct PreparedValueHomeLookups;
struct PreparedValueLocationFunction;

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

struct PreparedBirValueHomeAgreement {
  const PreparedValueHome* home = nullptr;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
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

[[nodiscard]] PreparedBirValueHomeAgreement prepared_bir_value_home_agreement(
    const PreparedNameTables& names,
    const PreparedValueHomeLookups* value_home_lookups,
    const PreparedRegallocFunction* regalloc,
    const PreparedValueLocationFunction* function_locations,
    const bir::Value& value);

}  // namespace c4c::backend::prepare
