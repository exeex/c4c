#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::x86::linker {

// Marker type for the staged x86 linker contract surface.
struct ContractSurface final {};

constexpr std::uint64_t BASE_ADDR = 0x400000;
constexpr std::uint64_t PAGE_SIZE = 0x1000;
inline constexpr const char INTERP[] = "/lib64/ld-linux-x86-64.so.2\0";

struct Rela {
  std::size_t sym_idx = 0;
  std::uint32_t rela_type = 0;
  std::int64_t addend = 0;
};

struct Symbol {
  std::string name;
  std::uint8_t info = 0;
  std::uint16_t shndx = 0;
  std::uint64_t value = 0;
  std::uint64_t size = 0;

  bool is_local() const { return (info >> 4) == 0; }
  std::uint8_t sym_type() const { return static_cast<std::uint8_t>(info & 0x0f); }
};

struct SectionInput {
  std::size_t object_idx = 0;
  std::size_t section_idx = 0;
  std::uint64_t output_offset = 0;
};

struct OutputSection {
  std::string name;
  std::uint64_t flags = 0;
  std::uint32_t sh_type = 0;
  std::uint64_t alignment = 1;
  std::uint64_t mem_size = 0;
  std::uint64_t addr = 0;
  std::uint64_t file_offset = 0;
  std::vector<std::uint8_t> data;
  std::vector<SectionInput> inputs;
};

struct ElfObject {
  std::vector<std::string> sections;
  std::vector<std::vector<Rela>> relocations;
  std::vector<Symbol> symbols;
  std::vector<std::vector<std::uint8_t>> section_data;
};

struct DynSymbol {
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t info = 0;
  std::optional<std::string> version;
  std::optional<std::string> from_lib;
};

struct LinkerScriptEntry {
  enum class Kind { Path, Lib } kind = Kind::Path;
  std::string value;
};

struct LinkerSymbolAddresses {
  std::uint64_t base_addr = 0;
  std::uint64_t got_addr = 0;
  std::uint64_t dynamic_addr = 0;
  std::uint64_t bss_addr = 0;
  std::uint64_t bss_size = 0;
  std::uint64_t text_end = 0;
  std::uint64_t data_start = 0;
  std::uint64_t init_array_start = 0;
  std::uint64_t init_array_size = 0;
  std::uint64_t fini_array_start = 0;
  std::uint64_t fini_array_size = 0;
  std::uint64_t preinit_array_start = 0;
  std::uint64_t preinit_array_size = 0;
  std::uint64_t rela_iplt_start = 0;
  std::uint64_t rela_iplt_size = 0;
};

struct StandardLinkerSymbol {
  const char* name = nullptr;
  std::uint64_t value = 0;
  std::uint8_t binding = 0;
};

struct GlobalSymbol {
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t info = 0;
  std::optional<std::size_t> defined_in;
  std::optional<std::string> from_lib;
  std::optional<std::size_t> plt_idx;
  std::optional<std::size_t> got_idx;
  std::uint16_t section_idx = 0;
  bool is_dynamic = false;
  bool copy_reloc = false;
  std::uint64_t lib_sym_value = 0;
  std::optional<std::string> version;
};

using SectionKey = std::pair<std::size_t, std::size_t>;
using SectionMap = std::map<SectionKey, std::pair<std::size_t, std::uint64_t>>;

class DynStrTab {
 public:
  void add(const std::string& value);
  std::size_t get_offset(const std::string& value) const;
  const std::vector<std::uint8_t>& as_bytes() const;

 private:
  std::vector<std::string> entries_;
  mutable std::vector<std::uint8_t> cached_bytes_;
  mutable bool dirty_ = true;
};

bool x86_should_replace_extra(const GlobalSymbol& existing);

bool parse_object(const std::vector<std::uint8_t>& data, const std::string& source_name, ElfObject* out);
bool parse_shared_library_symbols(const std::vector<std::uint8_t>& data,
                                  const std::string& lib_name,
                                  std::vector<DynSymbol>* out);
std::optional<std::string> parse_soname(const std::vector<std::uint8_t>& data);

bool load_file(const std::string& path,
               std::vector<ElfObject>* objects,
               std::map<std::string, GlobalSymbol>* globals,
               std::vector<std::string>* needed_sonames,
               const std::vector<std::string>& lib_paths,
               bool whole_archive,
               std::string* error = nullptr);

std::vector<std::string> collect_ifunc_symbols(const std::map<std::string, GlobalSymbol>& globals,
                                               bool is_static);
std::pair<std::vector<std::string>, std::vector<std::pair<std::string, bool>>>
create_plt_got(const std::vector<ElfObject>& objects,
               std::map<std::string, GlobalSymbol>* globals);

bool emit_executable(const std::vector<ElfObject>& objects,
                     std::map<std::string, GlobalSymbol>* globals,
                     std::vector<OutputSection>* output_sections,
                     const SectionMap& section_map,
                     const std::vector<std::string>& plt_names,
                     const std::vector<std::pair<std::string, bool>>& got_entries,
                     const std::vector<std::string>& needed_sonames,
                     const std::string& output_path,
                     bool export_dynamic,
                     const std::vector<std::string>& rpath_entries,
                     bool use_runpath,
                     bool is_static,
                     const std::vector<std::string>& ifunc_symbols,
                     std::string* error = nullptr);

bool emit_shared_library(const std::vector<ElfObject>& objects,
                         std::map<std::string, GlobalSymbol>* globals,
                         std::vector<OutputSection>* output_sections,
                         const SectionMap& section_map,
                         const std::vector<std::string>& needed_sonames,
                         const std::string& output_path,
                         const std::optional<std::string>& soname,
                         const std::vector<std::string>& rpath_entries,
                         bool use_runpath,
                         std::string* error = nullptr);

bool link_builtin(const std::vector<const char*>& object_files,
                  const std::string& output_path,
                  const std::vector<std::string>& user_args,
                  const std::vector<const char*>& lib_paths,
                  const std::vector<const char*>& needed_libs,
                  const std::vector<const char*>& crt_objects_before,
                  const std::vector<const char*>& crt_objects_after,
                  std::string* error = nullptr);

bool link_shared(const std::vector<const char*>& object_files,
                 const std::string& output_path,
                 const std::vector<std::string>& user_args,
                 const std::vector<const char*>& lib_paths,
                 const std::vector<const char*>& needed_libs,
                 std::string* error = nullptr);

}  // namespace c4c::backend::x86::linker
