// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/riscv/linker/symbols.rs
// Phase 3: global symbol table construction and GOT/PLT symbol scans.

#include "../../elf/mod.hpp"
#include "../../linker_common/mod.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::linker {

using ElfObject = linker_common::Elf64Object;
using Symbol = linker_common::Elf64Symbol;

struct DynSymbol {
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t info = 0;
  std::optional<std::string> version;
  std::optional<std::string> from_lib;

  std::uint8_t sym_type() const { return static_cast<std::uint8_t>(info & 0x0f); }
};

static constexpr std::uint16_t SHN_ABS = 0xfff1;
static constexpr std::uint16_t SHN_COMMON = 0xfff2;
static constexpr std::uint8_t STT_OBJECT = 1;
static constexpr std::uint8_t STT_SECTION = 3;
static constexpr std::uint64_t SHF_WRITE = 0x1;

static constexpr std::uint32_t R_RISCV_GOT_HI20 = 20;
static constexpr std::uint32_t R_RISCV_TLS_GOT_HI20 = 21;
static constexpr std::uint32_t R_RISCV_TLS_GD_HI20 = 22;

struct GlobalSym {
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t binding = 0;
  std::uint8_t sym_type = 0;
  std::uint8_t visibility = 0;
  bool defined = false;
  bool needs_plt = false;
  std::size_t plt_idx = 0;
  std::optional<std::uint64_t> got_offset;
  std::optional<std::size_t> section_idx;
};

struct MergedSection {
  std::string name;
  std::uint32_t sh_type = 0;
  std::uint64_t sh_flags = 0;
  std::vector<std::uint8_t> data;
  std::uint64_t vaddr = 0;
  std::uint64_t align = 1;
};

struct GotLocalSymInfo {
  std::size_t obj_idx = 0;
  std::size_t sym_idx = 0;
  std::int64_t addend = 0;
};

using SectionKey = std::pair<std::size_t, std::size_t>;
using SectionMapping = std::map<SectionKey, std::pair<std::size_t, std::uint64_t>>;

namespace {

std::uint8_t symbol_binding(const Symbol& sym) {
  return sym.binding();
}

std::uint8_t symbol_type(const Symbol& sym) {
  return sym.sym_type();
}

bool is_valid_c_identifier_for_section(std::string_view text) {
  if (text.empty()) return false;
  const unsigned char first = static_cast<unsigned char>(text.front());
  if (!(std::isalpha(first) || first == '_')) return false;
  for (std::size_t i = 1; i < text.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(text[i]);
    if (!(std::isalnum(ch) || ch == '_')) return false;
  }
  return true;
}

bool is_linker_defined_symbol(std::string_view name) {
  static constexpr const char* kLinkerDefined[] = {
      "_GLOBAL_OFFSET_TABLE_",
      "__bss_start", "__bss_start__", "__BSS_END__",
      "_edata", "edata", "_end", "end", "__end", "__end__",
      "_etext", "etext",
      "__ehdr_start", "__executable_start",
      "__dso_handle", "_DYNAMIC",
      "__data_start", "data_start", "__DATA_BEGIN__",
      "__SDATA_BEGIN__",
      "__init_array_start", "__init_array_end",
      "__fini_array_start", "__fini_array_end",
      "__preinit_array_start", "__preinit_array_end",
      "__rela_iplt_start", "__rela_iplt_end",
      "__rel_iplt_start", "__rel_iplt_end",
      "__global_pointer$",
      "_IO_stdin_used",
      "_init", "_fini",
      "___tls_get_addr",
      "__tls_get_addr",
      "_ITM_registerTMCloneTable", "_ITM_deregisterTMCloneTable",
      "__gcc_personality_v0", "_Unwind_Resume", "_Unwind_ForcedUnwind", "_Unwind_GetCFA",
      "__pthread_initialize_minimal", "_dl_rtld_map",
      "__GNU_EH_FRAME_HDR",
      "__getauxval",
      "_r_debug", "_dl_debug_state", "_dl_mcount",
  };

  for (const char* candidate : kLinkerDefined) {
    if (name == candidate) return true;
  }

  auto starts_with = [&](std::string_view prefix) {
    return name.size() >= prefix.size() && name.substr(0, prefix.size()) == prefix;
  };

  if (starts_with("__start_")) {
    return is_valid_c_identifier_for_section(name.substr(8));
  }
  if (starts_with("__stop_")) {
    return is_valid_c_identifier_for_section(name.substr(7));
  }
  return false;
}

std::pair<std::string, bool> got_sym_key(std::size_t obj_idx, const Symbol& sym, std::int64_t addend) {
  if (symbol_type(sym) == STT_SECTION) {
    return {std::string("__local_sec_") + std::to_string(obj_idx) + "_" +
                std::to_string(sym.shndx) + "_" + std::to_string(addend),
            true};
  }
  if (symbol_binding(sym) == elf::STB_LOCAL) {
    return {std::string("__local_") + std::to_string(obj_idx) + "_" + sym.name + "_" +
                std::to_string(addend),
            true};
  }
  return {sym.name, false};
}

GlobalSym make_undefined_global_sym(const Symbol& sym) {
  return GlobalSym{
      .value = 0,
      .size = 0,
      .binding = symbol_binding(sym),
      .sym_type = symbol_type(sym),
      .visibility = sym.visibility(),
      .defined = false,
      .needs_plt = false,
      .plt_idx = 0,
      .got_offset = std::nullopt,
      .section_idx = std::nullopt,
  };
}

GlobalSym make_defined_global_sym(const Symbol& sym, std::uint64_t value) {
  return GlobalSym{
      .value = value,
      .size = sym.size,
      .binding = symbol_binding(sym),
      .sym_type = symbol_type(sym),
      .visibility = sym.visibility(),
      .defined = true,
      .needs_plt = false,
      .plt_idx = 0,
      .got_offset = std::nullopt,
      .section_idx = std::nullopt,
  };
}

GlobalSym make_common_global_sym(const Symbol& sym, std::size_t bss_idx, std::uint64_t off) {
  return GlobalSym{
      .value = off,
      .size = sym.size,
      .binding = symbol_binding(sym),
      .sym_type = STT_OBJECT,
      .visibility = sym.visibility(),
      .defined = true,
      .needs_plt = false,
      .plt_idx = 0,
      .got_offset = std::nullopt,
      .section_idx = bss_idx,
  };
}

bool should_replace_with(const GlobalSym& existing, const Symbol& incoming) {
  return !existing.defined || (existing.binding == elf::STB_WEAK && symbol_binding(incoming) == elf::STB_GLOBAL);
}

bool contains_name(const std::vector<std::string>& values, const std::string& name) {
  return std::find(values.begin(), values.end(), name) != values.end();
}

}  // namespace

std::unordered_map<std::string, GlobalSym> build_global_symbols(
    const std::vector<std::pair<std::string, ElfObject>>& input_objs,
    const SectionMapping& sec_mapping,
    std::vector<MergedSection>* merged_sections,
    std::unordered_map<std::string, std::size_t>* merged_map) {
  std::unordered_map<std::string, GlobalSym> global_syms;

  for (std::size_t obj_idx = 0; obj_idx < input_objs.size(); ++obj_idx) {
    const auto& obj = input_objs[obj_idx].second;
    for (const auto& sym : obj.symbols) {
      if (sym.name.empty() || sym.binding() == elf::STB_LOCAL) {
        continue;
      }

      if (sym.shndx == elf::SHN_UNDEF) {
        global_syms.try_emplace(sym.name, make_undefined_global_sym(sym));
        continue;
      }

      if (sym.shndx == SHN_ABS) {
        auto [it, inserted] = global_syms.try_emplace(sym.name, make_defined_global_sym(sym, sym.value));
        (void)inserted;
        if (should_replace_with(it->second, sym)) {
          it->second.value = sym.value;
          it->second.size = sym.size;
          it->second.binding = symbol_binding(sym);
          it->second.sym_type = symbol_type(sym);
          it->second.visibility = sym.visibility();
          it->second.defined = true;
        }
        continue;
      }

      if (sym.shndx == SHN_COMMON) {
        const auto bss_idx = [&]() -> std::size_t {
          auto it = merged_map->find(".bss");
          if (it != merged_map->end()) return it->second;
          const std::size_t idx = merged_sections->size();
          merged_map->emplace(".bss", idx);
          merged_sections->push_back(MergedSection{
              .name = ".bss",
              .sh_type = elf::SHT_NOBITS,
              .sh_flags = elf::SHF_ALLOC | SHF_WRITE,
              .data = {},
              .vaddr = 0,
              .align = 8,
          });
          return idx;
        }();

        auto& ms = (*merged_sections)[bss_idx];
        const std::size_t align = static_cast<std::size_t>(std::max<std::uint64_t>(sym.value, 1));
        const std::size_t cur = ms.data.size();
        const std::size_t aligned = (cur + align - 1) & ~(align - 1);
        ms.data.resize(aligned, 0);
        const std::uint64_t off = ms.data.size();
        ms.data.resize(ms.data.size() + sym.size, 0);
        ms.align = std::max(ms.align, static_cast<std::uint64_t>(align));

        auto [it, inserted] = global_syms.try_emplace(sym.name, make_common_global_sym(sym, bss_idx, off));
        (void)inserted;
        if (should_replace_with(it->second, sym)) {
          it->second.value = off;
          it->second.size = std::max(it->second.size, sym.size);
          it->second.binding = symbol_binding(sym);
          it->second.defined = true;
          it->second.section_idx = bss_idx;
        }
        continue;
      }

      const std::size_t sec_idx = sym.shndx;
      const auto mapping_it = sec_mapping.find({obj_idx, sec_idx});
      if (mapping_it == sec_mapping.end()) {
        continue;
      }
      const auto [merged_idx, offset] = mapping_it->second;
      auto [it, inserted] = global_syms.try_emplace(sym.name, make_undefined_global_sym(sym));
      (void)inserted;

      if (it->second.defined && !(it->second.binding == elf::STB_WEAK && symbol_binding(sym) == elf::STB_GLOBAL)) {
        continue;
      }

      it->second.value = offset + sym.value;
      it->second.size = sym.size;
      it->second.binding = symbol_binding(sym);
      it->second.sym_type = symbol_type(sym);
      it->second.visibility = sym.visibility();
      it->second.defined = true;
      it->second.section_idx = merged_idx;
    }
  }

  return global_syms;
}

void allocate_common_symbol(const Symbol& sym,
                            std::unordered_map<std::string, GlobalSym>* global_syms,
                            std::vector<MergedSection>* merged_sections,
                            std::unordered_map<std::string, std::size_t>* merged_map) {
  const auto bss_idx = [&]() -> std::size_t {
    auto it = merged_map->find(".bss");
    if (it != merged_map->end()) return it->second;
    const std::size_t idx = merged_sections->size();
    merged_map->emplace(".bss", idx);
    merged_sections->push_back(MergedSection{
        .name = ".bss",
        .sh_type = elf::SHT_NOBITS,
        .sh_flags = elf::SHF_ALLOC | SHF_WRITE,
        .data = {},
        .vaddr = 0,
        .align = 8,
    });
    return idx;
  }();

  auto& ms = (*merged_sections)[bss_idx];
  const std::size_t align = static_cast<std::size_t>(std::max<std::uint64_t>(sym.value, 1));
  const std::size_t cur = ms.data.size();
  const std::size_t aligned = (cur + align - 1) & ~(align - 1);
  ms.data.resize(aligned, 0);
  const std::uint64_t off = ms.data.size();
  ms.data.resize(ms.data.size() + sym.size, 0);
  ms.align = std::max(ms.align, static_cast<std::uint64_t>(align));

  auto [it, inserted] = global_syms->try_emplace(sym.name, make_common_global_sym(sym, bss_idx, off));
  (void)inserted;
  if (!it->second.defined || (it->second.binding == elf::STB_WEAK && sym.binding() == elf::STB_GLOBAL)) {
    it->second.value = off;
    it->second.size = std::max(it->second.size, sym.size);
    it->second.binding = symbol_binding(sym);
    it->second.defined = true;
    it->second.section_idx = bss_idx;
  }
}

std::pair<std::vector<std::string>, std::vector<std::pair<std::string, std::uint64_t>>> mark_plt_and_copy_symbols(
    std::unordered_map<std::string, GlobalSym>* global_syms,
    const std::unordered_map<std::string, DynSymbol>& shared_lib_syms) {
  std::vector<std::string> plt_symbols;
  std::vector<std::pair<std::string, std::uint64_t>> copy_symbols;

  for (auto& [name, sym] : *global_syms) {
    if (!sym.defined) {
      auto it = shared_lib_syms.find(name);
      if (it == shared_lib_syms.end()) continue;
      if (it->second.sym_type() == STT_OBJECT) {
        copy_symbols.emplace_back(name, it->second.size);
      } else {
        sym.needs_plt = true;
        sym.plt_idx = plt_symbols.size();
        plt_symbols.push_back(name);
      }
    }
  }

  return {plt_symbols, copy_symbols};
}

std::tuple<std::vector<std::string>, std::unordered_set<std::string>, std::unordered_map<std::string, GotLocalSymInfo>>
collect_got_entries(const std::vector<std::pair<std::string, ElfObject>>& input_objs) {
  std::vector<std::string> got_symbols;
  std::unordered_set<std::string> tls_got_symbols;
  std::unordered_map<std::string, GotLocalSymInfo> local_got_sym_info;
  std::unordered_set<std::string> got_set;

  for (std::size_t obj_idx = 0; obj_idx < input_objs.size(); ++obj_idx) {
    const auto& obj = input_objs[obj_idx].second;
    for (const auto& relocs : obj.relocations) {
      for (const auto& reloc : relocs) {
        if (reloc.rela_type == R_RISCV_GOT_HI20 ||
            reloc.rela_type == R_RISCV_TLS_GOT_HI20 ||
            reloc.rela_type == R_RISCV_TLS_GD_HI20) {
          const auto sym_idx = static_cast<std::size_t>(reloc.sym_idx);
          if (sym_idx >= obj.symbols.size()) continue;
          const auto& sym = obj.symbols[sym_idx];
          const auto [name, is_local] = got_sym_key(obj_idx, sym, reloc.addend);
          if (!name.empty() && !got_set.contains(name)) {
            got_set.insert(name);
            got_symbols.push_back(name);
            if (is_local) {
              local_got_sym_info.emplace(name, GotLocalSymInfo{obj_idx, sym_idx, reloc.addend});
            }
          }
          if (reloc.rela_type == R_RISCV_TLS_GOT_HI20 || reloc.rela_type == R_RISCV_TLS_GD_HI20) {
            tls_got_symbols.insert(name);
          }
        }
      }
    }
  }

  return {got_symbols, tls_got_symbols, local_got_sym_info};
}

std::vector<std::vector<std::uint64_t>> build_local_sym_vaddrs(
    const std::vector<std::pair<std::string, ElfObject>>& input_objs,
    const SectionMapping& sec_mapping,
    const std::vector<std::uint64_t>& section_vaddrs,
    const std::unordered_map<std::string, GlobalSym>& global_syms) {
  std::vector<std::vector<std::uint64_t>> local_sym_vaddrs;
  local_sym_vaddrs.reserve(input_objs.size());
  for (std::size_t obj_idx = 0; obj_idx < input_objs.size(); ++obj_idx) {
    const auto& obj = input_objs[obj_idx].second;
    std::vector<std::uint64_t> sym_vaddrs(obj.symbols.size(), 0);
    for (std::size_t si = 0; si < obj.symbols.size(); ++si) {
      const auto& sym = obj.symbols[si];
      if (sym.shndx == elf::SHN_UNDEF || sym.shndx == SHN_ABS) {
        if (sym.shndx == SHN_ABS) {
          sym_vaddrs[si] = sym.value;
        }
        continue;
      }
      if (sym.shndx == SHN_COMMON) {
        auto it = global_syms.find(sym.name);
        if (it != global_syms.end()) {
          sym_vaddrs[si] = it->second.value;
        }
        continue;
      }
      const auto sec_idx = static_cast<std::size_t>(sym.shndx);
      auto mapping_it = sec_mapping.find({obj_idx, sec_idx});
      if (mapping_it != sec_mapping.end()) {
        const auto [merged_idx, offset] = mapping_it->second;
        sym_vaddrs[si] = section_vaddrs[merged_idx] + offset + sym.value;
      }
    }
    local_sym_vaddrs.push_back(std::move(sym_vaddrs));
  }
  return local_sym_vaddrs;
}

bool check_undefined_symbols(const std::unordered_map<std::string, GlobalSym>& global_syms,
                             const std::unordered_map<std::string, DynSymbol>& shared_lib_syms,
                             std::string* error) {
  std::vector<std::string> truly_undefined;
  for (const auto& [name, sym] : global_syms) {
    if (!sym.defined && !sym.needs_plt && sym.binding != elf::STB_WEAK &&
        !is_linker_defined_symbol(name) && !shared_lib_syms.contains(name)) {
      truly_undefined.push_back(name);
    }
  }

  if (!truly_undefined.empty()) {
    std::sort(truly_undefined.begin(), truly_undefined.end());
    if (truly_undefined.size() > 20) {
      truly_undefined.resize(20);
    }
    if (error != nullptr) {
      std::string message = "undefined symbols: ";
      for (std::size_t i = 0; i < truly_undefined.size(); ++i) {
        if (i != 0) message += ", ";
        message += truly_undefined[i];
      }
      *error = std::move(message);
    }
    return false;
  }

  return true;
}

}  // namespace c4c::backend::riscv::linker
