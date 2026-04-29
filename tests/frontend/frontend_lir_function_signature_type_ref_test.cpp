#include "arena.hpp"
#include "hir_to_lir.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"
#include "target_profile.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

c4c::hir::Module lower_hir_module(std::string_view source) {
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::C));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::C,
                     "frontend_lir_function_signature_type_ref_test.c");
  c4c::Node* root = parser.parse();
  auto result =
      c4c::sema::analyze_program(root, c4c::sema_profile_from(c4c::SourceProfile::C));

  expect_true(result.validation.ok,
              "fixture source should parse and validate successfully");
  expect_true(result.hir_module.has_value(),
              "fixture source should lower to HIR");
  return *result.hir_module;
}

const c4c::codegen::lir::LirFunction& require_function(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name,
    bool is_declaration) {
  const auto it = std::find_if(module.functions.begin(), module.functions.end(),
                               [&](const c4c::codegen::lir::LirFunction& fn) {
                                 return fn.name == name &&
                                        fn.is_declaration == is_declaration;
                               });
  expect_true(it != module.functions.end(),
              "fixture function should lower into LIR: " + std::string(name));
  return *it;
}

c4c::codegen::lir::LirFunction& require_mutable_function(
    c4c::codegen::lir::LirModule& module,
    std::string_view name,
    bool is_declaration) {
  auto it = std::find_if(module.functions.begin(), module.functions.end(),
                         [&](const c4c::codegen::lir::LirFunction& fn) {
                           return fn.name == name &&
                                  fn.is_declaration == is_declaration;
                         });
  expect_true(it != module.functions.end(),
              "fixture function should lower into LIR: " + std::string(name));
  return *it;
}

void expect_verify_rejects(const c4c::codegen::lir::LirModule& module,
                           const std::string& msg) {
  try {
    c4c::codegen::lir::verify_module(module);
    fail(msg);
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }
}

void expect_struct_type_ref(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirTypeRef& type_ref,
    std::string_view expected_text,
    const std::string& msg) {
  expect_eq(type_ref.str(), expected_text, msg + " text should match");
  expect_true(type_ref.has_struct_name_id(), msg + " should carry a StructNameId");
  expect_eq(module.struct_names.spelling(type_ref.struct_name_id()), expected_text,
            msg + " StructNameId should resolve to the mirrored signature text");
}

void expect_struct_signature_refs(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirFunction& fn) {
  expect_true(fn.signature_return_type_ref.has_value(),
              "function should carry a return type mirror");
  expect_struct_type_ref(module, *fn.signature_return_type_ref, "%struct.Pair",
                         "return type mirror");

  expect_eq(std::to_string(fn.signature_param_type_refs.size()), "1",
            "function should carry one emitted parameter mirror");
  expect_struct_type_ref(module, fn.signature_param_type_refs[0], "%struct.Pair",
                         "parameter type mirror");
}

void expect_byval_signature_refs(
    const c4c::codegen::lir::LirFunction& fn,
    std::string_view expected_param_text) {
  expect_true(fn.signature_return_type_ref.has_value(),
              "byval fixture should carry a return type mirror");
  expect_eq(fn.signature_return_type_ref->str(), "i32",
            "byval fixture return mirror should keep the emitted return type");
  expect_true(!fn.signature_return_type_ref->has_struct_name_id(),
              "non-aggregate return mirror should not carry a StructNameId");

  expect_eq(std::to_string(fn.signature_param_type_refs.size()), "1",
            "byval fixture should carry one emitted parameter mirror");
  const auto& param_ref = fn.signature_param_type_refs[0];
  expect_eq(param_ref.str(), expected_param_text,
            "byval parameter mirror should keep the emitted parameter type fragment");
  expect_true(!param_ref.has_struct_name_id(),
              "byval parameter mirror should stay raw when text is not the aggregate name");
}

void expect_single_signature_param(const c4c::codegen::lir::LirFunction& fn,
                                   std::string_view expected_name,
                                   c4c::TypeBase expected_base,
                                   const std::string& msg) {
  expect_eq(std::to_string(fn.signature_params.size()), "1",
            msg + " should carry one structured signature parameter");
  expect_eq(fn.signature_params[0].name, expected_name,
            msg + " should carry the lowered parameter name");
  expect_true(fn.signature_params[0].type.base == expected_base,
              msg + " should carry the HIR parameter type");
}

}  // namespace

int main() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct Pair {
  int left;
  int right;
};

struct Big {
  long long a;
  long long b;
  long long c;
};

struct Pair declared_pair(struct Pair input);
int declared_big(struct Big input);

struct Pair defined_pair(struct Pair input) {
  return input;
}

int defined_big(struct Big input) {
  return (int)input.a;
}

int declared_variadic(int fixed, ...);
int defined_variadic(int fixed, ...) {
  return fixed;
}

int declared_void_params(void);
int defined_void_params(void) {
  return 1;
}
)c");
  hir_module.target_profile =
      c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);

  const auto& declared_pair = require_function(lir_module, "declared_pair", true);
  expect_struct_signature_refs(lir_module, declared_pair);
  expect_single_signature_param(declared_pair, "%p.input", c4c::TB_STRUCT,
                                "declared aggregate signature metadata");
  expect_true(!declared_pair.signature_is_variadic,
              "non-variadic declaration should carry a structured variadic=false flag");
  expect_true(!declared_pair.signature_has_void_param_list,
              "aggregate declaration should not carry a void-param-list flag");

  const auto& defined_pair = require_function(lir_module, "defined_pair", false);
  expect_struct_signature_refs(lir_module, defined_pair);
  expect_single_signature_param(defined_pair, "%p.input", c4c::TB_STRUCT,
                                "defined aggregate signature metadata");
  expect_true(!defined_pair.signature_is_variadic,
              "non-variadic definition should carry a structured variadic=false flag");
  expect_true(!defined_pair.signature_has_void_param_list,
              "aggregate definition should not carry a void-param-list flag");

  const std::string byval_param_text = "ptr byval(%struct.Big) align 8";
  const auto& declared_big = require_function(lir_module, "declared_big", true);
  expect_byval_signature_refs(declared_big, byval_param_text);
  expect_single_signature_param(declared_big, "%p.input", c4c::TB_STRUCT,
                                "declared byval signature metadata");

  const auto& defined_big = require_function(lir_module, "defined_big", false);
  expect_byval_signature_refs(defined_big, byval_param_text);
  expect_single_signature_param(defined_big, "%p.input", c4c::TB_STRUCT,
                                "defined byval signature metadata");

  const auto& declared_variadic =
      require_function(lir_module, "declared_variadic", true);
  expect_true(declared_variadic.signature_is_variadic,
              "variadic declaration should carry a structured variadic flag");
  expect_true(!declared_variadic.signature_has_void_param_list,
              "variadic declaration should not carry a void-param-list flag");
  expect_single_signature_param(declared_variadic, "%p.fixed", c4c::TB_INT,
                                "declared variadic signature metadata");
  expect_eq(std::to_string(declared_variadic.signature_param_type_refs.size()), "1",
            "variadic declaration should mirror only fixed parameters");

  const auto& defined_variadic =
      require_function(lir_module, "defined_variadic", false);
  expect_true(defined_variadic.signature_is_variadic,
              "variadic definition should carry a structured variadic flag");
  expect_true(!defined_variadic.signature_has_void_param_list,
              "variadic definition should not carry a void-param-list flag");
  expect_single_signature_param(defined_variadic, "%p.fixed", c4c::TB_INT,
                                "defined variadic signature metadata");
  expect_eq(std::to_string(defined_variadic.signature_param_type_refs.size()), "1",
            "variadic definition should mirror only fixed parameters");

  const auto& declared_void_params =
      require_function(lir_module, "declared_void_params", true);
  expect_true(declared_void_params.signature_has_void_param_list,
              "void-parameter declaration should carry a structured void-param-list flag");
  expect_true(!declared_void_params.signature_is_variadic,
              "void-parameter declaration should not be variadic");
  expect_eq(std::to_string(declared_void_params.signature_params.size()), "0",
            "void-parameter declaration should not expose a fixed signature parameter");
  expect_eq(std::to_string(declared_void_params.signature_param_type_refs.size()), "0",
            "void-parameter declaration should not expose a fixed parameter mirror");

  const auto& defined_void_params =
      require_function(lir_module, "defined_void_params", false);
  expect_true(defined_void_params.signature_has_void_param_list,
              "void-parameter definition should carry a structured void-param-list flag");
  expect_true(!defined_void_params.signature_is_variadic,
              "void-parameter definition should not be variadic");
  expect_eq(std::to_string(defined_void_params.signature_params.size()), "0",
            "void-parameter definition should not expose a fixed signature parameter");
  expect_eq(std::to_string(defined_void_params.signature_param_type_refs.size()), "0",
            "void-parameter definition should not expose a fixed parameter mirror");

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("declare %struct.Pair @declared_pair(%struct.Pair)") !=
                  std::string::npos,
              "printer should keep using declaration signature_text");
  expect_true(llvm_ir.find("define %struct.Pair @defined_pair(%struct.Pair %p.input)") !=
                  std::string::npos,
              "printer should keep using definition signature_text");
  expect_true(llvm_ir.find("declare i32 @declared_big(ptr byval(%struct.Big) align 8)") !=
                  std::string::npos,
              "printer should keep using byval declaration signature_text");
  expect_true(llvm_ir.find("define i32 @defined_big(ptr byval(%struct.Big) align 8 %p.input)") !=
                  std::string::npos,
              "printer should keep using byval definition signature_text");

  const c4c::StructNameId pair_id = lir_module.struct_names.find("%struct.Pair");
  const c4c::StructNameId big_id = lir_module.struct_names.find("%struct.Big");
  expect_true(pair_id != c4c::kInvalidStructName,
              "fixture should declare Pair for structured verifier checks");
  expect_true(big_id != c4c::kInvalidStructName,
              "fixture should declare Big for mismatch verifier checks");

  c4c::codegen::lir::LirModule stale_return_text = lir_module;
  require_mutable_function(stale_return_text, "declared_pair", true)
      .signature_return_type_ref->str() = "%struct.StaleMirrorText";
  c4c::codegen::lir::verify_module(stale_return_text);

  c4c::codegen::lir::LirModule stale_param_text = lir_module;
  require_mutable_function(stale_param_text, "defined_pair", false)
      .signature_param_type_refs[0]
      .str() = "%struct.StaleMirrorText";
  c4c::codegen::lir::verify_module(stale_param_text);

  c4c::codegen::lir::LirModule missing_return_name = lir_module;
  require_mutable_function(missing_return_name, "declared_pair", true)
      .signature_return_type_ref = c4c::codegen::lir::LirTypeRef("%struct.Pair");
  expect_verify_rejects(
      missing_return_name,
      "verifier should reject a known struct signature return without StructNameId");

  c4c::codegen::lir::LirModule missing_param_name = lir_module;
  require_mutable_function(missing_param_name, "defined_pair", false)
      .signature_param_type_refs[0] =
      c4c::codegen::lir::LirTypeRef("%struct.Pair");
  expect_verify_rejects(
      missing_param_name,
      "verifier should reject a known struct signature parameter without StructNameId");

  c4c::codegen::lir::LirModule mismatched_return_name = lir_module;
  auto& mismatched_return_fn =
      require_mutable_function(mismatched_return_name, "declared_pair", true);
  mismatched_return_fn.signature_return_type_ref =
      mismatched_return_fn.signature_return_type_ref->with_struct_name_id(big_id);
  expect_verify_rejects(
      mismatched_return_name,
      "verifier should reject a signature return with mismatched StructNameId");

  c4c::codegen::lir::LirModule mismatched_param_name = lir_module;
  auto& mismatched_param_fn =
      require_mutable_function(mismatched_param_name, "defined_pair", false);
  mismatched_param_fn.signature_param_type_refs[0] =
      mismatched_param_fn.signature_param_type_refs[0].with_struct_name_id(big_id);
  expect_verify_rejects(
      mismatched_param_name,
      "verifier should reject a signature parameter with mismatched StructNameId");

  c4c::codegen::lir::LirModule param_text_fallback = lir_module;
  auto& fallback_param_fn =
      require_mutable_function(param_text_fallback, "defined_pair", false);
  fallback_param_fn.signature_text =
      "define %struct.Pair @defined_pair(%struct.NotDeclared %p.input) {";
  fallback_param_fn.signature_param_type_refs[0] =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.StaleMirrorText",
                                                 pair_id);
  expect_verify_rejects(
      param_text_fallback,
      "verifier should reject signature parameter mirror text mismatch "
      "without declared struct boundary");

  c4c::codegen::lir::LirModule byval_text_fallback = lir_module;
  require_mutable_function(byval_text_fallback, "declared_big", true)
      .signature_param_type_refs[0] = c4c::codegen::lir::LirTypeRef("i8");
  expect_verify_rejects(
      byval_text_fallback,
      "verifier should reject a byval signature parameter mirror text mismatch");

  c4c::codegen::lir::LirModule aggregate_param_module;
  c4c::codegen::lir::LirFunction aggregate_param_decl;
  aggregate_param_decl.name = "takes_complex";
  aggregate_param_decl.is_declaration = true;
  aggregate_param_decl.signature_text =
      "declare void @takes_complex({ double, double }, ptr byval(%struct.Big) align 8)";
  aggregate_param_decl.signature_param_type_refs.push_back(
      c4c::codegen::lir::LirTypeRef("{ double, double }"));
  aggregate_param_decl.signature_param_type_refs.push_back(
      c4c::codegen::lir::LirTypeRef("ptr"));
  aggregate_param_module.functions.push_back(std::move(aggregate_param_decl));
  c4c::codegen::lir::verify_module(aggregate_param_module);

  std::cout << "PASS: frontend_lir_function_signature_type_ref\n";
  return 0;
}
