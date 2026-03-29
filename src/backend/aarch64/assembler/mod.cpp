#include "mod.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::assembler {

bool write_elf_object(const std::vector<AsmStatement>& statements, const std::string& output_path);

std::vector<AsmStatement> expand_literal_pools(const std::vector<AsmStatement>& statements) {
  return statements;
}

bool is_numeric_label(const std::string& name) {
  for (char c : name) {
    if (c < '0' || c > '9') return false;
  }
  return !name.empty();
}

std::pair<std::string, bool> parse_numeric_ref(const std::string& name) {
  if (name.size() < 2) return {"", false};
  const char suffix = name.back();
  if (suffix == 'f' || suffix == 'F') return {name.substr(0, name.size() - 1), true};
  if (suffix == 'b' || suffix == 'B') return {name.substr(0, name.size() - 1), false};
  return {"", false};
}

std::string resolve_numeric_name(const std::string& name,
                                 std::size_t current_idx,
                                 const std::vector<std::pair<std::size_t, std::string>>& defs) {
  (void)current_idx;
  (void)defs;
  return name;
}

std::vector<AsmStatement> resolve_numeric_labels(const std::vector<AsmStatement>& statements) {
  return statements;
}

std::string resolve_numeric_refs_in_expr(const std::string& expr,
                                        std::size_t current_idx,
                                        const std::vector<std::pair<std::size_t, std::string>>& defs) {
  (void)current_idx;
  (void)defs;
  return expr;
}

std::string resolve_numeric_directive(const std::string& directive,
                                      std::size_t current_idx,
                                      const std::vector<std::pair<std::size_t, std::string>>& defs) {
  (void)current_idx;
  (void)defs;
  return directive;
}

std::vector<std::string> resolve_numeric_data_values(const std::vector<std::string>& values,
                                                     std::size_t current_idx,
                                                     const std::vector<std::pair<std::size_t, std::string>>& defs) {
  (void)current_idx;
  (void)defs;
  return values;
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
