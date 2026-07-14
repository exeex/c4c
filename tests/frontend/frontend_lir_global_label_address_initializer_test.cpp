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
                     "frontend_lir_global_label_address_initializer_test.c");
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

c4c::codegen::lir::LirGlobal& require_label_address_global(
    c4c::codegen::lir::LirModule& module) {
  const auto it = std::find_if(module.globals.begin(), module.globals.end(),
                               [](const c4c::codegen::lir::LirGlobal& global) {
                                 return !global.initializer_elements.empty();
                               });
  expect_true(it != module.globals.end(),
              "static label-address initializer should lower into an LIR global element");
  return *it;
}

}  // namespace

int main() {
  const c4c::hir::Module hir_module = lower_hir_module(R"c(
int static_label_address_table(void) {
  static void *table[] = { &&first, &&second };
  return table[0] != 0;
first:
  return 1;
second:
  return 2;
}
)c");
  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);
  const c4c::codegen::lir::LirFunction& owner =
      require_function(lir_module, "static_label_address_table");
  c4c::codegen::lir::LirGlobal& global = require_label_address_global(lir_module);

  expect_true(global.initializer_elements.size() == 2,
              "each static table label address should publish one structured element");
  for (const auto& element : global.initializer_elements) {
    const auto* label_address =
        std::get_if<c4c::codegen::lir::LirGlobalInitializerLabelAddress>(&element);
    expect_true(label_address != nullptr,
                "static table initializer element should be a structured label address");
    expect_true(label_address->enclosing_function == owner.link_name_id,
                "structured label address should retain its enclosing function LinkNameId");
    expect_true(label_address->target.valid(),
                "structured label address should retain a valid target LirBlockId");
    const auto block = std::find_if(owner.blocks.begin(), owner.blocks.end(),
                                    [&](const c4c::codegen::lir::LirBlock& candidate) {
                                      return candidate.id == label_address->target;
                                    });
    expect_true(block != owner.blocks.end(),
                "structured label address target should belong to its enclosing function");
  }
  c4c::codegen::lir::verify_module(lir_module);

  c4c::codegen::lir::LirModule wrong_owner = lir_module;
  auto& malformed_owner = require_label_address_global(wrong_owner);
  auto& malformed_address = std::get<c4c::codegen::lir::LirGlobalInitializerLabelAddress>(
      malformed_owner.initializer_elements.front());
  malformed_address.enclosing_function = c4c::kInvalidLinkName;
  try {
    c4c::codegen::lir::verify_module(wrong_owner);
    fail("verifier should reject a label address without an owning function identity");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule wrong_target = lir_module;
  auto& malformed_target_owner = require_label_address_global(wrong_target);
  auto& malformed_target = std::get<c4c::codegen::lir::LirGlobalInitializerLabelAddress>(
      malformed_target_owner.initializer_elements.front());
  malformed_target.target = c4c::codegen::lir::LirBlockId::invalid();
  try {
    c4c::codegen::lir::verify_module(wrong_target);
    fail("verifier should reject a label address without a valid target block identity");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  std::cout << "PASS: frontend_lir_global_label_address_initializer\n";
  return 0;
}
