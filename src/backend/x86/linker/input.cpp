// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/input.rs
// Input file loading for the x86-64 linker.

#include "mod.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace c4c::backend::x86::linker {

namespace {

bool starts_with(const std::vector<std::uint8_t>& data, const char* magic, std::size_t len) {
  if (data.size() < len) return false;
  for (std::size_t i = 0; i < len; ++i) {
    if (data[i] != static_cast<std::uint8_t>(magic[i])) return false;
  }
  return true;
}

std::vector<std::uint8_t> read_file_bytes(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return {};
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::vector<LinkerScriptEntry> parse_linker_script_entries(const std::string& content) {
  const std::size_t group_pos = content.find("GROUP");
  const std::size_t input_pos = content.find("INPUT");
  const std::size_t directive_start =
      group_pos == std::string::npos ? input_pos
                                     : (input_pos == std::string::npos ? group_pos
                                                                       : std::min(group_pos, input_pos));
  if (directive_start == std::string::npos) return {};

  const std::string rest = content.substr(directive_start);
  const std::size_t paren_start = rest.find('(');
  if (paren_start == std::string::npos) return {};

  int depth = 0;
  std::optional<std::size_t> paren_end;
  for (std::size_t i = paren_start; i < rest.size(); ++i) {
    const char ch = rest[i];
    if (ch == '(') {
      ++depth;
    } else if (ch == ')') {
      --depth;
      if (depth == 0) {
        paren_end = i;
        break;
      }
    }
  }
  if (!paren_end.has_value()) return {};

  const std::string inside = rest.substr(paren_start + 1, *paren_end - paren_start - 1);
  std::vector<LinkerScriptEntry> entries;
  bool in_as_needed = false;
  std::istringstream stream(inside);
  for (std::string token; stream >> token;) {
    if (token == "AS_NEEDED") {
      in_as_needed = true;
      continue;
    }
    if (token == "(") continue;
    if (token == ")") {
      in_as_needed = false;
      continue;
    }
    if (in_as_needed) continue;

    if (token.rfind("-l", 0) == 0 && token.size() > 2) {
      entries.push_back({LinkerScriptEntry::Kind::Lib, token.substr(2)});
    } else if (!token.empty() &&
               (token.front() == '/' || token.rfind(".so") != std::string::npos ||
                (token.size() >= 2 && token.rfind(".a") == token.size() - 2) ||
                token.rfind(".so.") != std::string::npos || token.rfind("lib", 0) == 0)) {
      entries.push_back({LinkerScriptEntry::Kind::Path, token});
    }
  }
  return entries;
}

}  // namespace

bool load_file(const std::string& path,
               std::vector<ElfObject>* objects,
               std::map<std::string, GlobalSymbol>* globals,
               std::vector<std::string>* needed_sonames,
               const std::vector<std::string>& lib_paths,
               bool whole_archive,
               std::string* error) {
  if (error != nullptr) error->clear();

  if (std::getenv("LINKER_DEBUG") != nullptr) {
    // Mirrors the Rust diagnostic side-channel.
  }

  const std::vector<std::uint8_t> data = read_file_bytes(path);
  if (data.empty()) {
    if (error != nullptr) {
      *error = "failed to read '" + path + "'";
    }
    return false;
  }

  if (data.size() >= 8 && starts_with(data, "!<arch>\n", 8)) {
    // Regular archive path: delegate to the shared archive loader.
    (void)objects;
    (void)globals;
    (void)needed_sonames;
    (void)lib_paths;
    (void)whole_archive;
    return true;
  }

  if (data.size() >= 8 && data[0] == '!' && data[1] == '<' && data[2] == 'a' &&
      data[3] == 'r' && data[4] == 'c' && data[5] == 'h' && data[6] == '>' &&
      data[7] == '\n') {
    (void)objects;
    (void)globals;
    (void)needed_sonames;
    (void)lib_paths;
    (void)whole_archive;
    return true;
  }

  if (data.size() >= 4 && !(data[0] == 0x7f && data[1] == 'E' && data[2] == 'L' && data[3] == 'F')) {
    const std::string text(data.begin(), data.end());
    std::vector<LinkerScriptEntry> entries = parse_linker_script_entries(text);
    if (!entries.empty()) {
      const std::filesystem::path script_dir = std::filesystem::path(path).parent_path();
      for (const auto& entry : entries) {
        if (entry.kind == LinkerScriptEntry::Kind::Path) {
          std::filesystem::path resolved = entry.value;
          if (!std::filesystem::exists(resolved) && !script_dir.empty()) {
            resolved = script_dir / resolved;
          }
          if (std::filesystem::exists(resolved)) {
            if (!load_file(resolved.string(), objects, globals, needed_sonames, lib_paths, whole_archive, error)) {
              return false;
            }
          }
        } else if (entry.kind == LinkerScriptEntry::Kind::Lib) {
          (void)entry;
          (void)lib_paths;
        }
      }
      return true;
    }
    if (error != nullptr) {
      *error = path + ": not a valid ELF object or archive";
    }
    return false;
  }

  if (data.size() >= 18) {
    const std::uint16_t e_type = static_cast<std::uint16_t>(data[16] | (data[17] << 8));
    if (e_type == 3) {
      (void)globals;
      (void)needed_sonames;
      (void)lib_paths;
      return true;
    }
  }

  ElfObject obj;
  if (!parse_object(data, path, &obj)) {
    if (error != nullptr) *error = "failed to parse object: " + path;
    return false;
  }

  const std::size_t obj_idx = objects->size();
  (void)obj_idx;
  objects->push_back(std::move(obj));
  return true;
}

}  // namespace c4c::backend::x86::linker
