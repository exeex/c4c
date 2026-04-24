#pragma once

// LIR model subheader for typed operand text.
//
// `ir.hpp` re-exports this as part of the public LIR package index. Direct
// includes are reserved for small model helpers such as `call_args.hpp` that do
// not otherwise need the full LIR instruction model.

#include <ostream>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::codegen::lir {

enum class LirOperandKind : unsigned char {
  SsaValue,
  Global,
  Label,
  Immediate,
  SpecialToken,
  RawText,
};

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

  [[nodiscard]] const std::string& str() const { return text_; }
  [[nodiscard]] std::string& str() { return text_; }
  [[nodiscard]] LirOperandKind kind() const { return kind_; }
  [[nodiscard]] bool empty() const { return text_.empty(); }

  operator std::string&() { return text_; }
  operator const std::string&() const { return text_; }
  operator std::string_view() const { return text_; }

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
};

}  // namespace c4c::codegen::lir
