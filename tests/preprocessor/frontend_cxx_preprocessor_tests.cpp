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

void test_macro_rescan_uses_following_source_tokens() {
  fs::path dir = make_test_dir("macro_rescan_following_tokens");
  fs::path file = dir / "main.c";

  write_text(file,
             "#define CAT2(a,b) a##b\n"
             "#define CAT(a,b) CAT2(a,b)\n"
             "#define AB(x) CAT(x,y)\n"
             "int xy = 42;\n"
             "int z = CAT(A,B)(x);\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int z = xy;", "rescan should expand a function-like macro introduced by replacement");
}

void test_token_paste_empty_operand_preserves_boundary() {
  fs::path dir = make_test_dir("token_paste_empty_operand_boundary");
  fs::path file = dir / "main.c";

  write_text(file,
             "#define Q(A,B) A ## B+\n"
             "int x = 60 Q(+,)3;\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int x = 60 + +3;", "empty-operand token paste should not accidentally form ++");
}

void test_predefined_lp64_macro() {
  fs::path dir = make_test_dir("predefined_lp64_macro");
  fs::path file = dir / "main.c";

  write_text(file,
             "#if defined(__LP64__)\n"
             "int lp64 = 1;\n"
             "#else\n"
             "int lp64 = 0;\n"
             "#endif\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int lp64 = 1;", "__LP64__ should be predefined on the current target model");
  expect_not_contains(out, "int lp64 = 0;", "__LP64__ false branch should be inactive");
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

void test_include_path_buckets() {
  // Test the GCC-compatible include search order:
  //   #include "file.h": current dir → quote → normal → system → after
  //   #include <file.h>: normal → system → after
  fs::path dir = make_test_dir("include_path_buckets");

  // Create subdirectories for each bucket
  fs::path quote_dir = dir / "quote";
  fs::path normal_dir = dir / "normal";
  fs::path system_dir = dir / "system";
  fs::path after_dir = dir / "after";
  fs::create_directories(quote_dir);
  fs::create_directories(normal_dir);
  fs::create_directories(system_dir);
  fs::create_directories(after_dir);

  // Place distinct headers in each bucket
  write_text(quote_dir / "q.h", "#define FROM_QUOTE 1\n");
  write_text(normal_dir / "n.h", "#define FROM_NORMAL 1\n");
  write_text(system_dir / "s.h", "#define FROM_SYSTEM 1\n");
  write_text(after_dir / "a.h", "#define FROM_AFTER 1\n");

  // Test 1: quoted include finds file in quote path
  {
    fs::path file = dir / "test_quote.c";
    write_text(file, "#include \"q.h\"\nint x = FROM_QUOTE;\n");
    Preprocessor pp;
    pp.add_quote_include_path(quote_dir.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 1;", "quoted include should find file in quote path");
  }

  // Test 2: angle include finds file in normal path
  {
    fs::path file = dir / "test_normal.c";
    write_text(file, "#include <n.h>\nint x = FROM_NORMAL;\n");
    Preprocessor pp;
    pp.add_include_path(normal_dir.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 1;", "angle include should find file in normal (-I) path");
  }

  // Test 3: angle include finds file in system path
  {
    fs::path file = dir / "test_system.c";
    write_text(file, "#include <s.h>\nint x = FROM_SYSTEM;\n");
    Preprocessor pp;
    pp.add_system_include_path(system_dir.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 1;", "angle include should find file in system path");
  }

  // Test 4: angle include finds file in after path
  {
    fs::path file = dir / "test_after.c";
    write_text(file, "#include <a.h>\nint x = FROM_AFTER;\n");
    Preprocessor pp;
    pp.add_after_include_path(after_dir.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 1;", "angle include should find file in after path");
  }

  // Test 5: search order priority — normal before system before after
  {
    write_text(normal_dir / "priority.h", "#define PRIO 10\n");
    write_text(system_dir / "priority.h", "#define PRIO 20\n");
    write_text(after_dir / "priority.h", "#define PRIO 30\n");

    fs::path file = dir / "test_priority.c";
    write_text(file, "#include <priority.h>\nint x = PRIO;\n");

    Preprocessor pp;
    pp.add_include_path(normal_dir.string());
    pp.add_system_include_path(system_dir.string());
    pp.add_after_include_path(after_dir.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 10;",
                    "normal path should have priority over system and after");
  }

  // Test 6: quoted include searches quote paths before normal paths
  {
    write_text(quote_dir / "qprio.h", "#define QPRIO 1\n");
    write_text(normal_dir / "qprio.h", "#define QPRIO 2\n");

    fs::path file = dir / "test_qprio.c";
    write_text(file, "#include \"qprio.h\"\nint x = QPRIO;\n");

    Preprocessor pp;
    pp.add_quote_include_path(quote_dir.string());
    pp.add_include_path(normal_dir.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 1;",
                    "quoted include should search quote paths before normal paths");
  }

  // Test 7: quoted include falls back to current dir first
  {
    fs::path subdir = dir / "curdir_test";
    fs::create_directories(subdir);
    write_text(subdir / "local.h", "#define LOCAL 99\n");
    fs::path file = subdir / "main.c";
    write_text(file, "#include \"local.h\"\nint x = LOCAL;\n");

    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 99;",
                    "quoted include should search current dir first (no paths configured)");
  }
}

void test_define_undefine_api() {
  fs::path dir = make_test_dir("define_undefine_api");

  // Test 1: define_macro("FOO") defines FOO as 1
  {
    fs::path file = dir / "test1.c";
    write_text(file, "int x = FOO;\n");
    Preprocessor pp;
    pp.define_macro("FOO");
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 1;", "define_macro(\"FOO\") should define FOO as 1");
  }

  // Test 2: define_macro("FOO=42") defines FOO as 42
  {
    fs::path file = dir / "test2.c";
    write_text(file, "int x = FOO;\n");
    Preprocessor pp;
    pp.define_macro("FOO=42");
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int x = 42;", "define_macro(\"FOO=42\") should define FOO as 42");
  }

  // Test 3: undefine_macro removes a predefined macro
  {
    fs::path file = dir / "test3.c";
    write_text(file,
               "#ifdef __STDC__\n"
               "int stdc = 1;\n"
               "#else\n"
               "int stdc = 0;\n"
               "#endif\n");
    Preprocessor pp;
    pp.undefine_macro("__STDC__");
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int stdc = 0;",
                    "undefine_macro should remove predefined macros");
  }

  // Test 4: driver-defined macro visible in #if
  {
    fs::path file = dir / "test4.c";
    write_text(file,
               "#if LEVEL > 2\n"
               "int high = 1;\n"
               "#else\n"
               "int high = 0;\n"
               "#endif\n");
    Preprocessor pp;
    pp.define_macro("LEVEL=5");
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int high = 1;",
                    "driver-defined macro should be usable in #if expressions");
  }
}

void test_computed_include() {
  fs::path dir = make_test_dir("computed_include");
  fs::path header = dir / "target.h";
  fs::path file = dir / "main.c";
  write_text(header, "#define FROM_TARGET 42\n");
  write_text(file,
             "#define HEADER \"target.h\"\n"
             "#include HEADER\n"
             "int x = FROM_TARGET;\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int x = 42;", "computed include should expand macro then include");
}

void test_gnu_named_variadic() {
  fs::path dir = make_test_dir("gnu_named_variadic");
  fs::path file = dir / "main.c";
  write_text(file,
             "#define debug(fmt, args...) call(fmt, args)\n"
             "int r = debug(\"hello\", 1, 2);\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int r = call(\"hello\", 1, 2);",
                  "GNU named variadic should substitute named args");
}

void test_has_include() {
  fs::path dir = make_test_dir("has_include");
  fs::path header = dir / "exists.h";
  fs::path file = dir / "main.c";
  write_text(header, "// exists\n");
  write_text(file,
             "#if __has_include(\"exists.h\")\n"
             "int found = 1;\n"
             "#else\n"
             "int found = 0;\n"
             "#endif\n"
             "#if __has_include(\"nope.h\")\n"
             "int missing = 1;\n"
             "#else\n"
             "int missing = 0;\n"
             "#endif\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int found = 1;",
                  "__has_include should return true for existing file");
  expect_contains(out, "int missing = 0;",
                  "__has_include should return false for non-existing file");
}

void test_preprocess_source() {
  // preprocess_source() preprocesses from a string instead of a file
  {
    Preprocessor pp;
    std::string out = pp.preprocess_source("#define X 42\nint x = X;\n", "test.c");
    expect_contains(out, "int x = 42;", "preprocess_source should expand macros");
    expect_contains(out, "# 1 \"test.c\"", "preprocess_source should emit line marker with given filename");
  }

  // Default filename is <stdin>
  {
    Preprocessor pp;
    std::string out = pp.preprocess_source("int y;\n");
    expect_contains(out, "# 1 \"<stdin>\"", "preprocess_source default filename should be <stdin>");
  }
}

void test_builtin_location_macros() {
  // Test __BASE_FILE__
  {
    fs::path dir = make_test_dir("builtin_location_macros");
    fs::path header = dir / "inc.h";
    fs::path file = dir / "main.c";
    write_text(header, "const char* base_in_header = __BASE_FILE__;\n");
    write_text(file,
               "#include \"inc.h\"\n"
               "const char* base_in_main = __BASE_FILE__;\n");
    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    // __BASE_FILE__ should always be the top-level file, even inside includes
    expect_contains(out, "const char* base_in_header = \"" + file.string() + "\";",
                    "__BASE_FILE__ in included file should refer to top-level file");
    expect_contains(out, "const char* base_in_main = \"" + file.string() + "\";",
                    "__BASE_FILE__ in main file should refer to itself");
  }

  // Test __COUNTER__
  {
    fs::path dir = make_test_dir("builtin_counter");
    fs::path file = dir / "main.c";
    write_text(file,
               "int a = __COUNTER__;\n"
               "int b = __COUNTER__;\n"
               "int c = __COUNTER__;\n");
    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int a = 0;", "__COUNTER__ should start at 0");
    expect_contains(out, "int b = 1;", "__COUNTER__ should increment to 1");
    expect_contains(out, "int c = 2;", "__COUNTER__ should increment to 2");
  }
}

void test_line_markers() {
  // Test initial line marker
  {
    fs::path dir = make_test_dir("line_markers_initial");
    fs::path file = dir / "main.c";
    write_text(file, "int x;\n");
    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "# 1 \"" + file.string() + "\"",
                    "output should start with initial line marker");
  }

  // Test include enter/return markers
  {
    fs::path dir = make_test_dir("line_markers_include");
    fs::path header = dir / "h.h";
    fs::path file = dir / "main.c";
    write_text(header, "int from_header;\n");
    write_text(file,
               "int before;\n"
               "#include \"h.h\"\n"
               "int after;\n");
    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    // Include enter marker: # 1 "h.h" 1
    expect_contains(out, "# 1 \"" + header.string() + "\" 1",
                    "include enter marker should be emitted");
    // Include return marker: # 3 "main.c" 2
    expect_contains(out, "# 3 \"" + file.string() + "\" 2",
                    "include return marker should be emitted");
  }
}

void test_line_directive() {
  fs::path dir = make_test_dir("line_directive");
  fs::path file = dir / "main.c";
  write_text(file,
             "#line 200 \"virt.c\"\n"
             "int x;\n");
  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "# 200 \"virt.c\"",
                  "#line directive should emit GCC line marker");
  expect_contains(out, "int x;", "#line directive should preserve following code");
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
    test_include_path_buckets();
    test_computed_include();
    test_has_include();
    test_gnu_named_variadic();
    test_define_undefine_api();
    test_preprocess_source();
    test_builtin_location_macros();
    test_line_markers();
    test_error_warning_diagnostics();
    test_macro_rescan_uses_following_source_tokens();
    test_token_paste_empty_operand_preserves_boundary();
    test_predefined_lp64_macro();
    test_line_directive();
    test_pending_function_like_macro(pending);
    test_pending_stringify(pending);
    test_pending_token_paste(pending);
    test_pending_variadic(pending);
    test_pending_if_expr_eval(pending);
    test_pending_include_angle(pending);
    test_pending_pragma_once(pending);
    pending.print_summary();
  } catch (const std::exception& ex) {
    std::cerr << "FAIL: exception: " << ex.what() << "\n";
    return 1;
  }

  std::cout << "PASS: frontend_cxx_preprocessor_tests\n";
  return 0;
}
