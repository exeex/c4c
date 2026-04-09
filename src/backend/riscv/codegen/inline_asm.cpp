#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::codegen {

// Self-contained translation of src/backend/riscv/codegen/inline_asm.rs.
// The broader RiscvCodegen / CodegenState surface is not shared yet, so this
// file only carries the local helper logic that can stand alone.

struct RvConstraintKind {
  enum class Tag {
    GpReg,
    FpReg,
    Memory,
    Address,
    Immediate,
    ZeroOrReg,
    Specific,
    Tied,
  };

  Tag tag = Tag::GpReg;
  std::string specific;
  std::size_t tied = 0;
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

bool is_ascii_digits(std::string_view text) {
  return !text.empty() &&
         std::all_of(text.begin(), text.end(), [](unsigned char ch) {
           return std::isdigit(ch) != 0;
         });
}

std::size_t parse_decimal(std::string_view text) {
  std::size_t value = 0;
  for (char ch : text) {
    value = value * 10 + static_cast<std::size_t>(ch - '0');
  }
  return value;
}

}  // namespace

RvConstraintKind classify_rv_constraint(std::string_view constraint) {
  const auto c = strip_constraint_prefix(constraint);
  if (is_ascii_digits(c)) {
    return {RvConstraintKind::Tag::Tied, {}, parse_decimal(c)};
  }

  if (c == "m") return {RvConstraintKind::Tag::Memory};
  if (c == "A") return {RvConstraintKind::Tag::Address};
  if (c == "f") return {RvConstraintKind::Tag::FpReg};
  if (c == "I" || c == "i" || c == "n") return {RvConstraintKind::Tag::Immediate};
  if (c == "J" || c == "rJ") return {RvConstraintKind::Tag::ZeroOrReg};

  if (c == "a0" || c == "a1" || c == "a2" || c == "a3" || c == "a4" || c == "a5" ||
      c == "a6" || c == "a7" || c == "ra" || c == "t0" || c == "t1" || c == "t2" ||
      c.starts_with("ft") || c.starts_with("fa") || c.starts_with("fs")) {
    return {RvConstraintKind::Tag::Specific, std::string(c)};
  }

  return {RvConstraintKind::Tag::GpReg};
}

std::string format_operand(std::size_t idx,
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

  switch (op_kinds[idx].tag) {
    case RvConstraintKind::Tag::Memory:
      if (idx < op_mem_addrs.size() && !op_mem_addrs[idx].empty()) {
        return op_mem_addrs[idx];
      }
      if (idx < op_mem_offsets.size()) {
        return std::to_string(op_mem_offsets[idx]) + "(s0)";
      }
      return "0(s0)";
    case RvConstraintKind::Tag::Address:
      return use_addr_format ? "(" + op_regs[idx] + ")" : op_regs[idx];
    case RvConstraintKind::Tag::Immediate:
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
  for (std::size_t i = 0; i < line.size(); ++i) {
    if (line[i] != '%' || i + 1 >= line.size()) {
      result.push_back(line[i]);
      continue;
    }

    ++i;
    if (line[i] == '%') {
      result.push_back('%');
      continue;
    }

    if (line[i] == 'z' && i + 1 < line.size()) {
      ++i;
      if (line[i] == '[') {
        ++i;
        const std::size_t start = i;
        while (i < line.size() && line[i] != ']') {
          ++i;
        }
        const std::string name = line.substr(start, i - start);
        if (i < line.size()) {
          ++i;
        }
        bool found = false;
        for (std::size_t idx = 0; idx < op_names.size(); ++idx) {
          if (op_names[idx].has_value() && *op_names[idx] == name) {
            if (idx < op_imm_values.size() && op_imm_values[idx].has_value() &&
                *op_imm_values[idx] == 0) {
              result += "zero";
            } else if (idx < op_regs.size()) {
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
        while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i])) != 0) {
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

    if (line[i] == 'l' && i + 1 < line.size() && line[i + 1] == 'o') {
      result += "%lo";
      ++i;
      continue;
    }
    if (line[i] == 'h' && i + 1 < line.size() && line[i + 1] == 'i') {
      result += "%hi";
      ++i;
      continue;
    }

    if (line[i] == '[') {
      ++i;
      const std::size_t start = i;
      while (i < line.size() && line[i] != ']') {
        ++i;
      }
      const std::string name = line.substr(start, i - start);
      if (i < line.size()) {
        ++i;
      }

      bool found = false;
      for (std::size_t idx = 0; idx < op_names.size(); ++idx) {
        if (op_names[idx].has_value() && *op_names[idx] == name) {
          result += format_operand(idx, op_regs, op_kinds, op_mem_offsets, op_mem_addrs,
                                   op_imm_values, op_imm_symbols, true);
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
      while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i])) != 0) {
        num = num * 10 + static_cast<std::size_t>(line[i] - '0');
        ++i;
      }
      const std::size_t internal_idx =
          num < gcc_to_internal.size() ? gcc_to_internal[num] : num;
      if (internal_idx < op_regs.size()) {
        result += format_operand(internal_idx, op_regs, op_kinds, op_mem_offsets,
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

}  // namespace c4c::backend::riscv::codegen
