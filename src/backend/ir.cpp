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

}  // namespace c4c::backend
