#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace c4c::backend::bir {

struct Module;

enum class TypeKind : unsigned char {
  Void,
  I8,
  I32,
  I64,
};

struct Value {
  enum class Kind : unsigned char {
    Immediate,
    Named,
  };

  Kind kind = Kind::Immediate;
  TypeKind type = TypeKind::Void;
  std::int64_t immediate = 0;
  std::string name;

  static Value immediate_i8(std::int8_t value);
  static Value immediate_i32(std::int32_t value);
  static Value immediate_i64(std::int64_t value);
  static Value named(TypeKind type, std::string name);
};

inline bool operator==(const Value& lhs, const Value& rhs) {
  return lhs.kind == rhs.kind && lhs.type == rhs.type && lhs.immediate == rhs.immediate &&
         lhs.name == rhs.name;
}

inline bool operator!=(const Value& lhs, const Value& rhs) {
  return !(lhs == rhs);
}

struct Param {
  TypeKind type = TypeKind::Void;
  std::string name;
};

struct LocalSlot {
  std::string name;
  TypeKind type = TypeKind::Void;
  std::size_t size_bytes = 0;
};

struct Global {
  std::string name;
  TypeKind type = TypeKind::Void;
  bool is_extern = false;
  std::optional<Value> initializer;
};

struct StringConstant {
  std::string name;
  std::string bytes;
};

enum class BinaryOpcode : unsigned char {
  Add,
  Sub,
  Mul,
  And,
  Or,
  Xor,
  Shl,
  LShr,
  AShr,
  SDiv,
  UDiv,
  SRem,
  URem,
  Eq,
  Ne,
  Slt,
  Sle,
  Sgt,
  Sge,
  Ult,
  Ule,
  Ugt,
  Uge,
};

struct BinaryInst {
  BinaryOpcode opcode = BinaryOpcode::Add;
  Value result;
  Value lhs;
  Value rhs;
};

struct SelectInst {
  BinaryOpcode predicate = BinaryOpcode::Eq;
  Value result;
  Value lhs;
  Value rhs;
  Value true_value;
  Value false_value;
};

enum class CastOpcode : unsigned char {
  SExt,
  ZExt,
  Trunc,
};

struct CastInst {
  CastOpcode opcode = CastOpcode::SExt;
  Value result;
  Value operand;
};

struct CallInst {
  std::optional<Value> result;
  std::string callee;
  std::vector<Value> args;
  std::string return_type_name;
};

struct LoadLocalInst {
  Value result;
  std::string slot_name;
};

struct LoadGlobalInst {
  Value result;
  std::string global_name;
};

struct StoreLocalInst {
  std::string slot_name;
  Value value;
};

using Inst = std::variant<BinaryInst,
                          SelectInst,
                          CastInst,
                          CallInst,
                          LoadLocalInst,
                          LoadGlobalInst,
                          StoreLocalInst>;

struct ReturnTerminator {
  std::optional<Value> value;
};

struct BranchTerminator {
  std::string target_label;
};

struct CondBranchTerminator {
  Value condition;
  std::string true_label;
  std::string false_label;
};

enum class TerminatorKind : unsigned char {
  Return,
  Branch,
  CondBranch,
};

struct Terminator {
  TerminatorKind kind = TerminatorKind::Return;
  std::optional<Value> value;
  Value condition;
  std::string target_label;
  std::string true_label;
  std::string false_label;

  Terminator() = default;
  Terminator(const ReturnTerminator& ret) : kind(TerminatorKind::Return), value(ret.value) {}
  Terminator(const BranchTerminator& br)
      : kind(TerminatorKind::Branch), target_label(br.target_label) {}
  Terminator(const CondBranchTerminator& br)
      : kind(TerminatorKind::CondBranch),
        condition(br.condition),
        true_label(br.true_label),
        false_label(br.false_label) {}
};

struct Block {
  std::string label;
  std::vector<Inst> insts;
  Terminator terminator;
};

struct Function {
  std::string name;
  TypeKind return_type = TypeKind::Void;
  std::vector<Param> params;
  std::vector<LocalSlot> local_slots;
  std::vector<Block> blocks;
  bool is_declaration = false;
};

struct Module {
  std::string target_triple;
  std::string data_layout;
  std::vector<Global> globals;
  std::vector<StringConstant> string_constants;
  std::vector<Function> functions;
};

inline std::optional<std::int64_t> parse_i32_return_immediate(const Function& function) {
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.insts.empty() ||
      block.terminator.kind != TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != Value::Kind::Immediate ||
      block.terminator.value->type != TypeKind::I32) {
    return std::nullopt;
  }

  return block.terminator.value->immediate;
}

std::string render_type(TypeKind type);
std::string render_binary_opcode(BinaryOpcode opcode);
std::string render_cast_opcode(CastOpcode opcode);

}  // namespace c4c::backend::bir
