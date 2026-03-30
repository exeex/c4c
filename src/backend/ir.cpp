#include "ir.hpp"

namespace c4c::backend {

const char* backend_binary_opcode_name(BackendBinaryOpcode opcode) {
  switch (opcode) {
    case BackendBinaryOpcode::Add:
      return "add";
    case BackendBinaryOpcode::Sub:
      return "sub";
    case BackendBinaryOpcode::Mul:
      return "mul";
    case BackendBinaryOpcode::SDiv:
      return "sdiv";
    case BackendBinaryOpcode::SRem:
      return "srem";
  }
  return "unknown";
}

const char* backend_compare_predicate_name(BackendComparePredicate predicate) {
  switch (predicate) {
    case BackendComparePredicate::Slt:
      return "slt";
    case BackendComparePredicate::Sle:
      return "sle";
    case BackendComparePredicate::Sgt:
      return "sgt";
    case BackendComparePredicate::Sge:
      return "sge";
    case BackendComparePredicate::Eq:
      return "eq";
    case BackendComparePredicate::Ne:
      return "ne";
    case BackendComparePredicate::Ult:
      return "ult";
    case BackendComparePredicate::Ule:
      return "ule";
    case BackendComparePredicate::Ugt:
      return "ugt";
    case BackendComparePredicate::Uge:
      return "uge";
  }
  return "unknown";
}

}  // namespace c4c::backend
