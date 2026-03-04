#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include "arena.hpp"
#include "ast.hpp"
#include "ir_builder.hpp"
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

// Run clang -E -P on the input file and return the preprocessed source.
// Falls back to reading the file directly if clang is not found.
std::string preprocess(const std::string& path) {
  std::string cmd = "clang -E -P -x c " + path + " 2>/dev/null";
  FILE* fp = popen(cmd.c_str(), "r");
  if (!fp) return read_file(path);
  std::string result;
  char buf[4096];
  while (fgets(buf, sizeof(buf), fp))
    result += buf;
  int rc = pclose(fp);
  if (rc != 0 || result.empty()) return read_file(path);
  return result;
}

void print_usage(const char *argv0) {
  std::cerr << "usage: " << argv0
            << " [--version] [--lex-only|--parse-only] [-o output.ll] <input.c>\n";
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
      std::cout << "tiny-c2ll frontend_cxx stage1 (M3-ir)\n";
      return 0;
    }

    bool        lex_only   = false;
    bool        parse_only = false;
    std::string input;
    std::string output;

    for (size_t i = 0; i < args.size(); i++) {
      const std::string& arg = args[i];
      if (arg == "--lex-only") {
        lex_only = true;
      } else if (arg == "--parse-only") {
        parse_only = true;
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
    if (lex_only && parse_only) {
      std::cerr << "cannot use --lex-only and --parse-only together\n";
      return 2;
    }

    std::string source = preprocess(input);
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

    // IR emission phase (M3)
    tc::IRBuilder builder;
    std::string ir = builder.emit_program(prog);

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
