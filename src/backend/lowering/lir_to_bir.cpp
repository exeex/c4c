#include "lir_to_bir.hpp"
#include "call_decode.hpp"

#include <charconv>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend {

namespace {

bool is_minimal_i32_param_type(const c4c::TypeSpec& type) {
  return type.base == TB_INT && type.ptr_level == 0 && type.array_rank == 0;
}

std::optional<std::int32_t> parse_i32_immediate(std::string_view text) {
  std::int32_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<bir::Value> lower_immediate_or_name(std::string_view value_text) {
  if (value_text.empty()) {
    return std::nullopt;
  }
  if (value_text.front() == '%') {
    return bir::Value::named(bir::TypeKind::I32, std::string(value_text));
  }
  const auto immediate = parse_i32_immediate(value_text);
  if (!immediate.has_value()) {
    return std::nullopt;
  }
  return bir::Value::immediate_i32(*immediate);
}

std::optional<bir::BinaryOpcode> lower_binary_opcode(std::string_view opcode) {
  if (opcode == "add") {
    return bir::BinaryOpcode::Add;
  }
  if (opcode == "sub") {
    return bir::BinaryOpcode::Sub;
  }
  return std::nullopt;
}

std::optional<bir::BinaryInst> lower_binary(const c4c::codegen::lir::LirInst& inst) {
  const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
  if (bin == nullptr || bin->type_str.str() != "i32" || bin->result.str().empty()) {
    return std::nullopt;
  }
  const auto opcode = lower_binary_opcode(bin->opcode.str());
  if (!opcode.has_value()) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(bin->lhs.str());
  const auto rhs = lower_immediate_or_name(bin->rhs.str());
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  bir::BinaryInst lowered;
  lowered.opcode = *opcode;
  lowered.result = bir::Value::named(bir::TypeKind::I32, bin->result.str());
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
  return AffineValue{lhs.uses_first_param, lhs.uses_second_param,
                     lhs.constant - rhs.constant};
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
  if (ret == nullptr || ret->type_str != "i32" || !ret->value_str.has_value()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I32;
  if (!lir_function.params.empty()) {
    for (const auto& [param_name, param_type] : lir_function.params) {
      if (!is_minimal_i32_param_type(param_type) || param_name.empty()) {
        return std::nullopt;
      }
      function.params.push_back(bir::Param{bir::TypeKind::I32, param_name});
    }
  } else if (const auto parsed_params =
                 parse_backend_function_signature_params(lir_function.signature_text);
             parsed_params.has_value()) {
    if (parsed_params->size() > 2) {
      return std::nullopt;
    }
    for (const auto& param : *parsed_params) {
      if (param.is_varargs || param.type != "i32" || param.operand.empty()) {
        return std::nullopt;
      }
      function.params.push_back(bir::Param{bir::TypeKind::I32, param.operand});
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
  for (const auto& lir_inst : lir_block.insts) {
    auto binary = lower_binary(lir_inst);
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

  auto return_value = lower_immediate_or_name(*ret->value_str);
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
        "bir scaffold lowering currently supports only straight-line single-block i32 return-immediate/add-sub slices plus bounded one- and two-parameter affine i32 chains");
  }
  return *lowered;
}

}  // namespace c4c::backend
