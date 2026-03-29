#include "types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::assembler {

static constexpr std::uint8_t AARCH64_NOP[4] = {0x1f, 0x20, 0x03, 0xd5};

struct PendingReloc {
  std::string section;
  std::uint64_t offset = 0;
  std::uint32_t reloc_type = 0;
  std::string symbol;
  std::int64_t addend = 0;
};

struct PendingExpr {
  std::string section;
  std::uint64_t offset = 0;
  std::string expr;
  std::size_t size = 0;
};

struct PendingSymDiff {
  std::string section;
  std::uint64_t offset = 0;
  std::string sym_a;
  std::string sym_b;
  std::int64_t extra_addend = 0;
  std::size_t size = 0;
};

struct PendingInstruction {
  std::string section;
  std::uint64_t offset = 0;
  std::string mnemonic;
  std::vector<Operand> operands;
  std::string raw_operands;
};

class ElfWriter {
 public:
  ElfWriter() = default;

  void process_statements(const std::vector<AsmStatement>& statements) {
    (void)statements;
  }

  void write_elf(const std::string& output_path) {
    (void)output_path;
  }

 private:
  void resolve_sym_diffs() {}
  void resolve_pending_exprs() {}
  void resolve_pending_instructions() {}
  void process_statement(const AsmStatement& stmt) { (void)stmt; }
};

bool is_branch_reloc_type(std::uint32_t elf_type) {
  return elf_type == 279 || elf_type == 280 || elf_type == 282 || elf_type == 283;
}

}  // namespace c4c::backend::aarch64::assembler
