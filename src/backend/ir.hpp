#pragma once

#include "../codegen/lir/call_args.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend {

enum class BackendScalarType : unsigned char;

enum class BackendFunctionLinkage : unsigned char {
  Unknown,
  Define,
  Declare,
};

inline std::optional<BackendFunctionLinkage> parse_backend_function_linkage(
    std::string_view linkage) {
  if (linkage == "define") {
    return BackendFunctionLinkage::Define;
  }
  if (linkage == "declare") {
    return BackendFunctionLinkage::Declare;
  }
  return std::nullopt;
}

inline std::string_view render_backend_function_linkage(BackendFunctionLinkage linkage) {
  switch (linkage) {
    case BackendFunctionLinkage::Define:
      return "define";
    case BackendFunctionLinkage::Declare:
      return "declare";
    case BackendFunctionLinkage::Unknown:
      break;
  }

  return {};
}

struct BackendParam {
  std::string type_str;
  std::string name;
};

struct BackendFunctionSignature {
  std::string linkage;
  BackendFunctionLinkage linkage_kind = BackendFunctionLinkage::Unknown;
  std::string return_type;
  std::string name;
  std::vector<BackendParam> params;
  bool is_vararg = false;
};

inline BackendFunctionLinkage backend_function_linkage(
    const BackendFunctionSignature& signature) {
  if (signature.linkage_kind != BackendFunctionLinkage::Unknown) {
    return signature.linkage_kind;
  }
  return parse_backend_function_linkage(signature.linkage)
      .value_or(BackendFunctionLinkage::Unknown);
}

inline std::string render_backend_function_linkage(
    const BackendFunctionSignature& signature) {
  const auto structured = render_backend_function_linkage(
      backend_function_linkage(signature));
  if (!structured.empty()) {
    return std::string(structured);
  }
  return signature.linkage;
}

inline bool backend_function_is_definition(const BackendFunctionSignature& signature) {
  return backend_function_linkage(signature) == BackendFunctionLinkage::Define;
}

inline bool backend_function_is_declaration(const BackendFunctionSignature& signature) {
  return backend_function_linkage(signature) == BackendFunctionLinkage::Declare;
}

enum class BackendGlobalLinkage : unsigned char {
  Default,
  External,
  ExternWeak,
};

inline std::optional<BackendGlobalLinkage> parse_backend_global_linkage(
    std::string_view linkage) {
  if (linkage.empty()) {
    return BackendGlobalLinkage::Default;
  }
  if (linkage == "external ") {
    return BackendGlobalLinkage::External;
  }
  if (linkage == "extern_weak ") {
    return BackendGlobalLinkage::ExternWeak;
  }
  return std::nullopt;
}

inline std::string_view render_backend_global_linkage(BackendGlobalLinkage linkage) {
  switch (linkage) {
    case BackendGlobalLinkage::Default:
      return {};
    case BackendGlobalLinkage::External:
      return "external ";
    case BackendGlobalLinkage::ExternWeak:
      return "extern_weak ";
  }

  return {};
}

struct BackendGlobal {
  enum class StorageKind : unsigned char {
    Mutable,
    Constant,
  };

  struct Initializer {
    enum class Kind : unsigned char {
      Declaration,
      Zero,
      IntegerLiteral,
      RawText,
    };

    Kind kind = Kind::Declaration;
    std::string raw_text;
    std::int64_t integer_value = 0;

    static Initializer declaration() { return {}; }

    static Initializer zero() {
      Initializer initializer;
      initializer.kind = Kind::Zero;
      return initializer;
    }

    static Initializer integer_literal(std::int64_t value) {
      Initializer initializer;
      initializer.kind = Kind::IntegerLiteral;
      initializer.integer_value = value;
      return initializer;
    }

    static Initializer raw(std::string text) {
      Initializer initializer;
      initializer.kind = Kind::RawText;
      initializer.raw_text = std::move(text);
      return initializer;
    }
  };

  std::string name;
  std::string linkage;
  BackendGlobalLinkage linkage_kind = BackendGlobalLinkage::Default;
  StorageKind storage = StorageKind::Mutable;
  std::string llvm_type;
  Initializer initializer;
  int align_bytes = 0;

  // Compatibility shims for backend paths that still consume legacy global text.
  std::string qualifier;
  std::string init_text;
  bool is_extern_decl = false;
};

using BackendGlobalStorageKind = BackendGlobal::StorageKind;
using BackendGlobalInitializer = BackendGlobal::Initializer;

inline BackendGlobalLinkage backend_global_linkage(const BackendGlobal& global) {
  if (global.linkage_kind != BackendGlobalLinkage::Default) {
    return global.linkage_kind;
  }
  return parse_backend_global_linkage(global.linkage)
      .value_or(BackendGlobalLinkage::Default);
}

inline std::string render_backend_global_linkage(const BackendGlobal& global) {
  const auto structured = render_backend_global_linkage(backend_global_linkage(global));
  if (!structured.empty()) {
    return std::string(structured);
  }
  return global.linkage;
}

inline bool backend_global_is_extern_declaration(const BackendGlobal& global) {
  return global.initializer.kind == BackendGlobalInitializer::Kind::Declaration &&
         (backend_global_linkage(global) == BackendGlobalLinkage::External ||
          backend_global_linkage(global) == BackendGlobalLinkage::ExternWeak);
}

inline bool backend_global_has_integer_initializer(const BackendGlobal& global,
                                                   std::int64_t* value = nullptr) {
  if (global.initializer.kind != BackendGlobalInitializer::Kind::IntegerLiteral) {
    return false;
  }
  if (value != nullptr) {
    *value = global.initializer.integer_value;
  }
  return true;
}

inline bool backend_global_has_zero_initializer(const BackendGlobal& global) {
  return global.initializer.kind == BackendGlobalInitializer::Kind::Zero;
}

inline std::string_view render_backend_global_storage(BackendGlobalStorageKind storage) {
  switch (storage) {
    case BackendGlobalStorageKind::Mutable:
      return "global ";
    case BackendGlobalStorageKind::Constant:
      return "constant ";
  }

  return "global ";
}

inline std::string render_backend_global_initializer(
    const BackendGlobalInitializer& initializer) {
  switch (initializer.kind) {
    case BackendGlobalInitializer::Kind::Declaration:
      return {};
    case BackendGlobalInitializer::Kind::Zero:
      return "zeroinitializer";
    case BackendGlobalInitializer::Kind::IntegerLiteral:
      return std::to_string(initializer.integer_value);
    case BackendGlobalInitializer::Kind::RawText:
      return initializer.raw_text;
  }

  return {};
}

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
  BackendScalarType value_type{};
};

struct BackendCompareInst {
  BackendComparePredicate predicate = BackendComparePredicate::Eq;
  std::string result;
  std::string type_str;
  std::string lhs;
  std::string rhs;
  BackendScalarType operand_type{};
};

struct BackendPhiIncoming {
  std::string value;
  std::string label;
};

struct BackendPhiInst {
  std::string result;
  std::string type_str;
  std::vector<BackendPhiIncoming> incoming;
  BackendScalarType value_type{};
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

enum class BackendScalarType : unsigned char {
  Unknown,
  I8,
  I32,
};

inline std::optional<BackendScalarType> parse_backend_scalar_type(std::string_view type) {
  if (type == "i8") {
    return BackendScalarType::I8;
  }
  if (type == "i32") {
    return BackendScalarType::I32;
  }
  return std::nullopt;
}

inline std::string_view render_backend_scalar_type(BackendScalarType type) {
  switch (type) {
    case BackendScalarType::I8:
      return "i8";
    case BackendScalarType::I32:
      return "i32";
    case BackendScalarType::Unknown:
      break;
  }

  return {};
}

inline std::size_t backend_scalar_type_size_bytes(BackendScalarType type) {
  switch (type) {
    case BackendScalarType::I8:
      return 1;
    case BackendScalarType::I32:
      return 4;
    case BackendScalarType::Unknown:
      break;
  }

  return 0;
}

inline std::string render_backend_scalar_type_or_fallback(BackendScalarType type,
                                                          std::string_view fallback) {
  const auto rendered = render_backend_scalar_type(type);
  if (!rendered.empty()) {
    return std::string(rendered);
  }
  return std::string(fallback);
}

inline BackendScalarType backend_binary_value_type(const BackendBinaryInst& binary) {
  if (binary.value_type != BackendScalarType::Unknown) {
    return binary.value_type;
  }
  return parse_backend_scalar_type(binary.type_str).value_or(BackendScalarType::Unknown);
}

inline std::string render_backend_binary_value_type(const BackendBinaryInst& binary) {
  return render_backend_scalar_type_or_fallback(backend_binary_value_type(binary),
                                                binary.type_str);
}

inline BackendScalarType backend_compare_operand_type(const BackendCompareInst& compare) {
  if (compare.operand_type != BackendScalarType::Unknown) {
    return compare.operand_type;
  }
  return parse_backend_scalar_type(compare.type_str).value_or(BackendScalarType::Unknown);
}

inline std::string render_backend_compare_operand_type(const BackendCompareInst& compare) {
  return render_backend_scalar_type_or_fallback(backend_compare_operand_type(compare),
                                                compare.type_str);
}

inline BackendScalarType backend_phi_value_type(const BackendPhiInst& phi) {
  if (phi.value_type != BackendScalarType::Unknown) {
    return phi.value_type;
  }
  return parse_backend_scalar_type(phi.type_str).value_or(BackendScalarType::Unknown);
}

inline std::string render_backend_phi_value_type(const BackendPhiInst& phi) {
  return render_backend_scalar_type_or_fallback(backend_phi_value_type(phi), phi.type_str);
}

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
  BackendScalarType value_type = BackendScalarType::Unknown;
  BackendScalarType memory_value_type = BackendScalarType::Unknown;
};

struct BackendStoreInst {
  std::string type_str;
  std::string value;
  BackendAddress address;
  BackendScalarType value_type = BackendScalarType::Unknown;
};

struct BackendPtrDiffEqInst {
  std::string result;
  std::string type_str;
  BackendAddress lhs_address;
  BackendAddress rhs_address;
  std::int64_t element_size = 1;
  std::int64_t expected_diff = 0;
  BackendScalarType result_type = BackendScalarType::Unknown;
  BackendScalarType element_type = BackendScalarType::Unknown;
};

inline BackendScalarType backend_load_value_type(const BackendLoadInst& load) {
  if (load.value_type != BackendScalarType::Unknown) {
    return load.value_type;
  }
  return parse_backend_scalar_type(load.type_str).value_or(BackendScalarType::Unknown);
}

inline BackendScalarType backend_load_memory_type(const BackendLoadInst& load) {
  if (load.memory_value_type != BackendScalarType::Unknown) {
    return load.memory_value_type;
  }
  if (!load.memory_type.empty()) {
    return parse_backend_scalar_type(load.memory_type).value_or(BackendScalarType::Unknown);
  }
  return backend_load_value_type(load);
}

inline std::string render_backend_load_value_type(const BackendLoadInst& load) {
  return render_backend_scalar_type_or_fallback(backend_load_value_type(load), load.type_str);
}

inline std::string render_backend_load_memory_type(const BackendLoadInst& load) {
  if (load.memory_type.empty() && load.memory_value_type == BackendScalarType::Unknown) {
    return render_backend_load_value_type(load);
  }
  return render_backend_scalar_type_or_fallback(backend_load_memory_type(load),
                                                load.memory_type);
}

inline BackendScalarType backend_store_value_type(const BackendStoreInst& store) {
  if (store.value_type != BackendScalarType::Unknown) {
    return store.value_type;
  }
  return parse_backend_scalar_type(store.type_str).value_or(BackendScalarType::Unknown);
}

inline std::string render_backend_store_value_type(const BackendStoreInst& store) {
  return render_backend_scalar_type_or_fallback(backend_store_value_type(store),
                                                store.type_str);
}

inline BackendScalarType backend_ptrdiff_result_type(const BackendPtrDiffEqInst& ptrdiff) {
  if (ptrdiff.result_type != BackendScalarType::Unknown) {
    return ptrdiff.result_type;
  }
  return parse_backend_scalar_type(ptrdiff.type_str).value_or(BackendScalarType::Unknown);
}

inline BackendScalarType backend_ptrdiff_element_type(const BackendPtrDiffEqInst& ptrdiff) {
  if (ptrdiff.element_type != BackendScalarType::Unknown) {
    return ptrdiff.element_type;
  }
  switch (ptrdiff.element_size) {
    case 1:
      return BackendScalarType::I8;
    case 4:
      return BackendScalarType::I32;
    default:
      return BackendScalarType::Unknown;
  }
}

inline std::string render_backend_ptrdiff_result_type(const BackendPtrDiffEqInst& ptrdiff) {
  return render_backend_scalar_type_or_fallback(backend_ptrdiff_result_type(ptrdiff),
                                                ptrdiff.type_str);
}

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
