#pragma once

#include "../codegen/lir/ir.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace c4c::backend {

enum class LirAdapterErrorKind {
  Unsupported,
  Malformed,
};

class LirAdapterError : public std::invalid_argument {
 public:
  LirAdapterError(LirAdapterErrorKind kind, const std::string& message)
      : std::invalid_argument(message), kind_(kind) {}

  LirAdapterErrorKind kind() const noexcept { return kind_; }

 private:
  LirAdapterErrorKind kind_;
};

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

enum class BackendBinaryOpcode {
  Add,
};

struct BackendBinaryInst {
  BackendBinaryOpcode opcode = BackendBinaryOpcode::Add;
  std::string result;
  std::string type_str;
  std::string lhs;
  std::string rhs;
};

using BackendInst = std::variant<BackendBinaryInst>;

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
  std::vector<BackendFunction> functions;
};

BackendModule adapt_minimal_module(const c4c::codegen::lir::LirModule& module);
std::string render_module(const BackendModule& module);

}  // namespace c4c::backend
