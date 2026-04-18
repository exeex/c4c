#include "lir_to_bir.hpp"

#include <charconv>

namespace c4c::backend {

using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::parse_i64;

std::optional<unsigned> BirFunctionLowerer::integer_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return 1u;
    case bir::TypeKind::I8:
      return 8u;
    case bir::TypeKind::I16:
      return 16u;
    case bir::TypeKind::I32:
      return 32u;
    case bir::TypeKind::I64:
      return 64u;
    default:
      return std::nullopt;
  }
}

std::uint64_t BirFunctionLowerer::integer_bit_mask(unsigned bits) {
  return bits >= 64u ? ~std::uint64_t{0} : ((std::uint64_t{1} << bits) - 1u);
}

std::int64_t BirFunctionLowerer::sign_extend_bits(std::uint64_t value, unsigned bits) {
  if (bits == 0u || bits >= 64u) {
    return static_cast<std::int64_t>(value);
  }
  const auto sign_bit = std::uint64_t{1} << (bits - 1u);
  const auto extended = (value ^ sign_bit) - sign_bit;
  return static_cast<std::int64_t>(extended);
}

std::optional<bir::Value> BirFunctionLowerer::make_integer_immediate(bir::TypeKind type,
                                                                     std::int64_t value) {
  switch (type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(value != 0);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value));
    case bir::TypeKind::I16:
      return bir::Value::immediate_i16(static_cast<std::int16_t>(value));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(value));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(value);
    default:
      return std::nullopt;
  }
}

bool BirFunctionLowerer::is_canonical_select_chain_binop(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
      return true;
    default:
      return false;
  }
}

std::optional<bir::TypeKind> BirFunctionLowerer::lower_scalar_or_function_pointer_type(
    std::string_view text) {
  const auto lowered = lower_integer_type(text);
  if (lowered.has_value()) {
    return lowered;
  }
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed == "float") {
    return bir::TypeKind::F32;
  }
  if (trimmed == "double") {
    return bir::TypeKind::F64;
  }
  if (trimmed == "x86_fp80" || trimmed == "f128") {
    return bir::TypeKind::F128;
  }
  if (trimmed == "ptr" || trimmed.rfind("ptr ", 0) == 0) {
    return bir::TypeKind::Ptr;
  }
  return std::nullopt;
}

std::optional<bir::CastOpcode> BirFunctionLowerer::lower_cast_opcode(
    c4c::codegen::lir::LirCastKind kind) {
  switch (kind) {
    case c4c::codegen::lir::LirCastKind::SExt:
      return bir::CastOpcode::SExt;
    case c4c::codegen::lir::LirCastKind::ZExt:
      return bir::CastOpcode::ZExt;
    case c4c::codegen::lir::LirCastKind::Trunc:
      return bir::CastOpcode::Trunc;
    default:
      return std::nullopt;
  }
}

std::optional<bir::BinaryOpcode> BirFunctionLowerer::lower_scalar_binary_opcode(
    const c4c::codegen::lir::LirBinaryOpcodeRef& opcode) {
  using c4c::codegen::lir::LirBinaryOpcode;
  switch (opcode.typed().value_or(LirBinaryOpcode::FNeg)) {
    case LirBinaryOpcode::Add:
      return bir::BinaryOpcode::Add;
    case LirBinaryOpcode::Sub:
      return bir::BinaryOpcode::Sub;
    case LirBinaryOpcode::Mul:
      return bir::BinaryOpcode::Mul;
    case LirBinaryOpcode::SDiv:
      return bir::BinaryOpcode::SDiv;
    case LirBinaryOpcode::UDiv:
      return bir::BinaryOpcode::UDiv;
    case LirBinaryOpcode::SRem:
      return bir::BinaryOpcode::SRem;
    case LirBinaryOpcode::URem:
      return bir::BinaryOpcode::URem;
    case LirBinaryOpcode::And:
      return bir::BinaryOpcode::And;
    case LirBinaryOpcode::Or:
      return bir::BinaryOpcode::Or;
    case LirBinaryOpcode::Xor:
      return bir::BinaryOpcode::Xor;
    case LirBinaryOpcode::Shl:
      return bir::BinaryOpcode::Shl;
    case LirBinaryOpcode::LShr:
      return bir::BinaryOpcode::LShr;
    case LirBinaryOpcode::AShr:
      return bir::BinaryOpcode::AShr;
    default:
      return std::nullopt;
  }
}

std::optional<bir::Value> BirFunctionLowerer::fold_i64_binary_immediates(
    bir::BinaryOpcode opcode,
    std::int64_t lhs,
    std::int64_t rhs) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return bir::Value::immediate_i64(lhs + rhs);
    case bir::BinaryOpcode::Sub:
      return bir::Value::immediate_i64(lhs - rhs);
    case bir::BinaryOpcode::Mul:
      return bir::Value::immediate_i64(lhs * rhs);
    case bir::BinaryOpcode::SDiv:
      if (rhs == 0) {
        return std::nullopt;
      }
      return bir::Value::immediate_i64(lhs / rhs);
    case bir::BinaryOpcode::And:
      return bir::Value::immediate_i64(lhs & rhs);
    case bir::BinaryOpcode::Or:
      return bir::Value::immediate_i64(lhs | rhs);
    case bir::BinaryOpcode::Xor:
      return bir::Value::immediate_i64(lhs ^ rhs);
    case bir::BinaryOpcode::Shl:
      return bir::Value::immediate_i64(lhs << rhs);
    case bir::BinaryOpcode::LShr:
      return bir::Value::immediate_i64(
          static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) >> rhs));
    case bir::BinaryOpcode::AShr:
      return bir::Value::immediate_i64(lhs >> rhs);
    default:
      return std::nullopt;
  }
}

std::optional<bir::BinaryOpcode> BirFunctionLowerer::lower_cmp_predicate(
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

std::optional<bir::Value> BirFunctionLowerer::lower_value(
    const c4c::codegen::lir::LirOperand& operand,
    bir::TypeKind expected_type,
    const ValueMap& value_aliases) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    const auto alias = value_aliases.find(operand.str());
    if (alias != value_aliases.end()) {
      return alias->second;
    }
    return bir::Value::named(expected_type, operand.str());
  }

  if (expected_type == bir::TypeKind::I1) {
    if (operand == "true") {
      return bir::Value::immediate_i1(true);
    }
    if (operand == "false") {
      return bir::Value::immediate_i1(false);
    }
  }

  if (expected_type == bir::TypeKind::Ptr && operand == "null") {
    return bir::Value{
        .kind = bir::Value::Kind::Immediate,
        .type = bir::TypeKind::Ptr,
        .immediate = 0,
        .immediate_bits = 0,
    };
  }

  const auto try_parse_fp_bits = [&](bir::TypeKind float_type) -> std::optional<bir::Value> {
    const auto text = operand.str();
    if (text.size() < 3 || text[0] != '0' || (text[1] != 'x' && text[1] != 'X')) {
      return std::nullopt;
    }
    std::uint64_t bits = 0;
    const auto* begin = text.data() + 2;
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, bits, 16);
    if (result.ec != std::errc() || result.ptr != end) {
      return std::nullopt;
    }
    if (float_type == bir::TypeKind::F32) {
      return bir::Value::immediate_f32_bits(static_cast<std::uint32_t>(bits));
    }
    return bir::Value::immediate_f64_bits(bits);
  };
  if (expected_type == bir::TypeKind::F32 || expected_type == bir::TypeKind::F64) {
    return try_parse_fp_bits(expected_type);
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::Immediate &&
      operand.kind() != c4c::codegen::lir::LirOperandKind::SpecialToken) {
    return std::nullopt;
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

std::optional<bir::Value> BirFunctionLowerer::lower_value(
    const c4c::codegen::lir::LirOperand& operand,
    bir::TypeKind expected_type) const {
  return lower_value(operand, expected_type, value_aliases_);
}

std::optional<bir::Value> BirFunctionLowerer::fold_integer_cast(
    c4c::codegen::lir::LirCastKind kind,
    const bir::Value& operand,
    bir::TypeKind to_type) {
  const auto from_bits = integer_type_bit_width(operand.type);
  const auto to_bits = integer_type_bit_width(to_type);
  if (!from_bits.has_value() || !to_bits.has_value()) {
    return std::nullopt;
  }
  if (operand.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }

  const auto masked_input =
      static_cast<std::uint64_t>(operand.immediate) & integer_bit_mask(*from_bits);
  std::int64_t cast_value = 0;
  switch (kind) {
    case c4c::codegen::lir::LirCastKind::Trunc:
      cast_value = sign_extend_bits(masked_input & integer_bit_mask(*to_bits), *to_bits);
      break;
    case c4c::codegen::lir::LirCastKind::ZExt:
      cast_value = static_cast<std::int64_t>(masked_input);
      break;
    case c4c::codegen::lir::LirCastKind::SExt:
      cast_value = sign_extend_bits(masked_input, *from_bits);
      break;
    default:
      return std::nullopt;
  }

  return make_integer_immediate(to_type, cast_value);
}

bool BirFunctionLowerer::lower_scalar_compare_inst(const c4c::codegen::lir::LirInst& inst,
                                                   ValueMap& value_aliases,
                                                   CompareMap& compare_exprs,
                                                   std::vector<bir::Inst>* lowered_insts) const {
  if (const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&inst)) {
    if (cmp->is_float) {
      return false;
    }
    const auto operand_type = lower_integer_type(cmp->type_str.str());
    const auto opcode = lower_cmp_predicate(cmp->predicate);
    if (!operand_type.has_value() || !opcode.has_value()) {
      return false;
    }
    const auto lhs = lower_value(cmp->lhs, *operand_type, value_aliases);
    const auto rhs = lower_value(cmp->rhs, *operand_type, value_aliases);
    if (!lhs.has_value() || !rhs.has_value()) {
      return false;
    }

    if (*opcode == bir::BinaryOpcode::Ne && *operand_type == bir::TypeKind::I32 &&
        cmp->lhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        cmp->rhs.kind() == c4c::codegen::lir::LirOperandKind::Immediate) {
      const auto alias = value_aliases.find(cmp->lhs.str());
      const auto compare_alias = compare_exprs.find(cmp->lhs.str());
      const auto imm = parse_i64(cmp->rhs.str());
      if (alias != value_aliases.end() && compare_alias != compare_exprs.end() &&
          imm.has_value() && *imm == 0) {
        value_aliases[cmp->result.str()] = alias->second;
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
    value_aliases[cmp->result.str()] = result;
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
      const auto alias = value_aliases.find(cast->operand.str());
      if (alias != value_aliases.end()) {
        value_aliases[cast->result.str()] = alias->second;
        const auto compare_alias = compare_exprs.find(cast->operand.str());
        if (compare_alias != compare_exprs.end()) {
          compare_exprs[cast->result.str()] = compare_alias->second;
        }
        return true;
      }
    }

    if ((cast->kind == c4c::codegen::lir::LirCastKind::SExt ||
         cast->kind == c4c::codegen::lir::LirCastKind::ZExt) &&
        cast->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        from_type.has_value() && to_type.has_value()) {
      const auto value = lower_value(cast->operand, *from_type, value_aliases);
      if (!value.has_value()) {
        return false;
      }

      if (value->kind == bir::Value::Kind::Immediate) {
        const auto imm = value->immediate;
        switch (*to_type) {
          case bir::TypeKind::I32:
            value_aliases[cast->result.str()] =
                bir::Value::immediate_i32(static_cast<std::int32_t>(imm));
            return true;
          case bir::TypeKind::I64:
            value_aliases[cast->result.str()] = bir::Value::immediate_i64(imm);
            return true;
          default:
            break;
        }
      }
    }
  }

  return false;
}

bool BirFunctionLowerer::resolve_select_chain_inst(const c4c::codegen::lir::LirInst& inst,
                                                   ValueMap& value_aliases,
                                                   CompareMap& compare_exprs,
                                                   std::vector<bir::Inst>* lowered_insts) const {
  if (lower_scalar_compare_inst(inst, value_aliases, compare_exprs, lowered_insts)) {
    return true;
  }

  if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
    if (cast->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto from_type = lower_integer_type(cast->from_type.str());
    const auto to_type = lower_integer_type(cast->to_type.str());
    if (!from_type.has_value() || !to_type.has_value()) {
      return false;
    }

    const auto operand = lower_value(cast->operand, *from_type, value_aliases);
    if (!operand.has_value()) {
      return false;
    }

    if (operand->kind == bir::Value::Kind::Named && operand->type == *to_type &&
        cast->kind == c4c::codegen::lir::LirCastKind::Bitcast) {
      value_aliases[cast->result.str()] = *operand;
      return true;
    }

    if (const auto folded = fold_integer_cast(cast->kind, *operand, *to_type);
        folded.has_value()) {
      value_aliases[cast->result.str()] = *folded;
      return true;
    }

    const auto opcode = lower_cast_opcode(cast->kind);
    if (!opcode.has_value() || lowered_insts == nullptr) {
      return false;
    }
    lowered_insts->push_back(bir::CastInst{
        .opcode = *opcode,
        .result = bir::Value::named(*to_type, cast->result.str()),
        .operand = *operand,
    });
    value_aliases[cast->result.str()] = bir::Value::named(*to_type, cast->result.str());
    return true;
  }

  if (const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst)) {
    if (bin->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto opcode = lower_scalar_binary_opcode(bin->opcode);
    const auto value_type = lower_integer_type(bin->type_str.str());
    if (!opcode.has_value() || !value_type.has_value() ||
        !is_canonical_select_chain_binop(*opcode)) {
      return false;
    }

    const auto lhs = lower_value(bin->lhs, *value_type, value_aliases);
    const auto rhs = lower_value(bin->rhs, *value_type, value_aliases);
    if (!lhs.has_value() || !rhs.has_value()) {
      return false;
    }

    if (*value_type == bir::TypeKind::I64 && lhs->kind == bir::Value::Kind::Immediate &&
        rhs->kind == bir::Value::Kind::Immediate) {
      const auto folded = fold_i64_binary_immediates(*opcode, lhs->immediate, rhs->immediate);
      if (folded.has_value()) {
        value_aliases[bin->result.str()] = *folded;
        return true;
      }
    }

    if (lowered_insts == nullptr) {
      return false;
    }
    lowered_insts->push_back(bir::BinaryInst{
        .opcode = *opcode,
        .result = bir::Value::named(*value_type, bin->result.str()),
        .operand_type = *value_type,
        .lhs = *lhs,
        .rhs = *rhs,
    });
    value_aliases[bin->result.str()] = bir::Value::named(*value_type, bin->result.str());
    return true;
  }

  return false;
}

bool BirFunctionLowerer::lower_canonical_select_entry_inst(
    const c4c::codegen::lir::LirInst& inst,
    ValueMap& value_aliases,
    CompareMap& compare_exprs,
    std::vector<bir::Inst>* lowered_insts) const {
  if (lower_scalar_compare_inst(inst, value_aliases, compare_exprs, nullptr)) {
    return true;
  }

  const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst);
  if (cast == nullptr || cast->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return false;
  }

  const auto opcode = lower_cast_opcode(cast->kind);
  const auto from_type = lower_integer_type(cast->from_type.str());
  const auto to_type = lower_integer_type(cast->to_type.str());
  if (!opcode.has_value() || !from_type.has_value() || !to_type.has_value()) {
    return false;
  }

  const auto operand = lower_value(cast->operand, *from_type, value_aliases);
  if (!operand.has_value()) {
    return false;
  }

  if (const auto folded = fold_integer_cast(cast->kind, *operand, *to_type); folded.has_value()) {
    value_aliases[cast->result.str()] = *folded;
    return true;
  }

  lowered_insts->push_back(bir::CastInst{
      .opcode = *opcode,
      .result = bir::Value::named(*to_type, cast->result.str()),
      .operand = *operand,
  });
  return true;
}

}  // namespace c4c::backend
