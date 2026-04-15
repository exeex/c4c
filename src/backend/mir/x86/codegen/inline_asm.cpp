#include "x86_codegen.hpp"

#include <cctype>
#include <limits>
#include <string_view>

namespace c4c::backend::x86 {

namespace {

std::string resolve_dialect_alternatives(std::string_view line) {
  if (line.find('{') == std::string_view::npos) {
    return std::string(line);
  }

  std::string result;
  result.reserve(line.size());
  for (std::size_t i = 0; i < line.size();) {
    if (line[i] != '{') {
      result.push_back(line[i]);
      ++i;
      continue;
    }

    ++i;
    while (i < line.size() && line[i] != '|' && line[i] != '}') {
      result.push_back(line[i]);
      ++i;
    }
    if (i < line.size() && line[i] == '|') {
      ++i;
      while (i < line.size() && line[i] != '}') {
        ++i;
      }
    }
    if (i < line.size() && line[i] == '}') {
      ++i;
    }
  }
  return result;
}

std::string negated_immediate_text(std::int64_t imm) {
  if (imm == std::numeric_limits<std::int64_t>::min()) {
    const auto magnitude =
        static_cast<unsigned long long>(std::numeric_limits<std::int64_t>::max()) + 1ULL;
    return "-" + std::to_string(magnitude);
  }
  return std::to_string(-imm);
}

bool emit_operand_common(std::string& result,
                         std::size_t idx,
                         std::optional<char> modifier,
                         const std::vector<std::string>& op_regs,
                         const std::vector<bool>& op_is_memory,
                         const std::vector<std::string>& op_mem_addrs,
                         const std::vector<std::optional<std::int64_t>>& op_imm_values,
                         const std::vector<std::optional<std::string>>& op_imm_symbols) {
  const bool is_raw = modifier == 'c' || modifier == 'P';
  const bool is_neg = modifier == 'n';
  const auto has_symbol = idx < op_imm_symbols.size() ? op_imm_symbols[idx] : std::nullopt;
  const auto has_imm = idx < op_imm_values.size() ? op_imm_values[idx] : std::nullopt;

  if (is_neg) {
    if (has_imm.has_value()) {
      result += negated_immediate_text(*has_imm);
    } else if (idx < op_regs.size()) {
      result += op_regs[idx];
    }
    return true;
  }

  if (is_raw) {
    if (has_symbol.has_value()) {
      result += *has_symbol;
    } else if (has_imm.has_value()) {
      result += std::to_string(*has_imm);
    } else if (idx < op_is_memory.size() && op_is_memory[idx] && idx < op_mem_addrs.size()) {
      result += op_mem_addrs[idx];
    } else if (idx < op_regs.size()) {
      result += op_regs[idx];
    }
    return true;
  }

  if (idx < op_is_memory.size() && op_is_memory[idx] && idx < op_mem_addrs.size()) {
    result += op_mem_addrs[idx];
    return true;
  }

  if (modifier != 'a') {
    if (has_symbol.has_value()) {
      result += "$" + *has_symbol;
      return true;
    }
    if (has_imm.has_value()) {
      result += "$" + std::to_string(*has_imm);
      return true;
    }
  }

  return false;
}

std::string block_id_text(const BlockId& block_id) {
  return ".LBB" + std::to_string(block_id.raw);
}

}  // namespace

std::string X86Codegen::substitute_x86_asm_operands(
    const std::string& line,
    const std::vector<std::string>& op_regs,
    const std::vector<std::optional<std::string>>& op_names,
    const std::vector<bool>& op_is_memory,
    const std::vector<std::string>& op_mem_addrs,
    const std::vector<IrType>& op_types,
    const std::vector<std::size_t>& gcc_to_internal,
    const std::vector<std::pair<std::string, BlockId>>& goto_labels,
    const std::vector<std::optional<std::int64_t>>& op_imm_values,
    const std::vector<std::optional<std::string>>& op_imm_symbols) {
  const std::string resolved = resolve_dialect_alternatives(line);
  std::string result;
  result.reserve(resolved.size());

  for (std::size_t i = 0; i < resolved.size();) {
    if (resolved[i] != '%' || i + 1 >= resolved.size()) {
      result.push_back(resolved[i]);
      ++i;
      continue;
    }

    ++i;
    if (resolved[i] == '%') {
      result.push_back('%');
      ++i;
      continue;
    }

    std::optional<char> modifier;
    if (resolved[i] == 'l' && i + 1 < resolved.size() && resolved[i + 1] == '[' &&
        !goto_labels.empty()) {
      const std::size_t saved_i = i;
      i += 2;
      const std::size_t name_start = i;
      while (i < resolved.size() && resolved[i] != ']') {
        ++i;
      }
      const std::string name = resolved.substr(name_start, i - name_start);
      if (i < resolved.size()) {
        ++i;
      }
      bool found_label = false;
      for (const auto& label : goto_labels) {
        if (label.first == name) {
          result += block_id_text(label.second);
          found_label = true;
          break;
        }
      }
      if (found_label) {
        continue;
      }
      i = saved_i;
      modifier = 'l';
      ++i;
    } else if (resolved[i] == 'l' && i + 1 < resolved.size() &&
               std::isdigit(static_cast<unsigned char>(resolved[i + 1])) &&
               !goto_labels.empty()) {
      const std::size_t saved_i = i;
      ++i;
      std::size_t num = 0;
      while (i < resolved.size() && std::isdigit(static_cast<unsigned char>(resolved[i]))) {
        num = num * 10 + static_cast<std::size_t>(resolved[i] - '0');
        ++i;
      }
      const std::size_t label_idx =
          num >= op_regs.size() ? num - op_regs.size() : goto_labels.size();
      if (label_idx < goto_labels.size()) {
        result += block_id_text(goto_labels[label_idx].second);
        continue;
      }
      i = saved_i;
      modifier = 'l';
      ++i;
    } else if (resolved[i] == 'P') {
      if (i + 1 < resolved.size() &&
          (std::isdigit(static_cast<unsigned char>(resolved[i + 1])) || resolved[i + 1] == '[')) {
        modifier = 'P';
        ++i;
      }
    } else if ((resolved[i] == 'k' || resolved[i] == 'w' || resolved[i] == 'b' ||
                resolved[i] == 'h' || resolved[i] == 'q' || resolved[i] == 'l' ||
                resolved[i] == 'c' || resolved[i] == 'a' || resolved[i] == 'n') &&
               i + 1 < resolved.size() &&
               (std::isdigit(static_cast<unsigned char>(resolved[i + 1])) ||
                resolved[i + 1] == '[')) {
      modifier = resolved[i];
      ++i;
    }

    if (resolved[i] == '[') {
      ++i;
      const std::size_t name_start = i;
      while (i < resolved.size() && resolved[i] != ']') {
        ++i;
      }
      const std::string name = resolved.substr(name_start, i - name_start);
      if (i < resolved.size()) {
        ++i;
      }

      bool found = false;
      for (std::size_t idx = 0; idx < op_names.size(); ++idx) {
        if (op_names[idx].has_value() && *op_names[idx] == name) {
          emit_operand_with_modifier(result,
                                     idx,
                                     modifier,
                                     op_regs,
                                     op_is_memory,
                                     op_mem_addrs,
                                     op_types,
                                     op_imm_values,
                                     op_imm_symbols);
          found = true;
          break;
        }
      }
      if (!found) {
        result.push_back('%');
        if (modifier.has_value()) {
          result.push_back(*modifier);
        }
        result.push_back('[');
        result += name;
        result.push_back(']');
      }
      continue;
    }

    if (std::isdigit(static_cast<unsigned char>(resolved[i]))) {
      std::size_t num = 0;
      while (i < resolved.size() && std::isdigit(static_cast<unsigned char>(resolved[i]))) {
        num = num * 10 + static_cast<std::size_t>(resolved[i] - '0');
        ++i;
      }
      const std::size_t internal_idx =
          num < gcc_to_internal.size() ? gcc_to_internal[num] : num;
      if (internal_idx < op_regs.size()) {
        emit_operand_with_modifier(result,
                                   internal_idx,
                                   modifier,
                                   op_regs,
                                   op_is_memory,
                                   op_mem_addrs,
                                   op_types,
                                   op_imm_values,
                                   op_imm_symbols);
      } else {
        result.push_back('%');
        if (modifier.has_value()) {
          result.push_back(*modifier);
        }
        result += std::to_string(num);
      }
      continue;
    }

    result.push_back('%');
    if (modifier.has_value()) {
      result.push_back(*modifier);
    }
    result.push_back(resolved[i]);
    ++i;
  }

  return result;
}

void X86Codegen::emit_operand_with_modifier(
    std::string& result,
    std::size_t idx,
    std::optional<char> modifier,
    const std::vector<std::string>& op_regs,
    const std::vector<bool>& op_is_memory,
    const std::vector<std::string>& op_mem_addrs,
    const std::vector<IrType>& op_types,
    const std::vector<std::optional<std::int64_t>>& op_imm_values,
    const std::vector<std::optional<std::string>>& op_imm_symbols) {
  if (emit_operand_common(result,
                          idx,
                          modifier,
                          op_regs,
                          op_is_memory,
                          op_mem_addrs,
                          op_imm_values,
                          op_imm_symbols)) {
    return;
  }

  const auto has_symbol = idx < op_imm_symbols.size() ? op_imm_symbols[idx] : std::nullopt;
  const auto has_imm = idx < op_imm_values.size() ? op_imm_values[idx] : std::nullopt;
  if (modifier == 'a') {
    if (has_symbol.has_value()) {
      result += *has_symbol + "(%rip)";
    } else if (has_imm.has_value()) {
      result += std::to_string(*has_imm);
    } else if (idx < op_is_memory.size() && op_is_memory[idx] && idx < op_mem_addrs.size()) {
      result += op_mem_addrs[idx];
    } else if (idx < op_regs.size()) {
      result += "(%" + op_regs[idx] + ")";
    }
    return;
  }

  std::optional<char> effective_modifier = modifier;
  if (!effective_modifier.has_value() && idx < op_types.size()) {
    effective_modifier = default_modifier_for_type(op_types[idx]);
  }
  result.push_back('%');
  if (idx < op_regs.size()) {
    result += format_x86_reg(op_regs[idx], effective_modifier);
  }
}

std::optional<char> X86Codegen::default_modifier_for_type(std::optional<IrType> ty) {
  switch (ty.value_or(IrType::Void)) {
    case IrType::I8: return 'b';
    case IrType::I16:
    case IrType::U16: return 'w';
    case IrType::I32:
    case IrType::U32:
    case IrType::F32: return 'k';
    default: return std::nullopt;
  }
}

std::string X86Codegen::format_x86_reg(const std::string& reg, std::optional<char> modifier) {
  if (reg.rfind("xmm", 0) == 0 || reg == "st" || reg.rfind("st(", 0) == 0) {
    return reg;
  }

  switch (modifier.value_or('q')) {
    case 'k':
    case 'l':
      return reg_to_32(reg);
    case 'w':
      return reg_to_16(reg);
    case 'b':
      return reg_to_8l(reg);
    case 'h':
      return x86_reg_name_to_8h(reg);
    case 'q':
      return reg;
    default:
      return reg;
  }
}

std::string X86Codegen::reg_to_32(const std::string& reg) { return x86_format_reg(reg, 'k'); }
std::string X86Codegen::reg_to_64(const std::string& reg) { return x86_reg_name_to_64(reg); }
std::string X86Codegen::reg_to_16(const std::string& reg) { return x86_reg_name_to_16(reg); }
std::string X86Codegen::reg_to_8l(const std::string& reg) { return x86_reg_name_to_8l(reg); }

const char* X86Codegen::gcc_cc_to_x86(const std::string& cond) {
  return x86_gcc_cc_to_x86(cond);
}

}  // namespace c4c::backend::x86
