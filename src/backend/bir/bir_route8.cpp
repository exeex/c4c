#include "bir.hpp"
#include "bir_private.hpp"

#include <utility>

namespace c4c::backend::bir {
namespace {

[[nodiscard]] bool route8_value_key_matches(
    const Route1SourceValueIdentity& record_value,
    const Route1SourceValueIdentity& query_value) {
  if (record_value.name_id != kInvalidValueName ||
      query_value.name_id != kInvalidValueName) {
    return record_value.name_id == query_value.name_id;
  }
  if (!record_value.name.empty() || !query_value.name.empty()) {
    return record_value.name == query_value.name &&
           record_value.type == query_value.type;
  }
  if (record_value.value != nullptr || query_value.value != nullptr) {
    return record_value.value == query_value.value;
  }
  if (record_value.integer_constant.has_value() ||
      query_value.integer_constant.has_value()) {
    return record_value.integer_constant == query_value.integer_constant &&
           record_value.type == query_value.type;
  }
  return false;
}

[[nodiscard]] bool route8_identity_matches(
    const Route1SourceValueIdentity& lhs,
    const Route1SourceValueIdentity& rhs) {
  if (!lhs && !rhs) {
    return true;
  }
  return route8_value_key_matches(lhs, rhs) &&
         route8_value_key_matches(rhs, lhs);
}

[[nodiscard]] bool route8_key_matches(const Route8ReturnChainValueKey& record,
                                      const Route8ReturnChainValueKey& query) {
  if (query.block == nullptr) {
    return false;
  }
  if (record.function != nullptr && query.function != nullptr &&
      record.function != query.function) {
    return false;
  }
  if (record.function_link_name_id != kInvalidLinkName &&
      query.function_link_name_id != kInvalidLinkName &&
      record.function_link_name_id != query.function_link_name_id) {
    return false;
  }
  if (!record.function_name.empty() && !query.function_name.empty() &&
      record.function_name != query.function_name) {
    return false;
  }
  if (record.instruction_index != query.instruction_index) {
    return false;
  }
  if (!route_block_matches(record.block_label, record.block_label_id,
                           *query.block)) {
    return false;
  }
  return route8_value_key_matches(record.chain_value, query.chain_value);
}

[[nodiscard]] bool route8_return_chain_binary_opcode_is_scalar_publication(
    BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Add:
    case BinaryOpcode::Sub:
    case BinaryOpcode::Mul:
    case BinaryOpcode::And:
    case BinaryOpcode::Or:
    case BinaryOpcode::Xor:
    case BinaryOpcode::Shl:
    case BinaryOpcode::LShr:
    case BinaryOpcode::AShr:
    case BinaryOpcode::SDiv:
    case BinaryOpcode::UDiv:
    case BinaryOpcode::SRem:
    case BinaryOpcode::URem:
      return true;
    case BinaryOpcode::Eq:
    case BinaryOpcode::Ne:
    case BinaryOpcode::Slt:
    case BinaryOpcode::Sle:
    case BinaryOpcode::Sgt:
    case BinaryOpcode::Sge:
    case BinaryOpcode::Ult:
    case BinaryOpcode::Ule:
    case BinaryOpcode::Ugt:
    case BinaryOpcode::Uge:
      return false;
  }
  return false;
}

[[nodiscard]] bool route8_is_named_value(const Value& value) {
  return value.kind == Value::Kind::Named && !value.name.empty();
}

[[nodiscard]] bool route8_value_matches_name(const Value& value,
                                             std::string_view name) {
  return route8_is_named_value(value) && value.name == name;
}

[[nodiscard]] bool route8_record_conflicts(
    const Route8ReturnChainRecord& lhs,
    const Route8ReturnChainRecord& rhs) {
  return !route8_identity_matches(lhs.terminal_return_value,
                                  rhs.terminal_return_value) ||
         !route8_identity_matches(lhs.next_operand_value,
                                  rhs.next_operand_value);
}

void route8_publish_return_chain_record(Route8ReturnChainIndex& index,
                                        Route8ReturnChainRecord record) {
  if (!record.available) {
    return;
  }
  for (auto& existing : index.records) {
    if (!route8_key_matches(existing.key, record.key)) {
      continue;
    }
    if (existing.status == Route8ReturnChainStatus::DuplicateRecord ||
        route8_record_conflicts(existing, record)) {
      existing.available = false;
      existing.status = Route8ReturnChainStatus::DuplicateRecord;
      existing.terminal_return_value = {};
      existing.next_operand_value = {};
    }
    return;
  }
  index.records.push_back(std::move(record));
}

void route8_publish_return_chain_records_for_block(Route8ReturnChainIndex& index,
                                                   const Function* function,
                                                   const Block& block) {
  if (block.terminator.kind != TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      !route8_is_named_value(*block.terminator.value)) {
    return;
  }

  for (std::size_t instruction_index = 0;
       instruction_index < block.insts.size();
       ++instruction_index) {
    const auto* binary =
        std::get_if<BinaryInst>(&block.insts[instruction_index]);
    if (binary == nullptr ||
        !route8_return_chain_binary_opcode_is_scalar_publication(
            binary->opcode) ||
        !route8_is_named_value(binary->result)) {
      continue;
    }

    std::string_view current_name = binary->result.name;
    const Value* next_operand_value = nullptr;
    for (std::size_t next_index = instruction_index + 1;
         next_index < block.insts.size();
         ++next_index) {
      const auto* next_binary =
          std::get_if<BinaryInst>(&block.insts[next_index]);
      if (next_binary == nullptr ||
          !route8_return_chain_binary_opcode_is_scalar_publication(
              next_binary->opcode)) {
        break;
      }
      const bool consumes_lhs =
          route8_value_matches_name(next_binary->lhs, current_name);
      const bool consumes_rhs =
          route8_value_matches_name(next_binary->rhs, current_name);
      if ((!consumes_lhs && !consumes_rhs) ||
          !route8_is_named_value(next_binary->result)) {
        break;
      }
      if (next_index == instruction_index + 1) {
        const Value& other_operand =
            consumes_lhs ? next_binary->rhs : next_binary->lhs;
        if (route8_is_named_value(other_operand)) {
          next_operand_value = &other_operand;
        }
      }
      current_name = next_binary->result.name;
    }

    if (!route8_value_matches_name(*block.terminator.value, current_name)) {
      continue;
    }

    route8_publish_return_chain_record(
        index,
        route8_return_chain_record(
            route8_return_chain_value_key(function,
                                          block,
                                          instruction_index,
                                          binary->result),
            &*block.terminator.value,
            kInvalidValueName,
            next_operand_value,
            kInvalidValueName));
  }
}

[[nodiscard]] Route8ReturnChainStatus route8_missing_block_status(
    const Route8ReturnChainIndex& index,
    const Route8ReturnChainValueKey& key) {
  if (key.block == nullptr) {
    return Route8ReturnChainStatus::MissingBlock;
  }
  if (!key.chain_value) {
    return Route8ReturnChainStatus::MissingChainValue;
  }
  if (key.instruction_index >= key.block->insts.size()) {
    return Route8ReturnChainStatus::MissingInstruction;
  }
  if (index.function == nullptr && index.block == nullptr) {
    return Route8ReturnChainStatus::MissingBlock;
  }
  if (index.block != nullptr &&
      !route_block_matches(key.block_label, key.block_label_id,
                           *index.block)) {
    return Route8ReturnChainStatus::MissingBlock;
  }
  if (index.function != nullptr) {
    for (const auto& block : index.function->blocks) {
      if (route_block_matches(key.block_label, key.block_label_id, block)) {
        return Route8ReturnChainStatus::NoMatch;
      }
    }
    return Route8ReturnChainStatus::MissingBlock;
  }
  return Route8ReturnChainStatus::NoMatch;
}

}  // namespace

Route8ReturnChainValueKey route8_return_chain_value_key(
    const Function* function,
    const Block& block,
    std::size_t instruction_index,
    const Value& chain_value,
    ValueNameId chain_value_name_id) {
  Route8ReturnChainValueKey key{
      .function = function,
      .block = &block,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .instruction_index = instruction_index,
      .chain_value =
          route1_source_value_identity(chain_value, chain_value_name_id),
  };
  if (function != nullptr) {
    key.function_name = function->name;
    key.function_link_name_id = function->link_name_id;
  }
  return key;
}

Route8ReturnChainRecord route8_return_chain_record(
    const Route8ReturnChainValueKey& key,
    const Value* terminal_return_value,
    ValueNameId terminal_return_value_name_id,
    const Value* next_operand_value,
    ValueNameId next_operand_value_name_id) {
  Route8ReturnChainRecord record{
      .status = Route8ReturnChainStatus::Unavailable,
      .key = key,
  };
  if (key.block == nullptr) {
    record.status = Route8ReturnChainStatus::MissingBlock;
    return record;
  }
  if (!key.chain_value) {
    record.status = Route8ReturnChainStatus::MissingChainValue;
    return record;
  }
  if (terminal_return_value == nullptr) {
    record.status = Route8ReturnChainStatus::MissingTerminalValue;
    return record;
  }
  record.available = true;
  record.status = Route8ReturnChainStatus::Available;
  record.terminal_return_value =
      route1_source_value_identity(*terminal_return_value,
                                   terminal_return_value_name_id);
  if (next_operand_value != nullptr) {
    record.next_operand_value =
        route1_source_value_identity(*next_operand_value,
                                     next_operand_value_name_id);
  }
  return record;
}

Route8ReturnChainIndex route8_build_return_chain_index(
    const Function& function) {
  Route8ReturnChainIndex index{
      .function = &function,
  };
  for (const auto& block : function.blocks) {
    route8_publish_return_chain_records_for_block(index, &function, block);
  }
  return index;
}

Route8ReturnChainIndex route8_build_return_chain_index(const Block& block) {
  Route8ReturnChainIndex index{
      .block = &block,
  };
  route8_publish_return_chain_records_for_block(index, nullptr, block);
  return index;
}

Route8ReturnChainRecord route8_find_return_chain_record(
    const Route8ReturnChainIndex& index,
    const Route8ReturnChainValueKey& key) {
  if (key.block == nullptr) {
    return Route8ReturnChainRecord{
        .status = Route8ReturnChainStatus::MissingBlock,
        .key = key,
    };
  }
  const Route8ReturnChainRecord* match = nullptr;
  for (const auto& record : index.records) {
    if (!route8_key_matches(record.key, key)) {
      continue;
    }
    if (match != nullptr) {
      return Route8ReturnChainRecord{
          .status = Route8ReturnChainStatus::DuplicateRecord,
          .key = key,
      };
    }
    match = &record;
  }
  if (match != nullptr) {
    return *match;
  }
  return Route8ReturnChainRecord{
      .status = route8_missing_block_status(index, key),
      .key = key,
  };
}

Route1SourceValueIdentity route8_find_return_chain_terminal_value(
    const Route8ReturnChainIndex& index,
    const Route8ReturnChainValueKey& key) {
  const auto record = route8_find_return_chain_record(index, key);
  if (!record.available) {
    return {};
  }
  return record.terminal_return_value;
}

Route1SourceValueIdentity route8_find_return_chain_next_operand_value(
    const Route8ReturnChainIndex& index,
    const Route8ReturnChainValueKey& key) {
  const auto record = route8_find_return_chain_record(index, key);
  if (!record.available) {
    return {};
  }
  return record.next_operand_value;
}

}  // namespace c4c::backend::bir
