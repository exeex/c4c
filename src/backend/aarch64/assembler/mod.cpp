#include "mod.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::assembler {

bool write_elf_object(const std::vector<AsmStatement>& statements, const std::string& output_path);

namespace {

using NumericDefs = std::vector<std::pair<std::size_t, std::string>>;

constexpr std::size_t kLiteralPoolAlign = 8;

struct PoolEntry {
  std::string label;
  std::string symbol;
  std::int64_t addend = 0;
};

bool is_section_changing_directive(const std::string& op) {
  return op == ".text" || op == ".data" || op == ".rodata" || op == ".bss" ||
         op == ".section" || op == ".pushsection" || op == ".popsection" ||
         op == ".previous" || op == ".subsection" || op == ".ltorg";
}

std::pair<std::string, std::int64_t> split_symbol_addend(const std::string& token) {
  const auto expr = trim_asm(token);
  if (expr.empty()) return {expr, 0};

  auto plus_pos = expr.find('+', 1);
  auto minus_pos = expr.find('-', 1);
  if (plus_pos == std::string::npos && minus_pos == std::string::npos) return {expr, 0};

  const auto split = std::min(plus_pos == std::string::npos ? std::string::npos : plus_pos,
                              minus_pos == std::string::npos ? std::string::npos : minus_pos);
  if (split == 0 || split == std::string::npos || split + 1 >= expr.size()) return {expr, 0};

  const auto symbol = trim_asm(expr.substr(0, split));
  const auto magnitude = trim_asm(expr.substr(split + 1));
  if (magnitude.empty()) return {expr, 0};

  std::int64_t addend = 0;
  for (char c : magnitude) {
    if (c < '0' || c > '9') return {expr, 0};
    addend = addend * 10 + (c - '0');
  }
  if (expr[split] == '-') addend = -addend;

  return {symbol, addend};
}

bool is_numeric_label(const std::string& name) {
  if (name.empty()) return false;
  for (char c : name) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}

std::pair<std::string, bool> parse_numeric_ref(const std::string& name) {
  if (name.size() < 2) return {"", false};

  const auto number = name.substr(0, name.size() - 1);
  if (!is_numeric_label(number)) return {"", false};

  const char suffix = name.back();
  if (suffix == 'f' || suffix == 'F') return {number, true};
  if (suffix == 'b' || suffix == 'B') return {number, false};
  return {"", false};
}

std::string resolve_numeric_name(
    const std::string& name,
    std::size_t current_idx,
    const std::unordered_map<std::string, NumericDefs>& all_defs) {
  const auto parsed = parse_numeric_ref(name);
  if (parsed.first.empty()) return name;

  const auto it = all_defs.find(parsed.first);
  if (it == all_defs.end()) return name;

  if (parsed.second) {
    for (const auto& [def_idx, replacement] : it->second) {
      if (def_idx > current_idx) return replacement;
    }
    return name;
  }

  for (auto it_rev = it->second.rbegin(); it_rev != it->second.rend(); ++it_rev) {
    if (it_rev->first < current_idx) return it_rev->second;
  }
  return name;
}

bool is_prefixed_numeric(const std::string& token, std::size_t start, std::size_t end) {
  return end >= start + 3 && token[start] == '0' && (token[start + 1] == 'x' || token[start + 1] == 'X' ||
                                                   token[start + 1] == 'b' || token[start + 1] == 'B');
}

std::string resolve_numeric_refs_in_expr(
    const std::string& expr,
    std::size_t current_idx,
    const std::unordered_map<std::string, NumericDefs>& all_defs) {
  std::string out;
  out.reserve(expr.size());

  for (std::size_t i = 0; i < expr.size();) {
    if (std::isdigit(static_cast<unsigned char>(expr[i]))) {
      const auto start = i;

      if (is_prefixed_numeric(expr, start, expr.size())) {
        std::size_t end = start + 2;
        while (end < expr.size() && std::isalnum(static_cast<unsigned char>(expr[end]))) ++end;
        out += expr.substr(start, end - start);
        i = end;
        continue;
      }

      std::size_t end = start;
      while (end < expr.size() && std::isdigit(static_cast<unsigned char>(expr[end]))) ++end;
      const bool has_ref_suffix =
          end < expr.size() && (expr[end] == 'f' || expr[end] == 'F' || expr[end] == 'b' ||
                                expr[end] == 'B');
      if (has_ref_suffix) {
        const auto token = expr.substr(start, end - start + 1);
        out += resolve_numeric_name(token, current_idx, all_defs);
        i = end + 1;
      } else {
        out += expr.substr(start, end - start);
        i = end;
      }
      continue;
    }

    out += expr[i];
    ++i;
  }

  return out;
}

std::string resolve_numeric_directive(
    const std::string& directive,
    std::size_t current_idx,
    const std::unordered_map<std::string, NumericDefs>& all_defs) {
  return resolve_numeric_refs_in_expr(directive, current_idx, all_defs);
}

std::vector<std::string> resolve_numeric_data_values(
    const std::vector<std::string>& values,
    std::size_t current_idx,
    const std::unordered_map<std::string, NumericDefs>& all_defs) {
  std::vector<std::string> resolved;
  resolved.reserve(values.size());
  for (const auto& value : values) {
    resolved.push_back(resolve_numeric_refs_in_expr(value, current_idx, all_defs));
  }
  return resolved;
}

}  // namespace

std::vector<AsmStatement> expand_literal_pools(const std::vector<AsmStatement>& statements) {
  std::vector<AsmStatement> expanded;
  expanded.reserve(statements.size());
  std::vector<PoolEntry> pool;
  std::size_t pool_counter = 0;

  const auto flush_pool = [&](std::vector<AsmStatement>& out, std::vector<PoolEntry>& in_pool) {
    if (in_pool.empty()) return;
    out.push_back(AsmStatement{
        .kind = AsmStatementKind::Directive,
        .text = ".balign 8",
        .op = ".balign",
        .operands = {Operand{std::to_string(kLiteralPoolAlign)}},
        .raw_operands = std::to_string(kLiteralPoolAlign),
    });

    for (const auto& entry : in_pool) {
      out.push_back(AsmStatement{
          .kind = AsmStatementKind::Label,
          .text = entry.label + ":",
          .op = entry.label,
      });

      if (entry.addend == 0) {
        out.push_back(AsmStatement{
            .kind = AsmStatementKind::Directive,
            .text = ".quad " + entry.symbol,
            .op = ".quad",
            .operands = {Operand{entry.symbol}},
            .raw_operands = entry.symbol,
        });
      } else {
        std::ostringstream ss;
        ss << entry.symbol << (entry.addend >= 0 ? "+" : "") << entry.addend;
        const auto emitted = ss.str();
        out.push_back(AsmStatement{
            .kind = AsmStatementKind::Directive,
            .text = ".quad " + emitted,
            .op = ".quad",
            .operands = {Operand{emitted}},
            .raw_operands = emitted,
        });
      }
    }
    in_pool.clear();
  };

  for (const auto& statement : statements) {
    if (statement.kind == AsmStatementKind::Directive &&
        (statement.op == ".ltorg" || is_section_changing_directive(statement.op))) {
      flush_pool(expanded, pool);
    }

    if (statement.kind == AsmStatementKind::Instruction && statement.op == "ldr" &&
        statement.operands.size() >= 2 && !statement.operands[1].text.empty() &&
        statement.operands[1].text.front() == '=') {
      const auto [symbol, addend] = split_symbol_addend(statement.operands[1].text.substr(1));
      const auto pool_label = ".Llpool_" + std::to_string(pool_counter++);
      pool.push_back(PoolEntry{pool_label, symbol, addend});

      const auto rewritten_operands = statement.operands[0].text + ", " + pool_label;
      expanded.push_back(AsmStatement{
          .kind = AsmStatementKind::Instruction,
          .text = statement.op + " " + rewritten_operands,
          .op = statement.op,
          .operands = {statement.operands[0], Operand{pool_label}},
          .raw_operands = rewritten_operands,
      });
      continue;
    }

    expanded.push_back(statement);
  }

  flush_pool(expanded, pool);
  return expanded;
}

std::vector<AsmStatement> resolve_numeric_labels(const std::vector<AsmStatement>& statements) {
  std::unordered_map<std::string, NumericDefs> defs;
  std::unordered_map<std::string, std::size_t> unique_counter;

  for (std::size_t i = 0; i < statements.size(); ++i) {
    if (statements[i].kind == AsmStatementKind::Label &&
        is_numeric_label(statements[i].op)) {
      const auto unique = ".Lnum_" + statements[i].op + "_" + std::to_string(unique_counter[statements[i].op]++);
      defs[statements[i].op].push_back({i, unique});
    }
  }

  if (defs.empty()) return statements;

  std::vector<AsmStatement> resolved;
  resolved.reserve(statements.size());

  for (std::size_t i = 0; i < statements.size(); ++i) {
    const auto& statement = statements[i];

    if (statement.kind == AsmStatementKind::Label && is_numeric_label(statement.op)) {
      const auto it = defs.find(statement.op);
      bool emitted = false;
      if (it != defs.end()) {
        for (const auto& [idx, replacement] : it->second) {
          if (idx == i) {
            resolved.push_back(AsmStatement{
                .kind = AsmStatementKind::Label,
                .text = replacement + ":",
                .op = replacement,
            });
            emitted = true;
            break;
          }
        }
      }
      if (emitted) continue;
    }

    if (statement.kind == AsmStatementKind::Instruction) {
      AsmStatement rewritten = statement;
      for (auto& operand : rewritten.operands) {
        operand.text = resolve_numeric_refs_in_expr(operand.text, i, defs);
      }
      std::string raw;
      for (std::size_t oi = 0; oi < rewritten.operands.size(); ++oi) {
        if (oi > 0) raw += ", ";
        raw += rewritten.operands[oi].text;
      }
      rewritten.raw_operands = raw;
      resolved.push_back(std::move(rewritten));
      continue;
    }

    if (statement.kind == AsmStatementKind::Directive) {
      AsmStatement rewritten = statement;
      rewritten.raw_operands = resolve_numeric_directive(statement.raw_operands, i, defs);
      std::vector<std::string> values;
      values.reserve(statement.operands.size());
      for (const auto& operand : statement.operands) values.push_back(operand.text);

      const auto resolved_values = resolve_numeric_data_values(values, i, defs);
      rewritten.operands.clear();
      rewritten.operands.reserve(resolved_values.size());
      for (const auto& value : resolved_values) {
        rewritten.operands.push_back(Operand{value});
      }

      std::string raw;
      for (std::size_t oi = 0; oi < rewritten.operands.size(); ++oi) {
        if (oi > 0) raw += ", ";
        raw += rewritten.operands[oi].text;
      }
      rewritten.raw_operands = raw;
      rewritten.text = rewritten.op + (raw.empty() ? "" : " " + raw);
      resolved.push_back(std::move(rewritten));
      continue;
    }

    resolved.push_back(statement);
  }

  return resolved;
}

AssembleResult assemble(const AssembleRequest& request) {
  const auto parsed = parse_asm(request.asm_text);
  const auto expanded = expand_literal_pools(parsed);
  const auto resolved = resolve_numeric_labels(expanded);
  bool object_emitted = false;
  if (!request.output_path.empty()) {
    object_emitted = write_elf_object(resolved, request.output_path);
  }
  return AssembleResult{
      .staged_text = request.asm_text,
      .output_path = request.output_path,
      .object_emitted = object_emitted,
  };
}

std::string assemble(const std::string& asm_text, const std::string& output_path) {
  return assemble(AssembleRequest{.asm_text = asm_text, .output_path = output_path}).staged_text;
}

}  // namespace c4c::backend::aarch64::assembler
