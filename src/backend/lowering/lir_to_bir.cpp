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
  if (opcode == "sdiv") {
    return bir::BinaryOpcode::SDiv;
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
  if (cmp == nullptr || cast == nullptr || cmp->is_float || cmp->predicate.str() != "eq" ||
      cmp->result.str().empty() || cast->result.str().empty() ||
      cast->kind != c4c::codegen::lir::LirCastKind::ZExt || cast->from_type.str() != "i1" ||
      cast->operand.str() != cmp->result.str()) {
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
  lowered.opcode = bir::BinaryOpcode::Eq;
  lowered.result = bir::Value::named(*type, cast->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  return lowered;
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
  if (opcode == bir::BinaryOpcode::SDiv) {
    if (rhs.constant == 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant / rhs.constant};
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
  return AffineValue{false, false, lhs.constant * rhs.constant};
}

}  // namespace

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  if (!module.globals.empty() || !module.string_pool.empty() || !module.extern_decls.empty() ||
      module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_function = module.functions.front();
  if (lir_function.is_declaration || !lir_function.alloca_insts.empty() ||
      lir_function.blocks.size() != 1 || lir_function.params.size() > 2) {
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

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *return_type;
  if (!lir_function.params.empty()) {
    for (const auto& [param_name, param_type] : lir_function.params) {
      const auto lowered_type = lower_minimal_scalar_type(param_type);
      if (!lowered_type.has_value() || param_name.empty()) {
        return std::nullopt;
      }
      function.params.push_back(bir::Param{*lowered_type, param_name});
    }
  } else if (const auto parsed_params =
                 parse_backend_function_signature_params(lir_function.signature_text);
             parsed_params.has_value()) {
    if (parsed_params->size() > 2) {
      return std::nullopt;
    }
    for (const auto& param : *parsed_params) {
      const auto lowered_type = lower_scalar_type_text(param.type);
      if (param.is_varargs || !lowered_type.has_value() || param.operand.empty()) {
        return std::nullopt;
      }
      function.params.push_back(bir::Param{*lowered_type, param.operand});
    }
  }

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
    if (!binary.has_value()) {
      return std::nullopt;
    }
    auto name_is_defined = [&](std::string_view name) {
      return std::find(defined_names.begin(), defined_names.end(), name) !=
             defined_names.end();
    };
    auto operand_is_available = [&](const bir::Value& value) {
      return value.kind != bir::Value::Kind::Named || name_is_defined(value.name);
    };
    if (!operand_is_available(binary->lhs) || !operand_is_available(binary->rhs) ||
        name_is_defined(binary->result.name)) {
      return std::nullopt;
    }
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
        "bir scaffold lowering currently supports only straight-line single-block i8/i32/i64 return-immediate/add-sub slices, constant-only mul/sdiv/srem/urem/eq materialization slices, plus bounded one- and two-parameter affine chains over those scalar types");
  }
  return *lowered;
}

}  // namespace c4c::backend
