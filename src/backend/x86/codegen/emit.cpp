#include "x86_codegen.hpp"
#include "peephole/peephole.hpp"

#include "../../backend.hpp"
#include "../../bir.hpp"
#include "../../lowering/call_decode.hpp"
#include "../../lowering/lir_to_bir.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <charconv>
#include <cctype>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace c4c::backend::x86 {

namespace {

struct MinimalAffineReturnSlice {
  std::string function_name;
  std::size_t param_count = 0;
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

struct MinimalTwoFunctionImmediateReturnSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t helper_imm = 0;
  std::int64_t entry_imm = 0;
};

struct MinimalThreeFunctionImmediateReturnSlice {
  std::string first_name;
  std::string second_name;
  std::string entry_name;
  std::int64_t first_imm = 0;
  std::int64_t second_imm = 0;
  std::int64_t entry_imm = 0;
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

std::string decode_llvm_byte_string(std::string_view text) {
  std::string bytes;
  bytes.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '\\' && index + 2 < text.size()) {
      auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      const int hi = hex_val(text[index + 1]);
      const int lo = hex_val(text[index + 2]);
      if (hi >= 0 && lo >= 0) {
        bytes.push_back(static_cast<char>(hi * 16 + lo));
        index += 2;
        continue;
      }
    }
    bytes.push_back(text[index]);
  }
  return bytes;
}

struct AffineValue {
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

const c4c::backend::bir::BinaryInst* get_binary_inst(
    const c4c::backend::bir::Inst& inst) {
  return std::get_if<c4c::backend::bir::BinaryInst>(&inst);
}

bool is_minimal_single_function_asm_slice(const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1 || !module.globals.empty()) {
    return false;
  }
  const auto& function = module.functions.front();
  return function.params.empty() && function.blocks.size() == 1 &&
         function.blocks.front().label == "entry" &&
         function.return_type == c4c::backend::bir::TypeKind::I32;
}

std::optional<AffineValue> lower_affine_operand(
    const c4c::backend::bir::Value& operand,
    const std::vector<std::string_view>& param_names,
    const std::unordered_map<std::string, AffineValue>& values) {
  if (const auto imm = parse_i64(operand); imm.has_value()) {
    return AffineValue{false, false, *imm};
  }
  if (operand.kind != c4c::backend::bir::Value::Kind::Named ||
      operand.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (!param_names.empty() && operand.name == param_names[0]) {
    return AffineValue{true, false, 0};
  }
  if (param_names.size() > 1 && operand.name == param_names[1]) {
    return AffineValue{false, true, 0};
  }
  auto it = values.find(operand.name);
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<AffineValue> combine_affine_values(const AffineValue& lhs,
                                                 const AffineValue& rhs,
                                                 c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      if ((lhs.uses_first_param && rhs.uses_first_param) ||
          (lhs.uses_second_param && rhs.uses_second_param)) {
        return std::nullopt;
      }
      return AffineValue{lhs.uses_first_param || rhs.uses_first_param,
                         lhs.uses_second_param || rhs.uses_second_param,
                         lhs.constant + rhs.constant};
    case c4c::backend::bir::BinaryOpcode::Sub:
      if (rhs.uses_first_param || rhs.uses_second_param) {
        return std::nullopt;
      }
      return AffineValue{lhs.uses_first_param, lhs.uses_second_param,
                         lhs.constant - rhs.constant};
    default:
      return std::nullopt;
  }
}

std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }
  const auto& block = module.functions.front().blocks.front();
  if (!block.insts.empty() || !block.terminator.value.has_value()) {
    return std::nullopt;
  }
  return parse_i64(*block.terminator.value);
}

std::optional<std::int64_t> parse_minimal_constant_return_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (!function.params.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::int64_t> values;
  auto resolve_value = [&](const c4c::backend::bir::Value& value)
      -> std::optional<std::int64_t> {
    if (const auto imm = parse_i64(value); imm.has_value()) {
      return imm;
    }
    if (value.kind != c4c::backend::bir::Value::Kind::Named || value.name.empty() ||
        value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    const auto it = values.find(value.name);
    if (it == values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  const auto& block = function.blocks.front();
  for (const auto& inst : block.insts) {
    const auto* bin = get_binary_inst(inst);
    if (bin == nullptr || bin->result.kind != c4c::backend::bir::Value::Kind::Named ||
        bin->result.name.empty() || bin->result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto lhs = resolve_value(bin->lhs);
    const auto rhs = resolve_value(bin->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    std::optional<std::int64_t> result;
    switch (bin->opcode) {
      case c4c::backend::bir::BinaryOpcode::Add:
        result = *lhs + *rhs;
        break;
      case c4c::backend::bir::BinaryOpcode::Sub:
        result = *lhs - *rhs;
        break;
      case c4c::backend::bir::BinaryOpcode::Mul:
        result = *lhs * *rhs;
        break;
      case c4c::backend::bir::BinaryOpcode::SDiv:
        if (*rhs == 0) return std::nullopt;
        result = *lhs / *rhs;
        break;
      case c4c::backend::bir::BinaryOpcode::SRem:
        if (*rhs == 0) return std::nullopt;
        result = *lhs % *rhs;
        break;
      default:
        return std::nullopt;
    }

    values[bin->result.name] = *result;
  }

  if (!block.terminator.value.has_value()) {
    return std::nullopt;
  }
  return resolve_value(*block.terminator.value);
}

std::optional<MinimalTwoFunctionImmediateReturnSlice> parse_minimal_two_function_immediate_return_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.is_declaration || entry.is_declaration ||
      helper.return_type != TypeKind::I32 || entry.return_type != TypeKind::I32 ||
      !helper.params.empty() || !entry.params.empty() || helper.blocks.size() != 1 ||
      entry.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& helper_block = helper.blocks.front();
  const auto& entry_block = entry.blocks.front();
  if (helper_block.label != "entry" || entry_block.label != "entry" ||
      !helper_block.insts.empty() || !entry_block.insts.empty() ||
      !helper_block.terminator.value.has_value() || !entry_block.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto helper_imm = parse_i64(*helper_block.terminator.value);
  const auto entry_imm = parse_i64(*entry_block.terminator.value);
  if (!helper_imm.has_value() || !entry_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalTwoFunctionImmediateReturnSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
      .helper_imm = *helper_imm,
      .entry_imm = *entry_imm,
  };
}

std::optional<MinimalThreeFunctionImmediateReturnSlice> parse_minimal_three_function_immediate_return_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 3 || !module.globals.empty() ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& first = module.functions[0];
  const auto& second = module.functions[1];
  const auto& entry = module.functions[2];
  const auto parse_function_imm = [](const Function& function) -> std::optional<std::int64_t> {
    if (function.is_declaration || function.return_type != TypeKind::I32 ||
        !function.params.empty() || function.blocks.size() != 1) {
      return std::nullopt;
    }
    const auto& block = function.blocks.front();
    if (block.label != "entry" || !block.insts.empty() || !block.terminator.value.has_value()) {
      return std::nullopt;
    }
    return parse_i64(*block.terminator.value);
  };

  const auto first_imm = parse_function_imm(first);
  const auto second_imm = parse_function_imm(second);
  const auto entry_imm = parse_function_imm(entry);
  if (!first_imm.has_value() || !second_imm.has_value() || !entry_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalThreeFunctionImmediateReturnSlice{
      .first_name = first.name,
      .second_name = second.name,
      .entry_name = entry.name,
      .first_imm = *first_imm,
      .second_imm = *second_imm,
      .entry_imm = *entry_imm,
  };
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
    const auto* bin = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
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

  if (!block.terminator.value.has_value()) {
    return std::nullopt;
  }
  const auto lowered_return =
      lower_affine_operand(*block.terminator.value, param_names, values);
  if (!lowered_return.has_value()) {
    return std::nullopt;
  }

  return MinimalAffineReturnSlice{
      .function_name = function.name,
      .param_count = function.params.size(),
      .uses_first_param = lowered_return->uses_first_param,
      .uses_second_param = lowered_return->uses_second_param,
      .constant = lowered_return->constant,
  };
}

std::string asm_symbol_name(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
}

std::string asm_private_data_label(std::string_view target_triple, std::string_view pool_name) {
  std::string label(pool_name);
  if (!label.empty() && label.front() == '@') {
    label.erase(label.begin());
  }
  while (!label.empty() && label.front() == '.') {
    label.erase(label.begin());
  }

  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "L" + label;
  }
  return ".L." + label;
}

std::string escape_asm_string(std::string_view raw_bytes) {
  std::ostringstream out;
  for (unsigned char ch : raw_bytes) {
    switch (ch) {
      case '\\': out << "\\\\"; break;
      case '"': out << "\\\""; break;
      case '\n': out << "\\n"; break;
      case '\t': out << "\\t"; break;
      default:
        if (std::isprint(ch) != 0) {
          out << static_cast<char>(ch);
        } else {
          constexpr char kHex[] = "0123456789ABCDEF";
          out << '\\' << kHex[(ch >> 4) & 0xF] << kHex[ch & 0xF];
        }
        break;
    }
  }
  return out.str();
}

std::string emit_function_prelude(std::string_view target_triple,
                                  std::string_view symbol_name) {
  std::ostringstream out;
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @function\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

std::string emit_minimal_return_imm_asm(const c4c::backend::bir::Module& module,
                                        std::int64_t imm) {
  std::ostringstream out;
  const auto symbol =
      asm_symbol_name(module.target_triple, module.functions.front().name);
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(module.target_triple, symbol)
      << "  mov eax, " << imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_function_immediate_return_asm(
    std::string_view target_triple,
    const MinimalTwoFunctionImmediateReturnSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, " << slice.helper_imm << "\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov eax, " << slice.entry_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_three_function_immediate_return_asm(
    std::string_view target_triple,
    const MinimalThreeFunctionImmediateReturnSlice& slice) {
  const auto first_symbol = asm_symbol_name(target_triple, slice.first_name);
  const auto second_symbol = asm_symbol_name(target_triple, slice.second_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, first_symbol)
      << "  mov eax, " << slice.first_imm << "\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, second_symbol)
      << "  mov eax, " << slice.second_imm << "\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov eax, " << slice.entry_imm << "\n"
      << "  ret\n";
  return out.str();
}

bool target_uses_x86_64_sysv_regs(std::string_view target_triple) {
  return target_triple.find("x86_64") != std::string::npos ||
         target_triple.find("amd64") != std::string::npos;
}

std::string emit_minimal_affine_return_asm(std::string_view target_triple,
                                           const MinimalAffineReturnSlice& slice) {
  if (slice.constant < std::numeric_limits<std::int32_t>::min() ||
      slice.constant > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }
  if ((slice.uses_first_param || slice.uses_second_param) &&
      !target_uses_x86_64_sysv_regs(target_triple)) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol);

  if (!slice.uses_first_param && !slice.uses_second_param) {
    out << "  mov eax, " << slice.constant << "\n"
        << "  ret\n";
    return out.str();
  }

  if (slice.uses_first_param) {
    out << "  mov eax, edi\n";
    if (slice.uses_second_param) {
      out << "  add eax, esi\n";
    }
  } else if (slice.uses_second_param) {
    out << "  mov eax, esi\n";
  }

  if (slice.constant > 0) {
    out << "  add eax, " << slice.constant << "\n";
  } else if (slice.constant < 0) {
    out << "  sub eax, " << -slice.constant << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_global_symbol_prelude(std::string_view target_triple,
                                       std::string_view symbol_name,
                                       std::size_t align_bytes,
                                       bool is_zero_init) {
  std::ostringstream out;
  out << (is_zero_init ? ".bss\n" : ".data\n")
      << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @object\n";
  }
  if (align_bytes > 1) {
    out << ".p2align " << (align_bytes == 2 ? 1 : 2) << "\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

[[noreturn]] void throw_unsupported_direct_bir_module() {
  throw std::invalid_argument(
      "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
}

[[noreturn]] void throw_x86_rewrite_in_progress() {
  throw std::invalid_argument(
      "x86 backend emitter rewrite in progress: move ownership into the translated sibling codegen translation units instead of adding emit.cpp-local matchers");
}

}  // namespace

std::optional<std::string> try_emit_module(const c4c::backend::bir::Module& module) {
  if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
    return emit_minimal_return_imm_asm(module, *imm);
  }
  if (const auto imm = parse_minimal_constant_return_imm(module); imm.has_value()) {
    return emit_minimal_return_imm_asm(module, *imm);
  }
  if (const auto slice = parse_minimal_two_function_immediate_return_slice(module);
      slice.has_value()) {
    return emit_minimal_two_function_immediate_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_three_function_immediate_return_slice(module);
      slice.has_value()) {
    return emit_minimal_three_function_immediate_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_affine_return_slice(module); slice.has_value()) {
    return emit_minimal_affine_return_asm(module.target_triple, *slice);
  }
  if (const auto asm_text = try_emit_direct_bir_helper_module(module); asm_text.has_value()) {
    return asm_text;
  }
  return std::nullopt;
}

std::string emit_module(const c4c::backend::bir::Module& module) {
  if (const auto asm_text = try_emit_module(module); asm_text.has_value()) {
    return c4c::backend::x86::codegen::peephole::peephole_optimize(*asm_text);
  }
  throw_unsupported_direct_bir_module();
}

std::optional<std::string> try_emit_prepared_lir_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto asm_text = try_emit_minimal_constant_branch_return_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_local_temp_module(module); asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_direct_prepared_lir_helper_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  return std::nullopt;
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  if (const auto lowered = c4c::backend::try_lower_to_bir(module); lowered.has_value()) {
    return emit_module(*lowered);
  }
  if (const auto asm_text = try_emit_prepared_lir_module(module); asm_text.has_value()) {
    return c4c::backend::x86::codegen::peephole::peephole_optimize(*asm_text);
  }
  throw_x86_rewrite_in_progress();
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(assembler::AssembleRequest{
      .asm_text = emit_module(module),
      .output_path = output_path,
  });
}

}  // namespace c4c::backend::x86
