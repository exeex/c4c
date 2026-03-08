#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#include <variant>

#include "arena.hpp"
#include "ast_to_hir.hpp"
#include "ast.hpp"
#include "hir_to_llvm.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"
#include "sema_validate.hpp"
#include "token.hpp"

namespace tc = tinyc2ll::frontend_cxx;

namespace {

void print_usage(const char *argv0) {
  std::cerr << "usage: " << argv0
            << " [--version] [--lex-only|--parse-only|--dump-hir|--dump-hir-summary]"
            << " [--pipeline=legacy|hir] [-o output.ll] <input.c>\n";
}

void print_pp_diags(const std::vector<tc::PreprocessorDiagnostic>& diags,
                    const char* level) {
  for (const auto& d : diags) {
    std::cerr << d.file << ":" << d.line << ":" << d.column << ": "
              << level << ": " << d.message << "\n";
  }
}

bool hir_has_unsupported_features(const tc::sema_ir::phase2::hir::Module& mod) {
  // Native HIR backend is still incomplete for these features.
  for (const auto& fn : mod.functions) {
    if (fn.attrs.variadic) return true;
  }
  return false;
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

    bool        lex_only   = false;
    bool        parse_only = false;
    bool        dump_hir_summary = false;
    bool        dump_hir = false;
    bool        pipeline_hir = true;
    std::string input;
    std::string output;

    for (size_t i = 0; i < args.size(); i++) {
      const std::string& arg = args[i];
      if (arg == "--lex-only") {
        lex_only = true;
      } else if (arg == "--parse-only") {
        parse_only = true;
      } else if (arg == "--pipeline=hir") {
        pipeline_hir = true;
      } else if (arg == "--pipeline=legacy") {
        pipeline_hir = false;
      } else if (arg == "--dump-hir") {
        dump_hir = true;
      } else if (arg == "--dump-hir-summary") {
        dump_hir_summary = true;
      } else if (arg == "-o") {
        if (i + 1 < args.size()) output = args[++i];
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
      int mode_count = (lex_only ? 1 : 0) + (parse_only ? 1 : 0) +
                       (dump_hir ? 1 : 0) + (dump_hir_summary ? 1 : 0);
      if (mode_count > 1) {
        std::cerr << "cannot combine --lex-only, --parse-only, --dump-hir, --dump-hir-summary\n";
        return 2;
      }
    }

    tc::Preprocessor preprocessor;
    std::string source = preprocessor.preprocess_file(input);
    if (!preprocessor.warnings().empty()) {
      print_pp_diags(preprocessor.warnings(), "warning");
    }
    if (!preprocessor.errors().empty()) {
      print_pp_diags(preprocessor.errors(), "error");
      return 1;
    }
    tc::Lexer lexer(source);
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

    tc::sema::ValidateResult sema_result = tc::sema::validate_program(prog);
    if (!sema_result.ok) {
      tc::sema::print_diagnostics(sema_result.diagnostics, input);
      return 1;
    }

    if (dump_hir || dump_hir_summary) {
      tc::sema_ir::phase2::hir::Module module =
          tc::sema_ir::phase2::hir::lower_ast_to_hir(prog);
      if (dump_hir) {
        std::cout << tc::sema_ir::phase2::hir::format_hir(module);
      } else {
        std::cout << tc::sema_ir::phase2::hir::format_summary(module) << "\n";
      }
      return 0;
    }

    // Phase 3: lower AST -> HIR, then emit LLVM IR.
    tc::sema_ir::phase2::hir::Module hir_mod =
        tc::sema_ir::phase2::hir::lower_ast_to_hir(prog);
    std::string ir;
    if (pipeline_hir) {
      if (hir_has_unsupported_features(hir_mod)) {
        throw std::runtime_error(
            "--pipeline=hir does not support variadic functions yet; "
            "use --pipeline=legacy");
      }
      ir = tinyc2ll::codegen::llvm_backend::emit_module_native(hir_mod);
    } else {
      ir = tinyc2ll::codegen::llvm_backend::emit_module(hir_mod, prog);
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
