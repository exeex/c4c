#include "bir.hpp"

#include <utility>

namespace c4c::backend::bir {

Value Value::immediate_i8(std::int8_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I8;
  result.immediate = value;
  return result;
}

Value Value::immediate_i32(std::int32_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I32;
  result.immediate = value;
  return result;
}

Value Value::immediate_i64(std::int64_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I64;
  result.immediate = value;
  return result;
}

Value Value::named(TypeKind type, std::string value_name) {
  Value result;
  result.kind = Kind::Named;
  result.type = type;
  result.name = std::move(value_name);
  return result;
}

std::string render_type(TypeKind type) {
  switch (type) {
    case TypeKind::Void:
      return "void";
    case TypeKind::I8:
      return "i8";
    case TypeKind::I32:
      return "i32";
    case TypeKind::I64:
      return "i64";
  }
  return "<unknown>";
}

std::string render_binary_opcode(BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Add:
      return "add";
    case BinaryOpcode::Sub:
      return "sub";
    case BinaryOpcode::Mul:
      return "mul";
    case BinaryOpcode::SDiv:
      return "sdiv";
    case BinaryOpcode::SRem:
      return "srem";
    case BinaryOpcode::URem:
      return "urem";
    case BinaryOpcode::Eq:
      return "eq";
  }
  return "<unknown>";
}

}  // namespace c4c::backend::bir
