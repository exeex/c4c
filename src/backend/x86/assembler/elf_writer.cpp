#include <cstdint>
#include <string>
#include <vector>

// x86-64 ELF relocatable object writer adaptor.

namespace c4c::backend::x86::assembler {

struct AsmItem;

struct X86_64Arch {
  static constexpr std::uint16_t elf_machine() { return 62; }  // EM_X86_64
  static constexpr std::uint8_t elf_class() { return 2; }       // ELFCLASS64
  static constexpr std::uint32_t reloc_abs(std::size_t size) {
    return size == 2 ? 12 : (size == 4 ? 10 : 1);
  }
  static constexpr std::uint32_t reloc_abs64() { return 1; }
  static constexpr std::uint32_t reloc_pc32() { return 2; }
  static constexpr std::uint32_t reloc_plt32() { return 4; }
  static constexpr bool uses_rel_format() { return false; }
  static constexpr std::uint32_t reloc_pc8_internal() { return 0x8000'0001u; }
  static constexpr std::uint32_t reloc_abs32_for_internal() { return 10; }
  static constexpr bool supports_deferred_skips() { return true; }
  static constexpr bool resolve_set_aliases_in_data() { return true; }
};

template <typename Arch>
class ElfWriterCore {
 public:
  bool build(const std::vector<AsmItem>& items, const std::string& output_path) {
    (void)items;
    (void)output_path;
    return false;
  }
};

using ElfWriter = ElfWriterCore<X86_64Arch>;

bool write_elf_object(const std::vector<AsmItem>& statements, const std::string& output_path) {
  ElfWriter writer;
  return writer.build(statements, output_path);
}

}  // namespace c4c::backend::x86::assembler
