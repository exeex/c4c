#include "lir_to_bir.hpp"
#include "call_decode.hpp"

#include <charconv>
#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend {

namespace {

std::optional<bir::TypeKind> lower_minimal_scalar_type(const c4c::TypeSpec& type) {
  if (type.ptr_level != 0 || type.array_rank != 0) {
    return std::nullopt;
  }
  if (type.base == TB_CHAR) {
    return bir::TypeKind::I8;
  }
  if (type.base == TB_INT) {
    return bir::TypeKind::I32;
  }
  if (type.base == TB_LONG || type.base == TB_ULONG || type.base == TB_LONGLONG ||
      type.base == TB_ULONGLONG) {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<bir::TypeKind> lower_scalar_type_text(std::string_view text) {
  if (text == "i8") {
    return bir::TypeKind::I8;
  }
  if (text == "i32") {
    return bir::TypeKind::I32;
  }
  if (text == "i64") {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<std::int64_t> parse_immediate(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool immediate_fits_type(std::int64_t value, bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I8:
      return value >= -128 && value <= 127;
    case bir::TypeKind::I32:
      return value >= std::numeric_limits<std::int32_t>::min() &&
             value <= std::numeric_limits<std::int32_t>::max();
    case bir::TypeKind::I64:
      return true;
    case bir::TypeKind::Void:
      return false;
  }
  return false;
}

std::optional<bir::Value> lower_immediate_or_name(std::string_view value_text,
                                                  bir::TypeKind type) {
  if (value_text.empty()) {
    return std::nullopt;
  }
  if (value_text.front() == '%') {
    return bir::Value::named(type, std::string(value_text));
  }
  const auto immediate = parse_immediate(value_text);
  if (!immediate.has_value() || !immediate_fits_type(*immediate, type)) {
    return std::nullopt;
  }
  switch (type) {
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(*immediate));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(*immediate));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(*immediate);
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<bir::BinaryOpcode> lower_binary_opcode(std::string_view opcode) {
  if (opcode == "add") {
    return bir::BinaryOpcode::Add;
  }
  if (opcode == "sub") {
    return bir::BinaryOpcode::Sub;
  }
  if (opcode == "mul") {
    return bir::BinaryOpcode::Mul;
  }
  if (opcode == "and") {
    return bir::BinaryOpcode::And;
  }
  if (opcode == "or") {
    return bir::BinaryOpcode::Or;
  }
  if (opcode == "xor") {
    return bir::BinaryOpcode::Xor;
  }
  if (opcode == "shl") {
    return bir::BinaryOpcode::Shl;
  }
  if (opcode == "lshr") {
    return bir::BinaryOpcode::LShr;
  }
  if (opcode == "sdiv") {
    return bir::BinaryOpcode::SDiv;
  }
  if (opcode == "udiv") {
    return bir::BinaryOpcode::UDiv;
  }
  if (opcode == "srem") {
    return bir::BinaryOpcode::SRem;
  }
  if (opcode == "urem") {
    return bir::BinaryOpcode::URem;
  }
  if (opcode == "eq") {
    return bir::BinaryOpcode::Eq;
  }
  return std::nullopt;
}

std::optional<bir::BinaryOpcode> lower_compare_materialization_opcode(std::string_view predicate) {
  if (predicate == "eq") {
    return bir::BinaryOpcode::Eq;
  }
  if (predicate == "ne") {
    return bir::BinaryOpcode::Ne;
  }
  if (predicate == "slt") {
    return bir::BinaryOpcode::Slt;
  }
  if (predicate == "sle") {
    return bir::BinaryOpcode::Sle;
  }
  if (predicate == "sgt") {
    return bir::BinaryOpcode::Sgt;
  }
  if (predicate == "sge") {
    return bir::BinaryOpcode::Sge;
  }
  if (predicate == "ult") {
    return bir::BinaryOpcode::Ult;
  }
  if (predicate == "ule") {
    return bir::BinaryOpcode::Ule;
  }
  if (predicate == "ugt") {
    return bir::BinaryOpcode::Ugt;
  }
  if (predicate == "uge") {
    return bir::BinaryOpcode::Uge;
  }
  return std::nullopt;
}

std::optional<bir::BinaryInst> lower_binary(const c4c::codegen::lir::LirInst& inst) {
  const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
  if (bin == nullptr || bin->result.str().empty()) {
    return std::nullopt;
  }
  const auto type = lower_scalar_type_text(bin->type_str.str());
  if (!type.has_value()) {
    return std::nullopt;
  }
  const auto opcode = lower_binary_opcode(bin->opcode.str());
  if (!opcode.has_value()) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(bin->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(bin->rhs.str(), *type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  bir::BinaryInst lowered;
  lowered.opcode = *opcode;
  lowered.result = bir::Value::named(*type, bin->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  return lowered;
}

std::optional<bir::BinaryInst> lower_compare_materialization(
    const c4c::codegen::lir::LirInst& compare_inst,
    const c4c::codegen::lir::LirInst& cast_inst) {
  const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&compare_inst);
  const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&cast_inst);
  if (cmp == nullptr || cast == nullptr || cmp->is_float || cmp->result.str().empty() ||
      cast->result.str().empty() ||
      cast->kind != c4c::codegen::lir::LirCastKind::ZExt || cast->from_type.str() != "i1" ||
      cast->operand.str() != cmp->result.str()) {
    return std::nullopt;
  }
  const auto opcode = lower_compare_materialization_opcode(cmp->predicate.str());
  if (!opcode.has_value()) {
    return std::nullopt;
  }

  const auto type = lower_scalar_type_text(cmp->type_str.str());
  const auto widened_type = lower_scalar_type_text(cast->to_type.str());
  if (!type.has_value() || !widened_type.has_value() || *type != *widened_type) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(cmp->rhs.str(), *type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  bir::BinaryInst lowered;
  lowered.opcode = *opcode;
  lowered.result = bir::Value::named(*type, cast->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  return lowered;
}

std::optional<bir::SelectInst> lower_select_materialization(
    const c4c::codegen::lir::LirInst& compare_inst,
    const c4c::codegen::lir::LirInst& select_inst) {
  const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&compare_inst);
  const auto* select = std::get_if<c4c::codegen::lir::LirSelectOp>(&select_inst);
  if (cmp == nullptr || select == nullptr || cmp->is_float || cmp->result.str().empty() ||
      select->result.str().empty() || select->cond.str() != cmp->result.str()) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp->predicate.str());
  const auto type = lower_scalar_type_text(cmp->type_str.str());
  const auto selected_type = lower_scalar_type_text(select->type_str.str());
  if (!predicate.has_value() || !type.has_value() || !selected_type.has_value() ||
      *type != *selected_type) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(cmp->rhs.str(), *type);
  const auto true_value = lower_immediate_or_name(select->true_val.str(), *type);
  const auto false_value = lower_immediate_or_name(select->false_val.str(), *type);
  if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
      !false_value.has_value()) {
    return std::nullopt;
  }

  bir::SelectInst lowered;
  lowered.predicate = *predicate;
  lowered.result = bir::Value::named(*type, select->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  lowered.true_value = *true_value;
  lowered.false_value = *false_value;
  return lowered;
}

std::optional<std::vector<bir::Param>> lower_function_params(
    const c4c::codegen::lir::LirFunction& lir_function) {
  std::vector<bir::Param> params;
  if (!lir_function.params.empty()) {
    if (lir_function.params.size() > 2) {
      return std::nullopt;
    }
    params.reserve(lir_function.params.size());
    for (const auto& [param_name, param_type] : lir_function.params) {
      const auto lowered_type = lower_minimal_scalar_type(param_type);
      if (!lowered_type.has_value() || param_name.empty()) {
        return std::nullopt;
      }
      params.push_back(bir::Param{*lowered_type, param_name});
    }
    return params;
  }

  const auto parsed_params =
      parse_backend_function_signature_params(lir_function.signature_text);
  if (!parsed_params.has_value() || parsed_params->size() > 2) {
    return std::nullopt;
  }

  params.reserve(parsed_params->size());
  for (const auto& param : *parsed_params) {
    const auto lowered_type = lower_scalar_type_text(param.type);
    if (param.is_varargs || !lowered_type.has_value() || param.operand.empty()) {
      return std::nullopt;
    }
    params.push_back(bir::Param{*lowered_type, param.operand});
  }
  return params;
}

struct AffineValue {
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

std::optional<AffineValue> combine_affine(const AffineValue& lhs,
                                          const AffineValue& rhs,
                                          bir::BinaryOpcode opcode) {
  if (opcode == bir::BinaryOpcode::Add) {
    if ((lhs.uses_first_param && rhs.uses_first_param) ||
        (lhs.uses_second_param && rhs.uses_second_param)) {
      return std::nullopt;
    }
    return AffineValue{lhs.uses_first_param || rhs.uses_first_param,
                       lhs.uses_second_param || rhs.uses_second_param,
                       lhs.constant + rhs.constant};
  }

  if (rhs.uses_first_param || rhs.uses_second_param) {
    return std::nullopt;
  }
  if (opcode == bir::BinaryOpcode::Sub) {
    return AffineValue{lhs.uses_first_param, lhs.uses_second_param,
                       lhs.constant - rhs.constant};
  }

  if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
      rhs.uses_second_param) {
    return std::nullopt;
  }
  if (opcode == bir::BinaryOpcode::And) {
    return AffineValue{false, false, lhs.constant & rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::Or) {
    return AffineValue{false, false, lhs.constant | rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::Shl) {
    if (rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant << rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::LShr) {
    if (lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) >> rhs.constant)};
  }
  if (opcode == bir::BinaryOpcode::SDiv) {
    if (rhs.constant == 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant / rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::UDiv) {
    if (lhs.constant < 0 || rhs.constant <= 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) /
                                  static_cast<std::uint64_t>(rhs.constant))};
  }
  if (opcode == bir::BinaryOpcode::SRem) {
    if (rhs.constant == 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant % rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::URem) {
    if (lhs.constant < 0 || rhs.constant <= 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) %
                                  static_cast<std::uint64_t>(rhs.constant))};
  }
  if (opcode == bir::BinaryOpcode::Eq) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant == rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Ne) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant != rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Slt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant < rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sle) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant <= rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sgt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant > rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sge) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant >= rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Ult) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) < static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Ule) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) <= static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Ugt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) > static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Uge) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) >= static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  return AffineValue{false, false, lhs.constant * rhs.constant};
}

std::optional<bool> evaluate_predicate(const AffineValue& lhs,
                                       const AffineValue& rhs,
                                       bir::BinaryOpcode opcode) {
  if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
      rhs.uses_second_param) {
    return std::nullopt;
  }
  switch (opcode) {
    case bir::BinaryOpcode::Eq:
      return lhs.constant == rhs.constant;
    case bir::BinaryOpcode::Ne:
      return lhs.constant != rhs.constant;
    case bir::BinaryOpcode::Slt:
      return lhs.constant < rhs.constant;
    case bir::BinaryOpcode::Sle:
      return lhs.constant <= rhs.constant;
    case bir::BinaryOpcode::Sgt:
      return lhs.constant > rhs.constant;
    case bir::BinaryOpcode::Sge:
      return lhs.constant >= rhs.constant;
    case bir::BinaryOpcode::Ult:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) <
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Ule:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) <=
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Ugt:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) >
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Uge:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) >=
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::vector<bir::BinaryInst>> lower_bounded_predecessor_chain(
    const c4c::codegen::lir::LirBlock& lir_block,
    std::string_view expected_result,
    const std::vector<bir::Param>& params,
    bir::TypeKind type) {
  std::vector<bir::BinaryInst> lowered;
  lowered.reserve(lir_block.insts.size());

  std::vector<std::string> available_names;
  available_names.reserve(params.size() + lir_block.insts.size());
  std::vector<AffineValue> affine_values;
  affine_values.reserve(params.size() + lir_block.insts.size());
  for (std::size_t index = 0; index < params.size(); ++index) {
    available_names.push_back(params[index].name);
    affine_values.push_back(AffineValue{index == 0, index == 1, 0});
  }

  auto name_is_available = [&](std::string_view name) {
    return std::find(available_names.begin(), available_names.end(), name) !=
           available_names.end();
  };
  auto operand_is_available = [&](const bir::Value& value) {
    return value.kind != bir::Value::Kind::Named || name_is_available(value.name);
  };
  auto lower_affine_value = [&](const bir::Value& value) -> std::optional<AffineValue> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return AffineValue{false, false, value.immediate};
    }
    for (std::size_t index = 0; index < available_names.size(); ++index) {
      if (available_names[index] == value.name) {
        return affine_values[index];
      }
    }
    return std::nullopt;
  };

  for (const auto& inst : lir_block.insts) {
    auto binary = lower_binary(inst);
    if (!binary.has_value() || binary->result.type != type ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    if (!operand_is_available(binary->lhs) || !operand_is_available(binary->rhs) ||
        name_is_available(binary->result.name)) {
      return std::nullopt;
    }
    const auto lhs = lower_affine_value(binary->lhs);
    const auto rhs = lower_affine_value(binary->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }
    const auto combined = combine_affine(*lhs, *rhs, binary->opcode);
    if (!combined.has_value()) {
      return std::nullopt;
    }
    available_names.push_back(binary->result.name);
    affine_values.push_back(*combined);
    lowered.push_back(*binary);
  }

  if (!lowered.empty() && !expected_result.empty() &&
      lowered.back().result.name != expected_result) {
    return std::nullopt;
  }
  return lowered;
}

std::optional<std::vector<bir::BinaryInst>> lower_bounded_predecessor_chain(
    const c4c::codegen::lir::LirBlock& first_block,
    const c4c::codegen::lir::LirBlock& second_block,
    std::string_view expected_result,
    const std::vector<bir::Param>& params,
    bir::TypeKind type) {
  auto lowered = lower_bounded_predecessor_chain(first_block, "", params, type);
  if (!lowered.has_value()) {
    return std::nullopt;
  }
  auto tail = lower_bounded_predecessor_chain(second_block, expected_result, params, type);
  if (!tail.has_value()) {
    return std::nullopt;
  }

  if (lowered->empty()) {
    return tail;
  }
  if (tail->empty()) {
    if (lowered->back().result.name != expected_result) {
      return std::nullopt;
    }
    return lowered;
  }

  std::vector<std::string> available_names;
  available_names.reserve(params.size() + lowered->size());
  for (const auto& param : params) {
    available_names.push_back(param.name);
  }
  for (const auto& inst : *lowered) {
    available_names.push_back(inst.result.name);
  }

  auto operand_is_available = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::find(available_names.begin(), available_names.end(), value.name) !=
           available_names.end();
  };
  for (const auto& inst : *tail) {
    if (!operand_is_available(inst.lhs) || !operand_is_available(inst.rhs) ||
        std::find(available_names.begin(), available_names.end(), inst.result.name) !=
            available_names.end()) {
      return std::nullopt;
    }
    lowered->push_back(inst);
    available_names.push_back(inst.result.name);
  }
  return lowered;
}

std::optional<bir::Function> try_lower_conditional_return_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 3) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[2];
  if (entry.label.empty() || entry.insts.size() != 3 || !true_block.insts.empty() ||
      !false_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || cmp0->result.str().empty() ||
      cast->result.str().empty() || cmp1->is_float || cmp1->result.str().empty() ||
      cast->kind != LirCastKind::ZExt || cast->from_type.str() != "i1" ||
      cast->operand.str() != cmp0->result.str() || cmp1->predicate.str() != "ne" ||
      cmp1->lhs.str() != cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str() || true_block.label != condbr->true_label ||
      false_block.label != condbr->false_label) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  const auto compare_type = lower_scalar_type_text(cmp0->type_str.str());
  const auto widened_type = lower_scalar_type_text(cast->to_type.str());
  const auto cond_type = lower_scalar_type_text(cmp1->type_str.str());
  if (!predicate.has_value() || !compare_type.has_value() || !widened_type.has_value() ||
      !cond_type.has_value() || *compare_type != *widened_type ||
      *compare_type != *cond_type) {
    return std::nullopt;
  }

  const auto* true_ret = std::get_if<LirRet>(&true_block.terminator);
  const auto* false_ret = std::get_if<LirRet>(&false_block.terminator);
  if (true_ret == nullptr || false_ret == nullptr || !true_ret->value_str.has_value() ||
      !false_ret->value_str.has_value() || true_ret->type_str != cmp0->type_str ||
      false_ret->type_str != cmp0->type_str) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp0->lhs.str(), *compare_type);
  const auto rhs = lower_immediate_or_name(cmp0->rhs.str(), *compare_type);
  const auto true_value = lower_immediate_or_name(*true_ret->value_str, *compare_type);
  const auto false_value = lower_immediate_or_name(*false_ret->value_str, *compare_type);
  if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
      !false_value.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs) ||
      !operand_is_param_or_immediate(*true_value) ||
      !operand_is_param_or_immediate(*false_value)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *compare_type;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(*compare_type, "%t.select"),
      *lhs,
      *rhs,
      *true_value,
      *false_value,
  });
  block.terminator.value = bir::Value::named(*compare_type, "%t.select");
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_conditional_phi_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 4 && lir_function.blocks.size() != 6) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  const auto* cmp0 = entry.insts.size() > 0 ? std::get_if<LirCmpOp>(&entry.insts[0]) : nullptr;
  const auto* cast = entry.insts.size() > 1 ? std::get_if<LirCastOp>(&entry.insts[1]) : nullptr;
  const auto* cmp1 = entry.insts.size() > 2 ? std::get_if<LirCmpOp>(&entry.insts[2]) : nullptr;
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (entry.label.empty() || entry.insts.size() != 3 || cmp0 == nullptr || cast == nullptr ||
      cmp1 == nullptr || condbr == nullptr || cmp0->is_float || cmp0->result.str().empty() ||
      cast->result.str().empty() || cmp1->is_float || cmp1->result.str().empty() ||
      cast->kind != LirCastKind::ZExt || cast->from_type.str() != "i1" ||
      cast->operand.str() != cmp0->result.str() || cmp1->predicate.str() != "ne" ||
      cmp1->lhs.str() != cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str()) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  const auto compare_type = lower_scalar_type_text(cmp0->type_str.str());
  const auto widened_type = lower_scalar_type_text(cast->to_type.str());
  const auto cond_type = lower_scalar_type_text(cmp1->type_str.str());
  if (!predicate.has_value() || !compare_type.has_value() || !widened_type.has_value() ||
      !cond_type.has_value() || *compare_type != *widened_type ||
      *compare_type != *cond_type) {
    return std::nullopt;
  }

  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[lir_function.blocks.size() == 4 ? 2 : 3];
  const auto* true_br = std::get_if<LirBr>(&true_block.terminator);
  const auto* false_br = std::get_if<LirBr>(&false_block.terminator);
  if (true_br == nullptr || false_br == nullptr || condbr->true_label != true_block.label ||
      condbr->false_label != false_block.label) {
    return std::nullopt;
  }

  const c4c::codegen::lir::LirBlock* true_phi_pred = &true_block;
  const c4c::codegen::lir::LirBlock* false_phi_pred = &false_block;
  const c4c::codegen::lir::LirBlock* true_value_block = &true_block;
  const c4c::codegen::lir::LirBlock* false_value_block = &false_block;
  const c4c::codegen::lir::LirBlock* join_block = nullptr;
  if (lir_function.blocks.size() == 4) {
    join_block = &lir_function.blocks[3];
    if (true_br->target_label != join_block->label || false_br->target_label != join_block->label) {
      return std::nullopt;
    }
  } else {
    const auto& true_end = lir_function.blocks[2];
    const auto& false_end = lir_function.blocks[4];
    const auto* true_end_br = std::get_if<LirBr>(&true_end.terminator);
    const auto* false_end_br = std::get_if<LirBr>(&false_end.terminator);
    join_block = &lir_function.blocks[5];
    if (true_end_br == nullptr || false_end_br == nullptr ||
        true_br->target_label != true_end.label ||
        false_br->target_label != false_end.label ||
        true_end_br->target_label != join_block->label ||
        false_end_br->target_label != join_block->label) {
      return std::nullopt;
    }
    true_phi_pred = &true_end;
    false_phi_pred = &false_end;
    true_value_block = &true_block;
    false_value_block = &false_block;
  }

  const auto* phi = join_block->insts.size() > 0 ? std::get_if<LirPhiOp>(&join_block->insts[0]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&join_block->terminator);
  if (phi == nullptr || ret == nullptr ||
      phi->result.str().empty() || phi->type_str.str() != cmp0->type_str.str() ||
      phi->incoming.size() != 2 || !ret->value_str.has_value() ||
      ret->type_str != cmp0->type_str ||
      phi->incoming[0].second != true_phi_pred->label ||
      phi->incoming[1].second != false_phi_pred->label) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp0->lhs.str(), *compare_type);
  const auto rhs = lower_immediate_or_name(cmp0->rhs.str(), *compare_type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  const auto true_chain =
      lir_function.blocks.size() == 4
          ? lower_bounded_predecessor_chain(*true_phi_pred, phi->incoming[0].first, params,
                                            *compare_type)
          : lower_bounded_predecessor_chain(*true_value_block, *true_phi_pred,
                                            phi->incoming[0].first, params, *compare_type);
  const auto false_chain =
      lir_function.blocks.size() == 4
          ? lower_bounded_predecessor_chain(*false_phi_pred, phi->incoming[1].first, params,
                                            *compare_type)
          : lower_bounded_predecessor_chain(*false_value_block, *false_phi_pred,
                                            phi->incoming[1].first, params, *compare_type);
  if (!true_chain.has_value() || !false_chain.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *compare_type;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  std::vector<std::string> available_names;
  available_names.reserve(params.size() + true_chain->size() + false_chain->size() + 1);
  for (const auto& param : params) {
    available_names.push_back(param.name);
  }
  for (const auto& inst : *true_chain) {
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }
  for (const auto& inst : *false_chain) {
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }

  auto lower_branch_value = [&](std::string_view value_text,
                                const std::vector<bir::BinaryInst>& chain)
      -> std::optional<bir::Value> {
    if (!chain.empty()) {
      return bir::Value::named(*compare_type, std::string(value_text));
    }
    return lower_immediate_or_name(value_text, *compare_type);
  };
  const auto true_value = lower_branch_value(phi->incoming[0].first, *true_chain);
  const auto false_value = lower_branch_value(phi->incoming[1].first, *false_chain);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  auto operand_is_available = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::find(available_names.begin(), available_names.end(), value.name) !=
           available_names.end();
  };
  if (!operand_is_available(*true_value) || !operand_is_available(*false_value)) {
    return std::nullopt;
  }

  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(*compare_type, phi->result.str()),
      *lhs,
      *rhs,
      *true_value,
      *false_value,
  });
  available_names.push_back(phi->result.str());

  for (std::size_t inst_index = 1; inst_index < join_block->insts.size(); ++inst_index) {
    auto binary = lower_binary(join_block->insts[inst_index]);
    if (!binary.has_value() || binary->result.type != *compare_type ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    if (!operand_is_available(binary->lhs) || !operand_is_available(binary->rhs) ||
        std::find(available_names.begin(), available_names.end(), binary->result.name) !=
            available_names.end()) {
      return std::nullopt;
    }
    block.insts.push_back(*binary);
    available_names.push_back(binary->result.name);
  }

  const auto return_value = lower_immediate_or_name(*ret->value_str, *compare_type);
  if (!return_value.has_value()) {
    return std::nullopt;
  }
  if (return_value->kind == bir::Value::Kind::Named &&
      std::find(available_names.begin(), available_names.end(), return_value->name) ==
          available_names.end()) {
    return std::nullopt;
  }
  block.terminator.value = *return_value;
  function.blocks.push_back(std::move(block));
  return function;
}

}  // namespace

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  if (!module.globals.empty() || !module.string_pool.empty() || !module.extern_decls.empty() ||
      module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_function = module.functions.front();
  if (lir_function.is_declaration || !lir_function.alloca_insts.empty() ||
      lir_function.params.size() > 2) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  const auto params = lower_function_params(lir_function);
  if (!params.has_value()) {
    return std::nullopt;
  }

  if (const auto select_function =
          try_lower_conditional_return_select_function(lir_function, *params);
      select_function.has_value()) {
    lowered.functions.push_back(*select_function);
    return lowered;
  }
  if (const auto select_function =
          try_lower_conditional_phi_select_function(lir_function, *params);
      select_function.has_value()) {
    lowered.functions.push_back(*select_function);
    return lowered;
  }

  if (lir_function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_block = lir_function.blocks.front();
  if (lir_block.label.empty()) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&lir_block.terminator);
  if (ret == nullptr || !ret->value_str.has_value()) {
    return std::nullopt;
  }
  const auto return_type = lower_scalar_type_text(ret->type_str);
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *return_type;
  function.params = *params;

  bir::Block block;
  block.label = lir_block.label;
  std::vector<std::string> defined_names;
  defined_names.reserve(function.params.size() + lir_block.insts.size());
  for (const auto& param : function.params) {
    defined_names.push_back(param.name);
  }
  std::vector<AffineValue> affine_values;
  affine_values.reserve(function.params.size() + lir_block.insts.size());
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    affine_values.push_back(
        AffineValue{index == 0, index == 1, 0});
  }
  for (std::size_t inst_index = 0; inst_index < lir_block.insts.size(); ++inst_index) {
    auto name_is_defined = [&](std::string_view name) {
      return std::find(defined_names.begin(), defined_names.end(), name) !=
             defined_names.end();
    };
    auto operand_is_available = [&](const bir::Value& value) {
      return value.kind != bir::Value::Kind::Named || name_is_defined(value.name);
    };
    auto lower_affine_value = [&](const bir::Value& value) -> std::optional<AffineValue> {
      if (value.kind == bir::Value::Kind::Immediate) {
        return AffineValue{false, false, value.immediate};
      }
      for (std::size_t index = 0; index < defined_names.size(); ++index) {
        if (defined_names[index] == value.name) {
          return affine_values[index];
        }
      }
      return std::nullopt;
    };

    auto binary = [&]() -> std::optional<bir::BinaryInst> {
      if (inst_index + 1 < lir_block.insts.size()) {
        auto lowered_compare = lower_compare_materialization(
            lir_block.insts[inst_index], lir_block.insts[inst_index + 1]);
        if (lowered_compare.has_value()) {
          ++inst_index;
          return lowered_compare;
        }
      }
      return lower_binary(lir_block.insts[inst_index]);
    }();
    if (binary.has_value()) {
      if (!operand_is_available(binary->lhs) || !operand_is_available(binary->rhs) ||
          name_is_defined(binary->result.name)) {
        return std::nullopt;
      }
      const auto lhs = lower_affine_value(binary->lhs);
      const auto rhs = lower_affine_value(binary->rhs);
      if (!lhs.has_value() || !rhs.has_value()) {
        return std::nullopt;
      }
      const auto combined = combine_affine(*lhs, *rhs, binary->opcode);
      if (!combined.has_value()) {
        return std::nullopt;
      }
      defined_names.push_back(binary->result.name);
      affine_values.push_back(*combined);
      block.insts.push_back(*binary);
      continue;
    }

    if (inst_index + 1 >= lir_block.insts.size()) {
      return std::nullopt;
    }
    auto select = lower_select_materialization(
        lir_block.insts[inst_index], lir_block.insts[inst_index + 1]);
    if (!select.has_value()) {
      return std::nullopt;
    }
    ++inst_index;
    if (!operand_is_available(select->lhs) || !operand_is_available(select->rhs) ||
        !operand_is_available(select->true_value) ||
        !operand_is_available(select->false_value) ||
        name_is_defined(select->result.name)) {
      return std::nullopt;
    }
    const auto lhs = lower_affine_value(select->lhs);
    const auto rhs = lower_affine_value(select->rhs);
    const auto true_value = lower_affine_value(select->true_value);
    const auto false_value = lower_affine_value(select->false_value);
    if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
        !false_value.has_value()) {
      return std::nullopt;
    }
    const auto predicate = evaluate_predicate(*lhs, *rhs, select->predicate);
    if (!predicate.has_value()) {
      return std::nullopt;
    }
    defined_names.push_back(select->result.name);
    affine_values.push_back(*predicate ? *true_value : *false_value);
    block.insts.push_back(*select);
  }

  auto return_value = lower_immediate_or_name(*ret->value_str, function.return_type);
  if (!return_value.has_value()) {
    return std::nullopt;
  }
  if (return_value->kind == bir::Value::Kind::Named &&
      std::find(defined_names.begin(), defined_names.end(), return_value->name) ==
          defined_names.end()) {
    return std::nullopt;
  }
  block.terminator.value = *return_value;

  function.blocks.push_back(std::move(block));
  lowered.functions.push_back(std::move(function));
  return lowered;
}

bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  auto lowered = try_lower_to_bir(module);
  if (!lowered.has_value()) {
    throw std::invalid_argument(
        "bir scaffold lowering currently supports only straight-line single-block i8/i32/i64 return-immediate/add-sub slices, constant-only mul/and/shl/lshr/sdiv/udiv/srem/urem/eq/ne/slt/sle/sgt/sge/ult/ule/ugt/uge materialization slices, bounded compare-fed integer select materialization, bounded compare-fed phi joins with empty or add/sub-only predecessor arms including join-local add/sub chains after the fused select, plus bounded one- and two-parameter affine chains over those scalar types");
  }
  return *lowered;
}

}  // namespace c4c::backend
