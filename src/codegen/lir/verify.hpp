#pragma once

#include "operands.hpp"
#include "types.hpp"

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>

namespace c4c::codegen::lir {

struct LirModule;

enum class LirVerifyErrorKind {
  Malformed,
};

class LirVerifyError : public std::invalid_argument {
 public:
  LirVerifyError(LirVerifyErrorKind kind, const std::string& message)
      : std::invalid_argument(message), kind_(kind) {}

  [[nodiscard]] LirVerifyErrorKind kind() const noexcept { return kind_; }

 private:
  LirVerifyErrorKind kind_;
};

const std::string& require_operand_kind(
    const LirOperand& operand,
    std::string_view field,
    std::initializer_list<LirOperandKind> allowed_kinds,
    bool allow_empty = false);

const std::string& require_type_ref(const LirTypeRef& type,
                                    std::string_view field,
                                    bool allow_void = false);

std::string_view render_binary_opcode(const LirBinaryOpcodeRef& opcode,
                                      std::string_view field);

std::string_view render_cmp_predicate(const LirCmpPredicateRef& predicate,
                                      std::string_view field);

void verify_module(const LirModule& mod);

}  // namespace c4c::codegen::lir
