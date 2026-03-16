#include <cstdio>
#include <fstream>
#include <iostream>
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
#include "source_profile.hpp"
#include "token.hpp"

namespace tc = tinyc2ll::frontend_cxx;

namespace {

void print_usage(const char *argv0) {
  std::cerr << "usage: " << argv0
            << " [--version] [--pp-only|--lex-only|--parse-only|--dump-hir|--dump-hir-summary]"
            << " [-D macro[=val]] [-U macro] [-I dir] [-iquote dir] [-isystem dir] [-idirafter dir]"
            << " [-O0|-O1|-O2|-O3|-Os] [-fPIC|-fpic] [-fPIE|-fpie]"
            << " [-o output.ll] <input.c>\n";
}

void print_pp_diags(const std::vector<tc::PreprocessorDiagnostic>& diags,
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
    std::string input;
    std::string output;

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
      } else if (arg == "-o") {
        if (i + 1 < args.size()) output = args[++i];
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
                       (dump_hir ? 1 : 0) + (dump_hir_summary ? 1 : 0);
      if (mode_count > 1) {
        std::cerr << "cannot combine --pp-only, --lex-only, --parse-only, --dump-hir, --dump-hir-summary\n";
        return 2;
      }
    }

    // Determine source profile from input file extension.
    auto source_profile = tc::source_profile_from_extension(input);
    auto lex_profile    = tc::lex_profile_from(source_profile);
    auto sema_profile   = tc::sema_profile_from(source_profile);

    tc::Preprocessor preprocessor;
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
    tc::Lexer lexer(source, lex_profile);
    std::vector<tc::Token> tokens = lexer.scan_all();

    if (lex_only) {
      for (const auto &tok : tokens) {
        std::cout << tc::token_kind_name(tok.kind) << " '" << tok.lexeme << "'"
                  << " @" << tok.line << ":" << tok.column << "\n";
      }
      return 0;
    }

    // Parse phase
    tc::Arena arena;
    tc::Parser parser(std::move(tokens), arena);
    tc::Node* prog = parser.parse();
    if (parser.had_error_) {
      return 1;
    }

    if (parse_only) {
      // Print a summary line followed by the full AST dump
      printf("Program(%d items)\n", prog ? prog->n_children : 0);
      if (prog) {
        tc::ast_dump(prog, 0);
      }
      return 0;
    }

    tc::sema::AnalyzeResult sema_result = tc::sema::analyze_program(prog, sema_profile);
    if (!sema_result.validation.ok) {
      tc::sema::print_diagnostics(sema_result.validation.diagnostics, input);
      return 1;
    }

    if (dump_hir || dump_hir_summary) {
      if (dump_hir) {
        std::cout << tc::sema::format_hir(*sema_result.hir_module);
      } else {
        std::cout << tc::sema::format_summary(*sema_result.hir_module) << "\n";
      }
      return 0;
    }

    std::string ir = tinyc2ll::codegen::llvm_backend::emit_module_native(
        *sema_result.hir_module);

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
