#include "mod.hpp"
#include "parser.hpp"

#include <exception>
#include <vector>

namespace c4c::backend::riscv::assembler {

bool write_elf_object(const std::vector<AsmStatement>& statements, const std::string& output_path);

namespace {

std::vector<AsmStatement> filter_nonempty_statements(const std::vector<AsmStatement>& statements) {
  std::vector<AsmStatement> filtered;
  filtered.reserve(statements.size());
  for (const auto& statement : statements) {
    if (statement.kind != AsmStatement::Kind::Empty) {
      filtered.push_back(statement);
    }
  }
  return filtered;
}

bool supports_emittable_slice(const std::vector<AsmStatement>& statements) {
  return write_elf_object(filter_nonempty_statements(statements), "");
}

}  // namespace

AssembleResult assemble(const AssembleRequest& request) {
  AssembleResult result;
  result.staged_text = request.asm_text;
  result.output_path = request.output_path;

  try {
    if (request.output_path.empty()) {
      return result;
    }

    const auto statements = parse_asm(request.asm_text);
    result.object_emitted = write_elf_object(filter_nonempty_statements(statements),
                                             request.output_path);
    if (!result.object_emitted) {
      if (!supports_emittable_slice(statements)) {
        result.error =
            "riscv64 built-in assembler object emission currently supports only the "
            "minimal prepared-LIR return-add handoff and the bounded single-object "
            "jal-helper relocation handoff";
      } else {
        result.error = "failed to emit riscv64 ELF relocatable object";
      }
    }
  } catch (const std::exception& ex) {
    result.error = ex.what();
  }

  return result;
}

std::string assemble(const std::string& asm_text, const std::string& output_path) {
  const auto result = assemble(AssembleRequest{
      .asm_text = asm_text,
      .output_path = output_path,
  });
  return result.staged_text;
}

}  // namespace c4c::backend::riscv::assembler
