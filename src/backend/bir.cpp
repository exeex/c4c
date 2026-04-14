#include "bir.hpp"

#include <utility>

namespace c4c::backend::bir {

Value Value::immediate_i1(bool value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I1;
  result.immediate = value ? 1 : 0;
  result.immediate_bits = value ? 1u : 0u;
  return result;
}

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
    case TypeKind::I1:
      return "i1";
    case TypeKind::I8:
      return "i8";
    case TypeKind::I32:
      return "i32";
    case TypeKind::I64:
      return "i64";
    case TypeKind::Ptr:
      return "ptr";
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
    case BinaryOpcode::And:
      return "and";
    case BinaryOpcode::Or:
      return "or";
    case BinaryOpcode::Xor:
      return "xor";
    case BinaryOpcode::Shl:
      return "shl";
    case BinaryOpcode::LShr:
      return "lshr";
    case BinaryOpcode::AShr:
      return "ashr";
    case BinaryOpcode::SDiv:
      return "sdiv";
    case BinaryOpcode::UDiv:
      return "udiv";
    case BinaryOpcode::SRem:
      return "srem";
    case BinaryOpcode::URem:
      return "urem";
    case BinaryOpcode::Eq:
      return "eq";
    case BinaryOpcode::Ne:
      return "ne";
    case BinaryOpcode::Slt:
      return "slt";
    case BinaryOpcode::Sle:
      return "sle";
    case BinaryOpcode::Sgt:
      return "sgt";
    case BinaryOpcode::Sge:
      return "sge";
    case BinaryOpcode::Ult:
      return "ult";
    case BinaryOpcode::Ule:
      return "ule";
    case BinaryOpcode::Ugt:
      return "ugt";
    case BinaryOpcode::Uge:
      return "uge";
  }
  return "<unknown>";
}

std::string render_cast_opcode(CastOpcode opcode) {
  switch (opcode) {
    case CastOpcode::SExt:
      return "sext";
    case CastOpcode::ZExt:
      return "zext";
    case CastOpcode::Trunc:
      return "trunc";
  }
  return "<unknown>";
}

}  // namespace c4c::backend::bir
