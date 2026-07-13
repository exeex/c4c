#include "src/backend/bir/bir.hpp"
#include "src/codegen/lir/ir.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expect(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

lir::LirFunction void_declaration(std::string name) {
  lir::LirFunction function;
  function.name = std::move(name);
  function.is_declaration = true;
  function.signature_return_type_ref = lir::LirTypeRef("void");
  return function;
}

lir::LirFunction void_definition(std::string name,
                                 std::vector<lir::LirBlock> blocks) {
  lir::LirFunction function;
  function.name = std::move(name);
  function.signature_return_type_ref = lir::LirTypeRef("void");
  function.blocks = std::move(blocks);
  function.entry = function.blocks.front().id;
  return function;
}

lir::LirBlock return_block(std::uint32_t id, std::string label) {
  lir::LirBlock block;
  block.id = lir::LirBlockId{id};
  block.label = std::move(label);
  block.terminator = lir::LirRet{std::nullopt, "void"};
  return block;
}

void test_supported_import_and_views() {
  lir::LirModule module;
  module.functions.push_back(void_declaration("decl"));
  module.functions.push_back(
      void_definition("returns", {return_block(0, "entry")}));

  lir::LirBlock branch_entry;
  branch_entry.id = lir::LirBlockId{0};
  branch_entry.label = "entry";
  branch_entry.terminator = lir::LirBr{"exit"};
  module.functions.push_back(void_definition(
      "branches", {std::move(branch_entry), return_block(1, "exit")}));

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(imported.has_value(), "supported LIR subset should publish RawBir");

  const bir::ModuleView module_view = imported.value().view();
  const auto functions = module_view.functions();
  expect(functions.size() == 3, "module view should preserve all functions");

  auto declaration = module_view.function(functions[0]);
  expect(declaration.has_value() && declaration.value().is_declaration(),
         "declaration should remain a declaration");
  expect(declaration.value().link_name() == "decl",
         "declaration should preserve its link name");
  expect(declaration.value().blocks().empty(),
         "declaration should not acquire a body");

  auto returns = module_view.function(functions[1]);
  expect(returns.has_value(), "returning definition should resolve");
  const auto return_blocks = returns.value().blocks();
  expect(return_blocks.size() == 1, "returning definition should have one block");
  auto return_term = returns.value().terminator(return_blocks[0]);
  expect(return_term.has_value() &&
             std::holds_alternative<bir::ReturnTerm>(return_term.value()),
         "void LIR return should become a BIR ReturnTerm");

  auto branches = module_view.function(functions[2]);
  expect(branches.has_value(), "branching definition should resolve");
  const auto branch_blocks = branches.value().blocks();
  expect(branch_blocks.size() == 2, "branching definition should preserve block order");
  auto successors = branches.value().successors(branch_blocks[0]);
  expect(successors.has_value() && successors.value().size() == 1 &&
             successors.value()[0] == branch_blocks[1],
         "CFG successor view should resolve the imported branch target");
}

void test_structured_rejection() {
  lir::LirModule module;
  module.globals.push_back(lir::LirGlobal{});

  auto imported = bir::lower_lir_to_raw_bir(module);
  expect(!imported.has_value(), "unsupported globals must not produce RawBir");
  expect(imported.error().code == bir::ImportErrorCode::UnsupportedGlobals,
         "unsupported globals should return their structured import error code");
  expect(!imported.error().detail.empty(),
         "structured import rejection should include diagnostic detail");
}

}  // namespace

int main() {
  test_supported_import_and_views();
  test_structured_rejection();
  return 0;
}
