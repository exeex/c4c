#pragma once

#include "../codegen/lir/call_args.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace c4c::backend {

struct BackendParam {
  std::string type_str;
  std::string name;
};

struct BackendFunctionSignature {
  std::string linkage;
  std::string return_type;
  std::string name;
  std::vector<BackendParam> params;
  bool is_vararg = false;
};

struct BackendGlobal {
  std::string name;
  std::string linkage;
  std::string qualifier;
  std::string llvm_type;
  std::string init_text;
  int align_bytes = 0;
  bool is_extern_decl = false;
};

enum class BackendBinaryOpcode {
  Add,
  Sub,
  Mul,
  SDiv,
  SRem,
};

struct BackendBinaryInst {
  BackendBinaryOpcode opcode = BackendBinaryOpcode::Add;
  std::string result;
  std::string type_str;
  std::string lhs;
  std::string rhs;
};

using BackendCallArg = c4c::codegen::lir::OwnedLirTypedCallArg;

struct BackendCallInst {
  std::string result;
  std::string return_type;
  std::string callee;
  std::vector<std::string> param_types;
  std::vector<BackendCallArg> args;
  bool render_callee_type_suffix = false;
};

struct BackendAddress {
  std::string base_symbol;
  std::int64_t byte_offset = 0;
};

struct BackendLoadInst {
  std::string result;
  std::string type_str;
  BackendAddress address;
};

struct BackendPtrDiffEqInst {
  std::string result;
  std::string type_str;
  BackendAddress lhs_address;
  BackendAddress rhs_address;
  std::int64_t element_size = 1;
  std::int64_t expected_diff = 0;
};

using BackendInst = std::variant<BackendBinaryInst,
                                 BackendCallInst,
                                 BackendLoadInst,
                                 BackendPtrDiffEqInst>;

struct BackendReturn {
  std::optional<std::string> value;
  std::string type_str;
};

struct BackendBlock {
  std::string label;
  std::vector<BackendInst> insts;
  BackendReturn terminator;
};

struct BackendFunction {
  BackendFunctionSignature signature;
  bool is_declaration = false;
  std::vector<BackendBlock> blocks;
};

struct BackendModule {
  std::string target_triple;
  std::string data_layout;
  std::vector<std::string> type_decls;
  std::vector<BackendGlobal> globals;
  std::vector<BackendFunction> functions;
};

const char* backend_binary_opcode_name(BackendBinaryOpcode opcode);

}  // namespace c4c::backend
