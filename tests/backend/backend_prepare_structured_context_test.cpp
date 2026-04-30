#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/bir/lir_to_bir/lowering.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/codegen/lir/ir.hpp"
#include "src/shared/text_id_table.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const prepare::PreparedControlFlowFunction* find_control_flow_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_name_id = prepared.names.function_names.find(function_name);
  if (function_name_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_control_flow_function(prepared.control_flow, function_name_id);
}

prepare::PreparedBirModule legalize_only(const bir::Module& module) {
  prepare::BirPreAlloc prealloc(
      module, c4c::target_profile_from_triple("x86_64-unknown-linux-gnu"));
  prealloc.run_legalize();
  return prealloc.prepared();
}

lir::LirModule make_structured_decl_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");
  const c4c::StructNameId nested_id = module.struct_names.intern("%struct.Nested");

  module.record_struct_decl(lir::LirStructDecl{
      .name_id = pair_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i64")}},
  });
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = nested_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("%struct.Pair")}},
      .is_packed = true,
  });
  module.type_decls.push_back("%struct.Pair = type { i32, i64 }");
  module.type_decls.push_back("%struct.Nested = type <{ %struct.Pair }>");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = lir::LirRet{
      .value_str = std::string("0"),
      .type_str = "i32",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_structured_aggregate_call_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = pair_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });

  lir::LirFunction decl;
  decl.name = "id_pair";
  decl.is_declaration = true;
  decl.signature_text = "declare %struct.Pair @id_pair(%struct.Pair)";
  module.functions.push_back(std::move(decl));

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main(%struct.Pair %p)";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand("%pair.copy"),
      .return_type = lir::LirTypeRef::struct_type("%struct.Pair", pair_id),
      .callee = lir::LirOperand("@id_pair"),
      .callee_type_suffix = "(%struct.Pair)",
      .args_str = "%struct.Pair %p",
      .arg_type_refs = {lir::LirTypeRef::struct_type("%struct.Pair", pair_id)},
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("0"),
      .type_str = "i32",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int check_structured_context_population() {
  const auto lir_module = make_structured_decl_lir_module();
  const auto lowered =
      c4c::backend::try_lower_to_bir_with_options(lir_module, c4c::backend::BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return fail("structured declaration fixture did not lower to BIR");
  }

  const auto& context = lowered.module->structured_types;
  if (context.declarations.size() != 2) {
    return fail("BIR structured type spelling context did not preserve declaration count");
  }

  const bir::StructuredTypeDeclSpelling* pair = context.find_struct_decl("%struct.Pair");
  if (pair == nullptr || pair->is_packed || pair->is_opaque || pair->fields.size() != 2 ||
      pair->fields[0].type_name != "i32" || pair->fields[1].type_name != "i64") {
    return fail("BIR structured type spelling context did not preserve Pair spelling metadata");
  }

  const bir::StructuredTypeDeclSpelling* nested = context.find_struct_decl("%struct.Nested");
  if (nested == nullptr || !nested->is_packed || nested->is_opaque ||
      nested->fields.size() != 1 || nested->fields[0].type_name != "%struct.Pair") {
    return fail("BIR structured type spelling context did not preserve Nested spelling metadata");
  }

  const std::string printed = bir::print(*lowered.module);
  if (printed.find("%struct.Pair") != std::string::npos ||
      printed.find("%struct.Nested") != std::string::npos) {
    return fail("BIR printer started rendering structured context before the render step");
  }
  return 0;
}

int check_lowered_aggregate_call_prints_through_structured_context() {
  const auto lir_module = make_structured_aggregate_call_lir_module();
  if (!lir_module.type_decls.empty()) {
    return fail("structured aggregate call fixture unexpectedly has legacy type declarations");
  }
  auto lowered =
      c4c::backend::try_lower_to_bir_with_options(lir_module, c4c::backend::BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return fail("structured aggregate call fixture did not lower to BIR");
  }

  bir::CallInst* aggregate_call = nullptr;
  for (auto& function : lowered.module->functions) {
    if (function.name != "main") {
      continue;
    }
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        if (auto* call = std::get_if<bir::CallInst>(&inst)) {
          if (call->callee == "id_pair") {
            aggregate_call = call;
          }
        }
      }
    }
  }
  if (aggregate_call == nullptr) {
    return fail("structured aggregate call fixture did not produce an id_pair BIR call");
  }
  if (!aggregate_call->structured_return_type_name.has_value() ||
      *aggregate_call->structured_return_type_name != "%struct.Pair") {
    return fail("lowered aggregate call did not preserve structured return metadata");
  }
  if (!aggregate_call->result_abi.has_value() ||
      !aggregate_call->result_abi->returned_in_memory ||
      aggregate_call->return_type != bir::TypeKind::Void) {
    return fail("lowered aggregate call did not use the expected sret ABI shape");
  }
  if (!aggregate_call->return_type_name.empty()) {
    return fail("lowered aggregate call still populated legacy return_type_name fallback text");
  }

  aggregate_call->return_type_name = "legacy-return-text-should-not-render";

  const std::string printed = bir::print(*lowered.module);
  if (printed.find("bir.call void id_pair(ptr sret(size=8, align=4)") ==
      std::string::npos) {
    return fail("lowered aggregate call did not print byte-stable sret void spelling");
  }
  if (printed.find("legacy-return-text-should-not-render") != std::string::npos) {
    return fail("lowered aggregate call printer used legacy return_type_name as active authority");
  }
  if (printed.find("call %struct.Pair") != std::string::npos) {
    return fail("lowered aggregate call printer regressed to source aggregate call spelling");
  }
  return 0;
}

int check_call_return_rendering_prefers_structured_context() {
  bir::Module module;
  module.structured_types.declarations.push_back(bir::StructuredTypeDeclSpelling{
      .name = "%struct.Pair",
      .fields = {bir::StructuredTypeFieldSpelling{.type_name = "i32"},
                 bir::StructuredTypeFieldSpelling{.type_name = "i32"}},
  });

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::Void;

  bir::Block entry;
  entry.label_id = module.names.block_labels.intern("entry");
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "make_pair",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%ret.sret")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      }},
      .structured_return_type_name = "%struct.Pair",
      .return_type_name = "legacy-return-text-should-not-render",
      .return_type = bir::TypeKind::Void,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Void,
          .primary_class = bir::AbiValueClass::Memory,
          .returned_in_memory = true,
      },
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const std::string printed = bir::print(module);
  if (printed.find("bir.call void make_pair(ptr sret(size=8, align=4) %ret.sret)") ==
      std::string::npos) {
    return fail("BIR call return rendering did not use structured context for sret void spelling");
  }
  if (printed.find("legacy-return-text-should-not-render") != std::string::npos) {
    return fail("BIR call return rendering used legacy return_type_name before structured context");
  }
  return 0;
}

int check_lir_to_bir_signature_lowering_prefers_structured_metadata() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = pair_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });

  lir::LirFunction decl;
  decl.name = "structured_sig";
  decl.is_declaration = true;
  decl.signature_text = "declare void @structured_sig(void)";
  decl.return_type = c4c::TypeSpec{.base = c4c::TB_VOID};
  decl.signature_return_type_ref = lir::LirTypeRef("i32");
  decl.signature_params.push_back(
      lir::LirSignatureParam{.name = "%pair", .type = c4c::TypeSpec{.base = c4c::TB_STRUCT,
                                                                     .tag = "Pair"}});
  decl.signature_param_type_refs.push_back(
      lir::LirTypeRef::struct_type("ptr byval(%struct.Pair) align 8", pair_id));
  decl.signature_is_variadic = true;
  module.functions.push_back(std::move(decl));

  const auto lowered =
      c4c::backend::try_lower_to_bir_with_options(module, c4c::backend::BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return fail("structured signature drift fixture did not lower to BIR");
  }
  if (lowered.module->functions.size() != 1) {
    return fail("structured signature drift fixture did not produce one declaration");
  }

  const bir::Function& function = lowered.module->functions.front();
  if (function.return_type != bir::TypeKind::I32 || !function.return_abi.has_value() ||
      function.return_abi->type != bir::TypeKind::I32) {
    return fail("BIR declaration lowering used drifted signature_text return spelling");
  }
  if (!function.is_variadic) {
    return fail("BIR declaration lowering ignored structured variadic metadata");
  }
  if (function.params.size() != 1 || function.params.front().name != "%pair" ||
      function.params.front().type != bir::TypeKind::Ptr || !function.params.front().is_byval ||
      function.params.front().size_bytes != 8 || function.params.front().align_bytes != 4) {
    return fail("BIR declaration lowering ignored structured byval parameter metadata");
  }
  return 0;
}

int check_backend_layout_lookup_prefers_structured_table() {
  using c4c::backend::lir_to_bir_detail::AggregateTypeLayout;
  using c4c::backend::lir_to_bir_detail::build_backend_structured_layout_table;
  using c4c::backend::lir_to_bir_detail::build_type_decl_map;
  using c4c::backend::lir_to_bir_detail::lookup_backend_aggregate_type_layout;

  lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = pair_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });

  const auto matching_legacy_decls = build_type_decl_map({"%struct.Pair = type { i32, i32 }"});
  const auto matching_table = build_backend_structured_layout_table(
      module.struct_decls, module.struct_names, matching_legacy_decls);
  const auto matching_layout =
      lookup_backend_aggregate_type_layout("%struct.Pair", matching_legacy_decls, matching_table);
  if (matching_layout.kind != AggregateTypeLayout::Kind::Struct ||
      matching_layout.size_bytes != 8 || matching_layout.align_bytes != 4 ||
      matching_layout.fields.size() != 2 || matching_layout.fields[1].byte_offset != 4) {
    return fail("structured-present matching layout lookup did not use the structured layout");
  }

  const auto mismatched_legacy_decls =
      build_type_decl_map({"%struct.Pair = type { i64, i64 }"});
  const auto mismatched_table = build_backend_structured_layout_table(
      module.struct_decls, module.struct_names, mismatched_legacy_decls);
  const auto mismatched_layout = lookup_backend_aggregate_type_layout(
      "%struct.Pair", mismatched_legacy_decls, mismatched_table);
  if (mismatched_layout.kind != AggregateTypeLayout::Kind::Struct ||
      mismatched_layout.size_bytes != 8 || mismatched_layout.align_bytes != 4 ||
      mismatched_layout.fields.size() != 2 || mismatched_layout.fields[1].byte_offset != 4) {
    return fail("structured-present mismatched legacy shadow should keep structured layout authority");
  }

  const c4c::backend::lir_to_bir_detail::BackendStructuredLayoutTable empty_structured_table;
  const auto fallback_layout = lookup_backend_aggregate_type_layout(
      "%struct.Pair", mismatched_legacy_decls, empty_structured_table);
  if (fallback_layout.kind != AggregateTypeLayout::Kind::Struct ||
      fallback_layout.size_bytes != 16 || fallback_layout.align_bytes != 8 ||
      fallback_layout.fields.size() != 2 || fallback_layout.fields[1].byte_offset != 8) {
    return fail("structured-missing layout lookup did not preserve legacy TypeDeclMap fallback");
  }

  return 0;
}

int check_block_label_rendering_prefers_structured_identity() {
  bir::Module module;
  const c4c::BlockLabelId entry_id = module.names.block_labels.intern("entry");
  const c4c::BlockLabelId pred_id = module.names.block_labels.intern("pred");
  const c4c::BlockLabelId then_id = module.names.block_labels.intern("then");
  const c4c::BlockLabelId else_id = module.names.block_labels.intern("else");
  const c4c::BlockLabelId join_id = module.names.block_labels.intern("join");

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "raw.entry";
  entry.label_id = entry_id;
  entry.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%phi"),
      .incomings = {bir::PhiIncoming{
          .label = "raw.pred",
          .value = bir::Value::immediate_i32(7),
          .label_id = pred_id,
      }},
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%addr"),
      .slot_name = "%slot",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::Label,
          .base_name = "raw.join.addr",
          .base_label_id = join_id,
      },
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "%cond"),
      .true_label = "raw.then",
      .false_label = "raw.else",
      .true_label_id = then_id,
      .false_label_id = else_id,
  };
  function.blocks.push_back(std::move(entry));

  bir::Block then_block;
  then_block.label = "raw.then";
  then_block.label_id = then_id;
  then_block.terminator = bir::BranchTerminator{
      .target_label = "raw.join",
      .target_label_id = join_id,
  };
  function.blocks.push_back(std::move(then_block));

  bir::Block else_block;
  else_block.label = "raw.else";
  else_block.label_id = else_id;
  else_block.terminator = bir::BranchTerminator{
      .target_label = "raw.join",
      .target_label_id = join_id,
  };
  function.blocks.push_back(std::move(else_block));

  bir::Block join_block;
  join_block.label = "raw.join";
  join_block.label_id = join_id;
  join_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  function.blocks.push_back(std::move(join_block));

  function.local_slots.push_back(bir::LocalSlot{
      .name = "%phi.slot",
      .type = bir::TypeKind::I32,
      .phi_observation = bir::PhiObservation{
          .result = bir::Value::named(bir::TypeKind::I32, "%observed"),
          .incomings = {bir::PhiIncoming{
                            .label = "raw.then",
                            .value = bir::Value::immediate_i32(1),
                            .label_id = then_id,
                        },
                        bir::PhiIncoming{
                            .label = "raw.missing",
                            .value = bir::Value::immediate_i32(2),
                        }},
      },
  });

  module.functions.push_back(std::move(function));

  const std::string printed = bir::print(module);
  if (printed.find("entry:\n") == std::string::npos ||
      printed.find("then:\n") == std::string::npos ||
      printed.find("join:\n") == std::string::npos) {
    return fail("BIR printer did not use structured ids for block headers");
  }
  if (printed.find("bir.cond_br i1 %cond, then, else") == std::string::npos ||
      printed.find("bir.br join") == std::string::npos) {
    return fail("BIR printer did not use structured ids for branch labels");
  }
  if (printed.find("bir.phi i32 [pred, 7]") == std::string::npos ||
      printed.find("; semantic_phi %observed = bir.phi i32 [then, 1] [raw.missing, 2]") ==
          std::string::npos) {
    return fail("BIR printer did not use structured ids for phi labels with raw fallback");
  }
  if (printed.find("addr join") == std::string::npos) {
    return fail("BIR printer did not use structured ids for label memory addresses");
  }
  if (printed.find("raw.entry") != std::string::npos ||
      printed.find("raw.then") != std::string::npos ||
      printed.find("raw.join") != std::string::npos ||
      printed.find("raw.pred") != std::string::npos) {
    return fail("BIR printer rendered raw label text despite available structured ids");
  }
  return 0;
}

int check_block_label_rendering_keeps_raw_fallbacks() {
  bir::Module module;
  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "raw.entry";
  entry.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%phi"),
      .incomings = {bir::PhiIncoming{
          .label = "raw.pred",
          .value = bir::Value::immediate_i32(9),
      }},
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%addr"),
      .slot_name = "%slot",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::Label,
          .base_name = "raw.addr",
      },
  });
  entry.terminator = bir::BranchTerminator{.target_label = "raw.join"};
  function.blocks.push_back(std::move(entry));

  bir::Block join_block;
  join_block.label = "raw.join";
  join_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  function.blocks.push_back(std::move(join_block));

  function.local_slots.push_back(bir::LocalSlot{
      .name = "%phi.slot",
      .type = bir::TypeKind::I32,
      .phi_observation = bir::PhiObservation{
          .result = bir::Value::named(bir::TypeKind::I32, "%observed"),
          .incomings = {bir::PhiIncoming{
              .label = "raw.pred",
              .value = bir::Value::immediate_i32(3),
          }},
      },
  });
  module.functions.push_back(std::move(function));

  const std::string printed = bir::print(module);
  if (printed.find("raw.entry:\n") == std::string::npos ||
      printed.find("bir.br raw.join") == std::string::npos ||
      printed.find("bir.phi i32 [raw.pred, 9]") == std::string::npos ||
      printed.find("; semantic_phi %observed = bir.phi i32 [raw.pred, 3]") ==
          std::string::npos ||
      printed.find("addr raw.addr") == std::string::npos) {
    return fail("BIR printer did not preserve raw label fallback spelling");
  }
  return 0;
}

int check_prepared_control_flow_prefers_structured_label_ids() {
  bir::Module module;
  const c4c::BlockLabelId entry_id = module.names.block_labels.intern("entry");
  const c4c::BlockLabelId then_id = module.names.block_labels.intern("then");
  const c4c::BlockLabelId else_id = module.names.block_labels.intern("else");
  const c4c::BlockLabelId join_id = module.names.block_labels.intern("join");

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "raw.entry";
  entry.label_id = entry_id;
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "%cond"),
      .true_label = "raw.then",
      .false_label = "raw.else",
      .true_label_id = then_id,
      .false_label_id = else_id,
  };
  function.blocks.push_back(std::move(entry));

  bir::Block then_block;
  then_block.label = "raw.then";
  then_block.label_id = then_id;
  then_block.terminator = bir::BranchTerminator{
      .target_label = "raw.join",
      .target_label_id = join_id,
  };
  function.blocks.push_back(std::move(then_block));

  bir::Block else_block;
  else_block.label = "raw.else";
  else_block.label_id = else_id;
  else_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  function.blocks.push_back(std::move(else_block));

  bir::Block join_block;
  join_block.label = "raw.join";
  join_block.label_id = join_id;
  join_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));

  const auto prepared = legalize_only(module);
  const auto* control_flow = find_control_flow_function(prepared, "main");
  if (control_flow == nullptr || control_flow->blocks.size() != 4 ||
      control_flow->branch_conditions.size() != 1) {
    return fail("prepared control-flow did not publish the expected structured-id fixture");
  }

  const auto& prepared_entry = control_flow->blocks[0];
  if (prepare::prepared_block_label(prepared.names, prepared_entry.block_label) != "entry" ||
      prepare::prepared_block_label(prepared.names, prepared_entry.true_label) != "then" ||
      prepare::prepared_block_label(prepared.names, prepared_entry.false_label) != "else") {
    return fail("prepared conditional branch labels did not prefer structured BIR ids");
  }

  const auto& prepared_then = control_flow->blocks[1];
  if (prepare::prepared_block_label(prepared.names, prepared_then.block_label) != "then" ||
      prepare::prepared_block_label(prepared.names, prepared_then.branch_target_label) != "join") {
    return fail("prepared branch target label did not prefer structured BIR ids");
  }

  const auto& condition = control_flow->branch_conditions.front();
  if (prepare::prepared_block_label(prepared.names, condition.block_label) != "entry" ||
      prepare::prepared_block_label(prepared.names, condition.true_label) != "then" ||
      prepare::prepared_block_label(prepared.names, condition.false_label) != "else") {
    return fail("prepared branch condition labels did not prefer structured BIR ids");
  }

  return 0;
}

int check_prepared_control_flow_keeps_raw_label_fallbacks() {
  bir::Module module;

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "raw.entry";
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "%cond"),
      .true_label = "raw.then",
      .false_label = "raw.else",
  };
  function.blocks.push_back(std::move(entry));

  bir::Block then_block;
  then_block.label = "raw.then";
  then_block.terminator = bir::BranchTerminator{.target_label = "raw.join"};
  function.blocks.push_back(std::move(then_block));

  bir::Block else_block;
  else_block.label = "raw.else";
  else_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  function.blocks.push_back(std::move(else_block));

  bir::Block join_block;
  join_block.label = "raw.join";
  join_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));

  const auto prepared = legalize_only(module);
  const auto* control_flow = find_control_flow_function(prepared, "main");
  if (control_flow == nullptr || control_flow->blocks.size() != 4 ||
      control_flow->branch_conditions.size() != 1) {
    return fail("prepared control-flow did not publish the expected raw fallback fixture");
  }

  const auto& prepared_entry = control_flow->blocks[0];
  const auto& prepared_then = control_flow->blocks[1];
  const auto& condition = control_flow->branch_conditions.front();
  if (prepare::prepared_block_label(prepared.names, prepared_entry.block_label) != "raw.entry" ||
      prepare::prepared_block_label(prepared.names, prepared_entry.true_label) != "raw.then" ||
      prepare::prepared_block_label(prepared.names, prepared_entry.false_label) != "raw.else" ||
      prepare::prepared_block_label(prepared.names, prepared_then.branch_target_label) !=
          "raw.join" ||
      prepare::prepared_block_label(prepared.names, condition.block_label) != "raw.entry" ||
      prepare::prepared_block_label(prepared.names, condition.true_label) != "raw.then" ||
      prepare::prepared_block_label(prepared.names, condition.false_label) != "raw.else") {
    return fail("prepared control-flow did not preserve raw label fallback spelling");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = check_structured_context_population(); status != 0) {
    return status;
  }
  if (const int status = check_lowered_aggregate_call_prints_through_structured_context();
      status != 0) {
    return status;
  }
  if (const int status = check_call_return_rendering_prefers_structured_context();
      status != 0) {
    return status;
  }
  if (const int status = check_lir_to_bir_signature_lowering_prefers_structured_metadata();
      status != 0) {
    return status;
  }
  if (const int status = check_backend_layout_lookup_prefers_structured_table(); status != 0) {
    return status;
  }
  if (const int status = check_block_label_rendering_prefers_structured_identity();
      status != 0) {
    return status;
  }
  if (const int status = check_block_label_rendering_keeps_raw_fallbacks(); status != 0) {
    return status;
  }
  if (const int status = check_prepared_control_flow_prefers_structured_label_ids();
      status != 0) {
    return status;
  }
  return check_prepared_control_flow_keeps_raw_label_fallbacks();
}
