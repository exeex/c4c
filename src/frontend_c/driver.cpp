#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include "arena.hpp"
#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "token.hpp"

namespace tc = tinyc2ll::frontend_cxx;

namespace {

std::string read_file(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream buf;
  buf << in.rdbuf();
  return buf.str();
}

void print_usage(const char *argv0) {
  std::cerr << "usage: " << argv0
            << " [--version] [--lex-only|--parse-only] <input.c>\n";
}

// Emit a minimal LLVM IR stub (used until M3 IR builder is wired in).
void print_ir_stub() {
  std::cout << "; tiny-c2ll-stage1 M2-parse (IR stub)\n";
  std::cout << "define i32 @main() {\n";
  std::cout << "entry:\n";
  std::cout << "  ret i32 0\n";
  std::cout << "}\n";
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
      std::cout << "tiny-c2ll frontend_cxx stage1 (M2-parse)\n";
      return 0;
    }

    bool lex_only   = false;
    bool parse_only = false;
    std::string input;
    for (const auto &arg : args) {
      if (arg == "--lex-only") {
        lex_only = true;
      } else if (arg == "--parse-only") {
        parse_only = true;
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
    if (lex_only && parse_only) {
      std::cerr << "cannot use --lex-only and --parse-only together\n";
      return 2;
    }

    std::string source = read_file(input);
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

    if (parse_only) {
      // Print a summary line followed by the full AST dump
      printf("Program(%d items)\n", prog ? prog->n_children : 0);
      if (prog) {
        tc::ast_dump(prog, 0);
      }
      return 0;
    }

    // IR emission phase (stub until M3 wired in)
    (void)prog;  // suppress unused warning
    print_ir_stub();
    return 0;

  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 1;
  }
}
