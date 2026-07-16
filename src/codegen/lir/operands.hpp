#pragma once

// LIR model subheader for typed operand presentation and authority.
//
// `ir.hpp` re-exports this as part of the public LIR package index. Direct
// includes are reserved for small model helpers such as `call_args.hpp` that do
// not otherwise need the full LIR instruction model.

#include <ostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "identity.hpp"

namespace c4c::codegen::lir {

enum class LirOperandKind : unsigned char {
  SsaValue,
  DirectConstant,
  Global,
  Label,
  Immediate,
  SpecialToken,
  RawText,
};

struct LirIntegerImmediate {
  long long value = 0;
};

[[nodiscard]] constexpr bool operator==(LirIntegerImmediate lhs,
                                        LirIntegerImmediate rhs) {
  return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(LirIntegerImmediate lhs,
                                        LirIntegerImmediate rhs) {
  return !(lhs == rhs);
}

// Native identity for the existing classified LLVM special-token vocabulary.
// The spelling is retained only as a printer compatibility mirror.
enum class LirSpecialToken : unsigned char {
  Null,
  Undef,
  Poison,
  ZeroInitializer,
  True,
  False,
};

[[nodiscard]] constexpr std::string_view lir_special_token_spelling(
    LirSpecialToken token) {
  switch (token) {
    case LirSpecialToken::Null: return "null";
    case LirSpecialToken::Undef: return "undef";
    case LirSpecialToken::Poison: return "poison";
    case LirSpecialToken::ZeroInitializer: return "zeroinitializer";
    case LirSpecialToken::True: return "true";
    case LirSpecialToken::False: return "false";
  }
  return {};
}

using LirOperandAuthority =
    std::variant<std::monostate, LirValueId, LinkNameId,
                 LirIntegerImmediate, LirSpecialToken>;

class LirOperand {
 public:
  LirOperand() = default;
  LirOperand(const char* text) : LirOperand(std::string(text)) {}
  LirOperand(std::string text,
             std::optional<LirOperandKind> kind = std::nullopt)
      : text_(std::move(text)),
        kind_(kind.value_or(classify(text_))) {}

  [[nodiscard]] static LirOperand raw(std::string text) {
    return LirOperand(std::move(text), LirOperandKind::RawText);
  }

  [[nodiscard]] static LirOperand ssa(std::string display, LirValueId id) {
    return LirOperand(std::move(display), LirOperandKind::SsaValue, id);
  }

  // A function-owned non-instruction constant.  Its display spelling is
  // deliberately not authoritative: legal consumers resolve the value ID
  // through the enclosing function's direct-constant table.
  [[nodiscard]] static LirOperand direct_constant(LirValueId id) {
    return LirOperand({}, LirOperandKind::DirectConstant, id);
  }

  [[nodiscard]] static LirOperand global(std::string display, LinkNameId id) {
    return LirOperand(std::move(display), LirOperandKind::Global, id);
  }

  [[nodiscard]] static LirOperand integer(std::string display,
                                          long long value) {
    return LirOperand(std::move(display), LirOperandKind::Immediate,
                      LirIntegerImmediate{value});
  }

  [[nodiscard]] static LirOperand special_token(LirSpecialToken token) {
    return special_token(std::string(lir_special_token_spelling(token)), token);
  }

  // `display` is deliberately accepted as a compatibility mirror so the
  // verifier can reject stale or misleading renderings without making text a
  // source of semantic identity.
  [[nodiscard]] static LirOperand special_token(std::string display,
                                                LirSpecialToken token,
                                                LirOperandKind kind =
                                                    LirOperandKind::SpecialToken) {
    return LirOperand(std::move(display), kind, token);
  }

  [[nodiscard]] const std::string& str() const { return text_; }
  [[nodiscard]] std::string& str() { return text_; }
  [[nodiscard]] LirOperandKind kind() const { return kind_; }
  [[nodiscard]] bool empty() const { return text_.empty(); }
  [[nodiscard]] bool has_authority() const {
    return !std::holds_alternative<std::monostate>(authority_);
  }
  [[nodiscard]] const LirOperandAuthority& authority() const {
    return authority_;
  }
  [[nodiscard]] const LirValueId* value_id() const {
    return std::get_if<LirValueId>(&authority_);
  }
  [[nodiscard]] const LinkNameId* link_name_id() const {
    return std::get_if<LinkNameId>(&authority_);
  }
  [[nodiscard]] const LirIntegerImmediate* integer_immediate() const {
    return std::get_if<LirIntegerImmediate>(&authority_);
  }
  [[nodiscard]] const LirSpecialToken* special_token() const {
    return std::get_if<LirSpecialToken>(&authority_);
  }
  [[nodiscard]] std::optional<bool> same_authority_as(
      const LirOperand& other) const {
    if (!has_authority() || !other.has_authority()) return std::nullopt;
    return authority_ == other.authority_;
  }

  [[nodiscard]] friend bool operator==(const LirOperand& lhs,
                                       const LirOperand& rhs) {
    return lhs.text_ == rhs.text_;
  }

  [[nodiscard]] friend bool operator==(const LirOperand& lhs,
                                       const std::string& rhs) {
    return lhs.text_ == rhs;
  }

  [[nodiscard]] friend bool operator==(const std::string& lhs,
                                       const LirOperand& rhs) {
    return lhs == rhs.text_;
  }

  [[nodiscard]] friend bool operator==(const LirOperand& lhs,
                                       const char* rhs) {
    return lhs.text_ == rhs;
  }

  [[nodiscard]] friend bool operator==(const char* lhs,
                                       const LirOperand& rhs) {
    return lhs == rhs.text_;
  }

  [[nodiscard]] friend bool operator!=(const LirOperand& lhs,
                                       const LirOperand& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const LirOperand& lhs,
                                       const std::string& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const std::string& lhs,
                                       const LirOperand& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const LirOperand& lhs,
                                       const char* rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const char* lhs,
                                       const LirOperand& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend std::string operator+(std::string lhs,
                                             const LirOperand& rhs) {
    lhs += rhs.text_;
    return lhs;
  }

  [[nodiscard]] friend std::string operator+(const char* lhs,
                                             const LirOperand& rhs) {
    return std::string(lhs) + rhs.text_;
  }

  friend std::ostream& operator<<(std::ostream& os, const LirOperand& operand) {
    os << operand.text_;
    return os;
  }

 private:
  LirOperand(std::string text, LirOperandKind kind,
             LirOperandAuthority authority)
      : text_(std::move(text)), kind_(kind), authority_(std::move(authority)) {}

  [[nodiscard]] static bool is_special_token(std::string_view text) {
    return text == "null" || text == "undef" || text == "poison" ||
           text == "zeroinitializer" || text == "true" || text == "false";
  }

  [[nodiscard]] static bool looks_numeric(std::string_view text) {
    if (text.empty()) return false;
    std::size_t index = (text.front() == '-' || text.front() == '+') ? 1 : 0;
    bool saw_digit = false;
    bool saw_decimal = false;
    for (; index < text.size(); ++index) {
      const char ch = text[index];
      if (ch >= '0' && ch <= '9') {
        saw_digit = true;
        continue;
      }
      if (ch == '.' && !saw_decimal) {
        saw_decimal = true;
        continue;
      }
      return false;
    }
    return saw_digit;
  }

  [[nodiscard]] static LirOperandKind classify(std::string_view text) {
    if (text.empty()) return LirOperandKind::RawText;
    if (text.front() == '%') return LirOperandKind::SsaValue;
    if (text.front() == '@') return LirOperandKind::Global;
    if (is_special_token(text)) return LirOperandKind::SpecialToken;
    if (looks_numeric(text)) return LirOperandKind::Immediate;
    if (text.find(' ') != std::string_view::npos ||
        text.find(',') != std::string_view::npos ||
        text.find('(') != std::string_view::npos) {
      return LirOperandKind::RawText;
    }
    if (text.rfind("label", 0) == 0 || text.rfind(".L", 0) == 0 ||
        text.rfind("block_", 0) == 0 || text.rfind("%ulbl_", 0) == 0) {
      return LirOperandKind::Label;
    }
    return LirOperandKind::RawText;
  }

  std::string text_;
  LirOperandKind kind_ = LirOperandKind::RawText;
  LirOperandAuthority authority_{};
};

}  // namespace c4c::codegen::lir
