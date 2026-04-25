#include "arena.hpp"
#include "hir_to_lir.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"

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
                     "frontend_lir_global_type_ref_test.c");
  c4c::Node* root = parser.parse();
  auto result =
      c4c::sema::analyze_program(root, c4c::sema_profile_from(c4c::SourceProfile::C));

  expect_true(result.validation.ok,
              "fixture source should parse and validate successfully");
  expect_true(result.hir_module.has_value(),
              "fixture source should lower to HIR");
  return *result.hir_module;
}

const c4c::codegen::lir::LirGlobal& require_global(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  const auto it = std::find_if(module.globals.begin(), module.globals.end(),
                               [&](const c4c::codegen::lir::LirGlobal& global) {
                                 return global.name == name;
                               });
  expect_true(it != module.globals.end(),
              "fixture global should lower into LIR: " + std::string(name));
  return *it;
}

c4c::codegen::lir::LirGlobal& require_global(
    c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  const auto it = std::find_if(module.globals.begin(), module.globals.end(),
                               [&](const c4c::codegen::lir::LirGlobal& global) {
                                 return global.name == name;
                               });
  expect_true(it != module.globals.end(),
              "fixture global should lower into LIR: " + std::string(name));
  return *it;
}

void expect_global_type_ref(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirGlobal& global,
    std::string_view expected_type) {
  expect_eq(global.llvm_type, expected_type,
            "global should keep the legacy LLVM type text");
  expect_true(global.llvm_type_ref.has_value(),
              "top-level struct/union globals should carry a structured LIR type mirror");
  expect_eq(global.llvm_type_ref->str(), global.llvm_type,
            "global type mirror text should shadow llvm_type exactly");
  expect_true(global.llvm_type_ref->has_struct_name_id(),
              "global type mirror should carry a StructNameId");
  expect_eq(module.struct_names.spelling(global.llvm_type_ref->struct_name_id()),
            expected_type,
            "global StructNameId should resolve to the rendered aggregate type");
}

}  // namespace

int main() {
  const c4c::hir::Module hir_module = lower_hir_module(R"c(
struct Pair {
  int left;
  int right;
};

union Slot {
  int int_value;
  float float_value;
};

struct Pair pair_global = {1, 2};
union Slot slot_global = {.int_value = 3};
)c");
  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);

  const auto& pair_global = require_global(lir_module, "pair_global");
  expect_global_type_ref(lir_module, pair_global, "%struct.Pair");

  const auto& slot_global = require_global(lir_module, "slot_global");
  expect_global_type_ref(lir_module, slot_global, "%struct.Slot");

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("@pair_global = global %struct.Pair ") != std::string::npos,
              "printer should keep using legacy llvm_type text for struct globals");
  expect_true(llvm_ir.find("@slot_global = global %struct.Slot ") != std::string::npos,
              "printer should keep using legacy llvm_type text for union globals");

  c4c::codegen::lir::LirModule structured_identity = lir_module;
  c4c::codegen::lir::LirGlobal& identity_pair =
      require_global(structured_identity, "pair_global");
  identity_pair.llvm_type_ref->str() = "%struct.StaleMirrorText";
  c4c::codegen::lir::verify_module(structured_identity);

  c4c::codegen::lir::LirModule text_fallback = lir_module;
  c4c::codegen::lir::LirGlobal& fallback_slot =
      require_global(text_fallback, "slot_global");
  fallback_slot.llvm_type_ref =
      c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(text_fallback);
    fail("verifier should reject a global mirror text mismatch without StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  std::cout << "PASS: frontend_lir_global_type_ref\n";
  return 0;
}
