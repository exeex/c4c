#include "prealloc.hpp"

#include <algorithm>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

bool should_promote_i1(const c4c::TargetProfile& target_profile) {
  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
    case c4c::TargetArch::I686:
    case c4c::TargetArch::Aarch64:
    case c4c::TargetArch::Riscv64:
      return true;
    case c4c::TargetArch::Unknown:
      return false;
  }
  return false;
}

bir::TypeKind legalize_type(const c4c::TargetProfile& target_profile, bir::TypeKind type) {
  if (should_promote_i1(target_profile) && type == bir::TypeKind::I1) {
    return bir::TypeKind::I32;
  }
  return type;
}

void legalize_value(const c4c::TargetProfile& target_profile, bir::Value& value) {
  const auto original_type = value.type;
  value.type = legalize_type(target_profile, value.type);
  if (original_type == bir::TypeKind::I1 && value.type == bir::TypeKind::I32 &&
      value.kind == bir::Value::Kind::Immediate) {
    value.immediate = value.immediate != 0 ? 1 : 0;
    value.immediate_bits = value.immediate != 0 ? 1u : 0u;
  }
}

std::size_t type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
      return 8;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return 16;
    default:
      return 0;
  }
}

void legalize_sized_type(const c4c::TargetProfile& target_profile,
                         bir::TypeKind& type,
                         std::size_t& size_bytes,
                         std::size_t& align_bytes) {
  const auto original_type = type;
  type = legalize_type(target_profile, type);
  if (original_type != type) {
    const auto legalized_size = type_size_bytes(type);
    size_bytes = legalized_size;
    align_bytes = legalized_size;
  }
}

void legalize_call_arg_abi(const c4c::TargetProfile& target_profile, bir::CallArgAbiInfo& abi) {
  legalize_sized_type(target_profile, abi.type, abi.size_bytes, abi.align_bytes);
}

void legalize_call_result_abi(const c4c::TargetProfile& target_profile,
                              bir::CallResultAbiInfo& abi) {
  abi.type = legalize_type(target_profile, abi.type);
}

std::optional<bir::CallArgAbiInfo> infer_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type) {
  if (type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  bir::CallArgAbiInfo abi{
      .type = type,
      .size_bytes = type_size_bytes(type),
      .align_bytes = type_size_bytes(type),
      .passed_in_register = true,
  };
  switch (type) {
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      abi.primary_class = target_profile.has_float_arg_registers
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.passed_in_register = false;
      abi.passed_on_stack = true;
      return abi;
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<bir::CallResultAbiInfo> infer_function_return_abi(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function) {
  if (function.return_type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  bir::CallResultAbiInfo abi;
  abi.type = function.return_type;
  switch (function.return_type) {
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      abi.primary_class = target_profile.has_float_return_registers
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      break;
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      break;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.returned_in_memory = true;
      break;
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return abi;
}

void legalize_memory_access_metadata(const c4c::TargetProfile& target_profile,
                                     bir::TypeKind original_type,
                                     std::size_t& align_bytes,
                                     std::optional<bir::MemoryAddress>& address) {
  const auto legalized_type = legalize_type(target_profile, original_type);
  if (legalized_type == original_type) {
    return;
  }
  const auto original_size = type_size_bytes(original_type);
  const auto legalized_size = type_size_bytes(legalized_type);
  if (original_size == 0 || legalized_size == 0) {
    return;
  }
  if (align_bytes == original_size) {
    align_bytes = legalized_size;
  }
  if (!address.has_value()) {
    return;
  }
  if (address->size_bytes == original_size) {
    address->size_bytes = legalized_size;
  }
  if (address->align_bytes == original_size) {
    address->align_bytes = legalized_size;
  }
}

std::string make_phi_slot_name(std::string_view result_name) {
  return std::string(result_name) + ".phi";
}

bir::LocalSlot* find_local_slot(bir::Function* function, std::string_view slot_name) {
  for (auto& slot : function->local_slots) {
    if (slot.name == slot_name) {
      return &slot;
    }
  }
  return nullptr;
}

bool is_materializable_inst(const bir::Inst& inst) {
  return std::holds_alternative<bir::BinaryInst>(inst) || std::holds_alternative<bir::CastInst>(inst) ||
         std::holds_alternative<bir::PhiInst>(inst);
}

std::optional<std::string> inst_result_name(const bir::Inst& inst) {
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    return binary->result.name;
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
    return cast->result.name;
  }
  if (const auto* phi = std::get_if<bir::PhiInst>(&inst)) {
    return phi->result.name;
  }
  if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&inst)) {
    return load_local->result.name;
  }
  if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst)) {
    return load_global->result.name;
  }
  if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
    if (call->result.has_value()) {
      return call->result->name;
    }
  }
  return std::nullopt;
}

const bir::Block* find_block(const bir::Function& function, std::string_view label) {
  for (const auto& block : function.blocks) {
    if (block.label == label) {
      return &block;
    }
  }
  return nullptr;
}

bool block_has_successor(const bir::Block& block, std::string_view successor_label) {
  if (block.terminator.kind == bir::TerminatorKind::Branch) {
    return block.terminator.target_label == successor_label;
  }
  if (block.terminator.kind == bir::TerminatorKind::CondBranch) {
    return block.terminator.true_label == successor_label ||
           block.terminator.false_label == successor_label;
  }
  return false;
}

bool is_reachable_without_revisiting(const bir::Function& function,
                                     std::string_view start_label,
                                     std::string_view target_label) {
  if (start_label == target_label) {
    return true;
  }

  std::vector<std::string> worklist{std::string(start_label)};
  std::unordered_set<std::string> visited;
  bool found_target = false;
  while (!worklist.empty()) {
    auto label = std::move(worklist.back());
    worklist.pop_back();
    if (!visited.insert(label).second) {
      continue;
    }

    const auto* block = find_block(function, label);
    if (block == nullptr) {
      continue;
    }

    const auto push_successor = [&](std::string_view successor_label) {
      if (successor_label == target_label) {
        found_target = true;
        return true;
      }
      if (visited.find(std::string(successor_label)) == visited.end()) {
        worklist.push_back(std::string(successor_label));
      }
      return false;
    };

    if (block->terminator.kind == bir::TerminatorKind::Branch) {
      if (push_successor(block->terminator.target_label)) {
        break;
      }
    } else if (block->terminator.kind == bir::TerminatorKind::CondBranch) {
      if (push_successor(block->terminator.true_label)) {
        break;
      }
      if (push_successor(block->terminator.false_label)) {
        break;
      }
    }
  }

  return found_target;
}

PreparedJoinTransferKind classify_phi_join_transfer_kind(const bir::Function& function,
                                                         std::string_view join_block_label,
                                                         const bir::PhiInst& phi) {
  for (const auto& incoming : phi.incomings) {
    const auto* predecessor = find_block(function, incoming.label);
    if (predecessor == nullptr || !block_has_successor(*predecessor, join_block_label)) {
      continue;
    }
    if (is_reachable_without_revisiting(function, join_block_label, incoming.label)) {
      return PreparedJoinTransferKind::LoopCarry;
    }
  }
  return PreparedJoinTransferKind::PhiEdge;
}

bool terminator_uses_named_value(const bir::Terminator& terminator,
                                 const std::unordered_set<std::string>& names) {
  const auto matches = [&](const std::optional<bir::Value>& value) {
    return value.has_value() && value->kind == bir::Value::Kind::Named &&
           names.find(value->name) != names.end();
  };
  if (matches(terminator.value)) {
    return true;
  }
  return terminator.kind == bir::TerminatorKind::CondBranch &&
         terminator.condition.kind == bir::Value::Kind::Named &&
         names.find(terminator.condition.name) != names.end();
}

bool block_uses_named_value(const bir::Block& block, const std::unordered_set<std::string>& names) {
  for (const auto& inst : block.insts) {
    const auto matches = [&](const bir::Value& value) {
      return value.kind == bir::Value::Kind::Named && names.find(value.name) != names.end();
    };
    if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
      if (matches(binary->lhs) || matches(binary->rhs)) {
        return true;
      }
      continue;
    }
    if (const auto* select = std::get_if<bir::SelectInst>(&inst)) {
      if (matches(select->lhs) || matches(select->rhs) || matches(select->true_value) ||
          matches(select->false_value)) {
        return true;
      }
      continue;
    }
    if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
      if (matches(cast->operand)) {
        return true;
      }
      continue;
    }
    if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
      if (call->callee_value.has_value() && matches(*call->callee_value)) {
        return true;
      }
      for (const auto& arg : call->args) {
        if (matches(arg)) {
          return true;
        }
      }
      continue;
    }
    if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&inst)) {
      if (load_local->address.has_value() && matches(load_local->address->base_value)) {
        return true;
      }
      continue;
    }
    if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst)) {
      if (load_global->address.has_value() && matches(load_global->address->base_value)) {
        return true;
      }
      continue;
    }
    if (const auto* store_local = std::get_if<bir::StoreLocalInst>(&inst)) {
      if (matches(store_local->value) ||
          (store_local->address.has_value() && matches(store_local->address->base_value))) {
        return true;
      }
      continue;
    }
    if (const auto* store_global = std::get_if<bir::StoreGlobalInst>(&inst)) {
      if (matches(store_global->value) ||
          (store_global->address.has_value() && matches(store_global->address->base_value))) {
        return true;
      }
      continue;
    }
  }
  return terminator_uses_named_value(block.terminator, names);
}

bool successor_tree_uses_named_value(
    const std::unordered_map<std::string, bir::Block*>& blocks_by_label,
    std::string_view start_label,
    const std::unordered_set<std::string>& names) {
  std::unordered_set<std::string> visited;
  std::vector<std::string> pending{std::string(start_label)};
  while (!pending.empty()) {
    auto current_label = std::move(pending.back());
    pending.pop_back();
    if (!visited.emplace(current_label).second) {
      continue;
    }
    const auto it = blocks_by_label.find(current_label);
    if (it == blocks_by_label.end()) {
      continue;
    }
    const auto& block = *it->second;
    if (block_uses_named_value(block, names)) {
      return true;
    }
    if (block.terminator.kind == bir::TerminatorKind::Branch) {
      pending.push_back(block.terminator.target_label);
      continue;
    }
    if (block.terminator.kind == bir::TerminatorKind::CondBranch) {
      pending.push_back(block.terminator.true_label);
      pending.push_back(block.terminator.false_label);
    }
  }
  return false;
}

struct BlockAnalysis {
  std::unordered_map<std::string, bir::Block*> blocks_by_label;
  std::unordered_map<std::string, const bir::Inst*> defs_by_name;
  std::unordered_map<std::string, bir::Block*> def_blocks_by_name;
};

BlockAnalysis analyze_function(bir::Function* function) {
  BlockAnalysis analysis;
  for (auto& block : function->blocks) {
    analysis.blocks_by_label.emplace(block.label, &block);
    for (auto& inst : block.insts) {
      if (const auto result_name = inst_result_name(inst); result_name.has_value()) {
        analysis.defs_by_name.emplace(*result_name, &inst);
        analysis.def_blocks_by_name.emplace(*result_name, &block);
      }
    }
  }
  return analysis;
}

std::optional<std::vector<std::string>> collect_funnel_leaf_labels(
    const std::unordered_map<std::string, bir::Block*>& blocks_by_label,
    std::string_view start_label,
    std::string_view join_label,
    std::unordered_set<std::string>* visiting) {
  const auto it = blocks_by_label.find(std::string(start_label));
  if (it == blocks_by_label.end() || !visiting->emplace(std::string(start_label)).second) {
    return std::nullopt;
  }

  const auto clear_visit = [&] { visiting->erase(std::string(start_label)); };
  const auto* block = it->second;
  if (block->terminator.kind == bir::TerminatorKind::Branch) {
    if (block->terminator.target_label == join_label) {
      clear_visit();
      return std::vector<std::string>{block->label};
    }
    auto next = collect_funnel_leaf_labels(
        blocks_by_label, block->terminator.target_label, join_label, visiting);
    clear_visit();
    return next;
  }
  if (block->terminator.kind == bir::TerminatorKind::CondBranch) {
    auto true_leaves = collect_funnel_leaf_labels(
        blocks_by_label, block->terminator.true_label, join_label, visiting);
    auto false_leaves = collect_funnel_leaf_labels(
        blocks_by_label, block->terminator.false_label, join_label, visiting);
    clear_visit();
    if (!true_leaves.has_value() || !false_leaves.has_value()) {
      return std::nullopt;
    }
    true_leaves->insert(
        true_leaves->end(), false_leaves->begin(), false_leaves->end());
    return true_leaves;
  }

  clear_visit();
  return std::nullopt;
}

const bir::BinaryInst* find_compare_for_condition(const bir::Block& block,
                                                  const bir::Value& condition) {
  if (condition.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  for (const auto& inst : block.insts) {
    const auto* binary = std::get_if<bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->result.name != condition.name ||
        !bir::is_compare_opcode(binary->opcode)) {
      continue;
    }
    return binary;
  }
  return nullptr;
}

struct PhiMaterializeContext {
  bir::Function* function = nullptr;
  const BlockAnalysis* analysis = nullptr;
  bir::Block* target_block = nullptr;
  std::vector<bir::Inst> emitted_insts;
  std::unordered_set<std::string> removed_names;
  std::unordered_set<std::string> materialized_names;
  std::size_t next_temp_index = 0;
};

PreparedJoinTransfer make_join_transfer(PreparedNameTables& names,
                                        FunctionNameId function_name,
                                        BlockLabelId join_block_label,
                                        const bir::PhiInst& phi,
                                        PreparedJoinTransferKind kind,
                                        PreparedJoinTransferCarrierKind carrier_kind =
                                            PreparedJoinTransferCarrierKind::None,
                                        std::optional<SlotNameId> storage_name = std::nullopt) {
  std::vector<PreparedEdgeValueTransfer> edge_transfers;
  edge_transfers.reserve(phi.incomings.size());
  for (const auto& incoming : phi.incomings) {
    edge_transfers.push_back(PreparedEdgeValueTransfer{
        .predecessor_label = names.block_labels.intern(incoming.label),
        .successor_label = join_block_label,
        .incoming_value = incoming.value,
        .destination_value = phi.result,
        .storage_name = storage_name,
    });
  }
  return PreparedJoinTransfer{
      .function_name = function_name,
      .join_block_label = join_block_label,
      .result = phi.result,
      .kind = kind,
      .carrier_kind = carrier_kind,
      .storage_name = std::move(storage_name),
      .incomings = phi.incomings,
      .edge_transfers = std::move(edge_transfers),
  };
}

std::optional<bir::Value> materialize_value(const bir::Value& value, PhiMaterializeContext* context);

std::string make_materialized_select_name(std::string_view result_name,
                                          PhiMaterializeContext* context) {
  while (true) {
    auto candidate = std::string(result_name) + ".phi.sel" +
                     std::to_string(context->next_temp_index++);
    if (context->analysis->defs_by_name.find(candidate) == context->analysis->defs_by_name.end() &&
        context->materialized_names.find(candidate) == context->materialized_names.end()) {
      return candidate;
    }
  }
}

std::optional<bir::Value> materialize_funnel_subtree(
    const bir::PhiInst& phi,
    std::string_view start_label,
    std::string_view join_label,
    const std::unordered_map<std::string, bir::Value>& incoming_values,
    const std::optional<bir::Value>& desired_result,
    std::unordered_set<std::string>* visiting,
    PhiMaterializeContext* context) {
  const auto block_it = context->analysis->blocks_by_label.find(std::string(start_label));
  if (block_it == context->analysis->blocks_by_label.end() ||
      !visiting->emplace(std::string(start_label)).second) {
    return std::nullopt;
  }

  const auto clear_visit = [&] { visiting->erase(std::string(start_label)); };
  const auto* block = block_it->second;
  if (block->terminator.kind == bir::TerminatorKind::Branch) {
    if (block->terminator.target_label == join_label) {
      const auto incoming_it = incoming_values.find(block->label);
      clear_visit();
      if (incoming_it == incoming_values.end()) {
        return std::nullopt;
      }
      return materialize_value(incoming_it->second, context);
    }
    const auto forwarded = materialize_funnel_subtree(
        phi,
        block->terminator.target_label,
        join_label,
        incoming_values,
        desired_result,
        visiting,
        context);
    clear_visit();
    return forwarded;
  }

  if (block->terminator.kind != bir::TerminatorKind::CondBranch) {
    clear_visit();
    return std::nullopt;
  }

  const auto* compare = find_compare_for_condition(*block, block->terminator.condition);
  if (compare == nullptr) {
    clear_visit();
    return std::nullopt;
  }

  const auto lowered_true = materialize_funnel_subtree(
      phi,
      block->terminator.true_label,
      join_label,
      incoming_values,
      std::nullopt,
      visiting,
      context);
  const auto lowered_false = materialize_funnel_subtree(
      phi,
      block->terminator.false_label,
      join_label,
      incoming_values,
      std::nullopt,
      visiting,
      context);
  clear_visit();
  if (!lowered_true.has_value() || !lowered_false.has_value()) {
    return std::nullopt;
  }

  bir::Value result = desired_result.value_or(
      bir::Value::named(phi.result.type, make_materialized_select_name(phi.result.name, context)));
  context->emitted_insts.push_back(bir::SelectInst{
      .predicate = compare->opcode,
      .result = result,
      .compare_type = compare->operand_type,
      .lhs = compare->lhs,
      .rhs = compare->rhs,
      .true_value = *lowered_true,
      .false_value = *lowered_false,
  });
  context->materialized_names.insert(result.name);
  return result;
}

std::optional<bir::Value> materialize_phi(const bir::PhiInst& phi,
                                          const bir::Block& phi_block,
                                          PhiMaterializeContext* context) {
  if (phi.incomings.size() < 2) {
    return std::nullopt;
  }

  std::unordered_map<std::string, bir::Value> incoming_values;
  incoming_values.reserve(phi.incomings.size());
  for (const auto& incoming : phi.incomings) {
    if (!incoming_values.emplace(incoming.label, incoming.value).second) {
      return std::nullopt;
    }
  }

  for (const auto& [_, block] : context->analysis->blocks_by_label) {
    if (block->terminator.kind != bir::TerminatorKind::CondBranch) {
      continue;
    }

    std::unordered_set<std::string> visiting;
    const auto leaves = collect_funnel_leaf_labels(
        context->analysis->blocks_by_label, block->label, phi_block.label, &visiting);
    if (!leaves.has_value()) {
      continue;
    }

    std::unordered_set<std::string> leaf_labels;
    leaf_labels.insert(leaves->begin(), leaves->end());
    if (leaf_labels.size() != incoming_values.size()) {
      continue;
    }

    bool covers_all_incomings = true;
    for (const auto& [label, _] : incoming_values) {
      if (leaf_labels.find(label) == leaf_labels.end()) {
        covers_all_incomings = false;
        break;
      }
    }
    if (!covers_all_incomings || find_compare_for_condition(*block, block->terminator.condition) == nullptr) {
      continue;
    }

    visiting.clear();
    const auto materialized = materialize_funnel_subtree(
        phi,
        block->label,
        phi_block.label,
        incoming_values,
        phi.result,
        &visiting,
        context);
    if (materialized.has_value()) {
      context->removed_names.insert(phi.result.name);
      context->materialized_names.insert(phi.result.name);
      return materialized;
    }
  }

  return std::nullopt;
}

std::optional<bir::Value> materialize_value(const bir::Value& value, PhiMaterializeContext* context) {
  if (value.kind != bir::Value::Kind::Named ||
      context->materialized_names.find(value.name) != context->materialized_names.end()) {
    return value;
  }

  const auto def_it = context->analysis->defs_by_name.find(value.name);
  if (def_it == context->analysis->defs_by_name.end()) {
    return value;
  }

  const auto* inst = def_it->second;
  if (!is_materializable_inst(*inst)) {
    return value;
  }

  if (const auto* binary = std::get_if<bir::BinaryInst>(inst)) {
    auto lowered = *binary;
    const auto lhs = materialize_value(lowered.lhs, context);
    const auto rhs = materialize_value(lowered.rhs, context);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }
    lowered.lhs = *lhs;
    lowered.rhs = *rhs;
    context->emitted_insts.push_back(std::move(lowered));
    context->removed_names.insert(binary->result.name);
    context->materialized_names.insert(binary->result.name);
    return binary->result;
  }

  if (const auto* cast = std::get_if<bir::CastInst>(inst)) {
    auto lowered = *cast;
    const auto operand = materialize_value(lowered.operand, context);
    if (!operand.has_value()) {
      return std::nullopt;
    }
    lowered.operand = *operand;
    context->emitted_insts.push_back(std::move(lowered));
    context->removed_names.insert(cast->result.name);
    context->materialized_names.insert(cast->result.name);
    return cast->result;
  }

  if (const auto* phi = std::get_if<bir::PhiInst>(inst)) {
    const auto block_it = context->analysis->def_blocks_by_name.find(phi->result.name);
    if (block_it == context->analysis->def_blocks_by_name.end()) {
      return std::nullopt;
    }
    return materialize_phi(*phi, *block_it->second, context);
  }

  return value;
}

bool try_materialize_root_phi_block(bir::Function* function,
                                    PreparedNameTables& names,
                                    const BlockAnalysis& analysis,
                                    bir::Block* block,
                                    std::vector<PreparedJoinTransfer>* join_transfers) {
  bool saw_phi = false;
  bool has_non_phi_after_top = false;
  std::unordered_set<std::string> phi_names;
  for (const auto& inst : block->insts) {
    if (const auto* phi = std::get_if<bir::PhiInst>(&inst); phi != nullptr && !has_non_phi_after_top) {
      saw_phi = true;
      phi_names.insert(phi->result.name);
      continue;
    }
    if (saw_phi) {
      has_non_phi_after_top = true;
      break;
    }
    break;
  }
  const bool terminator_uses_phi = terminator_uses_named_value(block->terminator, phi_names);
  bool successor_tree_uses_phi = false;
  if (!has_non_phi_after_top && !terminator_uses_phi) {
    if (block->terminator.kind == bir::TerminatorKind::Branch) {
      successor_tree_uses_phi = successor_tree_uses_named_value(
          analysis.blocks_by_label, block->terminator.target_label, phi_names);
    } else if (block->terminator.kind == bir::TerminatorKind::CondBranch) {
      successor_tree_uses_phi =
          successor_tree_uses_named_value(
              analysis.blocks_by_label, block->terminator.true_label, phi_names) ||
          successor_tree_uses_named_value(
              analysis.blocks_by_label, block->terminator.false_label, phi_names);
    }
  }
  if (!saw_phi || (!has_non_phi_after_top && !terminator_uses_phi && !successor_tree_uses_phi)) {
    return false;
  }

  PhiMaterializeContext context{
      .function = function,
      .analysis = &analysis,
      .target_block = block,
  };
  std::vector<PreparedJoinTransfer> block_join_transfers;

  for (const auto& inst : block->insts) {
    const auto* phi = std::get_if<bir::PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    if (!materialize_phi(*phi, *block, &context).has_value()) {
      return false;
    }
    if (join_transfers != nullptr) {
      const FunctionNameId function_name_id = names.function_names.intern(function->name);
      const BlockLabelId block_label_id = names.block_labels.intern(block->label);
      block_join_transfers.push_back(make_join_transfer(
          names,
          function_name_id,
          block_label_id,
          *phi,
          PreparedJoinTransferKind::PhiEdge,
          PreparedJoinTransferCarrierKind::SelectMaterialization));
    }
  }

  if (context.emitted_insts.empty()) {
    return false;
  }

  for (auto& rewrite_block : function->blocks) {
    std::vector<bir::Inst> rewritten;
    if (rewrite_block.label == block->label) {
      rewritten = context.emitted_insts;
    }
    for (auto& inst : rewrite_block.insts) {
      const auto result_name = inst_result_name(inst);
      if (result_name.has_value() &&
          context.removed_names.find(*result_name) != context.removed_names.end()) {
        continue;
      }
      rewritten.push_back(std::move(inst));
    }
    rewrite_block.insts = std::move(rewritten);
  }

  if (join_transfers != nullptr) {
    join_transfers->insert(join_transfers->end(),
                           std::make_move_iterator(block_join_transfers.begin()),
                           std::make_move_iterator(block_join_transfers.end()));
  }

  return true;
}

void materialize_reducible_phi_trees(bir::Function* function,
                                     PreparedNameTables& names,
                                     std::vector<PreparedJoinTransfer>* join_transfers) {
  while (true) {
    const auto analysis = analyze_function(function);
    bir::Block* root_block = nullptr;
    for (auto& block : function->blocks) {
      if (!block.insts.empty() && std::holds_alternative<bir::PhiInst>(block.insts.front())) {
        root_block = &block;
      }
    }
    if (root_block == nullptr ||
        !try_materialize_root_phi_block(function, names, analysis, root_block, join_transfers)) {
      return;
    }
  }
}

void lower_phi_nodes(bir::Function* function,
                     PreparedNameTables& names,
                     std::vector<PreparedJoinTransfer>* join_transfers) {
  struct PhiLoweringPlan {
    std::string slot_name;
    std::vector<bir::PhiIncoming> incomings;
  };

  materialize_reducible_phi_trees(function, names, join_transfers);

  std::unordered_map<std::string, std::vector<PhiLoweringPlan>> phis_by_block;
  std::unordered_set<std::string> used_slot_names;
  for (const auto& slot : function->local_slots) {
    used_slot_names.insert(slot.name);
  }

  for (auto& block : function->blocks) {
    std::vector<bir::Inst> rewritten;
    rewritten.reserve(block.insts.size());

    bool saw_non_phi = false;
    for (auto& inst : block.insts) {
      if (const auto* phi = std::get_if<bir::PhiInst>(&inst)) {
        if (saw_non_phi) {
          rewritten.push_back(inst);
          continue;
        }

        auto slot_name = make_phi_slot_name(phi->result.name);
        if (used_slot_names.emplace(slot_name).second) {
          const auto slot_size = type_size_bytes(phi->result.type);
          function->local_slots.push_back(bir::LocalSlot{
              .name = slot_name,
              .type = phi->result.type,
              .size_bytes = slot_size,
              .align_bytes = slot_size,
              .phi_observation = bir::PhiObservation{
                  .result = phi->result,
                  .incomings = phi->incomings,
              },
          });
        } else if (auto* slot = find_local_slot(function, slot_name); slot != nullptr &&
                   !slot->phi_observation.has_value()) {
          slot->phi_observation = bir::PhiObservation{
              .result = phi->result,
              .incomings = phi->incomings,
          };
        }

        phis_by_block[block.label].push_back(PhiLoweringPlan{
            .slot_name = slot_name,
            .incomings = phi->incomings,
        });
        if (join_transfers != nullptr) {
          const auto transfer_kind =
              classify_phi_join_transfer_kind(*function, block.label, *phi);
          join_transfers->push_back(make_join_transfer(
              names,
              names.function_names.intern(function->name),
              names.block_labels.intern(block.label),
              *phi,
              transfer_kind,
              PreparedJoinTransferCarrierKind::EdgeStoreSlot,
              names.slot_names.intern(slot_name)));
        }
        rewritten.push_back(bir::LoadLocalInst{
            .result = phi->result,
            .slot_name = std::move(slot_name),
        });
        continue;
      }

      saw_non_phi = true;
      rewritten.push_back(std::move(inst));
    }

    block.insts = std::move(rewritten);
  }

  if (phis_by_block.empty()) {
    return;
  }

  std::unordered_map<std::string, bir::Block*> blocks_by_label;
  for (auto& block : function->blocks) {
    blocks_by_label.emplace(block.label, &block);
  }

  for (const auto& [block_label, plans] : phis_by_block) {
    (void)block_label;
    for (const auto& plan : plans) {
      for (const auto& incoming : plan.incomings) {
        const auto pred_it = blocks_by_label.find(incoming.label);
        if (pred_it == blocks_by_label.end()) {
          continue;
        }
        pred_it->second->insts.push_back(bir::StoreLocalInst{
            .slot_name = plan.slot_name,
            .value = incoming.value,
        });
      }
    }
  }
}

PreparedBranchCondition make_branch_condition(PreparedNameTables& names,
                                              FunctionNameId function_name,
                                              const bir::Block& block) {
  PreparedBranchCondition condition{
      .function_name = function_name,
      .block_label = names.block_labels.intern(block.label),
      .condition_value = block.terminator.condition,
      .true_label = names.block_labels.intern(block.terminator.true_label),
      .false_label = names.block_labels.intern(block.terminator.false_label),
  };
  if (const auto* compare = find_compare_for_condition(block, block.terminator.condition);
      compare != nullptr) {
    condition.kind = PreparedBranchConditionKind::FusedCompare;
    condition.predicate = compare->opcode;
    condition.compare_type = compare->operand_type;
    condition.lhs = compare->lhs;
    condition.rhs = compare->rhs;
    condition.can_fuse_with_branch = true;
  }
  return condition;
}

const bir::BinaryInst* find_trailing_branch_compare(const bir::Block& block) {
  if (block.terminator.kind != bir::TerminatorKind::Branch || block.insts.empty()) {
    return nullptr;
  }

  const auto* compare = std::get_if<bir::BinaryInst>(&block.insts.back());
  if (compare == nullptr || !bir::is_compare_opcode(compare->opcode) ||
      (compare->result.type != bir::TypeKind::I1 &&
       compare->result.type != bir::TypeKind::I32)) {
    return nullptr;
  }
  return compare;
}

std::optional<std::string> find_linear_join_predecessor(
    const std::unordered_map<std::string, const bir::Block*>& blocks_by_label,
    std::string_view start_label,
    std::string_view join_label) {
  std::unordered_set<std::string_view> visited;
  auto current_it = blocks_by_label.find(std::string(start_label));
  if (current_it == blocks_by_label.end()) {
    return std::nullopt;
  }

  const auto* current = current_it->second;
  while (current != nullptr && visited.insert(current->label).second) {
    if (current->terminator.kind != bir::TerminatorKind::Branch) {
      return std::nullopt;
    }
    if (current->terminator.target_label == join_label) {
      return current->label;
    }
    current_it = blocks_by_label.find(current->terminator.target_label);
    if (current_it == blocks_by_label.end()) {
      return std::nullopt;
    }
    current = current_it->second;
  }

  return std::nullopt;
}

std::optional<std::size_t> find_join_transfer_edge_index_by_predecessor(
    const PreparedNameTables& names,
    const PreparedJoinTransfer& transfer,
    std::string_view predecessor_label) {
  const BlockLabelId predecessor_label_id = names.block_labels.find(predecessor_label);
  if (predecessor_label_id == kInvalidBlockLabel) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < transfer.edge_transfers.size(); ++index) {
    if (transfer.edge_transfers[index].predecessor_label == predecessor_label_id) {
      return index;
    }
  }
  return std::nullopt;
}

std::optional<std::string_view> prepared_named_value_name(const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  return value.name;
}

PreparedParallelCopyBundle make_parallel_copy_bundle(
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    std::vector<PreparedParallelCopyMove> moves) {
  struct MoveState {
    std::optional<std::string_view> source_name;
    std::optional<std::string_view> destination_name;
    bool uses_cycle_temp_source = false;
    bool emitted = false;
  };

  PreparedParallelCopyBundle bundle{
      .predecessor_label = predecessor_label,
      .successor_label = successor_label,
      .moves = std::move(moves),
  };
  std::vector<MoveState> states;
  states.reserve(bundle.moves.size());
  for (const auto& move : bundle.moves) {
    auto source_name = prepared_named_value_name(move.source_value);
    auto destination_name = prepared_named_value_name(move.destination_value);
    if (source_name.has_value() && destination_name.has_value() &&
        *source_name == *destination_name) {
      source_name.reset();
    }
    states.push_back(MoveState{
        .source_name = source_name,
        .destination_name = destination_name,
    });
  }

  std::size_t remaining_moves = states.size();
  while (remaining_moves > 0) {
    std::unordered_set<std::string_view> pending_sources;
    pending_sources.reserve(remaining_moves);
    for (const auto& state : states) {
      if (!state.emitted && state.source_name.has_value()) {
        pending_sources.insert(*state.source_name);
      }
    }

    bool progressed = false;
    for (std::size_t move_index = 0; move_index < states.size(); ++move_index) {
      auto& state = states[move_index];
      if (state.emitted) {
        continue;
      }
      if (state.destination_name.has_value() &&
          pending_sources.find(*state.destination_name) != pending_sources.end()) {
        continue;
      }
      bundle.steps.push_back(PreparedParallelCopyStep{
          .kind = PreparedParallelCopyStepKind::Move,
          .move_index = move_index,
          .uses_cycle_temp_source = state.uses_cycle_temp_source,
      });
      state.emitted = true;
      --remaining_moves;
      progressed = true;
    }
    if (progressed) {
      continue;
    }

    const auto cycle_it = std::find_if(states.begin(), states.end(), [](const MoveState& state) {
      return !state.emitted;
    });
    if (cycle_it == states.end()) {
      break;
    }
    const std::size_t cycle_move_index = static_cast<std::size_t>(cycle_it - states.begin());
    if (!states[cycle_move_index].destination_name.has_value()) {
      bundle.steps.push_back(PreparedParallelCopyStep{
          .kind = PreparedParallelCopyStepKind::Move,
          .move_index = cycle_move_index,
          .uses_cycle_temp_source = states[cycle_move_index].uses_cycle_temp_source,
      });
      states[cycle_move_index].emitted = true;
      --remaining_moves;
      continue;
    }

    bundle.has_cycle = true;
    bundle.steps.push_back(PreparedParallelCopyStep{
        .kind = PreparedParallelCopyStepKind::SaveDestinationToTemp,
        .move_index = cycle_move_index,
    });
    const std::string_view broken_destination = *states[cycle_move_index].destination_name;
    for (auto& state : states) {
      if (!state.emitted && state.source_name.has_value() &&
          *state.source_name == broken_destination) {
        state.source_name.reset();
        state.uses_cycle_temp_source = true;
      }
    }
  }

  return bundle;
}

void build_parallel_copy_bundles(const PreparedNameTables& names,
                                 PreparedControlFlowFunction* function_control_flow) {
  if (function_control_flow == nullptr) {
    return;
  }

  struct EdgeKey {
    BlockLabelId predecessor_label = kInvalidBlockLabel;
    BlockLabelId successor_label = kInvalidBlockLabel;

    bool operator==(const EdgeKey& other) const {
      return predecessor_label == other.predecessor_label &&
             successor_label == other.successor_label;
    }
  };

  struct EdgeKeyHash {
    std::size_t operator()(const EdgeKey& key) const {
      return (static_cast<std::size_t>(key.predecessor_label) << 1u) ^
             static_cast<std::size_t>(key.successor_label);
    }
  };

  std::unordered_map<EdgeKey, std::vector<PreparedParallelCopyMove>, EdgeKeyHash> moves_by_edge;
  for (std::size_t join_transfer_index = 0;
       join_transfer_index < function_control_flow->join_transfers.size();
       ++join_transfer_index) {
    const auto& join_transfer = function_control_flow->join_transfers[join_transfer_index];
    const auto carrier_kind = effective_prepared_join_transfer_carrier_kind(join_transfer);
    for (std::size_t edge_transfer_index = 0;
         edge_transfer_index < join_transfer.edge_transfers.size();
         ++edge_transfer_index) {
      const auto& edge_transfer = join_transfer.edge_transfers[edge_transfer_index];
      moves_by_edge[EdgeKey{
          .predecessor_label = edge_transfer.predecessor_label,
          .successor_label = edge_transfer.successor_label,
      }]
          .push_back(PreparedParallelCopyMove{
              .join_transfer_index = join_transfer_index,
              .edge_transfer_index = edge_transfer_index,
              .source_value = edge_transfer.incoming_value,
              .destination_value = edge_transfer.destination_value,
              .carrier_kind = carrier_kind,
              .storage_name = edge_transfer.storage_name,
          });
    }
  }

  function_control_flow->parallel_copy_bundles.clear();
  function_control_flow->parallel_copy_bundles.reserve(moves_by_edge.size());
  for (auto& [edge, moves] : moves_by_edge) {
    function_control_flow->parallel_copy_bundles.push_back(
        make_parallel_copy_bundle(edge.predecessor_label, edge.successor_label, std::move(moves)));
  }
  std::sort(function_control_flow->parallel_copy_bundles.begin(),
            function_control_flow->parallel_copy_bundles.end(),
            [](const PreparedParallelCopyBundle& lhs, const PreparedParallelCopyBundle& rhs) {
              if (lhs.successor_label != rhs.successor_label) {
                return lhs.successor_label < rhs.successor_label;
              }
              return lhs.predecessor_label < rhs.predecessor_label;
            });
}

void annotate_branch_owned_join_transfers(
    PreparedNameTables& names,
    const bir::Function& function,
    const std::vector<PreparedBranchCondition>& branch_conditions,
    std::vector<PreparedJoinTransfer>* join_transfers) {
  if (join_transfers == nullptr || branch_conditions.empty()) {
    return;
  }

  std::unordered_map<std::string, const bir::Block*> blocks_by_label;
  for (const auto& block : function.blocks) {
    blocks_by_label.emplace(block.label, &block);
  }

  for (auto& transfer : *join_transfers) {
    if (transfer.edge_transfers.size() != 2 || transfer.source_true_transfer_index.has_value() ||
        transfer.source_false_transfer_index.has_value()) {
      continue;
    }

    struct CandidateMapping {
      BlockLabelId source_branch_block_label = kInvalidBlockLabel;
      std::size_t true_transfer_index = 0;
      std::size_t false_transfer_index = 0;
      BlockLabelId true_incoming_label = kInvalidBlockLabel;
      BlockLabelId false_incoming_label = kInvalidBlockLabel;
    };

    std::optional<CandidateMapping> candidate;
    bool ambiguous = false;
    for (const auto& branch_condition : branch_conditions) {
      const auto true_incoming_label = find_linear_join_predecessor(
          blocks_by_label,
          prepared_block_label(names, branch_condition.true_label),
          prepared_block_label(names, transfer.join_block_label));
      const auto false_incoming_label = find_linear_join_predecessor(
          blocks_by_label,
          prepared_block_label(names, branch_condition.false_label),
          prepared_block_label(names, transfer.join_block_label));
      if (!true_incoming_label.has_value() || !false_incoming_label.has_value() ||
          *true_incoming_label == *false_incoming_label) {
        continue;
      }

      const auto true_transfer_index =
          find_join_transfer_edge_index_by_predecessor(names, transfer, *true_incoming_label);
      const auto false_transfer_index =
          find_join_transfer_edge_index_by_predecessor(names, transfer, *false_incoming_label);
      if (!true_transfer_index.has_value() || !false_transfer_index.has_value() ||
          *true_transfer_index == *false_transfer_index) {
        continue;
      }

      if (candidate.has_value()) {
        ambiguous = true;
        break;
      }

      candidate = CandidateMapping{
          .source_branch_block_label = branch_condition.block_label,
          .true_transfer_index = *true_transfer_index,
          .false_transfer_index = *false_transfer_index,
          .true_incoming_label = names.block_labels.intern(*true_incoming_label),
          .false_incoming_label = names.block_labels.intern(*false_incoming_label),
      };
    }

    if (!ambiguous && candidate.has_value()) {
      transfer.source_branch_block_label = std::move(candidate->source_branch_block_label);
      transfer.source_true_transfer_index = candidate->true_transfer_index;
      transfer.source_false_transfer_index = candidate->false_transfer_index;
      transfer.source_true_incoming_label = std::move(candidate->true_incoming_label);
      transfer.source_false_incoming_label = std::move(candidate->false_incoming_label);
    }
  }
}

void publish_short_circuit_continuation_branch_conditions(
    PreparedNameTables& names,
    const bir::Function& function,
    PreparedControlFlowFunction* function_control_flow) {
  if (function_control_flow == nullptr ||
      function_control_flow->branch_conditions.empty() ||
      function_control_flow->join_transfers.empty()) {
    return;
  }

  const std::size_t original_branch_condition_count =
      function_control_flow->branch_conditions.size();
  for (std::size_t index = 0; index < original_branch_condition_count; ++index) {
    const auto& entry_branch_condition = function_control_flow->branch_conditions[index];
    const auto* entry_block =
        find_block(function, prepared_block_label(names, entry_branch_condition.block_label));
    if (entry_block == nullptr) {
      continue;
    }

    const auto prepared_entry_targets =
        find_prepared_compare_branch_target_labels(names, entry_branch_condition, *entry_block);
    if (!prepared_entry_targets.has_value()) {
      continue;
    }

    const auto short_circuit_join_context =
        find_prepared_short_circuit_join_context(
            names,
            *function_control_flow,
            function,
            entry_branch_condition.block_label);
    if (!short_circuit_join_context.has_value()) {
      continue;
    }

    const auto prepared_branch_plan =
        find_prepared_short_circuit_branch_plan(names,
                                                *short_circuit_join_context,
                                                *prepared_entry_targets);
    if (!prepared_branch_plan.has_value()) {
      continue;
    }

    for (const auto* target :
         {&prepared_branch_plan->on_compare_true, &prepared_branch_plan->on_compare_false}) {
      if (target == nullptr || !target->continuation.has_value()) {
        continue;
      }

      if (find_prepared_branch_condition(*function_control_flow, target->block_label) !=
          nullptr) {
        continue;
      }

      const auto* continuation_block =
          find_block(function, prepared_block_label(names, target->block_label));
      if (continuation_block == nullptr) {
        continue;
      }

      const auto* continuation_compare = find_trailing_branch_compare(*continuation_block);
      if (continuation_compare == nullptr) {
        continue;
      }

      function_control_flow->branch_conditions.push_back(PreparedBranchCondition{
          .function_name = function_control_flow->function_name,
          .block_label = names.block_labels.intern(continuation_block->label),
          .kind = PreparedBranchConditionKind::FusedCompare,
          .condition_value = continuation_compare->result,
          .predicate = continuation_compare->opcode,
          .compare_type = continuation_compare->operand_type,
          .lhs = continuation_compare->lhs,
          .rhs = continuation_compare->rhs,
          .can_fuse_with_branch = true,
          .true_label = target->continuation->true_label,
          .false_label = target->continuation->false_label,
      });
    }
  }
}

void collect_select_materialized_join_transfers(
    PreparedNameTables& names,
    const bir::Function& function,
    const std::vector<PreparedBranchCondition>& branch_conditions,
    std::vector<PreparedJoinTransfer>* join_transfers) {
  if (join_transfers == nullptr) {
    return;
  }

  std::unordered_map<std::string, const bir::Block*> blocks_by_label;
  for (const auto& block : function.blocks) {
    blocks_by_label.emplace(block.label, &block);
  }

  std::unordered_set<std::string_view> existing_join_blocks;
  for (const auto& transfer : *join_transfers) {
    existing_join_blocks.insert(prepared_block_label(names, transfer.join_block_label));
  }

  for (const auto& block : function.blocks) {
    if (block.insts.empty() || existing_join_blocks.find(block.label) != existing_join_blocks.end()) {
      continue;
    }

    const auto* select = std::get_if<bir::SelectInst>(&block.insts.front());
    if (select == nullptr) {
      continue;
    }

    for (const auto& branch_condition : branch_conditions) {
      if (!branch_condition.predicate.has_value() || !branch_condition.compare_type.has_value() ||
          !branch_condition.lhs.has_value() || !branch_condition.rhs.has_value() ||
          *branch_condition.predicate != select->predicate ||
          *branch_condition.compare_type != select->compare_type ||
          *branch_condition.lhs != select->lhs || *branch_condition.rhs != select->rhs) {
        continue;
      }

      const auto true_incoming_label =
          find_linear_join_predecessor(
              blocks_by_label, prepared_block_label(names, branch_condition.true_label), block.label);
      const auto false_incoming_label =
          find_linear_join_predecessor(
              blocks_by_label, prepared_block_label(names, branch_condition.false_label), block.label);
      if (!true_incoming_label.has_value() || !false_incoming_label.has_value()) {
        continue;
      }

      join_transfers->push_back(PreparedJoinTransfer{
          .function_name = names.function_names.intern(function.name),
          .join_block_label = names.block_labels.intern(block.label),
          .result = select->result,
          .kind = PreparedJoinTransferKind::PhiEdge,
          .carrier_kind = PreparedJoinTransferCarrierKind::SelectMaterialization,
          .storage_name = std::nullopt,
          .incomings =
              {
                  bir::PhiIncoming{
                      .label = *true_incoming_label,
                      .value = select->true_value,
                  },
                  bir::PhiIncoming{
                      .label = *false_incoming_label,
                      .value = select->false_value,
                  },
              },
          .edge_transfers =
              {
                  PreparedEdgeValueTransfer{
                      .predecessor_label = names.block_labels.intern(*true_incoming_label),
                      .successor_label = names.block_labels.intern(block.label),
                      .incoming_value = select->true_value,
                      .destination_value = select->result,
                      .storage_name = std::nullopt,
                  },
                  PreparedEdgeValueTransfer{
                      .predecessor_label = names.block_labels.intern(*false_incoming_label),
                      .successor_label = names.block_labels.intern(block.label),
                      .incoming_value = select->false_value,
                      .destination_value = select->result,
                      .storage_name = std::nullopt,
                  },
              },
          .source_branch_block_label = branch_condition.block_label,
          .source_true_transfer_index = 0,
          .source_false_transfer_index = 1,
          .source_true_incoming_label = names.block_labels.intern(*true_incoming_label),
          .source_false_incoming_label = names.block_labels.intern(*false_incoming_label),
      });
      existing_join_blocks.insert(prepared_block_label(names, join_transfers->back().join_block_label));
      break;
    }
  }
}

void legalize_module(const c4c::TargetProfile& target_profile,
                     bir::Module& module,
                     PreparedNameTables& names,
                     PreparedControlFlow* control_flow) {
  for (auto& global : module.globals) {
    legalize_sized_type(target_profile, global.type, global.size_bytes, global.align_bytes);
    if (global.initializer.has_value()) {
      legalize_value(target_profile, *global.initializer);
    }
    for (auto& element : global.initializer_elements) {
      legalize_value(target_profile, element);
    }
  }

  for (auto& function : module.functions) {
    const FunctionNameId function_name_id = names.function_names.intern(function.name);
    PreparedControlFlowFunction function_control_flow{
        .function_name = function_name_id,
    };
    lower_phi_nodes(&function, names, &function_control_flow.join_transfers);
    legalize_sized_type(
        target_profile, function.return_type, function.return_size_bytes, function.return_align_bytes);
    if (!function.return_abi.has_value()) {
      function.return_abi = infer_function_return_abi(target_profile, function);
    }
    if (function.return_abi.has_value()) {
      legalize_call_result_abi(target_profile, *function.return_abi);
      if (function.return_abi->type == bir::TypeKind::Void &&
          !function.return_abi->returned_in_memory &&
          function.return_abi->primary_class != bir::AbiValueClass::Memory) {
        function.return_abi.reset();
      }
    }
    for (auto& param : function.params) {
      legalize_sized_type(target_profile, param.type, param.size_bytes, param.align_bytes);
      if (!param.abi.has_value()) {
        param.abi = infer_call_arg_abi(target_profile, param.type);
        if (param.abi.has_value()) {
          if (param.is_sret) {
            param.abi->sret_pointer = true;
          }
          if (param.is_byval) {
            param.abi->byval_copy = true;
            param.abi->size_bytes = param.size_bytes;
            param.abi->align_bytes = param.align_bytes;
          }
        }
      }
      if (param.abi.has_value()) {
        legalize_call_arg_abi(target_profile, *param.abi);
      }
    }
    for (auto& slot : function.local_slots) {
      legalize_sized_type(target_profile, slot.type, slot.size_bytes, slot.align_bytes);
    }
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        std::visit(
            [&](auto& lowered) {
              using T = std::decay_t<decltype(lowered)>;
              if constexpr (std::is_same_v<T, bir::BinaryInst>) {
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                lowered.operand_type = legalize_type(target_profile, lowered.operand_type);
                legalize_value(target_profile, lowered.lhs);
                legalize_value(target_profile, lowered.rhs);
              } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                lowered.compare_type = legalize_type(target_profile, lowered.compare_type);
                legalize_value(target_profile, lowered.lhs);
                legalize_value(target_profile, lowered.rhs);
                legalize_value(target_profile, lowered.true_value);
                legalize_value(target_profile, lowered.false_value);
              } else if constexpr (std::is_same_v<T, bir::CastInst>) {
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                legalize_value(target_profile, lowered.operand);
              } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                for (auto& incoming : lowered.incomings) {
                  legalize_value(target_profile, incoming.value);
                }
              } else if constexpr (std::is_same_v<T, bir::CallInst>) {
                lowered.return_type = legalize_type(target_profile, lowered.return_type);
                if (!lowered.return_type_name.empty()) {
                  lowered.return_type_name = bir::render_type(lowered.return_type);
                }
                if (lowered.result.has_value()) {
                  legalize_value(target_profile, *lowered.result);
                }
                if (lowered.callee_value.has_value()) {
                  legalize_value(target_profile, *lowered.callee_value);
                }
                for (auto& arg : lowered.args) {
                  legalize_value(target_profile, arg);
                }
                for (auto& arg_type : lowered.arg_types) {
                  arg_type = legalize_type(target_profile, arg_type);
                }
                for (auto& arg_abi : lowered.arg_abi) {
                  legalize_call_arg_abi(target_profile, arg_abi);
                }
                if (lowered.result_abi.has_value()) {
                  legalize_call_result_abi(target_profile, *lowered.result_abi);
                }
              } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                                   std::is_same_v<T, bir::LoadGlobalInst>) {
                const auto original_type = lowered.result.type;
                lowered.result.type = legalize_type(target_profile, lowered.result.type);
                legalize_memory_access_metadata(
                    target_profile, original_type, lowered.align_bytes, lowered.address);
                if (lowered.address.has_value()) {
                  legalize_value(target_profile, lowered.address->base_value);
                }
              } else if constexpr (std::is_same_v<T, bir::StoreLocalInst> ||
                                   std::is_same_v<T, bir::StoreGlobalInst>) {
                const auto original_type = lowered.value.type;
                legalize_value(target_profile, lowered.value);
                legalize_memory_access_metadata(
                    target_profile, original_type, lowered.align_bytes, lowered.address);
                if (lowered.address.has_value()) {
                  legalize_value(target_profile, lowered.address->base_value);
                }
              }
            },
            inst);
      }

      if (block.terminator.value.has_value()) {
        legalize_value(target_profile, *block.terminator.value);
      }
      legalize_value(target_profile, block.terminator.condition);
      PreparedControlFlowBlock prepared_block{
          .block_label = names.block_labels.intern(block.label),
          .terminator_kind = block.terminator.kind,
      };
      if (block.terminator.kind == bir::TerminatorKind::Branch) {
        prepared_block.branch_target_label =
            names.block_labels.intern(block.terminator.target_label);
      } else if (block.terminator.kind == bir::TerminatorKind::CondBranch) {
        prepared_block.true_label = names.block_labels.intern(block.terminator.true_label);
        prepared_block.false_label = names.block_labels.intern(block.terminator.false_label);
      }
      function_control_flow.blocks.push_back(std::move(prepared_block));
      if (block.terminator.kind == bir::TerminatorKind::CondBranch) {
        function_control_flow.branch_conditions.push_back(
            make_branch_condition(names, function_name_id, block));
      }
    }
    collect_select_materialized_join_transfers(
        names,
        function,
        function_control_flow.branch_conditions,
        &function_control_flow.join_transfers);
    annotate_branch_owned_join_transfers(
        names,
        function,
        function_control_flow.branch_conditions,
        &function_control_flow.join_transfers);
    build_parallel_copy_bundles(names, &function_control_flow);
    publish_short_circuit_continuation_branch_conditions(
        names, function, &function_control_flow);

    if (control_flow != nullptr &&
        (!function_control_flow.branch_conditions.empty() ||
         !function_control_flow.join_transfers.empty() ||
         !function_control_flow.parallel_copy_bundles.empty())) {
      control_flow->functions.push_back(std::move(function_control_flow));
    }
  }
}

}  // namespace

void BirPreAlloc::run_legalize() {
  prepared_.completed_phases.push_back("legalize");
  prepared_.control_flow.functions.clear();
  legalize_module(
      prepared_.target_profile, prepared_.module, prepared_.names, &prepared_.control_flow);
  prepared_.invariants.push_back(PreparedBirInvariant::NoPhiNodes);
  if (should_promote_i1(prepared_.target_profile)) {
    prepared_.invariants.push_back(PreparedBirInvariant::NoTargetFacingI1);
  }

  prepared_.notes.push_back(PrepareNote{
      .phase = "legalize",
      .message =
          "bootstrap BIR legalize removed phi nodes and promoted i1 values plus function "
          "signature/storage bookkeeping, memory-address/load-store bookkeeping, call ABI "
          "metadata, call return type text to i32 for x86/i686/aarch64/riscv64, and explicit "
          "prepared branch-condition/join-transfer/parallel-copy control-flow records",
  });
}

}  // namespace c4c::backend::prepare
