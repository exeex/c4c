#include <cstdio>
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
#include <unistd.h>

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

bool has_suffix(std::string_view value, std::string_view suffix) {
  if (value.size() < suffix.size()) return false;
  return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string shell_quote(std::string_view text) {
  std::string quoted = "'";
  for (const char ch : text) {
    if (ch == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(ch);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

std::optional<std::string> make_temp_file(const std::string& name_template) {
  const auto temp_dir = std::filesystem::temp_directory_path();
  std::string path = (temp_dir / name_template).string();
  std::vector<char> buffer(path.begin(), path.end());
  buffer.push_back('\0');

  const int fd = ::mkstemp(buffer.data());
  if (fd < 0) return std::nullopt;
  ::close(fd);
  return std::string(buffer.data());
}

bool write_text_file(const std::string& path, std::string_view text) {
  std::ofstream out(path, std::ios::binary);
  if (!out) return false;
  out << text;
  return out.good();
}

std::optional<std::string> read_text_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return std::nullopt;
  std::ostringstream buffer;
  buffer << in.rdbuf();
  if (!in.good() && !in.eof()) return std::nullopt;
  return buffer.str();
}

std::string trim_copy(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

std::string normalize_aarch64_fallback_asm(std::string text) {
  for (char& ch : text) {
    if (ch == '\t') ch = ' ';
  }

  std::vector<std::string> lines;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto end = text.find('\n', start);
    if (end == std::string::npos) {
      lines.push_back(text.substr(start));
      break;
    }
    lines.push_back(text.substr(start, end - start));
    start = end + 1;
  }

  for (std::size_t i = 0; i + 2 < lines.size(); ++i) {
    const auto adrp = trim_copy(lines[i]);
    const auto got_load = trim_copy(lines[i + 1]);
    const auto value_load = trim_copy(lines[i + 2]);
    if (adrp.rfind("adrp ", 0) != 0) continue;

    const auto comma = adrp.find(", :got:");
    if (comma == std::string::npos) continue;

    const auto addr_reg = adrp.substr(5, comma - 5);
    const auto symbol = adrp.substr(comma + 7);
    const std::string expected_got_load =
        "ldr " + addr_reg + ", [" + addr_reg + ", :got_lo12:" + symbol + "]";
    if (got_load != expected_got_load) continue;

    const std::string value_prefix = "ldr ";
    if (value_load.rfind(value_prefix, 0) != 0) continue;
    const auto value_comma = value_load.find(", [");
    if (value_comma == std::string::npos) continue;
    if (value_load.substr(value_comma + 3) != addr_reg + "]") continue;

    const auto indent_width = lines[i].find_first_not_of(' ');
    const std::string indent(indent_width == std::string::npos ? 0 : indent_width, ' ');
    lines[i] = indent + "adrp " + addr_reg + ", " + symbol;
    lines[i + 1] = indent + "add " + addr_reg + ", " + addr_reg + ", :lo12:" + symbol;
  }

  std::ostringstream normalized;
  bool first = true;
  for (const auto& line : lines) {
    const auto trimmed = trim_copy(line);
    if (trimmed.empty() ||
        trimmed.rfind(".cfi_", 0) == 0 ||
        trimmed.rfind(".file ", 0) == 0 ||
        trimmed.rfind(".ident ", 0) == 0 ||
        trimmed.rfind(".addrsig", 0) == 0 ||
        trimmed.rfind("//", 0) == 0) {
      continue;
    }
    if (!first) normalized << '\n';
    if (trimmed.rfind(".word ", 0) == 0) {
      normalized << line.substr(0, line.find(trimmed)) << ".long "
                 << trimmed.substr(6);
    } else {
      normalized << line;
    }
    first = false;
  }
  if (!text.empty() && text.back() == '\n') {
    normalized << '\n';
  }
  return normalized.str();
}

std::string normalize_asm_text(std::string text) {
  text = normalize_aarch64_fallback_asm(std::move(text));
  std::string_view view(text);
  while (!view.empty() && (view.front() == ' ' || view.front() == '\t' ||
                           view.front() == '\r' || view.front() == '\n')) {
    view.remove_prefix(1);
  }
  if (view.rfind(".text", 0) == 0) {
    return ".text\n" + text;
  }
  return text;
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

bool starts_with(std::string_view text, std::string_view prefix) {
  if (text.size() < prefix.size()) return false;
  return text.compare(0, prefix.size(), prefix) == 0;
}

bool reason_already_present(const std::vector<std::string>& reasons,
                           std::string_view reason) {
  for (const auto& existing : reasons) {
    if (existing == reason) return true;
  }
  return false;
}

std::vector<std::string> infer_asm_fallback_reasons(std::string_view ir) {
  std::vector<std::string> reasons;

  if (ir.find("@llvm.va_start") != std::string_view::npos ||
      ir.find("@llvm.va_end") != std::string_view::npos ||
      ir.find(" ...)") != std::string_view::npos) {
    reasons.push_back("encountered varargs-related lowering pattern");
  }

  if (ir.find("landingpad") != std::string_view::npos ||
      ir.find("invoke ") != std::string_view::npos) {
    reasons.push_back("encountered exception/cleanup constructs");
  }

  size_t pos = 0;
  while (true) {
    auto next = ir.find("declare", pos);
    if (next == std::string_view::npos) break;
    auto eol = ir.find('\n', next);
    auto line = ir.substr(
        next, eol == std::string_view::npos ? std::string_view::npos : eol - next);

    if (line.find("...") != std::string_view::npos) {
      auto at = line.find("@");
      if (at != std::string_view::npos) {
        auto name_end = line.find('(', at);
        auto maybe_name = line.substr(at + 1, name_end - (at + 1));
        if (name_end != std::string_view::npos) {
          if (starts_with(maybe_name, "llvm.")) {
            // ignore intrinsic declarations
          } else {
            std::string sym = "@" + std::string(maybe_name);
            if (!reason_already_present(reasons, sym)) {
              reasons.push_back("declared/used varargs function " + sym +
                                " may need LLVM-path fallback");
            }
          }
        }
      }
    }
    pos = (eol == std::string_view::npos) ? std::string_view::npos : (eol + 1);
    if (pos == std::string_view::npos) break;
  }

  if (reasons.empty()) {
    reasons.push_back("no specific unsupported pattern was detected in fallback IR");
  }
  return reasons;
}

void print_asm_fallback_hint(std::string_view ir) {
  auto reasons = infer_asm_fallback_reasons(ir);
  std::cerr << "error: --codegen asm did not emit assembly for this input and cannot write .s.\n";
  std::cerr << "       Reason detected in emitted IR:\n";
  for (const auto& reason : reasons) {
    std::cerr << "       - " << reason << "\n";
  }
  std::cerr << "       Re-run with --codegen llvm if you need IR output.\n";
}

std::optional<std::string> lower_llvm_ir_to_asm(std::string_view ir,
                                                std::string_view target_triple) {
  auto ir_path = make_temp_file("c4c-ir-XXXXXX");
  auto asm_path = make_temp_file("c4c-asm-XXXXXX");
  if (!ir_path.has_value() || !asm_path.has_value()) return std::nullopt;

  struct TempCleanup {
    std::string ir_path;
    std::string asm_path;
    ~TempCleanup() {
      std::error_code ignored;
      if (!ir_path.empty()) std::filesystem::remove(ir_path, ignored);
      if (!asm_path.empty()) std::filesystem::remove(asm_path, ignored);
    }
  } cleanup{*ir_path, *asm_path};

  if (!write_text_file(*ir_path, ir)) return std::nullopt;

  std::vector<std::string> commands;
  std::string llc_command = "llc -mtriple=" + shell_quote(target_triple);
  if (target_triple.find("x86_64") != std::string_view::npos) {
    llc_command += " -relocation-model=pic";
  }
  llc_command += " -filetype=asm " + shell_quote(*ir_path) + " -o " +
                 shell_quote(*asm_path) + " >/dev/null 2>&1";
  commands.push_back(std::move(llc_command));
  commands.push_back("clang -target " + shell_quote(target_triple) + " -x ir -S " +
                     shell_quote(*ir_path) + " -o " + shell_quote(*asm_path) +
                     " >/dev/null 2>&1");

  for (const auto& command : commands) {
    if (std::system(command.c_str()) != 0) continue;
    auto asm_text = read_text_file(*asm_path);
    if (asm_text.has_value() && !asm_text->empty()) {
      return normalize_asm_text(*asm_text);
    }
  }

  return std::nullopt;
}

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
  std::cerr
      << "Usage:\n"
      << "  " << argv0 << " [options] <input>\n"
      << "\n"
      << "General:\n"
      << "  -h, --help                 Show this help message\n"
      << "  --version                  Print version information\n"
      << "  -o <path>                  Write output to file\n"
      << "  --target <triple>          Override target triple\n"
      << "\n"
      << "Frontend inspection (mutually exclusive):\n"
      << "  --pp-only                  Run preprocessor only\n"
      << "  --lex-only                 Run lexer only\n"
      << "  --parse-only               Parse only; print AST summary and dump\n"
      << "  --dump-canonical           Print semantic canonical-type information\n"
      << "  --dump-hir                 Print full HIR plus compile-time stats\n"
      << "  --dump-hir-summary         Print compact HIR summary\n"
      << "\n"
      << "Parser debug:\n"
      << "  --parser-debug             Enable general parser debug output\n"
      << "  --parser-debug-tentative   Include tentative parse commit/rollback events\n"
      << "  --parser-debug-injected    Include injected-token parse events\n"
      << "\n"
      << "Code generation:\n"
      << "  --codegen llvm|asm|compare Select codegen backend path\n"
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
    bool        parser_debug = false;
    bool        parser_debug_tentative = false;
    bool        parser_debug_injected = false;
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
      } else if (arg == "--parser-debug") {
        parser_debug = true;
      } else if (arg == "--parser-debug-tentative") {
        parser_debug_tentative = true;
      } else if (arg == "--parser-debug-injected") {
        parser_debug_injected = true;
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

    const bool wants_asm_output =
        output.empty() || has_suffix(output, ".s") || has_suffix(output, ".S");
    const bool backend_returned_llvm_ir = looks_like_llvm_ir(ir);
    const bool backend_returned_no_asm = ir.empty();
    if (codegen_path == c4c::codegen::llvm_backend::CodegenPath::Lir &&
        wants_asm_output &&
        (backend_returned_llvm_ir || backend_returned_no_asm)) {
      std::optional<std::string> fallback_asm;
      if (backend_returned_llvm_ir) {
        fallback_asm = lower_llvm_ir_to_asm(ir, target_triple);
      }
      if (!fallback_asm.has_value()) {
        auto legacy_ir = c4c::codegen::llvm_backend::emit_module_native(
            *sema_result.hir_module, target_triple,
            c4c::codegen::llvm_backend::CodegenPath::Llvm);
        fallback_asm = lower_llvm_ir_to_asm(legacy_ir, target_triple);
        if (!backend_returned_llvm_ir) {
          ir = legacy_ir;
        }
      }
      if (!fallback_asm.has_value()) {
        print_asm_fallback_hint(ir);
        return 2;
      }
      ir = *fallback_asm;
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
