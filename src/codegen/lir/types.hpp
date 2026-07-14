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
#include <utility>

#include "../../shared/struct_name_table.hpp"

namespace c4c::codegen::lir {

enum class LirTypeKind : unsigned char {
  Void,
  Integer,
  Floating,
  Pointer,
  Vector,
  VrmRegister,
  Array,
  Struct,
  Function,
  Opaque,
  RawText,
};

// Closed scalar spellings that can carry semantic identity independently of
// their rendered LLVM text. Other LIR type forms remain runtime text-backed.
enum class LirBuiltinType : unsigned char {
  Void,
  Pointer,
  I1,
  I8,
  I16,
  I32,
  I64,
  I128,
  Half,
  Float,
  Double,
  Fp128,
  X86Fp80,
};

class LirTypeRef {
 public:
  LirTypeRef() = default;
  LirTypeRef(const char* text) : LirTypeRef(std::string(text)) {}
  LirTypeRef(std::string text, std::optional<LirTypeKind> kind = std::nullopt)
      : text_(std::move(text)),
        kind_(kind.value_or(classify(text_))),
        integer_bit_width_(derive_integer_bit_width(text_, kind_)),
        vrm_width_(derive_vrm_width(text_, kind_)),
        builtin_type_(builtin_type_for(text_, kind_)) {}
  LirTypeRef(std::string text, LirTypeKind kind)
      : text_(std::move(text)),
        kind_(kind),
        integer_bit_width_(derive_integer_bit_width(text_, kind_)),
        vrm_width_(derive_vrm_width(text_, kind_)),
        builtin_type_(builtin_type_for(text_, kind_)) {}
  LirTypeRef(std::string text, LirTypeKind kind, unsigned integer_bit_width)
      : text_(std::move(text)),
        kind_(kind),
        integer_bit_width_(integer_bit_width),
        builtin_type_(builtin_type_for(text_, kind_)) {}
  LirTypeRef(LirBuiltinType builtin_type)
      : text_(builtin_text(builtin_type)),
        kind_(builtin_kind(builtin_type)),
        integer_bit_width_(builtin_integer_bit_width(builtin_type)),
        builtin_type_(builtin_type) {}

  [[nodiscard]] static LirTypeRef integer(unsigned bit_width) {
    return LirTypeRef("i" + std::to_string(bit_width), LirTypeKind::Integer, bit_width);
  }

  [[nodiscard]] static LirTypeRef runtime_text(std::string text) {
    return LirTypeRef(std::move(text));
  }

  // Parsed LIR call argument types are compatibility text re-owned from input,
  // not a closed set of builtins. Keep this deprecated boundary local so builds
  // inventory the remaining parser/runtime-text constructions without warning
  // on unrelated LirTypeRef users.
  [[nodiscard, deprecated(
      "parsed/re-owned LIR typed-call argument text: audit this compatibility boundary")]]
  static LirTypeRef parsed_typed_call_argument_text(std::string text) {
    return runtime_text(std::move(text));
  }

  // Parsed LIR call return types are re-owned runtime text from input, not a
  // closed set of builtins. Dynamic aggregate, vector, struct, and function
  // spellings must remain supported through this local compatibility boundary.
  [[nodiscard, deprecated(
      "parsed/re-owned LIR typed-call return text: audit this compatibility boundary")]]
  static LirTypeRef parsed_typed_call_return_text(std::string text) {
    return runtime_text(std::move(text));
  }

  // Extern declarations retain return-type text from their external/stored
  // declaration payload. This is runtime text, not a closed builtin set: it
  // may spell dynamic aggregate, vector, struct, or function returns.
  [[nodiscard, deprecated(
      "stored/re-owned extern-declaration return text: audit this runtime-text compatibility boundary")]]
  static LirTypeRef stored_extern_declaration_return_text(std::string text) {
    return runtime_text(std::move(text));
  }

  [[nodiscard]] static LirTypeRef vrm_register(unsigned width) {
    LirTypeRef type("c4c.vrm" + std::to_string(width), LirTypeKind::VrmRegister);
    type.vrm_width_ = width;
    return type;
  }

  [[nodiscard]] static LirTypeRef struct_type(std::string rendered_text,
                                              StructNameId struct_name_id) {
    LirTypeRef type(std::move(rendered_text), LirTypeKind::Struct);
    type.set_struct_name_id(struct_name_id);
    return type;
  }

  [[nodiscard]] static LirTypeRef union_type(std::string rendered_text,
                                             StructNameId struct_name_id) {
    return struct_type(std::move(rendered_text), struct_name_id);
  }

  [[nodiscard]] const std::string& str() const { return text_; }
  [[nodiscard]] std::string& str() { return text_; }
  [[nodiscard]] LirTypeKind kind() const { return kind_; }
  [[nodiscard]] std::optional<LirBuiltinType> builtin_type() const {
    return builtin_type_;
  }
  [[nodiscard]] std::optional<unsigned> integer_bit_width() const {
    return integer_bit_width_;
  }
  [[nodiscard]] std::optional<unsigned> vrm_width() const { return vrm_width_; }
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
    if (lhs.has_struct_name_id() && rhs.has_struct_name_id()) {
      return lhs.struct_name_id_ == rhs.struct_name_id_;
    }
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
  [[nodiscard]] static std::string builtin_text(LirBuiltinType builtin_type) {
    switch (builtin_type) {
      case LirBuiltinType::Void: return "void";
      case LirBuiltinType::Pointer: return "ptr";
      case LirBuiltinType::I1: return "i1";
      case LirBuiltinType::I8: return "i8";
      case LirBuiltinType::I16: return "i16";
      case LirBuiltinType::I32: return "i32";
      case LirBuiltinType::I64: return "i64";
      case LirBuiltinType::I128: return "i128";
      case LirBuiltinType::Half: return "half";
      case LirBuiltinType::Float: return "float";
      case LirBuiltinType::Double: return "double";
      case LirBuiltinType::Fp128: return "fp128";
      case LirBuiltinType::X86Fp80: return "x86_fp80";
    }
    return {};
  }

  [[nodiscard]] static LirTypeKind builtin_kind(LirBuiltinType builtin_type) {
    switch (builtin_type) {
      case LirBuiltinType::Void: return LirTypeKind::Void;
      case LirBuiltinType::Pointer: return LirTypeKind::Pointer;
      case LirBuiltinType::I1:
      case LirBuiltinType::I8:
      case LirBuiltinType::I16:
      case LirBuiltinType::I32:
      case LirBuiltinType::I64:
      case LirBuiltinType::I128: return LirTypeKind::Integer;
      case LirBuiltinType::Half:
      case LirBuiltinType::Float:
      case LirBuiltinType::Double:
      case LirBuiltinType::Fp128:
      case LirBuiltinType::X86Fp80: return LirTypeKind::Floating;
    }
    return LirTypeKind::RawText;
  }

  [[nodiscard]] static std::optional<unsigned> builtin_integer_bit_width(
      LirBuiltinType builtin_type) {
    switch (builtin_type) {
      case LirBuiltinType::I1: return 1;
      case LirBuiltinType::I8: return 8;
      case LirBuiltinType::I16: return 16;
      case LirBuiltinType::I32: return 32;
      case LirBuiltinType::I64: return 64;
      case LirBuiltinType::I128: return 128;
      default: return std::nullopt;
    }
  }

  [[nodiscard]] static std::optional<LirBuiltinType> builtin_from_text(
      std::string_view text) {
    if (text == "void") return LirBuiltinType::Void;
    if (text == "ptr") return LirBuiltinType::Pointer;
    if (text == "i1") return LirBuiltinType::I1;
    if (text == "i8") return LirBuiltinType::I8;
    if (text == "i16") return LirBuiltinType::I16;
    if (text == "i32") return LirBuiltinType::I32;
    if (text == "i64") return LirBuiltinType::I64;
    if (text == "i128") return LirBuiltinType::I128;
    if (text == "half") return LirBuiltinType::Half;
    if (text == "float") return LirBuiltinType::Float;
    if (text == "double") return LirBuiltinType::Double;
    if (text == "fp128") return LirBuiltinType::Fp128;
    if (text == "x86_fp80") return LirBuiltinType::X86Fp80;
    return std::nullopt;
  }

  [[nodiscard]] static std::optional<LirBuiltinType> builtin_type_for(
      std::string_view text, LirTypeKind kind) {
    const auto builtin_type = builtin_from_text(text);
    if (builtin_type.has_value() && builtin_kind(*builtin_type) == kind) {
      return builtin_type;
    }
    return std::nullopt;
  }

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

  [[nodiscard]] static std::optional<unsigned> parse_vrm_width(
      std::string_view text) {
    constexpr std::string_view prefix = "c4c.vrm";
    if (text.rfind(prefix, 0) != 0 || text.size() <= prefix.size()) {
      return std::nullopt;
    }

    unsigned value = 0;
    for (std::size_t index = prefix.size(); index < text.size(); ++index) {
      const char ch = text[index];
      if (ch < '0' || ch > '9') return std::nullopt;
      value = (value * 10u) + static_cast<unsigned>(ch - '0');
    }
    switch (value) {
      case 1:
      case 2:
      case 4:
      case 8:
        return value;
      default:
        return std::nullopt;
    }
  }

  [[nodiscard]] static std::optional<unsigned> derive_vrm_width(
      std::string_view text,
      LirTypeKind kind) {
    if (kind != LirTypeKind::VrmRegister) {
      return std::nullopt;
    }
    return parse_vrm_width(text);
  }

  [[nodiscard]] static LirTypeKind classify(std::string_view text) {
    if (text.empty()) return LirTypeKind::RawText;
    if (text == "void") return LirTypeKind::Void;
    if (text == "ptr") return LirTypeKind::Pointer;
    if (parse_integer_bit_width(text).has_value()) return LirTypeKind::Integer;
    if (parse_vrm_width(text).has_value()) return LirTypeKind::VrmRegister;
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
  std::optional<unsigned> vrm_width_;
  std::optional<LirBuiltinType> builtin_type_;
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
