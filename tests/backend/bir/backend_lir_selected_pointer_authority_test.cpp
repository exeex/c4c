#include "src/codegen/lir/ir.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace lir = c4c::codegen::lir;

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expect(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

lir::LirModule selected_authority_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  const c4c::LinkNameId owner = module.link_names.intern("selected_owner");

  lir::LirFunction function;
  function.name = "presentation_is_not_authority";
  function.link_name_id = owner;
  function.signature_text = "declare void @presentation_is_not_authority()";
  function.selected_memcpy_pointer_authority =
      lir::LirSelectedMemcpyPointerAuthority{
          .byval_parameter = lir::LirCurrentFunctionPointerDefinition{
              .value = lir::LirValueId{41},
              .pointer_type = lir::LirTypeRef("ptr"),
              .object = lir::LirObjectId{7},
              .object_owner = owner,
              .role = lir::LirSelectedMemcpyPointerRole::ByvalParameter,
              .live_at_selected_site = true,
          },
          .destination_alloca = lir::LirCurrentFunctionPointerDefinition{
              .value = lir::LirValueId{42},
              .pointer_type = lir::LirTypeRef("ptr"),
              .object = lir::LirObjectId{8},
              .object_owner = owner,
              .role = lir::LirSelectedMemcpyPointerRole::DestinationAlloca,
              .live_at_selected_site = true,
          },
      };
  module.functions.push_back(std::move(function));
  return module;
}

void expect_rejected(lir::LirModule module, const std::string& message) {
  try {
    lir::verify_module(module);
    fail(message);
  } catch (const lir::LirVerifyError&) {
  }
}

void test_selected_current_function_pointer_authority() {
  auto valid = selected_authority_module();
  lir::verify_module(valid);

  auto invalid_value = selected_authority_module();
  invalid_value.functions[0].selected_memcpy_pointer_authority
      ->byval_parameter.value = lir::LirValueId::invalid();
  expect_rejected(std::move(invalid_value),
                  "invalid selected parameter definition must reject");

  auto invalid_object = selected_authority_module();
  invalid_object.functions[0].selected_memcpy_pointer_authority
      ->destination_alloca.object = lir::LirObjectId::invalid();
  expect_rejected(std::move(invalid_object),
                  "invalid selected destination object must reject");

  auto foreign_owner = selected_authority_module();
  const c4c::LinkNameId foreign = foreign_owner.link_names.intern("foreign_owner");
  foreign_owner.functions[0].selected_memcpy_pointer_authority
      ->destination_alloca.object_owner = foreign;
  expect_rejected(std::move(foreign_owner),
                  "foreign selected destination owner must reject");

  auto mismatched_object = selected_authority_module();
  mismatched_object.functions[0].selected_memcpy_pointer_authority
      ->destination_alloca.object = lir::LirObjectId{7};
  expect_rejected(std::move(mismatched_object),
                  "selected pointer definitions must not share a local object");

  auto mismatched_type = selected_authority_module();
  mismatched_type.functions[0].selected_memcpy_pointer_authority
      ->byval_parameter.pointer_type = lir::LirTypeRef::integer(32);
  expect_rejected(std::move(mismatched_type),
                  "non-pointer selected definition type must reject");

  auto dead_definition = selected_authority_module();
  dead_definition.functions[0].selected_memcpy_pointer_authority
      ->byval_parameter.live_at_selected_site = false;
  expect_rejected(std::move(dead_definition),
                  "non-live selected parameter definition must reject");
}

}  // namespace

int main() {
  test_selected_current_function_pointer_authority();
  return 0;
}
