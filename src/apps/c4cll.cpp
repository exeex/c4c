#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <variant>

#include "arena.hpp"
#include "ast.hpp"
#if C4C_ENABLE_BACKEND
#include "backend.hpp"
#endif
#include "hir_to_lir.hpp"
#include "llvm_codegen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"
#include "sema.hpp"
#include "hir.hpp"
#include "hir_printer.hpp"
#include "compile_time_engine.hpp"
#include "inline_expand.hpp"
#include "source_profile.hpp"
#include "target_profile.hpp"
#include "token.hpp"


namespace {

bool has_suffix(std::string_view value, std::string_view suffix) {
  if (value.size() < suffix.size()) return false;
  return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

class ScopedEnvVar {
 public:
  ScopedEnvVar(const char* name, std::optional<std::string_view> value)
      : name_(name),
        had_previous_(std::getenv(name) != nullptr),
        previous_value_(had_previous_ ? std::optional<std::string>(std::getenv(name))
                                      : std::nullopt) {
    if (value.has_value()) {
      setenv(name_, std::string(*value).c_str(), 1);
    } else {
      unsetenv(name_);
    }
  }

  ~ScopedEnvVar() {
    if (had_previous_) {
      setenv(name_, previous_value_->c_str(), 1);
    } else {
      unsetenv(name_);
    }
  }

 private:
  const char* name_;
  bool had_previous_ = false;
  std::optional<std::string> previous_value_;
};

std::string normalize_debug_value_selector(std::string value_name) {
  if (value_name.empty() || value_name.front() == '%' || value_name.front() == '@') {
    return value_name;
  }
  return "%" + value_name;
}

bool write_text_file(const std::string& path, std::string_view text) {
  std::ofstream out(path, std::ios::binary);
  if (!out) return false;
  out << text;
  return out.good();
}

bool looks_like_llvm_ir(std::string_view text) {
  while (!text.empty()) {
    auto eol = text.find('\n');
    auto line = (eol == std::string_view::npos) ? text : text.substr(0, eol);
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front()))) {
      line.remove_prefix(1);
    }

    if (!line.empty() && line.front() != ';') {
      if (line.rfind("target datalayout", 0) == 0 ||
          line.rfind("target triple", 0) == 0 ||
          line.rfind("source_filename", 0) == 0 ||
          line.rfind("define ", 0) == 0 ||
          line.rfind("declare ", 0) == 0 ||
          line.rfind("attributes ", 0) == 0) {
        return true;
      }
      return false;
    }

    if (eol == std::string_view::npos) break;
    text.remove_prefix(eol + 1);
  }
  return false;
}

std::string default_split_host_output_path(const std::string& input_path) {
  std::filesystem::path path(input_path);
  path.replace_extension(".ll");
  return path.string();
}

std::string default_split_device_output_path(const std::string& input_path) {
  const std::filesystem::path path(input_path);
  const std::filesystem::path parent = path.parent_path();
  const std::string stem = path.stem().string();
  return (parent / (stem + ".device.ll")).string();
}

bool execution_domain_matches(c4c::ExecutionDomain domain, bool want_device) {
  switch (domain) {
    case c4c::ExecutionDomain::Host:
      return !want_device;
    case c4c::ExecutionDomain::Device:
      return want_device;
    case c4c::ExecutionDomain::HostDevice:
      return true;
  }
  return false;
}

c4c::hir::Module filter_hir_module_for_domain(const c4c::hir::Module& input,
                                              c4c::ExecutionDomain domain,
                                              const c4c::TargetProfile& target_profile) {
  c4c::hir::Module filtered = input;
  filtered.target_profile = target_profile;
  filtered.data_layout.clear();
  filtered.functions.clear();
  filtered.globals.clear();
  filtered.fn_index.clear();
  filtered.global_index.clear();
  filtered.fn_structured_index.clear();
  filtered.global_structured_index.clear();

  const bool want_device = domain == c4c::ExecutionDomain::Device;
  for (const auto& fn : input.functions) {
    if (!execution_domain_matches(fn.execution_domain, want_device)) continue;
    filtered.index_function_decl(fn);
    filtered.functions.push_back(fn);
  }
  for (const auto& gv : input.globals) {
    if (!execution_domain_matches(gv.execution_domain, want_device)) continue;
    filtered.index_global_decl(gv);
    filtered.globals.push_back(gv);
  }
  filtered.sync_next_ids_from_contents();
  return filtered;
}

void append_env_include_paths(std::vector<std::string>& out,
                              std::set<std::string>& seen,
                              const char* env_name) {
  const char* raw = std::getenv(env_name);
  if (!raw || !raw[0]) return;
  std::string value(raw);
  size_t start = 0;
  while (start <= value.size()) {
    size_t end = value.find(':', start);
    std::string path = value.substr(start, end == std::string::npos ? std::string::npos
                                                                     : end - start);
    if (!path.empty() && std::filesystem::is_directory(path) && seen.insert(path).second) {
      out.push_back(path);
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
}

void append_default_include_path(std::vector<std::string>& out,
                                 std::set<std::string>& seen,
                                 const std::string& path) {
  if (path.empty()) return;
  if (!std::filesystem::is_directory(path)) return;
  if (seen.insert(path).second) out.push_back(path);
}

void append_sdk_include_paths(std::vector<std::string>& out,
                              std::set<std::string>& seen,
                              const std::string& sdk_root) {
  if (sdk_root.empty()) return;
  append_default_include_path(out, seen, sdk_root + "/usr/include/c++/v1");
  append_default_include_path(out, seen, sdk_root + "/usr/include");
}

void seed_default_system_include_paths(c4c::SourceProfile source_profile,
                                       std::vector<std::string>& system_include_paths) {
  std::set<std::string> seen(system_include_paths.begin(), system_include_paths.end());

  append_env_include_paths(system_include_paths, seen, "CPATH");
  if (source_profile == c4c::SourceProfile::CppSubset ||
      source_profile == c4c::SourceProfile::C4) {
    append_env_include_paths(system_include_paths, seen, "CPLUS_INCLUDE_PATH");
  } else {
    append_env_include_paths(system_include_paths, seen, "C_INCLUDE_PATH");
  }

  append_default_include_path(system_include_paths, seen, "/usr/local/include");
  append_default_include_path(system_include_paths, seen, "/usr/include");
  const std::filesystem::path usr_include_root("/usr/include");
  if (std::filesystem::is_directory(usr_include_root)) {
    for (const auto& arch_entry : std::filesystem::directory_iterator(usr_include_root)) {
      if (!arch_entry.is_directory()) continue;
      const std::string arch_name = arch_entry.path().filename().string();
      if (arch_name.find("-linux-gnu") == std::string::npos) continue;
      append_default_include_path(system_include_paths, seen, arch_entry.path().string());
    }
  }

  if (source_profile == c4c::SourceProfile::CppSubset ||
      source_profile == c4c::SourceProfile::C4) {
    const std::filesystem::path cpp_root("/usr/include/c++");
    if (std::filesystem::is_directory(cpp_root)) {
      for (const auto& entry : std::filesystem::directory_iterator(cpp_root)) {
        if (!entry.is_directory()) continue;
        const std::string version_dir = entry.path().string();
        append_default_include_path(system_include_paths, seen, version_dir);
        append_default_include_path(system_include_paths, seen, version_dir + "/backward");
        if (std::filesystem::is_directory(usr_include_root)) {
          for (const auto& arch_entry : std::filesystem::directory_iterator(usr_include_root)) {
            if (!arch_entry.is_directory()) continue;
            const std::string arch_name = arch_entry.path().filename().string();
            if (arch_name.find("-linux-gnu") == std::string::npos) continue;
            append_default_include_path(system_include_paths, seen,
                                        arch_entry.path().string() + "/c++/" +
                                            entry.path().filename().string());
          }
        }
      }
    }
  }

#if defined(__APPLE__)
  append_sdk_include_paths(system_include_paths, seen, std::getenv("SDKROOT") ? std::getenv("SDKROOT") : "");
  append_sdk_include_paths(system_include_paths, seen,
                           "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk");
  append_sdk_include_paths(system_include_paths, seen,
                           "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk");
  append_default_include_path(system_include_paths, seen,
                              "/Library/Developer/CommandLineTools/usr/include");
#endif
}

void print_usage(const char *argv0) {
  std::cerr
      << "Usage:\n"
      << "  " << argv0 << " [options] <input>\n"
      << "\n"
      << "General:\n"
      << "  -h, --help                 Show this help message\n"
      << "  --version                  Print version information\n"
      << "  -o <path>                  Write output to file\n"
      << "  --target <triple>          Override target triple\n"
      << "  --device-target <triple>   Device target triple for split LLVM output\n"
      << "  --emit-split-llvm          Emit separate host/device LLVM IR files\n"
      << "  --host-out <path>          Host LLVM output path for --emit-split-llvm\n"
      << "  --device-out <path>        Device LLVM output path for --emit-split-llvm\n"
      << "\n"
      << "Frontend inspection (mutually exclusive):\n"
      << "  --pp-only                  Run preprocessor only\n"
      << "  --lex-only                 Run lexer only\n"
      << "  --parse-only               Parse only; print AST summary and dump\n"
      << "  --dump-canonical           Print semantic canonical-type information\n"
      << "  --dump-hir                 Print full HIR plus compile-time stats\n"
      << "  --dump-hir-summary         Print compact HIR summary\n"
#if C4C_ENABLE_BACKEND
      << "  --dump-bir                 Print semantic backend BIR\n"
      << "  --dump-prepared-bir        Print prepared backend BIR plus metadata\n"
      << "  --dump-mir                 Print concise backend MIR-route summary\n"
      << "  --trace-mir                Print backend MIR-route trace\n"
      << "  --mir-focus-function <fn>  Limit backend dump/trace output to one function\n"
      << "  --mir-focus-block <label>  Limit focused backend dump/trace output to one block inside the focused function\n"
      << "  --mir-focus-value <name>   Limit focused backend dump/trace output to one value inside the focused function\n"
#endif
      << "\n"
      << "Parser debug:\n"
      << "  --parser-debug             Enable general parser debug output\n"
      << "  --parser-debug-tentative   Include tentative parse commit/rollback events\n"
      << "  --parser-debug-injected    Include injected-token parse events\n"
      << "\n"
      << "Code generation:\n"
#if C4C_ENABLE_BACKEND
      << "  --codegen llvm|asm|compare Select codegen backend path\n"
      << "  --backend-bir-stage prepared|semantic\n"
      << "                            For --codegen asm only, choose prepared\n"
      << "                            backend lowering (default) or semantic\n"
      << "                            BIR before prepare for bounded observation\n"
#else
      << "  --codegen llvm             Select LLVM output path\n"
      << "                            backend-native asm/compare routes are disabled in this build\n"
#endif
      << "\n"
      << "Preprocessor configuration:\n"
      << "  -D macro[=val]             Define macro\n"
      << "  -U macro                   Undefine macro\n"
      << "  -I <dir>                   Add include path\n"
      << "  -iquote <dir>              Add quote include path\n"
      << "  -isystem <dir>             Add system include path\n"
      << "  -idirafter <dir>           Add after-include path\n"
      << "\n"
      << "Optimization / relocation:\n"
      << "  -O0|-O1|-O2|-O3|-Os        Set optimization level\n"
      << "  -fPIC|-fpic                Enable PIC generation\n"
      << "  -fPIE|-fpie                Enable PIE generation\n"
      << "\n"
      << "Examples:\n"
      << "  " << argv0 << " --parse-only test.cpp\n"
      << "  " << argv0 << " --parser-debug --parse-only test.cpp\n"
      << "  " << argv0 << " --parser-debug --parser-debug-tentative --parse-only test.cpp\n"
      << "  " << argv0 << " --dump-hir test.cpp\n"
#if C4C_ENABLE_BACKEND
      << "  " << argv0 << " --dump-bir test.c\n"
      << "  " << argv0 << " --codegen asm --backend-bir-stage semantic test.c\n"
#endif
      << "\n"
      << "Notes:\n"
      << "  Only one frontend inspection mode may be selected at a time.\n";
}

void print_pp_diags(const std::vector<c4c::PreprocessorDiagnostic>& diags,
                    const char* level) {
  for (const auto& d : diags) {
    std::cerr << d.file << ":" << d.line << ":" << d.column << ": "
              << level << ": " << d.message << "\n";
  }
}

}  // namespace

int main(int argc, char **argv) {
  try {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
      args.emplace_back(argv[i]);
    }

    if (args.empty()) {
      print_usage(argv[0]);
      return 1;
    }
    if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h")) {
      print_usage(argv[0]);
      return 0;
    }
    if (args.size() == 1 && args[0] == "--version") {
      std::cout << "tiny-c2ll frontend_cxx next\n";
      return 0;
    }

    bool        pp_only    = false;
    bool        lex_only   = false;
    bool        parse_only = false;
    bool        dump_hir_summary = false;
    bool        dump_hir = false;
    bool        dump_canonical = false;
    bool        dump_bir = false;
    bool        dump_prepared_bir = false;
    bool        dump_mir = false;
    bool        trace_mir = false;
    std::optional<std::string> mir_focus_function;
    std::optional<std::string> mir_focus_block;
    std::optional<std::string> mir_focus_value;
    bool        parser_debug = false;
    bool        parser_debug_tentative = false;
    bool        parser_debug_injected = false;
    bool        emit_split_llvm = false;
    std::string input;
    std::string output;
    c4c::TargetProfile target_profile =
        c4c::target_profile_from_triple(c4c::default_host_target_triple());
    std::optional<c4c::TargetProfile> device_target_profile;
    std::string host_output;
    std::string device_output;
    bool emit_semantic_bir = false;

    // Preprocessor configuration collected from CLI flags.
    std::vector<std::string> defines;
    std::vector<std::string> undefines;
    std::vector<std::string> include_paths;
    std::vector<std::string> quote_include_paths;
    std::vector<std::string> system_include_paths;
    std::vector<std::string> after_include_paths;
    int  opt_level = 0;   // -O0 (default), -O1, -O2, -O3; -Os maps to 2
    bool opt_size  = false; // -Os
    int  pic_level = 0;   // -fPIC=2, -fpic=1
    int  pie_level = 0;   // -fPIE=2, -fpie=1
    auto codegen_path = c4c::codegen::llvm_backend::CodegenPath::Llvm;

    for (size_t i = 0; i < args.size(); i++) {
      const std::string& arg = args[i];
      if (arg == "--pp-only") {
        pp_only = true;
      } else if (arg == "--lex-only") {
        lex_only = true;
      } else if (arg == "--parse-only") {
        parse_only = true;
      } else if (arg == "--dump-hir") {
        dump_hir = true;
      } else if (arg == "--dump-hir-summary") {
        dump_hir_summary = true;
      } else if (arg == "--dump-canonical") {
        dump_canonical = true;
      } else if (arg == "--dump-bir") {
        dump_bir = true;
      } else if (arg == "--dump-prepared-bir") {
        dump_prepared_bir = true;
      } else if (arg == "--dump-mir" || arg == "--dump-x86-summary") {
        dump_mir = true;
      } else if (arg == "--trace-mir" || arg == "--trace-x86-handoff") {
        trace_mir = true;
      } else if (arg == "--mir-focus-function" && i + 1 < args.size()) {
        mir_focus_function = args[++i];
      } else if (arg == "--mir-focus-block" && i + 1 < args.size()) {
        mir_focus_block = args[++i];
      } else if (arg == "--mir-focus-value" && i + 1 < args.size()) {
        mir_focus_value = normalize_debug_value_selector(args[++i]);
      } else if (arg == "--parser-debug") {
        parser_debug = true;
      } else if (arg == "--parser-debug-tentative") {
        parser_debug_tentative = true;
      } else if (arg == "--parser-debug-injected") {
        parser_debug_injected = true;
      } else if (arg == "-o") {
        if (i + 1 < args.size()) output = args[++i];
      } else if (arg == "--target" && i + 1 < args.size()) {
        target_profile = c4c::target_profile_from_triple(args[++i]);
      } else if (arg == "--device-target" && i + 1 < args.size()) {
        device_target_profile = c4c::target_profile_from_triple(args[++i]);
      } else if (arg == "--host-out" && i + 1 < args.size()) {
        host_output = args[++i];
      } else if (arg == "--device-out" && i + 1 < args.size()) {
        device_output = args[++i];
      } else if (arg == "--emit-split-llvm") {
        emit_split_llvm = true;
      } else if (arg == "--backend-bir-stage" && i + 1 < args.size()) {
        const std::string& val = args[++i];
        if (val == "prepared") {
          emit_semantic_bir = false;
        } else if (val == "semantic") {
          emit_semantic_bir = true;
        } else {
          std::cerr << "unknown --backend-bir-stage value: " << val
                    << " (expected prepared or semantic)\n";
          return 2;
        }
      } else if (arg == "-D" && i + 1 < args.size()) {
        defines.push_back(args[++i]);
      } else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'D') {
        defines.push_back(arg.substr(2));
      } else if (arg == "-U" && i + 1 < args.size()) {
        undefines.push_back(args[++i]);
      } else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'U') {
        undefines.push_back(arg.substr(2));
      } else if (arg == "-I" && i + 1 < args.size()) {
        include_paths.push_back(args[++i]);
      } else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'I') {
        include_paths.push_back(arg.substr(2));
      } else if (arg == "-iquote" && i + 1 < args.size()) {
        quote_include_paths.push_back(args[++i]);
      } else if (arg == "-isystem" && i + 1 < args.size()) {
        system_include_paths.push_back(args[++i]);
      } else if (arg == "-idirafter" && i + 1 < args.size()) {
        after_include_paths.push_back(args[++i]);
      } else if (arg == "-O0") {
        opt_level = 0; opt_size = false;
      } else if (arg == "-O1" || arg == "-O") {
        opt_level = 1;
      } else if (arg == "-O2") {
        opt_level = 2;
      } else if (arg == "-O3") {
        opt_level = 3;
      } else if (arg == "-Os") {
        opt_level = 2; opt_size = true;
      } else if (arg == "-fPIC") {
        pic_level = 2;
      } else if (arg == "-fpic") {
        pic_level = 1;
      } else if (arg == "-fPIE") {
        pie_level = 2;
      } else if (arg == "-fpie") {
        pie_level = 1;
      } else if (arg == "--codegen" && i + 1 < args.size()) {
        const std::string& val = args[++i];
        if (val == "llvm") {
          codegen_path = c4c::codegen::llvm_backend::CodegenPath::Llvm;
        } else if (val == "asm") {
          codegen_path = c4c::codegen::llvm_backend::CodegenPath::Lir;
        } else if (val == "compare") {
          codegen_path = c4c::codegen::llvm_backend::CodegenPath::Compare;
        } else {
          std::cerr << "unknown --codegen value: " << val
                    << " (expected llvm, asm, or compare)\n";
          return 2;
        }
      } else if (arg == "--help" || arg == "-h") {
        print_usage(argv[0]);
        return 0;
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "unknown option: " << arg << "\n\n";
        print_usage(argv[0]);
        return 2;
      } else {
        input = arg;
      }
    }

    if (input.empty()) {
      print_usage(argv[0]);
      return 1;
    }
    if (emit_split_llvm) {
      if (!device_target_profile.has_value()) {
        std::cerr << "--emit-split-llvm requires --device-target <triple>\n";
        return 2;
      }
      if (codegen_path != c4c::codegen::llvm_backend::CodegenPath::Llvm) {
        std::cerr << "--emit-split-llvm cannot be combined with --codegen asm/compare\n";
        return 2;
      }
      if (!output.empty()) {
        std::cerr << "-o cannot be combined with --emit-split-llvm; use --host-out/--device-out\n";
        return 2;
      }
      if (host_output.empty()) host_output = default_split_host_output_path(input);
      if (device_output.empty()) device_output = default_split_device_output_path(input);
    }
    if (emit_semantic_bir &&
        codegen_path != c4c::codegen::llvm_backend::CodegenPath::Lir) {
      std::cerr << "--backend-bir-stage semantic requires --codegen asm\n";
      return 2;
    }
    {
      int mode_count = (pp_only ? 1 : 0) + (lex_only ? 1 : 0) + (parse_only ? 1 : 0) +
                       (dump_hir ? 1 : 0) + (dump_hir_summary ? 1 : 0) +
                       (dump_canonical ? 1 : 0) + (dump_bir ? 1 : 0) +
                       (dump_prepared_bir ? 1 : 0) + (dump_mir ? 1 : 0) + (trace_mir ? 1 : 0);
      if (mode_count > 1) {
        std::cerr << "cannot combine --pp-only, --lex-only, --parse-only, --dump-hir, --dump-hir-summary, --dump-canonical, --dump-bir, --dump-prepared-bir, --dump-mir, --trace-mir\n";
        return 2;
      }
    }
    if ((dump_bir || dump_prepared_bir || dump_mir || trace_mir) && emit_semantic_bir) {
      std::cerr << "--backend-bir-stage cannot be combined with --dump-bir, --dump-prepared-bir, --dump-mir, or --trace-mir\n";
      return 2;
    }
    if (mir_focus_function.has_value() &&
        !(dump_bir || dump_prepared_bir || dump_mir || trace_mir)) {
      std::cerr << "--mir-focus-function requires --dump-bir, --dump-prepared-bir, --dump-mir, or --trace-mir\n";
      return 2;
    }
    if (mir_focus_block.has_value() &&
        !(dump_bir || dump_prepared_bir || dump_mir || trace_mir)) {
      std::cerr << "--mir-focus-block requires --dump-bir, --dump-prepared-bir, --dump-mir, or --trace-mir\n";
      return 2;
    }
    if (mir_focus_block.has_value() && !mir_focus_function.has_value()) {
      std::cerr << "--mir-focus-block requires --mir-focus-function\n";
      return 2;
    }
    if (mir_focus_value.has_value() && !(dump_bir || dump_prepared_bir || dump_mir || trace_mir)) {
      std::cerr << "--mir-focus-value requires --dump-bir, --dump-prepared-bir, --dump-mir, or --trace-mir\n";
      return 2;
    }
    if (mir_focus_value.has_value() && !mir_focus_function.has_value()) {
      std::cerr << "--mir-focus-value requires --mir-focus-function\n";
      return 2;
    }

#if !C4C_ENABLE_BACKEND
    if (dump_bir || dump_prepared_bir || dump_mir || trace_mir ||
        mir_focus_function.has_value() || mir_focus_block.has_value() ||
        mir_focus_value.has_value() || emit_semantic_bir ||
        codegen_path != c4c::codegen::llvm_backend::CodegenPath::Llvm) {
      std::cerr << "backend support is disabled in this build\n";
      return 2;
    }
#endif

    // Determine source profile from input file extension.
    auto source_profile = c4c::source_profile_from_extension(input);
    auto lex_profile    = c4c::lex_profile_from(source_profile);
    auto sema_profile   = c4c::sema_profile_from(source_profile);
    seed_default_system_include_paths(source_profile, system_include_paths);

    c4c::Preprocessor preprocessor;
    preprocessor.set_target_profile(target_profile);
    preprocessor.set_source_profile(source_profile);
    // Apply optimization / PIC / PIE config toggles.
    if (opt_level > 0) {
      preprocessor.define_macro("__OPTIMIZE__");
      preprocessor.undefine_macro("__NO_INLINE__");
    }
    if (opt_size) {
      preprocessor.define_macro("__OPTIMIZE_SIZE__");
    }
    if (pic_level > 0) {
      preprocessor.define_macro("__PIC__=" + std::to_string(pic_level));
      preprocessor.define_macro("__pic__=" + std::to_string(pic_level));
    }
    if (pie_level > 0) {
      preprocessor.define_macro("__PIE__=" + std::to_string(pie_level));
      preprocessor.define_macro("__pie__=" + std::to_string(pie_level));
    }
    for (const auto& d : defines) preprocessor.define_macro(d);
    for (const auto& u : undefines) preprocessor.undefine_macro(u);
    for (const auto& p : include_paths) preprocessor.add_include_path(p);
    for (const auto& p : quote_include_paths) preprocessor.add_quote_include_path(p);
    for (const auto& p : system_include_paths) preprocessor.add_system_include_path(p);
    for (const auto& p : after_include_paths) preprocessor.add_after_include_path(p);
    std::string source = preprocessor.preprocess_file(input);
    if (!preprocessor.warnings().empty()) {
      print_pp_diags(preprocessor.warnings(), "warning");
    }
    if (!preprocessor.errors().empty()) {
      print_pp_diags(preprocessor.errors(), "error");
      return 1;
    }
    if (pp_only) {
      std::cout << source;
      return 0;
    }
    c4c::Lexer lexer(source, lex_profile);
    std::vector<c4c::Token> tokens = lexer.scan_all();

    if (lex_only) {
      for (const auto &tok : tokens) {
        const std::string spelling =
            tok.text_id != c4c::kInvalidText
                ? std::string(lexer.text_table().lookup(tok.text_id))
                : "";
        std::cout << c4c::token_kind_name(tok.kind) << " '" << spelling << "'"
                  << " @" << tok.line << ":" << tok.column << "\n";
      }
      return 0;
    }

    // Parse phase
    c4c::Arena arena;
    c4c::Parser parser(std::move(tokens), arena, &lexer.text_table(),
                       &lexer.file_table(), source_profile, input);
    unsigned parser_debug_channels = c4c::Parser::ParseDebugNone;
    if (parser_debug) parser_debug_channels = c4c::Parser::ParseDebugAll;
    if (parser_debug_tentative) {
      parser_debug_channels |= c4c::Parser::ParseDebugTentative;
    }
    if (parser_debug_injected) {
      parser_debug_channels |= c4c::Parser::ParseDebugInjected;
    }
    parser.set_parser_debug_channels(parser_debug_channels);
    c4c::Node* prog = parser.parse();
    if (parser.had_parse_error()) {
      return 1;
    }

    if (parse_only) {
      // Print a summary line followed by the full AST dump
      printf("Program(%d items)\n", prog ? prog->n_children : 0);
      if (prog) {
        c4c::ast_dump(prog, 0);
      }
      return 0;
    }

    c4c::sema::AnalyzeResult sema_result =
        c4c::sema::analyze_program(prog, sema_profile, target_profile);
    if (!sema_result.validation.ok) {
      c4c::sema::print_diagnostics(sema_result.validation.diagnostics, input);
      return 1;
    }

    if (dump_canonical) {
      std::cout << c4c::sema::format_canonical_result(sema_result.canonical);
      return 0;
    }

    // Run the HIR compile-time engine in verification mode; actual deferred
    // instantiation already happened inside build_hir().
    auto ct_stats = c4c::hir::run_compile_time_engine(
        *sema_result.hir_module);
    // Propagate deferred counts from the lowering-time pass.
    ct_stats.templates_deferred = sema_result.hir_module->ct_info.deferred_instantiations;
    ct_stats.consteval_deferred = sema_result.hir_module->ct_info.deferred_consteval;

    // Run materialization pass — decide which functions become concrete LLVM output.
    auto mat_stats = c4c::hir::materialize_ready_functions(
        *sema_result.hir_module);

    if (dump_hir || dump_hir_summary) {
      if (dump_hir) {
        std::cout << c4c::hir::format_hir(*sema_result.hir_module);
        std::cout << "\n--- compile-time reduction ---\n"
                  << "  " << c4c::hir::format_compile_time_stats(ct_stats)
                  << "\n--- materialization ---\n"
                  << "  " << c4c::hir::format_materialization_stats(mat_stats)
                  << "\n";
      } else {
        std::cout << c4c::hir::format_summary(*sema_result.hir_module) << "\n";
      }
      return 0;
    }

#if C4C_ENABLE_BACKEND
    if (dump_bir) {
      auto lir_mod = c4c::codegen::lir::lower(
          *sema_result.hir_module,
          c4c::codegen::lir::LowerOptions{
              .preserve_semantic_va_ops = true,
          });
      std::cout << c4c::backend::dump_module(
          c4c::backend::BackendModuleInput{lir_mod},
          c4c::backend::BackendOptions{
              .target_profile = target_profile,
              .route_debug_focus_function = mir_focus_function,
              .route_debug_focus_block = mir_focus_block,
              .route_debug_focus_value = mir_focus_value,
          },
          c4c::backend::BackendDumpStage::SemanticBir);
      return 0;
    }

    if (dump_prepared_bir || dump_mir || trace_mir) {
      auto lir_mod = c4c::codegen::lir::lower(
          *sema_result.hir_module,
          c4c::codegen::lir::LowerOptions{
              .preserve_semantic_va_ops = false,
          });
      const auto stage =
          dump_prepared_bir ? c4c::backend::BackendDumpStage::PreparedBir
          : dump_mir       ? c4c::backend::BackendDumpStage::MirSummary
                           : c4c::backend::BackendDumpStage::MirTrace;
      const ScopedEnvVar mir_focus_value_env(
          "C4C_MIR_FOCUS_VALUE",
          (dump_mir || trace_mir) && mir_focus_value.has_value()
              ? std::optional<std::string_view>(*mir_focus_value)
              : std::nullopt);
      std::cout << c4c::backend::dump_module(
          c4c::backend::BackendModuleInput{lir_mod},
          c4c::backend::BackendOptions{
              .target_profile = target_profile,
              .route_debug_focus_function = mir_focus_function,
              .route_debug_focus_block = mir_focus_block,
              .route_debug_focus_value = mir_focus_value,
          },
          stage);
      return 0;
    }
#endif

    // Run semantic inline expansion pass (Phase 1: discovery only, no-op for now).
    c4c::hir::run_inline_expansion(
        *sema_result.hir_module);

    if (emit_split_llvm) {
      auto host_module = filter_hir_module_for_domain(
          *sema_result.hir_module, c4c::ExecutionDomain::Host, target_profile);
      auto device_module = filter_hir_module_for_domain(
          *sema_result.hir_module, c4c::ExecutionDomain::Device, *device_target_profile);
      const std::string host_ir = c4c::codegen::llvm_backend::emit_module_native(
          host_module,
          target_profile,
          c4c::codegen::llvm_backend::CodegenPath::Llvm);
      const std::string device_ir = c4c::codegen::llvm_backend::emit_module_native(
          device_module,
          *device_target_profile,
          c4c::codegen::llvm_backend::CodegenPath::Llvm);
      const auto host_parent = std::filesystem::path(host_output).parent_path();
      const auto device_parent = std::filesystem::path(device_output).parent_path();
      if (!host_parent.empty()) std::filesystem::create_directories(host_parent);
      if (!device_parent.empty()) std::filesystem::create_directories(device_parent);
      if (!write_text_file(host_output, host_ir)) {
        std::cerr << "error: cannot open output file: " << host_output << "\n";
        return 1;
      }
      if (!write_text_file(device_output, device_ir)) {
        std::cerr << "error: cannot open output file: " << device_output << "\n";
        return 1;
      }
      return 0;
    }

    std::string ir = c4c::codegen::llvm_backend::emit_module_native(
        *sema_result.hir_module, target_profile, codegen_path, emit_semantic_bir);

    const bool wants_asm_output =
        output.empty() || has_suffix(output, ".s") || has_suffix(output, ".S");
    const bool backend_returned_llvm_ir = looks_like_llvm_ir(ir);
    const bool backend_returned_no_asm = ir.empty();
    if (codegen_path == c4c::codegen::llvm_backend::CodegenPath::Lir &&
        wants_asm_output &&
        (backend_returned_llvm_ir || backend_returned_no_asm)) {
      std::cerr << "error: --codegen asm requires backend-native assembly output.\n";
      if (output.empty()) {
        std::cerr << "       stdout assembly output is only available when the backend emits native asm.\n";
      } else {
        std::cerr << "       file output no longer falls back to LLVM-generated asm.\n";
      }
      std::cerr << "       Re-run with --codegen llvm if you need IR output.\n";
      return 2;
    }

    // Write to output file or stdout
    if (!output.empty()) {
      std::ofstream out(output, std::ios::binary);
      if (!out) {
        std::cerr << "error: cannot open output file: " << output << "\n";
        return 1;
      }
      out << ir;
    } else {
      std::cout << ir;
    }
    return 0;

  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 1;
  }
}
