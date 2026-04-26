#include "lowering.hpp"

#include <optional>
#include <unordered_set>
#include <utility>

namespace c4c::backend {

namespace {

using lir_to_bir_detail::build_type_decl_map;
using lir_to_bir_detail::build_backend_structured_layout_table;
using lir_to_bir_detail::FunctionSymbolSet;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::GlobalTypes;
using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::lower_minimal_global;
using lir_to_bir_detail::lower_string_constant_global;
using lir_to_bir_detail::report_backend_structured_layout_parity_notes;
using lir_to_bir_detail::resolve_known_global_address;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::resolve_pointer_initializer_offsets;
using lir_to_bir_detail::TypeDeclMap;

constexpr std::string_view kModuleCapabilityBucketSummary =
    "currently admitted capability buckets covering function-signature, "
    "scalar-control-flow, scalar/local-memory (including scalar-cast/"
    "scalar-binop and alloca/gep/load/store local-memory), and local/global "
    "memory semantics, plus semantic call families (direct-call, "
    "indirect-call, and call-return) and explicit runtime or intrinsic "
    "families such as variadic, stack-state, absolute-value, memcpy, memset, "
    "and inline-asm placeholders";

int semantic_failure_note_rank(std::string_view message) {
  if (message.find("failed in runtime/intrinsic family") != std::string::npos ||
      message.find("failed in semantic call family") != std::string::npos) {
    return 3;
  }
  constexpr std::string_view kFailedInPrefix = " failed in ";
  const std::size_t failure_pos = message.find(kFailedInPrefix);
  if (failure_pos == std::string::npos) {
    return 0;
  }

  const std::string_view family = message.substr(failure_pos + kFailedInPrefix.size());
  if (family.find("semantic family") == std::string::npos) {
    return 0;
  }

  constexpr std::string_view kUmbrellaFamilies[] = {
      "function-signature semantic family",
      "local-memory semantic family",
      "scalar-control-flow semantic family",
      "scalar/local-memory semantic family",
  };
  for (std::string_view umbrella : kUmbrellaFamilies) {
    if (family == umbrella) {
      return 1;
    }
  }
  return 2;
}

std::optional<std::string_view> latest_function_failure_note(const BirLoweringContext& context) {
  for (int rank = 3; rank >= 1; --rank) {
    for (auto it = context.notes.rbegin(); it != context.notes.rend(); ++it) {
      if (it->phase == "function" && semantic_failure_note_rank(it->message) == rank) {
        return it->message;
      }
    }
  }
  for (auto it = context.notes.rbegin(); it != context.notes.rend(); ++it) {
    if (it->phase == "function") {
      return it->message;
    }
  }
  return std::nullopt;
}

std::string_view resolve_link_name(const c4c::LinkNameTable& link_names,
                                   c4c::LinkNameId id) {
  if (id == c4c::kInvalidLinkName) return {};
  const std::string_view spelling = link_names.spelling(id);
  return spelling.empty() ? std::string_view{} : spelling;
}

std::string function_name_for_reporting(const c4c::codegen::lir::LirModule& module,
                                        const c4c::codegen::lir::LirFunction& function) {
  const std::string_view resolved_name = resolve_link_name(module.link_names,
                                                           function.link_name_id);
  if (!resolved_name.empty()) {
    return std::string(resolved_name);
  }
  return function.name;
}

c4c::codegen::lir::LirFunction function_with_resolved_name(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirFunction& function) {
  c4c::codegen::lir::LirFunction resolved = function;
  const std::string_view resolved_name = resolve_link_name(module.link_names,
                                                           function.link_name_id);
  if (!resolved_name.empty()) {
    resolved.name = std::string(resolved_name);
  }
  return resolved;
}

c4c::codegen::lir::LirGlobal global_with_resolved_name(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirGlobal& global) {
  c4c::codegen::lir::LirGlobal resolved = global;
  const std::string_view resolved_name = resolve_link_name(module.link_names,
                                                           global.link_name_id);
  if (!resolved_name.empty()) {
    resolved.name = std::string(resolved_name);
  }
  return resolved;
}

c4c::codegen::lir::LirExternDecl extern_decl_with_resolved_name(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirExternDecl& decl) {
  c4c::codegen::lir::LirExternDecl resolved = decl;
  const std::string_view resolved_name = resolve_link_name(module.link_names,
                                                           decl.link_name_id);
  if (!resolved_name.empty()) {
    resolved.name = std::string(resolved_name);
  }
  return resolved;
}

void record_block_integer_constant_alias(
    const c4c::codegen::lir::LirInst& inst,
    BirFunctionLowerer::ValueMap* block_value_aliases) {
  if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
    if (cast->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        !lower_integer_type(cast->to_type.str()).has_value()) {
      return;
    }
    const auto immediate = resolve_index_operand(cast->operand, *block_value_aliases);
    if (!immediate.has_value()) {
      return;
    }
    (*block_value_aliases)[cast->result.str()] = bir::Value::immediate_i64(*immediate);
  }
}

}  // namespace

BirFunctionLowerer::BirFunctionLowerer(BirLoweringContext& context,
                                       const c4c::codegen::lir::LirFunction& function,
                                       const GlobalTypes& global_types,
                                       const FunctionSymbolSet& function_symbols,
                                       const TypeDeclMap& type_decls,
                                       const lir_to_bir_detail::BackendStructuredLayoutTable&
                                           structured_layouts)
    : context_(context),
      function_(function),
      global_types_(global_types),
      function_symbols_(function_symbols),
      type_decls_(type_decls),
      structured_layouts_(structured_layouts) {}

bool BirFunctionLowerer::canonicalize_compare_return_alias(
    const c4c::codegen::lir::LirOperand& ret_value,
    const bir::Value& lowered_value,
    bir::TypeKind return_type,
    std::vector<bir::Inst>* lowered_insts,
    bir::ReturnTerminator* lowered_ret) const {
  if (ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      lowered_value.kind != bir::Value::Kind::Named ||
      lowered_value.type != bir::TypeKind::I1 ||
      return_type != bir::TypeKind::I32) {
    return false;
  }

  const auto compare_it = compare_exprs_.find(ret_value.str());
  if (compare_it == compare_exprs_.end()) {
    return false;
  }

  for (auto inst_it = lowered_insts->rbegin(); inst_it != lowered_insts->rend(); ++inst_it) {
    auto* binary = std::get_if<bir::BinaryInst>(&*inst_it);
    if (binary == nullptr || binary->result.name != lowered_value.name) {
      continue;
    }

    binary->result.name = ret_value.str();
    lowered_ret->value = bir::Value::named(return_type, ret_value.str());
    return true;
  }

  return false;
}

std::optional<bir::Value> BirFunctionLowerer::lower_select_chain_value(
    const BlockLookup& blocks,
    const BranchChain& chain,
    const c4c::codegen::lir::LirOperand& incoming,
    bir::TypeKind expected_type,
    const ValueMap& value_aliases,
    std::vector<bir::Inst>* lowered_insts) const {
  if (incoming.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return lower_value(incoming, expected_type, value_aliases);
  }
  if (const auto alias = value_aliases.find(incoming.str()); alias != value_aliases.end()) {
    return alias->second;
  }
  if (auto lowered = lower_value(incoming, expected_type, value_aliases);
      lowered.has_value() && lowered->kind == bir::Value::Kind::Immediate) {
    return lowered;
  }

  ValueMap chain_aliases = value_aliases;
  CompareMap chain_compare_exprs;
  for (const auto& label : chain.labels) {
    const auto block_it = blocks.find(label);
    if (block_it == blocks.end()) {
      return std::nullopt;
    }
    for (const auto& inst : block_it->second->insts) {
      if (!resolve_select_chain_inst(inst, chain_aliases, chain_compare_exprs, lowered_insts)) {
        return std::nullopt;
      }
    }
  }

  return lower_value(incoming, expected_type, chain_aliases);
}

std::optional<bir::Function> BirFunctionLowerer::try_lower_canonical_select_function() {
  if (function_.is_declaration || function_.blocks.empty()) {
    return std::nullopt;
  }

  const auto return_info = infer_function_return_info();
  if (!return_info.has_value() || return_info->returned_via_sret) {
    return std::nullopt;
  }

  const auto& entry = function_.blocks.front();
  const auto* cond_br = std::get_if<c4c::codegen::lir::LirCondBr>(&entry.terminator);
  if (cond_br == nullptr) {
    return std::nullopt;
  }

  ValueMap value_aliases;
  CompareMap compare_exprs;
  std::vector<bir::Inst> prelude_insts;
  for (const auto& inst : entry.insts) {
    if (!lower_canonical_select_entry_inst(inst, value_aliases, compare_exprs, &prelude_insts)) {
      return std::nullopt;
    }
  }

  const auto compare_it = compare_exprs.find(cond_br->cond_name);
  if (compare_it == compare_exprs.end()) {
    return std::nullopt;
  }

  const auto blocks = make_block_lookup();
  const auto true_chain = follow_canonical_select_chain(blocks, cond_br->true_label);
  const auto false_chain = follow_canonical_select_chain(blocks, cond_br->false_label);
  if (!true_chain.has_value() || !false_chain.has_value() ||
      true_chain->join_label != false_chain->join_label) {
    return std::nullopt;
  }

  const auto join_it = blocks.find(true_chain->join_label);
  if (join_it == blocks.end()) {
    return std::nullopt;
  }
  const auto& join_block = *join_it->second;

  if (join_block.insts.empty()) {
    return std::nullopt;
  }
  const auto* phi = std::get_if<c4c::codegen::lir::LirPhiOp>(&join_block.insts.front());
  if (phi == nullptr || phi->incoming.size() != 2) {
    return std::nullopt;
  }
  for (std::size_t index = 1; index < join_block.insts.size(); ++index) {
    if (std::holds_alternative<c4c::codegen::lir::LirPhiOp>(join_block.insts[index])) {
      return std::nullopt;
    }
  }
  const auto phi_type = lir_to_bir_detail::lower_integer_type(phi->type_str.str());
  if (!phi_type.has_value()) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&join_block.terminator);
  if (ret == nullptr || !ret->value_str.has_value()) {
    return std::nullopt;
  }
  const c4c::codegen::lir::LirOperand ret_value(*ret->value_str);

  std::optional<c4c::codegen::lir::LirOperand> true_incoming;
  std::optional<c4c::codegen::lir::LirOperand> false_incoming;
  for (const auto& [value, label] : phi->incoming) {
    if (label == true_chain->leaf_label) {
      true_incoming = c4c::codegen::lir::LirOperand(value);
    } else if (label == false_chain->leaf_label) {
      false_incoming = c4c::codegen::lir::LirOperand(value);
    }
  }
  if (!true_incoming.has_value() || !false_incoming.has_value()) {
    return std::nullopt;
  }

  std::unordered_set<std::string> covered_labels;
  covered_labels.insert(entry.label);
  covered_labels.insert(join_block.label);
  covered_labels.insert(true_chain->labels.begin(), true_chain->labels.end());
  covered_labels.insert(false_chain->labels.begin(), false_chain->labels.end());
  if (covered_labels.size() != function_.blocks.size()) {
    return std::nullopt;
  }

  const auto true_value = lower_select_chain_value(
      blocks, *true_chain, *true_incoming, *phi_type, value_aliases, &prelude_insts);
  const auto false_value = lower_select_chain_value(
      blocks, *false_chain, *false_incoming, *phi_type, value_aliases, &prelude_insts);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = function_.name;
  lowered.return_type = return_info->type;
  lowered.return_size_bytes = return_info->size_bytes;
  lowered.return_align_bytes = return_info->align_bytes;
  lowered.return_abi = return_info->abi;
  if (!lower_function_params(function_, context_.target_profile, return_info, type_decls_, &lowered)) {
    return std::nullopt;
  }
  if (!collect_phi_lowering_plans().has_value()) {
    return std::nullopt;
  }

  bir::Block lowered_block;
  lowered_block.label = entry.label;
  lowered_block.insts = std::move(prelude_insts);
  lowered_block.insts.push_back(bir::SelectInst{
      .predicate = compare_it->second.opcode,
      .result = bir::Value::named(*phi_type, phi->result.str()),
      .compare_type = compare_it->second.operand_type,
      .lhs = compare_it->second.lhs,
      .rhs = compare_it->second.rhs,
      .true_value = *true_value,
      .false_value = *false_value,
  });

  ValueMap join_aliases = value_aliases;
  CompareMap join_compare_exprs = compare_exprs;
  for (std::size_t index = 1; index < join_block.insts.size(); ++index) {
    if (!resolve_select_chain_inst(
            join_block.insts[index], join_aliases, join_compare_exprs, &lowered_block.insts)) {
      return std::nullopt;
    }
  }

  bir::ReturnTerminator lowered_ret;
  const auto lowered_ret_value = lower_value(ret_value, return_info->type, join_aliases);
  if (!lowered_ret_value.has_value()) {
    return std::nullopt;
  }
  lowered_ret.value = *lowered_ret_value;
  lowered_block.terminator = lowered_ret;
  lowered.blocks.push_back(std::move(lowered_block));
  return lowered;
}

bool BirFunctionLowerer::lower_alloca_insts() {
  for (const auto& inst : function_.alloca_insts) {
    if (!lower_scalar_or_local_memory_inst(inst, &hoisted_alloca_scratch_)) {
      note_function_lowering_family_failure("local-memory semantic family");
      return false;
    }
  }
  return true;
}

void BirFunctionLowerer::seed_pointer_param_addresses() {
  for (const auto& param : lowered_function_.params) {
    if (param.type != bir::TypeKind::Ptr || param.is_sret || param.is_byval ||
        param.name.empty()) {
      continue;
    }
    pointer_value_addresses_[param.name] = PointerAddress{
        .base_value = bir::Value::named(bir::TypeKind::Ptr, param.name),
        .value_type = bir::TypeKind::Void,
        .byte_offset = 0,
    };
  }
}

bool BirFunctionLowerer::lower_block_phi_insts(const c4c::codegen::lir::LirBlock& block,
                                               bir::Block* lowered_block) {
  const auto phi_it = phi_plans_.find(block.label);
  if (phi_it == phi_plans_.end()) {
    return true;
  }

  for (const auto& phi_plan : phi_it->second) {
    if (phi_plan.kind == PhiLoweringPlan::Kind::AggregateValue) {
      aggregate_value_aliases_[phi_plan.result_name] = phi_plan.result_name;
      continue;
    }

    std::vector<bir::PhiIncoming> incomings;
    incomings.reserve(phi_plan.incomings.size());
    for (const auto& [label, operand] : phi_plan.incomings) {
      const auto incoming_value = lower_value(operand, phi_plan.type);
      if (!incoming_value.has_value()) {
        note_function_lowering_family_failure("scalar-control-flow semantic family");
        return false;
      }
      incomings.push_back(bir::PhiIncoming{
          .label = label,
          .value = *incoming_value,
      });
    }
    lowered_block->insts.push_back(bir::PhiInst{
        .result = bir::Value::named(phi_plan.type, phi_plan.result_name),
        .incomings = std::move(incomings),
    });
  }

  return true;
}

bool BirFunctionLowerer::initialize_aggregate_phi_state() {
  pending_aggregate_phi_copies_.clear();

  for (const auto& [_, block_plans] : phi_plans_) {
    for (const auto& phi_plan : block_plans) {
      if (phi_plan.kind != PhiLoweringPlan::Kind::AggregateValue) {
        continue;
      }

      if (!declare_local_aggregate_slots(
              phi_plan.type_text, phi_plan.result_name, phi_plan.aggregate_align_bytes)) {
        note_function_lowering_family_failure("scalar-control-flow semantic family");
        return false;
      }

      for (const auto& [label, operand] : phi_plan.incomings) {
        pending_aggregate_phi_copies_[label].push_back(PendingAggregatePhiCopy{
            .source = operand,
            .target_slot_name = phi_plan.result_name,
            .temp_prefix = phi_plan.result_name + ".phi.copy." + label,
        });
      }
    }
  }

  return true;
}

bool BirFunctionLowerer::apply_pending_aggregate_phi_copies(std::string_view predecessor_label,
                                                            std::vector<bir::Inst>* lowered_insts) {
  const auto copies_it = pending_aggregate_phi_copies_.find(std::string(predecessor_label));
  if (copies_it == pending_aggregate_phi_copies_.end()) {
    return true;
  }

  for (const auto& copy : copies_it->second) {
    const auto source_alias_it = aggregate_value_aliases_.find(copy.source.str());
    const std::string_view source_slot_name =
        source_alias_it != aggregate_value_aliases_.end()
            ? std::string_view(source_alias_it->second)
            : std::string_view(copy.source.str());
    const auto source_aggregate_it = local_aggregate_slots_.find(std::string(source_slot_name));
    const auto target_aggregate_it = local_aggregate_slots_.find(copy.target_slot_name);
    if (source_aggregate_it == local_aggregate_slots_.end() ||
        target_aggregate_it == local_aggregate_slots_.end()) {
      note_function_lowering_family_failure("scalar-control-flow semantic family");
      return false;
    }
    if (!append_local_aggregate_copy_from_slots(source_aggregate_it->second,
                                                target_aggregate_it->second,
                                                copy.temp_prefix,
                                                lowered_insts)) {
      note_function_lowering_family_failure("scalar-control-flow semantic family");
      return false;
    }
  }

  pending_aggregate_phi_copies_.erase(copies_it);
  return true;
}

bool BirFunctionLowerer::lower_block_insts(const c4c::codegen::lir::LirBlock& block,
                                           bir::Block* lowered_block) {
  for (const auto& inst : block.insts) {
    if (std::holds_alternative<c4c::codegen::lir::LirPhiOp>(inst)) {
      continue;
    }
    if (!lower_scalar_or_local_memory_inst(inst, &lowered_block->insts)) {
      note_function_lowering_family_failure("scalar/local-memory semantic family");
      return false;
    }
  }
  return true;
}

bool BirFunctionLowerer::lower_block_terminator(const c4c::codegen::lir::LirBlock& block,
                                                bir::Block* lowered_block,
                                                std::vector<bir::Block>* trailing_blocks) {
  if (!return_info_.has_value()) {
    note_function_lowering_family_failure("scalar-control-flow semantic family");
    return false;
  }

  if (const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator)) {
    bir::ReturnTerminator lowered_ret;
    if (ret->value_str.has_value()) {
      const c4c::codegen::lir::LirOperand ret_value(*ret->value_str);
      if (return_info_->returned_via_sret) {
        if (ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
          note_function_lowering_family_failure("scalar-control-flow semantic family");
          return false;
        }
        const auto aggregate_alias_it = aggregate_value_aliases_.find(ret_value.str());
        if (aggregate_alias_it == aggregate_value_aliases_.end()) {
          note_function_lowering_family_failure("scalar-control-flow semantic family");
          return false;
        }
        const auto source_aggregate_it = local_aggregate_slots_.find(aggregate_alias_it->second);
        if (source_aggregate_it == local_aggregate_slots_.end()) {
          note_function_lowering_family_failure("scalar-control-flow semantic family");
          return false;
        }
        if (!append_local_aggregate_copy_to_pointer(source_aggregate_it->second,
                                                    bir::Value::named(bir::TypeKind::Ptr,
                                                                      "%ret.sret"),
                                                    return_info_->align_bytes,
                                                    function_.name + ".ret.sret.copy",
                                                    &lowered_block->insts)) {
          note_function_lowering_family_failure("scalar-control-flow semantic family");
          return false;
        }
      } else {
        const auto value = lower_value(ret_value, return_info_->type);
        if (!value.has_value()) {
          note_function_lowering_family_failure("scalar-control-flow semantic family");
          return false;
        }
        if (!canonicalize_compare_return_alias(
                ret_value, *value, return_info_->type, &lowered_block->insts, &lowered_ret)) {
          lowered_ret.value = *value;
        }
      }
    }
    lowered_block->terminator = lowered_ret;
    return true;
  }

  if (const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&block.terminator)) {
    lowered_block->terminator = bir::BranchTerminator{.target_label = br->target_label};
    return true;
  }

  if (const auto* cond_br = std::get_if<c4c::codegen::lir::LirCondBr>(&block.terminator)) {
    const auto condition = lower_value(cond_br->cond_name, bir::TypeKind::I1);
    if (!condition.has_value()) {
      note_function_lowering_family_failure("scalar-control-flow semantic family");
      return false;
    }
    lowered_block->terminator = bir::CondBranchTerminator{
        .condition = *condition,
        .true_label = cond_br->true_label,
        .false_label = cond_br->false_label,
    };
    return true;
  }

  if (const auto* sw = std::get_if<c4c::codegen::lir::LirSwitch>(&block.terminator)) {
    const auto selector_type = lower_integer_type(sw->selector_type);
    if (!selector_type.has_value()) {
      note_function_lowering_family_failure("scalar-control-flow semantic family");
      return false;
    }
    const auto selector =
        lower_value(c4c::codegen::lir::LirOperand(sw->selector_name), *selector_type);
    if (!selector.has_value()) {
      note_function_lowering_family_failure("scalar-control-flow semantic family");
      return false;
    }
    if (sw->cases.empty()) {
      lowered_block->terminator = bir::BranchTerminator{.target_label = sw->default_label};
      return true;
    }

    std::unordered_set<std::string> used_labels;
    used_labels.reserve(function_.blocks.size() + trailing_blocks->size() + 1u);
    for (const auto& function_block : function_.blocks) {
      used_labels.insert(function_block.label);
    }
    for (const auto& trailing_block : *trailing_blocks) {
      used_labels.insert(trailing_block.label);
    }
    used_labels.insert(block.label);

    auto make_unique_switch_label = [&](std::size_t case_index) {
      std::string label = block.label + ".switch.case." + std::to_string(case_index);
      std::size_t dedupe_index = 0;
      while (!used_labels.emplace(label).second) {
        ++dedupe_index;
        label = block.label + ".switch.case." + std::to_string(case_index) + "." +
                std::to_string(dedupe_index);
      }
      return label;
    };

    std::vector<std::string> compare_labels;
    compare_labels.reserve(sw->cases.size() > 0 ? sw->cases.size() - 1u : 0u);
    for (std::size_t case_index = 1; case_index < sw->cases.size(); ++case_index) {
      compare_labels.push_back(make_unique_switch_label(case_index));
    }

    std::vector<bir::Block> switch_blocks;
    if (sw->cases.size() > 1u) {
      switch_blocks.resize(sw->cases.size() - 1u);
      for (std::size_t block_index = 0; block_index < switch_blocks.size(); ++block_index) {
        switch_blocks[block_index].label = compare_labels[block_index];
      }
    }

    for (std::size_t case_index = 0; case_index < sw->cases.size(); ++case_index) {
      auto* compare_block = case_index == 0 ? lowered_block : &switch_blocks[case_index - 1u];
      const auto case_immediate = make_integer_immediate(*selector_type, sw->cases[case_index].first);
      if (!case_immediate.has_value()) {
        note_function_lowering_family_failure("scalar-control-flow semantic family");
        return false;
      }

      const std::string compare_name =
          "%" + block.label + ".switch.case." + std::to_string(case_index) + ".match";
      compare_block->insts.push_back(bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Eq,
          .result = bir::Value::named(bir::TypeKind::I1, compare_name),
          .operand_type = *selector_type,
          .lhs = *selector,
          .rhs = *case_immediate,
      });

      const std::string false_label = case_index + 1u < sw->cases.size()
                                          ? compare_labels[case_index]
                                          : sw->default_label;
      compare_block->terminator = bir::CondBranchTerminator{
          .condition = bir::Value::named(bir::TypeKind::I1, compare_name),
          .true_label = sw->cases[case_index].second,
          .false_label = false_label,
      };
    }

    trailing_blocks->insert(
        trailing_blocks->end(),
        std::make_move_iterator(switch_blocks.begin()),
        std::make_move_iterator(switch_blocks.end()));
    return true;
  }

  note_function_lowering_family_failure("scalar-control-flow semantic family");
  return false;
}

bool BirFunctionLowerer::lower_block(const c4c::codegen::lir::LirBlock& block,
                                     bool* emitted_hoisted_alloca_scratch) {
  bir::Block lowered_block;
  std::vector<bir::Block> trailing_blocks;
  lowered_block.label = block.label;
  if (!*emitted_hoisted_alloca_scratch &&
      block.label == function_.blocks.front().label &&
      !hoisted_alloca_scratch_.empty()) {
    lowered_block.insts = std::move(hoisted_alloca_scratch_);
    *emitted_hoisted_alloca_scratch = true;
  }

  if (!lower_block_phi_insts(block, &lowered_block) ||
      !lower_block_insts(block, &lowered_block) ||
      !apply_pending_aggregate_phi_copies(block.label, &lowered_block.insts) ||
      !lower_block_terminator(block, &lowered_block, &trailing_blocks)) {
    return false;
  }

  lowered_function_.blocks.push_back(std::move(lowered_block));
  lowered_function_.blocks.insert(
      lowered_function_.blocks.end(),
      std::make_move_iterator(trailing_blocks.begin()),
      std::make_move_iterator(trailing_blocks.end()));
  return true;
}

void BirFunctionLowerer::note_function_lowering_family_failure(std::string_view family) {
  const std::string function_name = function_name_for_reporting(context_.lir_module, function_);
  context_.note("function",
                "semantic lir_to_bir function '" + function_name +
                    "' failed in " + std::string(family));
}

std::optional<bir::Function> BirFunctionLowerer::lower() {
  if (function_.is_declaration || function_.blocks.empty()) {
    return std::nullopt;
  }

  if (auto lowered = try_lower_canonical_select_function(); lowered.has_value()) {
    return lowered;
  }

  return_info_ = infer_function_return_info();
  if (!return_info_.has_value()) {
    note_function_lowering_family_failure("function-signature semantic family");
    return std::nullopt;
  }

  lowered_function_.name = function_.name;
  lowered_function_.return_type = return_info_->type;
  lowered_function_.return_size_bytes = return_info_->size_bytes;
  lowered_function_.return_align_bytes = return_info_->align_bytes;
  lowered_function_.return_abi = return_info_->abi;
  if (!lower_function_params(function_,
                             context_.target_profile,
                             return_info_,
                             type_decls_,
                             &lowered_function_)) {
    note_function_lowering_family_failure("function-signature semantic family");
    return std::nullopt;
  }

  auto phi_plans = collect_phi_lowering_plans();
  if (!phi_plans.has_value()) {
    note_function_lowering_family_failure("scalar-control-flow semantic family");
    return std::nullopt;
  }
  phi_plans_ = std::move(*phi_plans);
  aggregate_params_ = collect_aggregate_params();
  seed_pointer_param_addresses();
  if (!initialize_aggregate_phi_state()) {
    return std::nullopt;
  }

  if (!materialize_aggregate_param_aliases(&hoisted_alloca_scratch_)) {
    note_function_lowering_family_failure("local-memory semantic family");
    return std::nullopt;
  }
  if (!lower_alloca_insts()) {
    return std::nullopt;
  }

  bool emitted_hoisted_alloca_scratch = false;
  for (const auto& block : function_.blocks) {
    if (!lower_block(block, &emitted_hoisted_alloca_scratch)) {
      return std::nullopt;
    }
  }

  return lowered_function_;
}

std::optional<bir::Module> lower_module(BirLoweringContext& context,
                                        const BirModuleAnalysis& analysis) {
  bir::Module module;
  module.target_triple = c4c::llvm_target_triple(context.target_profile);
  module.data_layout = context.lir_module.data_layout;

  if (analysis.function_count == 0 && analysis.global_count == 0 &&
      analysis.string_constant_count == 0 && analysis.extern_decl_count == 0) {
    context.note("module", "lowered empty module shell to BIR");
    return module;
  }

  if (analysis.global_count != 0 || analysis.string_constant_count != 0 ||
      analysis.extern_decl_count != 0) {
    context.note(
        "module",
        "bootstrap lir_to_bir only supports scalar integer/pointer globals, linear integer-array globals, aggregate-backed globals with honest byte-address semantics, string-backed byte data, and extern integer-array globals right now");
  }

  GlobalTypes global_types;
  FunctionSymbolSet function_symbols;
  global_types.reserve(context.lir_module.globals.size() + context.lir_module.string_pool.size());
  function_symbols.reserve(context.lir_module.extern_decls.size() +
                           context.lir_module.functions.size());
  for (const auto& decl : context.lir_module.extern_decls) {
    function_symbols.insert(extern_decl_with_resolved_name(context.lir_module, decl).name);
  }
  for (const auto& function : context.lir_module.functions) {
    function_symbols.insert(function_with_resolved_name(context.lir_module, function).name);
  }
  const auto type_decls = build_type_decl_map(context.lir_module.type_decls);
  const auto structured_layouts = build_backend_structured_layout_table(
      context.lir_module.struct_decls,
      context.lir_module.struct_names,
      type_decls);
  report_backend_structured_layout_parity_notes(context, structured_layouts);
  for (const auto& global : context.lir_module.globals) {
    const auto resolved_global = global_with_resolved_name(context.lir_module, global);
    GlobalInfo info;
    auto lowered_global = lower_minimal_global(resolved_global, type_decls, structured_layouts, &info);
    if (!lowered_global.has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports scalar integer/pointer globals, linear integer-array globals, and aggregate-backed globals with honest byte-address semantics right now");
      return std::nullopt;
    }
    global_types.emplace(lowered_global->name, info);
    module.globals.push_back(std::move(*lowered_global));
  }

  for (const auto& string_constant : context.lir_module.string_pool) {
    GlobalInfo info;
    auto lowered_global = lower_string_constant_global(string_constant, &info);
    if (!lowered_global.has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports byte-addressable string-pool constants right now");
      return std::nullopt;
    }
    global_types.emplace(lowered_global->name, info);
    module.globals.push_back(std::move(*lowered_global));
  }

  if (!resolve_pointer_initializer_offsets(global_types, function_symbols)) {
    context.note(
        "module",
        "bootstrap lir_to_bir only supports aggregate pointer fields initialized from addressable globals right now");
    return std::nullopt;
  }

  std::unordered_set<std::string> resolving_global_addresses;
  for (auto& [global_name, info] : global_types) {
    if (info.initializer_symbol_name.empty()) {
      continue;
    }
    if (!resolve_known_global_address(
             global_name, global_types, function_symbols, &resolving_global_addresses)
             .has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports pointer globals initialized from addressable globals or pointer-global aliases that resolve to in-bounds global data right now");
      return std::nullopt;
    }
  }

  for (const auto& function : context.lir_module.functions) {
    for (const auto& block : function.blocks) {
      BirFunctionLowerer::ValueMap block_value_aliases;
      std::unordered_map<std::string, std::pair<std::size_t, std::size_t>> block_calloc_results;
      for (const auto& inst : block.insts) {
        record_block_integer_constant_alias(inst, &block_value_aliases);
        if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
          if (call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
            continue;
          }
          const auto parsed_call = BirFunctionLowerer::parse_direct_global_typed_call(*call);
          if (!parsed_call.has_value() || parsed_call->symbol_name != "calloc" ||
              parsed_call->typed_call.args.size() != 2 ||
              parsed_call->typed_call.param_types.size() != 2) {
            continue;
          }
          const auto count_arg = lir_to_bir_detail::parse_typed_operand(
              std::string(parsed_call->typed_call.args[0].type) + " " +
              std::string(parsed_call->typed_call.args[0].operand));
          const auto size_arg = lir_to_bir_detail::parse_typed_operand(
              std::string(parsed_call->typed_call.args[1].type) + " " +
              std::string(parsed_call->typed_call.args[1].operand));
          if (!count_arg.has_value() || !size_arg.has_value()) {
            continue;
          }
          const auto count_value = resolve_index_operand(count_arg->operand, block_value_aliases);
          const auto size_value = resolve_index_operand(size_arg->operand, block_value_aliases);
          if (!count_value.has_value() || !size_value.has_value() || *count_value <= 0 ||
              *size_value <= 0) {
            continue;
          }
          block_calloc_results[call->result.str()] = std::make_pair(
              static_cast<std::size_t>(*count_value),
              static_cast<std::size_t>(*size_value));
          continue;
        }

        const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst);
        if (store == nullptr || store->type_str.str() != "ptr" ||
            store->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global ||
            store->val.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
          continue;
        }

        const std::string global_name = store->ptr.str().substr(1);
        const auto global_it = global_types.find(global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          continue;
        }

        const auto calloc_it = block_calloc_results.find(store->val.str());
        if (calloc_it == block_calloc_results.end()) {
          continue;
        }

        global_it->second.runtime_element_count = calloc_it->second.first;
        global_it->second.runtime_element_stride_bytes = calloc_it->second.second;
      }
    }
  }

  for (const auto& decl : context.lir_module.extern_decls) {
    auto lowered_decl = BirFunctionLowerer::lower_extern_decl(
        extern_decl_with_resolved_name(context.lir_module, decl),
        context.target_profile,
        type_decls,
        structured_layouts);
    if (!lowered_decl.has_value()) {
      continue;
    }
    module.functions.push_back(std::move(*lowered_decl));
  }

  for (const auto& function : context.lir_module.functions) {
    const auto resolved_function = function_with_resolved_name(context.lir_module, function);
    if (resolved_function.is_declaration) {
      auto lowered_decl =
          BirFunctionLowerer::lower_decl_function(
              resolved_function, context.target_profile, type_decls, structured_layouts);
      if (!lowered_decl.has_value()) {
        continue;
      }
      module.functions.push_back(std::move(*lowered_decl));
      continue;
    }

    BirFunctionLowerer function_lowerer(
        context, resolved_function, global_types, function_symbols, type_decls, structured_layouts);
    auto lowered_function = function_lowerer.lower();
    if (!lowered_function.has_value()) {
      std::string message = "semantic lir_to_bir failed outside the ";
      message += kModuleCapabilityBucketSummary;
      if (const auto function_failure = latest_function_failure_note(context);
          function_failure.has_value()) {
        message += "; latest function failure: ";
        message += *function_failure;
      }
      context.note("module", std::move(message));
      return std::nullopt;
    }
    module.functions.push_back(std::move(*lowered_function));
  }

  context.note(
      "module",
      std::string("lowered semantic-BIR module within the ") +
          std::string(kModuleCapabilityBucketSummary));
  return module;
}

}  // namespace c4c::backend
