#include "src/codegen/lir/ir.hpp"
#include "src/codegen/lir/hir_to_lir.hpp"
#include "src/frontend/hir/hir_ir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace lir = c4c::codegen::lir;
namespace hir = c4c::hir;
namespace bir = c4c::backend::bir;

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
  lir::LirBlock block;
  block.id = lir::LirBlockId{0};
  block.label = "entry";
  block.insts.push_back(lir::LirMemcpyOp{
      .dst = lir::LirOperand("%selected.dst"),
      .src = lir::LirOperand("%selected.src"),
      .size = lir::LirOperand("24"),
      .selected_authority = lir::LirSelectedMemcpyAuthority{
          .destination = lir::LirValueId{42},
          .source = lir::LirValueId{41},
          .size_type = lir::LirTypeRef::integer(64),
          .size = lir::LirIntegerImmediate{24},
          .destination_object = lir::LirObjectId{8},
          .source_object = lir::LirObjectId{7},
          .destination_object_owner = owner,
          .source_object_owner = owner,
          .destination_live_at_site = true,
          .source_live_at_site = true,
      },
  });
  block.terminator = lir::LirRet{std::nullopt, lir::LirTypeRef("void")};
  function.blocks.push_back(std::move(block));
  function.entry = lir::LirBlockId{0};
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
             memcpy_authority.size_type.kind() == lir::LirTypeKind::Integer &&
             memcpy_authority.size_type.integer_bit_width() == 64 &&
             memcpy_authority.size.value == 24,
         "selected memcpy must publish live pointer authority and typed i64 size");

  lir::verify_module(module);
}

void test_selected_memcpy_authority_verifier_boundary() {
  auto valid = lir::lower(selected_byval_materialization_module());
  lir::verify_module(valid);

  auto missing = valid;
  for (lir::LirBlock& block : missing.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        memcpy->selected_authority.reset();
      }
    }
  }
  expect_rejected(std::move(missing),
                  "selected producer must reject absent selected memcpy authority");

  auto wrong_value = valid;
  for (lir::LirBlock& block : wrong_value.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        memcpy->selected_authority->source = lir::LirValueId{999};
      }
    }
  }
  expect_rejected(std::move(wrong_value),
                  "selected memcpy must reject pointer identity disagreement");

  auto wrong_owner = valid;
  const c4c::LinkNameId foreign = wrong_owner.link_names.intern("foreign_memcpy_owner");
  for (lir::LirBlock& block : wrong_owner.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        memcpy->selected_authority->destination_object_owner = foreign;
      }
    }
  }
  expect_rejected(std::move(wrong_owner),
                  "selected memcpy must reject foreign object ownership");

  auto wrong_object = valid;
  for (lir::LirBlock& block : wrong_object.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        memcpy->selected_authority->source_object = lir::LirObjectId{999};
      }
    }
  }
  expect_rejected(std::move(wrong_object),
                  "selected memcpy must reject object-link disagreement");

  auto wrong_size = valid;
  for (lir::LirBlock& block : wrong_size.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        memcpy->selected_authority->size_type = lir::LirTypeRef::integer(32);
      }
    }
  }
  expect_rejected(std::move(wrong_size),
                  "selected memcpy must reject non-i64 size authority");

  auto nonpositive_size = valid;
  for (lir::LirBlock& block : nonpositive_size.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        memcpy->selected_authority->size = lir::LirIntegerImmediate{0};
      }
    }
  }
  expect_rejected(std::move(nonpositive_size),
                  "selected memcpy must reject nonpositive size authority");

  auto dead = valid;
  for (lir::LirBlock& block : dead.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        memcpy->selected_authority->source_live_at_site = false;
      }
    }
  }
  expect_rejected(std::move(dead),
                  "selected memcpy must reject non-live pointer authority");

  auto duplicate = valid;
  for (lir::LirBlock& block : duplicate.functions[0].blocks) {
    for (const lir::LirInst& inst : block.insts) {
      if (const auto* memcpy = std::get_if<lir::LirMemcpyOp>(&inst)) {
        block.insts.push_back(*memcpy);
        break;
      }
    }
  }
  expect_rejected(std::move(duplicate),
                  "selected pointer authority must reject duplicate selected rows");
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

void test_selected_memcpy_raw_bir_receipt_and_rollback() {
  auto valid = selected_authority_module();
  auto raw = bir::lower_lir_to_raw_bir(valid);
  if (!raw) fail("selected memcpy import detail: " + raw.error().detail);
  expect(raw.has_value(), "selected memcpy authority must publish one Raw-BIR module");
  const auto module = raw.value().view();
  const auto functions = module.functions();
  expect(functions.size() == 1, "selected memcpy receipt must retain its function");
  const auto function = module.function(functions.front());
  expect(function.has_value(), "published selected memcpy function must resolve");
  const auto blocks = function.value().blocks();
  expect(blocks.size() == 1, "selected memcpy receipt must retain its block");
  const auto instructions = function.value().instructions(blocks.front());
  expect(instructions.has_value() && instructions.value().size() == 1,
         "selected memcpy receipt must retain exactly one Raw-BIR row");
  const auto instruction = function.value().instruction(instructions.value().front());
  expect(instruction.has_value(), "selected memcpy Raw-BIR row must resolve");
  const auto* memcpy = instruction.value().selected_memcpy();
  expect(memcpy != nullptr && memcpy->destination.value == 42 &&
             memcpy->source.value == 41 && memcpy->destination_object.value == 8 &&
             memcpy->source_object.value == 7 && memcpy->size_bytes == 24 &&
             memcpy->destination_object_owner.valid() &&
             memcpy->source_object_owner.valid() &&
             memcpy->destination_object_owner == memcpy->source_object_owner &&
             memcpy->pointer_type == bir::Type{bir::TypeKind::Pointer} &&
             memcpy->destination_live_at_site && memcpy->source_live_at_site,
         "Raw-BIR selected memcpy row must preserve typed value/object/owner/pointer/size/lifetime authority");

  auto malformed = selected_authority_module();
  for (lir::LirBlock& block : malformed.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* row = std::get_if<lir::LirMemcpyOp>(&inst))
        row->selected_authority->source_object = lir::LirObjectId{999};
    }
  }
  expect(!bir::lower_lir_to_raw_bir(malformed).has_value(),
         "malformed selected authority must roll back whole Raw-BIR publication");

  auto foreign_owner = selected_authority_module();
  const c4c::LinkNameId foreign = foreign_owner.link_names.intern("foreign_owner");
  for (lir::LirBlock& block : foreign_owner.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* row = std::get_if<lir::LirMemcpyOp>(&inst))
        row->selected_authority->source_object_owner = foreign;
    }
  }
  expect(!bir::lower_lir_to_raw_bir(foreign_owner).has_value(),
         "cross-owner selected authority must roll back whole Raw-BIR publication");

  auto non_i64_size = selected_authority_module();
  for (lir::LirBlock& block : non_i64_size.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* row = std::get_if<lir::LirMemcpyOp>(&inst))
        row->selected_authority->size_type = lir::LirTypeRef::integer(32);
    }
  }
  expect(!bir::lower_lir_to_raw_bir(non_i64_size).has_value(),
         "non-i64 selected size must roll back whole Raw-BIR publication");

  auto nonpositive_size = selected_authority_module();
  for (lir::LirBlock& block : nonpositive_size.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* row = std::get_if<lir::LirMemcpyOp>(&inst))
        row->selected_authority->size = lir::LirIntegerImmediate{0};
    }
  }
  expect(!bir::lower_lir_to_raw_bir(nonpositive_size).has_value(),
         "nonpositive selected size must roll back whole Raw-BIR publication");

  auto dead = selected_authority_module();
  for (lir::LirBlock& block : dead.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* row = std::get_if<lir::LirMemcpyOp>(&inst))
        row->selected_authority->destination_live_at_site = false;
    }
  }
  expect(!bir::lower_lir_to_raw_bir(dead).has_value(),
         "dead selected authority must roll back whole Raw-BIR publication");

  auto duplicate = selected_authority_module();
  duplicate.functions[0].blocks[0].insts.push_back(
      duplicate.functions[0].blocks[0].insts.front());
  expect(!bir::lower_lir_to_raw_bir(duplicate).has_value(),
         "duplicate selected authority row must roll back whole Raw-BIR publication");

  auto unselected = selected_authority_module();
  for (lir::LirBlock& block : unselected.functions[0].blocks) {
    for (lir::LirInst& inst : block.insts) {
      if (auto* row = std::get_if<lir::LirMemcpyOp>(&inst))
        row->selected_authority.reset();
    }
  }
  expect(!bir::lower_lir_to_raw_bir(unselected).has_value(),
         "unselected memcpy must remain unsupported and publish nothing");
}

}  // namespace

int main() {
  test_selected_current_function_pointer_authority();
  test_selected_byval_materialization_populates_authority();
  test_selected_memcpy_authority_verifier_boundary();
  test_selected_memcpy_raw_bir_receipt_and_rollback();
  return 0;
}
