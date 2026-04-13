#include "lir_to_bir.hpp"

#include "call_decode.hpp"

#include <charconv>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace c4c::backend {

namespace {

using ValueMap = std::unordered_map<std::string, bir::Value>;
using LocalSlotTypes = std::unordered_map<std::string, bir::TypeKind>;

struct CompareExpr {
  bir::BinaryOpcode opcode = bir::BinaryOpcode::Eq;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  bir::Value lhs;
  bir::Value rhs;
};

using CompareMap = std::unordered_map<std::string, CompareExpr>;
using BlockLookup = std::unordered_map<std::string, const c4c::codegen::lir::LirBlock*>;

struct BranchChain {
  std::vector<std::string> labels;
  std::string leaf_label;
  std::string join_label;
};

std::optional<std::int64_t> parse_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<bir::TypeKind> lower_integer_type(std::string_view text) {
  if (text == "i1") {
    return bir::TypeKind::I1;
  }
  if (text == "i8") {
    return bir::TypeKind::I8;
  }
  if (text == "i32") {
    return bir::TypeKind::I32;
  }
  if (text == "i64") {
    return bir::TypeKind::I64;
  }
  if (text == "void") {
    return bir::TypeKind::Void;
  }
  return std::nullopt;
}

std::optional<bir::BinaryOpcode> lower_cmp_predicate(
    const c4c::codegen::lir::LirCmpPredicateRef& predicate) {
  using c4c::codegen::lir::LirCmpPredicate;
  switch (predicate.typed().value_or(LirCmpPredicate::Ord)) {
    case LirCmpPredicate::Eq:
      return bir::BinaryOpcode::Eq;
    case LirCmpPredicate::Ne:
      return bir::BinaryOpcode::Ne;
    case LirCmpPredicate::Slt:
      return bir::BinaryOpcode::Slt;
    case LirCmpPredicate::Sle:
      return bir::BinaryOpcode::Sle;
    case LirCmpPredicate::Sgt:
      return bir::BinaryOpcode::Sgt;
    case LirCmpPredicate::Sge:
      return bir::BinaryOpcode::Sge;
    case LirCmpPredicate::Ult:
      return bir::BinaryOpcode::Ult;
    case LirCmpPredicate::Ule:
      return bir::BinaryOpcode::Ule;
    case LirCmpPredicate::Ugt:
      return bir::BinaryOpcode::Ugt;
    case LirCmpPredicate::Uge:
      return bir::BinaryOpcode::Uge;
    default:
      return std::nullopt;
  }
}

std::optional<bir::Value> lower_value(const c4c::codegen::lir::LirOperand& operand,
                                      bir::TypeKind expected_type,
                                      const ValueMap& bool_aliases) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    const auto alias = bool_aliases.find(operand.str());
    if (alias != bool_aliases.end()) {
      return alias->second;
    }
    return bir::Value::named(expected_type, operand.str());
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::Immediate &&
      operand.kind() != c4c::codegen::lir::LirOperandKind::SpecialToken) {
    return std::nullopt;
  }

  if (expected_type == bir::TypeKind::I1) {
    if (operand == "true") {
      return bir::Value::immediate_i1(true);
    }
    if (operand == "false") {
      return bir::Value::immediate_i1(false);
    }
  }

  const auto parsed = parse_i64(operand.str());
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  switch (expected_type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(*parsed != 0);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(*parsed));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(*parsed));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(*parsed);
    default:
      return std::nullopt;
  }
}

std::optional<bir::TypeKind> lower_param_type(const c4c::TypeSpec& type) {
  if (type.base == TB_BOOL && type.ptr_level == 0 && type.array_rank == 0) {
    return bir::TypeKind::I1;
  }
  return backend_lir_lower_minimal_scalar_type(type);
}

std::optional<bir::TypeKind> infer_function_return_type(
    const c4c::codegen::lir::LirFunction& function) {
  std::optional<bir::TypeKind> return_type;
  for (const auto& block : function.blocks) {
    const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator);
    if (ret == nullptr) {
      continue;
    }
    const auto lowered_type = lower_integer_type(ret->type_str);
    if (!lowered_type.has_value()) {
      return std::nullopt;
    }
    if (!return_type.has_value()) {
      return_type = lowered_type;
      continue;
    }
    if (*return_type != *lowered_type) {
      return std::nullopt;
    }
  }
  return return_type;
}

bool lower_function_params(const c4c::codegen::lir::LirFunction& function,
                           bir::Function* lowered) {
  for (const auto& param : function.params) {
    const auto lowered_type = lower_param_type(param.second);
    if (!lowered_type.has_value()) {
      return false;
    }
    lowered->params.push_back(bir::Param{
        .type = *lowered_type,
        .name = param.first,
    });
  }

  if (!lowered->params.empty()) {
    return true;
  }

  const auto parsed_params = parse_backend_function_signature_params(function.signature_text);
  if (!parsed_params.has_value()) {
    return true;
  }

  for (const auto& param : *parsed_params) {
    if (param.is_varargs) {
      return false;
    }
    const auto lowered_type = lower_integer_type(param.type);
    if (!lowered_type.has_value() || param.operand.empty()) {
      return false;
    }
    lowered->params.push_back(bir::Param{
        .type = *lowered_type,
        .name = param.operand,
    });
  }
  return true;
}

bool lower_scalar_compare_inst(const c4c::codegen::lir::LirInst& inst,
                               ValueMap& bool_aliases,
                               CompareMap& compare_exprs,
                               std::vector<bir::Inst>* lowered_insts) {
  if (const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&inst)) {
    if (cmp->is_float) {
      return false;
    }
    const auto operand_type = lower_integer_type(cmp->type_str.str());
    const auto opcode = lower_cmp_predicate(cmp->predicate);
    if (!operand_type.has_value() || !opcode.has_value()) {
      return false;
    }
    const auto lhs = lower_value(cmp->lhs, *operand_type, bool_aliases);
    const auto rhs = lower_value(cmp->rhs, *operand_type, bool_aliases);
    if (!lhs.has_value() || !rhs.has_value()) {
      return false;
    }

    if (*opcode == bir::BinaryOpcode::Ne && *operand_type == bir::TypeKind::I32 &&
        cmp->lhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        cmp->rhs.kind() == c4c::codegen::lir::LirOperandKind::Immediate) {
      const auto alias = bool_aliases.find(cmp->lhs.str());
      const auto compare_alias = compare_exprs.find(cmp->lhs.str());
      const auto imm = parse_i64(cmp->rhs.str());
      if (alias != bool_aliases.end() && compare_alias != compare_exprs.end() &&
          imm.has_value() && *imm == 0) {
        bool_aliases[cmp->result.str()] = alias->second;
        compare_exprs[cmp->result.str()] = compare_alias->second;
        return true;
      }
    }

    const auto result = bir::Value::named(bir::TypeKind::I1, cmp->result.str());
    const CompareExpr expr{
        .opcode = *opcode,
        .operand_type = *operand_type,
        .lhs = *lhs,
        .rhs = *rhs,
    };
    bool_aliases[cmp->result.str()] = result;
    compare_exprs[cmp->result.str()] = expr;

    if (lowered_insts != nullptr) {
      lowered_insts->push_back(bir::BinaryInst{
          .opcode = *opcode,
          .result = result,
          .operand_type = *operand_type,
          .lhs = *lhs,
          .rhs = *rhs,
      });
    }
    return true;
  }

  if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
    const auto from_type = lower_integer_type(cast->from_type.str());
    const auto to_type = lower_integer_type(cast->to_type.str());
    if (cast->kind == c4c::codegen::lir::LirCastKind::ZExt &&
        from_type == bir::TypeKind::I1 && to_type == bir::TypeKind::I32 &&
        cast->operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto alias = bool_aliases.find(cast->operand.str());
      if (alias != bool_aliases.end()) {
        bool_aliases[cast->result.str()] = alias->second;
        const auto compare_alias = compare_exprs.find(cast->operand.str());
        if (compare_alias != compare_exprs.end()) {
          compare_exprs[cast->result.str()] = compare_alias->second;
        }
        return true;
      }
    }
  }

  return false;
}

bool lower_scalar_or_local_memory_inst(const c4c::codegen::lir::LirInst& inst,
                                       ValueMap& bool_aliases,
                                       CompareMap& compare_exprs,
                                       LocalSlotTypes& local_slot_types,
                                       bir::Function* lowered_function,
                                       std::vector<bir::Inst>* lowered_insts) {
  if (lower_scalar_compare_inst(inst, bool_aliases, compare_exprs, lowered_insts)) {
    return true;
  }

  if (const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst)) {
    if (alloca->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        !alloca->count.str().empty()) {
      return false;
    }

    const auto slot_type = lower_integer_type(alloca->type_str.str());
    if (!slot_type.has_value()) {
      return false;
    }

    const std::string slot_name = alloca->result.str();
    if (local_slot_types.find(slot_name) != local_slot_types.end()) {
      return false;
    }

    local_slot_types.emplace(slot_name, *slot_type);
    lowered_function->local_slots.push_back(bir::LocalSlot{
        .name = slot_name,
        .type = *slot_type,
        .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
    });
    return true;
  }

  if (const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst)) {
    if (store->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto value_type = lower_integer_type(store->type_str.str());
    if (!value_type.has_value()) {
      return false;
    }

    const auto slot_it = local_slot_types.find(store->ptr.str());
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return false;
    }

    const auto value = lower_value(store->val, *value_type, bool_aliases);
    if (!value.has_value()) {
      return false;
    }

    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = store->ptr.str(),
        .value = *value,
    });
    return true;
  }

  if (const auto* load = std::get_if<c4c::codegen::lir::LirLoadOp>(&inst)) {
    if (load->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        load->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto value_type = lower_integer_type(load->type_str.str());
    if (!value_type.has_value()) {
      return false;
    }

    const auto slot_it = local_slot_types.find(load->ptr.str());
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return false;
    }

    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(*value_type, load->result.str()),
        .slot_name = load->ptr.str(),
    });
    return true;
  }

  return false;
}

BlockLookup make_block_lookup(const c4c::codegen::lir::LirFunction& function) {
  BlockLookup blocks;
  for (const auto& block : function.blocks) {
    blocks.emplace(block.label, &block);
  }
  return blocks;
}

std::optional<BranchChain> follow_empty_branch_chain(const BlockLookup& blocks,
                                                     const std::string& start_label) {
  std::unordered_set<std::string> seen;
  const auto* current = [&]() -> const c4c::codegen::lir::LirBlock* {
    const auto it = blocks.find(start_label);
    return it == blocks.end() ? nullptr : it->second;
  }();
  if (current == nullptr) {
    return std::nullopt;
  }

  BranchChain chain;
  while (current != nullptr) {
    if (!seen.emplace(current->label).second || !current->insts.empty()) {
      return std::nullopt;
    }

    const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&current->terminator);
    if (br == nullptr) {
      return std::nullopt;
    }

    chain.labels.push_back(current->label);
    const auto next_it = blocks.find(br->target_label);
    if (next_it == blocks.end()) {
      return std::nullopt;
    }

    const auto* next = next_it->second;
    if (!next->insts.empty() ||
        !std::holds_alternative<c4c::codegen::lir::LirBr>(next->terminator)) {
      chain.leaf_label = current->label;
      chain.join_label = br->target_label;
      return chain;
    }

    current = next;
  }

  return std::nullopt;
}

std::optional<bir::Function> lower_select_family_function(
    BirLoweringContext& context,
    const c4c::codegen::lir::LirFunction& function) {
  (void)context;
  if (function.is_declaration || function.blocks.empty()) {
    return std::nullopt;
  }

  const auto return_type = infer_function_return_type(function);
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* cond_br = std::get_if<c4c::codegen::lir::LirCondBr>(&entry.terminator);
  if (cond_br == nullptr) {
    return std::nullopt;
  }

  ValueMap bool_aliases;
  CompareMap compare_exprs;
  for (const auto& inst : entry.insts) {
    if (!lower_scalar_compare_inst(inst, bool_aliases, compare_exprs, nullptr)) {
      return std::nullopt;
    }
  }

  const auto compare_it = compare_exprs.find(cond_br->cond_name);
  if (compare_it == compare_exprs.end()) {
    return std::nullopt;
  }

  const auto blocks = make_block_lookup(function);
  const auto true_chain = follow_empty_branch_chain(blocks, cond_br->true_label);
  const auto false_chain = follow_empty_branch_chain(blocks, cond_br->false_label);
  if (!true_chain.has_value() || !false_chain.has_value() ||
      true_chain->join_label != false_chain->join_label) {
    return std::nullopt;
  }

  const auto join_it = blocks.find(true_chain->join_label);
  if (join_it == blocks.end()) {
    return std::nullopt;
  }
  const auto& join_block = *join_it->second;

  if (join_block.insts.size() != 1) {
    return std::nullopt;
  }
  const auto* phi = std::get_if<c4c::codegen::lir::LirPhiOp>(&join_block.insts.front());
  if (phi == nullptr || phi->incoming.size() != 2) {
    return std::nullopt;
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
  if (ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      ret_value.str() != phi->result.str()) {
    return std::nullopt;
  }

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

  const auto true_value = lower_value(*true_incoming, *phi_type, bool_aliases);
  const auto false_value = lower_value(*false_incoming, *phi_type, bool_aliases);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = *return_type;
  if (!lower_function_params(function, &lowered)) {
    return std::nullopt;
  }

  bir::Block lowered_block;
  lowered_block.label = entry.label;
  lowered_block.insts.push_back(bir::SelectInst{
      .predicate = compare_it->second.opcode,
      .result = bir::Value::named(*phi_type, phi->result.str()),
      .compare_type = compare_it->second.operand_type,
      .lhs = compare_it->second.lhs,
      .rhs = compare_it->second.rhs,
      .true_value = *true_value,
      .false_value = *false_value,
  });
  lowered_block.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(*phi_type, phi->result.str()),
  };
  lowered.blocks.push_back(std::move(lowered_block));
  return lowered;
}

std::optional<bir::Function> lower_branch_family_function(
    BirLoweringContext& context,
    const c4c::codegen::lir::LirFunction& function) {
  (void)context;
  if (function.is_declaration || function.blocks.empty()) {
    return std::nullopt;
  }

  const auto return_type = infer_function_return_type(function);
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = *return_type;
  if (!lower_function_params(function, &lowered)) {
    return std::nullopt;
  }

  ValueMap bool_aliases;
  CompareMap compare_exprs;
  LocalSlotTypes local_slot_types;
  std::vector<bir::Inst> hoisted_alloca_scratch;

  for (const auto& inst : function.alloca_insts) {
    if (!lower_scalar_or_local_memory_inst(
            inst, bool_aliases, compare_exprs, local_slot_types, &lowered, &hoisted_alloca_scratch)) {
      return std::nullopt;
    }
  }
  if (!hoisted_alloca_scratch.empty()) {
    return std::nullopt;
  }

  for (const auto& block : function.blocks) {
    bir::Block lowered_block;
    lowered_block.label = block.label;

    for (const auto& inst : block.insts) {
      if (!lower_scalar_or_local_memory_inst(
              inst, bool_aliases, compare_exprs, local_slot_types, &lowered, &lowered_block.insts)) {
        return std::nullopt;
      }
    }

    if (const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator)) {
      bir::ReturnTerminator lowered_ret;
      if (ret->value_str.has_value()) {
        const auto value = lower_value(*ret->value_str, *return_type, bool_aliases);
        if (!value.has_value()) {
          return std::nullopt;
        }
        lowered_ret.value = *value;
      }
      lowered_block.terminator = lowered_ret;
    } else if (const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&block.terminator)) {
      lowered_block.terminator = bir::BranchTerminator{.target_label = br->target_label};
    } else if (const auto* cond_br =
                   std::get_if<c4c::codegen::lir::LirCondBr>(&block.terminator)) {
      const auto condition = lower_value(cond_br->cond_name, bir::TypeKind::I1, bool_aliases);
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
        "bootstrap lir_to_bir only supports function-only modules without globals/strings/externs");
    return std::nullopt;
  }

  for (const auto& function : context.lir_module.functions) {
    auto lowered_function = lower_select_family_function(context, function);
    if (!lowered_function.has_value()) {
      lowered_function = lower_branch_family_function(context, function);
    }
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
      "lowered function-only scalar compare/select/local-memory/return module to BIR");
  return module;
}

}  // namespace c4c::backend
