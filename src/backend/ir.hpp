#pragma once

#include "../codegen/lir/call_args.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
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

struct BackendStringConstant {
  std::string name;
  std::string raw_bytes;
  std::size_t byte_length = 0;
};

enum class BackendBinaryOpcode {
  Add,
  Sub,
  Mul,
  SDiv,
  SRem,
};

enum class BackendComparePredicate {
  Slt,
  Sle,
  Sgt,
  Sge,
  Eq,
  Ne,
  Ult,
  Ule,
  Ugt,
  Uge,
};

struct BackendBinaryInst {
  BackendBinaryOpcode opcode = BackendBinaryOpcode::Add;
  std::string result;
  std::string type_str;
  std::string lhs;
  std::string rhs;
};

struct BackendCompareInst {
  BackendComparePredicate predicate = BackendComparePredicate::Eq;
  std::string result;
  std::string type_str;
  std::string lhs;
  std::string rhs;
};

struct BackendPhiIncoming {
  std::string value;
  std::string label;
};

struct BackendPhiInst {
  std::string result;
  std::string type_str;
  std::vector<BackendPhiIncoming> incoming;
};

using BackendCallArg = c4c::codegen::lir::OwnedLirTypedCallArg;

enum class BackendCallCalleeKind : unsigned char {
  DirectGlobal,
  DirectIntrinsic,
  Indirect,
};

struct BackendCallCallee {
  BackendCallCalleeKind kind = BackendCallCalleeKind::Indirect;
  std::string symbol_name;
  std::string operand;

  static BackendCallCallee direct_global(std::string name) {
    BackendCallCallee callee;
    callee.kind = BackendCallCalleeKind::DirectGlobal;
    callee.symbol_name = std::move(name);
    return callee;
  }

  static BackendCallCallee direct_intrinsic(std::string name) {
    BackendCallCallee callee;
    callee.kind = BackendCallCalleeKind::DirectIntrinsic;
    callee.symbol_name = std::move(name);
    return callee;
  }

  static BackendCallCallee indirect(std::string value) {
    BackendCallCallee callee;
    callee.kind = BackendCallCalleeKind::Indirect;
    callee.operand = std::move(value);
    return callee;
  }
};

inline std::optional<BackendCallCallee> parse_backend_call_callee(std::string_view callee) {
  const auto parsed = c4c::codegen::lir::parse_lir_call_callee(callee);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  switch (parsed->kind) {
    case c4c::codegen::lir::LirCallCalleeKind::DirectGlobal:
      return BackendCallCallee::direct_global(std::string(parsed->symbol_name));
    case c4c::codegen::lir::LirCallCalleeKind::DirectIntrinsic:
      return BackendCallCallee::direct_intrinsic(std::string(parsed->symbol_name));
    case c4c::codegen::lir::LirCallCalleeKind::Indirect:
      return BackendCallCallee::indirect(std::string(callee));
  }

  return std::nullopt;
}

inline std::string render_backend_call_callee(const BackendCallCallee& callee) {
  switch (callee.kind) {
    case BackendCallCalleeKind::DirectGlobal:
    case BackendCallCalleeKind::DirectIntrinsic:
      return "@" + callee.symbol_name;
    case BackendCallCalleeKind::Indirect:
      return callee.operand;
  }

  return {};
}

struct BackendCallInst {
  std::string result;
  std::string return_type;
  BackendCallCallee callee;
  std::vector<std::string> param_types;
  std::vector<BackendCallArg> args;
  bool render_callee_type_suffix = false;
};

struct BackendAddress {
  std::string base_symbol;
  std::int64_t byte_offset = 0;
};

enum class BackendLoadExtension {
  None,
  SignExtend,
  ZeroExtend,
};

struct BackendLoadInst {
  std::string result;
  std::string type_str;
  std::string memory_type;
  BackendAddress address;
  BackendLoadExtension extension = BackendLoadExtension::None;
};

struct BackendStoreInst {
  std::string type_str;
  std::string value;
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

using BackendInst = std::variant<BackendPhiInst,
                                 BackendBinaryInst,
                                 BackendCompareInst,
                                 BackendCallInst,
                                 BackendLoadInst,
                                 BackendStoreInst,
                                 BackendPtrDiffEqInst>;

struct BackendReturn {
  std::optional<std::string> value;
  std::string type_str;
};

enum class BackendTerminatorKind {
  Return,
  Branch,
  CondBranch,
};

struct BackendTerminator : BackendReturn {
  BackendTerminatorKind kind = BackendTerminatorKind::Return;
  std::string cond_name;
  std::string true_label;
  std::string false_label;
  std::string target_label;

  BackendTerminator() = default;

  BackendTerminator(const BackendReturn& ret)
      : BackendReturn(ret), kind(BackendTerminatorKind::Return) {}

  BackendTerminator(BackendReturn&& ret)
      : BackendReturn(std::move(ret)), kind(BackendTerminatorKind::Return) {}

  BackendTerminator& operator=(const BackendReturn& ret) {
    static_cast<BackendReturn&>(*this) = ret;
    kind = BackendTerminatorKind::Return;
    cond_name.clear();
    true_label.clear();
    false_label.clear();
    target_label.clear();
    return *this;
  }

  BackendTerminator& operator=(BackendReturn&& ret) {
    static_cast<BackendReturn&>(*this) = std::move(ret);
    kind = BackendTerminatorKind::Return;
    cond_name.clear();
    true_label.clear();
    false_label.clear();
    target_label.clear();
    return *this;
  }

  static BackendTerminator make_branch(std::string target) {
    BackendTerminator terminator;
    terminator.kind = BackendTerminatorKind::Branch;
    terminator.type_str = "void";
    terminator.target_label = std::move(target);
    return terminator;
  }

  static BackendTerminator make_cond_branch(std::string cond,
                                            std::string true_target,
                                            std::string false_target) {
    BackendTerminator terminator;
    terminator.kind = BackendTerminatorKind::CondBranch;
    terminator.type_str = "void";
    terminator.cond_name = std::move(cond);
    terminator.true_label = std::move(true_target);
    terminator.false_label = std::move(false_target);
    return terminator;
  }
};

struct BackendBlock {
  std::string label;
  std::vector<BackendInst> insts;
  BackendTerminator terminator;
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
  std::vector<BackendStringConstant> string_constants;
  std::vector<BackendFunction> functions;
};

const char* backend_binary_opcode_name(BackendBinaryOpcode opcode);
const char* backend_compare_predicate_name(BackendComparePredicate predicate);

}  // namespace c4c::backend
