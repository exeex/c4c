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
)c");
  hir_module.target_profile =
      c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);

  const auto& declared_pair = require_function(lir_module, "declared_pair", true);
  expect_struct_signature_refs(lir_module, declared_pair);

  const auto& defined_pair = require_function(lir_module, "defined_pair", false);
  expect_struct_signature_refs(lir_module, defined_pair);

  const std::string byval_param_text = "ptr byval(%struct.Big) align 8";
  const auto& declared_big = require_function(lir_module, "declared_big", true);
  expect_byval_signature_refs(declared_big, byval_param_text);

  const auto& defined_big = require_function(lir_module, "defined_big", false);
  expect_byval_signature_refs(defined_big, byval_param_text);

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

  std::cout << "PASS: frontend_lir_function_signature_type_ref\n";
  return 0;
}
