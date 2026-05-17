#include "label_identity.hpp"

#include <string_view>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] BlockLabelId find_preferred_block_label_id(const PreparedNameTables& names,
                                                         const bir::NameTables& bir_names,
                                                         BlockLabelId label_id,
                                                         std::string_view raw_label) {
  if (label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(label_id);
    if (!structured_label.empty()) {
      const BlockLabelId prepared_label_id = names.block_labels.find(structured_label);
      if (prepared_label_id != kInvalidBlockLabel) {
        return prepared_label_id;
      }
    }
  }
  return names.block_labels.find(raw_label);
}

}  // namespace

void publish_prepared_bir_label_identity(PreparedBirModule& prepared) {
  for (auto& function : prepared.module.functions) {
    const auto function_name_id = prepared.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    const auto* control_flow =
        find_prepared_control_flow_function(prepared.control_flow, function_name_id);
    if (control_flow == nullptr) {
      continue;
    }

    auto intern_bir_label = [&](BlockLabelId prepared_label) -> BlockLabelId {
      if (prepared_label == kInvalidBlockLabel) {
        return kInvalidBlockLabel;
      }
      const std::string_view spelling = prepared_block_label(prepared.names, prepared_label);
      if (spelling.empty()) {
        return kInvalidBlockLabel;
      }
      return prepared.module.names.block_labels.intern(spelling);
    };

    auto block_for_prepared_label = [&](BlockLabelId prepared_label) -> bir::Block* {
      if (prepared_label == kInvalidBlockLabel) {
        return nullptr;
      }
      const std::string_view spelling = prepared_block_label(prepared.names, prepared_label);
      for (auto& block : function.blocks) {
        const auto preferred =
            find_preferred_block_label_id(prepared.names,
                                          prepared.module.names,
                                          block.label_id,
                                          block.label);
        if (preferred == prepared_label || block.label == spelling) {
          return &block;
        }
      }
      return nullptr;
    };

    for (const auto& prepared_block : control_flow->blocks) {
      auto* block = block_for_prepared_label(prepared_block.block_label);
      if (block == nullptr) {
        continue;
      }

      block->label_id = intern_bir_label(prepared_block.block_label);
      switch (block->terminator.kind) {
        case bir::TerminatorKind::Return:
          break;
        case bir::TerminatorKind::Branch:
          block->terminator.target_label_id =
              intern_bir_label(prepared_block.branch_target_label);
          break;
        case bir::TerminatorKind::CondBranch:
          block->terminator.true_label_id = intern_bir_label(prepared_block.true_label);
          block->terminator.false_label_id = intern_bir_label(prepared_block.false_label);
          break;
      }
    }

    for (const auto& transfer : control_flow->join_transfers) {
      for (const auto& incoming : transfer.incomings) {
        auto* block = block_for_prepared_label(
            prepared.names.block_labels.find(incoming.label));
        if (block != nullptr && block->label_id == kInvalidBlockLabel) {
          block->label_id =
              prepared.module.names.block_labels.intern(incoming.label);
        }
      }
    }
  }
}

}  // namespace c4c::backend::prepare
