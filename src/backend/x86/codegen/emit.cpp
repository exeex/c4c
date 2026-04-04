#include "emit.hpp"

#include "../../bir.hpp"
#include "../../generation.hpp"
#include "../../ir_printer.hpp"
#include "../../ir_validate.hpp"
#include "../../lir_adapter.hpp"
#include "../../lowering/call_decode.hpp"
#include "../../stack_layout/regalloc_helpers.hpp"
#include "../../../codegen/lir/call_args.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <charconv>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <string_view>

// Mechanical translation of the ref x86 emitter entrypoint.
// The larger target-specific lowering surface is split across the sibling
// translation units in this directory.

namespace c4c::backend::x86 {

namespace {

[[noreturn]] void throw_unsupported_direct_bir_module() {
  throw std::invalid_argument(
      "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
}

std::string asm_symbol_name(const c4c::backend::BackendModule& module,
                            std::string_view logical_name);
std::string asm_symbol_name(std::string_view target_triple,
                            std::string_view logical_name);

const std::string& synthetic_call_crossing_regalloc_source_value() {
  static const std::string kSyntheticCallCrossingRegallocSource =
      "%t.call_crossing.regalloc_source";
  return kSyntheticCallCrossingRegallocSource;
}

struct MinimalCallCrossingDirectCallSlice {
  std::string callee_name;
  std::int64_t source_imm = 0;
  std::int64_t helper_add_imm = 0;
  std::string regalloc_source_value = synthetic_call_crossing_regalloc_source_value();
};

using MinimalExternCallArgSlice = c4c::backend::ParsedBackendExternCallArg;

struct MinimalTwoArgDirectCallSlice {
  std::string callee_name;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct MinimalConditionalReturnSlice {
  enum class ValueKind : unsigned char {
    Immediate,
    Param0,
    Param1,
  };

  struct ValueSource {
    ValueKind kind = ValueKind::Immediate;
    std::int64_t imm = 0;
  };

  std::string function_name;
  std::size_t param_count = 0;
  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  ValueSource lhs;
  ValueSource rhs;
  std::string true_label;
  std::string false_label;
  ValueSource true_value;
  ValueSource false_value;
};

struct MinimalConditionalPhiJoinSlice {
  struct ArithmeticStep {
    c4c::backend::BackendBinaryOpcode opcode = c4c::backend::BackendBinaryOpcode::Add;
    std::int64_t imm = 0;
  };

  struct IncomingValue {
    std::int64_t base_imm = 0;
    std::vector<ArithmeticStep> steps;
  };

  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  std::int64_t lhs_imm = 0;
  std::int64_t rhs_imm = 0;
  std::string true_label;
  std::string false_label;
  std::string join_label;
  IncomingValue true_value;
  IncomingValue false_value;
  std::optional<std::int64_t> join_add_imm;
};

struct MinimalConditionalAffineI8ReturnSlice {
  enum class ValueKind : unsigned char {
    Immediate,
    Param0,
    Param1,
  };

  struct ArithmeticStep {
    c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Add;
    std::int64_t imm = 0;
  };

  struct ValueExpr {
    ValueKind kind = ValueKind::Immediate;
    std::int64_t imm = 0;
    std::vector<ArithmeticStep> steps;
  };

  std::string function_name;
  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  ValueExpr lhs;
  ValueExpr rhs;
  std::string true_label;
  std::string false_label;
  std::string join_label;
  ValueExpr true_value;
  ValueExpr false_value;
  std::vector<ArithmeticStep> post_steps;
};

struct MinimalConditionalAffineI32ReturnSlice {
  enum class ValueKind : unsigned char {
    Immediate,
    Param0,
    Param1,
  };

  struct ArithmeticStep {
    c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Add;
    std::int64_t imm = 0;
  };

  struct ValueExpr {
    ValueKind kind = ValueKind::Immediate;
    std::int64_t imm = 0;
    std::vector<ArithmeticStep> steps;
  };

  std::string function_name;
  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  ValueExpr lhs;
  ValueExpr rhs;
  std::string true_label;
  std::string false_label;
  std::string join_label;
  ValueExpr true_value;
  ValueExpr false_value;
  std::vector<ArithmeticStep> post_steps;
};

struct MinimalCastReturnSlice {
  std::string function_name;
  c4c::backend::bir::CastOpcode opcode = c4c::backend::bir::CastOpcode::SExt;
  c4c::backend::bir::TypeKind operand_type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::bir::TypeKind result_type = c4c::backend::bir::TypeKind::Void;
};

struct MinimalCountdownLoopSlice {
  std::int64_t initial_imm = 0;
  std::string loop_label;
  std::string body_label;
  std::string exit_label;
};

struct MinimalLocalArraySlice {
  std::int64_t store0_imm = 0;
  std::int64_t store1_imm = 0;
};

struct MinimalExternScalarGlobalLoadSlice {
  std::string global_name;
};

struct MinimalExternGlobalArrayLoadSlice {
  std::string global_name;
  std::int64_t byte_offset = 0;
};

struct MinimalGlobalCharPointerDiffSlice {
  std::string global_name;
  std::int64_t global_size = 0;
  std::int64_t byte_offset = 0;
};

struct MinimalGlobalIntPointerDiffSlice {
  std::string global_name;
  std::int64_t global_size = 0;
  std::int64_t byte_offset = 0;
  std::int64_t element_shift = 0;
};

struct MinimalScalarGlobalLoadSlice {
  std::string global_name;
  std::int64_t init_imm = 0;
};

struct MinimalScalarGlobalStoreReloadSlice {
  std::string global_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
};

struct MinimalStringLiteralCharSlice {
  std::string pool_name;
  std::string raw_bytes;
  std::int64_t byte_offset = 0;
  bool sign_extend = true;
};

struct MinimalAffineReturnSlice {
  std::string function_name;
  std::size_t param_count = 0;
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

std::optional<std::int64_t> parse_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::int64_t> parse_i64(const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  return value.immediate;
}

std::optional<c4c::codegen::lir::LirCmpPredicate> lower_bir_cmp_predicate(
    c4c::backend::bir::BinaryOpcode predicate) {
  using BirPred = c4c::backend::bir::BinaryOpcode;
  using LirPred = c4c::codegen::lir::LirCmpPredicate;

  switch (predicate) {
    case BirPred::Eq: return LirPred::Eq;
    case BirPred::Ne: return LirPred::Ne;
    case BirPred::Slt: return LirPred::Slt;
    case BirPred::Sle: return LirPred::Sle;
    case BirPred::Sgt: return LirPred::Sgt;
    case BirPred::Sge: return LirPred::Sge;
    case BirPred::Ult: return LirPred::Ult;
    case BirPred::Ule: return LirPred::Ule;
    case BirPred::Ugt: return LirPred::Ugt;
    case BirPred::Uge: return LirPred::Uge;
    default: return std::nullopt;
  }
}

std::optional<MinimalConditionalReturnSlice::ValueSource> lower_bir_i32_value_source(
    const c4c::backend::bir::Value& value,
    const std::vector<c4c::backend::bir::Param>& params) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return MinimalConditionalReturnSlice::ValueSource{
        MinimalConditionalReturnSlice::ValueKind::Immediate,
        value.immediate,
    };
  }
  if (!params.empty() && params[0].type == c4c::backend::bir::TypeKind::I32 &&
      value.name == params[0].name) {
    return MinimalConditionalReturnSlice::ValueSource{
        MinimalConditionalReturnSlice::ValueKind::Param0,
        0,
    };
  }
  if (params.size() > 1 && params[1].type == c4c::backend::bir::TypeKind::I32 &&
      value.name == params[1].name) {
    return MinimalConditionalReturnSlice::ValueSource{
        MinimalConditionalReturnSlice::ValueKind::Param1,
        0,
    };
  }
  return std::nullopt;
}

std::optional<MinimalConditionalAffineI8ReturnSlice::ValueExpr> lower_bir_i8_value_expr(
    const c4c::backend::bir::Value& value,
    const std::vector<c4c::backend::bir::Param>& params,
    const std::unordered_map<std::string, MinimalConditionalAffineI8ReturnSlice::ValueExpr>&
        values) {
  using ValueExpr = MinimalConditionalAffineI8ReturnSlice::ValueExpr;
  using ValueKind = MinimalConditionalAffineI8ReturnSlice::ValueKind;

  if (value.type != c4c::backend::bir::TypeKind::I8) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return ValueExpr{ValueKind::Immediate, value.immediate, {}};
  }
  if (!params.empty() && params[0].type == c4c::backend::bir::TypeKind::I8 &&
      value.name == params[0].name) {
    return ValueExpr{ValueKind::Param0, 0, {}};
  }
  if (params.size() > 1 && params[1].type == c4c::backend::bir::TypeKind::I8 &&
      value.name == params[1].name) {
    return ValueExpr{ValueKind::Param1, 0, {}};
  }
  const auto it = values.find(value.name);
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<MinimalConditionalAffineI32ReturnSlice::ValueExpr> lower_bir_i32_value_expr(
    const c4c::backend::bir::Value& value,
    const std::vector<c4c::backend::bir::Param>& params,
    const std::unordered_map<std::string, MinimalConditionalAffineI32ReturnSlice::ValueExpr>&
        values) {
  using ValueExpr = MinimalConditionalAffineI32ReturnSlice::ValueExpr;
  using ValueKind = MinimalConditionalAffineI32ReturnSlice::ValueKind;

  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return ValueExpr{ValueKind::Immediate, value.immediate, {}};
  }
  if (!params.empty() && params[0].type == c4c::backend::bir::TypeKind::I32 &&
      value.name == params[0].name) {
    return ValueExpr{ValueKind::Param0, 0, {}};
  }
  if (params.size() > 1 && params[1].type == c4c::backend::bir::TypeKind::I32 &&
      value.name == params[1].name) {
    return ValueExpr{ValueKind::Param1, 0, {}};
  }
  const auto it = values.find(value.name);
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
}

struct AffineValue {
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

std::optional<AffineValue> lower_affine_operand(
    std::string_view operand,
    const std::vector<std::string_view>& param_names,
    const std::unordered_map<std::string, AffineValue>& values) {
  if (const auto imm = parse_i64(operand); imm.has_value()) {
    return AffineValue{false, false, *imm};
  }
  if (!param_names.empty() && operand == param_names[0]) {
    return AffineValue{true, false, 0};
  }
  if (param_names.size() > 1 && operand == param_names[1]) {
    return AffineValue{false, true, 0};
  }
  auto it = values.find(std::string(operand));
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<AffineValue> combine_affine_values(const AffineValue& lhs,
                                                 const AffineValue& rhs,
                                                 c4c::backend::BackendBinaryOpcode opcode) {
  if (opcode == c4c::backend::BackendBinaryOpcode::Add) {
    if ((lhs.uses_first_param && rhs.uses_first_param) ||
        (lhs.uses_second_param && rhs.uses_second_param)) {
      return std::nullopt;
    }
    return AffineValue{lhs.uses_first_param || rhs.uses_first_param,
                       lhs.uses_second_param || rhs.uses_second_param,
                       lhs.constant + rhs.constant};
  }
  if (rhs.uses_first_param || rhs.uses_second_param) {
    return std::nullopt;
  }
  return AffineValue{lhs.uses_first_param, lhs.uses_second_param,
                     lhs.constant - rhs.constant};
}

std::optional<AffineValue> combine_affine_values(const AffineValue& lhs,
                                                 const AffineValue& rhs,
                                                 c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      return combine_affine_values(lhs, rhs, c4c::backend::BackendBinaryOpcode::Add);
    case c4c::backend::bir::BinaryOpcode::Sub:
      return combine_affine_values(lhs, rhs, c4c::backend::BackendBinaryOpcode::Sub);
  }
  return std::nullopt;
}

const c4c::backend::bir::BinaryInst* get_binary_inst(
    const c4c::backend::bir::Inst& inst) {
  return std::get_if<c4c::backend::bir::BinaryInst>(&inst);
}

bool is_i32_scalar_signature_return(const c4c::backend::BackendFunctionSignature& signature) {
  return c4c::backend::backend_signature_return_type_kind(signature) ==
             c4c::backend::BackendValueTypeKind::Scalar &&
         c4c::backend::backend_signature_return_scalar_type(signature) ==
             c4c::backend::BackendScalarType::I32;
}

bool is_i32_scalar_param(const c4c::backend::BackendParam& param) {
  return c4c::backend::backend_param_type_kind(param) ==
             c4c::backend::BackendValueTypeKind::Scalar &&
         c4c::backend::backend_param_scalar_type(param) ==
             c4c::backend::BackendScalarType::I32;
}

bool is_i32_scalar_call_return(const c4c::backend::BackendCallInst& call) {
  return c4c::backend::backend_call_return_type_kind(call) ==
             c4c::backend::BackendValueTypeKind::Scalar &&
         c4c::backend::backend_call_return_scalar_type(call) ==
             c4c::backend::BackendScalarType::I32;
}

bool is_i32_scalar_binary(const c4c::backend::BackendBinaryInst& inst) {
  return c4c::backend::backend_binary_value_type(inst) ==
         c4c::backend::BackendScalarType::I32;
}

bool is_i32_scalar_global(const c4c::backend::BackendGlobal& global) {
  return c4c::backend::backend_global_value_type_kind(global) ==
             c4c::backend::BackendValueTypeKind::Scalar &&
         c4c::backend::backend_global_scalar_type(global) ==
             c4c::backend::BackendScalarType::I32;
}

const c4c::backend::BackendLocalSlot* find_local_slot(
    const c4c::backend::BackendFunction& function,
    std::string_view slot_name) {
  for (const auto& slot : function.local_slots) {
    if (slot.name == slot_name) {
      return &slot;
    }
  }
  return nullptr;
}

bool is_two_element_i32_local_array_slot(const c4c::backend::BackendFunction& function,
                                         std::string_view slot_name) {
  const auto* slot = find_local_slot(function, slot_name);
  return slot != nullptr && slot->size_bytes == 8 &&
         slot->element_type == c4c::backend::BackendScalarType::I32 &&
         slot->element_size_bytes == 4;
}

std::optional<std::string_view> strip_typed_operand_prefix(std::string_view operand,
                                                           std::string_view type_prefix) {
  if (operand.size() <= type_prefix.size() + 1 ||
      operand.substr(0, type_prefix.size()) != type_prefix ||
      operand[type_prefix.size()] != ' ') {
    return std::nullopt;
  }
  return operand.substr(type_prefix.size() + 1);
}

std::string asm_symbol_name(const c4c::backend::BackendModule& module,
                            std::string_view logical_name) {
  return asm_symbol_name(module.target_triple, logical_name);
}

std::string asm_symbol_name(std::string_view target_triple,
                            std::string_view logical_name) {
  const bool is_darwin = target_triple.find("apple-darwin") != std::string::npos;
  if (!is_darwin) return std::string(logical_name);
  return std::string("_") + std::string(logical_name);
}

std::string asm_private_data_label(const c4c::backend::BackendModule& module,
                                   std::string_view pool_name) {
  std::string label(pool_name);
  if (!label.empty() && label.front() == '@') {
    label.erase(label.begin());
  }
  while (!label.empty() && label.front() == '.') {
    label.erase(label.begin());
  }

  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  if (is_darwin) {
    return "L." + label;
  }
  return ".L." + label;
}

std::string decode_llvm_c_string(std::string_view bytes) {
  auto hex_value = [](unsigned char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
  };

  std::string decoded;
  decoded.reserve(bytes.size());
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    const char ch = bytes[i];
    if (ch != '\\' || i + 1 >= bytes.size()) {
      decoded.push_back(ch);
      continue;
    }

    const char code = bytes[i + 1];
    const int hi = hex_value(static_cast<unsigned char>(code));
    if (code == 'x' && i + 3 < bytes.size()) {
      const int hi2 = hex_value(static_cast<unsigned char>(bytes[i + 2]));
      const int lo2 = hex_value(static_cast<unsigned char>(bytes[i + 3]));
      if (hi2 >= 0 && lo2 >= 0) {
        decoded.push_back(static_cast<char>((hi2 << 4) | lo2));
        i += 3;
        continue;
      }
      decoded.push_back('\\');
      continue;
    }

    if (hi >= 0 && i + 2 < bytes.size()) {
      const int lo = hex_value(static_cast<unsigned char>(bytes[i + 2]));
      if (lo >= 0) {
        decoded.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }

    ++i;
    switch (code) {
      case 'n':
        decoded.push_back('\n');
        break;
      case 'r':
        decoded.push_back('\r');
        break;
      case 't':
        decoded.push_back('\t');
        break;
      case '\"':
        decoded.push_back('\"');
        break;
      case '\\':
        decoded.push_back('\\');
        break;
      default:
        decoded.push_back('\\');
        decoded.push_back(code);
        break;
    }
  }
  return decoded;
}

std::string escape_asm_string(std::string_view bytes) {
  const std::string decoded = decode_llvm_c_string(bytes);

  static constexpr char kHexDigits[] = "0123456789abcdef";

  std::string escaped;
  escaped.reserve(decoded.size());
  for (unsigned char ch : decoded) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (ch >= 0x20 && ch <= 0x7e) {
          escaped.push_back(static_cast<char>(ch));
        } else {
          escaped += "\\x";
          escaped.push_back(kHexDigits[(ch >> 4) & 0xf]);
          escaped.push_back(kHexDigits[ch & 0xf]);
        }
        break;
    }
  }
  return escaped;
}

const c4c::codegen::lir::LirFunction* find_lir_function(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  for (const auto& function : module.functions) {
    if (function.name == name) return &function;
  }
  return nullptr;
}

const c4c::backend::BackendFunction* find_function(
    const c4c::backend::BackendModule& module,
    std::string_view name) {
  for (const auto& function : module.functions) {
    if (function.signature.name == name) return &function;
  }
  return nullptr;
}

const c4c::backend::BackendGlobal* find_global(
    const c4c::backend::BackendModule& module,
    std::string_view name) {
  for (const auto& global : module.globals) {
    if (global.name == name) return &global;
  }
  return nullptr;
}

const c4c::backend::BackendStringConstant* find_string_constant(
    const c4c::backend::BackendModule& module,
    std::string_view name) {
  for (const auto& string_constant : module.string_constants) {
    if (string_constant.name == name) return &string_constant;
  }
  return nullptr;
}

std::optional<std::int64_t> parse_single_block_return_imm(
    const c4c::backend::BackendFunction& function) {
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      !is_i32_scalar_signature_return(function.signature) ||
      !function.signature.params.empty() ||
      function.signature.is_vararg || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32 ||
      !block.insts.empty()) {
    return std::nullopt;
  }

  return parse_i64(*block.terminator.value);
}

const char* x86_reg64_name(c4c::backend::PhysReg reg) {
  switch (reg.index) {
    case 1:
      return "rbx";
    case 2:
      return "r12";
    case 3:
      return "r13";
    case 4:
      return "r14";
    case 5:
      return "r15";
    case 10:
      return "r11";
    case 11:
      return "r10";
    case 12:
      return "r8";
    case 13:
      return "r9";
    case 14:
      return "rdi";
    case 15:
      return "rsi";
    default:
      return nullptr;
  }
}

const char* x86_reg32_name(c4c::backend::PhysReg reg) {
  switch (reg.index) {
    case 1:
      return "ebx";
    case 2:
      return "r12d";
    case 3:
      return "r13d";
    case 4:
      return "r14d";
    case 5:
      return "r15d";
    case 10:
      return "r11d";
    case 11:
      return "r10d";
    case 12:
      return "r8d";
    case 13:
      return "r9d";
    case 14:
      return "edi";
    case 15:
      return "esi";
    default:
      return nullptr;
  }
}

std::int64_t aligned_frame_size(std::size_t saved_reg_count) {
  const auto raw = static_cast<std::int64_t>(saved_reg_count) * 8;
  return raw > 0 ? (raw + 15) & ~std::int64_t{15} : 0;
}

c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(
    const c4c::codegen::lir::LirFunction& function) {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{1}, {2}, {3}, {4}, {5}};
  config.caller_saved_regs = {{10}, {11}, {12}, {13}, {14}, {15}};
  return c4c::backend::run_regalloc_and_merge_clobbers(function, config, {});
}

c4c::backend::RegAllocIntegrationResult synthesize_shared_x86_call_crossing_regalloc(
    const MinimalCallCrossingDirectCallSlice& slice) {
  c4c::backend::RegAllocIntegrationResult regalloc;
  const std::string& source_value = slice.regalloc_source_value.empty()
                                        ? synthetic_call_crossing_regalloc_source_value()
                                        : slice.regalloc_source_value;
  regalloc.reg_assignments.emplace(source_value,
                                   c4c::backend::PhysReg{1});
  regalloc.used_callee_saved.push_back(c4c::backend::PhysReg{1});
  return regalloc;
}

bool is_minimal_single_function_asm_slice(const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) return false;

  const auto& function = module.functions.front();
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      !is_i32_scalar_signature_return(function.signature) || function.signature.is_vararg ||
      function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return false;
  }

  return true;
}

bool is_minimal_single_function_asm_slice(const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1) {
    return false;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::I32 ||
      function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  return true;
}

std::string minimal_single_function_symbol(const c4c::backend::BackendModule& module) {
  return asm_symbol_name(module, module.functions.front().signature.name);
}

std::string minimal_single_function_symbol(const c4c::backend::bir::Module& module) {
  return asm_symbol_name(module.target_triple, module.functions.front().name);
}

std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (!block.insts.empty()) {
    return std::nullopt;
  }

  return parse_i64(*block.terminator.value);
}

std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (!block.insts.empty()) {
    return std::nullopt;
  }

  return parse_i64(*block.terminator.value);
}

std::optional<std::int64_t> parse_minimal_return_add_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      !is_i32_scalar_binary(*add) || *block.terminator.value != add->result) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(add->lhs);
  const auto rhs = parse_i64(add->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  return *lhs + *rhs;
}

std::optional<std::int64_t> parse_minimal_return_add_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* add = get_binary_inst(block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::bir::BinaryOpcode::Add ||
      add->result.kind != c4c::backend::bir::Value::Kind::Named ||
      add->result.type != c4c::backend::bir::TypeKind::I32 ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->name != add->result.name) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(add->lhs);
  const auto rhs = parse_i64(add->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  return *lhs + *rhs;
}

std::optional<std::int64_t> parse_minimal_return_sub_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* sub = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  if (sub == nullptr || sub->opcode != c4c::backend::BackendBinaryOpcode::Sub ||
      !is_i32_scalar_binary(*sub) || *block.terminator.value != sub->result) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(sub->lhs);
  const auto rhs = parse_i64(sub->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  return *lhs - *rhs;
}

std::optional<std::int64_t> parse_minimal_return_sub_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* sub = get_binary_inst(block.insts.front());
  if (sub == nullptr || sub->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
      sub->result.kind != c4c::backend::bir::Value::Kind::Named ||
      sub->result.type != c4c::backend::bir::TypeKind::I32 ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->name != sub->result.name) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(sub->lhs);
  const auto rhs = parse_i64(sub->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  return *lhs - *rhs;
}

std::optional<MinimalAffineReturnSlice> parse_minimal_affine_return_slice(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.signature.params.size() > 2 || function.signature.params.empty()) {
    return std::nullopt;
  }
  for (const auto& param : function.signature.params) {
    if (!is_i32_scalar_param(param)) {
      return std::nullopt;
    }
  }

  std::vector<std::string_view> param_names;
  param_names.reserve(function.signature.params.size());
  for (const auto& param : function.signature.params) {
    param_names.push_back(param.name);
  }
  const auto& block = function.blocks.front();
  std::unordered_map<std::string, AffineValue> values;
  for (const auto& inst : block.insts) {
    const auto* bin = std::get_if<c4c::backend::BackendBinaryInst>(&inst);
    if (bin == nullptr || !is_i32_scalar_binary(*bin) || bin->result.empty()) {
      return std::nullopt;
    }
    const auto lhs = lower_affine_operand(bin->lhs, param_names, values);
    const auto rhs = lower_affine_operand(bin->rhs, param_names, values);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }
    const auto combined = combine_affine_values(*lhs, *rhs, bin->opcode);
    if (!combined.has_value()) {
      return std::nullopt;
    }
    values[bin->result] = *combined;
  }

  const auto lowered_return =
      lower_affine_operand(*block.terminator.value, param_names, values);
  if (!lowered_return.has_value()) {
    return std::nullopt;
  }

  return MinimalAffineReturnSlice{
      function.signature.name,
      function.signature.params.size(),
      lowered_return->uses_first_param,
      lowered_return->uses_second_param,
      lowered_return->constant,
  };
}

std::optional<AffineValue> lower_affine_operand(
    const c4c::backend::bir::Value& value,
    const std::vector<std::string_view>& param_names,
    const std::unordered_map<std::string, AffineValue>& values) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return AffineValue{false, false, value.immediate};
  }
  if (!param_names.empty() && value.name == param_names[0]) {
    return AffineValue{true, false, 0};
  }
  if (param_names.size() > 1 && value.name == param_names[1]) {
    return AffineValue{false, true, 0};
  }
  auto it = values.find(value.name);
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<MinimalAffineReturnSlice> parse_minimal_affine_return_slice(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.params.size() > 2) {
    return std::nullopt;
  }
  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.name.empty()) {
      return std::nullopt;
    }
  }

  std::vector<std::string_view> param_names;
  param_names.reserve(function.params.size());
  for (const auto& param : function.params) {
    param_names.push_back(param.name);
  }
  const auto& block = function.blocks.front();
  std::unordered_map<std::string, AffineValue> values;
  for (const auto& inst : block.insts) {
    const auto* bin = get_binary_inst(inst);
    if (bin == nullptr || bin->result.kind != c4c::backend::bir::Value::Kind::Named ||
        bin->result.type != c4c::backend::bir::TypeKind::I32 ||
        bin->result.name.empty()) {
      return std::nullopt;
    }
    const auto lhs = lower_affine_operand(bin->lhs, param_names, values);
    const auto rhs = lower_affine_operand(bin->rhs, param_names, values);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }
    const auto combined = combine_affine_values(*lhs, *rhs, bin->opcode);
    if (!combined.has_value()) {
      return std::nullopt;
    }
    values[bin->result.name] = *combined;
  }

  const auto lowered_return =
      lower_affine_operand(*block.terminator.value, param_names, values);
  if (!lowered_return.has_value()) {
    return std::nullopt;
  }

  return MinimalAffineReturnSlice{
      function.name,
      function.params.size(),
      lowered_return->uses_first_param,
      lowered_return->uses_second_param,
      lowered_return->constant,
  };
}

std::optional<MinimalCastReturnSlice> parse_minimal_cast_return_slice(
    const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.blocks.size() != 1 ||
      function.params.size() != 1 || function.blocks.front().insts.size() != 1 ||
      !function.blocks.front().terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry") {
    return std::nullopt;
  }

  const auto& param = function.params.front();
  if (param.name.empty()) {
    return std::nullopt;
  }

  const auto* cast =
      std::get_if<c4c::backend::bir::CastInst>(&block.insts.front());
  if (cast == nullptr || cast->result.kind != c4c::backend::bir::Value::Kind::Named ||
      cast->result.name.empty() || cast->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      cast->operand.name != param.name || cast->operand.type != param.type ||
      cast->result.type != function.return_type ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->name != cast->result.name ||
      block.terminator.value->type != cast->result.type) {
    return std::nullopt;
  }

  return MinimalCastReturnSlice{
      function.name,
      cast->opcode,
      cast->operand.type,
      cast->result.type,
  };
}

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::I32 ||
      function.blocks.size() != 1 || function.params.size() > 2) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&block.insts.front());
  if (select == nullptr ||
      select->result.kind != c4c::backend::bir::Value::Kind::Named ||
      select->result.type != c4c::backend::bir::TypeKind::I32 ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->name != select->result.name ||
      block.terminator.value->type != select->result.type) {
    return std::nullopt;
  }

  const auto predicate = lower_bir_cmp_predicate(select->predicate);
  const auto lhs = lower_bir_i32_value_source(select->lhs, function.params);
  const auto rhs = lower_bir_i32_value_source(select->rhs, function.params);
  const auto true_value = lower_bir_i32_value_source(select->true_value, function.params);
  const auto false_value = lower_bir_i32_value_source(select->false_value, function.params);
  if (!predicate.has_value() || !lhs.has_value() || !rhs.has_value() ||
      !true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  return MinimalConditionalReturnSlice{
      function.name,
      function.params.size(),
      *predicate,
      *lhs,
      *rhs,
      "select_true",
      "select_false",
      *true_value,
      *false_value,
  };
}

std::optional<MinimalConditionalAffineI8ReturnSlice> parse_minimal_conditional_affine_i8_return_slice(
    const c4c::backend::bir::Module& module) {
  using Slice = MinimalConditionalAffineI8ReturnSlice;
  using ValueExpr = MinimalConditionalAffineI8ReturnSlice::ValueExpr;

  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::I8 ||
      function.blocks.size() != 1 || function.params.size() != 2) {
    return std::nullopt;
  }
  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::I8 || param.name.empty()) {
      return std::nullopt;
    }
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.empty() || !block.terminator.value.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, ValueExpr> prefix_values;
  std::size_t select_index = 0;
  const c4c::backend::bir::SelectInst* select = nullptr;
  for (; select_index < block.insts.size(); ++select_index) {
    if (const auto* current_select =
            std::get_if<c4c::backend::bir::SelectInst>(&block.insts[select_index])) {
      select = current_select;
      break;
    }

    const auto* binary = get_binary_inst(block.insts[select_index]);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I8 || binary->result.name.empty()) {
      return std::nullopt;
    }
    auto lhs = lower_bir_i8_value_expr(binary->lhs, function.params, prefix_values);
    auto rhs = lower_bir_i8_value_expr(binary->rhs, function.params, prefix_values);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    ValueExpr combined;
    if (rhs->kind == Slice::ValueKind::Immediate && rhs->steps.empty()) {
      combined = *lhs;
      combined.steps.push_back({binary->opcode, rhs->imm});
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
               lhs->kind == Slice::ValueKind::Immediate && lhs->steps.empty()) {
      combined = *rhs;
      combined.steps.push_back({binary->opcode, lhs->imm});
    } else {
      return std::nullopt;
    }
    prefix_values[binary->result.name] = std::move(combined);
  }

  if (select == nullptr || select_index >= block.insts.size() ||
      select->result.kind != c4c::backend::bir::Value::Kind::Named ||
      select->result.type != c4c::backend::bir::TypeKind::I8 || select->result.name.empty()) {
    return std::nullopt;
  }

  const auto predicate = lower_bir_cmp_predicate(select->predicate);
  const auto lhs = lower_bir_i8_value_expr(select->lhs, function.params, prefix_values);
  const auto rhs = lower_bir_i8_value_expr(select->rhs, function.params, prefix_values);
  const auto true_value =
      lower_bir_i8_value_expr(select->true_value, function.params, prefix_values);
  const auto false_value =
      lower_bir_i8_value_expr(select->false_value, function.params, prefix_values);
  if (!predicate.has_value() || !lhs.has_value() || !rhs.has_value() ||
      !true_value.has_value() || !false_value.has_value() || !lhs->steps.empty() ||
      !rhs->steps.empty()) {
    return std::nullopt;
  }

  std::string current_name = select->result.name;
  std::vector<Slice::ArithmeticStep> post_steps;
  for (std::size_t index = select_index + 1; index < block.insts.size(); ++index) {
    const auto* binary = get_binary_inst(block.insts[index]);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I8 || binary->result.name.empty()) {
      return std::nullopt;
    }

    const bool lhs_is_current =
        binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->lhs.type == c4c::backend::bir::TypeKind::I8 && binary->lhs.name == current_name;
    const bool rhs_is_current =
        binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->rhs.type == c4c::backend::bir::TypeKind::I8 && binary->rhs.name == current_name;
    if (lhs_is_current == rhs_is_current) {
      return std::nullopt;
    }

    std::optional<std::int64_t> immediate;
    if (lhs_is_current && binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary->rhs.type == c4c::backend::bir::TypeKind::I8) {
      immediate = binary->rhs.immediate;
    } else if (rhs_is_current && binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
               binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
               binary->lhs.type == c4c::backend::bir::TypeKind::I8) {
      immediate = binary->lhs.immediate;
    } else {
      return std::nullopt;
    }

    post_steps.push_back({binary->opcode, *immediate});
    current_name = binary->result.name;
  }

  if (block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I8 ||
      block.terminator.value->name != current_name) {
    return std::nullopt;
  }

  return Slice{
      function.name,
      *predicate,
      *lhs,
      *rhs,
      "select_true",
      "select_false",
      "select_join",
      *true_value,
      *false_value,
      std::move(post_steps),
  };
}

std::optional<MinimalConditionalAffineI32ReturnSlice>
parse_minimal_conditional_affine_i32_return_slice(
    const c4c::backend::bir::Module& module) {
  using Slice = MinimalConditionalAffineI32ReturnSlice;
  using ValueExpr = MinimalConditionalAffineI32ReturnSlice::ValueExpr;

  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::I32 ||
      function.blocks.size() != 1 || function.params.size() != 2) {
    return std::nullopt;
  }
  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.name.empty()) {
      return std::nullopt;
    }
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.empty() || !block.terminator.value.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, ValueExpr> prefix_values;
  std::size_t select_index = 0;
  const c4c::backend::bir::SelectInst* select = nullptr;
  for (; select_index < block.insts.size(); ++select_index) {
    if (const auto* current_select =
            std::get_if<c4c::backend::bir::SelectInst>(&block.insts[select_index])) {
      select = current_select;
      break;
    }

    const auto* binary = get_binary_inst(block.insts[select_index]);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 || binary->result.name.empty()) {
      return std::nullopt;
    }
    auto lhs = lower_bir_i32_value_expr(binary->lhs, function.params, prefix_values);
    auto rhs = lower_bir_i32_value_expr(binary->rhs, function.params, prefix_values);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    ValueExpr combined;
    if (rhs->kind == Slice::ValueKind::Immediate && rhs->steps.empty()) {
      combined = *lhs;
      combined.steps.push_back({binary->opcode, rhs->imm});
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
               lhs->kind == Slice::ValueKind::Immediate && lhs->steps.empty()) {
      combined = *rhs;
      combined.steps.push_back({binary->opcode, lhs->imm});
    } else {
      return std::nullopt;
    }
    prefix_values[binary->result.name] = std::move(combined);
  }

  if (select == nullptr || select_index >= block.insts.size() ||
      select->result.kind != c4c::backend::bir::Value::Kind::Named ||
      select->result.type != c4c::backend::bir::TypeKind::I32 || select->result.name.empty()) {
    return std::nullopt;
  }

  const auto predicate = lower_bir_cmp_predicate(select->predicate);
  const auto lhs = lower_bir_i32_value_expr(select->lhs, function.params, prefix_values);
  const auto rhs = lower_bir_i32_value_expr(select->rhs, function.params, prefix_values);
  const auto true_value =
      lower_bir_i32_value_expr(select->true_value, function.params, prefix_values);
  const auto false_value =
      lower_bir_i32_value_expr(select->false_value, function.params, prefix_values);
  if (!predicate.has_value() || !lhs.has_value() || !rhs.has_value() ||
      !true_value.has_value() || !false_value.has_value() || !lhs->steps.empty() ||
      !rhs->steps.empty()) {
    return std::nullopt;
  }

  std::string current_name = select->result.name;
  std::vector<Slice::ArithmeticStep> post_steps;
  for (std::size_t index = select_index + 1; index < block.insts.size(); ++index) {
    const auto* binary = get_binary_inst(block.insts[index]);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 || binary->result.name.empty()) {
      return std::nullopt;
    }

    const bool lhs_is_current =
        binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->lhs.type == c4c::backend::bir::TypeKind::I32 && binary->lhs.name == current_name;
    const bool rhs_is_current =
        binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->rhs.type == c4c::backend::bir::TypeKind::I32 && binary->rhs.name == current_name;
    if (lhs_is_current == rhs_is_current) {
      return std::nullopt;
    }

    std::optional<std::int64_t> immediate;
    if (lhs_is_current && binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary->rhs.type == c4c::backend::bir::TypeKind::I32) {
      immediate = binary->rhs.immediate;
    } else if (rhs_is_current && binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
               binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
               binary->lhs.type == c4c::backend::bir::TypeKind::I32) {
      immediate = binary->lhs.immediate;
    } else {
      return std::nullopt;
    }

    post_steps.push_back({binary->opcode, *immediate});
    current_name = binary->result.name;
  }

  if (block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32 ||
      block.terminator.value->name != current_name) {
    return std::nullopt;
  }

  return Slice{
      function.name,
      *predicate,
      *lhs,
      *rhs,
      "select_true",
      "select_false",
      "select_join",
      *true_value,
      *false_value,
      std::move(post_steps),
  };
}

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      c4c::backend::backend_signature_return_scalar_type(function.signature) !=
          c4c::backend::BackendScalarType::I32 ||
      function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 3) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  if (entry.label != "entry" || entry.insts.size() != 1 ||
      entry.terminator.kind != c4c::backend::BackendTerminatorKind::CondBranch ||
      true_block.label != entry.terminator.true_label ||
      false_block.label != entry.terminator.false_label ||
      !true_block.insts.empty() || !false_block.insts.empty() ||
      true_block.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      false_block.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      !true_block.terminator.value.has_value() || !false_block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(true_block.terminator) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_return_scalar_type(false_block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&entry.insts.front());
  if (cmp == nullptr || cmp->result != entry.terminator.cond_name ||
      c4c::backend::backend_compare_operand_type(*cmp) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto lhs_imm = parse_i64(cmp->lhs);
  const auto rhs_imm = parse_i64(cmp->rhs);
  const auto true_return_imm = parse_i64(*true_block.terminator.value);
  const auto false_return_imm = parse_i64(*false_block.terminator.value);
  if (!lhs_imm.has_value() || !rhs_imm.has_value() || !true_return_imm.has_value() ||
      !false_return_imm.has_value()) {
    return std::nullopt;
  }

  using BackendPred = c4c::backend::BackendComparePredicate;
  using LirPred = c4c::codegen::lir::LirCmpPredicate;
  LirPred predicate = LirPred::Eq;
  switch (cmp->predicate) {
    case BackendPred::Slt: predicate = LirPred::Slt; break;
    case BackendPred::Sle: predicate = LirPred::Sle; break;
    case BackendPred::Sgt: predicate = LirPred::Sgt; break;
    case BackendPred::Sge: predicate = LirPred::Sge; break;
    case BackendPred::Eq: predicate = LirPred::Eq; break;
    case BackendPred::Ne: predicate = LirPred::Ne; break;
    case BackendPred::Ult: predicate = LirPred::Ult; break;
    case BackendPred::Ule: predicate = LirPred::Ule; break;
    case BackendPred::Ugt: predicate = LirPred::Ugt; break;
    case BackendPred::Uge: predicate = LirPred::Uge; break;
  }

  return MinimalConditionalReturnSlice{
      "main",
      0,
      predicate,
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *lhs_imm},
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *rhs_imm},
      true_block.label,
      false_block.label,
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *true_return_imm},
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *false_return_imm},
  };
}

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1) return std::nullopt;

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 3 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  if (entry.label != "entry" || entry.insts.size() != 3) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  const auto cmp0_predicate = cmp0 == nullptr ? std::nullopt : cmp0->predicate.typed();
  const auto cmp1_predicate = cmp1 == nullptr ? std::nullopt : cmp1->predicate.typed();
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || !cmp0_predicate.has_value() ||
      (*cmp0_predicate != LirCmpPredicate::Slt && *cmp0_predicate != LirCmpPredicate::Sle &&
       *cmp0_predicate != LirCmpPredicate::Sgt && *cmp0_predicate != LirCmpPredicate::Sge &&
       *cmp0_predicate != LirCmpPredicate::Eq && *cmp0_predicate != LirCmpPredicate::Ne &&
       *cmp0_predicate != LirCmpPredicate::Ult && *cmp0_predicate != LirCmpPredicate::Ule &&
       *cmp0_predicate != LirCmpPredicate::Ugt && *cmp0_predicate != LirCmpPredicate::Uge) ||
      cmp0->type_str != "i32" ||
      cast->kind != LirCastKind::ZExt || cast->from_type != "i1" ||
      cast->operand != cmp0->result || cast->to_type != "i32" || cmp1->is_float ||
      cmp1_predicate != LirCmpPredicate::Ne || cmp1->type_str != "i32" ||
      cmp1->lhs != cast->result || cmp1->rhs != "0" || condbr->cond_name != cmp1->result) {
    return std::nullopt;
  }

  const auto lhs_imm = parse_i64(cmp0->lhs);
  const auto rhs_imm = parse_i64(cmp0->rhs);
  if (!lhs_imm.has_value() || !rhs_imm.has_value()) {
    return std::nullopt;
  }

  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  if (true_block.label != condbr->true_label || false_block.label != condbr->false_label ||
      !true_block.insts.empty() || !false_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* true_ret = std::get_if<LirRet>(&true_block.terminator);
  const auto* false_ret = std::get_if<LirRet>(&false_block.terminator);
  if (true_ret == nullptr || false_ret == nullptr || !true_ret->value_str.has_value() ||
      !false_ret->value_str.has_value() || true_ret->type_str != "i32" ||
      false_ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto true_return_imm = parse_i64(*true_ret->value_str);
  const auto false_return_imm = parse_i64(*false_ret->value_str);
  if (!true_return_imm.has_value() || !false_return_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalConditionalReturnSlice{
      "main",
      0,
      *cmp0_predicate,
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *lhs_imm},
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *rhs_imm},
      condbr->true_label,
      condbr->false_label,
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *true_return_imm},
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *false_return_imm},
  };
}

std::optional<MinimalCountdownLoopSlice> parse_minimal_countdown_loop_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      c4c::backend::backend_signature_return_scalar_type(function.signature) !=
          c4c::backend::BackendScalarType::I32 ||
      function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 4) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& loop = function.blocks[1];
  const auto& body = function.blocks[2];
  const auto& exit = function.blocks[3];
  if (entry.label != "entry" || !entry.insts.empty() ||
      entry.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      entry.terminator.target_label != loop.label || loop.insts.size() != 2 ||
      loop.terminator.kind != c4c::backend::BackendTerminatorKind::CondBranch ||
      loop.terminator.true_label != body.label ||
      loop.terminator.false_label != exit.label || body.insts.size() != 1 ||
      body.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      body.terminator.target_label != loop.label || !exit.insts.empty() ||
      exit.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      !exit.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(exit.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* phi = std::get_if<c4c::backend::BackendPhiInst>(&loop.insts[0]);
  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&loop.insts[1]);
  const auto* dec = std::get_if<c4c::backend::BackendBinaryInst>(&body.insts.front());
  if (phi == nullptr || cmp == nullptr || dec == nullptr ||
      c4c::backend::backend_phi_value_type(*phi) != c4c::backend::BackendScalarType::I32 ||
      phi->incoming.size() != 2 || phi->incoming[0].label != entry.label ||
      phi->incoming[1].label != body.label || cmp->predicate != c4c::backend::BackendComparePredicate::Ne ||
      cmp->result != loop.terminator.cond_name ||
      c4c::backend::backend_compare_operand_type(*cmp) !=
          c4c::backend::BackendScalarType::I32 ||
      cmp->lhs != phi->result || cmp->rhs != "0" ||
      dec->opcode != c4c::backend::BackendBinaryOpcode::Sub ||
      c4c::backend::backend_binary_value_type(*dec) != c4c::backend::BackendScalarType::I32 ||
      dec->lhs != phi->result || dec->rhs != "1" || dec->result != phi->incoming[1].value ||
      *exit.terminator.value != phi->result) {
    return std::nullopt;
  }

  const auto initial_imm = parse_i64(phi->incoming[0].value);
  if (!initial_imm.has_value() || *initial_imm < 0) {
    return std::nullopt;
  }

  return MinimalCountdownLoopSlice{*initial_imm, loop.label, body.label, exit.label};
}

std::optional<MinimalConditionalPhiJoinSlice> parse_minimal_conditional_phi_join_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      c4c::backend::backend_signature_return_scalar_type(function.signature) !=
          c4c::backend::BackendScalarType::I32 ||
      function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 4) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  const auto& join_block = function.blocks[3];
  const auto parse_incoming_value =
      [](const c4c::backend::BackendBlock& block,
         std::string_view expected_result)
      -> std::optional<MinimalConditionalPhiJoinSlice::IncomingValue> {
    if (block.insts.empty()) {
      const auto imm = parse_i64(expected_result);
      if (!imm.has_value()) {
        return std::nullopt;
      }
      return MinimalConditionalPhiJoinSlice::IncomingValue{*imm, {}};
    }
    MinimalConditionalPhiJoinSlice::IncomingValue value;
    std::string previous_result;
    for (std::size_t index = 0; index < block.insts.size(); ++index) {
      const auto* inst = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts[index]);
      if (inst == nullptr ||
          c4c::backend::backend_binary_value_type(*inst) !=
              c4c::backend::BackendScalarType::I32 ||
          (inst->opcode != c4c::backend::BackendBinaryOpcode::Add &&
           inst->opcode != c4c::backend::BackendBinaryOpcode::Sub)) {
        return std::nullopt;
      }
      const auto rhs_imm = parse_i64(inst->rhs);
      if (!rhs_imm.has_value()) {
        return std::nullopt;
      }
      if (index == 0) {
        const auto base_imm = parse_i64(inst->lhs);
        if (!base_imm.has_value()) {
          return std::nullopt;
        }
        value.base_imm = *base_imm;
      } else if (inst->lhs != previous_result) {
        return std::nullopt;
      }
      value.steps.push_back({inst->opcode, *rhs_imm});
      previous_result = inst->result;
    }
    if (previous_result != expected_result) {
      return std::nullopt;
    }
    return value;
  };
  if (entry.label != "entry" || entry.insts.size() != 1 ||
      entry.terminator.kind != c4c::backend::BackendTerminatorKind::CondBranch ||
      true_block.label != entry.terminator.true_label ||
      false_block.label != entry.terminator.false_label ||
      true_block.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      false_block.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      true_block.terminator.target_label != join_block.label ||
      false_block.terminator.target_label != join_block.label ||
      (join_block.insts.size() != 1 && join_block.insts.size() != 2) ||
      join_block.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      !join_block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(join_block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&entry.insts.front());
  const auto* phi = std::get_if<c4c::backend::BackendPhiInst>(&join_block.insts.front());
  if (cmp == nullptr || phi == nullptr || cmp->result != entry.terminator.cond_name ||
      c4c::backend::backend_compare_operand_type(*cmp) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_phi_value_type(*phi) != c4c::backend::BackendScalarType::I32 ||
      phi->incoming.size() != 2 ||
      phi->incoming[0].label != true_block.label ||
      phi->incoming[1].label != false_block.label) {
    return std::nullopt;
  }

  std::optional<std::int64_t> join_add_imm;
  if (join_block.insts.size() == 1) {
    if (*join_block.terminator.value != phi->result) {
      return std::nullopt;
    }
  } else {
    const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&join_block.insts[1]);
    if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
        c4c::backend::backend_binary_value_type(*add) !=
            c4c::backend::BackendScalarType::I32 ||
        add->lhs != phi->result ||
        *join_block.terminator.value != add->result) {
      return std::nullopt;
    }
    join_add_imm = parse_i64(add->rhs);
    if (!join_add_imm.has_value()) {
      return std::nullopt;
    }
  }

  const auto lhs_imm = parse_i64(cmp->lhs);
  const auto rhs_imm = parse_i64(cmp->rhs);
  const auto true_value = parse_incoming_value(true_block, phi->incoming[0].value);
  const auto false_value = parse_incoming_value(false_block, phi->incoming[1].value);
  if (!lhs_imm.has_value() || !rhs_imm.has_value() || !true_value.has_value() ||
      !false_value.has_value()) {
    return std::nullopt;
  }

  using BackendPred = c4c::backend::BackendComparePredicate;
  using LirPred = c4c::codegen::lir::LirCmpPredicate;
  LirPred predicate = LirPred::Eq;
  switch (cmp->predicate) {
    case BackendPred::Slt: predicate = LirPred::Slt; break;
    case BackendPred::Sle: predicate = LirPred::Sle; break;
    case BackendPred::Sgt: predicate = LirPred::Sgt; break;
    case BackendPred::Sge: predicate = LirPred::Sge; break;
    case BackendPred::Eq: predicate = LirPred::Eq; break;
    case BackendPred::Ne: predicate = LirPred::Ne; break;
    case BackendPred::Ult: predicate = LirPred::Ult; break;
    case BackendPred::Ule: predicate = LirPred::Ule; break;
    case BackendPred::Ugt: predicate = LirPred::Ugt; break;
    case BackendPred::Uge: predicate = LirPred::Uge; break;
  }

  return MinimalConditionalPhiJoinSlice{predicate,
                                        *lhs_imm,
                                        *rhs_imm,
                                        true_block.label,
                                        false_block.label,
                                        join_block.label,
                                        *true_value,
                                        *false_value,
                                        join_add_imm};
}

std::optional<MinimalLocalArraySlice> parse_minimal_local_array_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (alloca == nullptr || alloca->type_str != "[2 x i32]" || !alloca->count.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::int64_t> widened_indices;
  std::unordered_map<std::string, std::int64_t> local_ptr_offsets;
  std::vector<std::pair<std::int64_t, std::int64_t>> stores;
  std::vector<std::int64_t> loads;
  std::string add_result;

  for (const auto& inst : entry.insts) {
    if (const auto* gep = std::get_if<LirGepOp>(&inst)) {
      if (gep->element_type == "[2 x i32]" && gep->ptr == alloca->result &&
          gep->indices.size() == 2 && gep->indices[0] == "i64 0" &&
          gep->indices[1] == "i64 0") {
        local_ptr_offsets[gep->result] = 0;
        continue;
      }

      if (gep->element_type == "i32" && gep->indices.size() == 1) {
        const auto base = local_ptr_offsets.find(gep->ptr);
        const auto typed_index = strip_typed_operand_prefix(gep->indices.front(), "i64");
        if (base == local_ptr_offsets.end() || !typed_index.has_value()) {
          return std::nullopt;
        }

        std::int64_t index = 0;
        if (const auto imm = parse_i64(*typed_index); imm.has_value()) {
          index = *imm;
        } else {
          const auto widened = widened_indices.find(std::string(*typed_index));
          if (widened == widened_indices.end()) {
            return std::nullopt;
          }
          index = widened->second;
        }

        if (index < 0 || index > 1) {
          return std::nullopt;
        }
        local_ptr_offsets[gep->result] = base->second + index * 4;
        continue;
      }

      return std::nullopt;
    }

    if (const auto* cast = std::get_if<LirCastOp>(&inst)) {
      if (cast->kind != LirCastKind::SExt || cast->from_type != "i32" ||
          cast->to_type != "i64") {
        return std::nullopt;
      }
      const auto imm = parse_i64(cast->operand);
      if (!imm.has_value()) {
        return std::nullopt;
      }
      widened_indices[cast->result] = *imm;
      continue;
    }

    if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
      if (store->type_str != "i32") {
        return std::nullopt;
      }
      const auto ptr = local_ptr_offsets.find(store->ptr);
      const auto imm = parse_i64(store->val);
      if (ptr == local_ptr_offsets.end() || !imm.has_value()) {
        return std::nullopt;
      }
      stores.emplace_back(ptr->second, *imm);
      continue;
    }

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str != "i32") {
        return std::nullopt;
      }
      const auto ptr = local_ptr_offsets.find(load->ptr);
      if (ptr == local_ptr_offsets.end()) {
        return std::nullopt;
      }
      loads.push_back(ptr->second);
      continue;
    }

    if (const auto* add = std::get_if<LirBinOp>(&inst)) {
      if (add->opcode != "add" || add->type_str != "i32" || loads.size() != 2) {
        return std::nullopt;
      }
      add_result = add->result;
      continue;
    }

    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i32" ||
      *ret->value_str != add_result || stores.size() != 2 || loads.size() != 2) {
    return std::nullopt;
  }

  std::optional<std::int64_t> store0;
  std::optional<std::int64_t> store1;
  for (const auto& [offset, imm] : stores) {
    if (offset == 0) {
      store0 = imm;
    } else if (offset == 4) {
      store1 = imm;
    } else {
      return std::nullopt;
    }
  }
  if (!store0.has_value() || !store1.has_value()) {
    return std::nullopt;
  }

  if (!((loads[0] == 0 && loads[1] == 4) || (loads[0] == 4 && loads[1] == 0))) {
    return std::nullopt;
  }

  return MinimalLocalArraySlice{*store0, *store1};
}

std::optional<MinimalExternGlobalArrayLoadSlice> parse_minimal_extern_global_array_load_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || !global.is_extern_decl ||
      global.linkage_vis != "external " || global.qualifier != "global " ||
      !global.init_text.empty()) {
    return std::nullopt;
  }

  const std::string element_prefix = "[";
  const std::string element_suffix = " x i32]";
  if (global.llvm_type.size() <= element_prefix.size() + element_suffix.size() ||
      global.llvm_type.substr(0, element_prefix.size()) != element_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - element_suffix.size()) !=
          element_suffix) {
    return std::nullopt;
  }

  const auto element_count_text = global.llvm_type.substr(
      element_prefix.size(),
      global.llvm_type.size() - element_prefix.size() - element_suffix.size());
  const auto element_count = parse_i64(element_count_text);
  if (!element_count.has_value() || *element_count <= 0) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 4) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index_cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* elem_gep = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[3]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep == nullptr || index_cast == nullptr || elem_gep == nullptr ||
      load == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  if (base_gep->element_type != global.llvm_type || base_gep->ptr != "@" + global.name ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0") {
    return std::nullopt;
  }

  if (index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
      index_cast->to_type != "i64") {
    return std::nullopt;
  }

  const auto element_index = parse_i64(index_cast->operand);
  if (!element_index.has_value() || *element_index < 0 || *element_index >= *element_count) {
    return std::nullopt;
  }

  if (elem_gep->element_type != "i32" || elem_gep->ptr != base_gep->result ||
      elem_gep->indices.size() != 1 ||
      elem_gep->indices[0] != ("i64 " + index_cast->result)) {
    return std::nullopt;
  }

  if (load->type_str != "i32" || load->ptr != elem_gep->result ||
      !ret->value_str.has_value() || *ret->value_str != load->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{global.name, *element_index * 4};
}

std::optional<MinimalExternGlobalArrayLoadSlice> parse_minimal_extern_global_array_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  const auto global_array_type =
      global == nullptr ? std::nullopt : c4c::backend::backend_global_array_type(*global);
  if (global == nullptr || !c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable ||
      c4c::backend::backend_global_linkage(*global) !=
          c4c::backend::BackendGlobalLinkage::External ||
      !global_array_type.has_value() ||
      global_array_type->element_type_kind != c4c::backend::BackendValueTypeKind::Scalar ||
      global_array_type->element_scalar_type != c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  const auto element_count = static_cast<std::int64_t>(global_array_type->element_count);
  const auto element_size = static_cast<std::int64_t>(sizeof(std::int32_t));
  if (load == nullptr ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      load->address.base_symbol != global->name ||
      *block.terminator.value != load->result || load->address.byte_offset < 0 ||
      load->address.byte_offset % element_size != 0 ||
      load->address.byte_offset / element_size >= element_count) {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{global->name, load->address.byte_offset};
}

std::optional<MinimalLocalArraySlice> parse_minimal_local_array_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 5 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* store0 = std::get_if<c4c::backend::BackendStoreInst>(&block.insts[0]);
  const auto* store1 = std::get_if<c4c::backend::BackendStoreInst>(&block.insts[1]);
  const auto* load0 = std::get_if<c4c::backend::BackendLoadInst>(&block.insts[2]);
  const auto* load1 = std::get_if<c4c::backend::BackendLoadInst>(&block.insts[3]);
  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts[4]);
  if (store0 == nullptr || store1 == nullptr || load0 == nullptr || load1 == nullptr ||
      add == nullptr ||
      c4c::backend::backend_store_value_type(*store0) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_store_value_type(*store1) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_value_type(*load0) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_value_type(*load1) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load0) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load1) !=
          c4c::backend::BackendScalarType::I32 ||
      add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      c4c::backend::backend_binary_value_type(*add) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  if (store0->address.base_symbol.empty() ||
      store0->address.base_symbol != store1->address.base_symbol ||
      store0->address.base_symbol != load0->address.base_symbol ||
      store0->address.base_symbol != load1->address.base_symbol ||
      store0->address.byte_offset != 0 || store1->address.byte_offset != 4 ||
      load0->address.byte_offset != 0 || load1->address.byte_offset != 4) {
    return std::nullopt;
  }
  if (store0->address.kind != c4c::backend::BackendAddressBaseKind::LocalSlot ||
      store1->address.kind != c4c::backend::BackendAddressBaseKind::LocalSlot ||
      load0->address.kind != c4c::backend::BackendAddressBaseKind::LocalSlot ||
      load1->address.kind != c4c::backend::BackendAddressBaseKind::LocalSlot ||
      !is_two_element_i32_local_array_slot(*main_fn, store0->address.base_symbol)) {
    return std::nullopt;
  }

  const auto store0_imm = parse_i64(store0->value);
  const auto store1_imm = parse_i64(store1->value);
  if (!store0_imm.has_value() || !store1_imm.has_value() ||
      *block.terminator.value != add->result) {
    return std::nullopt;
  }

  const bool add_matches_loads =
      (add->lhs == load0->result && add->rhs == load1->result) ||
      (add->lhs == load1->result && add->rhs == load0->result);
  if (!add_matches_loads) {
    return std::nullopt;
  }

  return MinimalLocalArraySlice{*store0_imm, *store1_imm};
}

std::optional<MinimalExternScalarGlobalLoadSlice> parse_minimal_extern_scalar_global_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || !c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable ||
      c4c::backend::backend_global_linkage(*global) !=
          c4c::backend::BackendGlobalLinkage::External ||
      !is_i32_scalar_global(*global)) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      load->address.base_symbol != global->name || load->address.byte_offset != 0 ||
      *block.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{global->name};
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable ||
      !is_i32_scalar_global(*global)) {
    return std::nullopt;
  }
  std::int64_t init_imm = 0;
  if (!c4c::backend::backend_global_has_integer_initializer(*global, &init_imm)) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      load->address.base_symbol != global->name || load->address.byte_offset != 0 ||
      *block.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{global->name, init_imm};
}

std::optional<MinimalGlobalCharPointerDiffSlice> parse_minimal_global_char_pointer_diff_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  const auto global_array_type =
      global == nullptr ? std::nullopt : c4c::backend::backend_global_array_type(*global);
  if (global == nullptr || c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable) {
    return std::nullopt;
  }
  if (!global_array_type.has_value() ||
      global_array_type->element_type_kind != c4c::backend::BackendValueTypeKind::Scalar ||
      global_array_type->element_scalar_type != c4c::backend::BackendScalarType::I8) {
    return std::nullopt;
  }
  const auto global_size = static_cast<std::int64_t>(global_array_type->element_count);
  if (global_size < 2) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* ptrdiff = std::get_if<c4c::backend::BackendPtrDiffEqInst>(&block.insts.front());
  if (ptrdiff == nullptr ||
      c4c::backend::backend_ptrdiff_result_type(*ptrdiff) !=
          c4c::backend::BackendScalarType::I32 ||
      ptrdiff->lhs_address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      ptrdiff->rhs_address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      ptrdiff->lhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.byte_offset != 0 ||
      c4c::backend::backend_ptrdiff_element_type(*ptrdiff) !=
          c4c::backend::BackendScalarType::I8 ||
      ptrdiff->element_size != 1 ||
      ptrdiff->expected_diff != ptrdiff->lhs_address.byte_offset ||
      ptrdiff->lhs_address.byte_offset <= 0 ||
      ptrdiff->lhs_address.byte_offset >= global_size ||
      *block.terminator.value != ptrdiff->result) {
    return std::nullopt;
  }

  return MinimalGlobalCharPointerDiffSlice{
      global->name,
      global_size,
      ptrdiff->lhs_address.byte_offset,
  };
}

std::optional<MinimalGlobalIntPointerDiffSlice> parse_minimal_global_int_pointer_diff_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  const auto global_array_type =
      global == nullptr ? std::nullopt : c4c::backend::backend_global_array_type(*global);
  if (global == nullptr || c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable) {
    return std::nullopt;
  }
  if (!global_array_type.has_value() ||
      global_array_type->element_type_kind != c4c::backend::BackendValueTypeKind::Scalar ||
      global_array_type->element_scalar_type != c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }
  const auto global_size = static_cast<std::int64_t>(global_array_type->element_count);
  if (global_size < 2) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* ptrdiff = std::get_if<c4c::backend::BackendPtrDiffEqInst>(&block.insts.front());
  if (ptrdiff == nullptr ||
      c4c::backend::backend_ptrdiff_result_type(*ptrdiff) !=
          c4c::backend::BackendScalarType::I32 ||
      ptrdiff->lhs_address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      ptrdiff->rhs_address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      ptrdiff->lhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.byte_offset != 0 || ptrdiff->expected_diff != 1 ||
      ptrdiff->lhs_address.byte_offset != ptrdiff->element_size ||
      c4c::backend::backend_ptrdiff_element_type(*ptrdiff) !=
          c4c::backend::BackendScalarType::I32 ||
      ptrdiff->element_size != 4 || *block.terminator.value != ptrdiff->result) {
    return std::nullopt;
  }

  return MinimalGlobalIntPointerDiffSlice{global->name, global_size, 4, 2};
}

std::optional<MinimalGlobalCharPointerDiffSlice> parse_minimal_global_char_pointer_diff_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global ") {
    return std::nullopt;
  }

  const std::string array_prefix = "[";
  const std::string array_suffix = " x i8]";
  if (global.llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global.llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) !=
          array_suffix) {
    return std::nullopt;
  }

  const auto global_size_text = global.llvm_type.substr(
      array_prefix.size(),
      global.llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_i64(global_size_text);
  if (!global_size.has_value() || *global_size < 2) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 12) {
    return std::nullopt;
  }

  const auto* base_gep1 = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index1 = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* byte_gep1 = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* base_gep0 = std::get_if<LirGepOp>(&entry.insts[3]);
  const auto* index0 = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* byte_gep0 = std::get_if<LirGepOp>(&entry.insts[5]);
  const auto* ptrtoint1 = std::get_if<LirCastOp>(&entry.insts[6]);
  const auto* ptrtoint0 = std::get_if<LirCastOp>(&entry.insts[7]);
  const auto* diff = std::get_if<LirBinOp>(&entry.insts[8]);
  const auto* expected_diff = std::get_if<LirCastOp>(&entry.insts[9]);
  const auto* cmp = std::get_if<LirCmpOp>(&entry.insts[10]);
  const auto* extend = std::get_if<LirCastOp>(&entry.insts[11]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep1 == nullptr || index1 == nullptr || byte_gep1 == nullptr ||
      base_gep0 == nullptr || index0 == nullptr || byte_gep0 == nullptr ||
      ptrtoint1 == nullptr || ptrtoint0 == nullptr || diff == nullptr ||
      expected_diff == nullptr || cmp == nullptr || extend == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  const std::string global_ptr = "@" + global.name;
  if (base_gep1->element_type != global.llvm_type || base_gep1->ptr != global_ptr ||
      base_gep1->indices.size() != 2 || base_gep1->indices[0] != "i64 0" ||
      base_gep1->indices[1] != "i64 0") {
    return std::nullopt;
  }
  if (base_gep0->element_type != global.llvm_type || base_gep0->ptr != global_ptr ||
      base_gep0->indices.size() != 2 || base_gep0->indices[0] != "i64 0" ||
      base_gep0->indices[1] != "i64 0") {
    return std::nullopt;
  }

  if (index1->kind != LirCastKind::SExt || index1->from_type != "i32" ||
      index1->to_type != "i64") {
    return std::nullopt;
  }
  const auto byte_index = parse_i64(index1->operand);
  if (!byte_index.has_value() || *byte_index <= 0 || *byte_index >= *global_size) {
    return std::nullopt;
  }

  if (index0->kind != LirCastKind::SExt || index0->from_type != "i32" ||
      index0->to_type != "i64" || index0->operand != "0") {
    return std::nullopt;
  }

  if (byte_gep1->element_type != "i8" || byte_gep1->ptr != base_gep1->result ||
      byte_gep1->indices.size() != 1 ||
      byte_gep1->indices[0] != ("i64 " + index1->result)) {
    return std::nullopt;
  }
  if (byte_gep0->element_type != "i8" || byte_gep0->ptr != base_gep0->result ||
      byte_gep0->indices.size() != 1 ||
      byte_gep0->indices[0] != ("i64 " + index0->result)) {
    return std::nullopt;
  }

  if (ptrtoint1->kind != LirCastKind::PtrToInt || ptrtoint1->from_type != "ptr" ||
      ptrtoint1->operand != byte_gep1->result || ptrtoint1->to_type != "i64") {
    return std::nullopt;
  }
  if (ptrtoint0->kind != LirCastKind::PtrToInt || ptrtoint0->from_type != "ptr" ||
      ptrtoint0->operand != byte_gep0->result || ptrtoint0->to_type != "i64") {
    return std::nullopt;
  }

  const auto diff_opcode = diff->opcode.typed();
  if (diff_opcode != LirBinaryOpcode::Sub || diff->type_str != "i64" ||
      diff->lhs != ptrtoint1->result ||
      diff->rhs != ptrtoint0->result) {
    return std::nullopt;
  }

  if (expected_diff->kind != LirCastKind::SExt || expected_diff->from_type != "i32" ||
      expected_diff->to_type != "i64") {
    return std::nullopt;
  }
  const auto expected_value = parse_i64(expected_diff->operand);
  if (!expected_value.has_value() || *expected_value != *byte_index) {
    return std::nullopt;
  }

  const auto cmp_predicate = cmp->predicate.typed();
  if (cmp->is_float || cmp_predicate != LirCmpPredicate::Eq || cmp->type_str != "i64" ||
      cmp->lhs != diff->result ||
      cmp->rhs != expected_diff->result) {
    return std::nullopt;
  }

  if (extend->kind != LirCastKind::ZExt || extend->from_type != "i1" ||
      extend->operand != cmp->result || extend->to_type != "i32") {
    return std::nullopt;
  }

  if (!ret->value_str.has_value() || *ret->value_str != extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalGlobalCharPointerDiffSlice{global.name, *global_size, *byte_index};
}

std::optional<MinimalGlobalIntPointerDiffSlice> parse_minimal_global_int_pointer_diff_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.align_bytes != 4 ||
      (global.init_text != "zeroinitializer" && global.init_text != "[i32 0, i32 0]")) {
    return std::nullopt;
  }

  const std::string array_prefix = "[";
  const std::string array_suffix = " x i32]";
  if (global.llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global.llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) !=
          array_suffix) {
    return std::nullopt;
  }

  const auto global_size_text = global.llvm_type.substr(
      array_prefix.size(),
      global.llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_i64(global_size_text);
  if (!global_size.has_value() || *global_size != 2) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 13) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index1 = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* elem_gep = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* base_gep0 = std::get_if<LirGepOp>(&entry.insts[3]);
  const auto* index0 = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* elem_gep0 = std::get_if<LirGepOp>(&entry.insts[5]);
  const auto* ptrtoint1 = std::get_if<LirCastOp>(&entry.insts[6]);
  const auto* ptrtoint0 = std::get_if<LirCastOp>(&entry.insts[7]);
  const auto* diff = std::get_if<LirBinOp>(&entry.insts[8]);
  const auto* scaled_diff = std::get_if<LirBinOp>(&entry.insts[9]);
  const auto* expected_diff = std::get_if<LirCastOp>(&entry.insts[10]);
  const auto* cmp = std::get_if<LirCmpOp>(&entry.insts[11]);
  const auto* final_extend = std::get_if<LirCastOp>(&entry.insts[12]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep == nullptr || index1 == nullptr || elem_gep == nullptr ||
      base_gep0 == nullptr || index0 == nullptr || elem_gep0 == nullptr ||
      ptrtoint1 == nullptr || ptrtoint0 == nullptr || diff == nullptr ||
      scaled_diff == nullptr || expected_diff == nullptr || cmp == nullptr ||
      final_extend == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  const std::string global_ptr = "@" + global.name;
  if (base_gep->element_type != global.llvm_type || base_gep->ptr != global_ptr ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0") {
    return std::nullopt;
  }
  if (elem_gep->element_type != "i32" || elem_gep->ptr != base_gep->result ||
      elem_gep->indices.size() != 1 ||
      elem_gep->indices[0] != ("i64 " + index1->result)) {
    return std::nullopt;
  }
  if (base_gep0->element_type != global.llvm_type || base_gep0->ptr != global_ptr ||
      base_gep0->indices.size() != 2 || base_gep0->indices[0] != "i64 0" ||
      base_gep0->indices[1] != "i64 0") {
    return std::nullopt;
  }
  if (index1->kind != LirCastKind::SExt || index1->from_type != "i32" ||
      index1->to_type != "i64" || index1->operand != "1") {
    return std::nullopt;
  }
  if (index0->kind != LirCastKind::SExt || index0->from_type != "i32" ||
      index0->to_type != "i64" || index0->operand != "0") {
    return std::nullopt;
  }
  if (elem_gep0->element_type != "i32" || elem_gep0->ptr != base_gep0->result ||
      elem_gep0->indices.size() != 1 ||
      elem_gep0->indices[0] != ("i64 " + index0->result)) {
    return std::nullopt;
  }

  if (ptrtoint1->kind != LirCastKind::PtrToInt || ptrtoint1->from_type != "ptr" ||
      ptrtoint1->operand != elem_gep->result || ptrtoint1->to_type != "i64") {
    return std::nullopt;
  }
  if (ptrtoint0->kind != LirCastKind::PtrToInt || ptrtoint0->from_type != "ptr" ||
      ptrtoint0->operand != elem_gep0->result || ptrtoint0->to_type != "i64") {
    return std::nullopt;
  }

  const auto diff_opcode = diff->opcode.typed();
  if (diff_opcode != LirBinaryOpcode::Sub || diff->type_str != "i64" ||
      diff->lhs != ptrtoint1->result ||
      diff->rhs != ptrtoint0->result) {
    return std::nullopt;
  }
  const auto scaled_diff_opcode = scaled_diff->opcode.typed();
  if (scaled_diff_opcode != LirBinaryOpcode::SDiv || scaled_diff->type_str != "i64" ||
      scaled_diff->lhs != diff->result || scaled_diff->rhs != "4") {
    return std::nullopt;
  }

  if (expected_diff->kind != LirCastKind::SExt || expected_diff->from_type != "i32" ||
      expected_diff->to_type != "i64" || expected_diff->operand != "1") {
    return std::nullopt;
  }

  const auto cmp_predicate = cmp->predicate.typed();
  if (cmp->is_float || cmp_predicate != LirCmpPredicate::Eq || cmp->type_str != "i64" ||
      cmp->lhs != scaled_diff->result ||
      cmp->rhs != expected_diff->result) {
    return std::nullopt;
  }

  if (final_extend->kind != LirCastKind::ZExt || final_extend->from_type != "i1" ||
      final_extend->operand != cmp->result || final_extend->to_type != "i32") {
    return std::nullopt;
  }

  if (!ret->value_str.has_value() || *ret->value_str != final_extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalGlobalIntPointerDiffSlice{global.name, *global_size, 4, 2};
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_global_int_pointer_roundtrip_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global.init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 2 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* addr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (addr_slot == nullptr || ptr_slot == nullptr || addr_slot->result == ptr_slot->result ||
      addr_slot->type_str != "i64" || ptr_slot->type_str != "ptr" ||
      addr_slot->align != 8 || ptr_slot->align != 8) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 7) {
    return std::nullopt;
  }

  const auto* ptrtoint = std::get_if<LirCastOp>(&entry.insts[0]);
  const auto* spill_addr = std::get_if<LirStoreOp>(&entry.insts[1]);
  const auto* reload_addr = std::get_if<LirLoadOp>(&entry.insts[2]);
  const auto* inttoptr = std::get_if<LirCastOp>(&entry.insts[3]);
  const auto* spill_ptr = std::get_if<LirStoreOp>(&entry.insts[4]);
  const auto* reload_ptr = std::get_if<LirLoadOp>(&entry.insts[5]);
  const auto* load_value = std::get_if<LirLoadOp>(&entry.insts[6]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (ptrtoint == nullptr || spill_addr == nullptr || reload_addr == nullptr ||
      inttoptr == nullptr || spill_ptr == nullptr || reload_ptr == nullptr ||
      load_value == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  const std::string global_ptr = "@" + global.name;
  if (ptrtoint->kind != LirCastKind::PtrToInt || ptrtoint->from_type != "ptr" ||
      ptrtoint->operand != global_ptr || ptrtoint->to_type != "i64") {
    return std::nullopt;
  }
  if (spill_addr->type_str != "i64" || spill_addr->val != ptrtoint->result ||
      spill_addr->ptr != addr_slot->result) {
    return std::nullopt;
  }
  if (reload_addr->type_str != "i64" || reload_addr->ptr != addr_slot->result) {
    return std::nullopt;
  }
  if (inttoptr->kind != LirCastKind::IntToPtr || inttoptr->from_type != "i64" ||
      inttoptr->operand != reload_addr->result || inttoptr->to_type != "ptr") {
    return std::nullopt;
  }
  if (spill_ptr->type_str != "ptr" || spill_ptr->val != inttoptr->result ||
      spill_ptr->ptr != ptr_slot->result) {
    return std::nullopt;
  }
  if (reload_ptr->type_str != "ptr" || reload_ptr->ptr != ptr_slot->result) {
    return std::nullopt;
  }
  if (load_value->type_str != "i32" || load_value->ptr != reload_ptr->result) {
    return std::nullopt;
  }
  if (!ret->value_str.has_value() || *ret->value_str != load_value->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{global.name, *init_imm};
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global.init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || entry.insts.size() != 1 || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* load = std::get_if<LirLoadOp>(&entry.insts.front());
  if (load == nullptr || load->type_str != "i32" ||
      load->ptr != ("@" + global.name) || *ret->value_str != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{global.name, *init_imm};
}

std::optional<MinimalScalarGlobalStoreReloadSlice> parse_minimal_scalar_global_store_reload_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global.init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || entry.insts.size() != 2 || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* store = std::get_if<LirStoreOp>(&entry.insts[0]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[1]);
  if (store == nullptr || load == nullptr || store->type_str != "i32" ||
      load->type_str != "i32" || store->ptr != ("@" + global.name) ||
      load->ptr != store->ptr || *ret->value_str != load->result) {
    return std::nullopt;
  }

  const auto store_imm = parse_i64(store->val);
  if (!store_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalScalarGlobalStoreReloadSlice{global.name, *init_imm, *store_imm};
}

std::optional<MinimalStringLiteralCharSlice> parse_minimal_string_literal_char_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.string_pool.size() != 1 ||
      !module.globals.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& string_const = module.string_pool.front();
  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 5) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index_cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* byte_gep = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[3]);
  const auto* extend = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep == nullptr || index_cast == nullptr || byte_gep == nullptr ||
      load == nullptr || extend == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  const auto string_array_type =
      "[" + std::to_string(string_const.byte_length) + " x i8]";
  if (base_gep->element_type != string_array_type || base_gep->ptr != string_const.pool_name ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0") {
    return std::nullopt;
  }

  if (index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
      index_cast->to_type != "i64") {
    return std::nullopt;
  }
  const auto byte_index = parse_i64(index_cast->operand);
  if (!byte_index.has_value() || *byte_index < 0 ||
      *byte_index >= static_cast<std::int64_t>(string_const.byte_length)) {
    return std::nullopt;
  }

  if (byte_gep->element_type != "i8" || byte_gep->ptr != base_gep->result ||
      byte_gep->indices.size() != 1 || byte_gep->indices[0] != ("i64 " + index_cast->result)) {
    return std::nullopt;
  }

  if (load->type_str != "i8" || load->ptr != byte_gep->result) {
    return std::nullopt;
  }

  if ((extend->kind != LirCastKind::SExt && extend->kind != LirCastKind::ZExt) ||
      extend->from_type != "i8" || extend->operand != load->result ||
      extend->to_type != "i32") {
    return std::nullopt;
  }

  if (!ret->value_str.has_value() || *ret->value_str != extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalStringLiteralCharSlice{
      string_const.pool_name,
      string_const.raw_bytes,
      *byte_index,
      extend->kind == LirCastKind::SExt,
  };
}

std::optional<MinimalStringLiteralCharSlice> parse_minimal_string_literal_char_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.string_constants.size() != 1 ||
      !module.globals.empty()) {
    return std::nullopt;
  }

  const auto* string_const =
      find_string_constant(module, module.string_constants.front().name);
  const auto* main_fn = find_function(module, "main");
  if (string_const == nullptr || main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr || load->result != *block.terminator.value ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I8 ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::StringConstant ||
      load->address.base_symbol != string_const->name ||
      load->address.byte_offset < 0 ||
      load->address.byte_offset >=
          static_cast<std::int64_t>(string_const->byte_length)) {
    return std::nullopt;
  }
  if (load->extension != c4c::backend::BackendLoadExtension::SignExtend &&
      load->extension != c4c::backend::BackendLoadExtension::ZeroExtend) {
    return std::nullopt;
  }

  return MinimalStringLiteralCharSlice{
      "@" + string_const->name,
      string_const->raw_bytes,
      load->address.byte_offset,
      load->extension == c4c::backend::BackendLoadExtension::SignExtend,
  };
}

std::optional<MinimalScalarGlobalStoreReloadSlice> parse_minimal_scalar_global_store_reload_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable ||
      !is_i32_scalar_global(*global)) {
    return std::nullopt;
  }
  std::int64_t init_imm = 0;
  if (!c4c::backend::backend_global_has_integer_initializer(*global, &init_imm)) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 2 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* store = std::get_if<c4c::backend::BackendStoreInst>(&block.insts[0]);
  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts[1]);
  if (store == nullptr || load == nullptr ||
      c4c::backend::backend_store_value_type(*store) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      store->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      store->address.base_symbol != global->name ||
      store->address.byte_offset != 0 || load->address.base_symbol != global->name ||
      load->address.byte_offset != 0 || *block.terminator.value != load->result) {
    return std::nullopt;
  }

  const auto store_imm = parse_i64(store->value);
  if (!store_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalScalarGlobalStoreReloadSlice{global->name, init_imm, *store_imm};
}

std::optional<MinimalCallCrossingDirectCallSlice> parse_minimal_call_crossing_direct_call_slice(
    const c4c::backend::BackendModule& module) {
  const auto parsed = c4c::backend::parse_backend_minimal_call_crossing_direct_call_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr ||
      parsed->regalloc_source_value.empty()) {
    return std::nullopt;
  }

  return MinimalCallCrossingDirectCallSlice{
      parsed->helper->signature.name,
      parsed->source_imm,
      parsed->helper_add_imm,
      std::string(parsed->regalloc_source_value),
  };
}

std::optional<MinimalTwoArgDirectCallSlice> parse_minimal_two_arg_direct_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = c4c::backend::parse_backend_minimal_two_arg_direct_call_lir_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr) {
    return std::nullopt;
  }

  return MinimalTwoArgDirectCallSlice{
      parsed->helper->name,
      parsed->lhs_call_arg_imm,
      parsed->rhs_call_arg_imm,
  };
}

void emit_function_prelude(std::ostringstream& out,
                           const c4c::backend::BackendModule& module,
                           std::string_view symbol,
                           bool is_global) {
  (void)module;
  if (is_global) out << ".globl " << symbol << "\n";
  out << ".type " << symbol << ", %function\n";
  out << symbol << ":\n";
}

void emit_function_prelude(std::ostringstream& out,
                           std::string_view symbol,
                           bool is_global) {
  if (is_global) out << ".globl " << symbol << "\n";
  out << ".type " << symbol << ", %function\n";
  out << symbol << ":\n";
}

std::string emit_minimal_return_asm(const c4c::backend::BackendModule& module,
                                    std::int64_t return_imm) {
  const std::string symbol = minimal_single_function_symbol(module);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  mov eax, " << return_imm << "\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_return_asm(const c4c::backend::bir::Module& module,
                                    std::int64_t return_imm) {
  const std::string symbol = minimal_single_function_symbol(module);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  mov eax, " << return_imm << "\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_affine_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalAffineReturnSlice& slice) {
  const std::string symbol = asm_symbol_name(module, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, symbol, true);
  const bool is_i686 = module.target_triple.find("i686") != std::string::npos;
  auto emit_param_move = [&](int index) {
    if (is_i686) {
      out << "  mov eax, DWORD PTR [esp + " << (4 * (index + 1)) << "]\n";
    } else if (index == 0) {
      out << "  mov eax, edi\n";
    } else {
      out << "  mov eax, esi\n";
    }
  };
  auto emit_param_add = [&](int index) {
    if (is_i686) {
      out << "  add eax, DWORD PTR [esp + " << (4 * (index + 1)) << "]\n";
    } else if (index == 0) {
      out << "  add eax, edi\n";
    } else {
      out << "  add eax, esi\n";
    }
  };

  if (slice.uses_first_param) {
    emit_param_move(0);
    if (slice.uses_second_param) {
      emit_param_add(1);
    }
  } else if (slice.uses_second_param) {
    emit_param_move(1);
  } else {
    out << "  mov eax, " << slice.constant << "\n";
    out << "  ret\n";
    return out.str();
  }

  if (slice.constant > 0) {
    out << "  add eax, " << slice.constant << "\n";
  } else if (slice.constant < 0) {
    out << "  sub eax, " << -slice.constant << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_affine_return_asm(
    const c4c::backend::bir::Module& module,
    const MinimalAffineReturnSlice& slice) {
  const std::string symbol = asm_symbol_name(module.target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, symbol, true);
  const bool is_i686 = module.target_triple.find("i686") != std::string::npos;
  auto emit_param_move = [&](int index) {
    if (is_i686) {
      out << "  mov eax, DWORD PTR [esp + " << (4 * (index + 1)) << "]\n";
    } else if (index == 0) {
      out << "  mov eax, edi\n";
    } else {
      out << "  mov eax, esi\n";
    }
  };
  auto emit_param_add = [&](int index) {
    if (is_i686) {
      out << "  add eax, DWORD PTR [esp + " << (4 * (index + 1)) << "]\n";
    } else if (index == 0) {
      out << "  add eax, edi\n";
    } else {
      out << "  add eax, esi\n";
    }
  };

  if (slice.uses_first_param) {
    emit_param_move(0);
    if (slice.uses_second_param) {
      emit_param_add(1);
    }
  } else if (slice.uses_second_param) {
    emit_param_move(1);
  } else {
    out << "  mov eax, " << slice.constant << "\n";
    out << "  ret\n";
    return out.str();
  }

  if (slice.constant > 0) {
    out << "  add eax, " << slice.constant << "\n";
  } else if (slice.constant < 0) {
    out << "  sub eax, " << -slice.constant << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_cast_return_asm(
    const c4c::backend::bir::Module& module,
    const MinimalCastReturnSlice& slice) {
  if (module.target_triple.find("i686") != std::string::npos) {
    throw_unsupported_direct_bir_module();
  }

  const std::string symbol = asm_symbol_name(module.target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, symbol, true);

  if (slice.opcode == c4c::backend::bir::CastOpcode::SExt &&
      slice.operand_type == c4c::backend::bir::TypeKind::I32 &&
      slice.result_type == c4c::backend::bir::TypeKind::I64) {
    out << "  movsxd rax, edi\n";
    out << "  ret\n";
    return out.str();
  }
  if (slice.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      slice.operand_type == c4c::backend::bir::TypeKind::I8 &&
      slice.result_type == c4c::backend::bir::TypeKind::I32) {
    out << "  movzx eax, dil\n";
    out << "  ret\n";
    return out.str();
  }
  if (slice.opcode == c4c::backend::bir::CastOpcode::Trunc &&
      slice.operand_type == c4c::backend::bir::TypeKind::I64 &&
      slice.result_type == c4c::backend::bir::TypeKind::I32) {
    out << "  mov eax, edi\n";
    out << "  ret\n";
    return out.str();
  }

  throw_unsupported_direct_bir_module();
}

std::string emit_minimal_direct_call_asm(const c4c::backend::BackendModule& module,
                                         const c4c::backend::ParsedBackendMinimalDirectCallModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "structured zero-argument direct-call slice without helper metadata");
  }

  if (slice.return_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.return_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "helper return immediates outside the minimal mov-supported range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.helper->signature.name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, " << slice.return_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_add_imm_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::ParsedBackendMinimalDirectCallAddImmModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "structured single-argument direct-call slice without helper metadata");
  }

  if (slice.call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.add_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.add_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "single-argument direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.helper->signature.name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, " << slice.add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  mov edi, " << slice.call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_arg_direct_call_asm(
    const c4c::backend::BackendModule& module,
    const MinimalTwoArgDirectCallSlice& slice) {
  if (slice.lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "two-argument direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, esi\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  mov edi, " << slice.lhs_call_arg_imm << "\n"
      << "  mov esi, " << slice.rhs_call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_arg_direct_call_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::ParsedBackendMinimalTwoArgDirectCallModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "structured two-argument direct-call slice without helper metadata");
  }

  if (slice.lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "two-argument direct-call immediates outside the minimal x86 slice range");
  }

  return emit_minimal_two_arg_direct_call_asm(
      module,
      MinimalTwoArgDirectCallSlice{
          slice.helper->signature.name,
          slice.lhs_call_arg_imm,
          slice.rhs_call_arg_imm,
      });
}

std::string emit_minimal_conditional_return_asm(
    std::string_view target_triple,
    const MinimalConditionalReturnSlice& slice) {
  auto emit_value_to_eax = [](std::ostringstream& out,
                              const MinimalConditionalReturnSlice::ValueSource& value) {
    switch (value.kind) {
      case MinimalConditionalReturnSlice::ValueKind::Immediate:
        out << "  mov eax, " << value.imm << "\n";
        return;
      case MinimalConditionalReturnSlice::ValueKind::Param0:
        out << "  mov eax, edi\n";
        return;
      case MinimalConditionalReturnSlice::ValueKind::Param1:
        out << "  mov eax, esi\n";
        return;
    }
  };
  auto cmp_rhs_operand = [](const MinimalConditionalReturnSlice::ValueSource& value) {
    switch (value.kind) {
      case MinimalConditionalReturnSlice::ValueKind::Immediate:
        return std::to_string(value.imm);
      case MinimalConditionalReturnSlice::ValueKind::Param0:
        return std::string("edi");
      case MinimalConditionalReturnSlice::ValueKind::Param1:
        return std::string("esi");
    }
    return std::string("0");
  };

  const std::string symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  const char* fail_branch = nullptr;
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "jge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "jg"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "jle"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "jl"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "jne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "je"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "jae"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "ja"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "jbe"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "jb"; break;
    default:
      throw c4c::backend::LirAdapterError(
          c4c::backend::LirAdapterErrorKind::Unsupported,
          "conditional-return predicates outside the current compare-and-branch x86 slice");
  }
  emit_value_to_eax(out, slice.lhs);
  out << "  cmp eax, " << cmp_rhs_operand(slice.rhs) << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  emit_value_to_eax(out, slice.true_value);
  out << "  ret\n";
  out << ".L" << slice.false_label << ":\n";
  emit_value_to_eax(out, slice.false_value);
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalReturnSlice& slice) {
  return emit_minimal_conditional_return_asm(module.target_triple, slice);
}

std::string emit_minimal_conditional_affine_i8_return_asm(
    std::string_view target_triple,
    const MinimalConditionalAffineI8ReturnSlice& slice) {
  auto emit_compare_source_to_al = [](std::ostringstream& out,
                                      const MinimalConditionalAffineI8ReturnSlice::ValueExpr& value) {
    switch (value.kind) {
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Immediate:
        out << "  mov al, " << value.imm << "\n";
        return;
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param0:
        out << "  mov al, dil\n";
        return;
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param1:
        out << "  mov al, sil\n";
        return;
    }
  };
  auto cmp_rhs_operand = [](const MinimalConditionalAffineI8ReturnSlice::ValueExpr& value) {
    switch (value.kind) {
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Immediate:
        return std::to_string(value.imm);
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param0:
        return std::string("dil");
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param1:
        return std::string("sil");
    }
    return std::string("0");
  };
  auto emit_value_expr_to_eax =
      [](std::ostringstream& out, const MinimalConditionalAffineI8ReturnSlice::ValueExpr& value) {
        switch (value.kind) {
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Immediate:
            out << "  mov eax, " << value.imm << "\n";
            break;
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param0:
            out << "  movzx eax, dil\n";
            break;
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param1:
            out << "  movzx eax, sil\n";
            break;
        }
        for (const auto& step : value.steps) {
          out << "  "
              << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
              << " eax, " << step.imm << "\n";
        }
      };

  const std::string symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";

  const char* fail_branch = nullptr;
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "jge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "jg"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "jle"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "jl"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "jne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "je"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "jae"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "ja"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "jbe"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "jb"; break;
    default:
      throw c4c::backend::LirAdapterError(
          c4c::backend::LirAdapterErrorKind::Unsupported,
          "conditional-i8-affine-return predicates outside the current compare-and-branch x86 slice");
  }

  emit_compare_source_to_al(out, slice.lhs);
  out << "  cmp al, " << cmp_rhs_operand(slice.rhs) << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  emit_value_expr_to_eax(out, slice.true_value);
  out << "  jmp .L" << slice.join_label << "\n";
  out << ".L" << slice.false_label << ":\n";
  emit_value_expr_to_eax(out, slice.false_value);
  out << ".L" << slice.join_label << ":\n";
  for (const auto& step : slice.post_steps) {
    out << "  " << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
        << " eax, " << step.imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_affine_i8_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalAffineI8ReturnSlice& slice) {
  return emit_minimal_conditional_affine_i8_return_asm(module.target_triple, slice);
}

std::string emit_minimal_conditional_affine_i32_return_asm(
    std::string_view target_triple,
    const MinimalConditionalAffineI32ReturnSlice& slice) {
  auto emit_compare_source_to_eax =
      [](std::ostringstream& out, const MinimalConditionalAffineI32ReturnSlice::ValueExpr& value) {
        switch (value.kind) {
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Immediate:
            out << "  mov eax, " << value.imm << "\n";
            return;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param0:
            out << "  mov eax, edi\n";
            return;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param1:
            out << "  mov eax, esi\n";
            return;
        }
      };
  auto cmp_rhs_operand = [](const MinimalConditionalAffineI32ReturnSlice::ValueExpr& value) {
    switch (value.kind) {
      case MinimalConditionalAffineI32ReturnSlice::ValueKind::Immediate:
        return std::to_string(value.imm);
      case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param0:
        return std::string("edi");
      case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param1:
        return std::string("esi");
    }
    return std::string("0");
  };
  auto emit_value_expr_to_eax =
      [](std::ostringstream& out, const MinimalConditionalAffineI32ReturnSlice::ValueExpr& value) {
        switch (value.kind) {
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Immediate:
            out << "  mov eax, " << value.imm << "\n";
            break;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param0:
            out << "  mov eax, edi\n";
            break;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param1:
            out << "  mov eax, esi\n";
            break;
        }
        for (const auto& step : value.steps) {
          out << "  "
              << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
              << " eax, " << step.imm << "\n";
        }
      };

  const std::string symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";

  const char* fail_branch = nullptr;
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "jge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "jg"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "jle"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "jl"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "jne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "je"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "jae"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "ja"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "jbe"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "jb"; break;
    default:
      throw c4c::backend::LirAdapterError(
          c4c::backend::LirAdapterErrorKind::Unsupported,
          "conditional-i32-affine-return predicates outside the current compare-and-branch x86 slice");
  }

  emit_compare_source_to_eax(out, slice.lhs);
  out << "  cmp eax, " << cmp_rhs_operand(slice.rhs) << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  emit_value_expr_to_eax(out, slice.true_value);
  out << "  jmp .L" << slice.join_label << "\n";
  out << ".L" << slice.false_label << ":\n";
  emit_value_expr_to_eax(out, slice.false_value);
  out << ".L" << slice.join_label << ":\n";
  for (const auto& step : slice.post_steps) {
    out << "  " << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
        << " eax, " << step.imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_affine_i32_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalAffineI32ReturnSlice& slice) {
  return emit_minimal_conditional_affine_i32_return_asm(module.target_triple, slice);
}

std::string emit_minimal_countdown_loop_asm(const c4c::backend::BackendModule& module,
                                            const MinimalCountdownLoopSlice& slice) {
  const std::string symbol = asm_symbol_name(module, "main");
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  mov eax, " << slice.initial_imm << "\n";
  out << ".L" << slice.loop_label << ":\n";
  out << "  cmp eax, 0\n";
  out << "  je .L" << slice.exit_label << "\n";
  out << ".L" << slice.body_label << ":\n";
  out << "  sub eax, 1\n";
  out << "  jmp .L" << slice.loop_label << "\n";
  out << ".L" << slice.exit_label << ":\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_phi_join_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalPhiJoinSlice& slice) {
  const std::string symbol = asm_symbol_name(module, "main");
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";

  const char* fail_branch = nullptr;
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "jge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "jg"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "jle"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "jl"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "jne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "je"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "jae"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "ja"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "jbe"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "jb"; break;
    default:
      throw c4c::backend::LirAdapterError(
          c4c::backend::LirAdapterErrorKind::Unsupported,
          "conditional-phi-join predicates outside the current compare-and-branch x86 slice");
  }

  out << "  mov eax, " << slice.lhs_imm << "\n";
  out << "  cmp eax, " << slice.rhs_imm << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  out << "  mov eax, " << slice.true_value.base_imm << "\n";
  for (const auto& step : slice.true_value.steps) {
    out << "  "
        << (step.opcode == c4c::backend::BackendBinaryOpcode::Sub ? "sub" : "add")
        << " eax, " << step.imm << "\n";
  }
  out << "  jmp .L" << slice.join_label << "\n";
  out << ".L" << slice.false_label << ":\n";
  out << "  mov eax, " << slice.false_value.base_imm << "\n";
  for (const auto& step : slice.false_value.steps) {
    out << "  "
        << (step.opcode == c4c::backend::BackendBinaryOpcode::Sub ? "sub" : "add")
        << " eax, " << step.imm << "\n";
  }
  out << ".L" << slice.join_label << ":\n";
  if (slice.join_add_imm.has_value()) {
    out << "  add eax, " << *slice.join_add_imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_local_array_asm(const c4c::backend::BackendModule& module,
                                         const MinimalLocalArraySlice& slice) {
  if (slice.store0_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store0_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.store1_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store1_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "local-array store immediates outside the minimal mov-supported range");
  }

  const std::string symbol = asm_symbol_name(module, "main");
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 16\n"
      << "  lea rcx, [rbp - 8]\n"
      << "  mov dword ptr [rcx], " << slice.store0_imm << "\n"
      << "  mov dword ptr [rcx + 4], " << slice.store1_imm << "\n"
      << "  mov eax, dword ptr [rcx]\n"
      << "  add eax, dword ptr [rcx + 4]\n"
      << "  mov rsp, rbp\n"
      << "  pop rbp\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_global_array_load_asm(
    const c4c::backend::BackendModule& module,
    const MinimalExternGlobalArrayLoadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax + " << slice.byte_offset << "]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_scalar_global_load_asm(
    const c4c::backend::BackendModule& module,
    const MinimalExternScalarGlobalLoadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_decl_call_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::ParsedBackendMinimalDeclaredDirectCallModuleView& slice) {
  static constexpr const char* kArgIntRegs[6] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
  static constexpr const char* kArgPtrRegs[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

  const std::string helper_symbol = asm_symbol_name(module, slice.parsed_call.symbol_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  if (slice.args.size() > 6) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "extern declaration call argument count exceeds minimal x86 register budget");
  }

  std::vector<std::string> emitted_string_constant_names;
  for (const auto& arg : slice.args) {
    if (arg.kind != MinimalExternCallArgSlice::Kind::Ptr || arg.operand.empty() ||
        arg.operand.front() != '@') {
      continue;
    }
    const auto* string_constant = find_string_constant(module, arg.operand.substr(1));
    if (string_constant == nullptr) {
      continue;
    }
    const auto& name = string_constant->name;
    bool duplicate = false;
    for (const auto& emitted : emitted_string_constant_names) {
      if (emitted == name) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      emitted_string_constant_names.push_back(name);
    }
  }

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  if (!emitted_string_constant_names.empty()) {
    out << ".section .rodata\n";
    for (const auto& name : emitted_string_constant_names) {
      const auto* string_constant = find_string_constant(module, name);
      if (string_constant == nullptr) {
        continue;
      }
      const std::string label = asm_private_data_label(module, "@" + name);
      out << label << ":\n"
          << "  .asciz \"" << escape_asm_string(string_constant->raw_bytes) << "\"\n";
    }
  }
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  for (std::size_t index = 0; index < slice.args.size(); ++index) {
    const auto& arg = slice.args[index];
    switch (arg.kind) {
      case MinimalExternCallArgSlice::Kind::I32Imm:
        out << "  mov " << kArgIntRegs[index] << ", " << arg.imm << "\n";
        break;
      case MinimalExternCallArgSlice::Kind::I64Imm:
        out << "  mov " << kArgPtrRegs[index] << ", " << arg.imm << "\n";
        break;
      case MinimalExternCallArgSlice::Kind::Ptr: {
        const auto* string_constant = find_string_constant(module, arg.operand.substr(1));
        if (string_constant == nullptr) {
          const std::string symbol = asm_symbol_name(module, arg.operand.substr(1));
          out << "  lea " << kArgPtrRegs[index] << ", " << symbol << "[rip]\n";
          break;
        }
        const auto label = asm_private_data_label(module, arg.operand);
        out << "  lea " << kArgPtrRegs[index] << ", " << label << "[rip]\n";
      } break;
    }
  }
  if (slice.callee != nullptr && slice.callee->signature.is_vararg) {
    out << "  mov al, 0\n";
  }
  out << "  call " << helper_symbol << "\n";
  if (!slice.return_call_result) {
    out << "  mov eax, " << slice.return_imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_global_char_pointer_diff_asm(
    const c4c::backend::BackendModule& module,
    const MinimalGlobalCharPointerDiffSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".bss\n"
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n"
      << "  .zero " << slice.global_size << "\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  lea rcx, [rax + " << slice.byte_offset << "]\n"
      << "  sub rcx, rax\n"
      << "  cmp rcx, " << slice.byte_offset << "\n"
      << "  sete al\n"
      << "  movzx eax, al\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_global_int_pointer_diff_asm(
    const c4c::backend::BackendModule& module,
    const MinimalGlobalIntPointerDiffSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".bss\n"
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n"
      << "  .zero " << (slice.global_size * 4) << "\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  lea rcx, [rax + " << slice.byte_offset << "]\n"
      << "  sub rcx, rax\n"
      << "  sar rcx, " << slice.element_shift << "\n"
      << "  cmp rcx, 1\n"
      << "  sete al\n"
      << "  movzx eax, al\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_load_asm(
    const c4c::backend::BackendModule& module,
    const MinimalScalarGlobalLoadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".data\n"
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n"
      << "  .long " << slice.init_imm << "\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_store_reload_asm(
    const c4c::backend::BackendModule& module,
    const MinimalScalarGlobalStoreReloadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".data\n"
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n"
      << "  .long " << slice.init_imm << "\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov dword ptr [rax], " << slice.store_imm << "\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_string_literal_char_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalStringLiteralCharSlice& slice) {
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const std::string string_label = asm_private_data_label(backend_module, slice.pool_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".section .rodata\n";
  out << string_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.raw_bytes) << "\"\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << string_label << "[rip]\n"
      << "  " << (slice.sign_extend ? "movsx" : "movzx")
      << " eax, byte ptr [rax + " << slice.byte_offset << "]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_call_crossing_direct_call_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::RegAllocIntegrationResult& regalloc,
    const MinimalCallCrossingDirectCallSlice& slice) {
  if (slice.source_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.source_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "call-crossing source immediates outside the minimal mov-supported range");
  }
  if (slice.helper_add_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.helper_add_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "call-crossing helper add immediates outside the minimal add-supported range");
  }

  const auto* source_reg =
      c4c::backend::stack_layout::find_assigned_reg(regalloc, slice.regalloc_source_value);
  if (source_reg == nullptr ||
      !c4c::backend::stack_layout::uses_callee_saved_reg(regalloc, *source_reg)) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "shared call-crossing regalloc state for the minimal x86 direct-call slice");
  }

  const char* reg64 = x86_reg64_name(*source_reg);
  const char* reg32 = x86_reg32_name(*source_reg);
  if (reg64 == nullptr || reg32 == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "minimal x86 direct-call slice received an unknown physical register");
  }

  const auto frame_size = aligned_frame_size(1);
  const auto helper_symbol = asm_symbol_name(module, slice.callee_name);
  const auto main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, " << slice.helper_add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  push rbp\n"
      << "  mov rbp, rsp\n";
  if (frame_size > 0) {
    out << "  sub rsp, " << frame_size << "\n"
        << "  mov qword ptr [rbp - " << frame_size << "], " << reg64 << "\n";
  }
  out << "  mov " << reg32 << ", " << slice.source_imm << "\n"
      << "  mov edi, " << reg32 << "\n"
      << "  call " << helper_symbol << "\n"
      << "  mov eax, eax\n"
      << "  add eax, " << reg32 << "\n";
  if (frame_size > 0) {
    out << "  mov " << reg64 << ", qword ptr [rbp - " << frame_size << "]\n";
  }
  out << "  mov rsp, rbp\n"
      << "  pop rbp\n"
      << "  ret\n";
  return out.str();
}

std::string remove_redundant_self_moves(std::string asm_text) {
  std::string optimized;
  optimized.reserve(asm_text.size());

  std::size_t start = 0;
  while (start < asm_text.size()) {
    const auto end = asm_text.find('\n', start);
    const auto line_end = end == std::string::npos ? asm_text.size() : end;
    std::string_view line(asm_text.data() + start, line_end - start);

    std::size_t trim = 0;
    while (trim < line.size() && (line[trim] == ' ' || line[trim] == '\t')) ++trim;
    std::string_view trimmed = line.substr(trim);

    bool drop_line = false;
    if (trimmed.size() >= 4 && trimmed.substr(0, 4) == "mov ") {
      const auto comma = trimmed.find(',');
      if (comma != std::string_view::npos) {
        auto lhs = trimmed.substr(4, comma - 4);
        auto rhs = trimmed.substr(comma + 1);
        while (!lhs.empty() && lhs.back() == ' ') lhs.remove_suffix(1);
        while (!rhs.empty() && rhs.front() == ' ') rhs.remove_prefix(1);
        drop_line = lhs == rhs;
      }
    }

    if (!drop_line) {
      optimized.append(line);
      if (end != std::string::npos) optimized.push_back('\n');
    }

    if (end == std::string::npos) break;
    start = end + 1;
  }

  return optimized;
}

}  // namespace

std::string emit_module(const c4c::backend::BackendModule& module,
                        const c4c::codegen::lir::LirModule* legacy_fallback) {
  std::string backend_ir_error;
  if (!c4c::backend::validate_backend_module(module, &backend_ir_error)) {
    return c4c::backend::print_backend_module(module);
  }
  try {
    if (const auto slice = parse_minimal_extern_scalar_global_load_slice(module);
        slice.has_value()) {
      return emit_minimal_extern_scalar_global_load_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_extern_global_array_load_slice(module);
        slice.has_value()) {
      return emit_minimal_extern_global_array_load_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_local_array_slice(module);
        slice.has_value()) {
      return emit_minimal_local_array_asm(module, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_declared_direct_call_module(module);
        slice.has_value()) {
      return emit_minimal_extern_decl_call_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_load_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_load_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_store_reload_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_global_char_pointer_diff_slice(module);
        slice.has_value()) {
      return emit_minimal_global_char_pointer_diff_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_diff_slice(module);
        slice.has_value()) {
      return emit_minimal_global_int_pointer_diff_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_string_literal_char_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_string_literal_char_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_conditional_return_slice(module);
        slice.has_value()) {
      return emit_minimal_conditional_return_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_conditional_phi_join_slice(module);
        slice.has_value()) {
      return emit_minimal_conditional_phi_join_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_countdown_loop_slice(module);
        slice.has_value()) {
      return emit_minimal_countdown_loop_asm(module, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_direct_call_module(module);
        slice.has_value()) {
      return emit_minimal_direct_call_asm(module, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_direct_call_add_imm_module(module);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(module, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_two_arg_direct_call_module(module);
        slice.has_value()) {
      return emit_minimal_two_arg_direct_call_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_call_crossing_direct_call_slice(module);
        slice.has_value()) {
      return remove_redundant_self_moves(emit_minimal_call_crossing_direct_call_asm(
          module, synthesize_shared_x86_call_crossing_regalloc(*slice), *slice));
    }
    if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
      return emit_minimal_return_asm(module, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(module); imm.has_value()) {
      return emit_minimal_return_asm(module, *imm);
    }
    if (const auto imm = parse_minimal_return_sub_imm(module); imm.has_value()) {
      return emit_minimal_return_asm(module, *imm);
    }
    if (const auto slice = parse_minimal_affine_return_slice(module);
        slice.has_value()) {
      return emit_minimal_affine_return_asm(module, *slice);
    }
  } catch (const c4c::backend::LirAdapterError&) {
  }

  if (legacy_fallback != nullptr) {
    return emit_module(*legacy_fallback);
  }
  return c4c::backend::print_backend_module(module);
}

std::string emit_module(const c4c::backend::bir::Module& module,
                        const c4c::codegen::lir::LirModule* legacy_fallback) {
  (void)legacy_fallback;
  if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
    return emit_minimal_return_asm(module, *imm);
  }
  if (const auto imm = parse_minimal_return_add_imm(module); imm.has_value()) {
    return emit_minimal_return_asm(module, *imm);
  }
  if (const auto imm = parse_minimal_return_sub_imm(module); imm.has_value()) {
    return emit_minimal_return_asm(module, *imm);
  }
  if (const auto slice = parse_minimal_affine_return_slice(module);
      slice.has_value()) {
    return emit_minimal_affine_return_asm(module, *slice);
  }
  if (const auto slice = parse_minimal_cast_return_slice(module);
      slice.has_value()) {
    return emit_minimal_cast_return_asm(module, *slice);
  }
  if (const auto slice = parse_minimal_conditional_return_slice(module);
      slice.has_value()) {
    return emit_minimal_conditional_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_conditional_affine_i8_return_slice(module);
      slice.has_value()) {
    return emit_minimal_conditional_affine_i8_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_conditional_affine_i32_return_slice(module);
      slice.has_value()) {
    return emit_minimal_conditional_affine_i32_return_asm(module.target_triple, *slice);
  }
  throw_unsupported_direct_bir_module();
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  try {
    if (const auto slice = parse_minimal_two_arg_direct_call_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_two_arg_direct_call_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_local_array_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_local_array_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_extern_global_array_load_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_extern_global_array_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_global_char_pointer_diff_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_global_char_pointer_diff_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_diff_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_global_int_pointer_diff_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_roundtrip_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_scalar_global_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_load_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_scalar_global_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_scalar_global_store_reload_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_string_literal_char_slice(module);
        slice.has_value()) {
      return emit_minimal_string_literal_char_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_conditional_return_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_conditional_return_asm(scaffold_module, *slice);
    }
    const auto adapted = c4c::backend::lower_lir_to_backend_module(module);
    if (const auto slice = c4c::backend::parse_backend_minimal_declared_direct_call_module(adapted);
        slice.has_value()) {
      return emit_minimal_extern_decl_call_asm(adapted, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_direct_call_module(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_asm(adapted, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_direct_call_add_imm_module(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(adapted, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_two_arg_direct_call_module(adapted);
        slice.has_value()) {
      return emit_minimal_two_arg_direct_call_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_call_crossing_direct_call_slice(adapted);
        slice.has_value()) {
      const auto* main_fn = find_lir_function(module, "main");
      if (main_fn == nullptr) {
        throw c4c::backend::LirAdapterError(
            c4c::backend::LirAdapterErrorKind::Unsupported,
            "main function for shared x86 call-crossing direct-call slice");
      }
      return remove_redundant_self_moves(emit_minimal_call_crossing_direct_call_asm(
          adapted, run_shared_x86_regalloc(*main_fn), *slice));
    }
    if (const auto slice = parse_minimal_countdown_loop_slice(adapted);
        slice.has_value()) {
      return emit_minimal_countdown_loop_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_conditional_phi_join_slice(adapted);
        slice.has_value()) {
      return emit_minimal_conditional_phi_join_asm(adapted, *slice);
    }
    if (const auto imm = parse_minimal_return_imm(adapted); imm.has_value()) {
      return emit_minimal_return_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(adapted); imm.has_value()) {
      return emit_minimal_return_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_sub_imm(adapted); imm.has_value()) {
      return emit_minimal_return_asm(adapted, *imm);
    }
  } catch (const c4c::backend::LirAdapterError&) {
  }

  return c4c::codegen::lir::print_llvm(module);
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(
      assembler::AssembleRequest{.asm_text = emit_module(module), .output_path = output_path});
}

}  // namespace c4c::backend::x86
