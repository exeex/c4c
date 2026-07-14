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
#include <variant>

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

c4c::hir::Module lower_hir_module(std::string_view source) {
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::C));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(
      tokens, arena, &lexer.text_table(), &lexer.file_table(),
      c4c::SourceProfile::C, "frontend_lir_label_address_table_decay_probe_test.c");
  c4c::Node* root = parser.parse();
  auto result =
      c4c::sema::analyze_program(root, c4c::sema_profile_from(c4c::SourceProfile::C));
  expect_true(result.validation.ok, "fixture source should parse and validate");
  expect_true(result.hir_module.has_value(), "fixture source should lower to HIR");
  return *result.hir_module;
}

const c4c::codegen::lir::LirFunction& require_function(
    const c4c::codegen::lir::LirModule& module, std::string_view name) {
  const auto it = std::find_if(module.functions.begin(), module.functions.end(),
                               [&](const c4c::codegen::lir::LirFunction& function) {
                                 return function.name == name;
                               });
  expect_true(it != module.functions.end(), "fixture function should lower into LIR");
  return *it;
}

const c4c::codegen::lir::LirGepOp& require_table_decay_gep(
    const c4c::codegen::lir::LirFunction& function) {
  const c4c::codegen::lir::LirGepOp* result = nullptr;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst);
      if (gep != nullptr && gep->element_type == c4c::codegen::lir::LirTypeRef("[2 x ptr]") &&
          gep->indices.size() == 2) {
        expect_true(result == nullptr,
                    "fixture should have one structured automatic-table decay GEP");
        result = gep;
      }
    }
  }
  expect_true(result != nullptr,
              "automatic local label-address table should reach a structured decay GEP");
  return *result;
}

}  // namespace

int main() {
  const c4c::hir::Module hir_module = lower_hir_module(R"c(
int automatic_local_label_address_table(void) {
  void *table[] = { &&first, &&second };
  void **cursor = table;
  return cursor[0] != 0;
first:
  return 1;
second:
  return 2;
}
)c");
  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);
  const c4c::codegen::lir::LirFunction& function =
      require_function(lir_module, "automatic_local_label_address_table");

  const auto table_slot = std::find_if(
      function.alloca_insts.begin(), function.alloca_insts.end(),
      [](const c4c::codegen::lir::LirInst& inst) {
        const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst);
        return alloca != nullptr &&
               alloca->type_str == c4c::codegen::lir::LirTypeRef("[2 x ptr]");
      });
  expect_true(table_slot != function.alloca_insts.end(),
              "fixture should retain an automatic two-entry label-address table slot");

  const c4c::codegen::lir::LirGepOp& decay = require_table_decay_gep(function);
  expect_true(decay.element_type == c4c::codegen::lir::LirTypeRef("[2 x ptr]") &&
                  decay.indices.size() == 2,
              "table use before indexing should retain the structured two-index decay seam");

  // This probe identifies only the source-form and structured decay seam. It
  // intentionally does not accept the current raw/no-ID producer result; a
  // future capability must provide typed zero indices and current-function
  // pointer authority for the table slot and GEP result.
  std::cout << "PASS: frontend_lir_label_address_table_decay_probe\n";
  return 0;
}
