#include "mod.hpp"

#include <exception>
#include <string>
#include <vector>

// Native x86-64 assembler entry point.

namespace c4c::backend::x86::assembler {

bool write_elf_object(const std::vector<AsmStatement>& statements,
                      const std::string& output_path);

AssembleResult assemble(const AssembleRequest& request) {
  AssembleResult result;
  result.staged_text = request.asm_text;
  result.output_path = request.output_path;

  try {
    const auto statements = parse_asm(request.asm_text);
    if (!request.output_path.empty()) {
      result.object_emitted = write_elf_object(statements, request.output_path);
      if (!result.object_emitted) {
        result.error = "failed to emit x86 ELF relocatable object";
      }
    }
  } catch (const std::exception& ex) {
    result.error = ex.what();
  }

  return result;
}

std::string assemble(const std::string& asm_text, const std::string& output_path) {
  const auto result = assemble(AssembleRequest{.asm_text = asm_text, .output_path = output_path});
  return result.staged_text;
}

}  // namespace c4c::backend::x86::assembler
