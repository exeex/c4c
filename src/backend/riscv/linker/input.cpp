// Translated from ref/claudes-c-compiler/src/backend/riscv/linker/input.rs
// Self-contained enough to validate in isolation while the shared RISC-V
// linker surface is still being translated.

#include "../../elf/mod.hpp"
#include "../../linker_common/mod.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::linker {
namespace {

using ElfObject = linker_common::Elf64Object;
using Symbol = linker_common::Elf64Symbol;

struct DynSymbol {
  std::string name;
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t info = 0;
  std::optional<std::string> version;
  std::optional<std::string> from_lib;
};

struct LoadedInputObject {
  std::string path;
  ElfObject object;
};

static constexpr std::uint16_t kEmRiscv = 243;
static constexpr std::uint16_t kEtDyn = 3;

bool starts_with_bytes(const std::vector<std::uint8_t>& data,
                       std::string_view magic) {
  return data.size() >= magic.size() &&
         std::equal(magic.begin(), magic.end(), data.begin());
}

bool is_elf_file(const std::vector<std::uint8_t>& data) {
  return starts_with_bytes(data, std::string_view("\x7f" "ELF", 4));
}

bool is_archive_file(const std::vector<std::uint8_t>& data) {
  return starts_with_bytes(data, "!<arch>\n");
}

bool is_thin_archive(const std::vector<std::uint8_t>& data) {
  return starts_with_bytes(data, "!<thin>\n");
}

std::vector<std::uint8_t> read_file_bytes(const std::string& path, std::string* error) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    if (error != nullptr) *error = "Cannot read " + path + ": failed to open file";
    return {};
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>());
}

std::string trim_right_spaces(std::string_view value) {
  std::size_t end = value.size();
  while (end > 0 && value[end - 1] == ' ') {
    --end;
  }
  return std::string(value.substr(0, end));
}

std::vector<std::string> parse_thin_archive_members(const std::vector<std::uint8_t>& data,
                                                    std::string* error) {
  if (error != nullptr) error->clear();
  if (!is_thin_archive(data)) {
    if (error != nullptr) *error = "not a thin archive file";
    return {};
  }

  std::vector<std::string> members;
  std::size_t pos = 8;
  std::optional<std::vector<std::uint8_t>> extended_names;

  while (pos + 60 <= data.size()) {
    const std::string name_raw(
        data.begin() + static_cast<std::ptrdiff_t>(pos),
        data.begin() + static_cast<std::ptrdiff_t>(pos + 16));
    const std::string size_text(
        data.begin() + static_cast<std::ptrdiff_t>(pos + 48),
        data.begin() + static_cast<std::ptrdiff_t>(pos + 58));
    if (data[pos + 58] != '`' || data[pos + 59] != '\n') {
      break;
    }

    std::size_t member_size = 0;
    try {
      member_size = static_cast<std::size_t>(std::stoull(trim_right_spaces(size_text)));
    } catch (const std::exception&) {
      member_size = 0;
    }

    const std::size_t data_start = pos + 60;
    std::string name_str = trim_right_spaces(name_raw);

    if (name_str == "/" || name_str == "/SYM64/") {
      pos = data_start + member_size;
      if ((pos & 1u) != 0u) ++pos;
      continue;
    }
    if (name_str == "//") {
      const std::size_t data_end = std::min(data_start + member_size, data.size());
      extended_names.emplace(data.begin() + static_cast<std::ptrdiff_t>(data_start),
                             data.begin() + static_cast<std::ptrdiff_t>(data_end));
      pos = data_start + member_size;
      if ((pos & 1u) != 0u) ++pos;
      continue;
    }

    if (!name_str.empty() && name_str.front() == '/') {
      const std::string rest = name_str.substr(1);
      const std::string digits = trim_right_spaces(rest);
      if (!digits.empty() && std::all_of(digits.begin(), digits.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
          }) && extended_names.has_value()) {
        const std::size_t name_off = static_cast<std::size_t>(std::stoull(digits));
        if (name_off < extended_names->size()) {
          const auto& ext = *extended_names;
          const auto begin = ext.begin() + static_cast<std::ptrdiff_t>(name_off);
          const auto end = std::find_if(begin, ext.end(), [](std::uint8_t ch) {
            return ch == '/' || ch == '\n' || ch == 0;
          });
          name_str = std::string(begin, end);
        }
      }
    } else if (!name_str.empty() && name_str.back() == '/') {
      name_str.pop_back();
    }

    members.push_back(std::move(name_str));
    pos = data_start + member_size;
    if ((pos & 1u) != 0u) ++pos;
  }

  return members;
}

std::optional<std::vector<std::pair<std::string, ElfObject>>> parse_archive(
    const std::vector<std::uint8_t>& data,
    const std::string& source_name,
    std::string* error) {
  if (error != nullptr) error->clear();

  std::string archive_error;
  const auto parsed_archive =
      linker_common::parse_elf64_archive(data, source_name, kEmRiscv, &archive_error);
  if (!parsed_archive.has_value()) {
    if (error != nullptr) *error = archive_error;
    return std::nullopt;
  }

  std::vector<std::pair<std::string, ElfObject>> objects;
  for (const auto& member : parsed_archive->members) {
    if (!member.object.has_value()) continue;
    objects.emplace_back(member.name, *member.object);
  }
  return objects;
}

std::optional<std::vector<std::pair<std::string, ElfObject>>> parse_thin_archive(
    const std::vector<std::uint8_t>& data,
    const std::string& archive_path,
    std::string* error) {
  if (error != nullptr) error->clear();

  std::string member_error;
  const std::vector<std::string> members = parse_thin_archive_members(data, &member_error);
  if (!member_error.empty()) {
    if (error != nullptr) *error = member_error;
    return std::nullopt;
  }

  const std::filesystem::path archive_dir =
      std::filesystem::path(archive_path).parent_path().empty()
          ? std::filesystem::path(".")
          : std::filesystem::path(archive_path).parent_path();

  std::vector<std::pair<std::string, ElfObject>> objects;
  for (const std::string& member_name : members) {
    const std::filesystem::path member_path = archive_dir / member_name;
    std::string read_error;
    const std::vector<std::uint8_t> member_data =
        read_file_bytes(member_path.string(), &read_error);
    if (!read_error.empty()) {
      if (error != nullptr) {
        *error = "thin archive " + archive_path + ": failed to read member '" +
                 member_path.string() + "': " + read_error;
      }
      return std::nullopt;
    }

    if (!is_elf_file(member_data)) continue;

    std::string parse_error;
    auto parsed = linker_common::parse_elf64_object(member_data, member_path.string(), kEmRiscv,
                                                    &parse_error);
    if (!parsed.has_value()) {
      if (error != nullptr) *error = parse_error;
      return std::nullopt;
    }
    objects.emplace_back(member_name, std::move(*parsed));
  }

  return objects;
}

std::optional<std::string> find_versioned_soname(const std::string& dir,
                                                 const std::string& libname) {
  const std::string prefix = "lib" + libname + ".so.";
  std::optional<std::string> best;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
    if (ec) break;
    const std::string name = entry.path().filename().string();
    if (!name.starts_with(prefix)) continue;
    if (!best.has_value() || name.size() < best->size()) {
      best = name;
    }
  }
  return best;
}

std::optional<std::string> resolve_lib(const std::string& libname,
                                       const std::vector<std::string>& lib_paths,
                                       bool prefer_static) {
  auto try_candidate = [&](const std::filesystem::path& candidate) -> std::optional<std::string> {
    if (std::filesystem::exists(candidate)) return candidate.string();
    return std::nullopt;
  };

  if (!libname.empty() && libname.front() == ':') {
    const std::string exact = libname.substr(1);
    for (const auto& dir : lib_paths) {
      const std::filesystem::path candidate = std::filesystem::path(dir) / exact;
      if (auto resolved = try_candidate(candidate)) return resolved;
    }
    return std::nullopt;
  }

  const std::vector<std::string> suffixes = prefer_static ? std::vector<std::string>{".a"}
                                                          : std::vector<std::string>{".so", ".a"};
  for (const auto& dir : lib_paths) {
    for (const auto& suffix : suffixes) {
      const std::filesystem::path candidate =
          std::filesystem::path(dir) / ("lib" + libname + suffix);
      if (auto resolved = try_candidate(candidate)) return resolved;
    }
  }
  return std::nullopt;
}

std::vector<DynSymbol> parse_shared_library_symbols(const std::vector<std::uint8_t>& data,
                                                    const std::string& lib_name) {
  (void)data;
  (void)lib_name;
  return {};
}

std::optional<std::string> parse_soname(const std::vector<std::uint8_t>& data) {
  (void)data;
  return std::nullopt;
}

std::vector<DynSymbol> read_shared_lib_symbols(const std::string& path) {
  const std::vector<std::uint8_t> data = read_file_bytes(path, nullptr);
  return parse_shared_library_symbols(data, path);
}

void register_obj_symbols(const ElfObject& obj,
                          std::unordered_set<std::string>* defined_syms,
                          std::unordered_set<std::string>* undefined_syms) {
  if (defined_syms == nullptr || undefined_syms == nullptr) return;
  for (const auto& sym : obj.symbols) {
    if (sym.shndx != elf::SHN_UNDEF && sym.binding() != elf::STB_LOCAL && !sym.name.empty()) {
      defined_syms->insert(sym.name);
      undefined_syms->erase(sym.name);
    }
  }
  for (const auto& sym : obj.symbols) {
    if (sym.shndx == elf::SHN_UNDEF && !sym.name.empty() && sym.binding() != elf::STB_LOCAL &&
        defined_syms->find(sym.name) == defined_syms->end()) {
      undefined_syms->insert(sym.name);
    }
  }
}

void resolve_archive_members(
    const std::vector<std::pair<std::string, ElfObject>>& members,
    std::vector<std::pair<std::string, ElfObject>>* input_objs,
    std::unordered_set<std::string>* defined_syms,
    std::unordered_set<std::string>* undefined_syms) {
  if (input_objs == nullptr || defined_syms == nullptr || undefined_syms == nullptr) return;

  std::vector<std::pair<std::string, ElfObject>> pool = members;
  while (!pool.empty()) {
    bool added_any = false;
    std::vector<std::pair<std::string, ElfObject>> remaining;
    for (auto& [name, obj] : pool) {
      bool needed = false;
      for (const auto& sym : obj.symbols) {
        if (sym.shndx == elf::SHN_UNDEF || sym.binding() == elf::STB_LOCAL || sym.name.empty()) continue;
        if (undefined_syms->find(sym.name) != undefined_syms->end()) {
          needed = true;
          break;
        }
      }
      if (needed) {
        register_obj_symbols(obj, defined_syms, undefined_syms);
        input_objs->push_back({name, std::move(obj)});
        added_any = true;
      } else {
        remaining.push_back({name, std::move(obj)});
      }
    }
    if (!added_any) break;
    pool = std::move(remaining);
  }
}

void process_linker_script(
    const std::vector<std::uint8_t>& data,
    const std::string& dir,
    const std::vector<std::string>& lib_search_paths,
    std::vector<std::pair<std::string, ElfObject>>* input_objs,
    std::unordered_set<std::string>* defined_syms,
    std::unordered_set<std::string>* undefined_syms,
    std::unordered_map<std::string, DynSymbol>* shared_lib_syms) {
  const std::string text(data.begin(), data.end());
  std::istringstream stream(text);
  for (std::string token; stream >> token;) {
    while (!token.empty() && (token.front() == '(' || token.front() == ')' || token.front() == ',')) {
      token.erase(token.begin());
    }
    while (!token.empty() && (token.back() == '(' || token.back() == ')' || token.back() == ',')) {
      token.pop_back();
    }
    if (token.rfind("-l", 0) == 0 && token.size() > 2) {
      const std::string so_name = "lib" + token.substr(2) + ".so";
      for (const auto& search_dir : lib_search_paths) {
        const std::filesystem::path candidate = std::filesystem::path(search_dir) / so_name;
        if (!std::filesystem::exists(candidate)) continue;
        for (const auto& sym : read_shared_lib_symbols(candidate.string())) {
          (*shared_lib_syms)[sym.name] = sym;
        }
        break;
      }
      continue;
    }

    if (token.find(".so") != std::string::npos &&
        (token.starts_with('/') || token.starts_with("lib"))) {
      std::filesystem::path actual_path = token;
      if (!token.starts_with('/')) {
        const std::filesystem::path local_candidate = std::filesystem::path(dir) / token;
        if (std::filesystem::exists(local_candidate)) {
          actual_path = local_candidate;
        } else {
          for (const auto& search_dir : lib_search_paths) {
            const std::filesystem::path candidate = std::filesystem::path(search_dir) / token;
            if (std::filesystem::exists(candidate)) {
              actual_path = candidate;
              break;
            }
          }
        }
      }
      for (const auto& sym : read_shared_lib_symbols(actual_path.string())) {
        (*shared_lib_syms)[sym.name] = sym;
      }
      if (actual_path.string().ends_with("_nonshared.a")) {
        std::string archive_error;
        const std::vector<std::uint8_t> archive_data =
            read_file_bytes(actual_path.string(), &archive_error);
        if (archive_error.empty()) {
          std::optional<std::vector<std::pair<std::string, ElfObject>>> parsed;
          if (is_archive_file(archive_data)) {
            parsed = parse_archive(archive_data, actual_path.string(), nullptr);
          } else if (is_thin_archive(archive_data)) {
            parsed = parse_thin_archive(archive_data, actual_path.string(), nullptr);
          }
          if (parsed.has_value()) {
            resolve_archive_members(*parsed, input_objs, defined_syms, undefined_syms);
          }
        }
      }
    }
  }
}

bool load_archive_file(const std::string& path,
                       const std::vector<std::uint8_t>& data,
                       std::vector<std::pair<std::string, ElfObject>>* input_objs,
                       std::unordered_set<std::string>* defined_syms,
                       std::unordered_set<std::string>* undefined_syms,
                       bool whole_archive,
                       std::string* error) {
  std::optional<std::vector<std::pair<std::string, ElfObject>>> parsed;
  if (is_archive_file(data)) {
    parsed = parse_archive(data, path, error);
  } else if (is_thin_archive(data)) {
    parsed = parse_thin_archive(data, path, error);
  }
  if (!parsed.has_value()) return false;
  if (whole_archive) {
    for (auto& member : *parsed) {
      register_obj_symbols(member.second, defined_syms, undefined_syms);
      input_objs->push_back(std::move(member));
    }
    return true;
  }
  resolve_archive_members(*parsed, input_objs, defined_syms, undefined_syms);
  return true;
}

}  // namespace

bool load_input_files(const std::vector<std::string>& all_inputs,
                      std::vector<std::pair<std::string, ElfObject>>* input_objs,
                      std::vector<std::string>* inline_archive_paths,
                      std::string* error) {
  if (error != nullptr) error->clear();
  if (input_objs == nullptr || inline_archive_paths == nullptr) {
    if (error != nullptr) *error = "missing output containers";
    return false;
  }

  for (const auto& path : all_inputs) {
    if (!std::filesystem::exists(path)) {
      if (error != nullptr) *error = "linker input file not found: " + path;
      return false;
    }
    std::string read_error;
    const std::vector<std::uint8_t> data = read_file_bytes(path, &read_error);
    if (!read_error.empty()) {
      if (error != nullptr) *error = read_error;
      return false;
    }

    if (is_archive_file(data) || is_thin_archive(data)) {
      inline_archive_paths->push_back(path);
      continue;
    }
    if (is_elf_file(data)) {
      std::string parse_error;
      auto obj = linker_common::parse_elf64_object(data, path, kEmRiscv, &parse_error);
      if (!obj.has_value()) {
        if (error != nullptr) *error = path + ": " + parse_error;
        return false;
      }
      input_objs->push_back({path, std::move(*obj)});
    }
  }

  return true;
}

void collect_initial_symbols(const std::vector<std::pair<std::string, ElfObject>>& input_objs,
                             std::unordered_set<std::string>* defined_syms,
                             std::unordered_set<std::string>* undefined_syms) {
  if (defined_syms == nullptr || undefined_syms == nullptr) return;

  for (const auto& [_, obj] : input_objs) {
    for (const auto& sym : obj.symbols) {
      if (sym.shndx != elf::SHN_UNDEF && sym.binding() != elf::STB_LOCAL && !sym.name.empty()) {
        defined_syms->insert(sym.name);
      }
    }
  }
  for (const auto& [_, obj] : input_objs) {
    for (const auto& sym : obj.symbols) {
      if (sym.shndx == elf::SHN_UNDEF && !sym.name.empty() && sym.binding() != elf::STB_LOCAL &&
          defined_syms->find(sym.name) == defined_syms->end()) {
        undefined_syms->insert(sym.name);
      }
    }
  }
}

void discover_shared_lib_symbols(
    const std::vector<std::string>& needed_libs,
    const std::vector<std::string>& lib_search_paths,
    std::vector<std::pair<std::string, ElfObject>>* input_objs,
    std::unordered_set<std::string>* defined_syms,
    std::unordered_set<std::string>* undefined_syms,
    std::unordered_map<std::string, DynSymbol>* shared_lib_syms,
    std::vector<std::string>* actual_needed_libs) {
  if (input_objs == nullptr || defined_syms == nullptr || undefined_syms == nullptr ||
      shared_lib_syms == nullptr || actual_needed_libs == nullptr) {
    return;
  }

  for (const auto& libname : needed_libs) {
    const std::string so_name = libname.starts_with(':') ? libname.substr(1)
                                                         : "lib" + libname + ".so";
    for (const auto& dir : lib_search_paths) {
      const std::filesystem::path path = std::filesystem::path(dir) / so_name;
      if (!std::filesystem::exists(path)) continue;

      std::string read_error;
      const std::vector<std::uint8_t> data = read_file_bytes(path.string(), &read_error);
      if (!read_error.empty()) continue;

      if (starts_with_bytes(data, "/* GNU ld script") ||
          starts_with_bytes(data, "OUTPUT_FORMAT") ||
          starts_with_bytes(data, "GROUP") ||
          starts_with_bytes(data, "INPUT")) {
        process_linker_script(data, dir, lib_search_paths, input_objs, defined_syms, undefined_syms,
                              shared_lib_syms);
      } else if (is_elf_file(data)) {
        for (const auto& sym : read_shared_lib_symbols(path.string())) {
          (*shared_lib_syms)[sym.name] = sym;
        }
      }

      if (const auto versioned = find_versioned_soname(dir, libname)) {
        if (std::find(actual_needed_libs->begin(), actual_needed_libs->end(), *versioned) ==
            actual_needed_libs->end()) {
          actual_needed_libs->push_back(*versioned);
        }
      }
      break;
    }
  }
}

void resolve_archives(const std::vector<std::string>& inline_archive_paths,
                      const std::vector<std::string>& needed_libs,
                      const std::vector<std::string>& lib_search_paths,
                      std::vector<std::pair<std::string, ElfObject>>* input_objs,
                      std::unordered_set<std::string>* defined_syms,
                      std::unordered_set<std::string>* undefined_syms,
                      const std::unordered_map<std::string, DynSymbol>& shared_lib_syms) {
  if (input_objs == nullptr || defined_syms == nullptr || undefined_syms == nullptr) return;

  std::vector<std::string> archive_paths;
  std::unordered_set<std::string> seen;

  for (const auto& path : inline_archive_paths) {
    if (seen.insert(path).second) {
      archive_paths.push_back(path);
    }
  }

  for (const auto& libname : needed_libs) {
    const std::string archive_name = libname.starts_with(':') ? libname.substr(1)
                                                              : "lib" + libname + ".a";
    for (const auto& dir : lib_search_paths) {
      const std::filesystem::path path = std::filesystem::path(dir) / archive_name;
      if (!std::filesystem::exists(path)) continue;
      if (seen.insert(path.string()).second) {
        archive_paths.push_back(path.string());
      }
      break;
    }
  }

  bool group_changed = true;
  while (group_changed) {
    group_changed = false;
    const std::size_t prev_count = input_objs->size();

    for (const auto& sym_name : shared_lib_syms) {
      if (defined_syms->find(sym_name.first) == defined_syms->end()) {
        undefined_syms->erase(sym_name.first);
      }
    }

    for (const auto& path : archive_paths) {
      std::string read_error;
      const std::vector<std::uint8_t> data = read_file_bytes(path, &read_error);
      if (!read_error.empty()) continue;

      std::optional<std::vector<std::pair<std::string, ElfObject>>> parsed;
      if (is_archive_file(data)) {
        parsed = parse_archive(data, path, nullptr);
      } else if (is_thin_archive(data)) {
        parsed = parse_thin_archive(data, path, nullptr);
      }
      if (parsed.has_value()) {
        resolve_archive_members(*parsed, input_objs, defined_syms, undefined_syms);
      }
    }

    if (input_objs->size() != prev_count) {
      group_changed = true;
    }
  }
}

bool load_shared_lib_inputs(const std::vector<std::string>& object_files,
                            const std::vector<std::string>& extra_object_files,
                            std::vector<std::pair<std::string, ElfObject>>* input_objs,
                            std::unordered_set<std::string>* defined_syms,
                            std::unordered_set<std::string>* undefined_syms,
                            std::string* error) {
  if (error != nullptr) error->clear();
  if (input_objs == nullptr || defined_syms == nullptr || undefined_syms == nullptr) {
    if (error != nullptr) *error = "missing output containers";
    return false;
  }

  const auto load = [&](const std::string& path) -> bool {
    std::string read_error;
    const std::vector<std::uint8_t> data = read_file_bytes(path, &read_error);
    if (!read_error.empty()) {
      if (error != nullptr) *error = "failed to read '" + path + "'";
      return false;
    }
    if (data.size() < 4) return true;

    if (is_archive_file(data)) {
      std::string parse_error;
      auto parsed = parse_archive(data, path, &parse_error);
      if (!parsed.has_value()) {
        if (error != nullptr) *error = parse_error;
        return false;
      }
      for (auto& [name, obj] : *parsed) {
        register_obj_symbols(obj, defined_syms, undefined_syms);
        input_objs->push_back({path + "(" + name + ")", std::move(obj)});
      }
    } else if (is_thin_archive(data)) {
      std::string parse_error;
      auto parsed = parse_thin_archive(data, path, &parse_error);
      if (!parsed.has_value()) {
        if (error != nullptr) *error = parse_error;
        return false;
      }
      for (auto& [name, obj] : *parsed) {
        register_obj_symbols(obj, defined_syms, undefined_syms);
        input_objs->push_back({path + "(" + name + ")", std::move(obj)});
      }
    } else if (is_elf_file(data)) {
      std::string parse_error;
      auto obj = linker_common::parse_elf64_object(data, path, kEmRiscv, &parse_error);
      if (!obj.has_value()) {
        if (error != nullptr) *error = parse_error;
        return false;
      }
      register_obj_symbols(*obj, defined_syms, undefined_syms);
      input_objs->push_back({path, std::move(*obj)});
    }
    return true;
  };

  for (const auto& path : object_files) {
    if (!load(path)) return false;
  }
  for (const auto& path : extra_object_files) {
    if (!load(path)) return false;
  }
  return true;
}

bool resolve_shared_lib_deps(const std::vector<std::string>& libs_to_load,
                             const std::vector<std::string>& all_lib_paths,
                             std::vector<std::pair<std::string, ElfObject>>* input_objs,
                             std::unordered_set<std::string>* defined_syms,
                             std::unordered_set<std::string>* undefined_syms,
                             std::vector<std::string>* needed_sonames,
                             std::string* error) {
  if (error != nullptr) error->clear();
  if (input_objs == nullptr || defined_syms == nullptr || undefined_syms == nullptr ||
      needed_sonames == nullptr) {
    if (error != nullptr) *error = "missing output containers";
    return false;
  }

  for (const auto& lib_name : libs_to_load) {
    const auto lib_path = resolve_lib(lib_name, all_lib_paths, false);
    if (!lib_path.has_value()) continue;

    std::string read_error;
    const std::vector<std::uint8_t> data = read_file_bytes(*lib_path, &read_error);
    if (!read_error.empty()) continue;

    if (is_archive_file(data)) {
      std::string parse_error;
      auto parsed = parse_archive(data, *lib_path, &parse_error);
      if (!parsed.has_value()) continue;
      resolve_archive_members(*parsed, input_objs, defined_syms, undefined_syms);
    } else if (is_elf_file(data)) {
      if (data.size() >= 18) {
        const std::uint16_t e_type = static_cast<std::uint16_t>(data[16] | (data[17] << 8));
        if (e_type == kEtDyn) {
          if (const auto soname = parse_soname(data)) {
            if (std::find(needed_sonames->begin(), needed_sonames->end(), *soname) ==
                needed_sonames->end()) {
              needed_sonames->push_back(*soname);
            }
          } else {
            const std::string base =
                std::filesystem::path(*lib_path).filename().string().empty()
                    ? lib_name
                    : std::filesystem::path(*lib_path).filename().string();
            if (std::find(needed_sonames->begin(), needed_sonames->end(), base) ==
                needed_sonames->end()) {
              needed_sonames->push_back(base);
            }
          }
          continue;
        }
      }

      std::string parse_error;
      auto obj = linker_common::parse_elf64_object(data, *lib_path, kEmRiscv, &parse_error);
      if (!obj.has_value()) continue;
      register_obj_symbols(*obj, defined_syms, undefined_syms);
      input_objs->push_back({*lib_path, std::move(*obj)});
    } else if (starts_with_bytes(data, "/* GNU ld script") ||
               starts_with_bytes(data, "OUTPUT_FORMAT") ||
               starts_with_bytes(data, "GROUP") ||
               starts_with_bytes(data, "INPUT")) {
      continue;
    }
  }
  return true;
}

}  // namespace c4c::backend::riscv::linker
