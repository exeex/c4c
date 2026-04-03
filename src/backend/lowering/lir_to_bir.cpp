#include "lir_to_bir.hpp"

#include <charconv>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace c4c::backend {

namespace {

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

std::optional<bir::BinaryInst> lower_binary(const c4c::codegen::lir::LirInst& inst) {
  const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
  if (bin == nullptr || bin->opcode.str() != "add" || bin->type_str.str() != "i32" ||
      bin->result.str().empty()) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(bin->lhs.str());
  const auto rhs = lower_immediate_or_name(bin->rhs.str());
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  bir::BinaryInst lowered;
  lowered.opcode = bir::BinaryOpcode::Add;
  lowered.result = bir::Value::named(bir::TypeKind::I32, bin->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  return lowered;
}

}  // namespace

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  if (!module.globals.empty() || !module.string_pool.empty() || !module.extern_decls.empty() ||
      module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_function = module.functions.front();
  if (lir_function.is_declaration || !lir_function.alloca_insts.empty() ||
      lir_function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_block = lir_function.blocks.front();
  if (lir_block.label.empty() || lir_block.insts.size() > 1) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&lir_block.terminator);
  if (ret == nullptr || ret->type_str != "i32" || !ret->value_str.has_value()) {
    return std::nullopt;
  }

  auto return_value = lower_immediate_or_name(*ret->value_str);
  if (!return_value.has_value()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I32;

  bir::Block block;
  block.label = lir_block.label;
  if (!lir_block.insts.empty()) {
    auto binary = lower_binary(lir_block.insts.front());
    if (!binary.has_value() || return_value->kind != bir::Value::Kind::Named ||
        return_value->name != binary->result.name) {
      return std::nullopt;
    }
    block.insts.push_back(*binary);
  } else if (return_value->kind == bir::Value::Kind::Named) {
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
        "bir scaffold lowering currently supports only single-block i32 return-immediate/add slices");
  }
  return *lowered;
}

}  // namespace c4c::backend
