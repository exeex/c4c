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
using LocalPointerSlots = std::unordered_map<std::string, std::string>;

struct GlobalInfo {
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t element_size_bytes = 0;
  std::size_t element_count = 0;
  bool supports_direct_value = false;
  bool supports_linear_addressing = false;
  std::string type_text;
};

using GlobalTypes = std::unordered_map<std::string, GlobalInfo>;

struct GlobalAddress {
  std::string global_name;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t byte_offset = 0;
};

using GlobalPointerMap = std::unordered_map<std::string, GlobalAddress>;

struct LocalArraySlots {
  bir::TypeKind element_type = bir::TypeKind::Void;
  std::vector<std::string> element_slots;
};

using LocalArraySlotMap = std::unordered_map<std::string, LocalArraySlots>;

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

struct ParsedTypedOperand {
  std::string type_text;
  c4c::codegen::lir::LirOperand operand;
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

std::optional<std::pair<std::size_t, bir::TypeKind>> parse_local_array_type(std::string_view text) {
  if (text.size() < 6 || text.front() != '[' || text.back() != ']') {
    return std::nullopt;
  }

  const auto x_pos = text.find(" x ");
  if (x_pos == std::string_view::npos || x_pos <= 1) {
    return std::nullopt;
  }

  const auto count = parse_i64(text.substr(1, x_pos - 1));
  if (!count.has_value() || *count <= 0) {
    return std::nullopt;
  }

  const auto element_type = lower_integer_type(text.substr(x_pos + 3, text.size() - x_pos - 4));
  if (!element_type.has_value()) {
    return std::nullopt;
  }

  return std::pair<std::size_t, bir::TypeKind>{static_cast<std::size_t>(*count), *element_type};
}

struct IntegerArrayType {
  std::vector<std::size_t> extents;
  bir::TypeKind element_type = bir::TypeKind::Void;
};

std::optional<IntegerArrayType> parse_integer_array_type(std::string_view text) {
  IntegerArrayType lowered;
  std::string_view remainder = text;
  while (true) {
    if (const auto scalar_type = lower_integer_type(remainder); scalar_type.has_value()) {
      if (lowered.extents.empty()) {
        return std::nullopt;
      }
      lowered.element_type = *scalar_type;
      return lowered;
    }

    if (remainder.size() < 6 || remainder.front() != '[' || remainder.back() != ']') {
      return std::nullopt;
    }

    const auto x_pos = remainder.find(" x ");
    if (x_pos == std::string_view::npos || x_pos <= 1) {
      return std::nullopt;
    }

    const auto count = parse_i64(remainder.substr(1, x_pos - 1));
    if (!count.has_value() || *count <= 0) {
      return std::nullopt;
    }

    lowered.extents.push_back(static_cast<std::size_t>(*count));
    remainder = remainder.substr(x_pos + 3, remainder.size() - x_pos - 4);
  }
}

std::optional<std::string_view> peel_integer_array_layer(std::string_view text) {
  if (text.size() < 6 || text.front() != '[' || text.back() != ']') {
    return std::nullopt;
  }

  const auto x_pos = text.find(" x ");
  if (x_pos == std::string_view::npos || x_pos <= 1) {
    return std::nullopt;
  }

  const auto count = parse_i64(text.substr(1, x_pos - 1));
  if (!count.has_value() || *count <= 0) {
    return std::nullopt;
  }

  return text.substr(x_pos + 3, text.size() - x_pos - 4);
}

std::size_t type_size_bytes_from_text(std::string_view text) {
  if (const auto scalar_type = lower_integer_type(text); scalar_type.has_value()) {
    return type_size_bytes(*scalar_type);
  }

  const auto array_type = parse_integer_array_type(text);
  if (!array_type.has_value()) {
    return 0;
  }

  const auto element_size_bytes = type_size_bytes(array_type->element_type);
  if (element_size_bytes == 0) {
    return 0;
  }

  std::size_t total_elements = 1;
  for (const auto extent : array_type->extents) {
    total_elements *= extent;
  }
  return total_elements * element_size_bytes;
}

std::optional<ParsedTypedOperand> parse_typed_operand(std::string_view text) {
  const auto space = text.find(' ');
  if (space == std::string_view::npos || space == 0 || space + 1 >= text.size()) {
    return std::nullopt;
  }
  return ParsedTypedOperand{
      .type_text = std::string(text.substr(0, space)),
      .operand = c4c::codegen::lir::LirOperand(std::string(text.substr(space + 1))),
  };
}

std::optional<std::int64_t> resolve_index_operand(const c4c::codegen::lir::LirOperand& operand,
                                                  const ValueMap& value_aliases) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::Immediate ||
      operand.kind() == c4c::codegen::lir::LirOperandKind::SpecialToken) {
    return parse_i64(operand.str());
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return std::nullopt;
  }

  const auto alias = value_aliases.find(operand.str());
  if (alias == value_aliases.end() || alias->second.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  return alias->second.immediate;
}

std::optional<bir::BinaryOpcode> lower_scalar_binary_opcode(
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
                                      const ValueMap& value_aliases) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    const auto alias = value_aliases.find(operand.str());
    if (alias != value_aliases.end()) {
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

std::optional<bir::Value> lower_global_initializer(std::string_view text,
                                                   bir::TypeKind type) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.empty()) {
    return std::nullopt;
  }

  if (trimmed == "zeroinitializer") {
    switch (type) {
      case bir::TypeKind::I1:
        return bir::Value::immediate_i1(false);
      case bir::TypeKind::I8:
        return bir::Value::immediate_i8(0);
      case bir::TypeKind::I32:
        return bir::Value::immediate_i32(0);
      case bir::TypeKind::I64:
        return bir::Value::immediate_i64(0);
      default:
        return std::nullopt;
    }
  }

  if (type == bir::TypeKind::I1) {
    if (trimmed == "true") {
      return bir::Value::immediate_i1(true);
    }
    if (trimmed == "false") {
      return bir::Value::immediate_i1(false);
    }
  }

  const auto parsed = parse_i64(trimmed);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  switch (type) {
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

std::optional<bir::Global> lower_scalar_global(const c4c::codegen::lir::LirGlobal& global) {
  if (backend_lir_global_uses_nonminimal_types(global)) {
    return std::nullopt;
  }

  const auto lowered_type = lower_integer_type(global.llvm_type);
  if (!lowered_type.has_value()) {
    return std::nullopt;
  }

  bir::Global lowered;
  lowered.name = global.name;
  lowered.type = *lowered_type;
  lowered.is_extern = global.is_extern_decl;
  lowered.is_constant = global.is_const;
  lowered.align_bytes = global.align_bytes > 0 ? static_cast<std::size_t>(global.align_bytes) : 0;
  if (!global.is_extern_decl) {
    const auto initializer = lower_global_initializer(global.init_text, *lowered_type);
    if (!initializer.has_value()) {
      return std::nullopt;
    }
    lowered.initializer = *initializer;
  }
  return lowered;
}

std::optional<bir::Global> lower_minimal_global(const c4c::codegen::lir::LirGlobal& global,
                                                GlobalInfo* info) {
  if (auto lowered = lower_scalar_global(global); lowered.has_value()) {
    info->value_type = lowered->type;
    info->element_size_bytes = type_size_bytes(lowered->type);
    info->element_count = 1;
    info->supports_direct_value = true;
    info->supports_linear_addressing = true;
    if (lowered->size_bytes == 0) {
      lowered->size_bytes = info->element_size_bytes;
    }
    return lowered;
  }

  if (!global.is_extern_decl) {
    return std::nullopt;
  }

  const auto integer_array = parse_integer_array_type(global.llvm_type);
  if (!integer_array.has_value()) {
    return std::nullopt;
  }

  const auto element_size_bytes = type_size_bytes(integer_array->element_type);
  if (element_size_bytes == 0) {
    return std::nullopt;
  }

  std::size_t total_elements = 1;
  for (const auto extent : integer_array->extents) {
    total_elements *= extent;
  }

  bir::Global lowered;
  lowered.name = global.name;
  lowered.type = integer_array->element_type;
  lowered.is_extern = true;
  lowered.is_constant = global.is_const;
  lowered.size_bytes = total_elements * element_size_bytes;
  lowered.align_bytes = global.align_bytes > 0 ? static_cast<std::size_t>(global.align_bytes) : 0;

  info->value_type = integer_array->element_type;
  info->element_size_bytes = element_size_bytes;
  info->element_count = total_elements;
  info->supports_direct_value = false;
  info->supports_linear_addressing = true;
  info->type_text = global.llvm_type;
  return lowered;
}

bool is_void_param_sentinel(const c4c::TypeSpec& type) {
  return type.base == TB_VOID && type.ptr_level == 0 && type.array_rank == 0;
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
                           bir::Function* lowered);

std::optional<bir::TypeKind> lower_signature_return_type(std::string_view signature_text) {
  const auto line = c4c::codegen::lir::trim_lir_arg_text(signature_text);
  const auto first_space = line.find(' ');
  const auto at_pos = line.find('@');
  if (first_space == std::string_view::npos || at_pos == std::string_view::npos ||
      first_space >= at_pos) {
    return std::nullopt;
  }
  return lower_integer_type(
      c4c::codegen::lir::trim_lir_arg_text(line.substr(first_space + 1, at_pos - first_space - 1)));
}

std::optional<bir::Function> lower_extern_decl(const c4c::codegen::lir::LirExternDecl& decl) {
  auto return_type = lower_integer_type(decl.return_type_str);
  if (!return_type.has_value()) {
    return_type = lower_integer_type(decl.return_type.str());
  }
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = decl.name;
  lowered.return_type = *return_type;
  lowered.is_declaration = true;
  return lowered;
}

std::optional<bir::Function> lower_decl_function(const c4c::codegen::lir::LirFunction& function) {
  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = lower_param_type(function.return_type)
                            .value_or(bir::TypeKind::Void);
  if (lowered.return_type == bir::TypeKind::Void) {
    const auto signature_return_type = lower_signature_return_type(function.signature_text);
    if (!signature_return_type.has_value()) {
      return std::nullopt;
    }
    lowered.return_type = *signature_return_type;
  }
  if (!lower_function_params(function, &lowered)) {
    return std::nullopt;
  }
  lowered.is_declaration = true;
  return lowered;
}

bool lower_function_params(const c4c::codegen::lir::LirFunction& function,
                           bir::Function* lowered) {
  if (function.params.size() == 1 && is_void_param_sentinel(function.params.front().second)) {
    return true;
  }

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

  if (parsed_params->size() == 1 && !parsed_params->front().is_varargs &&
      c4c::codegen::lir::trim_lir_arg_text(parsed_params->front().type) == "void") {
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
                               ValueMap& value_aliases,
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
            value_aliases[cast->result.str()] = bir::Value::immediate_i32(
                static_cast<std::int32_t>(imm));
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

bool lower_scalar_or_local_memory_inst(const c4c::codegen::lir::LirInst& inst,
                                       ValueMap& value_aliases,
                                       CompareMap& compare_exprs,
                                       LocalSlotTypes& local_slot_types,
                                       LocalPointerSlots& local_pointer_slots,
                                       LocalArraySlotMap& local_array_slots,
                                       GlobalPointerMap& global_pointer_slots,
                                       const GlobalTypes& global_types,
                                       bir::Function* lowered_function,
                                       std::vector<bir::Inst>* lowered_insts) {
  if (lower_scalar_compare_inst(inst, value_aliases, compare_exprs, lowered_insts)) {
    return true;
  }

  if (const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst)) {
    if (bin->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto opcode = lower_scalar_binary_opcode(bin->opcode);
    const auto value_type = lower_integer_type(bin->type_str.str());
    if (!opcode.has_value() || !value_type.has_value()) {
      return false;
    }

    const auto lhs = lower_value(bin->lhs, *value_type, value_aliases);
    const auto rhs = lower_value(bin->rhs, *value_type, value_aliases);
    if (!lhs.has_value() || !rhs.has_value()) {
      return false;
    }

    lowered_insts->push_back(bir::BinaryInst{
        .opcode = *opcode,
        .result = bir::Value::named(*value_type, bin->result.str()),
        .operand_type = *value_type,
        .lhs = *lhs,
        .rhs = *rhs,
    });
    return true;
  }

  if (const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst)) {
    if (alloca->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        !alloca->count.str().empty()) {
      return false;
    }

    const auto slot_type = lower_integer_type(alloca->type_str.str());
    const std::string slot_name = alloca->result.str();
    if (local_slot_types.find(slot_name) != local_slot_types.end() ||
        local_array_slots.find(slot_name) != local_array_slots.end()) {
      return false;
    }

    if (slot_type.has_value()) {
      local_slot_types.emplace(slot_name, *slot_type);
      local_pointer_slots.emplace(slot_name, slot_name);
      lowered_function->local_slots.push_back(bir::LocalSlot{
          .name = slot_name,
          .type = *slot_type,
          .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
      });
      return true;
    }

    const auto array_type = parse_local_array_type(alloca->type_str.str());
    if (!array_type.has_value()) {
      return false;
    }

    LocalArraySlots array_slots{.element_type = array_type->second};
    array_slots.element_slots.reserve(array_type->first);
    for (std::size_t index = 0; index < array_type->first; ++index) {
      const std::string element_slot = slot_name + "." + std::to_string(index);
      local_slot_types.emplace(element_slot, array_type->second);
      local_pointer_slots.emplace(element_slot, element_slot);
      array_slots.element_slots.push_back(element_slot);
      lowered_function->local_slots.push_back(bir::LocalSlot{
          .name = element_slot,
          .type = array_type->second,
          .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
      });
    }
    local_array_slots.emplace(slot_name, std::move(array_slots));
    return true;
  }

  if (const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst)) {
    if (gep->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        (gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
         gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
      return false;
    }

    std::string resolved_slot;
    if (gep->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = gep->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_linear_addressing) {
        return false;
      }
      if (gep->indices.size() != 2) {
        return false;
      }
      const auto base_index = parse_typed_operand(gep->indices[0]);
      const auto elem_index = parse_typed_operand(gep->indices[1]);
      if (!base_index.has_value() || !elem_index.has_value()) {
        return false;
      }
      const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
      const auto elem_imm = resolve_index_operand(elem_index->operand, value_aliases);
      const auto pointee_type = peel_integer_array_layer(global_it->second.type_text);
      if (!base_imm.has_value() || !elem_imm.has_value() || !pointee_type.has_value() ||
          *base_imm != 0 || *elem_imm < 0) {
        return false;
      }
      const auto array_type = parse_integer_array_type(global_it->second.type_text);
      if (!array_type.has_value() || array_type->extents.empty() ||
          static_cast<std::size_t>(*elem_imm) >= array_type->extents.front()) {
        return false;
      }
      const auto stride_bytes = type_size_bytes_from_text(*pointee_type);
      if (stride_bytes == 0) {
        return false;
      }

      global_pointer_slots[gep->result.str()] = GlobalAddress{
          .global_name = global_name,
          .value_type = lower_integer_type(*pointee_type).value_or(bir::TypeKind::Void),
          .byte_offset = static_cast<std::size_t>(*elem_imm) * stride_bytes,
      };
      return true;
    }

    if (const auto array_it = local_array_slots.find(gep->ptr.str()); array_it != local_array_slots.end()) {
      if (gep->indices.size() != 2) {
        return false;
      }
      const auto base_index = parse_typed_operand(gep->indices[0]);
      const auto elem_index = parse_typed_operand(gep->indices[1]);
      if (!base_index.has_value() || !elem_index.has_value()) {
        return false;
      }
      const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
      const auto elem_imm = resolve_index_operand(elem_index->operand, value_aliases);
      if (!base_imm.has_value() || !elem_imm.has_value() || *base_imm != 0 ||
          *elem_imm < 0 ||
          static_cast<std::size_t>(*elem_imm) >= array_it->second.element_slots.size()) {
        return false;
      }
      resolved_slot = array_it->second.element_slots[static_cast<std::size_t>(*elem_imm)];
    } else if (const auto global_ptr_it = global_pointer_slots.find(gep->ptr.str());
               global_ptr_it != global_pointer_slots.end()) {
      if (gep->indices.size() != 1) {
        return false;
      }
      const auto parsed_index = parse_typed_operand(gep->indices.front());
      if (!parsed_index.has_value()) {
        return false;
      }
      const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
      const auto stride_bytes = type_size_bytes_from_text(gep->element_type.str());
      if (!index_imm.has_value() || *index_imm < 0 || stride_bytes == 0) {
        return false;
      }

      global_pointer_slots[gep->result.str()] = GlobalAddress{
          .global_name = global_ptr_it->second.global_name,
          .value_type = lower_integer_type(gep->element_type.str()).value_or(bir::TypeKind::Void),
          .byte_offset =
              global_ptr_it->second.byte_offset +
              static_cast<std::size_t>(*index_imm) * stride_bytes,
      };
      return true;
    } else {
      const auto ptr_it = local_pointer_slots.find(gep->ptr.str());
      if (ptr_it == local_pointer_slots.end() || gep->indices.size() != 1) {
        return false;
      }
      const auto slot_it = local_slot_types.find(ptr_it->second);
      if (slot_it == local_slot_types.end()) {
        return false;
      }

      const auto parsed_index = parse_typed_operand(gep->indices.front());
      if (!parsed_index.has_value()) {
        return false;
      }
      const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_imm.has_value()) {
        return false;
      }

      const auto dot = ptr_it->second.rfind('.');
      if (dot == std::string::npos) {
        if (*index_imm != 0) {
          return false;
        }
        resolved_slot = ptr_it->second;
      } else {
        const auto base_name = ptr_it->second.substr(0, dot);
        const auto base_array_it = local_array_slots.find(base_name);
        if (base_array_it == local_array_slots.end()) {
          return false;
        }
        const auto base_offset = parse_i64(std::string_view(ptr_it->second).substr(dot + 1));
        if (!base_offset.has_value()) {
          return false;
        }
        const auto final_index = *base_offset + *index_imm;
        if (final_index < 0 ||
            static_cast<std::size_t>(final_index) >= base_array_it->second.element_slots.size()) {
          return false;
        }
        resolved_slot =
            base_array_it->second.element_slots[static_cast<std::size_t>(final_index)];
      }
    }

    local_pointer_slots[gep->result.str()] = std::move(resolved_slot);
    return true;
  }

  if (const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst)) {
    if (store->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
        store->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global) {
      return false;
    }

    const auto value_type = lower_integer_type(store->type_str.str());
    if (!value_type.has_value()) {
      return false;
    }

    const auto value = lower_value(store->val, *value_type, value_aliases);
    if (!value.has_value()) {
      return false;
    }

    if (store->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = store->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != *value_type) {
        return false;
      }
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_name,
          .value = *value,
      });
      return true;
    }

    if (const auto global_ptr_it = global_pointer_slots.find(store->ptr.str());
        global_ptr_it != global_pointer_slots.end()) {
      if (global_ptr_it->second.value_type != *value_type) {
        return false;
      }
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_ptr_it->second.global_name,
          .value = *value,
          .byte_offset = global_ptr_it->second.byte_offset,
      });
      return true;
    }

    const auto ptr_it = local_pointer_slots.find(store->ptr.str());
    if (ptr_it == local_pointer_slots.end()) {
      return false;
    }

    const auto slot_it = local_slot_types.find(ptr_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return false;
    }

    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = ptr_it->second,
        .value = *value,
    });
    return true;
  }

  if (const auto* load = std::get_if<c4c::codegen::lir::LirLoadOp>(&inst)) {
    if (load->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        (load->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
         load->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
      return false;
    }

    const auto value_type = lower_integer_type(load->type_str.str());
    if (!value_type.has_value()) {
      return false;
    }

    if (load->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = load->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != *value_type) {
        return false;
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .global_name = global_name,
      });
      return true;
    }

    if (const auto global_ptr_it = global_pointer_slots.find(load->ptr.str());
        global_ptr_it != global_pointer_slots.end()) {
      if (global_ptr_it->second.value_type != *value_type) {
        return false;
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .global_name = global_ptr_it->second.global_name,
          .byte_offset = global_ptr_it->second.byte_offset,
      });
      return true;
    }

    const auto ptr_it = local_pointer_slots.find(load->ptr.str());
    if (ptr_it == local_pointer_slots.end()) {
      return false;
    }

    const auto slot_it = local_slot_types.find(ptr_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return false;
    }

    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(*value_type, load->result.str()),
        .slot_name = ptr_it->second,
    });
    return true;
  }

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
    const auto return_type = lower_integer_type(call->return_type.str());
    if (!return_type.has_value()) {
      return false;
    }

    std::vector<bir::Value> lowered_args;
    std::vector<bir::TypeKind> lowered_arg_types;
    std::optional<std::string> callee_name;

    if (const auto parsed_call = parse_backend_direct_global_typed_call(*call);
        parsed_call.has_value()) {
      callee_name = std::string(parsed_call->symbol_name);
      lowered_args.reserve(parsed_call->typed_call.args.size());
      lowered_arg_types.reserve(parsed_call->typed_call.param_types.size());
      for (std::size_t index = 0; index < parsed_call->typed_call.args.size(); ++index) {
        const auto arg_type = lower_integer_type(parsed_call->typed_call.param_types[index]);
        if (!arg_type.has_value()) {
          return false;
        }
        const auto arg =
            lower_value(c4c::codegen::lir::LirOperand(
                            std::string(parsed_call->typed_call.args[index].operand)),
                        *arg_type,
                        value_aliases);
        if (!arg.has_value()) {
          return false;
        }
        lowered_arg_types.push_back(*arg_type);
        lowered_args.push_back(*arg);
      }
    } else if (const auto zero_arg_callee =
                   c4c::codegen::lir::parse_lir_direct_global_callee(call->callee);
               zero_arg_callee.has_value() &&
               c4c::codegen::lir::trim_lir_arg_text(call->args_str).empty()) {
      callee_name = std::string(*zero_arg_callee);
    } else {
      return false;
    }

    bir::CallInst lowered_call;
    if (*return_type != bir::TypeKind::Void) {
      if (call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return false;
      }
      lowered_call.result = bir::Value::named(*return_type, call->result.str());
    } else if (call->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }
    lowered_call.callee = std::move(*callee_name);
    lowered_call.args = std::move(lowered_args);
    lowered_call.arg_types = std::move(lowered_arg_types);
    lowered_call.return_type_name = std::string(call->return_type.str());
    lowered_call.return_type = *return_type;
    lowered_call.is_indirect = false;
    lowered_insts->push_back(std::move(lowered_call));
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

  ValueMap value_aliases;
  CompareMap compare_exprs;
  for (const auto& inst : entry.insts) {
    if (!lower_scalar_compare_inst(inst, value_aliases, compare_exprs, nullptr)) {
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

  const auto true_value = lower_value(*true_incoming, *phi_type, value_aliases);
  const auto false_value = lower_value(*false_incoming, *phi_type, value_aliases);
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
    const c4c::codegen::lir::LirFunction& function,
    const GlobalTypes& global_types) {
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

  ValueMap value_aliases;
  CompareMap compare_exprs;
  LocalSlotTypes local_slot_types;
  LocalPointerSlots local_pointer_slots;
  LocalArraySlotMap local_array_slots;
  GlobalPointerMap global_pointer_slots;
  std::vector<bir::Inst> hoisted_alloca_scratch;

  for (const auto& inst : function.alloca_insts) {
    if (!lower_scalar_or_local_memory_inst(
            inst,
            value_aliases,
            compare_exprs,
            local_slot_types,
            local_pointer_slots,
            local_array_slots,
            global_pointer_slots,
            global_types,
            &lowered,
            &hoisted_alloca_scratch)) {
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
              inst,
              value_aliases,
              compare_exprs,
              local_slot_types,
              local_pointer_slots,
              local_array_slots,
              global_pointer_slots,
              global_types,
              &lowered,
              &lowered_block.insts)) {
        return std::nullopt;
      }
    }

    if (const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator)) {
      bir::ReturnTerminator lowered_ret;
      if (ret->value_str.has_value()) {
        const auto value = lower_value(*ret->value_str, *return_type, value_aliases);
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
      analysis.string_constant_count != 0) {
    context.note(
        "module",
        "bootstrap lir_to_bir only supports minimal scalar globals and no strings right now");
    if (analysis.string_constant_count != 0) {
      return std::nullopt;
    }
  }

  GlobalTypes global_types;
  global_types.reserve(context.lir_module.globals.size());
  for (const auto& global : context.lir_module.globals) {
    GlobalInfo info;
    auto lowered_global = lower_minimal_global(global, &info);
    if (!lowered_global.has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports minimal scalar globals and extern integer-array globals right now");
      return std::nullopt;
    }
    global_types.emplace(lowered_global->name, info);
    module.globals.push_back(std::move(*lowered_global));
  }

  if (analysis.string_constant_count != 0) {
    return std::nullopt;
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

    auto lowered_function = lower_select_family_function(context, function);
    if (!lowered_function.has_value()) {
      lowered_function = lower_branch_family_function(context, function, global_types);
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
      "lowered scalar compare/select/local-memory/global/call/return module to BIR");
  return module;
}

}  // namespace c4c::backend
