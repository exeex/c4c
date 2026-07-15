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

lir::LirModule native_memset_authority_module() {
  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  const c4c::LinkNameId owner = module.link_names.intern("native_memset_owner");
  const lir::LirCurrentFunctionLocalObjectPointer pointer{
      .pointer_definition = lir::LirValueId{1},
      .object = lir::LirObjectId{1},
      .owner = owner,
      .pointer_type = lir::LirTypeRef("ptr"),
      .pointee_type = lir::LirTypeRef("i32"),
      .live = true,
  };
  lir::LirFunction function;
  function.name = "native_memset_authority";
  function.link_name_id = owner;
  function.signature_text = "declare void @native_memset_authority()";
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand::ssa("%slot", pointer.pointer_definition),
      .type_str = pointer.pointee_type,
      .local_object_authority = pointer,
  });
  lir::LirBlock block;
  block.id = lir::LirBlockId{0};
  block.label = "entry";
  block.insts.push_back(lir::LirMemsetOp{
      .dst = lir::LirOperand::ssa("%slot", pointer.pointer_definition),
      .byte_val = lir::LirOperand::integer("0", 0),
      .size = lir::LirOperand::integer("4", 4),
      .requires_native_memory_va_authority = true,
      .dst_authority = lir::LirMemoryVaPointerAuthority{pointer},
      .byte_authority = lir::LirMemoryVaIntegerAuthority{
          lir::LirTypeRef::integer(8), lir::LirIntegerImmediate{0}},
      .size_authority = lir::LirMemoryVaIntegerAuthority{
          lir::LirTypeRef::integer(64), lir::LirIntegerImmediate{4}},
  });
  block.terminator = lir::LirRet{std::nullopt, lir::LirTypeRef("void")};
  function.blocks.push_back(std::move(block));
  function.entry = lir::LirBlockId{0};
  module.functions.push_back(std::move(function));
  return module;
}

void test_native_memory_va_authority_verifier_boundary() {
  auto valid = native_memset_authority_module();
  lir::verify_module(valid);

  auto missing = native_memset_authority_module();
  std::get<lir::LirMemsetOp>(missing.functions[0].blocks[0].insts[0])
      .dst_authority.reset();
  expect_rejected(std::move(missing), "selected memset must reject missing authority");

  auto foreign = native_memset_authority_module();
  const auto foreign_owner = foreign.link_names.intern("foreign_memset_owner");
  std::get<lir::LirMemsetOp>(foreign.functions[0].blocks[0].insts[0])
      .dst_authority->local_pointer.owner = foreign_owner;
  expect_rejected(std::move(foreign), "selected memset must reject foreign authority");

  auto wrong_type = native_memset_authority_module();
  std::get<lir::LirMemsetOp>(wrong_type.functions[0].blocks[0].insts[0])
      .dst_authority->local_pointer.pointer_type = lir::LirTypeRef::integer(32);
  expect_rejected(std::move(wrong_type), "selected memset must reject type-mismatched authority");

  auto dead = native_memset_authority_module();
  std::get<lir::LirMemsetOp>(dead.functions[0].blocks[0].insts[0])
      .dst_authority->local_pointer.live = false;
  expect_rejected(std::move(dead), "selected memset must reject dead authority");

  const auto va_binding = std::get<lir::LirMemsetOp>(
      native_memset_authority_module().functions[0].blocks[0].insts[0]).dst_authority;
  auto selected_va_start = native_memset_authority_module();
  selected_va_start.functions[0].blocks[0].insts.push_back(lir::LirVaStartOp{
      .ap_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}),
      .requires_native_memory_va_authority = true,
      .ap_authority = va_binding,
  });
  lir::verify_module(selected_va_start);
  auto missing_va_start = selected_va_start;
  std::get<lir::LirVaStartOp>(missing_va_start.functions[0].blocks[0].insts.back())
      .ap_authority.reset();
  expect_rejected(std::move(missing_va_start),
                  "selected va_start must reject missing pointer authority");

  auto selected_va_end = native_memset_authority_module();
  selected_va_end.functions[0].blocks[0].insts.push_back(lir::LirVaEndOp{
      .ap_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}),
      .requires_native_memory_va_authority = true,
      .ap_authority = va_binding,
  });
  lir::verify_module(selected_va_end);
  auto dead_va_end = selected_va_end;
  std::get<lir::LirVaEndOp>(dead_va_end.functions[0].blocks[0].insts.back())
      .ap_authority->local_pointer.live = false;
  expect_rejected(std::move(dead_va_end),
                  "selected va_end must reject dead pointer authority");

  auto selected_va_copy = native_memset_authority_module();
  selected_va_copy.functions[0].blocks[0].insts.push_back(lir::LirVaCopyOp{
      .dst_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}),
      .src_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}),
      .requires_native_memory_va_authority = true,
      .dst_authority = va_binding,
      .src_authority = va_binding,
  });
  lir::verify_module(selected_va_copy);
  auto missing_va_copy = selected_va_copy;
  std::get<lir::LirVaCopyOp>(missing_va_copy.functions[0].blocks[0].insts.back())
      .dst_authority.reset();
  expect_rejected(std::move(missing_va_copy),
                  "selected va_copy must reject incomplete pointer authority");

  auto foreign_va_copy = selected_va_copy;
  const auto foreign_va_copy_owner =
      foreign_va_copy.link_names.intern("foreign_va_copy_owner");
  std::get<lir::LirVaCopyOp>(foreign_va_copy.functions[0].blocks[0].insts.back())
      .src_authority->local_pointer.owner = foreign_va_copy_owner;
  expect_rejected(std::move(foreign_va_copy),
                  "selected va_copy must reject foreign pointer authority");

  auto dead_va_copy = selected_va_copy;
  std::get<lir::LirVaCopyOp>(dead_va_copy.functions[0].blocks[0].insts.back())
      .dst_authority->local_pointer.live = false;
  expect_rejected(std::move(dead_va_copy),
                  "selected va_copy must reject dead pointer authority");

  auto wrong_size = native_memset_authority_module();
  std::get<lir::LirMemsetOp>(wrong_size.functions[0].blocks[0].insts[0])
      .size_authority->value = lir::LirIntegerImmediate{8};
  expect_rejected(std::move(wrong_size), "selected memset must reject size disagreement");

  const auto compatibility_pointer = native_memset_authority_module();
  const auto binding = std::get<lir::LirMemsetOp>(
      compatibility_pointer.functions[0].blocks[0].insts[0]).dst_authority;
  auto reject_unselected_fields = [&](lir::LirInst inst, const std::string& message) {
    auto module = native_memset_authority_module();
    module.functions[0].blocks[0].insts.push_back(std::move(inst));
    expect_rejected(std::move(module), message);
  };
  reject_unselected_fields(lir::LirMemcpyOp{
      .dst = lir::LirOperand::ssa("%slot", lir::LirValueId{1}),
      .src = lir::LirOperand::ssa("%slot", lir::LirValueId{1}),
      .size = lir::LirOperand::integer("4", 4), .dst_authority = binding},
      "unselected memcpy authority fields must reject");
  reject_unselected_fields(lir::LirVaStartOp{
      .ap_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}), .ap_authority = binding},
      "unselected va_start authority fields must reject");
  reject_unselected_fields(lir::LirVaEndOp{
      .ap_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}), .ap_authority = binding},
      "unselected va_end authority fields must reject");
  reject_unselected_fields(lir::LirVaCopyOp{
      .dst_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}),
      .src_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}), .dst_authority = binding},
      "unselected va_copy authority fields must reject");
  reject_unselected_fields(lir::LirVaArgOp{
      .result = lir::LirOperand::ssa("%va", lir::LirValueId{2}),
      .ap_ptr = lir::LirOperand::ssa("%slot", lir::LirValueId{1}),
      .type_str = lir::LirTypeRef::integer(32), .ap_authority = binding},
      "unselected va_arg authority fields must reject");
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

hir::Module local_aggregate_zero_memset_module(int array_size = 4) {
  hir::Module module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);

  c4c::TypeSpec int_type{};
  int_type.base = c4c::TB_INT;
  int_type.enum_underlying_base = c4c::TB_VOID;
  int_type.array_size = -1;
  c4c::TypeSpec array_type = int_type;
  array_type.array_rank = 1;
  array_type.array_size = array_size;
  array_type.array_dims[0] = array_size;

  hir::Expr zero;
  zero.id = module.alloc_expr_id();
  zero.type.spec = int_type;
  zero.type.category = hir::ValueCategory::RValue;
  zero.payload = hir::IntLiteral{0, false};

  hir::LocalDecl local;
  local.id = module.alloc_local_id();
  local.name = "zeroed";
  local.type.spec = array_type;
  local.type.category = hir::ValueCategory::LValue;
  local.init = zero.id;

  hir::Function function;
  function.id = module.alloc_function_id();
  function.name = "local_aggregate_zero_memset";
  function.link_name_id = module.link_names.intern(function.name);
  function.return_type.spec.base = c4c::TB_VOID;
  function.return_type.spec.enum_underlying_base = c4c::TB_VOID;
  function.return_type.spec.array_size = -1;
  function.entry = module.alloc_block_id();
  hir::Block entry;
  entry.id = function.entry;
  entry.stmts.push_back(hir::Stmt{.payload = local});
  function.blocks.push_back(std::move(entry));
  module.expr_pool.push_back(std::move(zero));
  module.index_function_decl(function);
  module.functions.push_back(std::move(function));
  return module;
}

void test_local_aggregate_zero_memset_populates_authority() {
  const lir::LirModule module = lir::lower(local_aggregate_zero_memset_module());
  const auto& function = module.functions.front();
  const lir::LirMemsetOp* memset = nullptr;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* candidate = std::get_if<lir::LirMemsetOp>(&inst)) {
        memset = candidate;
        break;
      }
    }
    if (memset) break;
  }
  expect(memset != nullptr, "aggregate-zero local lowering must emit memset");
  expect(memset->requires_native_memory_va_authority && memset->dst_authority &&
             memset->byte_authority && memset->size_authority,
         "aggregate-zero local memset must opt into native authority");
  const auto& pointer = memset->dst_authority->local_pointer;
  expect(memset->dst.value_id() && *memset->dst.value_id() == pointer.pointer_definition &&
             pointer.owner == function.link_name_id && pointer.object.valid() &&
             pointer.pointer_type.kind() == lir::LirTypeKind::Pointer && pointer.live &&
             memset->byte_authority->type == lir::LirTypeRef::integer(8) &&
             memset->byte_authority->value.value == 0 &&
             memset->size_authority->type == lir::LirTypeRef::integer(64) &&
             memset->size_authority->value.value == 16,
         "aggregate-zero local memset must retain pointer/object/owner/type/live and typed size facts");
  lir::verify_module(module);
}

void test_zero_sized_local_aggregate_memset_remains_compatibility_only() {
  const lir::LirModule module = lir::lower(local_aggregate_zero_memset_module(0));
  const auto& function = module.functions.front();
  const lir::LirMemsetOp* memset = nullptr;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* candidate = std::get_if<lir::LirMemsetOp>(&inst)) {
        memset = candidate;
        break;
      }
    }
    if (memset) break;
  }
  expect(memset != nullptr, "zero-sized aggregate local lowering must emit compatibility memset");
  expect(!memset->requires_native_memory_va_authority && !memset->dst_authority &&
             !memset->byte_authority && !memset->size_authority &&
             memset->size.str() == "0",
         "zero-sized aggregate memset must remain compatibility-only");
  lir::verify_module(module);
}

hir::Module direct_local_va_lifecycle_module() {
  hir::Module module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);

  c4c::TypeSpec va_list_type{};
  va_list_type.base = c4c::TB_VA_LIST;
  va_list_type.enum_underlying_base = c4c::TB_VOID;
  va_list_type.array_size = -1;
  c4c::TypeSpec void_type{};
  void_type.base = c4c::TB_VOID;
  void_type.enum_underlying_base = c4c::TB_VOID;
  void_type.array_size = -1;

  hir::LocalDecl local;
  local.id = module.alloc_local_id();
  local.name = "ap";
  local.type.spec = va_list_type;
  local.type.category = hir::ValueCategory::LValue;

  hir::LocalDecl copy_local;
  copy_local.id = module.alloc_local_id();
  copy_local.name = "ap_copy";
  copy_local.type.spec = va_list_type;
  copy_local.type.category = hir::ValueCategory::LValue;

  hir::Expr ap_ref;
  ap_ref.id = module.alloc_expr_id();
  ap_ref.type.spec = va_list_type;
  ap_ref.type.category = hir::ValueCategory::LValue;
  ap_ref.payload = hir::DeclRef{.name = "ap", .local = local.id};
  hir::Expr copy_ap_ref;
  copy_ap_ref.id = module.alloc_expr_id();
  copy_ap_ref.type.spec = va_list_type;
  copy_ap_ref.type.category = hir::ValueCategory::LValue;
  copy_ap_ref.payload = hir::DeclRef{.name = "ap_copy", .local = copy_local.id};
  hir::Expr va_start_callee;
  va_start_callee.id = module.alloc_expr_id();
  va_start_callee.type.spec = void_type;
  va_start_callee.payload = hir::DeclRef{.name = "__builtin_va_start"};
  hir::Expr va_end_callee;
  va_end_callee.id = module.alloc_expr_id();
  va_end_callee.type.spec = void_type;
  va_end_callee.payload = hir::DeclRef{.name = "__builtin_va_end"};
  hir::Expr va_copy_callee;
  va_copy_callee.id = module.alloc_expr_id();
  va_copy_callee.type.spec = void_type;
  va_copy_callee.payload = hir::DeclRef{.name = "__builtin_va_copy"};
  hir::Expr va_start;
  va_start.id = module.alloc_expr_id();
  va_start.type.spec = void_type;
  va_start.payload = hir::CallExpr{
      .callee = va_start_callee.id, .args = {ap_ref.id}, .builtin_id = c4c::BuiltinId::VaStart};
  hir::Expr va_end;
  va_end.id = module.alloc_expr_id();
  va_end.type.spec = void_type;
  va_end.payload = hir::CallExpr{
      .callee = va_end_callee.id, .args = {ap_ref.id}, .builtin_id = c4c::BuiltinId::VaEnd};
  hir::Expr va_copy;
  va_copy.id = module.alloc_expr_id();
  va_copy.type.spec = void_type;
  va_copy.payload = hir::CallExpr{
      .callee = va_copy_callee.id,
      .args = {copy_ap_ref.id, ap_ref.id},
      .builtin_id = c4c::BuiltinId::VaCopy};

  hir::Function function;
  function.id = module.alloc_function_id();
  function.name = "direct_local_va_lifecycle";
  function.link_name_id = module.link_names.intern(function.name);
  function.return_type.spec = void_type;
  function.entry = module.alloc_block_id();
  hir::Block entry;
  entry.id = function.entry;
  entry.stmts.push_back(hir::Stmt{.payload = local});
  entry.stmts.push_back(hir::Stmt{.payload = copy_local});
  entry.stmts.push_back(hir::Stmt{.payload = hir::ExprStmt{va_start.id}});
  entry.stmts.push_back(hir::Stmt{.payload = hir::ExprStmt{va_copy.id}});
  entry.stmts.push_back(hir::Stmt{.payload = hir::ExprStmt{va_end.id}});
  function.blocks.push_back(std::move(entry));
  module.expr_pool.push_back(std::move(ap_ref));
  module.expr_pool.push_back(std::move(copy_ap_ref));
  module.expr_pool.push_back(std::move(va_start_callee));
  module.expr_pool.push_back(std::move(va_end_callee));
  module.expr_pool.push_back(std::move(va_copy_callee));
  module.expr_pool.push_back(std::move(va_start));
  module.expr_pool.push_back(std::move(va_end));
  module.expr_pool.push_back(std::move(va_copy));
  module.index_function_decl(function);
  module.functions.push_back(std::move(function));
  return module;
}

void test_direct_local_va_lifecycle_populates_authority() {
  const lir::LirModule module = lir::lower(direct_local_va_lifecycle_module());
  const auto& function = module.functions.front();
  const lir::LirVaStartOp* va_start = nullptr;
  const lir::LirVaEndOp* va_end = nullptr;
  const lir::LirVaCopyOp* va_copy = nullptr;
  for (const auto& block : function.blocks) for (const auto& inst : block.insts) {
    if (const auto* candidate = std::get_if<lir::LirVaStartOp>(&inst)) va_start = candidate;
    if (const auto* candidate = std::get_if<lir::LirVaEndOp>(&inst)) va_end = candidate;
    if (const auto* candidate = std::get_if<lir::LirVaCopyOp>(&inst)) va_copy = candidate;
  }
  expect(va_start && va_end && va_copy,
         "direct local va-list lowering must emit va_start, va_copy, and va_end");
  const auto check = [&](const auto& op, const char* message) {
    expect(op->requires_native_memory_va_authority && op->ap_authority &&
               op->ap_ptr.value_id() &&
               *op->ap_ptr.value_id() == op->ap_authority->local_pointer.pointer_definition &&
               op->ap_authority->local_pointer.owner == function.link_name_id &&
               op->ap_authority->local_pointer.object.valid() &&
               op->ap_authority->local_pointer.pointer_type.kind() == lir::LirTypeKind::Pointer &&
               op->ap_authority->local_pointer.live,
           message);
  };
  check(va_start, "direct local va_start must retain native pointer authority");
  check(va_end, "direct local va_end must retain native pointer authority");
  expect(va_copy->requires_native_memory_va_authority && va_copy->dst_authority &&
             va_copy->src_authority && va_copy->dst_ptr.value_id() &&
             va_copy->src_ptr.value_id() &&
             *va_copy->dst_ptr.value_id() ==
                 va_copy->dst_authority->local_pointer.pointer_definition &&
             *va_copy->src_ptr.value_id() ==
                 va_copy->src_authority->local_pointer.pointer_definition &&
             va_copy->dst_authority->local_pointer.owner == function.link_name_id &&
             va_copy->src_authority->local_pointer.owner == function.link_name_id &&
             va_copy->dst_authority->local_pointer.object.valid() &&
             va_copy->src_authority->local_pointer.object.valid() &&
             va_copy->dst_authority->local_pointer.live &&
             va_copy->src_authority->local_pointer.live,
         "direct local va_copy must retain both native pointer authorities");
  lir::verify_module(module);
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
  test_local_aggregate_zero_memset_populates_authority();
  test_zero_sized_local_aggregate_memset_remains_compatibility_only();
  test_direct_local_va_lifecycle_populates_authority();
  test_native_memory_va_authority_verifier_boundary();
  test_selected_current_function_pointer_authority();
  test_selected_byval_materialization_populates_authority();
  test_selected_memcpy_authority_verifier_boundary();
  test_selected_memcpy_raw_bir_receipt_and_rollback();
  return 0;
}
