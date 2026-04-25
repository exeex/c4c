#include "arena.hpp"
#include "call_args_ops.hpp"
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
                     "frontend_lir_call_type_ref_test.c");
  c4c::Node* root = parser.parse();
  auto result =
      c4c::sema::analyze_program(root, c4c::sema_profile_from(c4c::SourceProfile::C));

  expect_true(result.validation.ok,
              "fixture source should parse and validate successfully");
  expect_true(result.hir_module.has_value(),
              "fixture source should lower to HIR");
  return *result.hir_module;
}

c4c::codegen::lir::LirFunction& require_function(
    c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  const auto it = std::find_if(module.functions.begin(), module.functions.end(),
                               [&](const c4c::codegen::lir::LirFunction& fn) {
                                 return fn.name == name && !fn.is_declaration;
                               });
  expect_true(it != module.functions.end(),
              "fixture function should lower into LIR: " + std::string(name));
  return *it;
}

c4c::codegen::lir::LirCallOp& require_call_to(
    c4c::codegen::lir::LirFunction& fn,
    std::string_view callee) {
  for (auto& block : fn.blocks) {
    for (auto& inst : block.insts) {
      auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (call && call->callee == callee) return *call;
    }
  }
  fail("fixture function should contain call to " + std::string(callee));
}

void expect_struct_type_ref(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirTypeRef& type_ref,
    std::string_view expected_text,
    const std::string& msg) {
  expect_eq(type_ref.str(), expected_text, msg + " text should match");
  expect_true(type_ref.has_struct_name_id(), msg + " should carry a StructNameId");
  expect_eq(module.struct_names.spelling(type_ref.struct_name_id()), expected_text,
            msg + " StructNameId should resolve to mirrored text");
}

}  // namespace

int main() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct Pair {
  int left;
  int right;
};

struct Pair make_pair(struct Pair input) {
  return input;
}

struct Pair call_pair(struct Pair value) {
  return make_pair(value);
}

int sink(int seed, ...);

int call_variadic(struct Pair tail) {
  return sink(1, tail);
}
)c");
  hir_module.target_profile =
      c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);
  c4c::codegen::lir::LirFunction& call_pair = require_function(lir_module, "call_pair");
  c4c::codegen::lir::LirCallOp& direct_call = require_call_to(call_pair, "@make_pair");

  expect_struct_type_ref(lir_module, direct_call.return_type, "%struct.Pair",
                         "call return mirror");
  expect_eq(std::to_string(direct_call.arg_type_refs.size()), "1",
            "direct call should carry one argument mirror");
  expect_struct_type_ref(lir_module, direct_call.arg_type_refs[0], "%struct.Pair",
                         "call argument mirror");

  const std::string formatted =
      c4c::codegen::lir::format_lir_call_site(direct_call);
  expect_true(formatted.find("@make_pair(%struct.Pair ") != std::string::npos,
              "format_lir_call_site should keep the existing call text shape");

  c4c::codegen::lir::verify_module(lir_module);
  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("call %struct.Pair (%struct.Pair) @make_pair(%struct.Pair ") !=
                  std::string::npos,
              "printer should keep using formatted call-site text");

  c4c::codegen::lir::LirFunction& call_variadic =
      require_function(lir_module, "call_variadic");
  c4c::codegen::lir::LirCallOp& variadic_call =
      require_call_to(call_variadic, "@sink");
  expect_eq(std::to_string(variadic_call.arg_type_refs.size()), "0",
            "variadic aggregate call should not carry argument mirrors when "
            "the call signature cannot parse against emitted ABI arguments");

  c4c::codegen::lir::LirModule missing_return_name = lir_module;
  c4c::codegen::lir::LirCallOp& missing_return_call =
      require_call_to(require_function(missing_return_name, "call_pair"), "@make_pair");
  missing_return_call.return_type = c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(missing_return_name);
    fail("verifier should reject a known struct call return without StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule missing_arg_name = lir_module;
  c4c::codegen::lir::LirCallOp& missing_arg_call =
      require_call_to(require_function(missing_arg_name, "call_pair"), "@make_pair");
  missing_arg_call.arg_type_refs[0] = c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(missing_arg_name);
    fail("verifier should reject a known struct call argument without StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  std::cout << "PASS: frontend_lir_call_type_ref\n";
  return 0;
}
