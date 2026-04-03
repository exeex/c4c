#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::bir {

struct Module;

enum class TypeKind : unsigned char {
  Void,
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

  static Value immediate_i32(std::int32_t value);
  static Value immediate_i64(std::int64_t value);
  static Value named(TypeKind type, std::string name);
};

struct Param {
  TypeKind type = TypeKind::Void;
  std::string name;
};

enum class BinaryOpcode : unsigned char {
  Add,
  Sub,
};

struct BinaryInst {
  BinaryOpcode opcode = BinaryOpcode::Add;
  Value result;
  Value lhs;
  Value rhs;
};

struct ReturnTerminator {
  std::optional<Value> value;
};

struct Block {
  std::string label;
  std::vector<BinaryInst> insts;
  ReturnTerminator terminator;
};

struct Function {
  std::string name;
  TypeKind return_type = TypeKind::Void;
  std::vector<Param> params;
  std::vector<Block> blocks;
  bool is_declaration = false;
};

struct Module {
  std::string target_triple;
  std::string data_layout;
  std::vector<Function> functions;
};

std::string render_type(TypeKind type);
std::string render_binary_opcode(BinaryOpcode opcode);

}  // namespace c4c::backend::bir
