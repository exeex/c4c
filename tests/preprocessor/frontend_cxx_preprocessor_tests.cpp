#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "preprocessor.hpp"

namespace fs = std::filesystem;
using tinyc2ll::frontend_cxx::Preprocessor;

namespace {

struct PendingTracker {
  std::vector<std::string> items;

  void add(const std::string& feature, const std::string& detail) {
    items.push_back(feature + ": " + detail);
  }

  void print_summary() const {
    if (items.empty()) return;
    std::cout << "PENDING preprocessor features (" << items.size() << ")\n";
    for (const auto& s : items) {
      std::cout << "  - " << s << "\n";
    }
  }
};

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void expect_contains(const std::string& text, const std::string& needle,
                     const std::string& msg) {
  if (text.find(needle) == std::string::npos) {
    fail(msg + "\nExpected to find: " + needle + "\nActual:\n" + text);
  }
}

void expect_not_contains(const std::string& text, const std::string& needle,
                         const std::string& msg) {
  if (text.find(needle) != std::string::npos) {
    fail(msg + "\nUnexpected: " + needle + "\nActual:\n" + text);
  }
}

void write_text(const fs::path& path, const std::string& content) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("cannot write: " + path.string());
  }
  out << content;
}

fs::path make_test_dir(const char* name) {
  fs::path base = fs::temp_directory_path() / "tinyc2ll_preprocessor_tests";
  fs::create_directories(base);
  fs::path dir = base / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

// Returns true when output already matches expected behavior.
bool probe_feature_contains(const std::string& out, const std::string& needle) {
  return out.find(needle) != std::string::npos;
}

void test_phase2_line_splice() {
  fs::path dir = make_test_dir("phase2_line_splice");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define A 12\\\n"
             "34\n"
             "int x = A;\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int x = 1234;", "phase2 line splicing should merge \\\\n");
}

void test_phase3_comment_strip() {
  fs::path dir = make_test_dir("phase3_comment_strip");
  fs::path file = dir / "main.c";
  write_text(file,
             "int a = 1; // remove me\n"
             "int b = 2/*blk*/+3;\n"
             "const char* s = \"/* keep inside string */\";\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_not_contains(out, "//", "line comments should be removed");
  expect_not_contains(out, "/*blk*/", "block comments should be removed");
  expect_contains(out, "const char* s = \"/* keep inside string */\";",
                  "string literals must stay intact");
}

void test_define_undef_object_macro() {
  fs::path dir = make_test_dir("define_undef_object_macro");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define X 9\n"
             "int a = X;\n"
             "#undef X\n"
             "int b = X;\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int a = 9;", "object-like macro should expand");
  expect_contains(out, "int b = X;", "undefined macro should stop expanding");
}

void test_conditional_ifdef_ifndef() {
  fs::path dir = make_test_dir("conditional_ifdef_ifndef");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define FOO\n"
             "#ifdef FOO\n"
             "int a = 1;\n"
             "#else\n"
             "int a = 2;\n"
             "#endif\n"
             "#ifndef FOO\n"
             "int b = 3;\n"
             "#else\n"
             "int b = 4;\n"
             "#endif\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int a = 1;", "#ifdef true branch should be active");
  expect_not_contains(out, "int a = 2;", "#ifdef false branch should be inactive");
  expect_contains(out, "int b = 4;", "#ifndef false branch should go to #else");
  expect_not_contains(out, "int b = 3;", "inactive #ifndef branch should be removed");
}

void test_conditional_if_elif_else() {
  fs::path dir = make_test_dir("conditional_if_elif_else");
  fs::path file = dir / "main.c";
  write_text(file,
             "#if 0\n"
             "int c = 0;\n"
             "#elif 1\n"
             "int c = 1;\n"
             "#else\n"
             "int c = 2;\n"
             "#endif\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int c = 1;", "#elif branch should be selected");
  expect_not_contains(out, "int c = 0;", "#if 0 branch should be inactive");
  expect_not_contains(out, "int c = 2;", "#else branch should be inactive when #elif taken");
}

void test_include_quoted() {
  fs::path dir = make_test_dir("include_quoted");
  fs::path header = dir / "my_header.h";
  fs::path file = dir / "main.c";

  write_text(header,
             "#define MAGIC 42\n"
             "int from_header = MAGIC;\n");
  write_text(file,
             "#include \"my_header.h\"\n"
             "int from_main = MAGIC;\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int from_header = 42;", "quoted include content should be inlined");
  expect_contains(out, "int from_main = 42;", "macros from included file should persist");
}

void test_error_warning_diagnostics() {
  fs::path dir = make_test_dir("error_warning_diagnostics");
  fs::path file = dir / "main.c";

  write_text(file,
             "#warning watch out\n"
             "#error hard fail\n"
             "int ok = 1;\n");

  Preprocessor pp;
  (void)pp.preprocess_file(file.string());
  expect_true(pp.warnings().size() == 1, "should record one #warning");
  expect_true(pp.errors().size() == 1, "should record one #error");
  expect_contains(pp.warnings()[0].message, "watch out", "warning message should be captured");
  expect_contains(pp.errors()[0].message, "hard fail", "error message should be captured");
}

// ---- README-aligned pending skeletons ----
// These tests document target behavior from
// ref/claudes-c-compiler/src/frontend/preprocessor/README.md.
// If current implementation does not satisfy the feature, we record PENDING.

void test_pending_function_like_macro(PendingTracker& pending) {
  fs::path dir = make_test_dir("pending_function_like_macro");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define ADD(a,b) ((a)+(b))\n"
             "int x = ADD(2, 3);\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  if (!probe_feature_contains(out, "int x = ((2)+(3));")) {
    pending.add("function-like macro", "#define F(x) invocation expansion");
  }
}

void test_pending_stringify(PendingTracker& pending) {
  fs::path dir = make_test_dir("pending_stringify");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define S(x) #x\n"
             "const char* s = S(abc);\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  if (!probe_feature_contains(out, "const char* s = \"abc\";")) {
    pending.add("stringification", "# operator in macro body");
  }
}

void test_pending_token_paste(PendingTracker& pending) {
  fs::path dir = make_test_dir("pending_token_paste");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define CAT(a,b) a##b\n"
             "int xy = 7;\n"
             "int z = CAT(x, y);\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  if (!probe_feature_contains(out, "int z = xy;")) {
    pending.add("token pasting", "## operator support");
  }
}

void test_pending_variadic(PendingTracker& pending) {
  fs::path dir = make_test_dir("pending_variadic");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define CALL(f, ...) f(__VA_ARGS__)\n"
             "int r = CALL(sum, 1, 2);\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  if (!probe_feature_contains(out, "int r = sum(1, 2);")) {
    pending.add("variadic macros", "__VA_ARGS__ substitution");
  }
}

void test_pending_if_expr_eval(PendingTracker& pending) {
  fs::path dir = make_test_dir("pending_if_expr_eval");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define A 3\n"
             "#if (A * 2) == 6 && defined(A)\n"
             "int ok = 1;\n"
             "#else\n"
             "int ok = 0;\n"
             "#endif\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  if (!probe_feature_contains(out, "int ok = 1;")) {
    pending.add("if-expression evaluator", "operators/defined() in #if");
  }
}

void test_pending_include_angle(PendingTracker& pending) {
  fs::path dir = make_test_dir("pending_include_angle");
  fs::path file = dir / "main.c";
  write_text(file, "#include <stdint.h>\nint x = 1;\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  // We only check that this path is intentionally tracked. Output content is environment-dependent.
  if (probe_feature_contains(out, "#include <stdint.h>") || out.empty()) {
    pending.add("system include search", "#include <...> path resolution");
  }
}

void test_pending_pragma_once(PendingTracker& pending) {
  fs::path dir = make_test_dir("pending_pragma_once");
  fs::path header = dir / "once.h";
  fs::path file = dir / "main.c";
  write_text(header,
             "#pragma once\n"
             "int v;\n");
  write_text(file,
             "#include \"once.h\"\n"
             "#include \"once.h\"\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  size_t first = out.find("int v;");
  size_t second = (first == std::string::npos) ? std::string::npos : out.find("int v;", first + 1);
  if (first == std::string::npos || second != std::string::npos) {
    pending.add("pragma once", "deduplicate repeated include");
  }
}

void test_pending_line_directive(PendingTracker& pending) {
  fs::path dir = make_test_dir("pending_line_directive");
  fs::path file = dir / "main.c";
  write_text(file,
             "#line 200 \"virt.c\"\n"
             "int x;\n");
  Preprocessor pp;
  (void)pp.preprocess_file(file.string());
  // Current C++ frontend lexer drops '#' lines anyway; we track this as pending
  // until preprocessor exposes stable line-marker output contract.
  pending.add("line markers", "#line mapping and output contract");
}

}  // namespace

int main() {
  try {
    PendingTracker pending;

    test_phase2_line_splice();
    test_phase3_comment_strip();
    test_define_undef_object_macro();
    test_conditional_ifdef_ifndef();
    test_conditional_if_elif_else();
    test_include_quoted();
    test_error_warning_diagnostics();
    test_pending_function_like_macro(pending);
    test_pending_stringify(pending);
    test_pending_token_paste(pending);
    test_pending_variadic(pending);
    test_pending_if_expr_eval(pending);
    test_pending_include_angle(pending);
    test_pending_pragma_once(pending);
    test_pending_line_directive(pending);
    pending.print_summary();
  } catch (const std::exception& ex) {
    std::cerr << "FAIL: exception: " << ex.what() << "\n";
    return 1;
  }

  std::cout << "PASS: frontend_cxx_preprocessor_tests\n";
  return 0;
}
