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
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::C,
                     "frontend_lir_label_address_rvalue_probe_test.c");
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

const c4c::codegen::lir::LirStoreOp& require_pointer_initializer_store(
    const c4c::codegen::lir::LirFunction& function) {
  const c4c::codegen::lir::LirStoreOp* result = nullptr;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst);
      if (store != nullptr && store->type_str == "ptr") {
        expect_true(result == nullptr,
                    "fixture should have one immediate pointer initializer store");
        result = store;
      }
    }
  }
  expect_true(result != nullptr,
              "automatic local label address should reach a structured pointer store");
  return *result;
}

}  // namespace

int main() {
  const c4c::hir::Module hir_module = lower_hir_module(R"c(
int automatic_local_label_address(void) {
  void *target = &&dispatch;
  return target != 0;
dispatch:
  return 1;
}
)c");
  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);
  const c4c::codegen::lir::LirStoreOp& store =
      require_pointer_initializer_store(require_function(lir_module,
                                                         "automatic_local_label_address"));

  // This probe fixes the source-form and immediate structured-consumer seam.
  // It deliberately makes no producer-authority assertion: the selected future
  // capability must define typed pointer/current-function authority separately.
  expect_true(store.type_str == "ptr",
              "automatic local label-address initializer should reach a pointer store seam");

  std::cout << "PASS: frontend_lir_label_address_rvalue_probe\n";
  return 0;
}
