#include "lir_to_bir.hpp"

#include "call_decode.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace c4c::backend {

namespace {

using lir_to_bir_detail::AggregateField;
using lir_to_bir_detail::AggregateTypeLayout;
using lir_to_bir_detail::build_type_decl_map;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::FunctionSymbolSet;
using lir_to_bir_detail::GlobalAddress;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::GlobalTypes;
using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::lower_minimal_global;
using lir_to_bir_detail::lower_string_constant_global;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::ParsedTypedOperand;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::resolve_known_global_address;
using lir_to_bir_detail::resolve_pointer_initializer_offsets;
using lir_to_bir_detail::split_top_level_initializer_items;
using lir_to_bir_detail::type_size_bytes;
using lir_to_bir_detail::TypeDeclMap;
using lir_to_bir_detail::ValueMap;
using lir_to_bir_detail::AddressedGlobalPointerSlots;
using lir_to_bir_detail::AggregateParamInfo;
using lir_to_bir_detail::AggregateParamMap;
using lir_to_bir_detail::AggregateValueAliasMap;
using lir_to_bir_detail::BlockLookup;
using lir_to_bir_detail::BranchChain;
using lir_to_bir_detail::collect_phi_lowering_plans;
using lir_to_bir_detail::CompareExpr;
using lir_to_bir_detail::CompareMap;
using lir_to_bir_detail::DynamicGlobalAggregateArrayAccess;
using lir_to_bir_detail::DynamicGlobalAggregateArrayMap;
using lir_to_bir_detail::DynamicGlobalPointerArrayAccess;
using lir_to_bir_detail::DynamicGlobalPointerArrayMap;
using lir_to_bir_detail::DynamicLocalAggregateArrayAccess;
using lir_to_bir_detail::DynamicLocalAggregateArrayMap;
using lir_to_bir_detail::DynamicLocalPointerArrayAccess;
using lir_to_bir_detail::DynamicLocalPointerArrayMap;
using lir_to_bir_detail::follow_canonical_select_chain;
using lir_to_bir_detail::follow_empty_branch_chain;
using lir_to_bir_detail::GlobalAddressIntMap;
using lir_to_bir_detail::GlobalAddressSlots;
using lir_to_bir_detail::GlobalObjectAddressIntMap;
using lir_to_bir_detail::GlobalObjectPointerMap;
using lir_to_bir_detail::GlobalPointerMap;
using lir_to_bir_detail::GlobalPointerSlotKey;
using lir_to_bir_detail::GlobalPointerSlotKeyHash;
using lir_to_bir_detail::LocalAddressSlots;
using lir_to_bir_detail::LocalAggregateFieldSet;
using lir_to_bir_detail::LocalAggregateSlotMap;
using lir_to_bir_detail::LocalAggregateSlots;
using lir_to_bir_detail::LocalArraySlotMap;
using lir_to_bir_detail::LocalArraySlots;
using lir_to_bir_detail::LocalPointerArrayBase;
using lir_to_bir_detail::LocalPointerArrayBaseMap;
using lir_to_bir_detail::LocalPointerSlots;
using lir_to_bir_detail::LocalPointerValueAliasMap;
using lir_to_bir_detail::LocalSlotTypes;
using lir_to_bir_detail::LoweredReturnInfo;
using lir_to_bir_detail::make_block_lookup;
using lir_to_bir_detail::fold_i64_binary_immediates;
using lir_to_bir_detail::fold_integer_cast;
using lir_to_bir_detail::infer_function_return_info;
using lir_to_bir_detail::lower_canonical_select_entry_inst;
using lir_to_bir_detail::lower_cast_opcode;
using lir_to_bir_detail::lower_cmp_predicate;
using lir_to_bir_detail::lower_decl_function;
using lir_to_bir_detail::lower_extern_decl;
using lir_to_bir_detail::lower_function_params;
using lir_to_bir_detail::lower_param_type;
using lir_to_bir_detail::lower_return_info_from_type;
using lir_to_bir_detail::lower_scalar_binary_opcode;
using lir_to_bir_detail::lower_scalar_compare_inst;
using lir_to_bir_detail::lower_scalar_or_function_pointer_type;
using lir_to_bir_detail::lower_signature_return_info;
using lir_to_bir_detail::lower_value;
using lir_to_bir_detail::resolve_select_chain_inst;
using lir_to_bir_detail::PhiBlockPlanMap;
using lir_to_bir_detail::PhiLoweringPlan;
using lir_to_bir_detail::append_local_aggregate_copy_from_slots;
using lir_to_bir_detail::append_local_aggregate_copy_to_pointer;
using lir_to_bir_detail::append_local_aggregate_scalar_slots;
using lir_to_bir_detail::collect_aggregate_params;
using lir_to_bir_detail::collect_sorted_leaf_slots;
using lir_to_bir_detail::declare_local_aggregate_slots;
using lir_to_bir_detail::materialize_aggregate_param_aliases;
using lir_to_bir_detail::lower_scalar_or_local_memory_inst;

bool canonicalize_compare_return_alias(const c4c::codegen::lir::LirOperand& ret_value,
                                       const bir::Value& lowered_value,
                                       bir::TypeKind return_type,
                                       const CompareMap& compare_exprs,
                                       std::vector<bir::Inst>* lowered_insts,
                                       bir::ReturnTerminator* lowered_ret) {
  if (ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      lowered_value.kind != bir::Value::Kind::Named ||
      lowered_value.type != bir::TypeKind::I1 ||
      return_type != bir::TypeKind::I32) {
    return false;
  }

  const auto compare_it = compare_exprs.find(ret_value.str());
  if (compare_it == compare_exprs.end()) {
    return false;
  }

  for (auto inst_it = lowered_insts->rbegin(); inst_it != lowered_insts->rend(); ++inst_it) {
    auto* binary = std::get_if<bir::BinaryInst>(&*inst_it);
    if (binary == nullptr) {
      continue;
    }
    if (binary->result.name != lowered_value.name) {
      continue;
    }

    binary->result.name = ret_value.str();
    lowered_ret->value = bir::Value::named(return_type, ret_value.str());
    return true;
  }

  return false;
}

std::optional<bir::Value> lower_select_chain_value(const BlockLookup& blocks,
                                                   const BranchChain& chain,
                                                   const c4c::codegen::lir::LirOperand& incoming,
                                                   bir::TypeKind expected_type,
                                                   const ValueMap& value_aliases,
                                                   std::vector<bir::Inst>* lowered_insts) {
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

std::optional<bir::Function> lower_canonical_select_function(
    BirLoweringContext& context,
    const c4c::codegen::lir::LirFunction& function,
    const TypeDeclMap& type_decls) {
  (void)context;
  if (function.is_declaration || function.blocks.empty()) {
    return std::nullopt;
  }

  const auto return_info = infer_function_return_info(function, type_decls);
  if (!return_info.has_value() || return_info->returned_via_sret) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
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

  const auto blocks = make_block_lookup(function);
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
  const auto phi_type = lower_integer_type(phi->type_str.str());
  if (!phi_type.has_value()) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&join_block.terminator);
  if (ret == nullptr || !ret->value_str.has_value()) {
    return std::nullopt;
  }
  const auto ret_value = c4c::codegen::lir::LirOperand(*ret->value_str);

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
  if (covered_labels.size() != function.blocks.size()) {
    return std::nullopt;
  }

  const auto true_value =
      lower_select_chain_value(
          blocks, *true_chain, *true_incoming, *phi_type, value_aliases, &prelude_insts);
  const auto false_value =
      lower_select_chain_value(
          blocks, *false_chain, *false_incoming, *phi_type, value_aliases, &prelude_insts);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = return_info->type;
  if (!lower_function_params(function, return_info, type_decls, &lowered)) {
    return std::nullopt;
  }
  if (!collect_phi_lowering_plans(function).has_value()) {
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

std::optional<bir::Function> lower_branch_family_function(
    BirLoweringContext& context,
    const c4c::codegen::lir::LirFunction& function,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    const TypeDeclMap& type_decls) {
  (void)context;
  if (function.is_declaration || function.blocks.empty()) {
    return std::nullopt;
  }

  if (auto lowered = lower_canonical_select_function(context, function, type_decls);
      lowered.has_value()) {
    return lowered;
  }

  const auto return_info = infer_function_return_info(function, type_decls);
  if (!return_info.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = return_info->type;
  lowered.return_size_bytes = return_info->size_bytes;
  lowered.return_align_bytes = return_info->align_bytes;
  if (!lower_function_params(function, return_info, type_decls, &lowered)) {
    return std::nullopt;
  }
  const auto phi_plans = collect_phi_lowering_plans(function);
  if (!phi_plans.has_value()) {
    return std::nullopt;
  }
  const auto aggregate_params = collect_aggregate_params(function, type_decls);

  ValueMap value_aliases;
  CompareMap compare_exprs;
  AggregateValueAliasMap aggregate_value_aliases;
  LocalSlotTypes local_slot_types;
  LocalPointerSlots local_pointer_slots;
  LocalArraySlotMap local_array_slots;
  LocalPointerArrayBaseMap local_pointer_array_bases;
  DynamicLocalPointerArrayMap dynamic_local_pointer_arrays;
  DynamicLocalAggregateArrayMap dynamic_local_aggregate_arrays;
  LocalAggregateSlotMap local_aggregate_slots;
  LocalAggregateFieldSet local_aggregate_field_slots;
  LocalPointerValueAliasMap local_pointer_value_aliases;
  LocalAddressSlots local_address_slots;
  GlobalAddressSlots global_address_slots;
  AddressedGlobalPointerSlots addressed_global_pointer_slots;
  GlobalPointerMap global_pointer_slots;
  DynamicGlobalPointerArrayMap dynamic_global_pointer_arrays;
  DynamicGlobalAggregateArrayMap dynamic_global_aggregate_arrays;
  GlobalObjectPointerMap global_object_pointer_slots;
  GlobalAddressIntMap global_address_ints;
  GlobalObjectAddressIntMap global_object_address_ints;
  std::vector<bir::Inst> hoisted_alloca_scratch;

  if (!materialize_aggregate_param_aliases(aggregate_params,
                                           type_decls,
                                           local_slot_types,
                                           local_pointer_slots,
                                           local_aggregate_field_slots,
                                           aggregate_value_aliases,
                                           &lowered,
                                           local_aggregate_slots,
                                           &hoisted_alloca_scratch)) {
    return std::nullopt;
  }

  for (const auto& inst : function.alloca_insts) {
    if (!lower_scalar_or_local_memory_inst(
            inst,
            value_aliases,
            compare_exprs,
            aggregate_value_aliases,
            local_slot_types,
            local_pointer_slots,
            local_array_slots,
            local_pointer_array_bases,
            dynamic_local_pointer_arrays,
            dynamic_local_aggregate_arrays,
            local_aggregate_slots,
            local_aggregate_field_slots,
            local_pointer_value_aliases,
            local_address_slots,
            global_address_slots,
            addressed_global_pointer_slots,
            global_pointer_slots,
            dynamic_global_pointer_arrays,
            dynamic_global_aggregate_arrays,
            global_object_pointer_slots,
            global_address_ints,
            global_object_address_ints,
            aggregate_params,
            global_types,
            function_symbols,
            type_decls,
            &lowered,
            &hoisted_alloca_scratch)) {
      return std::nullopt;
    }
  }

  bool emitted_hoisted_alloca_scratch = false;

  for (const auto& block : function.blocks) {
    bir::Block lowered_block;
    lowered_block.label = block.label;
    if (!emitted_hoisted_alloca_scratch && block.label == function.blocks.front().label &&
        !hoisted_alloca_scratch.empty()) {
      lowered_block.insts = std::move(hoisted_alloca_scratch);
      emitted_hoisted_alloca_scratch = true;
    }

    if (const auto phi_it = phi_plans->find(block.label); phi_it != phi_plans->end()) {
      for (const auto& phi_plan : phi_it->second) {
        std::vector<bir::PhiIncoming> incomings;
        incomings.reserve(phi_plan.incomings.size());
        for (const auto& [label, operand] : phi_plan.incomings) {
          const auto incoming_value = lower_value(operand, phi_plan.type, value_aliases);
          if (!incoming_value.has_value()) {
            return std::nullopt;
          }
          incomings.push_back(bir::PhiIncoming{
              .label = label,
              .value = *incoming_value,
          });
        }
        lowered_block.insts.push_back(bir::PhiInst{
            .result = bir::Value::named(phi_plan.type, phi_plan.result_name),
            .incomings = std::move(incomings),
        });
      }
    }

    for (const auto& inst : block.insts) {
      if (std::holds_alternative<c4c::codegen::lir::LirPhiOp>(inst)) {
        continue;
      }
      if (!lower_scalar_or_local_memory_inst(
              inst,
              value_aliases,
              compare_exprs,
              aggregate_value_aliases,
              local_slot_types,
              local_pointer_slots,
              local_array_slots,
              local_pointer_array_bases,
              dynamic_local_pointer_arrays,
              dynamic_local_aggregate_arrays,
              local_aggregate_slots,
              local_aggregate_field_slots,
              local_pointer_value_aliases,
              local_address_slots,
              global_address_slots,
              addressed_global_pointer_slots,
              global_pointer_slots,
              dynamic_global_pointer_arrays,
              dynamic_global_aggregate_arrays,
              global_object_pointer_slots,
              global_address_ints,
              global_object_address_ints,
              aggregate_params,
              global_types,
              function_symbols,
              type_decls,
              &lowered,
              &lowered_block.insts)) {
        return std::nullopt;
      }
    }

    if (const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator)) {
      bir::ReturnTerminator lowered_ret;
      if (ret->value_str.has_value()) {
        const c4c::codegen::lir::LirOperand ret_value(*ret->value_str);
        if (return_info->returned_via_sret) {
          if (ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
            return std::nullopt;
          }
          const auto aggregate_alias_it = aggregate_value_aliases.find(ret_value.str());
          if (aggregate_alias_it == aggregate_value_aliases.end()) {
            return std::nullopt;
          }
          const auto source_aggregate_it = local_aggregate_slots.find(aggregate_alias_it->second);
          if (source_aggregate_it == local_aggregate_slots.end()) {
            return std::nullopt;
          }
          if (!append_local_aggregate_copy_to_pointer(source_aggregate_it->second,
                                                      local_slot_types,
                                                      bir::Value::named(bir::TypeKind::Ptr,
                                                                        "%ret.sret"),
                                                      return_info->align_bytes,
                                                      function.name + ".ret.sret.copy",
                                                      &lowered_block.insts)) {
            return std::nullopt;
          }
        } else {
          const auto value = lower_value(ret_value, return_info->type, value_aliases);
          if (!value.has_value()) {
            return std::nullopt;
          }
          if (!canonicalize_compare_return_alias(ret_value,
                                                 *value,
                                                 return_info->type,
                                                 compare_exprs,
                                                 &lowered_block.insts,
                                                 &lowered_ret)) {
            lowered_ret.value = *value;
          }
        }
      }
      lowered_block.terminator = lowered_ret;
    } else if (const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&block.terminator)) {
      lowered_block.terminator = bir::BranchTerminator{.target_label = br->target_label};
    } else if (const auto* cond_br =
                   std::get_if<c4c::codegen::lir::LirCondBr>(&block.terminator)) {
      const auto condition = lower_value(cond_br->cond_name, bir::TypeKind::I1, value_aliases);
      if (!condition.has_value()) {
        return std::nullopt;
      }
      lowered_block.terminator = bir::CondBranchTerminator{
          .condition = *condition,
          .true_label = cond_br->true_label,
          .false_label = cond_br->false_label,
      };
    } else {
      return std::nullopt;
    }

    lowered.blocks.push_back(std::move(lowered_block));
  }

  return lowered;
}

}  // namespace

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
    auto lowered_decl = lower_extern_decl(decl);
    if (!lowered_decl.has_value()) {
      continue;
    }
    module.functions.push_back(std::move(*lowered_decl));
  }

  for (const auto& function : context.lir_module.functions) {
    if (function.is_declaration) {
      auto lowered_decl = lower_decl_function(function);
      if (!lowered_decl.has_value()) {
        continue;
      }
      module.functions.push_back(std::move(*lowered_decl));
      continue;
    }

    auto lowered_function = lower_branch_family_function(
        context, function, global_types, function_symbols, type_decls);
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
