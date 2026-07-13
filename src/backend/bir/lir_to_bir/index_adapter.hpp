#pragma once

#include "../../../codegen/lir/ir.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::lir_to_bir_detail {

struct RawTypedOperandParts {
  std::string type_text;
  std::string value_text;
};

inline std::optional<RawTypedOperandParts> parse_raw_typed_operand_parts(
    std::string_view text) {
  const auto space = text.find(' ');
  if (space == std::string_view::npos || space == 0 ||
      space + 1 >= text.size()) {
    return std::nullopt;
  }
  return RawTypedOperandParts{
      .type_text = std::string(text.substr(0, space)),
      .value_text = std::string(text.substr(space + 1)),
  };
}

struct AuthoritativeGepIndexFacts {
  std::string type_text;
  c4c::codegen::lir::LirOperand operand;
};

inline std::optional<AuthoritativeGepIndexFacts>
authoritative_gep_index_facts(
    const c4c::codegen::lir::LirGepIndex& index) {
  if (!index.is_authoritative()) return std::nullopt;
  return AuthoritativeGepIndexFacts{
      .type_text = index.type_ref().str(),
      .operand = index.value(),
  };
}

template <typename RawImmediateParser, typename RawAliasResolver>
std::optional<std::int64_t> resolve_index_operand_authority_first(
    const c4c::codegen::lir::LirOperand& operand,
    RawImmediateParser&& parse_raw_immediate,
    RawAliasResolver&& resolve_raw_alias) {
  if (const auto* immediate = operand.integer_immediate()) {
    return immediate->value;
  }
  if (operand.has_authority()) return std::nullopt;

  if (operand.kind() == c4c::codegen::lir::LirOperandKind::Immediate ||
      operand.kind() == c4c::codegen::lir::LirOperandKind::SpecialToken) {
    return parse_raw_immediate(operand.str());
  }
  if (operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return std::nullopt;
  }
  return resolve_raw_alias(operand.str());
}

}  // namespace c4c::backend::lir_to_bir_detail
