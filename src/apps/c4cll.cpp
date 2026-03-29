#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#include <variant>

#include "arena.hpp"
#include "ast.hpp"
#include "llvm_codegen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"
#include "sema.hpp"
#include "hir.hpp"
#include "compile_time_engine.hpp"
#include "inline_expand.hpp"
#include "source_profile.hpp"
#include "token.hpp"


namespace {

std::string default_host_target_triple() {
#if defined(__aarch64__) || defined(_M_ARM64)
#if defined(__APPLE__)
  return "aarch64-apple-darwin";
#elif defined(__linux__)
  return "aarch64-unknown-linux-gnu";
#else
  return "aarch64-unknown-unknown";
#endif
#elif defined(__x86_64__) || defined(_M_X64)
#if defined(__APPLE__)
  return "x86_64-apple-darwin";
#elif defined(__linux__)
  return "x86_64-unknown-linux-gnu";
#else
  return "x86_64-unknown-unknown";
#endif
#else
  return "unknown-unknown-unknown";
#endif
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
  std::cerr << "usage: " << argv0
            << " [--version] [--pp-only|--lex-only|--parse-only|--dump-hir|--dump-hir-summary|--dump-canonical|--parser-debug]"
            << " [--target triple] [--codegen legacy|lir|compare]"
            << " [-D macro[=val]] [-U macro] [-I dir] [-iquote dir] [-isystem dir] [-idirafter dir]"
            << " [-O0|-O1|-O2|-O3|-Os] [-fPIC|-fpic] [-fPIE|-fpie]"
            << " [-o output.ll] <input.c>\n";
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
    bool        parser_debug = false;
    std::string input;
    std::string output;
    std::string target_triple = default_host_target_triple();

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
    auto codegen_path = c4c::codegen::llvm_backend::CodegenPath::Legacy;

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
      } else if (arg == "--parser-debug") {
        parser_debug = true;
      } else if (arg == "-o") {
        if (i + 1 < args.size()) output = args[++i];
      } else if (arg == "--target" && i + 1 < args.size()) {
        target_triple = args[++i];
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
        if (val == "legacy") {
          codegen_path = c4c::codegen::llvm_backend::CodegenPath::Legacy;
        } else if (val == "lir") {
          codegen_path = c4c::codegen::llvm_backend::CodegenPath::Lir;
        } else if (val == "compare") {
          codegen_path = c4c::codegen::llvm_backend::CodegenPath::Compare;
        } else {
          std::cerr << "unknown --codegen value: " << val
                    << " (expected legacy, lir, or compare)\n";
          return 2;
        }
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "unknown option: " << arg << "\n";
        return 2;
      } else {
        input = arg;
      }
    }

    if (input.empty()) {
      print_usage(argv[0]);
      return 1;
    }
    {
      int mode_count = (pp_only ? 1 : 0) + (lex_only ? 1 : 0) + (parse_only ? 1 : 0) +
                       (dump_hir ? 1 : 0) + (dump_hir_summary ? 1 : 0) +
                       (dump_canonical ? 1 : 0);
      if (mode_count > 1) {
        std::cerr << "cannot combine --pp-only, --lex-only, --parse-only, --dump-hir, --dump-hir-summary, --dump-canonical\n";
        return 2;
      }
    }

    // Determine source profile from input file extension.
    auto source_profile = c4c::source_profile_from_extension(input);
    auto lex_profile    = c4c::lex_profile_from(source_profile);
    auto sema_profile   = c4c::sema_profile_from(source_profile);
    seed_default_system_include_paths(source_profile, system_include_paths);

    c4c::Preprocessor preprocessor;
    preprocessor.set_target_triple(target_triple);
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
        std::cout << c4c::token_kind_name(tok.kind) << " '" << tok.lexeme << "'"
                  << " @" << tok.line << ":" << tok.column << "\n";
      }
      return 0;
    }

    // Parse phase
    c4c::Arena arena;
    c4c::Parser parser(std::move(tokens), arena, source_profile, input);
    parser.set_parser_debug(parser_debug);
    c4c::Node* prog = parser.parse();
    if (parser.had_error_) {
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
        c4c::sema::analyze_program(prog, sema_profile, target_triple);
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

    // Run semantic inline expansion pass (Phase 1: discovery only, no-op for now).
    c4c::hir::run_inline_expansion(
        *sema_result.hir_module);

    std::string ir = c4c::codegen::llvm_backend::emit_module_native(
        *sema_result.hir_module, target_triple, codegen_path);

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
