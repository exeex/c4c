#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

std::string read_emit_source() {
  std::ifstream input(std::string(C4C_SOURCE_DIR) +
                      "/src/backend/mir/aarch64/codegen/emit.cpp");
  std::ostringstream out;
  out << input.rdbuf();
  return out.str();
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

int check_fast_paths_no_longer_parse_signature_text() {
  const std::string source = read_emit_source();
  if (source.empty()) {
    return fail("could not read aarch64 emit.cpp");
  }

  if (contains(source, "function.signature_text") ||
      contains(source, "backend_lir_is_zero_arg_i32_definition(function.signature_text)") ||
      contains(source, "backend_lir_is_i32_definition(function.signature_text)") ||
      contains(source,
               "backend_lir_function_signature_uses_nonminimal_types(function.signature_text)")) {
    return fail("aarch64 fast-path predicates still parse function.signature_text");
  }

  if (!contains(source, "lir_function_is_zero_arg_i32_definition(function)") ||
      !contains(source, "lir_function_is_i32_definition(function)") ||
      !contains(source, "lir_function_signature_uses_nonminimal_types(function)")) {
    return fail("aarch64 fast paths are not guarded by structured signature helpers");
  }

  return 0;
}

int check_structured_helpers_cover_signature_authorities() {
  const std::string source = read_emit_source();
  if (!contains(source, "function.signature_return_type_ref.has_value()") ||
      !contains(source, "function.signature_params.size() == expected_count") ||
      !contains(source, "function.signature_param_type_refs.size() ==") ||
      !contains(source, "function.signature_is_variadic") ||
      !contains(source, "function.signature_has_void_param_list") ||
      !contains(source, "backend_lir_type_uses_nonminimal_types(type_ref.str())")) {
    return fail("aarch64 structured signature helpers do not cover return, params, variadic, and nonminimal gates");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = check_fast_paths_no_longer_parse_signature_text(); status != 0) {
    return status;
  }
  if (const int status = check_structured_helpers_cover_signature_authorities(); status != 0) {
    return status;
  }
  return 0;
}
