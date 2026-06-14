#include "lookup_agreement.hpp"

#include "module.hpp"

#include <string_view>

namespace c4c::backend::prepare {

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

}  // namespace c4c::backend::prepare
