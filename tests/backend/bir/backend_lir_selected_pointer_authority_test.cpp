#include "src/codegen/lir/ir.hpp"
#include "src/codegen/lir/hir_to_lir.hpp"
#include "src/frontend/hir/hir_ir.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace lir = c4c::codegen::lir;
namespace hir = c4c::hir;

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

c4c::TypeSpec long_long_type() {
  c4c::TypeSpec type{};
  type.base = c4c::TB_LONGLONG;
  type.enum_underlying_base = c4c::TB_VOID;
  type.array_size = -1;
  return type;
}

c4c::TypeSpec large_aggregate_type(c4c::TextId tag_id) {
  c4c::TypeSpec type{};
  type.base = c4c::TB_STRUCT;
  type.enum_underlying_base = c4c::TB_VOID;
  type.tag_text_id = tag_id;
  type.array_size = -1;
  return type;
}

hir::Module selected_byval_materialization_module() {
  hir::Module module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);

  const c4c::TextId aggregate_tag =
      module.link_name_texts->intern("LargeAggregate");
  hir::HirStructDef aggregate;
  aggregate.tag = "LargeAggregate";
  aggregate.tag_text_id = aggregate_tag;
  aggregate.size_bytes = 24;
  aggregate.align_bytes = 8;
  aggregate.fields = {
      hir::HirStructField{
          .name = "a",
          .elem_type = long_long_type(),
          .llvm_idx = 0,
          .offset_bytes = 0,
          .size_bytes = 8,
          .align_bytes = 8,
      },
      hir::HirStructField{
          .name = "b",
          .elem_type = long_long_type(),
          .llvm_idx = 1,
          .offset_bytes = 8,
          .size_bytes = 8,
          .align_bytes = 8,
      },
      hir::HirStructField{
          .name = "c",
          .elem_type = long_long_type(),
          .llvm_idx = 2,
          .offset_bytes = 16,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  };
  module.struct_defs.emplace(aggregate.tag, aggregate);
  module.struct_def_order.push_back(aggregate.tag);
  module.index_struct_def_owner(aggregate, true);

  const c4c::TypeSpec aggregate_type = large_aggregate_type(aggregate_tag);
  c4c::TypeSpec aggregate_pointer_type = aggregate_type;
  aggregate_pointer_type.ptr_level = 1;

  hir::Expr parameter_ref;
  parameter_ref.id = module.alloc_expr_id();
  parameter_ref.type.spec = aggregate_type;
  parameter_ref.type.category = hir::ValueCategory::LValue;
  parameter_ref.payload = hir::DeclRef{
      .name = "p",
      .param_index = 0,
  };

  hir::Expr address_of_parameter;
  address_of_parameter.id = module.alloc_expr_id();
  address_of_parameter.type.spec = aggregate_pointer_type;
  address_of_parameter.payload = hir::UnaryExpr{
      .op = hir::UnaryOp::AddrOf,
      .operand = parameter_ref.id,
  };

  hir::Function function;
  function.id = module.alloc_function_id();
  function.name = "selected_byval_materialization";
  function.link_name_id = module.link_names.intern(function.name);
  function.return_type.spec.base = c4c::TB_VOID;
  function.return_type.spec.enum_underlying_base = c4c::TB_VOID;
  function.return_type.spec.array_size = -1;
  function.params.push_back(hir::Param{
      .name = "p",
      .type = hir::QualType{
          .spec = aggregate_type,
      },
  });
  function.entry = module.alloc_block_id();

  hir::Block entry;
  entry.id = function.entry;
  entry.stmts.push_back(hir::Stmt{
      .payload = hir::ExprStmt{address_of_parameter.id},
  });
  function.blocks.push_back(std::move(entry));

  module.expr_pool.push_back(std::move(parameter_ref));
  module.expr_pool.push_back(std::move(address_of_parameter));
  module.index_function_decl(function);
  module.functions.push_back(std::move(function));
  return module;
}

void test_selected_byval_materialization_populates_authority() {
  lir::LirModule module =
      lir::lower(selected_byval_materialization_module());
  expect(module.functions.size() == 1,
         "selected producer fixture should lower one function");
  const lir::LirFunction& function = module.functions.front();
  expect(function.selected_memcpy_pointer_authority.has_value(),
         "selected byval materialization should publish pointer authority");

  const auto& authority = *function.selected_memcpy_pointer_authority;
  expect(authority.byval_parameter.role ==
             lir::LirSelectedMemcpyPointerRole::ByvalParameter,
         "selected parameter definition should carry byval role");
  expect(authority.destination_alloca.role ==
             lir::LirSelectedMemcpyPointerRole::DestinationAlloca,
         "selected destination definition should carry destination role");
  expect(authority.byval_parameter.object_owner == function.link_name_id &&
             authority.destination_alloca.object_owner == function.link_name_id,
         "selected definitions should be owned by the current function");
  expect(authority.byval_parameter.value.valid() &&
             authority.destination_alloca.value.valid() &&
             authority.byval_parameter.value != authority.destination_alloca.value,
         "selected definitions should use distinct value IDs");
  expect(authority.byval_parameter.object.valid() &&
             authority.destination_alloca.object.valid() &&
             authority.byval_parameter.object != authority.destination_alloca.object,
         "selected definitions should use distinct local objects");
  expect(authority.byval_parameter.pointer_type.kind() ==
             lir::LirTypeKind::Pointer &&
             authority.destination_alloca.pointer_type.kind() ==
                 lir::LirTypeKind::Pointer,
         "selected definitions should carry pointer type authority");
  expect(authority.byval_parameter.live_at_selected_site &&
             authority.destination_alloca.live_at_selected_site,
         "selected definitions should be live at the selected site");

  const lir::LirMemcpyOp* selected_memcpy = nullptr;
  for (const lir::LirBlock& block : function.blocks) {
    for (const lir::LirInst& inst : block.insts) {
      if (const auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        selected_memcpy = memcpy;
        break;
      }
    }
    if (selected_memcpy) break;
  }
  expect(selected_memcpy != nullptr,
         "selected producer fixture should emit its fixed-aggregate memcpy");
  expect(selected_memcpy->selected_authority.has_value(),
         "selected memcpy must publish typed authority rather than use display operands");
  const auto& memcpy_authority = *selected_memcpy->selected_authority;
  expect(memcpy_authority.destination == authority.destination_alloca.value &&
             memcpy_authority.source == authority.byval_parameter.value,
         "selected memcpy pointer identities must come from current-function authority");
  expect(memcpy_authority.destination_object == authority.destination_alloca.object &&
             memcpy_authority.source_object == authority.byval_parameter.object &&
             memcpy_authority.destination_object_owner == function.link_name_id &&
             memcpy_authority.source_object_owner == function.link_name_id,
         "selected memcpy objects must retain current-function ownership");
  expect(memcpy_authority.destination_live_at_site &&
             memcpy_authority.source_live_at_site &&
             memcpy_authority.size.value == 24,
         "selected memcpy must publish live pointer authority and typed i64 size");

  lir::verify_module(module);
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
  test_selected_byval_materialization_populates_authority();
  return 0;
}
