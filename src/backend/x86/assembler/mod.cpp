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

AssembleResult assemble(const AssembleRequest& request) {
  AssembleResult result;
  result.staged_text = request.asm_text;
  result.output_path = request.output_path;

  if (!request.output_path.empty()) {
    std::ofstream out(request.output_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      result.error = "failed to open output path";
      return result;
    }
    out.write(request.asm_text.data(),
              static_cast<std::streamsize>(request.asm_text.size()));
    result.object_emitted = out.good();
    if (!result.object_emitted) result.error = "failed to stage assembler output";
  }

  return result;
}

std::string assemble(const std::string& asm_text, const std::string& output_path) {
  const auto result = assemble(AssembleRequest{.asm_text = asm_text, .output_path = output_path});
  return result.staged_text;
}

}  // namespace c4c::backend::x86::assembler
