#pragma once

// LIR model subheader for typed LLVM-ish type text and opcode predicates.
//
// `ir.hpp` re-exports this as part of the public LIR package index. Keep direct
// includes limited to model-level helpers that do not otherwise need the full
// LIR instruction model.

#include <ostream>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "../../shared/struct_name_table.hpp"

namespace c4c::codegen::lir {

enum class LirTypeKind : unsigned char {
  Void,
  Integer,
  Floating,
  Pointer,
  Vector,
  Array,
  Struct,
  Function,
  Opaque,
  RawText,
};

class LirTypeRef {
 public:
  LirTypeRef() = default;
  LirTypeRef(const char* text) : LirTypeRef(std::string(text)) {}
  LirTypeRef(std::string text, std::optional<LirTypeKind> kind = std::nullopt)
      : text_(std::move(text)),
        kind_(kind.value_or(classify(text_))),
        integer_bit_width_(derive_integer_bit_width(text_, kind_)) {}
  LirTypeRef(std::string text, LirTypeKind kind)
      : text_(std::move(text)),
        kind_(kind),
        integer_bit_width_(derive_integer_bit_width(text_, kind_)) {}
  LirTypeRef(std::string text, LirTypeKind kind, unsigned integer_bit_width)
      : text_(std::move(text)),
        kind_(kind),
        integer_bit_width_(integer_bit_width) {}

  [[nodiscard]] static LirTypeRef integer(unsigned bit_width) {
    return LirTypeRef("i" + std::to_string(bit_width), LirTypeKind::Integer, bit_width);
  }

  [[nodiscard]] const std::string& str() const { return text_; }
  [[nodiscard]] std::string& str() { return text_; }
  [[nodiscard]] LirTypeKind kind() const { return kind_; }
  [[nodiscard]] std::optional<unsigned> integer_bit_width() const {
    return integer_bit_width_;
  }
  [[nodiscard]] StructNameId struct_name_id() const { return struct_name_id_; }
  [[nodiscard]] bool has_struct_name_id() const {
    return struct_name_id_ != kInvalidStructName;
  }
  void set_struct_name_id(StructNameId struct_name_id) {
    struct_name_id_ = struct_name_id;
  }
  [[nodiscard]] LirTypeRef with_struct_name_id(StructNameId struct_name_id) const {
    LirTypeRef copy = *this;
    copy.set_struct_name_id(struct_name_id);
    return copy;
  }
  [[nodiscard]] bool empty() const { return text_.empty(); }

  operator std::string&() { return text_; }
  operator const std::string&() const { return text_; }
  operator std::string_view() const { return text_; }

  [[nodiscard]] friend bool operator==(const LirTypeRef& lhs,
                                       const LirTypeRef& rhs) {
    return lhs.text_ == rhs.text_;
  }

  [[nodiscard]] friend bool operator==(const LirTypeRef& lhs,
                                       const std::string& rhs) {
    return lhs.text_ == rhs;
  }

  [[nodiscard]] friend bool operator==(const std::string& lhs,
                                       const LirTypeRef& rhs) {
    return lhs == rhs.text_;
  }

  [[nodiscard]] friend bool operator==(const LirTypeRef& lhs,
                                       const char* rhs) {
    return lhs.text_ == rhs;
  }

  [[nodiscard]] friend bool operator==(const char* lhs,
                                       const LirTypeRef& rhs) {
    return lhs == rhs.text_;
  }

  [[nodiscard]] friend bool operator!=(const LirTypeRef& lhs,
                                       const LirTypeRef& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const LirTypeRef& lhs,
                                       const std::string& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const std::string& lhs,
                                       const LirTypeRef& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const LirTypeRef& lhs,
                                       const char* rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const char* lhs,
                                       const LirTypeRef& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend std::string operator+(std::string lhs,
                                             const LirTypeRef& rhs) {
    lhs += rhs.text_;
    return lhs;
  }

  [[nodiscard]] friend std::string operator+(const char* lhs,
                                             const LirTypeRef& rhs) {
    return std::string(lhs) + rhs.text_;
  }

  friend std::ostream& operator<<(std::ostream& os, const LirTypeRef& type) {
    os << type.text_;
    return os;
  }

 private:
  [[nodiscard]] static std::optional<unsigned> parse_integer_bit_width(
      std::string_view text) {
    if (text.size() <= 1 || text.front() != 'i') {
      return std::nullopt;
    }

    unsigned value = 0;
    for (std::size_t index = 1; index < text.size(); ++index) {
      const char ch = text[index];
      if (ch < '0' || ch > '9') {
        return std::nullopt;
      }
      value = (value * 10u) + static_cast<unsigned>(ch - '0');
    }
    return value;
  }

  [[nodiscard]] static std::optional<unsigned> derive_integer_bit_width(
      std::string_view text,
      LirTypeKind kind) {
    if (kind != LirTypeKind::Integer) {
      return std::nullopt;
    }
    return parse_integer_bit_width(text);
  }

  [[nodiscard]] static LirTypeKind classify(std::string_view text) {
    if (text.empty()) return LirTypeKind::RawText;
    if (text == "void") return LirTypeKind::Void;
    if (text == "ptr") return LirTypeKind::Pointer;
    if (parse_integer_bit_width(text).has_value()) return LirTypeKind::Integer;
    if (text == "half" || text == "float" || text == "double" ||
        text == "fp128" || text == "x86_fp80") {
      return LirTypeKind::Floating;
    }
    if (text.front() == '<') return LirTypeKind::Vector;
    if (text.front() == '[') return LirTypeKind::Array;
    if (text.front() == '{' || text.rfind("%struct.", 0) == 0) {
      return LirTypeKind::Struct;
    }
    if (text.find('(') != std::string_view::npos &&
        text.find(')') != std::string_view::npos) {
      return LirTypeKind::Function;
    }
    if (text.rfind("%", 0) == 0) return LirTypeKind::Opaque;
    return LirTypeKind::RawText;
  }

  std::string text_;
  LirTypeKind kind_ = LirTypeKind::RawText;
  std::optional<unsigned> integer_bit_width_;
  StructNameId struct_name_id_ = kInvalidStructName;
};

enum class LirBinaryOpcode : unsigned char {
  Add,
  Sub,
  Mul,
  SDiv,
  UDiv,
  SRem,
  URem,
  FAdd,
  FSub,
  FMul,
  FDiv,
  FRem,
  And,
  Or,
  Xor,
  Shl,
  LShr,
  AShr,
  FNeg,
};

class LirBinaryOpcodeRef {
 public:
  LirBinaryOpcodeRef() = default;
  LirBinaryOpcodeRef(const char* text) : LirBinaryOpcodeRef(std::string(text)) {}
  LirBinaryOpcodeRef(std::string text)
      : text_(std::move(text)), opcode_(parse(text_)) {}
  LirBinaryOpcodeRef(LirBinaryOpcode opcode)
      : text_(to_string(opcode)), opcode_(opcode) {}

  [[nodiscard]] const std::string& str() const { return text_; }
  [[nodiscard]] std::optional<LirBinaryOpcode> typed() const { return opcode_; }

  operator const std::string&() const { return text_; }
  operator std::string_view() const { return text_; }

  [[nodiscard]] friend bool operator==(const LirBinaryOpcodeRef& lhs,
                                       const std::string& rhs) {
    return lhs.text_ == rhs;
  }

  [[nodiscard]] friend bool operator==(const LirBinaryOpcodeRef& lhs,
                                       const char* rhs) {
    return lhs.text_ == rhs;
  }

  [[nodiscard]] friend bool operator!=(const LirBinaryOpcodeRef& lhs,
                                       const std::string& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const LirBinaryOpcodeRef& lhs,
                                       const char* rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend std::string operator+(std::string lhs,
                                             const LirBinaryOpcodeRef& rhs) {
    lhs += rhs.text_;
    return lhs;
  }

  [[nodiscard]] friend std::string operator+(const char* lhs,
                                             const LirBinaryOpcodeRef& rhs) {
    return std::string(lhs) + rhs.text_;
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const LirBinaryOpcodeRef& opcode) {
    os << opcode.text_;
    return os;
  }

 private:
  [[nodiscard]] static std::optional<LirBinaryOpcode> parse(std::string_view text) {
    if (text == "add") return LirBinaryOpcode::Add;
    if (text == "sub") return LirBinaryOpcode::Sub;
    if (text == "mul") return LirBinaryOpcode::Mul;
    if (text == "sdiv") return LirBinaryOpcode::SDiv;
    if (text == "udiv") return LirBinaryOpcode::UDiv;
    if (text == "srem") return LirBinaryOpcode::SRem;
    if (text == "urem") return LirBinaryOpcode::URem;
    if (text == "fadd") return LirBinaryOpcode::FAdd;
    if (text == "fsub") return LirBinaryOpcode::FSub;
    if (text == "fmul") return LirBinaryOpcode::FMul;
    if (text == "fdiv") return LirBinaryOpcode::FDiv;
    if (text == "frem") return LirBinaryOpcode::FRem;
    if (text == "and") return LirBinaryOpcode::And;
    if (text == "or") return LirBinaryOpcode::Or;
    if (text == "xor") return LirBinaryOpcode::Xor;
    if (text == "shl") return LirBinaryOpcode::Shl;
    if (text == "lshr") return LirBinaryOpcode::LShr;
    if (text == "ashr") return LirBinaryOpcode::AShr;
    if (text == "fneg") return LirBinaryOpcode::FNeg;
    return std::nullopt;
  }

  [[nodiscard]] static std::string to_string(LirBinaryOpcode opcode) {
    switch (opcode) {
      case LirBinaryOpcode::Add: return "add";
      case LirBinaryOpcode::Sub: return "sub";
      case LirBinaryOpcode::Mul: return "mul";
      case LirBinaryOpcode::SDiv: return "sdiv";
      case LirBinaryOpcode::UDiv: return "udiv";
      case LirBinaryOpcode::SRem: return "srem";
      case LirBinaryOpcode::URem: return "urem";
      case LirBinaryOpcode::FAdd: return "fadd";
      case LirBinaryOpcode::FSub: return "fsub";
      case LirBinaryOpcode::FMul: return "fmul";
      case LirBinaryOpcode::FDiv: return "fdiv";
      case LirBinaryOpcode::FRem: return "frem";
      case LirBinaryOpcode::And: return "and";
      case LirBinaryOpcode::Or: return "or";
      case LirBinaryOpcode::Xor: return "xor";
      case LirBinaryOpcode::Shl: return "shl";
      case LirBinaryOpcode::LShr: return "lshr";
      case LirBinaryOpcode::AShr: return "ashr";
      case LirBinaryOpcode::FNeg: return "fneg";
    }
    return {};
  }

  std::string text_;
  std::optional<LirBinaryOpcode> opcode_;
};

enum class LirCmpPredicate : unsigned char {
  Eq,
  Ne,
  Sgt,
  Sge,
  Slt,
  Sle,
  Ugt,
  Uge,
  Ult,
  Ule,
  OEq,
  ONe,
  OGt,
  OGe,
  OLt,
  OLe,
  UEq,
  UNe,
  Ord,
  Uno,
};

class LirCmpPredicateRef {
 public:
  LirCmpPredicateRef() = default;
  LirCmpPredicateRef(const char* text) : LirCmpPredicateRef(std::string(text)) {}
  LirCmpPredicateRef(std::string text)
      : text_(std::move(text)), predicate_(parse(text_)) {}
  LirCmpPredicateRef(LirCmpPredicate predicate)
      : text_(to_string(predicate)), predicate_(predicate) {}

  [[nodiscard]] const std::string& str() const { return text_; }
  [[nodiscard]] std::optional<LirCmpPredicate> typed() const { return predicate_; }

  operator const std::string&() const { return text_; }
  operator std::string_view() const { return text_; }

  [[nodiscard]] friend bool operator==(const LirCmpPredicateRef& lhs,
                                       const std::string& rhs) {
    return lhs.text_ == rhs;
  }

  [[nodiscard]] friend bool operator==(const LirCmpPredicateRef& lhs,
                                       const char* rhs) {
    return lhs.text_ == rhs;
  }

  [[nodiscard]] friend bool operator!=(const LirCmpPredicateRef& lhs,
                                       const std::string& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend bool operator!=(const LirCmpPredicateRef& lhs,
                                       const char* rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] friend std::string operator+(std::string lhs,
                                             const LirCmpPredicateRef& rhs) {
    lhs += rhs.text_;
    return lhs;
  }

  [[nodiscard]] friend std::string operator+(const char* lhs,
                                             const LirCmpPredicateRef& rhs) {
    return std::string(lhs) + rhs.text_;
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const LirCmpPredicateRef& predicate) {
    os << predicate.text_;
    return os;
  }

 private:
  [[nodiscard]] static std::optional<LirCmpPredicate> parse(std::string_view text) {
    if (text == "eq") return LirCmpPredicate::Eq;
    if (text == "ne") return LirCmpPredicate::Ne;
    if (text == "sgt") return LirCmpPredicate::Sgt;
    if (text == "sge") return LirCmpPredicate::Sge;
    if (text == "slt") return LirCmpPredicate::Slt;
    if (text == "sle") return LirCmpPredicate::Sle;
    if (text == "ugt") return LirCmpPredicate::Ugt;
    if (text == "uge") return LirCmpPredicate::Uge;
    if (text == "ult") return LirCmpPredicate::Ult;
    if (text == "ule") return LirCmpPredicate::Ule;
    if (text == "oeq") return LirCmpPredicate::OEq;
    if (text == "one") return LirCmpPredicate::ONe;
    if (text == "ogt") return LirCmpPredicate::OGt;
    if (text == "oge") return LirCmpPredicate::OGe;
    if (text == "olt") return LirCmpPredicate::OLt;
    if (text == "ole") return LirCmpPredicate::OLe;
    if (text == "ueq") return LirCmpPredicate::UEq;
    if (text == "une") return LirCmpPredicate::UNe;
    if (text == "ord") return LirCmpPredicate::Ord;
    if (text == "uno") return LirCmpPredicate::Uno;
    return std::nullopt;
  }

  [[nodiscard]] static std::string to_string(LirCmpPredicate predicate) {
    switch (predicate) {
      case LirCmpPredicate::Eq: return "eq";
      case LirCmpPredicate::Ne: return "ne";
      case LirCmpPredicate::Sgt: return "sgt";
      case LirCmpPredicate::Sge: return "sge";
      case LirCmpPredicate::Slt: return "slt";
      case LirCmpPredicate::Sle: return "sle";
      case LirCmpPredicate::Ugt: return "ugt";
      case LirCmpPredicate::Uge: return "uge";
      case LirCmpPredicate::Ult: return "ult";
      case LirCmpPredicate::Ule: return "ule";
      case LirCmpPredicate::OEq: return "oeq";
      case LirCmpPredicate::ONe: return "one";
      case LirCmpPredicate::OGt: return "ogt";
      case LirCmpPredicate::OGe: return "oge";
      case LirCmpPredicate::OLt: return "olt";
      case LirCmpPredicate::OLe: return "ole";
      case LirCmpPredicate::UEq: return "ueq";
      case LirCmpPredicate::UNe: return "une";
      case LirCmpPredicate::Ord: return "ord";
      case LirCmpPredicate::Uno: return "uno";
    }
    return {};
  }

  std::string text_;
  std::optional<LirCmpPredicate> predicate_;
};

}  // namespace c4c::codegen::lir
