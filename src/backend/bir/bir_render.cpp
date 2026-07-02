#include "bir.hpp"

namespace c4c::backend::bir {

std::string render_type(TypeKind type) {
  switch (type) {
    case TypeKind::Void:
      return "void";
    case TypeKind::I1:
      return "i1";
    case TypeKind::I8:
      return "i8";
    case TypeKind::I16:
      return "i16";
    case TypeKind::I32:
      return "i32";
    case TypeKind::I64:
      return "i64";
    case TypeKind::Ptr:
      return "ptr";
    case TypeKind::F32:
      return "float";
    case TypeKind::F64:
      return "double";
    case TypeKind::F128:
      return "f128";
    case TypeKind::Vrm1:
      return "c4c.vrm1";
    case TypeKind::Vrm2:
      return "c4c.vrm2";
    case TypeKind::Vrm4:
      return "c4c.vrm4";
    case TypeKind::Vrm8:
      return "c4c.vrm8";
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
    case CastOpcode::FPTrunc:
      return "fptrunc";
    case CastOpcode::FPExt:
      return "fpext";
    case CastOpcode::FPToSI:
      return "fptosi";
    case CastOpcode::FPToUI:
      return "fptoui";
    case CastOpcode::SIToFP:
      return "sitofp";
    case CastOpcode::UIToFP:
      return "uitofp";
    case CastOpcode::PtrToInt:
      return "ptrtoint";
    case CastOpcode::IntToPtr:
      return "inttoptr";
    case CastOpcode::Bitcast:
      return "bitcast";
  }
  return "<unknown>";
}

}  // namespace c4c::backend::bir
