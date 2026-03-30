// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/plt_got.rs
// PLT/GOT construction and IFUNC collection.

#include "mod.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::x86::linker {

namespace {

bool contains_name(const std::vector<std::string>& values, const std::string& name) {
  return std::find(values.begin(), values.end(), name) != values.end();
}

}  // namespace

std::vector<std::string> collect_ifunc_symbols(const std::map<std::string, GlobalSymbol>& globals,
                                               bool is_static) {
  if (!is_static) return {};
  std::vector<std::string> ifunc_symbols;
  for (const auto& [name, gsym] : globals) {
    if (gsym.defined_in.has_value() && (gsym.info & 0x0f) == 10 /* STT_GNU_IFUNC */) {
      ifunc_symbols.push_back(name);
    }
  }
  std::sort(ifunc_symbols.begin(), ifunc_symbols.end());
  return ifunc_symbols;
}

std::pair<std::vector<std::string>, std::vector<std::pair<std::string, bool>>>
create_plt_got(const std::vector<ElfObject>& objects,
               std::map<std::string, GlobalSymbol>* globals) {
  std::vector<std::string> plt_names;
  std::vector<std::string> got_only_names;
  std::vector<std::string> copy_reloc_names;

  for (const auto& obj : objects) {
    for (std::size_t sec_idx = 0; sec_idx < obj.relocations.size(); ++sec_idx) {
      for (const auto& rela : obj.relocations[sec_idx]) {
        const std::size_t si = rela.sym_idx;
        if (si >= obj.symbols.size()) continue;
        const Symbol& sym = obj.symbols[si];
        if (sym.name.empty() || sym.is_local()) continue;
        const auto gsym_it = globals->find(sym.name);
        const bool is_dynamic = gsym_it != globals->end() && gsym_it->second.is_dynamic;
        const std::uint8_t sym_type = gsym_it == globals->end() ? 0 : (gsym_it->second.info & 0x0f);

        if ((rela.rela_type == 4 /* PLT32 */ || rela.rela_type == 2 /* PC32 */) && is_dynamic) {
          if (sym_type == 1 /* STT_OBJECT */) {
            if (!contains_name(copy_reloc_names, sym.name)) {
              copy_reloc_names.push_back(sym.name);
            }
          } else if (!contains_name(plt_names, sym.name)) {
            plt_names.push_back(sym.name);
          }
          continue;
        }

        if (rela.rela_type == 9 /* GOTPCREL */ ||
            rela.rela_type == 41 /* GOTPCRELX */ ||
            rela.rela_type == 42 /* REX_GOTPCRELX */) {
          if (!contains_name(got_only_names, sym.name)) {
            got_only_names.push_back(sym.name);
          }
          continue;
        }

        if (rela.rela_type == 22 /* GOTTPOFF */) {
          if (!contains_name(got_only_names, sym.name) && !contains_name(plt_names, sym.name)) {
            got_only_names.push_back(sym.name);
          }
          continue;
        }

        if (is_dynamic) {
          if (sym_type != 1 /* STT_OBJECT */ && rela.rela_type == 1 /* R_X86_64_64 */) {
            if (!contains_name(plt_names, sym.name)) {
              plt_names.push_back(sym.name);
            }
          } else if (!contains_name(plt_names, sym.name) && !contains_name(got_only_names, sym.name)) {
            got_only_names.push_back(sym.name);
          }
        }
      }
    }
  }

  std::vector<std::pair<std::string, std::uint64_t>> copy_reloc_lib_addrs;
  for (const auto& name : copy_reloc_names) {
    auto it = globals->find(name);
    if (it == globals->end()) continue;
    it->second.copy_reloc = true;
    if (it->second.from_lib.has_value() && (it->second.info & 0x0f) == 1 && it->second.lib_sym_value != 0) {
      const auto key = std::make_pair(*it->second.from_lib, it->second.lib_sym_value);
      if (std::find(copy_reloc_lib_addrs.begin(), copy_reloc_lib_addrs.end(), key) ==
          copy_reloc_lib_addrs.end()) {
        copy_reloc_lib_addrs.push_back(key);
      }
    }
  }

  if (!copy_reloc_lib_addrs.empty()) {
    std::vector<std::string> alias_names;
    for (const auto& [name, gsym] : *globals) {
      if (gsym.is_dynamic && !gsym.copy_reloc && (gsym.info & 0x0f) == 1 &&
          std::find(copy_reloc_names.begin(), copy_reloc_names.end(), name) == copy_reloc_names.end() &&
          gsym.from_lib.has_value() && gsym.lib_sym_value != 0) {
        const auto key = std::make_pair(*gsym.from_lib, gsym.lib_sym_value);
        if (std::find(copy_reloc_lib_addrs.begin(), copy_reloc_lib_addrs.end(), key) !=
            copy_reloc_lib_addrs.end()) {
          alias_names.push_back(name);
        }
      }
    }
    for (const auto& name : alias_names) {
      (*globals)[name].copy_reloc = true;
    }
  }

  std::vector<std::pair<std::string, bool>> got_entries;
  got_entries.emplace_back(std::string(), false);
  got_entries.emplace_back(std::string(), false);
  got_entries.emplace_back(std::string(), false);

  for (std::size_t plt_idx = 0; plt_idx < plt_names.size(); ++plt_idx) {
    const std::size_t got_idx = got_entries.size();
    got_entries.emplace_back(plt_names[plt_idx], true);
    if (auto it = globals->find(plt_names[plt_idx]); it != globals->end()) {
      it->second.plt_idx = plt_idx;
      it->second.got_idx = got_idx;
    }
  }

  for (const auto& name : got_only_names) {
    const std::size_t got_idx = got_entries.size();
    got_entries.emplace_back(name, false);
    if (auto it = globals->find(name); it != globals->end()) {
      it->second.got_idx = got_idx;
    }
  }

  return {plt_names, got_entries};
}

}  // namespace c4c::backend::x86::linker

