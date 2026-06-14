#pragma once

#include "../../shared/text_id_table.hpp"

namespace c4c::backend::bir {
struct Block;
struct Function;
}  // namespace c4c::backend::bir

namespace c4c::backend::prepare {

struct PreparedBirModule;
struct PreparedControlFlowBlock;
struct PreparedControlFlowFunction;

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

}  // namespace c4c::backend::prepare
