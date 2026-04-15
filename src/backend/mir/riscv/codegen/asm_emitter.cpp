#include "riscv_codegen.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::codegen {

// Concrete translation of the helper logic that is local to
// src/backend/riscv/codegen/asm_emitter.rs and inline_asm.rs.
//
// The remaining methods in the Rust impl block are tied to the broader
// RiscvCodegen / InlineAsmEmitter surface, which is not yet shared in C++.
// Those are intentionally left for the other backend slices.

struct RiscvAsmOperandView {
  std::string reg;
  std::string name;
  std::string mem_addr;
  std::vector<std::string> comments_only;
  std::optional<long long> imm_value;
  std::optional<std::string> imm_symbol;
  long long mem_offset = 0;
  RvConstraint constraint;
};

namespace {

std::string_view strip_constraint_prefix(std::string_view constraint) {
  while (!constraint.empty()) {
    const char ch = constraint.front();
    if (ch == '=' || ch == '+' || ch == '&' || ch == '%') {
      constraint.remove_prefix(1);
      continue;
    }
    break;
  }
  return constraint;
}

bool has_prefix(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool is_all_digits(std::string_view text) {
  return !text.empty() &&
         std::all_of(text.begin(), text.end(), [](unsigned char ch) {
           return std::isdigit(ch) != 0;
         });
}

std::string append_uint(std::string out, std::size_t value) {
  out += std::to_string(value);
  return out;
}

std::string substitute_named_or_positional_operand(
    std::size_t idx,
    const std::vector<std::string>& op_regs,
    const std::vector<RvConstraintKind>& op_kinds,
    const std::vector<long long>& op_mem_offsets,
    const std::vector<std::string>& op_mem_addrs,
    const std::vector<std::optional<long long>>& op_imm_values,
    const std::vector<std::optional<std::string>>& op_imm_symbols,
    bool use_addr_format) {
  if (idx >= op_kinds.size()) {
    return {};
  }

  switch (op_kinds[idx]) {
    case RvConstraintKind::Memory:
      if (idx < op_mem_addrs.size() && !op_mem_addrs[idx].empty()) {
        return op_mem_addrs[idx];
      }
      return std::to_string(op_mem_offsets[idx]) + "(s0)";
    case RvConstraintKind::Address:
      return use_addr_format ? "(" + op_regs[idx] + ")" : op_regs[idx];
    case RvConstraintKind::Immediate:
      if (idx < op_imm_symbols.size() && op_imm_symbols[idx].has_value()) {
        return *op_imm_symbols[idx];
      }
      if (idx < op_imm_values.size() && op_imm_values[idx].has_value()) {
        return std::to_string(*op_imm_values[idx]);
      }
      return "0";
    default:
      return op_regs[idx];
  }
}

}  // namespace

RvConstraint classify_rv_constraint(std::string_view constraint) {
  const auto c = strip_constraint_prefix(constraint);
  if (is_all_digits(c)) {
    return {RvConstraintKind::Tied, {}, static_cast<std::size_t>(std::stoul(std::string(c)))};
  }

  if (c == "m") return {RvConstraintKind::Memory};
  if (c == "A") return {RvConstraintKind::Address};
  if (c == "f") return {RvConstraintKind::FpReg};
  if (c == "I" || c == "i" || c == "n") return {RvConstraintKind::Immediate};
  if (c == "J" || c == "rJ") return {RvConstraintKind::ZeroOrReg};

  if (c == "a0" || c == "a1" || c == "a2" || c == "a3" || c == "a4" || c == "a5" ||
      c == "a6" || c == "a7" || c == "ra" || c == "t0" || c == "t1" || c == "t2" ||
      has_prefix(c, "ft") || has_prefix(c, "fa") || has_prefix(c, "fs")) {
    return {RvConstraintKind::Specific, std::string(c)};
  }

  return {RvConstraintKind::GpReg};
}

std::string format_riscv_operand(std::size_t idx,
                                 const std::vector<std::string>& op_regs,
                                 const std::vector<RvConstraintKind>& op_kinds,
                                 const std::vector<long long>& op_mem_offsets,
                                 const std::vector<std::string>& op_mem_addrs,
                                 const std::vector<std::optional<long long>>& op_imm_values,
                                 const std::vector<std::optional<std::string>>& op_imm_symbols,
                                 bool use_addr_format) {
  return substitute_named_or_positional_operand(idx, op_regs, op_kinds, op_mem_offsets,
                                                op_mem_addrs, op_imm_values, op_imm_symbols,
                                                use_addr_format);
}

std::string substitute_riscv_asm_operands(
    const std::string& line,
    const std::vector<std::string>& op_regs,
    const std::vector<std::optional<std::string>>& op_names,
    const std::vector<RvConstraintKind>& op_kinds,
    const std::vector<long long>& op_mem_offsets,
    const std::vector<std::string>& op_mem_addrs,
    const std::vector<std::optional<long long>>& op_imm_values,
    const std::vector<std::optional<std::string>>& op_imm_symbols,
    const std::vector<std::size_t>& gcc_to_internal) {
  std::string result;
  const std::size_t len = line.size();
  for (std::size_t i = 0; i < len; ++i) {
    if (line[i] != '%' || i + 1 >= len) {
      result.push_back(line[i]);
      continue;
    }

    ++i;
    if (line[i] == '%') {
      result.push_back('%');
      continue;
    }

    if (line[i] == 'z' && i + 1 < len) {
      ++i;
      if (line[i] == '[') {
        ++i;
        const std::size_t start = i;
        while (i < len && line[i] != ']') {
          ++i;
        }
        const std::string name = line.substr(start, i - start);
        if (i < len) {
          ++i;
        }
        bool found = false;
        for (std::size_t idx = 0; idx < op_names.size(); ++idx) {
          if (op_names[idx].has_value() && *op_names[idx] == name) {
            if (idx < op_imm_values.size() && op_imm_values[idx].has_value() &&
                *op_imm_values[idx] == 0) {
              result += "zero";
            } else {
              result += op_regs[idx];
            }
            found = true;
            break;
          }
        }
        if (!found) {
          result += "%z[";
          result += name;
          result.push_back(']');
        }
        --i;
        continue;
      }

      if (std::isdigit(static_cast<unsigned char>(line[i])) != 0) {
        std::size_t num = 0;
        while (i < len && std::isdigit(static_cast<unsigned char>(line[i])) != 0) {
          num = num * 10 + static_cast<std::size_t>(line[i] - '0');
          ++i;
        }
        const std::size_t internal_idx =
            num < gcc_to_internal.size() ? gcc_to_internal[num] : num;
        if (internal_idx < op_regs.size()) {
          if (op_regs[internal_idx] == "zero") {
            result += "zero";
          } else if (internal_idx < op_imm_values.size() &&
                     op_imm_values[internal_idx].has_value() &&
                     *op_imm_values[internal_idx] == 0) {
            result += "zero";
          } else {
            result += op_regs[internal_idx];
          }
        } else {
          result += "%z";
          result += std::to_string(num);
        }
        --i;
        continue;
      }

      result += "%z";
      continue;
    }

    if (line[i] == 'l' && i + 1 < len && line[i + 1] == 'o') {
      result += "%lo";
      ++i;
      continue;
    }
    if (line[i] == 'h' && i + 1 < len && line[i + 1] == 'i') {
      result += "%hi";
      ++i;
      continue;
    }

    if (line[i] == '[') {
      ++i;
      const std::size_t start = i;
      while (i < len && line[i] != ']') {
        ++i;
      }
      const std::string name = line.substr(start, i - start);
      if (i < len) {
        ++i;
      }

      bool found = false;
      for (std::size_t idx = 0; idx < op_names.size(); ++idx) {
        if (op_names[idx].has_value() && *op_names[idx] == name) {
          result += format_riscv_operand(idx, op_regs, op_kinds, op_mem_offsets,
                                         op_mem_addrs, op_imm_values, op_imm_symbols, true);
          found = true;
          break;
        }
      }
      if (!found) {
        result += "%[";
        result += name;
        result.push_back(']');
      }
      --i;
      continue;
    }

    if (std::isdigit(static_cast<unsigned char>(line[i])) != 0) {
      std::size_t num = 0;
      while (i < len && std::isdigit(static_cast<unsigned char>(line[i])) != 0) {
        num = num * 10 + static_cast<std::size_t>(line[i] - '0');
        ++i;
      }
      const std::size_t internal_idx =
          num < gcc_to_internal.size() ? gcc_to_internal[num] : num;
      if (internal_idx < op_regs.size()) {
        result += format_riscv_operand(internal_idx, op_regs, op_kinds, op_mem_offsets,
                                       op_mem_addrs, op_imm_values, op_imm_symbols, true);
      } else {
        result += '%';
        result += std::to_string(num);
      }
      --i;
      continue;
    }

    result.push_back('%');
    result.push_back(line[i]);
  }
  return result;
}

bool constant_fits_riscv_immediate(std::string_view constraint, long long value) {
  const auto stripped = strip_constraint_prefix(constraint);
  if (stripped.find('i') != std::string_view::npos ||
      stripped.find('n') != std::string_view::npos) {
    return true;
  }

  for (char ch : stripped) {
    switch (ch) {
      case 'I':
        if (value >= -2048 && value <= 2047) {
          return true;
        }
        break;
      case 'K':
        if (value >= 0 && value <= 31) {
          return true;
        }
        break;
      default:
        break;
    }
  }
  return false;
}

std::string find_gp_scratch_for_output(std::string_view current_output_reg,
                                       const std::vector<std::string>& all_output_regs) {
  static constexpr std::string_view kCandidates[] = {
      "t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0", "a1",
  };

  for (std::string_view candidate : kCandidates) {
    const bool in_outputs =
        std::find(all_output_regs.begin(), all_output_regs.end(), candidate) !=
        all_output_regs.end();
    if (!in_outputs && candidate != current_output_reg) {
      return std::string(candidate);
    }
  }

  return current_output_reg != "t0" ? "t0" : "t1";
}

// The Rust impl for InlineAsmEmitter<RiscvCodegen> maps to the existing
// backend state machine. That translation needs shared C++ declarations for
// RiscvCodegen, CodegenState, AsmOperand, Operand, and BlockId before it can
// be emitted as method definitions in this tree.

}  // namespace c4c::backend::riscv::codegen
