// Translated from ref/claudes-c-compiler/src/backend/riscv/linker/link.rs
// Structural mirror of the orchestration layer; concrete helpers are provided
// where they are self-contained in this translation unit.

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::linker {
namespace {

bool contains(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

void append_unique(std::vector<std::string>* values, const std::string& value) {
  if (values == nullptr) return;
  if (!contains(*values, value)) values->push_back(value);
}

std::vector<std::string> to_strings(const std::vector<const char*>& values) {
  std::vector<std::string> out;
  out.reserve(values.size());
  for (const char* value : values) {
    out.emplace_back(value != nullptr ? value : "");
  }
  return out;
}

bool has_suffix(std::string_view value, std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.substr(value.size() - suffix.size()) == suffix;
}

std::vector<std::string> split_csv(std::string_view text) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t comma = text.find(',', start);
    const std::size_t count =
        comma == std::string_view::npos ? std::string_view::npos : comma - start;
    parts.emplace_back(text.substr(start, count));
    if (comma == std::string_view::npos) break;
    start = comma + 1;
  }
  return parts;
}

std::optional<std::string> resolve_library_candidate(
    const std::string& lib_name,
    const std::vector<std::string>& lib_paths,
    bool is_static) {
  const std::vector<std::string> suffixes = is_static ? std::vector<std::string>{".a"}
                                                      : std::vector<std::string>{".a", ".so"};
  for (const auto& dir : lib_paths) {
    for (const auto& suffix : suffixes) {
      const std::filesystem::path candidate =
          std::filesystem::path(dir) / ("lib" + lib_name + suffix);
      if (std::filesystem::exists(candidate)) {
        return candidate.string();
      }
    }
  }
  return std::nullopt;
}

struct ParsedLinkArgs {
  bool is_static = false;
  bool use_runpath = false;
  bool whole_archive = false;
  std::optional<std::string> soname;
  std::vector<std::string> extra_lib_paths;
  std::vector<std::string> libs_to_load;
  std::vector<std::string> extra_object_files;
  std::vector<std::string> rpath_entries;
  std::vector<std::pair<std::string, std::string>> defsym_defs;
};

ParsedLinkArgs parse_linker_args(const std::vector<std::string>& user_args) {
  ParsedLinkArgs parsed;
  bool pending_rpath = false;
  bool pending_soname = false;

  for (std::size_t i = 0; i < user_args.size(); ++i) {
    const std::string& arg = user_args[i];

    if (arg == "-static") {
      parsed.is_static = true;
      continue;
    }

    if (arg == "-shared" || arg == "-nostdlib" || arg == "-o") {
      if (arg == "-o" && i + 1 < user_args.size()) ++i;
      continue;
    }

    if (arg.rfind("-L", 0) == 0 && arg.size() > 2) {
      parsed.extra_lib_paths.push_back(arg.substr(2));
      continue;
    }

    if (arg.rfind("-l", 0) == 0 && arg.size() > 2) {
      parsed.libs_to_load.push_back(arg.substr(2));
      continue;
    }

    if (arg.rfind("-defsym=", 0) == 0) {
      const std::string payload = arg.substr(8);
      const std::size_t eq = payload.find('=');
      if (eq != std::string::npos) {
        parsed.defsym_defs.emplace_back(payload.substr(0, eq), payload.substr(eq + 1));
      }
      continue;
    }

    if (arg.rfind("-Wl,", 0) == 0) {
      const std::vector<std::string> parts = split_csv(arg.substr(4));
      if ((pending_rpath || pending_soname) && !parts.empty()) {
        if (pending_rpath) {
          parsed.rpath_entries.push_back(parts[0]);
          pending_rpath = false;
        } else if (pending_soname) {
          parsed.soname = parts[0];
          pending_soname = false;
        }
        continue;
      }

      for (std::size_t j = 0; j < parts.size(); ++j) {
        const std::string& part = parts[j];
        if (part.rfind("-soname=", 0) == 0) {
          parsed.soname = part.substr(8);
        } else if (part == "-soname" && j + 1 < parts.size()) {
          parsed.soname = parts[++j];
        } else if (part == "-soname") {
          pending_soname = true;
        } else if (part.rfind("-rpath=", 0) == 0) {
          parsed.rpath_entries.push_back(part.substr(7));
        } else if (part == "-rpath" && j + 1 < parts.size()) {
          parsed.rpath_entries.push_back(parts[++j]);
        } else if (part == "-rpath") {
          pending_rpath = true;
        } else if (part == "--enable-new-dtags") {
          parsed.use_runpath = true;
        } else if (part == "--disable-new-dtags") {
          parsed.use_runpath = false;
        } else if (part == "--whole-archive") {
          parsed.whole_archive = true;
        } else if (part == "--no-whole-archive") {
          parsed.whole_archive = false;
        } else if (part.rfind("-L", 0) == 0) {
          parsed.extra_lib_paths.push_back(part.substr(2));
        } else if (part.rfind("-l", 0) == 0) {
          parsed.libs_to_load.push_back(part.substr(2));
        } else if (part.rfind("--defsym=", 0) == 0) {
          const std::string payload = part.substr(9);
          const std::size_t eq = payload.find('=');
          if (eq != std::string::npos) {
            parsed.defsym_defs.emplace_back(payload.substr(0, eq), payload.substr(eq + 1));
          }
        }
      }
      continue;
    }

    if (!arg.empty() && arg[0] != '-' && std::filesystem::exists(arg)) {
      parsed.extra_object_files.push_back(arg);
    }
  }

  return parsed;
}

struct BuiltinLinkInputs {
  ParsedLinkArgs parsed_args;
  std::vector<std::string> all_inputs;
  std::vector<std::string> lib_search_paths;
  std::vector<std::string> needed_libs;
};

BuiltinLinkInputs build_builtin_link_inputs(
    const std::vector<const char*>& object_files,
    const std::vector<std::string>& user_args,
    const std::vector<const char*>& lib_paths,
    const std::vector<const char*>& needed_libs,
    const std::vector<const char*>& crt_objects_before,
    const std::vector<const char*>& crt_objects_after) {
  BuiltinLinkInputs inputs;
  inputs.parsed_args = parse_linker_args(user_args);

  inputs.all_inputs.reserve(crt_objects_before.size() + object_files.size() +
                            inputs.parsed_args.extra_object_files.size() +
                            crt_objects_after.size());
  for (const char* crt : crt_objects_before) {
    if (crt != nullptr) inputs.all_inputs.emplace_back(crt);
  }
  for (const char* obj : object_files) {
    if (obj != nullptr) inputs.all_inputs.emplace_back(obj);
  }
  inputs.all_inputs.insert(inputs.all_inputs.end(), inputs.parsed_args.extra_object_files.begin(),
                           inputs.parsed_args.extra_object_files.end());
  for (const char* crt : crt_objects_after) {
    if (crt != nullptr) inputs.all_inputs.emplace_back(crt);
  }

  inputs.lib_search_paths = to_strings(lib_paths);
  for (const std::string& path : inputs.parsed_args.extra_lib_paths) {
    append_unique(&inputs.lib_search_paths, path);
  }

  inputs.needed_libs = to_strings(needed_libs);
  inputs.needed_libs.insert(inputs.needed_libs.end(), inputs.parsed_args.libs_to_load.begin(),
                            inputs.parsed_args.libs_to_load.end());
  return inputs;
}

struct SharedLinkInputs {
  ParsedLinkArgs parsed_args;
  std::vector<std::string> object_inputs;
  std::vector<std::string> lib_search_paths;
};

SharedLinkInputs build_shared_link_inputs(const std::vector<const char*>& object_files,
                                          const std::vector<std::string>& user_args,
                                          const std::vector<const char*>& lib_paths) {
  SharedLinkInputs inputs;
  inputs.parsed_args = parse_linker_args(user_args);
  inputs.object_inputs = to_strings(object_files);
  inputs.lib_search_paths = to_strings(lib_paths);
  for (const std::string& path : inputs.parsed_args.extra_lib_paths) {
    append_unique(&inputs.lib_search_paths, path);
  }
  return inputs;
}

}  // namespace

// The Rust file continues into phases that depend on translated input/section/
// symbol/emit modules. Those C++ surfaces are not yet present in this subtree,
// so the orchestration entry points are preserved here as translation notes.
//
// pub fn link_builtin(...) -> Result<(), String> {
//   Phase 0: parse linker args and collect all inputs
//   Phase 1: load inputs, discover shared libs, resolve archives
//   Phase 2: merge sections
//   Phase 3: build global symbol table and apply defsym aliases
//   Phase 4+: emit executable
// }
//
// pub fn link_shared(...) -> Result<(), String> {
//   Phase 0: parse linker args, object files, and shared-lib specific flags
//   Phase 1: load shared-lib inputs and dependencies
//   Phase 2: merge sections
//   Phase 3: build global symbol table and collect GOT entries
//   Phase 4+: emit shared library
// }

[[maybe_unused]] static std::optional<std::string> riscv_linker_resolve_library_for_plan(
    const std::string& lib_name,
    const std::vector<std::string>& lib_paths,
    bool is_static) {
  return resolve_library_candidate(lib_name, lib_paths, is_static);
}

}  // namespace c4c::backend::riscv::linker
