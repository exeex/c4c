#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "preprocessor.hpp"

namespace fs = std::filesystem;
using tinyc2ll::frontend_cxx::Preprocessor;

namespace {

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

}  // namespace

int main() {
  try {
    test_phase2_line_splice();
    test_phase3_comment_strip();
    test_define_undef_object_macro();
    test_conditional_ifdef_ifndef();
    test_conditional_if_elif_else();
    test_include_quoted();
    test_error_warning_diagnostics();
  } catch (const std::exception& ex) {
    std::cerr << "FAIL: exception: " << ex.what() << "\n";
    return 1;
  }

  std::cout << "PASS: frontend_cxx_preprocessor_tests\n";
  return 0;
}
