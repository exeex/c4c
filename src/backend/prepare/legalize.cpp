#include "legalize.hpp"

#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

bool should_promote_i1(Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
    case Target::Aarch64:
    case Target::Riscv64:
      return true;
  }
  return false;
}

bir::TypeKind legalize_type(Target target, bir::TypeKind type) {
  if (should_promote_i1(target) && type == bir::TypeKind::I1) {
    return bir::TypeKind::I32;
  }
  return type;
}

void legalize_value(Target target, bir::Value& value) {
  const auto original_type = value.type;
  value.type = legalize_type(target, value.type);
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
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 8;
    default:
      return 0;
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

bool try_materialize_root_phi_block(bir::Function* function, const BlockAnalysis& analysis, bir::Block* block) {
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

  for (const auto& inst : block->insts) {
    const auto* phi = std::get_if<bir::PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    if (!materialize_phi(*phi, *block, &context).has_value()) {
      return false;
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

  return true;
}

void materialize_reducible_phi_trees(bir::Function* function) {
  while (true) {
    const auto analysis = analyze_function(function);
    bir::Block* root_block = nullptr;
    for (auto& block : function->blocks) {
      if (!block.insts.empty() && std::holds_alternative<bir::PhiInst>(block.insts.front())) {
        root_block = &block;
      }
    }
    if (root_block == nullptr || !try_materialize_root_phi_block(function, analysis, root_block)) {
      return;
    }
  }
}

void lower_phi_nodes(bir::Function* function) {
  struct PhiLoweringPlan {
    std::string slot_name;
    std::vector<bir::PhiIncoming> incomings;
  };

  materialize_reducible_phi_trees(function);

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

void legalize_module(Target target, bir::Module& module) {
  for (auto& global : module.globals) {
    global.type = legalize_type(target, global.type);
    if (global.initializer.has_value()) {
      legalize_value(target, *global.initializer);
    }
    for (auto& element : global.initializer_elements) {
      legalize_value(target, element);
    }
  }

  for (auto& function : module.functions) {
    lower_phi_nodes(&function);
    function.return_type = legalize_type(target, function.return_type);
    for (auto& param : function.params) {
      param.type = legalize_type(target, param.type);
    }
    for (auto& slot : function.local_slots) {
      slot.type = legalize_type(target, slot.type);
    }
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        std::visit(
            [&](auto& lowered) {
              using T = std::decay_t<decltype(lowered)>;
              if constexpr (std::is_same_v<T, bir::BinaryInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                lowered.operand_type = legalize_type(target, lowered.operand_type);
                legalize_value(target, lowered.lhs);
                legalize_value(target, lowered.rhs);
              } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                lowered.compare_type = legalize_type(target, lowered.compare_type);
                legalize_value(target, lowered.lhs);
                legalize_value(target, lowered.rhs);
                legalize_value(target, lowered.true_value);
                legalize_value(target, lowered.false_value);
              } else if constexpr (std::is_same_v<T, bir::CastInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                legalize_value(target, lowered.operand);
              } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                for (auto& incoming : lowered.incomings) {
                  legalize_value(target, incoming.value);
                }
              } else if constexpr (std::is_same_v<T, bir::CallInst>) {
                lowered.return_type = legalize_type(target, lowered.return_type);
                if (lowered.result.has_value()) {
                  legalize_value(target, *lowered.result);
                }
                if (lowered.callee_value.has_value()) {
                  legalize_value(target, *lowered.callee_value);
                }
                for (auto& arg : lowered.args) {
                  legalize_value(target, arg);
                }
                for (auto& arg_type : lowered.arg_types) {
                  arg_type = legalize_type(target, arg_type);
                }
                if (lowered.result_abi.has_value()) {
                  lowered.result_abi->type = legalize_type(target, lowered.result_abi->type);
                }
              } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                                   std::is_same_v<T, bir::LoadGlobalInst>) {
                lowered.result.type = legalize_type(target, lowered.result.type);
                if (lowered.address.has_value()) {
                  legalize_value(target, lowered.address->base_value);
                }
              } else if constexpr (std::is_same_v<T, bir::StoreLocalInst> ||
                                   std::is_same_v<T, bir::StoreGlobalInst>) {
                legalize_value(target, lowered.value);
                if (lowered.address.has_value()) {
                  legalize_value(target, lowered.address->base_value);
                }
              }
            },
            inst);
      }

      if (block.terminator.value.has_value()) {
        legalize_value(target, *block.terminator.value);
      }
      legalize_value(target, block.terminator.condition);
    }
  }
}

}  // namespace

void run_legalize(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("legalize");
  module.notes.push_back(PrepareNote{
      .phase = "legalize",
      .message =
          "target-dependent legalization skeleton is wired; bool promotion and ABI/type legalization still need implementation",
  });
}

void run_legalize(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("legalize");
  legalize_module(module.target, module.module);
  module.invariants.push_back(PreparedBirInvariant::NoPhiNodes);
  if (should_promote_i1(module.target)) {
    module.invariants.push_back(PreparedBirInvariant::NoTargetFacingI1);
  }

  module.notes.push_back(PrepareNote{
      .phase = "legalize",
      .message =
          "bootstrap BIR legalize removed phi nodes and promoted i1 values to i32 for "
          "x86/i686/aarch64/riscv64",
  });
}

}  // namespace c4c::backend::prepare
