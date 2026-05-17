#include "consumer_moves.hpp"

#include "move_records.hpp"
#include "storage.hpp"

#include <initializer_list>
#include <optional>
#include <string_view>
#include <type_traits>

namespace c4c::backend::prepare::regalloc_detail {

namespace {

[[nodiscard]] std::optional<BlockLabelId> resolve_existing_consumer_block_label_id(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Block& block) {
  if (block.label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      const BlockLabelId prepared_label_id = names.block_labels.find(structured_label);
      if (prepared_label_id != kInvalidBlockLabel) {
        return prepared_label_id;
      }
    }
  }
  return resolve_prepared_block_label_id(names, block.label);
}

}  // namespace

void append_consumer_move_resolution(const PreparedNameTables& names,
                                     const bir::NameTables& bir_names,
                                     const bir::Function& function,
                                     const PreparedControlFlowFunction* function_cf,
                                     PreparedRegallocFunction& regalloc_function) {
  auto is_authoritative_select_materialized_join_result =
      [&](const bir::Block& block, const bir::SelectInst& select) {
        if (function_cf == nullptr || select.result.kind != bir::Value::Kind::Named) {
          return false;
        }

        const auto block_label_id = resolve_existing_consumer_block_label_id(names, bir_names, block);
        if (!block_label_id.has_value()) {
          return false;
        }

        const auto* join_transfer =
            find_prepared_join_transfer(names, *function_cf, *block_label_id, select.result.name);
        return join_transfer != nullptr &&
               effective_prepared_join_transfer_carrier_kind(*join_transfer) ==
                   PreparedJoinTransferCarrierKind::SelectMaterialization;
      };

  auto append_for_instruction = [&](const bir::Value& result,
                                    std::initializer_list<const bir::Value*> operands,
                                    std::size_t block_index,
                                    std::size_t instruction_index) {
    if (result.kind != bir::Value::Kind::Named) {
      return;
    }

    const auto* destination = find_regalloc_value(regalloc_function, names, result.name);
    if (destination == nullptr) {
      return;
    }

    for (const bir::Value* operand : operands) {
      if (operand == nullptr || operand->kind != bir::Value::Kind::Named) {
        continue;
      }
      const auto* source = find_regalloc_value(regalloc_function, names, operand->name);
      if (source == nullptr) {
        continue;
      }

      append_move_resolution_record(regalloc_function,
                                    *source,
                                    *destination,
                                    block_index,
                                    instruction_index,
                                    false,
                                    false,
                                    std::nullopt,
                                    PreparedMoveResolutionOpKind::Move,
                                    PreparedMoveAuthorityKind::None,
                                    storage_transfer_reason("consumer", *source, *destination));
    }
  };

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      std::visit(
          [&](const auto& inst) {
            using Inst = std::decay_t<decltype(inst)>;
            if constexpr (std::is_same_v<Inst, bir::BinaryInst>) {
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs},
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::SelectInst>) {
              // Select-materialized joins are already owned by published out-of-SSA
              // join-transfer authority. Re-adding them here would regress into
              // consumer-shaped reconstruction.
              if (is_authoritative_select_materialized_join_result(block, inst)) {
                return;
              }
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs, &inst.true_value, &inst.false_value},
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::CastInst>) {
              append_for_instruction(inst.result, {&inst.operand}, block_index, instruction_index);
            }
          },
          block.insts[instruction_index]);
    }
  }
}

}  // namespace c4c::backend::prepare::regalloc_detail
