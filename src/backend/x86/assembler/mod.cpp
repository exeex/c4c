#include "mod.hpp"

#include <fstream>
#include <string>
#include <vector>

// Native x86-64 assembler entry point.

namespace c4c::backend::x86::assembler {

struct AssembleRequest {
  std::string asm_text;
  std::string output_path;
};

struct AssembleResult {
  std::string staged_text;
  std::string output_path;
  bool object_emitted = false;
  std::string error;
};

std::vector<AsmItem> parse_asm(const std::string& text);
bool write_elf_object(const std::vector<AsmItem>& items, const std::string& output_path);

AssembleResult assemble(const AssembleRequest& request) {
  AssembleResult result;
  result.staged_text = request.asm_text;
  result.output_path = request.output_path;

  const auto items = parse_asm(request.asm_text);
  if (items.empty() && !request.asm_text.empty()) {
    result.error = "assembler parse produced no items";
    return result;
  }

  if (!request.output_path.empty()) {
    result.object_emitted = write_elf_object(items, request.output_path);
    if (!result.object_emitted) {
      result.error = "failed to write ELF object";
    }
  }

  return result;
}

std::string assemble(const std::string& asm_text, const std::string& output_path) {
  const auto result = assemble(AssembleRequest{.asm_text = asm_text, .output_path = output_path});
  return result.staged_text;
}

}  // namespace c4c::backend::x86::assembler
