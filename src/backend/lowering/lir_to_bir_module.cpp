#include "lir_to_bir.hpp"

#include <optional>
#include <unordered_set>
#include <utility>

namespace c4c::backend {

namespace {

using lir_to_bir_detail::build_type_decl_map;
using lir_to_bir_detail::FunctionSymbolSet;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::GlobalTypes;
using lir_to_bir_detail::lower_minimal_global;
using lir_to_bir_detail::lower_string_constant_global;
using lir_to_bir_detail::resolve_known_global_address;
using lir_to_bir_detail::resolve_pointer_initializer_offsets;
using lir_to_bir_detail::TypeDeclMap;

}  // namespace

BirFunctionLowerer::BirFunctionLowerer(BirLoweringContext& context,
                                       const c4c::codegen::lir::LirFunction& function,
                                       const GlobalTypes& global_types,
                                       const FunctionSymbolSet& function_symbols,
                                       const TypeDeclMap& type_decls)
    : context_(context),
      function_(function),
      global_types_(global_types),
      function_symbols_(function_symbols),
      type_decls_(type_decls) {}

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
  if (!lower_function_params(function_, return_info, type_decls_, &lowered)) {
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
      return false;
    }
  }
  return true;
}

bool BirFunctionLowerer::lower_block_phi_insts(const c4c::codegen::lir::LirBlock& block,
                                               bir::Block* lowered_block) {
  const auto phi_it = phi_plans_.find(block.label);
  if (phi_it == phi_plans_.end()) {
    return true;
  }

  for (const auto& phi_plan : phi_it->second) {
    std::vector<bir::PhiIncoming> incomings;
    incomings.reserve(phi_plan.incomings.size());
    for (const auto& [label, operand] : phi_plan.incomings) {
      const auto incoming_value = lower_value(operand, phi_plan.type);
      if (!incoming_value.has_value()) {
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

bool BirFunctionLowerer::lower_block_insts(const c4c::codegen::lir::LirBlock& block,
                                           bir::Block* lowered_block) {
  for (const auto& inst : block.insts) {
    if (std::holds_alternative<c4c::codegen::lir::LirPhiOp>(inst)) {
      continue;
    }
    if (!lower_scalar_or_local_memory_inst(inst, &lowered_block->insts)) {
      return false;
    }
  }
  return true;
}

bool BirFunctionLowerer::lower_block_terminator(const c4c::codegen::lir::LirBlock& block,
                                                bir::Block* lowered_block) {
  if (!return_info_.has_value()) {
    return false;
  }

  if (const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator)) {
    bir::ReturnTerminator lowered_ret;
    if (ret->value_str.has_value()) {
      const c4c::codegen::lir::LirOperand ret_value(*ret->value_str);
      if (return_info_->returned_via_sret) {
        if (ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
          return false;
        }
        const auto aggregate_alias_it = aggregate_value_aliases_.find(ret_value.str());
        if (aggregate_alias_it == aggregate_value_aliases_.end()) {
          return false;
        }
        const auto source_aggregate_it = local_aggregate_slots_.find(aggregate_alias_it->second);
        if (source_aggregate_it == local_aggregate_slots_.end()) {
          return false;
        }
        if (!append_local_aggregate_copy_to_pointer(source_aggregate_it->second,
                                                    bir::Value::named(bir::TypeKind::Ptr,
                                                                      "%ret.sret"),
                                                    return_info_->align_bytes,
                                                    function_.name + ".ret.sret.copy",
                                                    &lowered_block->insts)) {
          return false;
        }
      } else {
        const auto value = lower_value(ret_value, return_info_->type);
        if (!value.has_value()) {
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
      return false;
    }
    lowered_block->terminator = bir::CondBranchTerminator{
        .condition = *condition,
        .true_label = cond_br->true_label,
        .false_label = cond_br->false_label,
    };
    return true;
  }

  return false;
}

bool BirFunctionLowerer::lower_block(const c4c::codegen::lir::LirBlock& block,
                                     bool* emitted_hoisted_alloca_scratch) {
  bir::Block lowered_block;
  lowered_block.label = block.label;
  if (!*emitted_hoisted_alloca_scratch &&
      block.label == function_.blocks.front().label &&
      !hoisted_alloca_scratch_.empty()) {
    lowered_block.insts = std::move(hoisted_alloca_scratch_);
    *emitted_hoisted_alloca_scratch = true;
  }

  if (!lower_block_phi_insts(block, &lowered_block) ||
      !lower_block_insts(block, &lowered_block) ||
      !lower_block_terminator(block, &lowered_block)) {
    return false;
  }

  lowered_function_.blocks.push_back(std::move(lowered_block));
  return true;
}

std::optional<bir::Function> BirFunctionLowerer::lower() {
  (void)context_;
  if (function_.is_declaration || function_.blocks.empty()) {
    return std::nullopt;
  }

  if (auto lowered = try_lower_canonical_select_function(); lowered.has_value()) {
    return lowered;
  }

  return_info_ = infer_function_return_info();
  if (!return_info_.has_value()) {
    return std::nullopt;
  }

  lowered_function_.name = function_.name;
  lowered_function_.return_type = return_info_->type;
  lowered_function_.return_size_bytes = return_info_->size_bytes;
  lowered_function_.return_align_bytes = return_info_->align_bytes;
  if (!lower_function_params(function_, return_info_, type_decls_, &lowered_function_)) {
    return std::nullopt;
  }

  auto phi_plans = collect_phi_lowering_plans();
  if (!phi_plans.has_value()) {
    return std::nullopt;
  }
  phi_plans_ = std::move(*phi_plans);
  aggregate_params_ = collect_aggregate_params();

  if (!materialize_aggregate_param_aliases(&hoisted_alloca_scratch_) || !lower_alloca_insts()) {
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
  module.target_triple = context.lir_module.target_triple;
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
        "bootstrap lir_to_bir only supports minimal scalar globals, linear integer-array globals, aggregate-backed globals with honest byte-address semantics, string-backed byte data, and extern integer-array globals right now");
  }

  GlobalTypes global_types;
  FunctionSymbolSet function_symbols;
  global_types.reserve(context.lir_module.globals.size() + context.lir_module.string_pool.size());
  function_symbols.reserve(context.lir_module.extern_decls.size() +
                           context.lir_module.functions.size());
  for (const auto& decl : context.lir_module.extern_decls) {
    function_symbols.insert(decl.name);
  }
  for (const auto& function : context.lir_module.functions) {
    function_symbols.insert(function.name);
  }
  const auto type_decls = build_type_decl_map(context.lir_module.type_decls);
  for (const auto& global : context.lir_module.globals) {
    GlobalInfo info;
    auto lowered_global = lower_minimal_global(global, type_decls, &info);
    if (!lowered_global.has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports minimal scalar globals, linear integer-array globals, and aggregate-backed globals with honest byte-address semantics right now");
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

  for (const auto& decl : context.lir_module.extern_decls) {
    auto lowered_decl = BirFunctionLowerer::lower_extern_decl(decl);
    if (!lowered_decl.has_value()) {
      continue;
    }
    module.functions.push_back(std::move(*lowered_decl));
  }

  for (const auto& function : context.lir_module.functions) {
    if (function.is_declaration) {
      auto lowered_decl = BirFunctionLowerer::lower_decl_function(function);
      if (!lowered_decl.has_value()) {
        continue;
      }
      module.functions.push_back(std::move(*lowered_decl));
      continue;
    }

    BirFunctionLowerer function_lowerer(
        context, function, global_types, function_symbols, type_decls);
    auto lowered_function = function_lowerer.lower();
    if (!lowered_function.has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports scalar compare/branch/select/return functions right now");
      return std::nullopt;
    }
    module.functions.push_back(std::move(*lowered_function));
  }

  context.note(
      "module",
      "lowered scalar compare/select/local-memory/global/call/return module to BIR");
  return module;
}

}  // namespace c4c::backend
