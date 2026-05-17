#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

std::string read_source(std::string_view relative_path) {
  std::ifstream input(std::string(C4C_SOURCE_DIR) + "/" + std::string(relative_path));
  std::ostringstream out;
  out << input.rdbuf();
  return out.str();
}

std::string read_aarch64_compiled_sources() {
  const std::vector<std::string_view> sources = {
      "src/backend/mir/aarch64/abi/abi.cpp",
      "src/backend/mir/aarch64/abi/abi.hpp",
      "src/backend/mir/aarch64/api/api.cpp",
      "src/backend/mir/aarch64/api/api.hpp",
      "src/backend/mir/aarch64/codegen/alu.cpp",
      "src/backend/mir/aarch64/codegen/alu.hpp",
      "src/backend/mir/aarch64/codegen/calls.cpp",
      "src/backend/mir/aarch64/codegen/calls.hpp",
      "src/backend/mir/aarch64/codegen/comparison.cpp",
      "src/backend/mir/aarch64/codegen/comparison.hpp",
      "src/backend/mir/aarch64/codegen/compatibility_projection.cpp",
      "src/backend/mir/aarch64/codegen/compatibility_projection.hpp",
      "src/backend/mir/aarch64/codegen/dispatch.cpp",
      "src/backend/mir/aarch64/codegen/dispatch.hpp",
      "src/backend/mir/aarch64/codegen/globals.cpp",
      "src/backend/mir/aarch64/codegen/globals.hpp",
      "src/backend/mir/aarch64/codegen/instruction.cpp",
      "src/backend/mir/aarch64/codegen/instruction.hpp",
      "src/backend/mir/aarch64/codegen/machine_printer.cpp",
      "src/backend/mir/aarch64/codegen/machine_printer.hpp",
      "src/backend/mir/aarch64/codegen/module_compile.cpp",
      "src/backend/mir/aarch64/codegen/module_compile.hpp",
      "src/backend/mir/aarch64/codegen/memory.cpp",
      "src/backend/mir/aarch64/codegen/memory.hpp",
      "src/backend/mir/aarch64/codegen/operands.cpp",
      "src/backend/mir/aarch64/codegen/operands.hpp",
      "src/backend/mir/aarch64/codegen/returns.cpp",
      "src/backend/mir/aarch64/codegen/returns.hpp",
      "src/backend/mir/aarch64/codegen/traversal.cpp",
      "src/backend/mir/aarch64/codegen/traversal.hpp",
      "src/backend/mir/aarch64/module/module.cpp",
      "src/backend/mir/aarch64/module/module.hpp",
  };

  std::ostringstream out;
  for (const auto source_path : sources) {
    const std::string source = read_source(source_path);
    if (source.empty()) {
      return {};
    }
    out << "\n// " << source_path << "\n" << source;
  }
  return out.str();
}

std::string read_call_abi_source() {
  return read_source("src/backend/bir/lir_to_bir/call_abi.cpp");
}

std::string read_aarch64_codegen_public_header() {
  return read_source("src/backend/mir/aarch64/codegen/codegen.hpp");
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

int check_aarch64_public_codegen_compiled_module_api() {
  const std::string source = read_aarch64_codegen_public_header();
  if (source.empty()) {
    return fail("could not read aarch64 public codegen header");
  }

  if (!contains(source, "using CompiledModule = module::Module;") ||
      !contains(source, "using CompileResult = module::BuildResult;") ||
      !contains(source, "CompileResult compile_prepared_module(\n"
                        "    const c4c::backend::prepare::PreparedBirModule& prepared)")) {
    return fail("aarch64 public codegen header does not expose the compiled-module API");
  }

  return 0;
}

int check_fast_paths_no_longer_parse_signature_text() {
  const std::string source = read_aarch64_compiled_sources();
  if (source.empty()) {
    return fail("could not read aarch64 compiled sources");
  }

  if (contains(source, "function.signature_text") ||
      contains(source, "parse_function_signature_params") ||
      contains(source, "trim_lir_arg_text(function.signature_text)") ||
      contains(source, "signature_text.find(")) {
    return fail("aarch64 compiled routes still parse rendered signature text");
  }

  if (!contains(source, "PreparedBirModule") ||
      !contains(source, "PreparedCallPlan") ||
      !contains(source, "PreparedMemoryReturnPlan") ||
      !contains(source, "PreparedValueHome") ||
      !contains(source, "build_module(\n"
                        "    const c4c::backend::prepare::PreparedBirModule& prepared)")) {
    return fail("aarch64 compiled routes are not auditing the prepared-record owners");
  }

  return 0;
}

int check_structured_helpers_cover_signature_authorities() {
  const std::string source = read_call_abi_source();
  if (source.empty()) {
    return fail("could not read call_abi.cpp");
  }

  if (!contains(source, "bool enforce_structured_signature_aggregate_layouts(") ||
      !contains(source, "return target_profile.arch == c4c::TargetArch::Aarch64;") ||
      !contains(source, "function_.signature_return_type_ref.has_value() &&\n"
                        "      enforce_structured_signature_aggregate_layouts") ||
      !contains(source, "function.signature_has_void_param_list || function.signature_is_variadic") ||
      !contains(source, "!function.signature_params.empty() || "
                        "!function.signature_param_type_refs.empty()") ||
      !contains(source, "function.signature_params.size() != "
                        "function.signature_param_type_refs.size()") ||
      !contains(source, "? structured_signature_params(function)\n"
                        "                                 : parse_function_signature_params("
                        "function.signature_text)")) {
    return fail("call ABI structured signature helpers do not cover return, params, variadic, and legacy fallback gates");
  }
  return 0;
}

int check_signature_abi_uses_structured_aggregate_refs() {
  const std::string source = read_call_abi_source();
  if (source.empty()) {
    return fail("could not read call_abi.cpp");
  }

  if (!contains(source, "lookup_backend_aggregate_type_ref_layout_result") ||
      !contains(source, "type_ref->has_struct_name_id()") ||
      !contains(source, "TargetArch::Aarch64") ||
      !contains(source, "&*function.signature_return_type_ref") ||
      !contains(source, "function.signature_param_type_refs[index]")) {
    return fail("signature ABI lowering does not require structured aggregate type refs");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = check_aarch64_public_codegen_compiled_module_api(); status != 0) {
    return status;
  }
  if (const int status = check_fast_paths_no_longer_parse_signature_text(); status != 0) {
    return status;
  }
  if (const int status = check_structured_helpers_cover_signature_authorities(); status != 0) {
    return status;
  }
  if (const int status = check_signature_abi_uses_structured_aggregate_refs(); status != 0) {
    return status;
  }
  return 0;
}
