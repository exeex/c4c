// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/link.rs
// x86-64 linker orchestration.

#include "mod.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::x86::linker {

namespace {

bool contains(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

std::string resolve_lib_stub(const std::string& lib_name,
                             const std::vector<std::string>& lib_paths,
                             bool /*is_static*/) {
  const std::vector<std::string> prefixes = {"lib"};
  const std::vector<std::string> suffixes = {".a", ".so"};
  for (const auto& dir : lib_paths) {
    for (const auto& prefix : prefixes) {
      for (const auto& suffix : suffixes) {
        const std::filesystem::path candidate =
            std::filesystem::path(dir) / (prefix + lib_name + suffix);
        if (std::filesystem::exists(candidate)) {
          return candidate.string();
        }
      }
    }
  }
  return {};
}

std::vector<std::string> to_strings(const std::vector<const char*>& values) {
  std::vector<std::string> out;
  out.reserve(values.size());
  for (const char* value : values) {
    out.emplace_back(value != nullptr ? value : "");
  }
  return out;
}

void add_unique(std::vector<std::string>* values, const std::string& value) {
  if (!contains(*values, value)) values->push_back(value);
}

}  // namespace

bool link_builtin(const std::vector<const char*>& object_files,
                  const std::string& output_path,
                  const std::vector<std::string>& user_args,
                  const std::vector<const char*>& lib_paths,
                  const std::vector<const char*>& needed_libs,
                  const std::vector<const char*>& crt_objects_before,
                  const std::vector<const char*>& crt_objects_after,
                  std::string* error) {
  if (error != nullptr) error->clear();

  const bool is_static = std::find(user_args.begin(), user_args.end(), "-static") != user_args.end();
  std::vector<ElfObject> objects;
  std::map<std::string, GlobalSymbol> globals;
  std::vector<std::string> needed_sonames;
  const std::vector<std::string> lib_path_strings = to_strings(lib_paths);

  for (const auto* path : crt_objects_before) {
    if (path != nullptr && std::filesystem::exists(path)) {
      if (!load_file(path, &objects, &globals, &needed_sonames, lib_path_strings, false, error)) {
        return false;
      }
    }
  }

  for (const auto* path : object_files) {
    if (!load_file(path, &objects, &globals, &needed_sonames, lib_path_strings, false, error)) {
      return false;
    }
  }

  std::vector<std::string> extra_lib_paths;
  std::vector<std::string> libs_to_load;
  std::vector<std::string> extra_object_files;
  std::vector<std::string> rpath_entries;
  std::vector<std::pair<std::string, std::string>> defsym_defs;
  bool export_dynamic = false;
  bool use_runpath = false;
  bool gc_sections = false;

  // Keep the shape of the Rust parser-driven argument handling visible even
  // though the shared linker-arg parser lives elsewhere in this repository.
  for (const auto& arg : user_args) {
    if (arg == "-export-dynamic") export_dynamic = true;
    if (arg == "--enable-new-dtags") use_runpath = true;
    if (arg == "--gc-sections") gc_sections = true;
    if (arg.rfind("-L", 0) == 0 && arg.size() > 2) extra_lib_paths.push_back(arg.substr(2));
    if (arg.rfind("-l", 0) == 0 && arg.size() > 2) libs_to_load.push_back(arg.substr(2));
    if (arg.size() > 3 && arg.substr(arg.size() - 2) == ".o") extra_object_files.push_back(arg);
  }

  std::vector<std::string> deferred_libs;
  for (const auto& path : extra_object_files) {
    if (path.size() >= 2 && (path.substr(path.size() - 2) == ".a" || path.find(".so") != std::string::npos)) {
      deferred_libs.push_back(path);
    } else {
      if (!load_file(path, &objects, &globals, &needed_sonames, lib_path_strings, false, error)) {
        return false;
      }
    }
  }

  std::vector<std::string> all_lib_paths = extra_lib_paths;
  all_lib_paths.insert(all_lib_paths.end(), lib_path_strings.begin(), lib_path_strings.end());

  for (const auto* path : crt_objects_after) {
    if (path != nullptr && std::filesystem::exists(path)) {
      if (!load_file(path, &objects, &globals, &needed_sonames, all_lib_paths, false, error)) {
        return false;
      }
    }
  }

  {
    std::vector<std::string> all_lib_names;
    for (const auto* lib : needed_libs) all_lib_names.emplace_back(lib != nullptr ? lib : "");
    all_lib_names.insert(all_lib_names.end(), libs_to_load.begin(), libs_to_load.end());

    std::vector<std::string> lib_paths_resolved;
    for (const auto& lib_path : deferred_libs) {
      add_unique(&lib_paths_resolved, lib_path);
    }
    const std::size_t needed_lib_count = needed_libs.size();
    for (std::size_t idx = 0; idx < all_lib_names.size(); ++idx) {
      const std::string resolved = resolve_lib_stub(all_lib_names[idx], all_lib_paths, is_static);
      if (!resolved.empty()) {
        add_unique(&lib_paths_resolved, resolved);
      } else if (idx >= needed_lib_count) {
        if (error != nullptr) {
          *error = "cannot find -l" + all_lib_names[idx] + ": No such file or directory";
        }
        return false;
      }
    }

    bool changed = true;
    while (changed) {
      changed = false;
      const std::size_t prev_obj_count = objects.size();
      const std::size_t prev_dyn_count = needed_sonames.size();
      for (const auto& lib_path : lib_paths_resolved) {
        if (!load_file(lib_path, &objects, &globals, &needed_sonames, all_lib_paths, false, error)) {
          return false;
        }
      }
      if (objects.size() != prev_obj_count || needed_sonames.size() != prev_dyn_count) {
        changed = true;
      }
    }
  }

  if (!is_static) {
    const std::vector<std::string> default_libs = {
        "libc.so.6", "libm.so.6", "libgcc_s.so.1", "ld-linux-x86-64.so.2"};
    (void)default_libs;
  }

  for (const auto& [alias, target] : defsym_defs) {
    auto it = globals.find(target);
    if (it != globals.end()) {
      globals[alias] = it->second;
    }
  }

  std::set<std::pair<std::size_t, std::size_t>> dead_sections;
  if (gc_sections) {
    // The Rust implementation delegates to linker_common::gc_collect_sections_elf64.
    // We keep the control-flow shape here and allow the dead-section set to stay empty.
  }

  if (gc_sections) {
    std::set<std::string> referenced_from_live;
    for (std::size_t obj_idx = 0; obj_idx < objects.size(); ++obj_idx) {
      const auto& obj = objects[obj_idx];
      for (std::size_t sec_idx = 0; sec_idx < obj.relocations.size(); ++sec_idx) {
        if (dead_sections.contains({obj_idx, sec_idx})) continue;
        for (const auto& rela : obj.relocations[sec_idx]) {
          if (rela.sym_idx < obj.symbols.size()) {
            const auto& sym = obj.symbols[rela.sym_idx];
            if (!sym.name.empty()) referenced_from_live.insert(sym.name);
          }
        }
      }
    }

    std::vector<std::string> keep_names;
    for (const auto& [name, sym] : globals) {
      const bool keep = sym.defined_in.has_value() || sym.is_dynamic || ((sym.info >> 4) == 2) ||
                        referenced_from_live.contains(name);
      if (keep) keep_names.push_back(name);
    }
    std::map<std::string, GlobalSymbol> filtered;
    for (const auto& name : keep_names) filtered[name] = globals[name];
    globals.swap(filtered);
  }

  std::vector<OutputSection> output_sections;
  SectionMap section_map;

  std::vector<std::string> plt_names;
  std::vector<std::pair<std::string, bool>> got_entries;
  std::tie(plt_names, got_entries) = create_plt_got(objects, &globals);

  const std::vector<std::string> ifunc_symbols = collect_ifunc_symbols(globals, is_static);

  return emit_executable(objects,
                         &globals,
                         &output_sections,
                         section_map,
                         plt_names,
                         got_entries,
                         needed_sonames,
                         output_path,
                         export_dynamic,
                         rpath_entries,
                         use_runpath,
                         is_static,
                         ifunc_symbols,
                         error);
}

bool link_shared(const std::vector<const char*>& object_files,
                 const std::string& output_path,
                 const std::vector<std::string>& user_args,
                 const std::vector<const char*>& lib_paths,
                 const std::vector<const char*>& needed_libs,
                 std::string* error) {
  if (error != nullptr) error->clear();

  std::vector<ElfObject> objects;
  std::map<std::string, GlobalSymbol> globals;
  std::vector<std::string> needed_sonames;
  const std::vector<std::string> lib_path_strings = to_strings(lib_paths);

  std::vector<std::string> extra_lib_paths;
  std::optional<std::string> soname;
  std::vector<std::string> rpath_entries;
  bool use_runpath = false;
  bool pending_rpath = false;
  bool pending_soname = false;
  bool whole_archive = false;

  std::vector<std::pair<std::string, bool>> ordered_items;

  for (std::size_t i = 0; i < user_args.size(); ++i) {
    const std::string& arg = user_args[i];
    if (arg.rfind("-L", 0) == 0) {
      extra_lib_paths.push_back(arg.size() > 2 ? arg.substr(2) : "");
    } else if (arg.rfind("-l", 0) == 0) {
      ordered_items.emplace_back(arg.size() > 2 ? arg.substr(2) : "", true);
    } else if (arg.rfind("-Wl,", 0) == 0) {
      const std::string wl_arg = arg.substr(4);
      std::vector<std::string> parts;
      std::size_t start = 0;
      while (start <= wl_arg.size()) {
        const std::size_t comma = wl_arg.find(',', start);
        parts.push_back(wl_arg.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (comma == std::string::npos) break;
        start = comma + 1;
      }
      if ((pending_rpath || pending_soname) && !parts.empty()) {
        if (pending_rpath) {
          rpath_entries.push_back(parts[0]);
          pending_rpath = false;
        } else if (pending_soname) {
          soname = parts[0];
          pending_soname = false;
        }
        continue;
      }
      for (std::size_t j = 0; j < parts.size(); ++j) {
        const std::string& part = parts[j];
        if (part.rfind("-soname=", 0) == 0) {
          soname = part.substr(8);
        } else if (part == "-soname" && j + 1 < parts.size()) {
          soname = parts[++j];
        } else if (part == "-soname") {
          pending_soname = true;
        } else if (part.rfind("-rpath=", 0) == 0) {
          rpath_entries.push_back(part.substr(7));
        } else if (part == "-rpath" && j + 1 < parts.size()) {
          rpath_entries.push_back(parts[++j]);
        } else if (part == "-rpath") {
          pending_rpath = true;
        } else if (part == "--enable-new-dtags") {
          use_runpath = true;
        } else if (part == "--disable-new-dtags") {
          use_runpath = false;
        } else if (part == "--whole-archive") {
          whole_archive = true;
        } else if (part == "--no-whole-archive") {
          whole_archive = false;
        } else if (part.rfind("-L", 0) == 0) {
          extra_lib_paths.push_back(part.substr(2));
        } else if (part.rfind("-l", 0) == 0) {
          ordered_items.emplace_back(part.substr(2), true);
        }
      }
    } else if (arg == "-shared" || arg == "-nostdlib" || arg == "-o") {
      if (arg == "-o" && i + 1 < user_args.size()) ++i;
    } else if (!arg.empty() && arg[0] != '-' && std::filesystem::exists(arg)) {
      ordered_items.emplace_back(arg, false);
    }
  }

  for (const auto* path : object_files) {
    if (!load_file(path, &objects, &globals, &needed_sonames, lib_path_strings, false, error)) {
      return false;
    }
  }

  std::vector<std::string> all_lib_paths = extra_lib_paths;
  all_lib_paths.insert(all_lib_paths.end(), lib_path_strings.begin(), lib_path_strings.end());

  std::vector<std::pair<std::string, bool>> libs_to_load_later;
  for (const auto& [item, is_lib] : ordered_items) {
    if (is_lib) {
      libs_to_load_later.emplace_back(item, true);
    } else {
      if (!load_file(item, &objects, &globals, &needed_sonames, all_lib_paths, whole_archive, error)) {
        return false;
      }
    }
  }

  if (!libs_to_load_later.empty()) {
    std::vector<std::pair<std::string, bool>> lib_paths_resolved;
    for (const auto& [lib_name, wa] : libs_to_load_later) {
      const std::string resolved = resolve_lib_stub(lib_name, all_lib_paths, false);
      if (!resolved.empty()) {
        lib_paths_resolved.emplace_back(resolved, wa);
      } else {
        if (error != nullptr) {
          *error = "cannot find -l" + lib_name + ": No such file or directory";
        }
        return false;
      }
    }

    std::set<std::string> whole_archive_loaded;
    bool changed = true;
    while (changed) {
      changed = false;
      const std::size_t prev_count = objects.size();
      for (const auto& [lib_path, wa] : lib_paths_resolved) {
        if (wa && whole_archive_loaded.contains(lib_path)) continue;
        if (!load_file(lib_path, &objects, &globals, &needed_sonames, all_lib_paths, wa, error)) {
          return false;
        }
        if (wa) whole_archive_loaded.insert(lib_path);
      }
      if (objects.size() != prev_count) changed = true;
    }
  }

  if (!needed_libs.empty()) {
    std::vector<std::string> implicit_paths;
    for (const auto* lib_name : needed_libs) {
      const std::string resolved = resolve_lib_stub(lib_name != nullptr ? lib_name : "", all_lib_paths, false);
      if (!resolved.empty() && !contains(implicit_paths, resolved)) {
        implicit_paths.push_back(resolved);
      }
    }
    bool changed = true;
    while (changed) {
      changed = false;
      const std::size_t prev_count = objects.size();
      for (const auto& lib_path : implicit_paths) {
        if (!load_file(lib_path, &objects, &globals, &needed_sonames, all_lib_paths, false, error)) {
          return false;
        }
      }
      if (objects.size() != prev_count) changed = true;
    }
  }

  const std::vector<std::string> default_libs = {
      "libc.so.6", "libm.so.6", "libgcc_s.so.1", "ld-linux-x86-64.so.2"};
  (void)default_libs;

  std::vector<OutputSection> output_sections;
  SectionMap section_map;
  if (!emit_shared_library(objects,
                           &globals,
                           &output_sections,
                           section_map,
                           needed_sonames,
                           output_path,
                           soname,
                           rpath_entries,
                           use_runpath,
                           error)) {
    return false;
  }

  return true;
}

}  // namespace c4c::backend::x86::linker
