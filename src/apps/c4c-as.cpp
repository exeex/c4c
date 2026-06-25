#if C4C_ENABLE_BACKEND
#include "mir/riscv/codegen/rv64_line_assembler.hpp"
#endif

#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct CliOptions {
  std::string input_path;
  std::string output_path;
};

struct ParseResult {
  std::vector<std::string> globals;
  std::vector<std::string> labels;
#if C4C_ENABLE_BACKEND
  struct Instruction {
    int line_number = 0;
    c4c::backend::riscv::codegen::Rv64AsmLine parsed;
  };
  std::vector<Instruction> instructions;
#else
  std::vector<std::string> instructions;
#endif
};

void print_help(std::ostream& out) {
  out << "Usage:\n"
      << "  c4c-as [options] <input.s>\n\n"
      << "Options:\n"
      << "  -h, --help       Show this help message\n"
      << "  --version        Print version information\n"
      << "  -o <path>        Output object path\n\n"
      << "Status:\n"
      << "  assembles the initial RV64 .text instruction subset into text bytes;\n"
      << "  relocatable object emission is not implemented yet\n";
}

std::string_view trim_ascii(std::string_view text) {
  while (!text.empty() &&
         std::isspace(static_cast<unsigned char>(text.front())) != 0) {
    text.remove_prefix(1);
  }
  while (!text.empty() &&
         std::isspace(static_cast<unsigned char>(text.back())) != 0) {
    text.remove_suffix(1);
  }
  return text;
}

std::string_view strip_comment(std::string_view line) {
  const auto comment = line.find('#');
  if (comment == std::string_view::npos) {
    return line;
  }
  return line.substr(0, comment);
}

bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool is_globl_directive(std::string_view line) {
  constexpr std::string_view prefix = ".globl";
  return starts_with(line, prefix) &&
         line.size() > prefix.size() &&
         std::isspace(static_cast<unsigned char>(line[prefix.size()])) != 0;
}

bool is_symbol_start(char c) {
  const auto uc = static_cast<unsigned char>(c);
  return std::isalpha(uc) != 0 || c == '_' || c == '.' || c == '$';
}

bool is_symbol_continue(char c) {
  const auto uc = static_cast<unsigned char>(c);
  return std::isalnum(uc) != 0 || c == '_' || c == '.' || c == '$';
}

bool is_valid_symbol(std::string_view symbol) {
  if (symbol.empty() || !is_symbol_start(symbol.front())) {
    return false;
  }
  for (const char c : symbol.substr(1)) {
    if (!is_symbol_continue(c)) {
      return false;
    }
  }
  return true;
}

#if C4C_ENABLE_BACKEND
std::optional<c4c::backend::riscv::codegen::Rv64AsmLine> parse_rv64_instruction(
    std::string_view line) {
  return c4c::backend::riscv::codegen::parse_rv64_asm_line(line);
}
#endif

std::string hex_bytes(const std::vector<std::uint8_t>& bytes) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const auto byte : bytes) {
    out << std::setw(2) << static_cast<unsigned int>(byte);
  }
  return out.str();
}

std::optional<CliOptions> parse_cli(int argc, char** argv) {
  CliOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view arg = argv[index];
    if (arg == "-h" || arg == "--help") {
      print_help(std::cout);
      return std::nullopt;
    }
    if (arg == "--version") {
      std::cout << "c4c-as 0.1\n";
      return std::nullopt;
    }
    if (arg == "-o") {
      if (index + 1 >= argc) {
        std::cerr << "c4c-as: error: -o requires an output path\n";
        return std::nullopt;
      }
      if (!options.output_path.empty()) {
        std::cerr << "c4c-as: error: duplicate -o output path\n";
        return std::nullopt;
      }
      options.output_path = argv[++index];
      if (options.output_path.empty()) {
        std::cerr << "c4c-as: error: -o requires an output path\n";
        return std::nullopt;
      }
      continue;
    }
    if (!arg.empty() && arg.front() == '-') {
      std::cerr << "c4c-as: error: unsupported option '" << arg << "'\n";
      return std::nullopt;
    }
    if (!options.input_path.empty()) {
      std::cerr << "c4c-as: error: multiple input files are not supported\n";
      return std::nullopt;
    }
    options.input_path = std::string(arg);
  }

  if (options.input_path.empty()) {
    std::cerr << "c4c-as: error: missing input file\n";
    return std::nullopt;
  }
  if (options.output_path.empty()) {
    std::cerr << "c4c-as: error: assembly input requires -o <path>\n";
    return std::nullopt;
  }
  return options;
}

std::optional<ParseResult> parse_assembly_file(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    std::cerr << "c4c-as: error: failed to open input file '" << path << "'\n";
    return std::nullopt;
  }

  ParseResult result;
  bool in_text = false;
  std::string raw_line;
  int line_number = 0;
  while (std::getline(input, raw_line)) {
    ++line_number;
    const auto line = trim_ascii(strip_comment(raw_line));
    if (line.empty()) {
      continue;
    }

    if (line == ".text") {
      in_text = true;
      continue;
    }

    if (is_globl_directive(line)) {
      const auto symbol = trim_ascii(line.substr(std::string_view(".globl").size()));
      if (symbol.empty() || symbol.find_first_of(" \t\r\n") != std::string_view::npos ||
          !is_valid_symbol(symbol)) {
        std::cerr << "c4c-as: error: " << path << ':' << line_number
                  << ": malformed .globl directive\n";
        return std::nullopt;
      }
      result.globals.emplace_back(symbol);
      continue;
    }

    const auto colon = line.find(':');
    if (colon != std::string_view::npos) {
      if (colon + 1 != line.size() || !is_valid_symbol(line.substr(0, colon))) {
        std::cerr << "c4c-as: error: " << path << ':' << line_number
                  << ": malformed label\n";
        return std::nullopt;
      }
      if (!in_text) {
        std::cerr << "c4c-as: error: " << path << ':' << line_number
                  << ": label outside .text section\n";
        return std::nullopt;
      }
      result.labels.emplace_back(line.substr(0, colon));
      continue;
    }

#if C4C_ENABLE_BACKEND
    auto parsed_instruction = parse_rv64_instruction(line);
    const bool instruction_parsed = parsed_instruction.has_value();
#else
    const bool instruction_parsed = false;
#endif
    if (!line.empty() && line.front() == '.' && !instruction_parsed) {
      std::cerr << "c4c-as: error: " << path << ':' << line_number
                << ": unsupported directive '" << line << "'\n";
      return std::nullopt;
    }

    if (!in_text) {
      std::cerr << "c4c-as: error: " << path << ':' << line_number
                << ": instruction outside .text section\n";
      return std::nullopt;
    }
    if (!instruction_parsed) {
#if C4C_ENABLE_BACKEND
      std::cerr << "c4c-as: error: " << path << ':' << line_number
                << ": unsupported RV64 instruction '" << line << "'\n";
#else
      std::cerr << "c4c-as: error: backend support is disabled; cannot parse RV64 instructions\n";
#endif
      return std::nullopt;
    }
#if C4C_ENABLE_BACKEND
    result.instructions.push_back(ParseResult::Instruction{line_number, *parsed_instruction});
#else
    result.instructions.emplace_back(line);
#endif
  }

  if (!in_text) {
    std::cerr << "c4c-as: error: " << path << ": missing .text section\n";
    return std::nullopt;
  }
  return result;
}

std::optional<std::vector<std::uint8_t>> assemble_text_bytes(const ParseResult& parsed,
                                                            const std::string& path) {
  std::vector<std::uint8_t> bytes;
#if C4C_ENABLE_BACKEND
  for (const auto& instruction : parsed.instructions) {
    const auto encoded =
        c4c::backend::riscv::codegen::encode_rv64_asm_line(instruction.parsed);
    if (!encoded.has_value()) {
      std::cerr << "c4c-as: error: " << path << ':' << instruction.line_number
                << ": failed to encode RV64 instruction\n";
      return std::nullopt;
    }
    bytes.insert(bytes.end(), encoded->begin(), encoded->end());
  }
  return bytes;
#else
  (void)parsed;
  (void)path;
  std::cerr << "c4c-as: error: backend support is disabled; cannot encode RV64 instructions\n";
  return std::nullopt;
#endif
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 1) {
    print_help(std::cout);
    return 0;
  }

  for (int index = 1; index < argc; ++index) {
    const std::string_view arg = argv[index];
    if (arg == "-h" || arg == "--help") {
      print_help(std::cout);
      return 0;
    }
    if (arg == "--version") {
      std::cout << "c4c-as 0.1\n";
      return 0;
    }
  }

  auto options = parse_cli(argc, argv);
  if (!options.has_value()) {
    return 1;
  }

  const auto parsed = parse_assembly_file(options->input_path);
  if (!parsed.has_value()) {
    return 1;
  }

  const auto text_bytes = assemble_text_bytes(*parsed, options->input_path);
  if (!text_bytes.has_value()) {
    return 1;
  }

  std::cout << "c4c-as: assembled " << parsed->instructions.size()
            << " instruction line(s) from " << options->input_path << " into "
            << text_bytes->size() << " text byte(s): " << hex_bytes(*text_bytes)
            << "; relocatable object emission is not implemented yet for "
            << options->output_path << '\n';
  return 0;
}
