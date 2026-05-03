#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

std::string read_file(const std::vector<std::string>& paths) {
  std::ifstream in;
  std::string opened;
  for (const std::string& path : paths) {
    in.open(path);
    if (in) {
      opened = path;
      break;
    }
    in.clear();
  }
  if (!in) fail("could not open expressions.cpp from expected build dirs");
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void expect_absent(const std::string& text, const std::string& needle) {
  expect_true(text.find(needle) == std::string::npos,
              "expressions.cpp should not contain direct residual `" + needle +
                  "`");
}

void test_expression_residual_sites_use_metadata_helpers() {
  const std::string source =
      read_file({"../src/frontend/parser/impl/expressions.cpp",
                 "../../src/frontend/parser/impl/expressions.cpp",
                 "../../../src/frontend/parser/impl/expressions.cpp"});

  expect_absent(source, "ts.tag ? ts.tag");
  expect_absent(source, "cast_ts.tag");
  expect_absent(source, "owner_name = ts.tag");

  expect_true(source.find("expressions_typespec_display_name") !=
                  std::string::npos,
              "expression residuals should route display through structured metadata helper");
  expect_true(source.find("expressions_constructor_display_name") !=
                  std::string::npos,
              "constructor display should use explicit final-spelling helper");
  expect_true(source.find("type_spec_mangled_display_name_if_needed") !=
                  std::string::npos,
              "local mangling should use field-detected display fallback");
}

}  // namespace

int main() {
  test_expression_residual_sites_use_metadata_helpers();
  std::cout << "PASS: cpp_hir_parser_expressions_residual_metadata_test\n";
  return 0;
}
