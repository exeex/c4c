#include "lir_to_bir.hpp"

#include <charconv>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace c4c::backend {

namespace {

using ValueMap = std::unordered_map<std::string, bir::Value>;

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

std::optional<bir::Function> lower_branch_family_function(BirLoweringContext& context,
                                                          const c4c::codegen::lir::LirFunction& function) {
  if (function.is_declaration || !function.params.empty() || function.blocks.empty()) {
    return std::nullopt;
  }

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
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = *return_type;

  ValueMap bool_aliases;

  for (const auto& block : function.blocks) {
    bir::Block lowered_block;
    lowered_block.label = block.label;

    for (const auto& inst : block.insts) {
      if (const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&inst)) {
        if (cmp->is_float) {
          return std::nullopt;
        }
        const auto operand_type = lower_integer_type(cmp->type_str.str());
        const auto opcode = lower_cmp_predicate(cmp->predicate);
        if (!operand_type.has_value() || !opcode.has_value()) {
          return std::nullopt;
        }
        const auto lhs = lower_value(cmp->lhs, *operand_type, bool_aliases);
        const auto rhs = lower_value(cmp->rhs, *operand_type, bool_aliases);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }

        // Canonicalize `icmp ne (zext i1 %x to i32), 0` back to `%x`.
        if (*opcode == bir::BinaryOpcode::Ne && *operand_type == bir::TypeKind::I32 &&
            cmp->lhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
            cmp->rhs.kind() == c4c::codegen::lir::LirOperandKind::Immediate) {
          const auto alias = bool_aliases.find(cmp->lhs.str());
          const auto imm = parse_i64(cmp->rhs.str());
          if (alias != bool_aliases.end() && imm.has_value() && *imm == 0) {
            bool_aliases.emplace(cmp->result.str(), alias->second);
            continue;
          }
        }

        bir::BinaryInst lowered_cmp;
        lowered_cmp.opcode = *opcode;
        lowered_cmp.result = bir::Value::named(bir::TypeKind::I1, cmp->result.str());
        lowered_cmp.operand_type = *operand_type;
        lowered_cmp.lhs = *lhs;
        lowered_cmp.rhs = *rhs;
        bool_aliases[cmp->result.str()] = lowered_cmp.result;
        lowered_block.insts.push_back(std::move(lowered_cmp));
        continue;
      }

      if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
        const auto from_type = lower_integer_type(cast->from_type.str());
        const auto to_type = lower_integer_type(cast->to_type.str());
        if (cast->kind == c4c::codegen::lir::LirCastKind::ZExt &&
            from_type == bir::TypeKind::I1 && to_type == bir::TypeKind::I32 &&
            cast->operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
          const auto alias = bool_aliases.find(cast->operand.str());
          if (alias != bool_aliases.end()) {
            bool_aliases.emplace(cast->result.str(), alias->second);
            continue;
          }
        }
        return std::nullopt;
      }

      return std::nullopt;
    }

    if (const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator)) {
      bir::ReturnTerminator lowered_ret;
      if (ret->value_str.has_value()) {
        const auto value =
            lower_value(*ret->value_str, *return_type, bool_aliases);
        if (!value.has_value()) {
          return std::nullopt;
        }
        lowered_ret.value = *value;
      }
      lowered_block.terminator = lowered_ret;
    } else if (const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&block.terminator)) {
      lowered_block.terminator = bir::BranchTerminator{.target_label = br->target_label};
    } else if (const auto* cond_br = std::get_if<c4c::codegen::lir::LirCondBr>(&block.terminator)) {
      const auto condition =
          lower_value(cond_br->cond_name, bir::TypeKind::I1, bool_aliases);
      if (!condition.has_value() || condition->type != bir::TypeKind::I1) {
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
    context.note("module",
                 "bootstrap lir_to_bir only supports function-only modules without globals/strings/externs");
    return std::nullopt;
  }

  for (const auto& function : context.lir_module.functions) {
    auto lowered_function = lower_branch_family_function(context, function);
    if (!lowered_function.has_value()) {
      context.note("module",
                   "bootstrap lir_to_bir only supports compare/branch/return scalar functions right now");
      return std::nullopt;
    }
    module.functions.push_back(std::move(*lowered_function));
  }

  context.note("module",
               "lowered function-only scalar compare/branch/return module to BIR");
  return module;
}

}  // namespace c4c::backend
