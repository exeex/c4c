#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "preprocessor.hpp"

namespace fs = std::filesystem;
using c4c::Preprocessor;
using c4c::SourceProfile;

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

void test_anti_paste_guards() {
  fs::path dir = make_test_dir("anti_paste_guards");
  fs::path file = dir / "main.c";

  write_text(file,
             // 1. Parameter substitution: expanded arg ending with + before body +
             "#define FOO(a) a+\n"
             "int x1 = FOO(x+)3;\n"
             // 2. ## with number right operand
             "#define PASTE_NUM(a) a ## 1\n"
             "int PASTE_NUM(var);\n"
             // 3. ## with punctuation right operand
             "#define PASTE_EQ(a) a ## =\n"
             "int PASTE_EQ(x);\n"
             // 4. Anti-paste at macro expansion boundary with following text
             "#define BAR(a, b) a b\n"
             "int x4 = BAR(1, +)+3;\n"
             // 5. ## with 0x prefix in body pasted with identifier arg
             "#define HEX(a) 0x ## a\n"
             "int x5 = HEX(FF);\n"
             );

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "x+ +3", "anti-paste: FOO(x+)3 should not form x++");
  expect_contains(out, "int var1;", "## with number right operand");
  expect_contains(out, "int x=;", "## with punctuation right operand");
  expect_contains(out, "1 + +3", "anti-paste: BAR(1, +)+3 should not form ++");
  expect_contains(out, "int x5 = 0xFF;", "## with hex prefix body");
}

void test_multiline_funclike_invocation() {
  fs::path dir = make_test_dir("multiline_funclike_invocation");
  fs::path file = dir / "main.c";

  write_text(file,
             "#define ADD(a, b) ((a) + (b))\n"
             "int x = ADD(1,\n"
             "            2);\n"
             "int y = ADD(\n"
             "  10,\n"
             "  20\n"
             ");\n");

  Preprocessor pp;
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "((1) + (2))", "multi-line macro: two-line invocation");
  expect_contains(out, "((10) + (20))", "multi-line macro: four-line invocation");
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

void test_predefined_no_structured_binding_macro() {
  fs::path dir = make_test_dir("predefined_no_structured_binding_macro");
  fs::path file = dir / "main.cpp";

  write_text(file,
             "#if defined(EA_COMPILER_NO_STRUCTURED_BINDING)\n"
             "int no_structured_binding = EA_COMPILER_NO_STRUCTURED_BINDING;\n"
             "#else\n"
             "int no_structured_binding = 0;\n"
             "#endif\n");

  Preprocessor pp;
  pp.set_source_profile(SourceProfile::CppSubset);
  std::string out = pp.preprocess_file(file.string());
  expect_contains(out, "int no_structured_binding = 1;",
                  "EA_COMPILER_NO_STRUCTURED_BINDING should be predefined for C++ profiles");
  expect_not_contains(out, "int no_structured_binding = 0;",
                      "EA_COMPILER_NO_STRUCTURED_BINDING false branch should be inactive");
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

void test_missing_angle_include_reports_error() {
  fs::path dir = make_test_dir("missing_angle_include_reports_error");
  fs::path file = dir / "main.c";
  write_text(file,
             "#include <definitely_missing_header_for_c4c_tests.h>\n"
             "int x = 1;\n");

  Preprocessor pp;
  (void)pp.preprocess_file(file.string());
  expect_true(pp.errors().size() == 1, "missing <...> include should record one error");
  expect_contains(pp.errors()[0].message, "include file not found: definitely_missing_header_for_c4c_tests.h",
                  "missing <...> include should surface a direct diagnostic");
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

void test_config_toggle_macros() {
  // Test __OPTIMIZE__: defined when optimization is enabled
  {
    fs::path dir = make_test_dir("config_toggle_optimize");
    fs::path file = dir / "main.c";
    write_text(file,
               "#ifdef __OPTIMIZE__\n"
               "int opt = 1;\n"
               "#else\n"
               "int opt = 0;\n"
               "#endif\n"
               "#ifdef __NO_INLINE__\n"
               "int noinline = 1;\n"
               "#else\n"
               "int noinline = 0;\n"
               "#endif\n");

    // Default (no optimization): __OPTIMIZE__ not defined, __NO_INLINE__ defined
    {
      Preprocessor pp;
      std::string out = pp.preprocess_file(file.string());
      expect_contains(out, "int opt = 0;",
                      "default: __OPTIMIZE__ should not be defined");
      expect_contains(out, "int noinline = 1;",
                      "default: __NO_INLINE__ should be defined");
    }

    // With optimization: __OPTIMIZE__ defined, __NO_INLINE__ not defined
    {
      Preprocessor pp;
      pp.define_macro("__OPTIMIZE__");
      pp.undefine_macro("__NO_INLINE__");
      std::string out = pp.preprocess_file(file.string());
      expect_contains(out, "int opt = 1;",
                      "optimizing: __OPTIMIZE__ should be defined");
      expect_contains(out, "int noinline = 0;",
                      "optimizing: __NO_INLINE__ should not be defined");
    }
  }

  // Test __OPTIMIZE_SIZE__
  {
    fs::path dir = make_test_dir("config_toggle_opt_size");
    fs::path file = dir / "main.c";
    write_text(file,
               "#ifdef __OPTIMIZE_SIZE__\n"
               "int os = 1;\n"
               "#else\n"
               "int os = 0;\n"
               "#endif\n");

    Preprocessor pp;
    pp.define_macro("__OPTIMIZE_SIZE__");
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int os = 1;",
                    "__OPTIMIZE_SIZE__ should be definable");
  }

  // Test __PIC__ / __pic__
  {
    fs::path dir = make_test_dir("config_toggle_pic");
    fs::path file = dir / "main.c";
    write_text(file,
               "#ifdef __PIC__\n"
               "int pic = __PIC__;\n"
               "#else\n"
               "int pic = 0;\n"
               "#endif\n"
               "#ifdef __pic__\n"
               "int pic_lower = __pic__;\n"
               "#else\n"
               "int pic_lower = 0;\n"
               "#endif\n");

    // Default: no PIC
    {
      Preprocessor pp;
      std::string out = pp.preprocess_file(file.string());
      expect_contains(out, "int pic = 0;",
                      "default: __PIC__ should not be defined");
      expect_contains(out, "int pic_lower = 0;",
                      "default: __pic__ should not be defined");
    }

    // -fPIC: level 2
    {
      Preprocessor pp;
      pp.define_macro("__PIC__=2");
      pp.define_macro("__pic__=2");
      std::string out = pp.preprocess_file(file.string());
      expect_contains(out, "int pic = 2;",
                      "-fPIC: __PIC__ should be 2");
      expect_contains(out, "int pic_lower = 2;",
                      "-fPIC: __pic__ should be 2");
    }
  }

  // Test __PIE__ / __pie__
  {
    fs::path dir = make_test_dir("config_toggle_pie");
    fs::path file = dir / "main.c";
    write_text(file,
               "#ifdef __PIE__\n"
               "int pie = __PIE__;\n"
               "#else\n"
               "int pie = 0;\n"
               "#endif\n");

    Preprocessor pp;
    pp.define_macro("__PIE__=2");
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int pie = 2;",
                    "__PIE__ should be definable with level");
  }
}

// ---------- #include_next regression tests ----------

void test_include_next() {
  // Test 1: basic #include_next skips the current directory and finds the
  // header in the next search path.
  {
    fs::path dir = make_test_dir("include_next_basic");
    fs::path dir_a = dir / "a";
    fs::path dir_b = dir / "b";
    fs::create_directories(dir_a);
    fs::create_directories(dir_b);

    // dir_a/wrap.h uses #include_next to forward to dir_b/wrap.h
    write_text(dir_a / "wrap.h",
               "#define FROM_A 100\n"
               "#include_next \"wrap.h\"\n");
    write_text(dir_b / "wrap.h",
               "#define FROM_B 200\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include \"wrap.h\"\n"
               "int a = FROM_A;\n"
               "int b = FROM_B;\n");

    Preprocessor pp;
    pp.add_include_path(dir_a.string());
    pp.add_include_path(dir_b.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int a = 100;",
                    "#include_next: macro from first header should be visible");
    expect_contains(out, "int b = 200;",
                    "#include_next: macro from next header should be visible");
  }

  // Test 2: #include_next with angle brackets
  {
    fs::path dir = make_test_dir("include_next_angle");
    fs::path dir_a = dir / "a";
    fs::path dir_b = dir / "b";
    fs::create_directories(dir_a);
    fs::create_directories(dir_b);

    write_text(dir_a / "hdr.h",
               "#define ANGLE_A 10\n"
               "#include_next <hdr.h>\n");
    write_text(dir_b / "hdr.h",
               "#define ANGLE_B 20\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include <hdr.h>\n"
               "int a = ANGLE_A;\n"
               "int b = ANGLE_B;\n");

    Preprocessor pp;
    pp.add_include_path(dir_a.string());
    pp.add_include_path(dir_b.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int a = 10;",
                    "#include_next angle: first header macro");
    expect_contains(out, "int b = 20;",
                    "#include_next angle: next header macro");
  }

  // Test 3: chained #include_next across three directories
  {
    fs::path dir = make_test_dir("include_next_chain");
    fs::path dir_a = dir / "a";
    fs::path dir_b = dir / "b";
    fs::path dir_c = dir / "c";
    fs::create_directories(dir_a);
    fs::create_directories(dir_b);
    fs::create_directories(dir_c);

    write_text(dir_a / "chain.h",
               "#define CHAIN_A 1\n"
               "#include_next \"chain.h\"\n");
    write_text(dir_b / "chain.h",
               "#define CHAIN_B 2\n"
               "#include_next \"chain.h\"\n");
    write_text(dir_c / "chain.h",
               "#define CHAIN_C 3\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include \"chain.h\"\n"
               "int a = CHAIN_A;\n"
               "int b = CHAIN_B;\n"
               "int c = CHAIN_C;\n");

    Preprocessor pp;
    pp.add_include_path(dir_a.string());
    pp.add_include_path(dir_b.string());
    pp.add_include_path(dir_c.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int a = 1;", "#include_next chain: A");
    expect_contains(out, "int b = 2;", "#include_next chain: B");
    expect_contains(out, "int c = 3;", "#include_next chain: C");
  }

  // Test 4: #include_next across different bucket types (normal → system)
  {
    fs::path dir = make_test_dir("include_next_cross_bucket");
    fs::path normal_dir = dir / "normal";
    fs::path system_dir = dir / "system";
    fs::create_directories(normal_dir);
    fs::create_directories(system_dir);

    write_text(normal_dir / "cross.h",
               "#define CROSS_NORMAL 10\n"
               "#include_next \"cross.h\"\n");
    write_text(system_dir / "cross.h",
               "#define CROSS_SYSTEM 20\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include \"cross.h\"\n"
               "int n = CROSS_NORMAL;\n"
               "int s = CROSS_SYSTEM;\n");

    Preprocessor pp;
    pp.add_include_path(normal_dir.string());
    pp.add_system_include_path(system_dir.string());
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int n = 10;",
                    "#include_next cross-bucket: normal header macro");
    expect_contains(out, "int s = 20;",
                    "#include_next cross-bucket: system header macro via include_next");
  }
}

// ---------- Include resolution cache regression tests ----------

void test_include_resolution_cache() {
  // Test 1: same header included twice — second inclusion should hit cache
  // and produce identical results.
  {
    fs::path dir = make_test_dir("include_cache_basic");
    fs::path header = dir / "cached.h";
    write_text(header, "int cached_val;\n");

    fs::path file = dir / "main.c";
    // Include twice — no guard, so content appears twice.
    // The key test is that it doesn't error or produce wrong content.
    write_text(file,
               "#include \"cached.h\"\n"
               "#include \"cached.h\"\n"
               "int end_marker;\n");

    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    // Content should appear (at least once — guard-free file gets included twice)
    expect_contains(out, "int cached_val;",
                    "cache: header content should be included");
    expect_contains(out, "int end_marker;",
                    "cache: main file content should be present");
  }

  // Test 2: same relative path from different directories resolves correctly
  // (quoted includes use directory-qualified cache keys).
  {
    fs::path dir = make_test_dir("include_cache_dir_qualified");
    fs::path sub_a = dir / "a";
    fs::path sub_b = dir / "b";
    fs::create_directories(sub_a);
    fs::create_directories(sub_b);

    write_text(sub_a / "common.h", "#define COMMON_VAL 111\n");
    write_text(sub_b / "common.h", "#define COMMON_VAL 222\n");

    // File in dir_a includes "common.h" — should get dir_a's version
    write_text(sub_a / "use_a.c",
               "#include \"common.h\"\n"
               "int va = COMMON_VAL;\n");

    Preprocessor pp_a;
    std::string out_a = pp_a.preprocess_file((sub_a / "use_a.c").string());
    expect_contains(out_a, "int va = 111;",
                    "cache dir-qualified: dir_a's common.h should resolve to 111");

    // Separate pp instance — file in dir_b gets dir_b's version
    Preprocessor pp_b;
    write_text(sub_b / "use_b.c",
               "#include \"common.h\"\n"
               "int vb = COMMON_VAL;\n");
    std::string out_b = pp_b.preprocess_file((sub_b / "use_b.c").string());
    expect_contains(out_b, "int vb = 222;",
                    "cache dir-qualified: dir_b's common.h should resolve to 222");
  }

  // Test 3: angle include cache key is directory-independent
  {
    fs::path dir = make_test_dir("include_cache_angle");
    fs::path inc_dir = dir / "inc";
    fs::create_directories(inc_dir);

    write_text(inc_dir / "sys.h", "#define SYS_VAL 999\n");

    // Include from two different source files in different dirs
    fs::path src_a = dir / "a";
    fs::path src_b = dir / "b";
    fs::create_directories(src_a);
    fs::create_directories(src_b);

    write_text(src_a / "main.c",
               "#include <sys.h>\n"
               "int sa = SYS_VAL;\n");
    write_text(src_b / "other.c",
               "#include <sys.h>\n"
               "int sb = SYS_VAL;\n");

    // Same pp instance — second angle include should hit cache
    Preprocessor pp;
    pp.add_include_path(inc_dir.string());
    std::string out_a = pp.preprocess_file((src_a / "main.c").string());
    expect_contains(out_a, "int sa = 999;",
                    "cache angle: first inclusion works");
  }
}

// ---------- Include guard optimization regression tests ----------

void test_include_guard_optimization() {
  // Test 1: classic #ifndef/#define guard — second include is skipped
  {
    fs::path dir = make_test_dir("include_guard_basic");
    fs::path header = dir / "guarded.h";
    write_text(header,
               "#ifndef GUARDED_H\n"
               "#define GUARDED_H\n"
               "int guarded_decl;\n"
               "#endif\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include \"guarded.h\"\n"
               "#include \"guarded.h\"\n"
               "int end;\n");

    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    expect_contains(out, "int guarded_decl;",
                    "guard: content should appear on first include");
    expect_contains(out, "int end;",
                    "guard: main file content should be present");

    // Count occurrences — should appear exactly once
    size_t first = out.find("int guarded_decl;");
    size_t second = out.find("int guarded_decl;", first + 1);
    expect_true(second == std::string::npos,
                "guard: guarded content should appear only once (guard skip on re-include)");
  }

  // Test 2: guard with content before #ifndef — guard detection should fail,
  // so file gets included twice.
  {
    fs::path dir = make_test_dir("include_guard_content_before");
    fs::path header = dir / "bad_guard.h";
    write_text(header,
               "int before_guard;\n"
               "#ifndef BAD_GUARD_H\n"
               "#define BAD_GUARD_H\n"
               "int inside_guard;\n"
               "#endif\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include \"bad_guard.h\"\n"
               "#include \"bad_guard.h\"\n"
               "int end;\n");

    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    // "before_guard" is outside the guard, so it should appear twice
    size_t first = out.find("int before_guard;");
    size_t second = out.find("int before_guard;", first + 1);
    expect_true(second != std::string::npos,
                "guard: content before #ifndef means no guard optimization — should appear twice");
  }

  // Test 3: guard macro #undef'd between includes — file should be re-included
  {
    fs::path dir = make_test_dir("include_guard_undef");
    fs::path header = dir / "undef_guard.h";
    write_text(header,
               "#ifndef UNDEF_GUARD_H\n"
               "#define UNDEF_GUARD_H\n"
               "int undef_guarded;\n"
               "#endif\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include \"undef_guard.h\"\n"
               "#undef UNDEF_GUARD_H\n"
               "#include \"undef_guard.h\"\n"
               "int end;\n");

    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    // After #undef, the guard macro is gone, so second include should work
    size_t first = out.find("int undef_guarded;");
    size_t second = out.find("int undef_guarded;", first + 1);
    expect_true(second != std::string::npos,
                "guard: after #undef of guard macro, re-include should work");
  }

  // Test 4: pragma once and guard coexistence
  {
    fs::path dir = make_test_dir("include_guard_pragma_once");
    fs::path header = dir / "both.h";
    write_text(header,
               "#pragma once\n"
               "#ifndef BOTH_H\n"
               "#define BOTH_H\n"
               "int both_guarded;\n"
               "#endif\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include \"both.h\"\n"
               "#include \"both.h\"\n"
               "int end;\n");

    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    size_t first = out.find("int both_guarded;");
    size_t second = out.find("int both_guarded;", first + 1);
    expect_true(second == std::string::npos,
                "guard+pragma: content should appear only once");
  }

  // Test 5: nested #ifdef inside guard — should still detect outer guard
  {
    fs::path dir = make_test_dir("include_guard_nested");
    fs::path header = dir / "nested.h";
    write_text(header,
               "#ifndef NESTED_H\n"
               "#define NESTED_H\n"
               "#ifdef __linux__\n"
               "int linux_decl;\n"
               "#endif\n"
               "int common_decl;\n"
               "#endif\n");

    fs::path file = dir / "main.c";
    write_text(file,
               "#include \"nested.h\"\n"
               "#include \"nested.h\"\n"
               "int end;\n");

    Preprocessor pp;
    std::string out = pp.preprocess_file(file.string());
    size_t first = out.find("int common_decl;");
    size_t second = out.find("int common_decl;", first + 1);
    expect_true(second == std::string::npos,
                "guard nested: guard detection should work with nested #ifdef");
  }
}

// ---------- CLI integration tests for include flags ----------

void test_cli_include_flags() {
  // These tests invoke c4cll --pp-only as a subprocess to verify
  // that -I, -isystem, -iquote, -idirafter flags are wired correctly.
  fs::path dir = make_test_dir("cli_include_flags");

  // Find the c4cll binary
  fs::path c4cll;
  if (fs::exists("build/c4cll")) {
    c4cll = "build/c4cll";
  } else if (fs::exists("../build/c4cll")) {
    c4cll = "../build/c4cll";
  } else {
    // Try to find it relative to the test binary
    c4cll = fs::path(__FILE__).parent_path().parent_path().parent_path() / "build" / "c4cll";
  }

  if (!fs::exists(c4cll)) {
    std::cerr << "SKIP: test_cli_include_flags — c4cll binary not found\n";
    return;
  }

  auto run_cmd = [](const std::string& cmd) -> std::pair<int, std::string> {
    std::string full_cmd = cmd + " 2>&1";
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) return {-1, ""};
    char buf[4096];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) {
      result += buf;
    }
    int status = pclose(pipe);
    return {WEXITSTATUS(status), result};
  };

  // Test 1: -I flag
  {
    fs::path inc = dir / "cli_I";
    fs::create_directories(inc);
    write_text(inc / "cli_header.h", "#define CLI_I_VAL 42\n");

    fs::path src = dir / "cli_I_test.c";
    write_text(src,
               "#include <cli_header.h>\n"
               "int x = CLI_I_VAL;\n");

    auto [rc, out] = run_cmd(c4cll.string() + " --pp-only -I " + inc.string() + " " + src.string());
    expect_true(rc == 0, "CLI -I: should succeed (exit code 0), got: " + std::to_string(rc));
    expect_contains(out, "int x = 42;", "CLI -I: header macro should be expanded");
  }

  // Test 2: -isystem flag
  {
    fs::path sys = dir / "cli_isystem";
    fs::create_directories(sys);
    write_text(sys / "sys_header.h", "#define SYS_CLI_VAL 77\n");

    fs::path src = dir / "cli_isystem_test.c";
    write_text(src,
               "#include <sys_header.h>\n"
               "int x = SYS_CLI_VAL;\n");

    auto [rc, out] = run_cmd(c4cll.string() + " --pp-only -isystem " + sys.string() + " " + src.string());
    expect_true(rc == 0, "CLI -isystem: should succeed");
    expect_contains(out, "int x = 77;", "CLI -isystem: system header macro should be expanded");
  }

  // Test 3: -iquote flag
  {
    fs::path qt = dir / "cli_iquote";
    fs::create_directories(qt);
    write_text(qt / "quote_header.h", "#define QUOTE_CLI_VAL 55\n");

    fs::path src = dir / "cli_iquote_test.c";
    write_text(src,
               "#include \"quote_header.h\"\n"
               "int x = QUOTE_CLI_VAL;\n");

    auto [rc, out] = run_cmd(c4cll.string() + " --pp-only -iquote " + qt.string() + " " + src.string());
    expect_true(rc == 0, "CLI -iquote: should succeed");
    expect_contains(out, "int x = 55;", "CLI -iquote: quote header macro should be expanded");
  }

  // Test 4: -idirafter flag
  {
    fs::path aft = dir / "cli_idirafter";
    fs::create_directories(aft);
    write_text(aft / "after_header.h", "#define AFTER_CLI_VAL 33\n");

    fs::path src = dir / "cli_idirafter_test.c";
    write_text(src,
               "#include <after_header.h>\n"
               "int x = AFTER_CLI_VAL;\n");

    auto [rc, out] = run_cmd(c4cll.string() + " --pp-only -idirafter " + aft.string() + " " + src.string());
    expect_true(rc == 0, "CLI -idirafter: should succeed");
    expect_contains(out, "int x = 33;", "CLI -idirafter: after header macro should be expanded");
  }

  // Test 5: -I takes precedence over -isystem
  {
    fs::path inc = dir / "cli_prio_I";
    fs::path sys = dir / "cli_prio_sys";
    fs::create_directories(inc);
    fs::create_directories(sys);
    write_text(inc / "prio.h", "#define PRIO_CLI 1\n");
    write_text(sys / "prio.h", "#define PRIO_CLI 2\n");

    fs::path src = dir / "cli_prio_test.c";
    write_text(src,
               "#include <prio.h>\n"
               "int x = PRIO_CLI;\n");

    auto [rc, out] = run_cmd(c4cll.string() + " --pp-only -I " + inc.string() +
                             " -isystem " + sys.string() + " " + src.string());
    expect_true(rc == 0, "CLI priority: should succeed");
    expect_contains(out, "int x = 1;", "CLI priority: -I should take precedence over -isystem");
  }

  // Test 6: -D flag combined with include
  {
    fs::path inc = dir / "cli_D_inc";
    fs::create_directories(inc);
    write_text(inc / "use_def.h",
               "#ifdef MY_DEF\n"
               "int defined_val = MY_DEF;\n"
               "#endif\n");

    fs::path src = dir / "cli_D_test.c";
    write_text(src,
               "#include <use_def.h>\n"
               "int end;\n");

    auto [rc, out] = run_cmd(c4cll.string() + " --pp-only -DMY_DEF=123 -I " +
                             inc.string() + " " + src.string());
    expect_true(rc == 0, "CLI -D+include: should succeed");
    expect_contains(out, "int defined_val = 123;",
                    "CLI -D+include: -D macro should be visible inside included header");
  }

  // Test 7: CPLUS_INCLUDE_PATH should seed default system include search for C++ input
  {
    fs::path sys = dir / "cli_env_cplus";
    fs::create_directories(sys);
    write_text(sys / "env_header.hpp", "#define ENV_HEADER_VAL 77\n");

    fs::path src = dir / "cli_env_test.cpp";
    write_text(src,
               "#include <env_header.hpp>\n"
               "int x = ENV_HEADER_VAL;\n");

    auto [rc, out] = run_cmd("env CPLUS_INCLUDE_PATH=" + sys.string() + " " +
                             c4cll.string() + " --pp-only " + src.string());
    expect_true(rc == 0, "CLI env include path: should succeed");
    expect_contains(out, "int x = 77;",
                    "CLI env include path: CPLUS_INCLUDE_PATH should be searched automatically");
  }
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
    test_missing_angle_include_reports_error();
    test_has_include();
    test_gnu_named_variadic();
    test_define_undefine_api();
    test_preprocess_source();
    test_builtin_location_macros();
    test_line_markers();
    test_error_warning_diagnostics();
    test_macro_rescan_uses_following_source_tokens();
    test_token_paste_empty_operand_preserves_boundary();
    test_anti_paste_guards();
    test_multiline_funclike_invocation();
    test_predefined_lp64_macro();
    test_predefined_no_structured_binding_macro();
    test_line_directive();
    test_config_toggle_macros();
    test_include_next();
    test_include_resolution_cache();
    test_include_guard_optimization();
    test_cli_include_flags();
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
