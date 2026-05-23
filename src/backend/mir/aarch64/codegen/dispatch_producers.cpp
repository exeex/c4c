#include "dispatch_producers.hpp"

#include "dispatch_lookup.hpp"
#include "dispatch_publication.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] const bir::BinaryInst* find_same_block_binary_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  for (const auto& inst : context.bir_block->insts) {
    const auto* binary = std::get_if<bir::BinaryInst>(&inst);
    if (binary != nullptr &&
        binary->result.kind == bir::Value::Kind::Named &&
        binary->result.name == value.name) {
      return binary;
    }
  }
  return nullptr;
}

[[nodiscard]] SameBlockSelectProducer find_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return {};
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const std::size_t candidate_index = index - 1;
    const auto* select =
        std::get_if<bir::SelectInst>(&context.bir_block->insts[candidate_index]);
    if (select != nullptr &&
        select->result.kind == bir::Value::Kind::Named &&
        select->result.name == value.name) {
      return SameBlockSelectProducer{.select = select,
                                     .instruction_index = candidate_index};
    }
  }
  return {};
}

[[nodiscard]] std::optional<std::int64_t> evaluate_same_block_integer_constant(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    unsigned depth) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return value.immediate;
  }
  if (depth > 4U) {
    return std::nullopt;
  }
  const auto* binary = find_same_block_binary_producer(context, value);
  if (binary == nullptr) {
    return std::nullopt;
  }
  const auto lhs = evaluate_same_block_integer_constant(context, binary->lhs, depth + 1);
  const auto rhs = evaluate_same_block_integer_constant(context, binary->rhs, depth + 1);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  switch (binary->opcode) {
    case bir::BinaryOpcode::Add:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(*lhs) +
                                       static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::Sub:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(*lhs) -
                                       static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::Mul:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(*lhs) *
                                       static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::And:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(*lhs) &
                                       static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::Or:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(*lhs) |
                                       static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::Xor:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(*lhs) ^
                                       static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::Shl:
      if (*rhs < 0 || *rhs >= 64) {
        return std::nullopt;
      }
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(*lhs)
                                       << static_cast<unsigned>(*rhs));
    case bir::BinaryOpcode::LShr:
      if (*rhs < 0 || *rhs >= 64) {
        return std::nullopt;
      }
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(*lhs)
                                       >> static_cast<unsigned>(*rhs));
    case bir::BinaryOpcode::AShr:
      if (*rhs < 0 || *rhs >= 64) {
        return std::nullopt;
      }
      return *lhs >> static_cast<unsigned>(*rhs);
    case bir::BinaryOpcode::SDiv:
      if (*rhs == 0) {
        return std::nullopt;
      }
      return *lhs / *rhs;
    case bir::BinaryOpcode::UDiv:
      if (*rhs == 0) {
        return std::nullopt;
      }
      return static_cast<std::int64_t>(
          static_cast<std::uint64_t>(*lhs) / static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::SRem:
      if (*rhs == 0) {
        return std::nullopt;
      }
      return *lhs % *rhs;
    case bir::BinaryOpcode::URem:
      if (*rhs == 0) {
        return std::nullopt;
      }
      return static_cast<std::int64_t>(
          static_cast<std::uint64_t>(*lhs) % static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::Eq:
      return *lhs == *rhs ? 1 : 0;
    case bir::BinaryOpcode::Ne:
      return *lhs != *rhs ? 1 : 0;
    case bir::BinaryOpcode::Slt:
      return *lhs < *rhs ? 1 : 0;
    case bir::BinaryOpcode::Sle:
      return *lhs <= *rhs ? 1 : 0;
    case bir::BinaryOpcode::Sgt:
      return *lhs > *rhs ? 1 : 0;
    case bir::BinaryOpcode::Sge:
      return *lhs >= *rhs ? 1 : 0;
    case bir::BinaryOpcode::Ult:
      return static_cast<std::uint64_t>(*lhs) < static_cast<std::uint64_t>(*rhs)
                 ? 1
                 : 0;
    case bir::BinaryOpcode::Ule:
      return static_cast<std::uint64_t>(*lhs) <= static_cast<std::uint64_t>(*rhs)
                 ? 1
                 : 0;
    case bir::BinaryOpcode::Ugt:
      return static_cast<std::uint64_t>(*lhs) > static_cast<std::uint64_t>(*rhs)
                 ? 1
                 : 0;
    case bir::BinaryOpcode::Uge:
      return static_cast<std::uint64_t>(*lhs) >= static_cast<std::uint64_t>(*rhs)
                 ? 1
                 : 0;
  }
  return std::nullopt;
}

[[nodiscard]] bool select_chain_contains_direct_global_load(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth) {
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  if (producer == nullptr) {
    return false;
  }
  if (std::get_if<bir::LoadGlobalInst>(producer) != nullptr) {
    return true;
  }
  const auto producer_index = producer_instruction_index(context, producer);
  const auto nested_before = producer_index.value_or(before_instruction_index);
  if (const auto* select = std::get_if<bir::SelectInst>(producer);
      select != nullptr) {
    return select_chain_contains_direct_global_load(
               context, select->true_value, nested_before, depth + 1) ||
           select_chain_contains_direct_global_load(
               context, select->false_value, nested_before, depth + 1);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    return select_chain_contains_direct_global_load(
        context, cast->operand, nested_before, depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer);
      binary != nullptr) {
    return select_chain_contains_direct_global_load(
               context, binary->lhs, nested_before, depth + 1) ||
           select_chain_contains_direct_global_load(
               context, binary->rhs, nested_before, depth + 1);
  }
  return false;
}

[[nodiscard]] const bir::Inst* find_same_block_named_producer(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr || value_name.empty()) {
    return nullptr;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const auto& candidate = context.bir_block->insts[index - 1];
    bool matches = false;
    std::visit(
        [&](const auto& typed_inst) {
          using T = std::decay_t<decltype(typed_inst)>;
          if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                        std::is_same_v<T, bir::CastInst> ||
                        std::is_same_v<T, bir::SelectInst> ||
                        std::is_same_v<T, bir::LoadLocalInst> ||
                        std::is_same_v<T, bir::LoadGlobalInst>) {
            matches = typed_inst.result.kind == bir::Value::Kind::Named &&
                      typed_inst.result.name == value_name;
          }
        },
        candidate);
    if (matches) {
      return &candidate;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const module::BlockLoweringContext& context,
    const bir::Inst* producer) {
  if (context.bir_block == nullptr || producer == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < context.bir_block->insts.size(); ++index) {
    if (&context.bir_block->insts[index] == producer) {
      return index;
    }
  }
  return std::nullopt;
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
      find_value_home(context,
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
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      if (move.to_value_id == result_home->value_id) {
        return true;
      }
      if (move.source_immediate_i32.has_value() ||
          move.from_value_id != result_home->value_id ||
          move.from_value_id == move.to_value_id) {
        continue;
      }
      const auto* destination_home =
          find_value_home(context,
move.to_value_id);
      if (destination_home != nullptr &&
          (prepared_edge_select_source_is_destination_register(*result_home,
                                                              *destination_home) ||
           result_home->kind == prepare::PreparedValueHomeKind::StackSlot)) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace c4c::backend::aarch64::codegen
