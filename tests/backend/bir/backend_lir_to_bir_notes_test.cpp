#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/bir/lir_to_bir/lowering.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace lir = c4c::codegen::lir;

using c4c::backend::BirLoweringNote;
using c4c::backend::BirLoweringOptions;
using c4c::backend::bir::TypeKind;
using c4c::backend::try_lower_to_bir_with_options;
using c4c::codegen::lir::LirBlock;
using c4c::codegen::lir::LirCastKind;
using c4c::codegen::lir::LirCastOp;
using c4c::codegen::lir::LirCallOp;
using c4c::codegen::lir::LirCmpOp;
using c4c::codegen::lir::LirGepOp;
using c4c::codegen::lir::LirFunction;
using c4c::codegen::lir::LirGlobal;
using c4c::codegen::lir::LirInlineAsmOp;
using c4c::codegen::lir::LirBinOp;
using c4c::codegen::lir::LirCondBr;
using c4c::codegen::lir::LirLoadOp;
using c4c::codegen::lir::LirModule;
using c4c::codegen::lir::LirOperand;
using c4c::codegen::lir::LirRet;
using c4c::codegen::lir::LirAllocaOp;
using c4c::codegen::lir::LirAbsOp;
using c4c::codegen::lir::LirBr;
using c4c::codegen::lir::LirStackSaveOp;
using c4c::codegen::lir::LirStoreOp;
using c4c::codegen::lir::LirPhiOp;
using c4c::codegen::lir::LirSelectOp;
using c4c::codegen::lir::LirSwitch;
using c4c::codegen::lir::LirVaArgOp;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bool contains_note(const std::vector<BirLoweringNote>& notes,
                   std::string_view phase,
                   std::string_view needle) {
  for (const auto& note : notes) {
    if (note.phase == phase && note.message.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

lir::LirCallSignature void_call_signature(
    std::vector<std::string> fixed_param_types,
    std::vector<lir::LirTypeRef> fixed_param_type_refs = {}) {
  lir::LirCallSignature signature;
  signature.return_type_ref = lir::LirTypeRef("void");
  signature.fixed_param_types = std::move(fixed_param_types);
  if (fixed_param_type_refs.empty()) {
    fixed_param_type_refs.reserve(signature.fixed_param_types.size());
    for (const std::string& type : signature.fixed_param_types) {
      fixed_param_type_refs.push_back(lir::LirTypeRef(type));
    }
  }
  signature.fixed_param_type_refs = std::move(fixed_param_type_refs);
  return signature;
}

LirModule make_admitted_scalar_float_globals_module();
LirModule make_f128_scalar_constant_binop_fails_closed_module();
LirModule make_admitted_scalar_i16_globals_module();
int expect_aarch64_extern_data_global_uses_got_policy();
LirModule make_admitted_aggregate_pointer_field_global_module();
LirModule make_admitted_aggregate_zero_sized_member_global_module();
LirModule make_admitted_aggregate_string_array_field_global_module();
LirModule make_admitted_aggregate_long_double_field_global_module();
LirModule make_structured_block_label_id_module();
LirModule make_dynamic_indexed_gep_global_member_array_module();

int expect_link_name_id_symbol_identity_survives_drifted_display_names();
int expect_link_name_id_global_identity_rejects_unresolved_id();
int expect_link_name_id_extern_identity_rejects_unresolved_id();
int expect_dynamic_global_scalar_array_loads_carry_link_name_id();
int expect_dynamic_global_scalar_array_loads_reject_missing_link_name_spelling();
int expect_dynamic_global_scalar_array_loads_keep_no_id_compatibility();
int expect_pointer_initializer_symbol_names_carry_link_name_id();
int expect_pointer_value_symbol_identity_carrier();
int expect_addressed_global_pointer_provenance_uses_link_name_id_keys();
int expect_bir_verifier_rejects_known_link_name_mismatches();
int expect_bir_verifier_rejects_local_slot_id_mismatches();
int expect_string_pool_direct_call_bridge_prefers_function_link_name_id();
int expect_string_pool_direct_call_bridge_requires_text_id();
int expect_legacy_byval_call_arg_without_type_refs_still_lowers();
int expect_metadata_rich_byval_call_arg_without_struct_id_fails_closed();
int expect_metadata_rich_byval_call_arg_mismatch_fails_closed();
int expect_direct_call_prefers_structured_callee_signature_over_stale_suffix();
int expect_direct_call_structured_byval_signature_materializes_aggregate_abi();
int expect_direct_call_structured_byval_signature_mismatch_fails_closed();
int expect_metadata_rich_direct_call_without_link_name_id_fails_closed();
int expect_metadata_rich_direct_call_with_unknown_link_name_id_fails_closed();
int expect_raw_direct_call_without_metadata_still_lowers();
int expect_metadata_rich_direct_call_without_signature_fails_closed();
int expect_indirect_call_prefers_structured_callee_signature_over_stale_suffix();
int expect_indirect_call_signature_mismatch_fails_despite_stale_suffix_match();
int expect_aarch64_direct_hfa_call_uses_fp_lanes_not_byval();
int expect_aarch64_variadic_hfa_call_uses_fp_lanes();
int expect_structured_incoming_byval_param_materializes_from_type_ref();
int expect_aarch64_crc32w_intrinsic_carries_bir_semantics();
int expect_aarch64_v16i8_vector_load_intrinsic_carries_bir_semantics();
int expect_aarch64_v16i8_vector_add_intrinsic_carries_bir_semantics();
int expect_aarch64_dmb_intrinsic_carries_bir_semantics_without_selection();
int expect_aarch64_dc_cvau_intrinsic_carries_cache_semantics_without_selection();
int expect_aarch64_semantic_intrinsic_fail_closed_cases();
int expect_incoming_byval_param_with_missing_struct_layout_fails_closed();
int expect_incoming_byval_param_with_mismatched_struct_id_fails_closed();
int expect_incoming_byval_param_with_opaque_struct_layout_fails_closed();
int expect_metadata_rich_incoming_byval_param_without_struct_id_fails_closed();
int expect_metadata_rich_incoming_byval_param_without_byval_flag_fails_closed();
int expect_non_aarch64_metadata_rich_incoming_byval_param_without_struct_id_uses_legacy_layout();
int expect_legacy_incoming_byval_param_without_signature_type_ref_uses_legacy_layout();
int expect_mixed_scalar_and_aggregate_params_materialize_late_aggregate_param();
int expect_aarch64_hfa_va_arg_uses_aggregate_helper_handoff();
int expect_structured_signature_return_materializes_sret_from_type_ref();
int expect_metadata_rich_signature_return_without_struct_id_fails_closed();
int expect_signature_return_with_mismatched_struct_id_fails_closed();

int expect_failure_notes(std::string_view case_name,
                         const LirModule& module,
                         std::string_view module_summary,
                         std::string_view function_failure,
                         std::string_view module_failure,
                         const char* missing_module_summary,
                         const char* missing_function_note,
                         const char* missing_module_note) {
  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail((std::string("expected semantic BIR lowering to fail for ") +
                 std::string(case_name))
                    .c_str());
  }
  if (!contains_note(result.notes, "module", module_summary)) {
    return fail(missing_module_summary);
  }
  if (!contains_note(result.notes, "function", function_failure)) {
    return fail(missing_function_note);
  }
  if (!contains_note(result.notes, "module", module_failure)) {
    return fail(missing_module_note);
  }
  return 0;
}

int expect_success_without_function_note(std::string_view case_name,
                                         const LirModule& module,
                                         std::string_view unexpected_function_failure,
                                         std::string_view unexpected_module_failure,
                                         const char* unexpected_function_note,
                                         const char* unexpected_module_note) {
  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail((std::string("expected semantic BIR lowering to succeed for ") +
                 std::string(case_name))
                    .c_str());
  }
  if (contains_note(result.notes, "function", unexpected_function_failure)) {
    return fail(unexpected_function_note);
  }
  if (contains_note(result.notes, "module", unexpected_module_failure)) {
    return fail(unexpected_module_note);
  }
  return 0;
}

int expect_admitted_scalar_float_globals() {
  auto result = try_lower_to_bir_with_options(make_admitted_scalar_float_globals_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("expected semantic BIR lowering to admit scalar floating globals");
  }

  const auto find_global = [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : result.module->globals) {
      if (global.name == name) {
        return &global;
      }
    }
    return nullptr;
  };

  const auto* float_global = find_global("gf");
  if (float_global == nullptr || float_global->type != TypeKind::F32 ||
      !float_global->initializer.has_value() ||
      *float_global->initializer != c4c::backend::bir::Value::immediate_f32_bits(0x3FA00000U)) {
    return fail("scalar float globals should lower into an admitted F32 BIR initializer lane");
  }

  const auto* double_global = find_global("gd");
  if (double_global == nullptr || double_global->type != TypeKind::F64 ||
      !double_global->initializer.has_value() ||
      *double_global->initializer !=
          c4c::backend::bir::Value::immediate_f64_bits(0x4059000000000000ULL)) {
    return fail("scalar double globals should lower into an admitted F64 BIR initializer lane");
  }

  return 0;
}

int expect_f128_scalar_constant_binop_fails_closed() {
  auto result = try_lower_to_bir_with_options(
      make_f128_scalar_constant_binop_fails_closed_module(), BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("F128 scalar constants must not lower through the current 64-bit immediate lane");
  }
  if (!contains_note(result.notes,
                     "function",
                     "semantic lir_to_bir function 'f128_scalar_constant_binop_fails_closed' "
                     "failed in scalar-binop semantic family")) {
    return fail("missing scalar-binop failure for unsupported F128 scalar constant");
  }
  if (!contains_note(result.notes,
                     "module",
                     "latest function failure: semantic lir_to_bir function "
                     "'f128_scalar_constant_binop_fails_closed' failed in scalar-binop "
                     "semantic family")) {
    return fail("missing module summary for unsupported F128 scalar constant");
  }
  return 0;
}

int expect_admitted_scalar_i16_globals() {
  auto result = try_lower_to_bir_with_options(make_admitted_scalar_i16_globals_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("expected semantic BIR lowering to admit scalar i16 globals");
  }

  const auto find_global = [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : result.module->globals) {
      if (global.name == name) {
        return &global;
      }
    }
    return nullptr;
  };

  const auto* zero_global = find_global("gzero");
  if (zero_global == nullptr || zero_global->type != TypeKind::I16 ||
      !zero_global->initializer.has_value() ||
      *zero_global->initializer != c4c::backend::bir::Value::immediate_i16(0)) {
    return fail("scalar i16 zeroinitializers should lower into an admitted I16 BIR initializer lane");
  }

  const auto* value_global = find_global("gvalue");
  if (value_global == nullptr || value_global->type != TypeKind::I16 ||
      !value_global->initializer.has_value() ||
      *value_global->initializer != c4c::backend::bir::Value::immediate_i16(17)) {
    return fail("scalar i16 nonzero globals should lower into an admitted I16 BIR initializer lane");
  }

  return 0;
}

int expect_aarch64_extern_data_global_uses_got_policy() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  LirGlobal extern_global;
  extern_global.name = "external_data_symbol";
  extern_global.link_name_id = module.link_names.intern("external_data_symbol");
  extern_global.llvm_type = "ptr";
  extern_global.is_extern_decl = true;
  extern_global.align_bytes = 8;
  module.globals.push_back(std::move(extern_global));

  LirGlobal internal_global;
  internal_global.name = "internal_data_symbol";
  internal_global.link_name_id = module.link_names.intern("internal_data_symbol");
  internal_global.is_internal = true;
  internal_global.llvm_type = "i32";
  internal_global.init_text = "i32 7";
  internal_global.align_bytes = 4;
  module.globals.push_back(std::move(internal_global));

  LirGlobal owned_global;
  owned_global.name = "owned_data_symbol";
  owned_global.link_name_id = module.link_names.intern("owned_data_symbol");
  owned_global.llvm_type = "i32";
  owned_global.init_text = "i32 11";
  owned_global.align_bytes = 4;
  module.globals.push_back(std::move(owned_global));

  LirFunction function;
  function.name = "aarch64_extern_data_global_uses_got_policy";
  function.signature_text = "define i32 @aarch64_extern_data_global_uses_got_policy()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = "0",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("expected AArch64 extern-data policy fixture to lower");
  }

  const auto find_global = [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : result.module->globals) {
      if (global.name == name) {
        return &global;
      }
    }
    return nullptr;
  };

  const auto* extern_lowered = find_global("external_data_symbol");
  if (extern_lowered == nullptr || !extern_lowered->is_extern ||
      extern_lowered->address_materialization_policy !=
          c4c::backend::bir::GlobalAddressMaterializationPolicy::GotRequired) {
    return fail("AArch64 extern data globals should require GOT address materialization");
  }

  const auto* internal_lowered = find_global("internal_data_symbol");
  if (internal_lowered == nullptr || internal_lowered->is_extern ||
      internal_lowered->address_materialization_policy !=
          c4c::backend::bir::GlobalAddressMaterializationPolicy::Direct) {
    return fail("AArch64 internal data globals should keep direct address materialization");
  }

  const auto* owned_lowered = find_global("owned_data_symbol");
  if (owned_lowered == nullptr || owned_lowered->is_extern ||
      owned_lowered->address_materialization_policy !=
          c4c::backend::bir::GlobalAddressMaterializationPolicy::Direct) {
    return fail("AArch64 compiler-owned data globals should keep direct address materialization");
  }

  return 0;
}

int expect_link_name_id_symbol_identity_survives_drifted_display_names() {
  LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::LinkNameId global_id = module.link_names.intern("semantic_global");
  const c4c::LinkNameId callee_id = module.link_names.intern("semantic_callee");
  const c4c::LinkNameId user_id = module.link_names.intern("semantic_user");

  LirGlobal global;
  global.name = "drifted_global_display";
  global.link_name_id = global_id;
  global.llvm_type = "i32";
  global.init_text = "i32 7";
  global.align_bytes = 4;
  module.globals.push_back(std::move(global));

  c4c::codegen::lir::LirExternDecl callee;
  callee.name = "drifted_callee_display";
  callee.link_name_id = callee_id;
  callee.return_type_str = "void";
  callee.return_type = c4c::codegen::lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(callee));

  LirFunction function;
  function.name = "drifted_user_display";
  function.link_name_id = user_id;
  function.return_type.base = c4c::TB_INT;

  LirBlock entry;
  entry.id = function.alloc_block();
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{
      .type_str = c4c::codegen::lir::LirTypeRef("i32"),
      .val = LirOperand("5"),
      .ptr = LirOperand("@semantic_global"),
  });
  entry.insts.push_back(LirCallOp{
      .return_type = c4c::codegen::lir::LirTypeRef("void"),
      .callee = LirOperand("@stale_callee_display"),
      .direct_callee_link_name_id = callee_id,
      .callee_signature = void_call_signature({}),
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%loaded"),
      .type_str = c4c::codegen::lir::LirTypeRef("i32"),
      .ptr = LirOperand("@semantic_global"),
  });
  entry.terminator = LirRet{std::string("%loaded"), "i32"};
  function.entry = entry.id;
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("LinkNameId symbol identity fixture should lower to BIR");
  }

  if (result.module->globals.empty() ||
      result.module->globals.front().name != "semantic_global" ||
      result.module->globals.front().link_name_id != global_id) {
    return fail("BIR global should use LinkNameId spelling as semantic identity");
  }
  if (result.module->globals.front().name == "drifted_global_display") {
    return fail("BIR global should not keep drifted legacy display spelling as identity");
  }
  const auto& lowered_function = result.module->functions.back();
  if (lowered_function.name != "semantic_user" ||
      lowered_function.link_name_id != user_id) {
    return fail("BIR function should use LinkNameId spelling as semantic identity");
  }
  if (lowered_function.name == "drifted_user_display") {
    return fail("BIR function should not keep drifted legacy display spelling as identity");
  }

  const auto* store =
      std::get_if<c4c::backend::bir::StoreGlobalInst>(&lowered_function.blocks.front().insts[0]);
  if (store == nullptr || store->global_name != "semantic_global" ||
      store->global_name_id != global_id) {
    return fail("BIR global store should carry LinkNameId identity for semantic global refs");
  }
  const auto* call =
      std::get_if<c4c::backend::bir::CallInst>(&lowered_function.blocks.front().insts[1]);
  if (call == nullptr || call->callee != "semantic_callee" ||
      call->callee_link_name_id != callee_id) {
    return fail("BIR direct call should use LinkNameId spelling as semantic callee identity");
  }
  if (call->callee == "stale_callee_display") {
    return fail("BIR direct call should not keep stale legacy callee spelling as identity");
  }
  const auto* load =
      std::get_if<c4c::backend::bir::LoadGlobalInst>(&lowered_function.blocks.front().insts[2]);
  if (load == nullptr || load->global_name != "semantic_global" ||
      load->global_name_id != global_id) {
    return fail("BIR global load should carry LinkNameId identity for semantic global refs");
  }
  return 0;
}

int expect_link_name_id_global_identity_rejects_unresolved_id() {
  LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  LirGlobal global;
  global.name = "stale_raw_global";
  global.link_name_id = 9999;
  global.llvm_type = "i32";
  global.init_text = "i32 7";
  global.align_bytes = 4;
  module.globals.push_back(std::move(global));

  const auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("unresolved global LinkNameId should not recover through raw LIR global name");
  }
  if (!contains_note(result.notes,
                     "module",
                     "LinkNameId-bearing LIR global must resolve through the link-name table")) {
    return fail("unresolved global LinkNameId failure should identify the fail-closed path");
  }
  return 0;
}

int expect_link_name_id_extern_identity_rejects_unresolved_id() {
  LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  constexpr c4c::LinkNameId kStaleExternId = 9999;

  c4c::codegen::lir::LirExternDecl callee;
  callee.name = "stale_raw_extern";
  callee.link_name_id = kStaleExternId;
  callee.return_type_str = "void";
  callee.return_type = c4c::codegen::lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(callee));

  LirFunction function;
  function.name = "extern_identity_user";
  function.signature_text = "define void @extern_identity_user()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .return_type = c4c::codegen::lir::LirTypeRef("void"),
      .callee = LirOperand("@stale_raw_extern"),
      .direct_callee_link_name_id = kStaleExternId,
      .callee_signature = void_call_signature({}),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  const auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("unresolved extern LinkNameId should not recover through raw LIR spelling");
  }
  if (!contains_note(result.notes,
                     "module",
                     "LinkNameId-bearing LIR extern declaration must resolve through the link-name table")) {
    return fail("unresolved extern LinkNameId failure should identify the fail-closed path");
  }
  return 0;
}

int expect_string_pool_direct_call_bridge_prefers_function_link_name_id() {
  LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::LinkNameId sink_id = module.link_names.intern("semantic_sink");
  const c4c::LinkNameId user_id = module.link_names.intern("semantic_user");

  module.string_pool.push_back(c4c::codegen::lir::LirStringConst{
      .pool_name = "@.str.good",
      .raw_bytes = "good\0",
      .byte_length = 5,
  });
  module.string_pool.push_back(c4c::codegen::lir::LirStringConst{
      .pool_name = "@.str.bad",
      .raw_bytes = "bad\0",
      .byte_length = 4,
  });

  c4c::codegen::lir::LirExternDecl sink;
  sink.name = "drifted_sink_display";
  sink.link_name_id = sink_id;
  sink.return_type_str = "void";
  sink.return_type = c4c::codegen::lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(sink));

  LirFunction raw_name_decoy;
  raw_name_decoy.name = "semantic_user";
  raw_name_decoy.signature_text = "define void @semantic_user()";
  LirBlock decoy_entry;
  decoy_entry.label = "entry";
  decoy_entry.insts.push_back(LirGepOp{
      .result = LirOperand("%bad_ptr"),
      .element_type = c4c::codegen::lir::LirTypeRef("[4 x i8]"),
      .ptr = LirOperand("@.str.bad"),
      .inbounds = true,
      .indices = {"i64 0", "i64 0"},
  });
  decoy_entry.insts.push_back(LirCallOp{
      .return_type = c4c::codegen::lir::LirTypeRef("void"),
      .callee = LirOperand("@semantic_sink"),
      .direct_callee_link_name_id = sink_id,
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %bad_ptr",
      .callee_signature = void_call_signature({"ptr"}),
  });
  decoy_entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };
  raw_name_decoy.blocks.push_back(std::move(decoy_entry));
  module.functions.push_back(std::move(raw_name_decoy));

  LirFunction semantic_user;
  semantic_user.name = "drifted_user_display";
  semantic_user.link_name_id = user_id;
  semantic_user.signature_text = "define void @semantic_user()";
  LirBlock user_entry;
  user_entry.label = "entry";
  user_entry.insts.push_back(LirGepOp{
      .result = LirOperand("%good_ptr"),
      .element_type = c4c::codegen::lir::LirTypeRef("[5 x i8]"),
      .ptr = LirOperand("@.str.good"),
      .inbounds = true,
      .indices = {"i64 0", "i64 0"},
  });
  user_entry.insts.push_back(LirCallOp{
      .return_type = c4c::codegen::lir::LirTypeRef("void"),
      .callee = LirOperand("@stale_sink_display"),
      .direct_callee_link_name_id = sink_id,
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %good_ptr",
      .callee_signature = void_call_signature({"ptr"}),
  });
  user_entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };
  semantic_user.blocks.push_back(std::move(user_entry));
  module.functions.push_back(std::move(semantic_user));

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("string-pool direct-call LinkNameId bridge fixture should lower to BIR");
  }

  const c4c::backend::bir::Function* lowered_user = nullptr;
  for (const auto& function : result.module->functions) {
    if (function.link_name_id == user_id) {
      lowered_user = &function;
      break;
    }
  }
  if (lowered_user == nullptr || lowered_user->name != "semantic_user") {
    return fail("semantic user function should lower with LinkNameId identity");
  }
  if (result.module->string_constants.size() != 2) {
    return fail("string-pool lowering should materialize BIR string constants");
  }
  for (const auto& string_constant : result.module->string_constants) {
    if (string_constant.name_id == c4c::kInvalidText) {
      return fail("BIR string constants should carry TextId identity");
    }
    if (result.module->names.texts.lookup(string_constant.name_id) != string_constant.name) {
      return fail("BIR string constant TextId should resolve to the display spelling");
    }
  }

  bool saw_good_string_arg = false;
  bool saw_bad_string_arg = false;
  for (const auto& block : lowered_user->blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr || call->args.empty()) {
        continue;
      }
      saw_good_string_arg =
          saw_good_string_arg ||
          call->args.front() == c4c::backend::bir::Value::named(TypeKind::Ptr, "@.str.good");
      saw_bad_string_arg =
          saw_bad_string_arg ||
          call->args.front() == c4c::backend::bir::Value::named(TypeKind::Ptr, "@.str.bad");
    }
  }
  if (!saw_good_string_arg) {
    return fail("string-pool direct-call bridge should choose the LIR function by LinkNameId");
  }
  if (saw_bad_string_arg) {
    return fail("string-pool direct-call bridge should not use raw-name decoy aliases");
  }
  return 0;
}

int expect_string_pool_direct_call_bridge_requires_text_id() {
  LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::LinkNameId sink_id = module.link_names.intern("text_id_required_sink");
  const c4c::LinkNameId user_id = module.link_names.intern("text_id_required_user");

  LirGlobal raw_only_global;
  raw_only_global.name = ".str.raw_only";
  raw_only_global.qualifier = "constant ";
  raw_only_global.llvm_type = "[5 x i8]";
  raw_only_global.init_text = "c\"raw\\00\"";
  raw_only_global.align_bytes = 1;
  module.globals.push_back(std::move(raw_only_global));

  c4c::codegen::lir::LirExternDecl sink;
  sink.name = "text_id_required_sink";
  sink.link_name_id = sink_id;
  sink.return_type_str = "void";
  sink.return_type = c4c::codegen::lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(sink));

  LirFunction function;
  function.name = "text_id_required_user";
  function.link_name_id = user_id;
  function.signature_text = "define void @text_id_required_user()";
  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%stale_ptr"),
      .element_type = c4c::codegen::lir::LirTypeRef("[5 x i8]"),
      .ptr = LirOperand("@.str.raw_only"),
      .inbounds = true,
      .indices = {"i64 0", "i64 0"},
  });
  entry.insts.push_back(LirCallOp{
      .return_type = c4c::codegen::lir::LirTypeRef("void"),
      .callee = LirOperand("@text_id_required_sink"),
      .direct_callee_link_name_id = sink_id,
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %stale_ptr",
      .callee_signature = void_call_signature({"ptr"}),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("raw-only string pointer fixture should still lower to BIR");
  }
  if (!result.module->string_constants.empty()) {
    return fail("raw-only string pointer fixture should not synthesize a BIR TextId carrier");
  }

  for (const auto& function : result.module->functions) {
    if (function.link_name_id != user_id) {
      continue;
    }
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
        if (call == nullptr || call->args.empty()) {
          continue;
        }
        if (call->args.front() == c4c::backend::bir::Value::named(TypeKind::Ptr,
                                                                  "@.str.raw_only")) {
          return fail("string-pool rewrite must not recover through raw spelling without TextId");
        }
      }
    }
    return 0;
  }
  return fail("raw-only string pointer fixture lost the lowered function");
}

int expect_dynamic_global_scalar_array_loads_carry_link_name_id() {
  LirModule module = make_dynamic_indexed_gep_global_member_array_module();
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::LinkNameId cases_id = module.link_names.intern("cases");
  const c4c::LinkNameId function_id =
      module.link_names.intern("dynamic_indexed_gep_global_member_array");
  module.globals.front().name = "drifted_cases_display";
  module.globals.front().link_name_id = cases_id;
  module.functions.front().name = "drifted_dynamic_array_display";
  module.functions.front().link_name_id = function_id;

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("dynamic global scalar-array fixture should lower to BIR");
  }
  std::size_t dynamic_load_count = 0;
  const auto& lowered_function = result.module->functions.back();
  for (const auto& block : lowered_function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst);
      if (load == nullptr) {
        continue;
      }
      if (load->global_name != "cases") {
        return fail("dynamic scalar-array load should keep semantic global reference text");
      }
      if (load->global_name_id != cases_id) {
        return fail("dynamic scalar-array load should carry LinkNameId despite drifted display spelling");
      }
      ++dynamic_load_count;
    }
  }
  if (dynamic_load_count == 0) {
    return fail("dynamic scalar-array fixture should materialize global loads");
  }
  return 0;
}

int expect_dynamic_global_scalar_array_loads_reject_missing_link_name_spelling() {
  LirModule module = make_dynamic_indexed_gep_global_member_array_module();
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  module.globals.front().link_name_id = module.link_names.intern("cases");
  module.link_names.attach_text_table(nullptr);

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("dynamic scalar-array load should reject missing LinkNameId spelling");
  }
  return 0;
}

int expect_dynamic_global_scalar_array_loads_keep_no_id_compatibility() {
  LirModule module = make_dynamic_indexed_gep_global_member_array_module();

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("raw dynamic global scalar-array fixture should lower to BIR");
  }
  std::size_t dynamic_load_count = 0;
  const auto& lowered_function = result.module->functions.back();
  for (const auto& block : lowered_function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst);
      if (load == nullptr) {
        continue;
      }
      if (load->global_name != "cases") {
        return fail("raw dynamic scalar-array load should keep raw global reference text");
      }
      if (load->global_name_id != c4c::kInvalidLinkName) {
        return fail("raw dynamic scalar-array load should not invent LinkNameId metadata");
      }
      ++dynamic_load_count;
    }
  }
  if (dynamic_load_count == 0) {
    return fail("raw dynamic scalar-array fixture should materialize global loads");
  }
  return 0;
}

int expect_pointer_initializer_symbol_names_carry_link_name_id() {
  LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::LinkNameId target_id = module.link_names.intern("semantic_target");
  const c4c::LinkNameId ptr_to_global_id = module.link_names.intern("semantic_ptr_to_global");
  const c4c::LinkNameId callee_id = module.link_names.intern("semantic_callee");
  const c4c::LinkNameId ptr_to_function_id = module.link_names.intern("semantic_ptr_to_function");
  const c4c::LinkNameId raw_shadow_id = module.link_names.intern("rendered_shadow");
  const c4c::LinkNameId ptr_to_raw_function_id =
      module.link_names.intern("semantic_ptr_to_raw_function");

  LirGlobal target;
  target.name = "drifted_target_display";
  target.link_name_id = target_id;
  target.llvm_type = "i32";
  target.init_text = "i32 11";
  target.align_bytes = 4;
  module.globals.push_back(std::move(target));

  LirGlobal ptr_to_global;
  ptr_to_global.name = "drifted_ptr_to_global_display";
  ptr_to_global.link_name_id = ptr_to_global_id;
  ptr_to_global.llvm_type = "ptr";
  ptr_to_global.init_text = "ptr @semantic_target";
  ptr_to_global.align_bytes = 8;
  module.globals.push_back(std::move(ptr_to_global));

  c4c::codegen::lir::LirExternDecl callee;
  callee.name = "drifted_callee_display";
  callee.link_name_id = callee_id;
  callee.return_type_str = "void";
  callee.return_type = c4c::codegen::lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(callee));

  c4c::codegen::lir::LirExternDecl raw_shadow;
  raw_shadow.name = "raw_shadow_display";
  raw_shadow.link_name_id = raw_shadow_id;
  raw_shadow.return_type_str = "void";
  raw_shadow.return_type = c4c::codegen::lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(raw_shadow));

  LirGlobal ptr_to_function;
  ptr_to_function.name = "drifted_ptr_to_function_display";
  ptr_to_function.link_name_id = ptr_to_function_id;
  ptr_to_function.llvm_type = "ptr";
  ptr_to_function.init_text = "ptr @drifted_callee_display";
  ptr_to_function.initializer_function_link_name_ids.push_back(callee_id);
  ptr_to_function.align_bytes = 8;
  module.globals.push_back(std::move(ptr_to_function));

  LirGlobal ptr_to_raw_function;
  ptr_to_raw_function.name = "drifted_ptr_to_raw_function_display";
  ptr_to_raw_function.link_name_id = ptr_to_raw_function_id;
  ptr_to_raw_function.llvm_type = "ptr";
  ptr_to_raw_function.init_text = "ptr @rendered_shadow";
  ptr_to_raw_function.align_bytes = 8;
  module.globals.push_back(std::move(ptr_to_raw_function));

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("pointer initializer LinkNameId fixture should lower to BIR");
  }

  const auto find_global = [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : result.module->globals) {
      if (global.name == name) {
        return &global;
      }
    }
    return nullptr;
  };

  const auto* lowered_ptr_to_global = find_global("semantic_ptr_to_global");
  if (lowered_ptr_to_global == nullptr ||
      lowered_ptr_to_global->initializer_symbol_name != "semantic_target" ||
      lowered_ptr_to_global->initializer_symbol_name_id != target_id) {
    return fail("pointer initializer to a known global should carry the target LinkNameId");
  }

  const auto* lowered_ptr_to_function = find_global("semantic_ptr_to_function");
  if (lowered_ptr_to_function == nullptr ||
      lowered_ptr_to_function->initializer_symbol_name != "drifted_callee_display" ||
      lowered_ptr_to_function->initializer_symbol_name_id != callee_id) {
    return fail("pointer initializer with LinkNameId metadata should keep the id authoritative over raw display spelling");
  }

  const auto* lowered_ptr_to_raw_function = find_global("semantic_ptr_to_raw_function");
  if (lowered_ptr_to_raw_function == nullptr ||
      lowered_ptr_to_raw_function->initializer_symbol_name != "rendered_shadow" ||
      lowered_ptr_to_raw_function->initializer_symbol_name_id != raw_shadow_id) {
    return fail("raw-import/no-id function initializer compatibility should remain available without LinkNameId metadata");
  }

  LirModule conflict_module;
  conflict_module.link_name_texts = std::make_shared<c4c::TextTable>();
  conflict_module.link_names.attach_text_table(conflict_module.link_name_texts.get());
  conflict_module.struct_names.attach_text_table(conflict_module.link_name_texts.get());

  const c4c::LinkNameId conflict_raw_shadow_id =
      conflict_module.link_names.intern("rendered_shadow");
  const c4c::LinkNameId missing_callee_id =
      conflict_module.link_names.intern("semantic_missing_callee");
  const c4c::LinkNameId ptr_to_missing_function_id =
      conflict_module.link_names.intern("semantic_ptr_to_missing_function");

  c4c::codegen::lir::LirExternDecl conflict_raw_shadow;
  conflict_raw_shadow.name = "raw_shadow_display";
  conflict_raw_shadow.link_name_id = conflict_raw_shadow_id;
  conflict_raw_shadow.return_type_str = "void";
  conflict_raw_shadow.return_type = c4c::codegen::lir::LirTypeRef("void");
  conflict_module.extern_decls.push_back(std::move(conflict_raw_shadow));

  LirGlobal ptr_to_missing_function;
  ptr_to_missing_function.name = "drifted_ptr_to_missing_function_display";
  ptr_to_missing_function.link_name_id = ptr_to_missing_function_id;
  ptr_to_missing_function.llvm_type = "ptr";
  ptr_to_missing_function.init_text = "ptr @rendered_shadow";
  ptr_to_missing_function.initializer_function_link_name_ids.push_back(missing_callee_id);
  ptr_to_missing_function.align_bytes = 8;
  conflict_module.globals.push_back(std::move(ptr_to_missing_function));

  auto conflict_result = try_lower_to_bir_with_options(conflict_module, BirLoweringOptions{});
  if (conflict_result.module.has_value()) {
    return fail("present initializer function LinkNameId miss should fail closed instead of falling back to raw symbol lookup");
  }

  LirModule aggregate_module;
  aggregate_module.link_name_texts = std::make_shared<c4c::TextTable>();
  aggregate_module.link_names.attach_text_table(aggregate_module.link_name_texts.get());
  aggregate_module.struct_names.attach_text_table(aggregate_module.link_name_texts.get());
  aggregate_module.type_decls.push_back("%struct.fnslot = type { ptr }");
  const c4c::LinkNameId aggregate_callee_id =
      aggregate_module.link_names.intern("semantic_aggregate_callee");
  const c4c::LinkNameId aggregate_slot_id =
      aggregate_module.link_names.intern("semantic_aggregate_slot");

  c4c::codegen::lir::LirExternDecl aggregate_callee;
  aggregate_callee.name = "drifted_aggregate_callee_display";
  aggregate_callee.link_name_id = aggregate_callee_id;
  aggregate_callee.return_type_str = "void";
  aggregate_callee.return_type = c4c::codegen::lir::LirTypeRef("void");
  aggregate_module.extern_decls.push_back(std::move(aggregate_callee));

  LirGlobal aggregate_slot;
  aggregate_slot.name = "drifted_aggregate_slot_display";
  aggregate_slot.link_name_id = aggregate_slot_id;
  aggregate_slot.llvm_type = "%struct.fnslot";
  aggregate_slot.init_text = "{ ptr @drifted_aggregate_callee_display }";
  aggregate_slot.initializer_function_link_name_ids.push_back(aggregate_callee_id);
  aggregate_slot.align_bytes = 8;
  aggregate_module.globals.push_back(std::move(aggregate_slot));

  auto aggregate_result = try_lower_to_bir_with_options(aggregate_module, BirLoweringOptions{});
  if (!aggregate_result.module.has_value()) {
    return fail("aggregate pointer initializer should use LinkNameId authority for function fields");
  }
  const c4c::backend::bir::Global* lowered_aggregate_slot = nullptr;
  for (const auto& global : aggregate_result.module->globals) {
    if (global.link_name_id == aggregate_slot_id) {
      lowered_aggregate_slot = &global;
      break;
    }
  }
  if (lowered_aggregate_slot == nullptr ||
      lowered_aggregate_slot->initializer_elements.size() != 1 ||
      lowered_aggregate_slot->initializer_elements.front().pointer_symbol_link_name_id !=
          aggregate_callee_id) {
    return fail("aggregate pointer initializer element values should carry function LinkNameId");
  }
  if (lowered_aggregate_slot->initializer_elements.front() ==
      c4c::backend::bir::Value::named(TypeKind::Ptr, "@drifted_aggregate_callee_display")) {
    return fail("aggregate function pointer initializer value must not be raw display spelling only");
  }

  LirModule aggregate_global_module;
  aggregate_global_module.link_name_texts = std::make_shared<c4c::TextTable>();
  aggregate_global_module.link_names.attach_text_table(
      aggregate_global_module.link_name_texts.get());
  aggregate_global_module.struct_names.attach_text_table(
      aggregate_global_module.link_name_texts.get());
  aggregate_global_module.type_decls.push_back("%struct.gslot = type { ptr }");
  const c4c::LinkNameId aggregate_target_id =
      aggregate_global_module.link_names.intern("semantic_aggregate_target");
  const c4c::LinkNameId aggregate_global_slot_id =
      aggregate_global_module.link_names.intern("semantic_aggregate_global_slot");

  LirGlobal aggregate_target;
  aggregate_target.name = "drifted_aggregate_target_display";
  aggregate_target.link_name_id = aggregate_target_id;
  aggregate_target.llvm_type = "i32";
  aggregate_target.init_text = "i32 19";
  aggregate_target.align_bytes = 4;
  aggregate_global_module.globals.push_back(std::move(aggregate_target));

  LirGlobal aggregate_global_slot;
  aggregate_global_slot.name = "drifted_aggregate_global_slot_display";
  aggregate_global_slot.link_name_id = aggregate_global_slot_id;
  aggregate_global_slot.llvm_type = "%struct.gslot";
  aggregate_global_slot.init_text = "{ ptr @semantic_aggregate_target }";
  aggregate_global_slot.align_bytes = 8;
  aggregate_global_module.globals.push_back(std::move(aggregate_global_slot));

  auto aggregate_global_result =
      try_lower_to_bir_with_options(aggregate_global_module, BirLoweringOptions{});
  if (!aggregate_global_result.module.has_value()) {
    return fail("aggregate pointer initializer should use LinkNameId authority for global fields");
  }
  const c4c::backend::bir::Global* lowered_aggregate_global_slot = nullptr;
  for (const auto& global : aggregate_global_result.module->globals) {
    if (global.link_name_id == aggregate_global_slot_id) {
      lowered_aggregate_global_slot = &global;
      break;
    }
  }
  if (lowered_aggregate_global_slot == nullptr ||
      lowered_aggregate_global_slot->initializer_elements.size() != 1 ||
      lowered_aggregate_global_slot->initializer_elements.front().pointer_symbol_link_name_id !=
          aggregate_target_id) {
    return fail("aggregate pointer initializer element values should carry global LinkNameId");
  }
  if (lowered_aggregate_global_slot->initializer_elements.front() ==
      c4c::backend::bir::Value::named(TypeKind::Ptr, "@semantic_aggregate_target")) {
    return fail("aggregate global pointer initializer value must not be raw display spelling only");
  }

  LirModule aggregate_conflict_module;
  aggregate_conflict_module.link_name_texts = std::make_shared<c4c::TextTable>();
  aggregate_conflict_module.link_names.attach_text_table(
      aggregate_conflict_module.link_name_texts.get());
  aggregate_conflict_module.struct_names.attach_text_table(
      aggregate_conflict_module.link_name_texts.get());
  aggregate_conflict_module.type_decls.push_back("%struct.fnslot = type { ptr }");
  const c4c::LinkNameId aggregate_raw_shadow_id =
      aggregate_conflict_module.link_names.intern("rendered_shadow");
  const c4c::LinkNameId aggregate_missing_callee_id =
      aggregate_conflict_module.link_names.intern("semantic_missing_aggregate_callee");
  const c4c::LinkNameId aggregate_conflict_slot_id =
      aggregate_conflict_module.link_names.intern("semantic_aggregate_conflict_slot");

  c4c::codegen::lir::LirExternDecl aggregate_raw_shadow;
  aggregate_raw_shadow.name = "raw_shadow_display";
  aggregate_raw_shadow.link_name_id = aggregate_raw_shadow_id;
  aggregate_raw_shadow.return_type_str = "void";
  aggregate_raw_shadow.return_type = c4c::codegen::lir::LirTypeRef("void");
  aggregate_conflict_module.extern_decls.push_back(std::move(aggregate_raw_shadow));

  LirGlobal aggregate_conflict_slot;
  aggregate_conflict_slot.name = "drifted_aggregate_conflict_slot_display";
  aggregate_conflict_slot.link_name_id = aggregate_conflict_slot_id;
  aggregate_conflict_slot.llvm_type = "%struct.fnslot";
  aggregate_conflict_slot.init_text = "{ ptr @rendered_shadow }";
  aggregate_conflict_slot.initializer_function_link_name_ids.push_back(
      aggregate_missing_callee_id);
  aggregate_conflict_slot.align_bytes = 8;
  aggregate_conflict_module.globals.push_back(std::move(aggregate_conflict_slot));

  auto aggregate_conflict_result =
      try_lower_to_bir_with_options(aggregate_conflict_module, BirLoweringOptions{});
  if (aggregate_conflict_result.module.has_value()) {
    return fail("aggregate pointer initializer LinkNameId miss should fail closed instead of falling back to raw spelling");
  }

  c4c::backend::bir::Module compatibility_module;
  compatibility_module.globals.push_back(c4c::backend::bir::Global{
      .name = "compat_ptr",
      .type = c4c::backend::bir::TypeKind::Ptr,
      .is_extern = false,
      .size_bytes = 8,
      .initializer_symbol_name = "compat_unknown",
  });
  std::string error;
  if (!c4c::backend::bir::validate(compatibility_module, &error) ||
      compatibility_module.globals.front().initializer_symbol_name_id != c4c::kInvalidLinkName) {
    return fail("unknown raw-import/no-id compatibility initializer symbols should remain valid only without LinkNameId");
  }

  return 0;
}

int expect_pointer_value_symbol_identity_carrier() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.payload = type { i32 }");

  const c4c::LinkNameId sink_id = module.link_names.intern("semantic_pointer_sink");
  const c4c::LinkNameId fn_arg_id = module.link_names.intern("semantic_function_arg");
  const c4c::LinkNameId byval_global_id = module.link_names.intern("semantic_byval_payload");
  const c4c::LinkNameId user_id = module.link_names.intern("semantic_pointer_user");
  const c4c::LinkNameId returner_id =
      module.link_names.intern("semantic_pointer_returner");

  c4c::codegen::lir::LirExternDecl sink;
  sink.name = "drifted_sink_display";
  sink.link_name_id = sink_id;
  sink.return_type_str = "void";
  sink.return_type = c4c::codegen::lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(sink));

  c4c::codegen::lir::LirExternDecl fn_arg;
  fn_arg.name = "drifted_function_arg_display";
  fn_arg.link_name_id = fn_arg_id;
  fn_arg.return_type_str = "void";
  fn_arg.return_type = c4c::codegen::lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(fn_arg));

  LirGlobal byval_global;
  byval_global.name = "drifted_byval_payload_display";
  byval_global.link_name_id = byval_global_id;
  byval_global.llvm_type = "%struct.payload";
  byval_global.init_text = "{ i32 42 }";
  byval_global.align_bytes = 4;
  module.globals.push_back(std::move(byval_global));

  LirFunction function;
  function.name = "drifted_pointer_user_display";
  function.link_name_id = user_id;
  function.signature_text = "define void @drifted_pointer_user_display()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%ptr.slot"),
      .type_str = "ptr",
      .count = LirOperand(""),
      .align = 8,
  });
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%global.ptr.slot"),
      .type_str = "ptr",
      .count = LirOperand(""),
      .align = 8,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@semantic_pointer_sink"),
      .direct_callee_link_name_id = sink_id,
      .callee_type_suffix = "(ptr, ptr)",
      .args_str =
          "ptr @semantic_function_arg, ptr byval(%struct.payload) @semantic_byval_payload",
      .callee_signature = void_call_signature({"ptr", "ptr"}),
  });
  entry.insts.push_back(LirStoreOp{
      .type_str = "ptr",
      .val = LirOperand("@semantic_function_arg"),
      .ptr = LirOperand("%ptr.slot"),
  });
  entry.insts.push_back(LirStoreOp{
      .type_str = "ptr",
      .val = LirOperand("@semantic_byval_payload"),
      .ptr = LirOperand("%global.ptr.slot"),
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%stored.fn"),
      .type_str = "ptr",
      .ptr = LirOperand("%ptr.slot"),
  });
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@semantic_pointer_sink"),
      .direct_callee_link_name_id = sink_id,
      .callee_type_suffix = "(ptr, ptr)",
      .args_str =
          "ptr %stored.fn, ptr byval(%struct.payload) @semantic_byval_payload",
      .callee_signature = void_call_signature({"ptr", "ptr"}),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  LirFunction returner;
  returner.name = "drifted_pointer_returner_display";
  returner.link_name_id = returner_id;
  returner.signature_text = "define ptr @drifted_pointer_returner_display()";
  LirBlock returner_entry;
  returner_entry.label = "entry";
  returner_entry.terminator = LirRet{
      .value_str = "@semantic_function_arg",
      .type_str = "ptr",
  };
  returner.blocks.push_back(std::move(returner_entry));
  module.functions.push_back(std::move(returner));

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("pointer-value LinkNameId carrier fixture should lower to BIR");
  }

  const c4c::backend::bir::Function* lowered_user = nullptr;
  const c4c::backend::bir::Function* lowered_returner = nullptr;
  for (const auto& lowered_function : result.module->functions) {
    if (lowered_function.link_name_id == user_id) {
      lowered_user = &lowered_function;
    }
    if (lowered_function.link_name_id == returner_id) {
      lowered_returner = &lowered_function;
    }
  }
  if (lowered_user == nullptr || lowered_user->blocks.empty() ||
      lowered_user->blocks.front().insts.empty()) {
    return fail("pointer-value carrier fixture should lower the user call");
  }
  if (lowered_returner == nullptr || lowered_returner->blocks.empty() ||
      !lowered_returner->blocks.front().terminator.value.has_value() ||
      lowered_returner->blocks.front().terminator.value->name != "@semantic_function_arg" ||
      lowered_returner->blocks.front().terminator.value->pointer_symbol_link_name_id !=
          fn_arg_id) {
    return fail("direct function pointer returns should preserve LinkNameId identity");
  }
  if (*lowered_returner->blocks.front().terminator.value ==
      c4c::backend::bir::Value::named(TypeKind::Ptr, "@semantic_function_arg")) {
    return fail("direct function pointer return values must not be raw display spelling only");
  }

  std::vector<const c4c::backend::bir::CallInst*> calls;
  const c4c::backend::bir::StoreLocalInst* function_pointer_store = nullptr;
  const c4c::backend::bir::StoreLocalInst* global_pointer_store = nullptr;
  for (const auto& inst : lowered_user->blocks.front().insts) {
    if (const auto* lowered_call = std::get_if<c4c::backend::bir::CallInst>(&inst);
        lowered_call != nullptr) {
      calls.push_back(lowered_call);
    }
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
        store != nullptr && store->slot_name == "%ptr.slot") {
      function_pointer_store = store;
    }
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
        store != nullptr && store->slot_name == "%global.ptr.slot") {
      global_pointer_store = store;
    }
  }
  if (calls.size() != 2 || calls[0]->args.size() != 2 || calls[1]->args.size() != 2) {
    return fail("pointer-value carrier fixture should lower both pointer arguments");
  }
  if (function_pointer_store == nullptr ||
      function_pointer_store->value.name != "@semantic_function_arg" ||
      function_pointer_store->value.pointer_symbol_link_name_id != fn_arg_id) {
    return fail("direct function pointer stores should preserve LinkNameId on the stored value");
  }
  if (function_pointer_store->value ==
      c4c::backend::bir::Value::named(TypeKind::Ptr, "@semantic_function_arg")) {
    return fail("direct function pointer store values must not be raw display spelling only");
  }
  if (global_pointer_store == nullptr ||
      global_pointer_store->value.name != "@semantic_byval_payload" ||
      global_pointer_store->value.pointer_symbol_link_name_id != byval_global_id) {
    return fail("direct global pointer stores should preserve LinkNameId on the stored value");
  }
  if (global_pointer_store->value ==
      c4c::backend::bir::Value::named(TypeKind::Ptr, "@semantic_byval_payload")) {
    return fail("direct global pointer store values must not be raw display spelling only");
  }

  const auto& direct_call = *calls[0];
  const auto& recovered_call = *calls[1];
  if (direct_call.args[0].name != "@semantic_function_arg" ||
      direct_call.args[0].pointer_symbol_link_name_id != fn_arg_id) {
    return fail("direct function pointer values should carry LinkNameId identity");
  }
  if (direct_call.args[0].pointer_symbol_link_name_id == c4c::kInvalidLinkName) {
    return fail("direct function pointer value identity must not be raw display spelling only");
  }
  if (recovered_call.args[0].name != "@semantic_function_arg" ||
      recovered_call.args[0].pointer_symbol_link_name_id != fn_arg_id) {
    return fail("stored then loaded function pointer values should recover LinkNameId identity");
  }

  if (direct_call.args[1].name != "@semantic_byval_payload" ||
      direct_call.args[1].pointer_symbol_link_name_id != byval_global_id) {
    return fail("byval global pointer values should carry the global LinkNameId");
  }
  if (direct_call.args[1].pointer_symbol_link_name_id == c4c::kInvalidLinkName) {
    return fail("byval global pointer value identity must not be raw display spelling only");
  }

  if (direct_call.args[0] ==
          c4c::backend::bir::Value::named(TypeKind::Ptr, "@semantic_function_arg") ||
      direct_call.args[1] ==
          c4c::backend::bir::Value::named(TypeKind::Ptr, "@semantic_byval_payload")) {
    return fail("symbol pointer values should not compare equal to raw display-only values");
  }

  LirModule missing_spelling_module = module;
  missing_spelling_module.link_names.attach_text_table(nullptr);
  auto missing_spelling_result =
      try_lower_to_bir_with_options(missing_spelling_module, BirLoweringOptions{});
  if (missing_spelling_result.module.has_value()) {
    return fail(
        "metadata-rich pointer store operands must not recover through raw symbol spelling when "
        "LinkNameId spelling is missing");
  }
  if (!contains_note(missing_spelling_result.notes,
                     "module",
                     "LinkNameId-bearing LIR extern declaration must resolve through the "
                     "link-name table before raw-name compatibility is allowed")) {
    return fail("missing pointer-value LinkNameId spelling should fail at the module boundary");
  }

  return 0;
}

int expect_addressed_global_pointer_provenance_uses_link_name_id_keys() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.PointerBox = type { ptr }");

  const c4c::LinkNameId target_id = module.link_names.intern("semantic_target");
  const c4c::LinkNameId replacement_id = module.link_names.intern("semantic_replacement");
  const c4c::LinkNameId box_id = module.link_names.intern("semantic_pointer_box");
  const c4c::LinkNameId user_id = module.link_names.intern("semantic_addressed_user");

  LirGlobal target;
  target.name = "drifted_target_display";
  target.link_name_id = target_id;
  target.llvm_type = "i32";
  target.init_text = "i32 11";
  target.align_bytes = 4;
  module.globals.push_back(std::move(target));

  LirGlobal replacement;
  replacement.name = "drifted_replacement_display";
  replacement.link_name_id = replacement_id;
  replacement.llvm_type = "i32";
  replacement.init_text = "i32 19";
  replacement.align_bytes = 4;
  module.globals.push_back(std::move(replacement));

  LirGlobal box;
  box.name = "drifted_pointer_box_display";
  box.link_name_id = box_id;
  box.llvm_type = "%struct.PointerBox";
  box.init_text = "{ ptr null }";
  box.align_bytes = 8;
  module.globals.push_back(std::move(box));

  LirFunction function;
  function.name = "drifted_addressed_user_display";
  function.link_name_id = user_id;
  function.signature_text = "define i32 @semantic_addressed_user()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%slot"),
      .element_type = "%struct.PointerBox",
      .ptr = LirOperand("@semantic_pointer_box"),
      .indices = {LirOperand("i32 0"), LirOperand("i32 0")},
  });
  entry.insts.push_back(LirStoreOp{
      .type_str = "ptr",
      .val = LirOperand("@semantic_target"),
      .ptr = LirOperand("%slot"),
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%loaded.ptr"),
      .type_str = "ptr",
      .ptr = LirOperand("%slot"),
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%loaded.value"),
      .type_str = "i32",
      .ptr = LirOperand("%loaded.ptr"),
  });
  entry.terminator = LirRet{
      .value_str = "%loaded.value",
      .type_str = "i32",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("addressed global pointer provenance fixture should lower to BIR");
  }

  const c4c::backend::bir::Function* lowered_user = nullptr;
  for (const auto& lowered_function : result.module->functions) {
    if (lowered_function.link_name_id == user_id) {
      lowered_user = &lowered_function;
      break;
    }
  }
  if (lowered_user == nullptr || lowered_user->blocks.empty()) {
    return fail("addressed global pointer provenance fixture should lower the user function");
  }

  const c4c::backend::bir::StoreGlobalInst* pointer_store = nullptr;
  const c4c::backend::bir::LoadGlobalInst* pointer_load = nullptr;
  const c4c::backend::bir::LoadGlobalInst* recovered_load = nullptr;
  for (const auto& inst : lowered_user->blocks.front().insts) {
    if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst);
        store != nullptr && store->value.type == TypeKind::Ptr) {
      pointer_store = store;
    }
    if (const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst);
        load != nullptr && load->result.type == TypeKind::Ptr) {
      pointer_load = load;
    }
    if (const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst);
        load != nullptr && load->result.name == "%loaded.value") {
      recovered_load = load;
    }
  }

  if (pointer_store == nullptr || pointer_store->global_name != "semantic_pointer_box" ||
      pointer_store->global_name_id != box_id || pointer_store->byte_offset != 0 ||
      pointer_store->value.pointer_symbol_link_name_id != target_id) {
    return fail("addressed global pointer store should carry structured box and target identities");
  }
  if (pointer_load == nullptr || pointer_load->global_name != "semantic_pointer_box" ||
      pointer_load->global_name_id != box_id || pointer_load->byte_offset != 0) {
    return fail("addressed global pointer load should carry structured box identity");
  }
  if (recovered_load == nullptr || recovered_load->global_name != "semantic_target" ||
      recovered_load->global_name_id != target_id) {
    return fail("addressed global pointer load should recover the stored target LinkNameId");
  }
  if (recovered_load->global_name_id == replacement_id) {
    return fail("addressed global pointer load must not recover a stale same-spelling target id");
  }

  namespace backend = c4c::backend;
  namespace detail = c4c::backend::lir_to_bir_detail;

  const backend::GlobalPointerSlotKey structured_key{
      .link_name_id = box_id,
      .global_name = "same_rendered_box",
      .byte_offset = 0,
  };
  const backend::GlobalPointerSlotKey stale_same_spelling_key{
      .link_name_id = replacement_id,
      .global_name = "same_rendered_box",
      .byte_offset = 0,
  };
  const backend::GlobalPointerSlotKey raw_same_spelling_key{
      .link_name_id = c4c::kInvalidLinkName,
      .global_name = "same_rendered_box",
      .byte_offset = 0,
  };

  backend::AddressedGlobalPointerSlots addressed_slots;
  addressed_slots.emplace(structured_key,
                          detail::GlobalAddress{
                              .global_name = "semantic_target",
                              .link_name_id = target_id,
                              .value_type = TypeKind::I32,
                              .byte_offset = 0,
                          });
  addressed_slots.emplace(raw_same_spelling_key, std::nullopt);
  if (addressed_slots.find(structured_key) == addressed_slots.end()) {
    return fail("structured addressed global pointer key should be retrievable by LinkNameId");
  }
  if (addressed_slots.find(stale_same_spelling_key) != addressed_slots.end()) {
    return fail("structured addressed global pointer key must reject stale same-spelling LinkNameId");
  }
  if (addressed_slots.find(raw_same_spelling_key) == addressed_slots.end()) {
    return fail("raw/no-id addressed global pointer key should remain distinct via kInvalidLinkName");
  }

  backend::AddressedGlobalPointerValueSlots addressed_value_slots;
  addressed_value_slots.emplace(
      structured_key,
      backend::PointerAddress{
          .base_value = c4c::backend::bir::Value::named_symbol_pointer(
              "@semantic_target", target_id),
          .value_type = TypeKind::Ptr,
      });
  addressed_value_slots.emplace(raw_same_spelling_key, std::nullopt);
  if (addressed_value_slots.find(stale_same_spelling_key) != addressed_value_slots.end()) {
    return fail("structured addressed global pointer-value key must reject stale same-spelling LinkNameId");
  }
  if (addressed_value_slots.find(raw_same_spelling_key) == addressed_value_slots.end()) {
    return fail("raw/no-id addressed global pointer-value key should remain distinguishable");
  }

  return 0;
}

c4c::backend::bir::Module make_link_name_mismatch_verifier_module() {
  namespace bir = c4c::backend::bir;

  bir::Module module;
  const c4c::LinkNameId actual_global_id = module.names.link_names.intern("actual_global");
  const c4c::LinkNameId other_global_id = module.names.link_names.intern("other_global");
  const c4c::LinkNameId actual_callee_id = module.names.link_names.intern("actual_callee");
  const c4c::LinkNameId other_callee_id = module.names.link_names.intern("other_callee");

  module.globals.push_back(bir::Global{
      .name = "actual_global",
      .link_name_id = actual_global_id,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .size_bytes = 4,
  });
  module.globals.push_back(bir::Global{
      .name = "other_global",
      .link_name_id = other_global_id,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .size_bytes = 4,
  });

  module.functions.push_back(bir::Function{
      .name = "actual_callee",
      .link_name_id = actual_callee_id,
      .return_type = bir::TypeKind::Void,
      .is_declaration = true,
  });
  module.functions.push_back(bir::Function{
      .name = "other_callee",
      .link_name_id = other_callee_id,
      .return_type = bir::TypeKind::Void,
      .is_declaration = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{};

  module.functions.push_back(bir::Function{
      .name = "user",
      .return_type = bir::TypeKind::Void,
      .blocks = {std::move(entry)},
  });
  return module;
}

bool validate_rejects_with_message(const c4c::backend::bir::Module& module,
                                   std::string_view needle) {
  std::string error;
  if (c4c::backend::bir::validate(module, &error)) {
    return false;
  }
  return error.find(needle) != std::string::npos;
}

bool validate_accepts(const c4c::backend::bir::Module& module) {
  std::string error;
  return c4c::backend::bir::validate(module, &error);
}

int expect_bir_verifier_rejects_known_link_name_mismatches() {
  namespace bir = c4c::backend::bir;

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee = "actual_callee",
        .args = {bir::Value::named(bir::TypeKind::Ptr, "@compat_unknown")},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should preserve compatibility pointer values without LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee = "actual_callee",
        .args = {bir::Value::named_symbol_pointer("@actual_global", 9999)},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    if (!validate_rejects_with_message(
            module, "bir call arg in @user must reference a known LinkNameId")) {
      return fail("BIR verifier should reject pointer values with unknown LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee = "actual_callee",
        .args = {bir::Value::named_symbol_pointer("", actual_id)},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    if (!validate_rejects_with_message(module, "bir call arg in @user must not use an empty name")) {
      return fail("BIR verifier should reject pointer values with empty symbol names");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee = "actual_callee",
        .args = {bir::Value::named_symbol_pointer("@stale_global_display", actual_id)},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should allow drifted pointer displays when LinkNameId is declared");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee = "actual_callee",
        .args = {bir::Value::named_symbol_pointer("@other_global", actual_id)},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    if (!validate_rejects_with_message(
            module,
            "bir call arg in @user must not pair LinkNameId with a different declared global name")) {
      return fail("BIR verifier should reject pointer-value global LinkNameId/display-name mismatch");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_callee");
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee = "actual_callee",
        .args = {bir::Value::named_symbol_pointer("@other_callee", actual_id)},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    if (!validate_rejects_with_message(
            module,
            "bir call arg in @user must not pair LinkNameId with a different declared function name")) {
      return fail(
          "BIR verifier should reject pointer-value function LinkNameId/display-name mismatch");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.globals.front().is_extern = false;
    module.globals.front().initializer_elements = {
        bir::Value::named_symbol_pointer("@other_global", actual_id)};
    if (!validate_rejects_with_message(
            module,
            "bir global initializer element @actual_global must not pair LinkNameId with a different declared global name")) {
      return fail(
          "BIR verifier should reject initializer element pointer-value LinkNameId/display-name mismatch");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.functions.back().blocks.front().insts.push_back(bir::StoreGlobalInst{
        .global_name = "other_global",
        .global_name_id = actual_id,
        .value = bir::Value::immediate_i32(1),
    });
    if (!validate_rejects_with_message(
            module,
            "bir global store in @user must not pair LinkNameId with a different declared global name")) {
      return fail("BIR verifier should reject global store LinkNameId/display-name mismatch");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.functions.back().blocks.front().insts.push_back(bir::LoadGlobalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
        .global_name = "other_global",
        .global_name_id = actual_id,
    });
    if (!validate_rejects_with_message(
            module,
            "bir global load in @user must not pair LinkNameId with a different declared global name")) {
      return fail("BIR verifier should reject global load LinkNameId/display-name mismatch");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.functions.back().blocks.front().insts.push_back(bir::StoreGlobalInst{
        .global_name_id = actual_id,
        .value = bir::Value::immediate_i32(1),
    });
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should allow ID-only global stores to declared globals");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.functions.back().blocks.front().insts.push_back(bir::LoadGlobalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
        .global_name_id = actual_id,
    });
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should allow ID-only global loads from declared globals");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.functions.back().blocks.front().insts.push_back(bir::StoreGlobalInst{
        .global_name = "actual_global",
        .value = bir::Value::immediate_i32(1),
    });
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should preserve raw-import/no-id global store compatibility without LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.functions.back().blocks.front().insts.push_back(bir::LoadGlobalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
        .global_name = "actual_global",
    });
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should preserve raw-import/no-id global load compatibility without LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.functions.back().blocks.front().insts.push_back(bir::StoreGlobalInst{
        .global_name = "actual_global",
        .global_name_id = 9999,
        .value = bir::Value::immediate_i32(1),
    });
    if (!validate_rejects_with_message(
            module, "bir global store in @user must reference a known LinkNameId")) {
      return fail("BIR verifier should reject global stores with unknown LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.functions.back().blocks.front().insts.push_back(bir::LoadGlobalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
        .global_name = "actual_global",
        .global_name_id = 9999,
    });
    if (!validate_rejects_with_message(
            module, "bir global load in @user must reference a known LinkNameId")) {
      return fail("BIR verifier should reject global loads with unknown LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId missing_id = module.names.link_names.intern("missing_global");
    module.functions.back().blocks.front().insts.push_back(bir::StoreGlobalInst{
        .global_name = "actual_global",
        .global_name_id = missing_id,
        .value = bir::Value::immediate_i32(1),
    });
    if (!validate_rejects_with_message(
            module, "bir global store in @user must reference a declared global")) {
      return fail("BIR verifier should reject global stores whose raw name only matches by name");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId missing_id = module.names.link_names.intern("missing_global");
    module.functions.back().blocks.front().insts.push_back(bir::LoadGlobalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
        .global_name = "actual_global",
        .global_name_id = missing_id,
    });
    if (!validate_rejects_with_message(
            module, "bir global load in @user must reference a declared global")) {
      return fail("BIR verifier should reject global loads whose raw name only matches by name");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_callee");
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee = "other_callee",
        .callee_link_name_id = actual_id,
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    if (!validate_rejects_with_message(
            module,
            "bir call in @user must not pair LinkNameId with a different declared function name")) {
      return fail("BIR verifier should reject direct-call LinkNameId/display-name mismatch");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_callee");
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee_link_name_id = actual_id,
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should allow ID-only direct calls to declared functions");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee_link_name_id = 9999,
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    if (!validate_rejects_with_message(
            module, "bir call in @user must reference a known LinkNameId")) {
      return fail("BIR verifier should reject direct calls with unknown LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId missing_id = module.names.link_names.intern("missing_callee");
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee_link_name_id = missing_id,
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    if (!validate_rejects_with_message(
            module,
            "bir call in @user must reference a declared function by LinkNameId")) {
      return fail("BIR verifier should reject ID-only direct calls to undeclared functions");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId missing_id = module.names.link_names.intern("missing_callee");
    module.functions.back().blocks.front().insts.push_back(bir::CallInst{
        .callee = "actual_callee",
        .callee_link_name_id = missing_id,
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    if (!validate_rejects_with_message(
            module,
            "bir call in @user must reference a declared function by LinkNameId")) {
      return fail("BIR verifier should reject direct calls whose raw callee only matches by name");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.globals.front().is_extern = false;
    module.globals.front().initializer_symbol_name = "other_global";
    module.globals.front().initializer_symbol_name_id = actual_id;
    if (!validate_rejects_with_message(
            module,
            "bir global initializer symbol @actual_global must not pair LinkNameId with a different declared global name")) {
      return fail("BIR verifier should reject initializer-symbol LinkNameId/display-name mismatch");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.globals.front().is_extern = false;
    module.globals.front().initializer_symbol_name = "compat_unknown";
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail(
          "BIR verifier should preserve raw-import/no-id initializer-symbol compatibility without LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_global");
    module.globals.front().is_extern = false;
    module.globals.front().initializer_symbol_name = "stale_global_display";
    module.globals.front().initializer_symbol_name_id = actual_id;
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail(
          "BIR verifier should allow drifted initializer-symbol global displays when LinkNameId is declared");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId actual_id = module.names.link_names.intern("actual_callee");
    module.globals.front().is_extern = false;
    module.globals.front().initializer_symbol_name = "stale_function_display";
    module.globals.front().initializer_symbol_name_id = actual_id;
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail(
          "BIR verifier should allow drifted initializer-symbol function displays when LinkNameId is declared");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    module.globals.front().is_extern = false;
    module.globals.front().initializer_symbol_name = "actual_global";
    module.globals.front().initializer_symbol_name_id = 9999;
    if (!validate_rejects_with_message(
            module, "bir global initializer symbol @actual_global must reference a known LinkNameId")) {
      return fail("BIR verifier should reject initializer symbols with unknown LinkNameId");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId missing_id = module.names.link_names.intern("missing_global");
    module.globals.front().is_extern = false;
    module.globals.front().initializer_symbol_name = "stale_missing_display";
    module.globals.front().initializer_symbol_name_id = missing_id;
    if (!validate_rejects_with_message(
            module,
            "bir global initializer symbol @actual_global must reference a declared global or function by LinkNameId")) {
      return fail(
          "BIR verifier should reject initializer symbols whose LinkNameId has no declared symbol");
    }
  }

  {
    auto module = make_link_name_mismatch_verifier_module();
    const c4c::LinkNameId missing_id = module.names.link_names.intern("missing_global");
    module.globals.front().is_extern = false;
    module.globals.front().initializer_symbol_name = "actual_global";
    module.globals.front().initializer_symbol_name_id = missing_id;
    if (!validate_rejects_with_message(
            module,
            "bir global initializer symbol @actual_global must not pair LinkNameId with a different declared global name")) {
      return fail(
          "BIR verifier should reject initializer symbols whose raw name only matches by name");
    }
  }

  return 0;
}

c4c::backend::bir::Module make_local_slot_id_verifier_module() {
  namespace bir = c4c::backend::bir;

  bir::Module module;
  const c4c::SlotNameId alpha_id = module.names.slot_names.intern("slot.alpha");
  const c4c::SlotNameId beta_id = module.names.slot_names.intern("slot.beta");

  bir::Function function;
  function.name = "slot_user";
  function.return_type = bir::TypeKind::Void;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "slot.alpha",
      .slot_id = alpha_id,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "slot.beta",
      .slot_id = beta_id,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

int expect_bir_verifier_rejects_local_slot_id_mismatches() {
  namespace bir = c4c::backend::bir;

  {
    auto module = make_local_slot_id_verifier_module();
    auto& slots = module.functions.back().local_slots;
    slots[1].slot_id = slots[0].slot_id;
    if (!validate_rejects_with_message(
            module, "bir local slot in @slot_user must not reuse an existing SlotNameId")) {
      return fail("BIR verifier should reject duplicate LocalSlot SlotNameId in one function");
    }
  }

  {
    auto module = make_local_slot_id_verifier_module();
    const c4c::SlotNameId alpha_id = module.functions.back().local_slots[0].slot_id;
    module.functions.back().blocks.front().insts.push_back(bir::LoadLocalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
        .slot_name = "slot.beta",
        .slot_id = alpha_id,
    });
    if (!validate_rejects_with_message(
            module,
            "bir local load in @slot_user must not pair SlotNameId with a different local slot name")) {
      return fail("BIR verifier should reject LoadLocal SlotNameId/display-name mismatch");
    }
  }

  {
    auto module = make_local_slot_id_verifier_module();
    const c4c::SlotNameId alpha_id = module.functions.back().local_slots[0].slot_id;
    module.functions.back().blocks.front().insts.push_back(bir::StoreLocalInst{
        .slot_name = "slot.beta",
        .slot_id = alpha_id,
        .value = bir::Value::immediate_i32(7),
    });
    if (!validate_rejects_with_message(
            module,
            "bir local store in @slot_user must not pair SlotNameId with a different local slot name")) {
      return fail("BIR verifier should reject StoreLocal SlotNameId/display-name mismatch");
    }
  }

  {
    auto module = make_local_slot_id_verifier_module();
    const c4c::SlotNameId alpha_id = module.functions.back().local_slots[0].slot_id;
    module.functions.back().blocks.front().insts.push_back(bir::LoadLocalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
        .slot_name = "slot.alpha",
        .slot_id = alpha_id,
        .address = bir::MemoryAddress{
            .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
            .base_name = "slot.beta",
            .size_bytes = 4,
            .align_bytes = 4,
            .base_slot_id = alpha_id,
        },
    });
    if (!validate_rejects_with_message(
            module,
            "bir local load address in @slot_user must not pair SlotNameId with a different local slot name")) {
      return fail("BIR verifier should reject local MemoryAddress SlotNameId/display-name mismatch");
    }
  }

  {
    auto module = make_local_slot_id_verifier_module();
    module.functions.back().blocks.front().insts.push_back(bir::LoadLocalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
        .slot_name = "slot.alpha",
    });
    if (!validate_accepts(module)) {
      return fail("BIR verifier should preserve no-id local-slot spelling compatibility");
    }
  }

  return 0;
}

int expect_admitted_aggregate_pointer_field_global() {
  auto result = try_lower_to_bir_with_options(make_admitted_aggregate_pointer_field_global_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("expected semantic BIR lowering to admit aggregate globals with pointer fields");
  }

  const auto find_global = [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : result.module->globals) {
      if (global.name == name) {
        return &global;
      }
    }
    return nullptr;
  };

  const auto* global = find_global("gptrpad");
  if (global == nullptr || global->type != TypeKind::I8 || global->size_bytes != 16 ||
      global->initializer_elements.size() != 9) {
    return fail("aggregate pointer-field globals should lower into byte-addressable aggregate storage");
  }
  if (global->initializer_elements.front() !=
      c4c::backend::bir::Value::named(TypeKind::Ptr, "@.str0")) {
    return fail("aggregate pointer-field globals should preserve the pointed-to string symbol");
  }
  if (global->initializer_elements[1] != c4c::backend::bir::Value::immediate_i8(99)) {
    return fail("aggregate pointer-field globals should preserve the explicit byte payload");
  }
  for (std::size_t index = 2; index < global->initializer_elements.size(); ++index) {
    if (global->initializer_elements[index] != c4c::backend::bir::Value::immediate_i8(0)) {
      return fail("aggregate pointer-field globals should zero-fill the aggregate tail bytes");
    }
  }

  return 0;
}

int expect_admitted_aggregate_zero_sized_member_global() {
  auto result =
      try_lower_to_bir_with_options(make_admitted_aggregate_zero_sized_member_global_module(),
                                    BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("expected semantic BIR lowering to admit aggregate globals with zero-sized aggregate members");
  }

  const auto find_global = [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : result.module->globals) {
      if (global.name == name) {
        return &global;
      }
    }
    return nullptr;
  };

  const auto* global = find_global("gzero_member");
  if (global == nullptr || global->type != TypeKind::I8 || global->size_bytes != 2 ||
      global->initializer_elements.size() != 2) {
    return fail("aggregate globals with zero-sized members should collapse to the surrounding byte-addressable storage");
  }
  if (global->initializer_elements[0] != c4c::backend::bir::Value::immediate_i8(1) ||
      global->initializer_elements[1] != c4c::backend::bir::Value::immediate_i8(18)) {
    return fail("aggregate globals with zero-sized members should preserve only the concrete surrounding payload bytes");
  }

  return 0;
}

int expect_structured_block_label_ids() {
  auto result = try_lower_to_bir_with_options(make_structured_block_label_id_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("expected semantic BIR lowering to admit structured block label id fixture");
  }

  const auto& module = *result.module;
  if (module.functions.size() != 1) {
    return fail("structured block label id fixture should lower the test function");
  }

  const auto* switch_function = &module.functions.front();
  const auto find_block = [&](const c4c::backend::bir::Function& function,
                              std::string_view label) -> const c4c::backend::bir::Block* {
    for (const auto& block : function.blocks) {
      if (block.label == label) {
        return &block;
      }
    }
    return nullptr;
  };
  const auto expect_label_id = [&](const c4c::backend::bir::Block* block) {
    return block != nullptr && block->label_id != c4c::kInvalidBlockLabel &&
           module.names.block_labels.spelling(block->label_id) == block->label;
  };

  const auto* entry = find_block(*switch_function, "entry");
  const auto* switch_case_1 = find_block(*switch_function, "entry.switch.case.1");
  const auto* case_one = find_block(*switch_function, "case_one");
  const auto* case_two = find_block(*switch_function, "case_two");
  const auto* case_default = find_block(*switch_function, "case_default");
  const auto* join = find_block(*switch_function, "join");
  if (!expect_label_id(entry) || !expect_label_id(switch_case_1) ||
      !expect_label_id(case_one) || !expect_label_id(case_two) ||
      !expect_label_id(case_default) || !expect_label_id(join)) {
    return fail("lowered BIR blocks should carry interned structured label ids");
  }

  if (entry->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      entry->terminator.true_label_id != case_one->label_id ||
      entry->terminator.false_label_id != switch_case_1->label_id) {
    return fail("switch entry compare terminator should carry structured case label ids");
  }
  if (switch_case_1->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      switch_case_1->terminator.true_label_id != case_two->label_id ||
      switch_case_1->terminator.false_label_id != case_default->label_id) {
    return fail("switch-generated compare block should carry structured successor label ids");
  }
  if (case_one->terminator.target_label_id != join->label_id ||
      case_two->terminator.target_label_id != join->label_id ||
      case_default->terminator.target_label_id != join->label_id) {
    return fail("plain branch terminators should carry structured target label ids");
  }

  const auto* phi = join->insts.empty()
                        ? nullptr
                        : std::get_if<c4c::backend::bir::PhiInst>(&join->insts.front());
  if (phi == nullptr || phi->incomings.size() != 4) {
    return fail("structured block label id fixture should lower the scalar phi join");
  }
  for (const auto& incoming : phi->incomings) {
    const auto* source = find_block(*switch_function, incoming.label);
    if (source != nullptr && incoming.label_id != source->label_id) {
      return fail("scalar phi incoming labels should carry matching structured label ids");
    }
    if (source == nullptr &&
        (incoming.label != "ghost" || incoming.label_id != c4c::kInvalidBlockLabel ||
         module.names.block_labels.find(incoming.label) != c4c::kInvalidBlockLabel)) {
      return fail("unresolved phi incoming labels should preserve raw strings and invalid ids");
    }
  }

  return 0;
}

c4c::backend::bir::Module make_block_label_verifier_identity_module() {
  namespace bir = c4c::backend::bir;

  bir::Module module;
  const c4c::BlockLabelId entry_id = module.names.block_labels.intern("entry");
  const c4c::BlockLabelId then_id = module.names.block_labels.intern("then");
  const c4c::BlockLabelId else_id = module.names.block_labels.intern("else");
  const c4c::BlockLabelId join_id = module.names.block_labels.intern("join");

  bir::Function function;
  function.name = "block_identity";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I1,
      .name = "%cond",
  });

  bir::Block entry;
  entry.label = "raw.entry";
  entry.label_id = entry_id;
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "%cond"),
      .true_label = "stale.then.raw",
      .false_label = "stale.else.raw",
      .true_label_id = then_id,
      .false_label_id = else_id,
  };
  function.blocks.push_back(std::move(entry));

  bir::Block then_block;
  then_block.label = "raw.then";
  then_block.label_id = then_id;
  then_block.terminator = bir::BranchTerminator{
      .target_label = "stale.join.raw",
      .target_label_id = join_id,
  };
  function.blocks.push_back(std::move(then_block));

  bir::Block else_block;
  else_block.label = "raw.else";
  else_block.label_id = else_id;
  else_block.terminator = bir::BranchTerminator{
      .target_label = "stale.join.raw",
      .target_label_id = join_id,
  };
  function.blocks.push_back(std::move(else_block));

  bir::Block join_block;
  join_block.label = "raw.join";
  join_block.label_id = join_id;
  join_block.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%merged"),
      .incomings = {bir::PhiIncoming{
                        .label = "stale.then.raw",
                        .value = bir::Value::immediate_i32(1),
                        .label_id = then_id,
                    },
                    bir::PhiIncoming{
                        .label = "stale.else.raw",
                        .value = bir::Value::immediate_i32(2),
                        .label_id = else_id,
                    }},
  });
  join_block.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "%merged")};
  function.blocks.push_back(std::move(join_block));

  module.functions.push_back(std::move(function));
  return module;
}

int expect_bir_verifier_prefers_block_label_ids() {
  namespace bir = c4c::backend::bir;

  {
    const auto module = make_block_label_verifier_identity_module();
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should accept stale raw block labels when structured ids match");
    }
  }

  {
    auto module = make_block_label_verifier_identity_module();
    const c4c::BlockLabelId ghost_id = module.names.block_labels.intern("ghost.branch");
    module.functions.front().blocks[1].terminator.target_label = "raw.join";
    module.functions.front().blocks[1].terminator.target_label_id = ghost_id;
    if (!validate_rejects_with_message(
            module,
            "bir branch in @block_identity must target a block in the same function by BlockLabelId")) {
      return fail("BIR verifier should reject branch target ids even when raw labels match");
    }
  }

  {
    auto module = make_block_label_verifier_identity_module();
    const c4c::BlockLabelId ghost_id = module.names.block_labels.intern("ghost.cond");
    module.functions.front().blocks[0].terminator.true_label = "raw.then";
    module.functions.front().blocks[0].terminator.true_label_id = ghost_id;
    if (!validate_rejects_with_message(
            module,
            "bir cond_br in @block_identity must target blocks in the same function by BlockLabelId")) {
      return fail("BIR verifier should reject cond_br target ids even when raw labels match");
    }
  }

  {
    auto module = make_block_label_verifier_identity_module();
    const c4c::BlockLabelId ghost_id = module.names.block_labels.intern("ghost.phi");
    auto* phi = std::get_if<bir::PhiInst>(&module.functions.front().blocks[3].insts.front());
    if (phi == nullptr) {
      return fail("BIR verifier block-label test fixture lost its phi");
    }
    phi->incomings.front().label = "raw.then";
    phi->incomings.front().label_id = ghost_id;
    if (!validate_rejects_with_message(
            module,
            "bir phi in @block_identity must reference blocks in the same function by BlockLabelId")) {
      return fail("BIR verifier should reject phi incoming ids even when raw labels match");
    }
  }

  {
    auto module = make_block_label_verifier_identity_module();
    auto& function = module.functions.front();
    for (auto& block : function.blocks) {
      block.label = std::string(module.names.block_labels.spelling(block.label_id));
      block.label_id = c4c::kInvalidBlockLabel;
      switch (block.terminator.kind) {
        case bir::TerminatorKind::Branch:
          block.terminator.target_label =
              std::string(module.names.block_labels.spelling(block.terminator.target_label_id));
          block.terminator.target_label_id = c4c::kInvalidBlockLabel;
          break;
        case bir::TerminatorKind::CondBranch:
          block.terminator.true_label =
              std::string(module.names.block_labels.spelling(block.terminator.true_label_id));
          block.terminator.false_label =
              std::string(module.names.block_labels.spelling(block.terminator.false_label_id));
          block.terminator.true_label_id = c4c::kInvalidBlockLabel;
          block.terminator.false_label_id = c4c::kInvalidBlockLabel;
          break;
        case bir::TerminatorKind::Return:
          break;
      }
      for (auto& inst : block.insts) {
        if (auto* phi = std::get_if<bir::PhiInst>(&inst); phi != nullptr) {
          for (auto& incoming : phi->incomings) {
            incoming.label = std::string(module.names.block_labels.spelling(incoming.label_id));
            incoming.label_id = c4c::kInvalidBlockLabel;
          }
        }
      }
    }
    std::string error;
    if (!bir::validate(module, &error)) {
      return fail("BIR verifier should preserve raw block-label compatibility without ids");
    }
  }

  return 0;
}

int expect_admitted_aggregate_string_array_field_global() {
  auto result =
      try_lower_to_bir_with_options(make_admitted_aggregate_string_array_field_global_module(),
                                    BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("expected semantic BIR lowering to admit aggregate globals with string-backed array fields");
  }

  const auto find_global = [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : result.module->globals) {
      if (global.name == name) {
        return &global;
      }
    }
    return nullptr;
  };

  const auto* global = find_global("gstring_field");
  if (global == nullptr || global->type != TypeKind::I8 || global->size_bytes != 17 ||
      global->initializer_elements.size() != 17) {
    return fail("aggregate globals with string-backed array fields should lower into byte-addressable aggregate storage");
  }
  if (global->initializer_elements[0] != c4c::backend::bir::Value::immediate_i8('h') ||
      global->initializer_elements[1] != c4c::backend::bir::Value::immediate_i8('e') ||
      global->initializer_elements[2] != c4c::backend::bir::Value::immediate_i8('l') ||
      global->initializer_elements[3] != c4c::backend::bir::Value::immediate_i8('l') ||
      global->initializer_elements[4] != c4c::backend::bir::Value::immediate_i8('o')) {
    return fail("aggregate globals with string-backed array fields should preserve the string byte prefix");
  }
  for (std::size_t index = 5; index < 16; ++index) {
    if (global->initializer_elements[index] != c4c::backend::bir::Value::immediate_i8(0)) {
      return fail("aggregate globals with string-backed array fields should zero-fill the remaining array bytes");
    }
  }
  if (global->initializer_elements[16] != c4c::backend::bir::Value::immediate_i8(42)) {
    return fail("aggregate globals with string-backed array fields should preserve following scalar payload bytes");
  }

  return 0;
}

int expect_admitted_aggregate_long_double_field_global() {
  auto result =
      try_lower_to_bir_with_options(make_admitted_aggregate_long_double_field_global_module(),
                                    BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("expected semantic BIR lowering to admit aggregate globals with x86_fp80 fields");
  }

  const auto find_global = [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : result.module->globals) {
      if (global.name == name) {
        return &global;
      }
    }
    return nullptr;
  };

  const auto* global = find_global("gld_field");
  if (global == nullptr || global->type != TypeKind::I8 || global->size_bytes != 16 ||
      global->initializer_elements.size() != 16) {
    return fail("aggregate globals with x86_fp80 fields should lower into 16-byte byte-addressable storage");
  }

  const std::array<std::int8_t, 16> expected_bytes = {
      static_cast<std::int8_t>(0x00), static_cast<std::int8_t>(0xD0),
      static_cast<std::int8_t>(0xCC), static_cast<std::int8_t>(0xCC),
      static_cast<std::int8_t>(0xCC), static_cast<std::int8_t>(0xCC),
      static_cast<std::int8_t>(0xCC), static_cast<std::int8_t>(0xF8),
      static_cast<std::int8_t>(0x03), static_cast<std::int8_t>(0x40),
      0, 0, 0, 0, 0, 0,
  };
  for (std::size_t index = 0; index < expected_bytes.size(); ++index) {
    if (global->initializer_elements[index] !=
        c4c::backend::bir::Value::immediate_i8(expected_bytes[index])) {
      return fail("aggregate globals with x86_fp80 fields should preserve little-endian long-double bytes with tail padding");
    }
  }

  return 0;
}

void assign_semantic_function_name(LirModule* module,
                                   std::string_view semantic_name,
                                   std::string_view corrupted_raw_name) {
  module->link_name_texts = std::make_shared<c4c::TextTable>();
  module->link_names.attach_text_table(module->link_name_texts.get());
  module->functions.front().link_name_id = module->link_names.intern(semantic_name);
  module->functions.front().name = std::string(corrupted_raw_name);
}

LirModule make_unsupported_inline_asm_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_inline_asm";
  function.signature_text = "define void @bad_inline_asm()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirInlineAsmOp{
      .result = LirOperand("%t0"),
      .ret_type = "{ i32, i32 }",
      .asm_text = "",
      .constraints = "=r,=r",
      .side_effects = true,
      .args_str = "",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_structured_inline_asm_metadata_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  c4c::TypeSpec int_type{};
  int_type.base = c4c::TB_INT;
  int_type.enum_underlying_base = c4c::TB_VOID;

  LirFunction function;
  function.name = "structured_inline_asm_metadata";
  function.signature_text = "define i32 @structured_inline_asm_metadata(i32 %x)";
  function.params.push_back({"%x", int_type});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirInlineAsmOp{
      .result = LirOperand("%out"),
      .ret_type = "i32",
      .asm_text = "add %w0, %x1, #7",
      .constraints = "=r,0,I,~{memory},~{cc}",
      .side_effects = true,
      .args_str = "i32 %x, i32 7",
      .clobbers = {"memory", "cc"},
  });
  entry.terminator = LirRet{
      .value_str = "%out",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_rendered_only_clobber_inline_asm_metadata_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  c4c::TypeSpec int_type{};
  int_type.base = c4c::TB_INT;
  int_type.enum_underlying_base = c4c::TB_VOID;

  LirFunction function;
  function.name = "rendered_only_clobber_inline_asm_metadata";
  function.signature_text =
      "define void @rendered_only_clobber_inline_asm_metadata(i32 %x)";
  function.params.push_back({"%x", int_type});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirInlineAsmOp{
      .result = LirOperand(""),
      .ret_type = "void",
      .asm_text = "mov %0, %0",
      .constraints = "r,~{memory}",
      .side_effects = true,
      .args_str = "i32 %x",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_unsupported_template_modifier_inline_asm_metadata_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  c4c::TypeSpec int_type{};
  int_type.base = c4c::TB_INT;
  int_type.enum_underlying_base = c4c::TB_VOID;

  LirFunction function;
  function.name = "unsupported_template_modifier_inline_asm_metadata";
  function.signature_text =
      "define i32 @unsupported_template_modifier_inline_asm_metadata(i32 %x)";
  function.params.push_back({"%x", int_type});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirInlineAsmOp{
      .result = LirOperand("%out"),
      .ret_type = "i32",
      .asm_text = "mov %q0, %1",
      .constraints = "=r,0",
      .side_effects = true,
      .args_str = "i32 %x",
  });
  entry.terminator = LirRet{
      .value_str = "%out",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int inline_asm_lir_lowering_preserves_structured_operand_metadata() {
  auto result = try_lower_to_bir_with_options(make_structured_inline_asm_metadata_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("structured inline asm metadata fixture should lower to BIR");
  }
  const auto& function = result.module->functions.front();
  const auto* call =
      std::get_if<c4c::backend::bir::CallInst>(&function.blocks.front().insts.front());
  if (call == nullptr || !call->inline_asm.has_value()) {
    return fail("structured inline asm fixture should lower to an inline asm call");
  }
  const auto& inline_asm = *call->inline_asm;
  if (inline_asm.asm_text != "add %w0, %x1, #7" ||
      inline_asm.constraints != "=r,0,I,~{memory},~{cc}" ||
      !inline_asm.side_effects ||
      !inline_asm.args_text.empty() ||
      inline_asm.clobbers.size() != 2 ||
      inline_asm.clobbers[0] != "memory" ||
      inline_asm.clobbers[1] != "cc" ||
      inline_asm.operands.size() != 5 ||
      inline_asm.has_named_operand_references ||
      !inline_asm.has_template_modifiers ||
      !inline_asm.unsupported_facts.empty()) {
    return fail("structured inline asm metadata lost top-level facts");
  }
  if (inline_asm.operands[0].kind !=
          c4c::backend::bir::InlineAsmOperandKind::RegisterOutput ||
      inline_asm.operands[0].output_index.value_or(99) != 0 ||
      inline_asm.operands[1].kind !=
          c4c::backend::bir::InlineAsmOperandKind::TiedInput ||
      inline_asm.operands[1].arg_index.value_or(99) != 0 ||
      inline_asm.operands[1].tied_output_index.value_or(99) != 0 ||
      inline_asm.operands[2].kind !=
          c4c::backend::bir::InlineAsmOperandKind::IntegerImmediateInput ||
      inline_asm.operands[2].arg_index.value_or(99) != 1 ||
      inline_asm.operands[3].kind !=
          c4c::backend::bir::InlineAsmOperandKind::Clobber ||
      inline_asm.operands[3].name.value_or("") != "memory" ||
      inline_asm.operands[4].kind !=
          c4c::backend::bir::InlineAsmOperandKind::Clobber ||
      inline_asm.operands[4].name.value_or("") != "cc") {
    return fail("structured inline asm operand facts were not preserved");
  }
  if (call->args.size() != 2 ||
      call->args[0] != c4c::backend::bir::Value::named(TypeKind::I32, "%x") ||
      call->args[1] != c4c::backend::bir::Value::immediate_i32(7)) {
    return fail("structured inline asm typed operands were not lowered");
  }

  auto unsupported_result = try_lower_to_bir_with_options(
      make_unsupported_template_modifier_inline_asm_metadata_module(),
      BirLoweringOptions{});
  if (!unsupported_result.module.has_value()) {
    return fail("unsupported inline asm modifier fixture should still lower to BIR facts");
  }
  const auto& unsupported_function = unsupported_result.module->functions.front();
  const auto* unsupported_call =
      std::get_if<c4c::backend::bir::CallInst>(
          &unsupported_function.blocks.front().insts.front());
  if (unsupported_call == nullptr || !unsupported_call->inline_asm.has_value() ||
      !unsupported_call->inline_asm->has_template_modifiers) {
    return fail("unsupported inline asm modifier fixture lost modifier visibility");
  }
  bool saw_unsupported_modifier_fact = false;
  for (const auto& fact : unsupported_call->inline_asm->unsupported_facts) {
    saw_unsupported_modifier_fact =
        saw_unsupported_modifier_fact || fact == "unsupported_template_modifiers";
  }
  if (!saw_unsupported_modifier_fact) {
    return fail("unsupported inline asm modifier should emit fail-closed fact");
  }

  auto rendered_only_result = try_lower_to_bir_with_options(
      make_rendered_only_clobber_inline_asm_metadata_module(), BirLoweringOptions{});
  if (!rendered_only_result.module.has_value()) {
    return fail("rendered-only clobber fixture should still lower to BIR facts");
  }
  const auto& rendered_only_function = rendered_only_result.module->functions.front();
  const auto* rendered_only_call =
      std::get_if<c4c::backend::bir::CallInst>(
          &rendered_only_function.blocks.front().insts.front());
  if (rendered_only_call == nullptr ||
      !rendered_only_call->inline_asm.has_value() ||
      !rendered_only_call->inline_asm->clobbers.empty() ||
      rendered_only_call->inline_asm->operands.size() != 2 ||
      rendered_only_call->inline_asm->operands[1].kind !=
          c4c::backend::bir::InlineAsmOperandKind::Clobber ||
      rendered_only_call->inline_asm->operands[1].name.has_value()) {
    return fail("rendered-only clobber should not become structured BIR authority");
  }
  bool saw_rendered_only_clobber_fact = false;
  for (const auto& fact : rendered_only_call->inline_asm->unsupported_facts) {
    saw_rendered_only_clobber_fact =
        saw_rendered_only_clobber_fact || fact == "unsupported_clobber_constraint1";
  }
  if (!saw_rendered_only_clobber_fact) {
    return fail("rendered-only clobber should emit fail-closed fact");
  }
  return 0;
}

LirModule make_bad_direct_call_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_direct_call";
  function.signature_text = "define void @bad_direct_call()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@callee"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr @missing_object",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_byval_call_arg_boundary_module(
    std::vector<lir::LirTypeRef> arg_type_refs) {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.Payload = type { i32 }");
  module.type_decls.push_back("%struct.OtherPayload = type { i32 }");

  const c4c::StructNameId payload_id = module.struct_names.intern("%struct.Payload");
  const c4c::StructNameId other_payload_id =
      module.struct_names.intern("%struct.OtherPayload");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")}},
  });
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = other_payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")}},
  });

  LirGlobal payload;
  payload.name = "payload";
  payload.llvm_type = "%struct.Payload";
  payload.init_text = "{ i32 7 }";
  payload.align_bytes = 4;
  module.globals.push_back(std::move(payload));

  LirFunction function;
  function.name = "byval_call_arg_boundary";
  function.signature_text = "define void @byval_call_arg_boundary()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@sink"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr byval(%struct.Payload) @payload",
      .arg_type_refs = std::move(arg_type_refs),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_legacy_byval_call_arg_without_type_refs_still_lowers() {
  auto result = try_lower_to_bir_with_options(make_byval_call_arg_boundary_module({}),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("legacy byval call-argument fixture without arg_type_refs should still lower");
  }

  for (const auto& function : result.module->functions) {
    if (function.name != "byval_call_arg_boundary" || function.blocks.empty()) {
      continue;
    }
    for (const auto& inst : function.blocks.front().insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr) {
        continue;
      }
      if (call->arg_abi.size() == 1 && call->arg_abi.front().byval_copy &&
          call->arg_abi.front().size_bytes == 4 && call->arg_abi.front().align_bytes == 4) {
        return 0;
      }
    }
  }
  return fail("legacy byval call-argument lowering should preserve byval ABI metadata");
}

int expect_metadata_rich_byval_call_arg_without_struct_id_fails_closed() {
  auto result = try_lower_to_bir_with_options(
      make_byval_call_arg_boundary_module({lir::LirTypeRef("ptr byval(%struct.Payload)")}),
      BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("metadata-rich byval call argument without StructNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in semantic call family 'direct-call semantic family'")) {
    return fail("missing direct-call family failure for byval call argument without StructNameId");
  }
  return 0;
}

int expect_metadata_rich_byval_call_arg_mismatch_fails_closed() {
  LirModule module = make_byval_call_arg_boundary_module({});
  const c4c::StructNameId other_payload_id =
      module.struct_names.find("%struct.OtherPayload");
  module.functions.front().blocks.front().insts.front() = LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@sink"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr byval(%struct.Payload) @payload",
      .arg_type_refs = {
          lir::LirTypeRef::struct_type("ptr byval(%struct.Payload)", other_payload_id)},
  };

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("metadata-rich byval call argument with mismatched StructNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in semantic call family 'direct-call semantic family'")) {
    return fail("missing direct-call family failure for mismatched byval call argument StructNameId");
  }
  return 0;
}

LirModule make_direct_structured_call_signature_module(bool with_signature) {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const c4c::LinkNameId callee_id = module.link_names.intern("semantic_direct_sink");

  c4c::codegen::lir::LirExternDecl callee;
  callee.name = "stale_direct_sink";
  callee.link_name_id = callee_id;
  callee.return_type_str = "void";
  callee.return_type = lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(callee));

  LirFunction function;
  function.name = with_signature ? "direct_call_prefers_structured_signature"
                                 : "direct_call_missing_structured_signature";
  function.signature_text = with_signature
                                ? "define void @direct_call_prefers_structured_signature(ptr %arg)"
                                : "define void @direct_call_missing_structured_signature()";
  if (with_signature) {
    c4c::TypeSpec ptr_type{};
    ptr_type.base = c4c::TB_VOID;
    ptr_type.ptr_level = 1;
    function.params.push_back({"%arg", ptr_type});
  }

  LirBlock entry;
  entry.label = "entry";
  LirCallOp call{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@stale_direct_sink"),
      .direct_callee_link_name_id = callee_id,
      .callee_type_suffix = with_signature ? "(i32)" : "",
      .args_str = with_signature ? "i32 7" : "",
  };
  if (with_signature) {
    call.callee_signature = void_call_signature({"ptr"});
    call.structured_args.push_back(lir::LirCallArg{
        .type = "ptr",
        .operand = LirOperand("%arg"),
        .type_ref = lir::LirTypeRef("ptr"),
    });
  }
  entry.insts.push_back(std::move(call));
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_direct_call_prefers_structured_callee_signature_over_stale_suffix() {
  auto result = try_lower_to_bir_with_options(
      make_direct_structured_call_signature_module(true), BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("direct call should lower using structured callee signature despite stale suffix");
  }

  for (const auto& function : result.module->functions) {
    if (function.name != "direct_call_prefers_structured_signature" ||
        function.blocks.empty()) {
      continue;
    }
    for (const auto& inst : function.blocks.front().insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr) continue;
      if (!call->is_indirect && call->callee == "semantic_direct_sink" &&
          call->arg_types.size() == 1 && call->arg_types.front() == TypeKind::Ptr) {
        return 0;
      }
    }
  }
  return fail("direct call should preserve structured pointer parameter type in BIR");
}

LirModule make_direct_structured_byval_call_signature_module(bool mismatched_signature) {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.Payload = type { i32, i32 }");
  module.type_decls.push_back("%struct.OtherPayload = type { i32, i32 }");

  const c4c::LinkNameId callee_id = module.link_names.intern("semantic_byval_sink");
  c4c::codegen::lir::LirExternDecl callee;
  callee.name = "stale_byval_sink";
  callee.link_name_id = callee_id;
  callee.return_type_str = "void";
  callee.return_type = lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(callee));

  const c4c::StructNameId payload_id = module.struct_names.intern("%struct.Payload");
  const c4c::StructNameId other_payload_id = module.struct_names.intern("%struct.OtherPayload");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = other_payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });
  const c4c::StructNameId signature_arg_id =
      mismatched_signature ? other_payload_id : payload_id;

  LirGlobal payload;
  payload.name = "payload";
  payload.llvm_type = "%struct.Payload";
  payload.init_text = "{ i32 7, i32 11 }";
  payload.align_bytes = 4;
  module.globals.push_back(std::move(payload));

  LirFunction function;
  function.name = !mismatched_signature
                      ? "direct_call_structured_byval_signature"
                      : "direct_call_structured_byval_signature_mismatch";
  function.signature_text = std::string("define void @") + function.name + "()";

  lir::LirCallSignature signature;
  signature.return_type_ref = lir::LirTypeRef("void");
  signature.fixed_param_types = {"ptr"};
  signature.fixed_param_type_refs = {
      lir::LirTypeRef("ptr")};

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@stale_byval_sink"),
      .direct_callee_link_name_id = callee_id,
      .callee_type_suffix = "(i32)",
      .args_str = "i32 7",
      .arg_type_refs = {
          lir::LirTypeRef::struct_type("ptr byval(%struct.Payload)", signature_arg_id)},
      .callee_signature = std::move(signature),
      .structured_args = {
          lir::LirCallArg{
              .type = "ptr byval(%struct.Payload)",
              .operand = LirOperand("@payload"),
              .type_ref = lir::LirTypeRef::struct_type(
                  "ptr byval(%struct.Payload)", signature_arg_id),
          },
      },
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_direct_call_structured_byval_signature_materializes_aggregate_abi() {
  auto result = try_lower_to_bir_with_options(
      make_direct_structured_byval_call_signature_module(false),
      BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("structured direct byval call should lower using aggregate metadata");
  }

  for (const auto& function : result.module->functions) {
    if (function.name != "direct_call_structured_byval_signature" ||
        function.blocks.empty()) {
      continue;
    }
    for (const auto& inst : function.blocks.front().insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr) continue;
      if (!call->is_indirect && call->callee == "semantic_byval_sink" &&
          call->arg_types.size() == 1 && call->arg_types.front() == TypeKind::Ptr &&
          call->arg_abi.size() == 1 && call->arg_abi.front().byval_copy &&
          call->arg_abi.front().size_bytes == 8 &&
          call->arg_abi.front().align_bytes == 4) {
        return 0;
      }
    }
  }
  return fail("structured direct byval call should materialize aggregate ABI facts in BIR");
}

int expect_direct_call_structured_byval_signature_mismatch_fails_closed() {
  LirModule module = make_direct_structured_byval_call_signature_module(true);

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("structured direct byval call with mismatched signature StructNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in semantic call family 'direct-call semantic family'")) {
    return fail("missing direct-call family failure for mismatched direct byval signature metadata");
  }
  return 0;
}

LirModule make_aarch64_direct_hfa_fp_lane_call_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::LinkNameId callee_id = module.link_names.intern("semantic_hfa_sink");
  c4c::codegen::lir::LirExternDecl callee;
  callee.name = "stale_hfa_sink";
  callee.link_name_id = callee_id;
  callee.return_type_str = "void";
  callee.return_type = lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(callee));

  LirFunction function;
  function.name = "aarch64_direct_hfa_fp_lane_call";
  function.signature_text = "define void @aarch64_direct_hfa_fp_lane_call()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@stale_hfa_sink"),
      .direct_callee_link_name_id = callee_id,
      .callee_type_suffix = "(float, float)",
      .args_str = "float 0x3FF19999A0000000, float 0x40019999A0000000",
      .arg_type_refs = {lir::LirTypeRef("float"), lir::LirTypeRef("float")},
      .callee_signature = void_call_signature({"float", "float"}),
      .structured_args = {
          lir::LirCallArg{
              .type = "float",
              .operand = LirOperand("0x3FF19999A0000000"),
              .type_ref = lir::LirTypeRef("float"),
          },
          lir::LirCallArg{
              .type = "float",
              .operand = LirOperand("0x40019999A0000000"),
              .type_ref = lir::LirTypeRef("float"),
          },
      },
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_aarch64_direct_hfa_call_uses_fp_lanes_not_byval() {
  auto result = try_lower_to_bir_with_options(
      make_aarch64_direct_hfa_fp_lane_call_module(), BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("AArch64 fixed HFA direct-call fixture should lower scalar FP ABI lanes");
  }

  for (const auto& function : result.module->functions) {
    if (function.name != "aarch64_direct_hfa_fp_lane_call" ||
        function.blocks.empty()) {
      continue;
    }
    for (const auto& inst : function.blocks.front().insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr) continue;
      if (!call->is_indirect && call->callee == "semantic_hfa_sink" &&
          call->arg_types.size() == 2 &&
          call->arg_types[0] == TypeKind::F32 &&
          call->arg_types[1] == TypeKind::F32 &&
          call->arg_abi.size() == 2 &&
          !call->arg_abi[0].byval_copy &&
          !call->arg_abi[1].byval_copy) {
        return 0;
      }
    }
  }
  return fail("AArch64 fixed HFA call should be represented as FP lanes, not ptr byval");
}

LirModule make_aarch64_variadic_hfa_fp_lane_call_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::LinkNameId callee_id = module.link_names.intern("semantic_variadic_hfa_sink");
  c4c::codegen::lir::LirExternDecl callee;
  callee.name = "stale_variadic_hfa_sink";
  callee.link_name_id = callee_id;
  callee.return_type_str = "void";
  callee.return_type = lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(callee));

  LirFunction function;
  function.name = "aarch64_variadic_hfa_fp_lane_call";
  function.signature_text = "define void @aarch64_variadic_hfa_fp_lane_call(ptr %fmt)";
  function.params.emplace_back("%fmt", c4c::TypeSpec{.base = c4c::TB_VOID, .ptr_level = 1});

  lir::LirCallSignature signature;
  signature.return_type_ref = lir::LirTypeRef("void");
  signature.fixed_param_types = {"ptr"};
  signature.fixed_param_type_refs = {lir::LirTypeRef("ptr")};
  signature.is_variadic = true;

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@stale_variadic_hfa_sink"),
      .direct_callee_link_name_id = callee_id,
      .callee_type_suffix = "(ptr, ...)",
      .args_str = "ptr %fmt, float 0x3FF19999A0000000, float 0x40019999A0000000",
      .arg_type_refs = {lir::LirTypeRef("ptr"), lir::LirTypeRef("float"),
                        lir::LirTypeRef("float")},
      .callee_signature = signature,
      .structured_args = {
          lir::LirCallArg{
              .type = "ptr",
              .operand = LirOperand("%fmt"),
              .type_ref = lir::LirTypeRef("ptr"),
          },
          lir::LirCallArg{
              .type = "float",
              .operand = LirOperand("0x3FF19999A0000000"),
              .type_ref = lir::LirTypeRef("float"),
          },
          lir::LirCallArg{
              .type = "float",
              .operand = LirOperand("0x40019999A0000000"),
              .type_ref = lir::LirTypeRef("float"),
          },
      },
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_aarch64_variadic_hfa_call_uses_fp_lanes() {
  auto result = try_lower_to_bir_with_options(
      make_aarch64_variadic_hfa_fp_lane_call_module(), BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("AArch64 variadic HFA call fixture should lower scalar FP ABI lanes");
  }

  for (const auto& function : result.module->functions) {
    if (function.name != "aarch64_variadic_hfa_fp_lane_call" ||
        function.blocks.empty()) {
      continue;
    }
    for (const auto& inst : function.blocks.front().insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr) continue;
      if (!call->is_indirect && call->callee == "semantic_variadic_hfa_sink" &&
          call->is_variadic &&
          call->arg_types.size() == 3 &&
          call->arg_types[0] == TypeKind::Ptr &&
          call->arg_types[1] == TypeKind::F32 &&
          call->arg_types[2] == TypeKind::F32 &&
          call->arg_abi.size() == 3 &&
          !call->arg_abi[1].byval_copy &&
          !call->arg_abi[2].byval_copy &&
          call->arg_abi[1].primary_class == c4c::backend::bir::AbiValueClass::Sse &&
          call->arg_abi[2].primary_class == c4c::backend::bir::AbiValueClass::Sse) {
        return 0;
      }
    }
  }
  return fail("AArch64 variadic HFA call should be represented as FP lanes");
}

LirModule make_direct_call_symbol_identity_boundary_module(bool structured_metadata,
                                                           c4c::LinkNameId override_callee_id) {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  const c4c::LinkNameId callee_id = module.link_names.intern("semantic_direct_boundary_sink");

  c4c::codegen::lir::LirExternDecl callee;
  callee.name = "semantic_direct_boundary_sink";
  callee.link_name_id = callee_id;
  callee.return_type_str = "void";
  callee.return_type = lir::LirTypeRef("void");
  module.extern_decls.push_back(std::move(callee));

  LirFunction function;
  function.name = structured_metadata ? "direct_call_missing_link_name_id"
                                      : "raw_direct_call_without_metadata";
  function.signature_text = std::string("define void @") + function.name + "()";

  LirBlock entry;
  entry.label = "entry";
  LirCallOp call{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@semantic_direct_boundary_sink"),
      .direct_callee_link_name_id = override_callee_id,
      .callee_type_suffix = "()",
      .args_str = "",
  };
  if (structured_metadata) {
    call.callee_signature = void_call_signature({});
  }
  entry.insts.push_back(std::move(call));
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_metadata_rich_direct_call_without_link_name_id_fails_closed() {
  auto result = try_lower_to_bir_with_options(
      make_direct_call_symbol_identity_boundary_module(true, c4c::kInvalidLinkName),
      BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("metadata-rich direct call without direct callee LinkNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in semantic call family 'direct-call semantic family'")) {
    return fail("missing direct-call family failure for missing direct callee LinkNameId");
  }
  return 0;
}

int expect_metadata_rich_direct_call_with_unknown_link_name_id_fails_closed() {
  LirModule module =
      make_direct_call_symbol_identity_boundary_module(true, c4c::kInvalidLinkName);
  const c4c::LinkNameId stale_callee_id =
      module.link_names.intern("semantic_missing_direct_boundary_sink");
  auto* call =
      std::get_if<LirCallOp>(&module.functions.front().blocks.front().insts.front());
  if (call == nullptr) {
    return fail("direct-call symbol identity fixture lost its call instruction");
  }
  call->direct_callee_link_name_id = stale_callee_id;

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("metadata-rich direct call with unknown LinkNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in semantic call family 'direct-call semantic family'")) {
    return fail("missing direct-call family failure for unknown direct callee LinkNameId");
  }
  return 0;
}

int expect_raw_direct_call_without_metadata_still_lowers() {
  auto result = try_lower_to_bir_with_options(
      make_direct_call_symbol_identity_boundary_module(false, c4c::kInvalidLinkName),
      BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("raw direct call without structured metadata should still lower");
  }
  return 0;
}

int expect_metadata_rich_direct_call_without_signature_fails_closed() {
  auto result = try_lower_to_bir_with_options(
      make_direct_structured_call_signature_module(false), BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("metadata-rich direct call without callee signature should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in semantic call family 'direct-call semantic family'")) {
    return fail("missing direct-call family failure for direct call without callee signature");
  }
  return 0;
}

LirModule make_indirect_structured_call_signature_module(bool matching_structured_signature) {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  c4c::TypeSpec ptr_type{};
  ptr_type.base = c4c::TB_VOID;
  ptr_type.ptr_level = 1;
  c4c::TypeSpec int_type{};
  int_type.base = c4c::TB_INT;

  LirFunction function;
  function.name = matching_structured_signature
                      ? "indirect_call_prefers_structured_signature"
                      : "indirect_call_rejects_structured_signature_mismatch";
  function.signature_text =
      matching_structured_signature
          ? "define void @indirect_call_prefers_structured_signature(ptr %fp, ptr %arg)"
          : "define void @indirect_call_rejects_structured_signature_mismatch(ptr %fp, i32 %x)";
  function.params.push_back({"%fp", ptr_type});
  function.params.push_back(
      matching_structured_signature ? std::pair<std::string, c4c::TypeSpec>{"%arg", ptr_type}
                                    : std::pair<std::string, c4c::TypeSpec>{"%x", int_type});

  lir::LirCallSignature callee_signature;
  callee_signature.return_type_ref = lir::LirTypeRef("void");
  callee_signature.fixed_param_types.push_back("ptr");
  callee_signature.fixed_param_type_refs.push_back(lir::LirTypeRef("ptr"));

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("%fp"),
      .callee_type_suffix = "(i32)",
      .args_str = matching_structured_signature ? "i32 7" : "ptr null",
      .callee_signature = std::move(callee_signature),
      .structured_args = {
          lir::LirCallArg{
              .type = matching_structured_signature ? "ptr" : "i32",
              .operand = matching_structured_signature ? LirOperand("%arg") : LirOperand("%x"),
              .type_ref = matching_structured_signature ? lir::LirTypeRef("ptr")
                                                        : lir::LirTypeRef("i32"),
          },
      },
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_indirect_call_prefers_structured_callee_signature_over_stale_suffix() {
  auto result = try_lower_to_bir_with_options(
      make_indirect_structured_call_signature_module(true), BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("indirect call should lower using structured callee signature despite stale suffix");
  }

  for (const auto& function : result.module->functions) {
    if (function.name != "indirect_call_prefers_structured_signature" ||
        function.blocks.empty()) {
      continue;
    }
    for (const auto& inst : function.blocks.front().insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr) continue;
      if (call->is_indirect && call->arg_types.size() == 1 &&
          call->arg_types.front() == TypeKind::Ptr) {
        return 0;
      }
    }
  }
  return fail("indirect call should preserve structured pointer parameter type in BIR");
}

int expect_indirect_call_signature_mismatch_fails_despite_stale_suffix_match() {
  auto result = try_lower_to_bir_with_options(
      make_indirect_structured_call_signature_module(false), BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("indirect call with structured callee signature mismatch should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in semantic call family 'indirect-call semantic family'")) {
    return fail("missing indirect-call family failure for structured callee signature mismatch");
  }
  return 0;
}

LirModule make_no_signature_structured_fabs_intrinsic_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  c4c::TypeSpec double_type{};
  double_type.base = c4c::TB_DOUBLE;

  LirFunction function;
  function.name = "no_signature_structured_fabs_intrinsic";
  function.signature_text = "define void @no_signature_structured_fabs_intrinsic(double %x)";
  function.params.push_back({"%x", double_type});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand("%abs"),
      .return_type = "double",
      .callee = LirOperand("@llvm.fabs.double"),
      .callee_type_suffix = "",
      .args_str = "i32 7",
      .structured_args = {
          lir::LirCallArg{
              .type = "double",
              .operand = LirOperand("%x"),
              .type_ref = lir::LirTypeRef("double"),
          },
      },
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_no_signature_structured_fabs_intrinsic_infers_arg_type() {
  auto result = try_lower_to_bir_with_options(
      make_no_signature_structured_fabs_intrinsic_module(), BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("no-signature structured fabs intrinsic should infer parameter facts from structured args");
  }

  for (const auto& function : result.module->functions) {
    if (function.name != "no_signature_structured_fabs_intrinsic" ||
        function.blocks.empty()) {
      continue;
    }
    for (const auto& inst : function.blocks.front().insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr) continue;
      if (call->callee == "llvm.fabs.double" && call->arg_types.size() == 1 &&
          call->arg_types.front() == TypeKind::F64 && call->return_type == TypeKind::F64) {
        return 0;
      }
    }
  }
  return fail("no-signature structured fabs intrinsic should materialize F64 BIR call facts");
}

LirModule make_aarch64_crc32w_intrinsic_module(std::string return_type = "i32",
                                               std::string data_type = "i32") {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  c4c::TypeSpec int_type{};
  int_type.base = c4c::TB_INT;

  LirFunction function;
  function.name = "aarch64_crc32w_intrinsic";
  function.signature_text = "define void @aarch64_crc32w_intrinsic(i32 %acc, i32 %data)";
  function.params.push_back({"%acc", int_type});
  function.params.push_back({"%data", int_type});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand("%crc"),
      .return_type = return_type,
      .callee = LirOperand("@llvm.aarch64.crc32w"),
      .callee_type_suffix = "(i32, " + data_type + ")",
      .args_str = "i32 %acc, " + data_type + " %data",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_aarch64_vector_load_intrinsic_module(std::string return_type = "<16 x i8>",
                                                    std::string pointer_type = "ptr") {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  c4c::TypeSpec ptr_type{};
  ptr_type.base = c4c::TB_VOID;
  ptr_type.ptr_level = 1;

  LirFunction function;
  function.name = "aarch64_vector_load_intrinsic";
  function.signature_text = "define void @aarch64_vector_load_intrinsic(ptr %p)";
  function.params.push_back({"%p", ptr_type});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand("%vec"),
      .return_type = return_type,
      .callee = LirOperand("@llvm.aarch64.neon.ld1.v16i8.p0i8"),
      .callee_type_suffix = "(" + pointer_type + ")",
      .args_str = pointer_type + " %p",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_aarch64_vector_add_intrinsic_module(std::string vector_type = "<16 x i8>") {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  c4c::TypeSpec ptr_type{};
  ptr_type.base = c4c::TB_VOID;
  ptr_type.ptr_level = 1;

  LirFunction function;
  function.name = "aarch64_vector_add_intrinsic";
  function.signature_text = "define void @aarch64_vector_add_intrinsic(ptr %p, ptr %q)";
  function.params.push_back({"%p", ptr_type});
  function.params.push_back({"%q", ptr_type});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand("%lhs"),
      .return_type = "<16 x i8>",
      .callee = LirOperand("@llvm.aarch64.neon.ld1.v16i8.p0i8"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %p",
  });
  entry.insts.push_back(LirCallOp{
      .result = LirOperand("%rhs"),
      .return_type = "<16 x i8>",
      .callee = LirOperand("@llvm.aarch64.neon.ld1.v16i8.p0i8"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %q",
  });
  entry.insts.push_back(LirCallOp{
      .result = LirOperand("%sum"),
      .return_type = vector_type,
      .callee = LirOperand("@llvm.aarch64.neon.add.v16i8"),
      .callee_type_suffix = "(" + vector_type + ", " + vector_type + ")",
      .args_str = vector_type + " %lhs, " + vector_type + " %rhs",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_aarch64_dmb_intrinsic_module(
    std::string target_triple = "aarch64-unknown-linux-gnu",
    std::string domain_type = "i32",
    std::string domain_value = "15",
    std::string return_type = "void",
    LirOperand result = LirOperand()) {
  LirModule module;
  module.target_profile = target_triple == "x86_64-unknown-linux-gnu"
                              ? c4c::default_target_profile(c4c::TargetArch::X86_64)
                              : c4c::target_profile_from_triple(target_triple);

  LirFunction function;
  function.name = "aarch64_dmb_intrinsic";
  function.signature_text = "define void @aarch64_dmb_intrinsic()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = std::move(result),
      .return_type = return_type,
      .callee = LirOperand("@llvm.aarch64.dmb"),
      .callee_type_suffix = "(" + domain_type + ")",
      .args_str = domain_type + " " + domain_value,
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_aarch64_dc_cvau_intrinsic_module(
    std::string target_triple = "aarch64-unknown-linux-gnu",
    std::string pointer_type = "ptr",
    std::string return_type = "void",
    LirOperand result = LirOperand()) {
  LirModule module;
  module.target_profile = target_triple == "x86_64-unknown-linux-gnu"
                              ? c4c::default_target_profile(c4c::TargetArch::X86_64)
                              : c4c::target_profile_from_triple(target_triple);

  c4c::TypeSpec ptr_type{};
  ptr_type.base = c4c::TB_VOID;
  ptr_type.ptr_level = 1;

  LirFunction function;
  function.name = "aarch64_dc_cvau_intrinsic";
  function.signature_text = "define void @aarch64_dc_cvau_intrinsic(ptr %p)";
  function.params.push_back({"%p", ptr_type});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = std::move(result),
      .return_type = return_type,
      .callee = LirOperand("@llvm.aarch64.dc.cvau"),
      .callee_type_suffix = "(" + pointer_type + ")",
      .args_str = pointer_type + " %p",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_aarch64_hint_intrinsic_module(
    std::string target_triple = "aarch64-unknown-linux-gnu",
    std::string hint_type = "i32",
    std::string hint_value = "1",
    std::string return_type = "void",
    LirOperand result = LirOperand()) {
  LirModule module;
  module.target_profile = target_triple == "x86_64-unknown-linux-gnu"
                              ? c4c::default_target_profile(c4c::TargetArch::X86_64)
                              : c4c::target_profile_from_triple(target_triple);

  LirFunction function;
  function.name = "aarch64_hint_intrinsic";
  function.signature_text = "define void @aarch64_hint_intrinsic()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = std::move(result),
      .return_type = return_type,
      .callee = LirOperand("@llvm.aarch64.hint"),
      .callee_type_suffix = "(" + hint_type + ")",
      .args_str = hint_type + " " + hint_value,
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_x86_only_intrinsic_on_aarch64_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  LirFunction function;
  function.name = "x86_only_intrinsic_on_aarch64";
  function.signature_text = "define void @x86_only_intrinsic_on_aarch64()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand("%x"),
      .return_type = "i32",
      .callee = LirOperand("@llvm.x86.aesni.aesenc"),
      .callee_type_suffix = "(i32, i32)",
      .args_str = "i32 1, i32 2",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

const c4c::backend::bir::CallInst* find_intrinsic_call(
    const c4c::backend::bir::Module& module,
    c4c::backend::bir::IntrinsicOperationKind operation) {
  for (const auto& function : module.functions) {
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
        if (call != nullptr && call->intrinsic.has_value() &&
            call->intrinsic->operation == operation) {
          return call;
        }
      }
    }
  }
  return nullptr;
}

int expect_aarch64_crc32w_intrinsic_carries_bir_semantics() {
  auto result = try_lower_to_bir_with_options(make_aarch64_crc32w_intrinsic_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("aarch64 crc32w intrinsic should lower to BIR semantic intrinsic facts");
  }
  const auto* call = find_intrinsic_call(
      *result.module, c4c::backend::bir::IntrinsicOperationKind::Crc32W);
  if (call == nullptr || !call->intrinsic.has_value()) {
    return fail("aarch64 crc32w intrinsic should carry a BIR intrinsic operation");
  }
  const auto& intrinsic = *call->intrinsic;
  if (intrinsic.family != c4c::backend::bir::IntrinsicFamilyKind::Crc ||
      intrinsic.required_feature != c4c::backend::bir::IntrinsicFeatureKind::AArch64Crc ||
      intrinsic.operand_type != TypeKind::I32 || intrinsic.result_type != TypeKind::I32 ||
      intrinsic.operand_roles.size() != 2 ||
      intrinsic.operand_roles[0] != c4c::backend::bir::IntrinsicOperandRole::Accumulator ||
      intrinsic.operand_roles[1] != c4c::backend::bir::IntrinsicOperandRole::Data ||
      intrinsic.signedness != c4c::backend::bir::IntrinsicSignedness::Unsigned ||
      intrinsic.memory_operand.has_value() ||
      intrinsic.memory_access != c4c::backend::bir::IntrinsicMemoryAccessKind::None ||
      intrinsic.has_side_effects) {
    return fail("aarch64 crc32w intrinsic should carry complete scalar CRC semantic facts");
  }
  return 0;
}

int expect_aarch64_v16i8_vector_load_intrinsic_carries_bir_semantics() {
  auto result = try_lower_to_bir_with_options(make_aarch64_vector_load_intrinsic_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("aarch64 vector load intrinsic should lower to BIR semantic intrinsic facts");
  }
  const auto* call = find_intrinsic_call(
      *result.module, c4c::backend::bir::IntrinsicOperationKind::VectorLoad);
  if (call == nullptr || !call->intrinsic.has_value()) {
    return fail("aarch64 vector load intrinsic should carry a BIR intrinsic operation");
  }
  const auto& intrinsic = *call->intrinsic;
  if (intrinsic.family != c4c::backend::bir::IntrinsicFamilyKind::VectorMemory ||
      intrinsic.required_feature != c4c::backend::bir::IntrinsicFeatureKind::AArch64Neon ||
      intrinsic.operand_type != TypeKind::Ptr || intrinsic.result_type != TypeKind::I128 ||
      intrinsic.operand_roles.size() != 1 ||
      intrinsic.operand_roles[0] != c4c::backend::bir::IntrinsicOperandRole::Pointer ||
      intrinsic.vector_element_type != TypeKind::I8 ||
      intrinsic.vector_element_width_bytes != 1 || intrinsic.vector_lane_count != 16 ||
      intrinsic.vector_total_width_bytes != 16 ||
      intrinsic.signedness != c4c::backend::bir::IntrinsicSignedness::Unsigned ||
      !intrinsic.memory_operand.has_value() ||
      intrinsic.memory_operand->base_kind !=
          c4c::backend::bir::MemoryAddress::BaseKind::PointerValue ||
      intrinsic.memory_operand->size_bytes != 16 ||
      intrinsic.memory_operand->align_bytes != 16 ||
      intrinsic.memory_operand->is_volatile ||
      intrinsic.memory_access != c4c::backend::bir::IntrinsicMemoryAccessKind::Read ||
      intrinsic.has_side_effects) {
    return fail("aarch64 vector load intrinsic should carry complete memory/vector facts");
  }
  return 0;
}

int expect_aarch64_v16i8_vector_add_intrinsic_carries_bir_semantics() {
  auto result = try_lower_to_bir_with_options(make_aarch64_vector_add_intrinsic_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("aarch64 vector add intrinsic should lower to BIR semantic intrinsic facts");
  }
  const auto* call = find_intrinsic_call(
      *result.module, c4c::backend::bir::IntrinsicOperationKind::VectorAdd);
  if (call == nullptr || !call->intrinsic.has_value()) {
    return fail("aarch64 vector add intrinsic should carry a BIR intrinsic operation");
  }
  const auto& intrinsic = *call->intrinsic;
  if (intrinsic.family != c4c::backend::bir::IntrinsicFamilyKind::VectorOperation ||
      intrinsic.required_feature != c4c::backend::bir::IntrinsicFeatureKind::AArch64Neon ||
      intrinsic.operand_type != TypeKind::I128 || intrinsic.result_type != TypeKind::I128 ||
      intrinsic.operand_roles.size() != 2 ||
      intrinsic.operand_roles[0] != c4c::backend::bir::IntrinsicOperandRole::VectorLhs ||
      intrinsic.operand_roles[1] != c4c::backend::bir::IntrinsicOperandRole::VectorRhs ||
      intrinsic.vector_element_type != TypeKind::I8 ||
      intrinsic.vector_element_width_bytes != 1 || intrinsic.vector_lane_count != 16 ||
      intrinsic.vector_total_width_bytes != 16 ||
      intrinsic.signedness != c4c::backend::bir::IntrinsicSignedness::Unsigned ||
      intrinsic.memory_operand.has_value() ||
      intrinsic.memory_access != c4c::backend::bir::IntrinsicMemoryAccessKind::None ||
      intrinsic.has_side_effects) {
    return fail("aarch64 vector add intrinsic should carry complete vector operation facts");
  }
  return 0;
}

int expect_aarch64_dmb_intrinsic_carries_bir_semantics_without_selection() {
  auto result = try_lower_to_bir_with_options(make_aarch64_dmb_intrinsic_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("aarch64 dmb intrinsic should lower to BIR semantic intrinsic facts");
  }
  const auto* call = find_intrinsic_call(
      *result.module, c4c::backend::bir::IntrinsicOperationKind::BarrierDmb);
  if (call == nullptr || !call->intrinsic.has_value()) {
    return fail("aarch64 dmb intrinsic should carry a BIR intrinsic operation");
  }
  const auto& intrinsic = *call->intrinsic;
  if (call->result.has_value() || call->return_type != TypeKind::Void ||
      intrinsic.family != c4c::backend::bir::IntrinsicFamilyKind::Barrier ||
      intrinsic.required_feature != c4c::backend::bir::IntrinsicFeatureKind::None ||
      intrinsic.operand_type != TypeKind::I32 || intrinsic.result_type != TypeKind::Void ||
      intrinsic.operand_roles.size() != 1 ||
      intrinsic.operand_roles[0] != c4c::backend::bir::IntrinsicOperandRole::BarrierDomain ||
      intrinsic.barrier_domain != c4c::backend::bir::IntrinsicBarrierDomainKind::Sy ||
      !intrinsic.has_immediate_operand || !intrinsic.requires_immediate_operand ||
      !intrinsic.immediate_value.has_value() || *intrinsic.immediate_value != 15 ||
      intrinsic.memory_operand.has_value() ||
      intrinsic.memory_access != c4c::backend::bir::IntrinsicMemoryAccessKind::None ||
      !intrinsic.has_side_effects) {
    return fail("aarch64 dmb intrinsic should carry complete no-result barrier facts");
  }
  return 0;
}

int expect_aarch64_dc_cvau_intrinsic_carries_cache_semantics_without_selection() {
  auto result = try_lower_to_bir_with_options(make_aarch64_dc_cvau_intrinsic_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("aarch64 dc cvau intrinsic should lower to BIR semantic intrinsic facts");
  }
  const auto* call = find_intrinsic_call(
      *result.module, c4c::backend::bir::IntrinsicOperationKind::CacheDcCvau);
  if (call == nullptr || !call->intrinsic.has_value()) {
    return fail("aarch64 dc cvau intrinsic should carry a BIR intrinsic operation");
  }
  const auto& intrinsic = *call->intrinsic;
  if (call->result.has_value() || call->return_type != TypeKind::Void ||
      intrinsic.family != c4c::backend::bir::IntrinsicFamilyKind::CacheMaintenance ||
      intrinsic.required_feature != c4c::backend::bir::IntrinsicFeatureKind::None ||
      intrinsic.operand_type != TypeKind::Ptr || intrinsic.result_type != TypeKind::Void ||
      intrinsic.operand_roles.size() != 1 ||
      intrinsic.operand_roles[0] != c4c::backend::bir::IntrinsicOperandRole::CacheAddress ||
      !intrinsic.memory_operand.has_value() ||
      intrinsic.memory_operand->base_kind !=
          c4c::backend::bir::MemoryAddress::BaseKind::PointerValue ||
      intrinsic.memory_operand->address_space != c4c::backend::bir::AddressSpace::Default ||
      intrinsic.memory_operand->size_bytes != 0 ||
      intrinsic.memory_operand->align_bytes != 1 ||
      intrinsic.memory_operand->is_volatile ||
      intrinsic.memory_access != c4c::backend::bir::IntrinsicMemoryAccessKind::None ||
      intrinsic.has_immediate_operand || intrinsic.requires_immediate_operand ||
      intrinsic.immediate_value.has_value() ||
      !intrinsic.has_side_effects) {
    return fail("aarch64 dc cvau intrinsic should carry complete cache address facts");
  }
  return 0;
}

int expect_aarch64_hint_intrinsic_carries_pause_hint_semantics_without_selection() {
  auto result = try_lower_to_bir_with_options(make_aarch64_hint_intrinsic_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("aarch64 hint intrinsic should lower to BIR semantic intrinsic facts");
  }
  const auto* call = find_intrinsic_call(
      *result.module, c4c::backend::bir::IntrinsicOperationKind::HintYield);
  if (call == nullptr || !call->intrinsic.has_value()) {
    return fail("aarch64 hint intrinsic should carry a BIR intrinsic operation");
  }
  const auto& intrinsic = *call->intrinsic;
  if (call->result.has_value() || call->return_type != TypeKind::Void ||
      intrinsic.family != c4c::backend::bir::IntrinsicFamilyKind::PauseHint ||
      intrinsic.required_feature != c4c::backend::bir::IntrinsicFeatureKind::None ||
      intrinsic.operand_type != TypeKind::I32 || intrinsic.result_type != TypeKind::Void ||
      intrinsic.operand_roles.size() != 1 ||
      intrinsic.operand_roles[0] != c4c::backend::bir::IntrinsicOperandRole::HintImmediate ||
      intrinsic.memory_operand.has_value() ||
      intrinsic.memory_access != c4c::backend::bir::IntrinsicMemoryAccessKind::None ||
      intrinsic.barrier_domain != c4c::backend::bir::IntrinsicBarrierDomainKind::None ||
      !intrinsic.has_immediate_operand || !intrinsic.requires_immediate_operand ||
      !intrinsic.immediate_value.has_value() || *intrinsic.immediate_value != 1 ||
      !intrinsic.has_side_effects) {
    return fail("aarch64 hint intrinsic should carry complete pause/hint facts");
  }
  return 0;
}

int expect_aarch64_semantic_intrinsic_fail_closed_cases() {
  constexpr std::string_view kFamily = "aarch64 semantic intrinsic family";
  const std::array<std::pair<std::string_view, LirModule>, 13> modules = {
      std::pair<std::string_view, LirModule>{"x86-only intrinsic",
                                             make_x86_only_intrinsic_on_aarch64_module()},
      std::pair<std::string_view, LirModule>{"crc wrong data type",
                                             make_aarch64_crc32w_intrinsic_module("i32", "i64")},
      std::pair<std::string_view, LirModule>{
          "vector load missing pointer",
          make_aarch64_vector_load_intrinsic_module("<16 x i8>", "i32")},
      std::pair<std::string_view, LirModule>{"vector add lane mismatch",
                                             make_aarch64_vector_add_intrinsic_module("<8 x i8>")},
      std::pair<std::string_view, LirModule>{"dmb target mismatch",
                                             make_aarch64_dmb_intrinsic_module(
                                                 "x86_64-unknown-linux-gnu")},
      std::pair<std::string_view, LirModule>{"dmb unsupported domain",
                                             make_aarch64_dmb_intrinsic_module(
                                                 "aarch64-unknown-linux-gnu", "i32", "14")},
      std::pair<std::string_view, LirModule>{"dmb non-immediate domain",
                                             make_aarch64_dmb_intrinsic_module(
                                                 "aarch64-unknown-linux-gnu", "i32", "%domain")},
      std::pair<std::string_view, LirModule>{"dc cvau target mismatch",
                                             make_aarch64_dc_cvau_intrinsic_module(
                                                 "x86_64-unknown-linux-gnu")},
      std::pair<std::string_view, LirModule>{"dc cvau missing pointer",
                                             make_aarch64_dc_cvau_intrinsic_module(
                                                 "aarch64-unknown-linux-gnu", "i32")},
      std::pair<std::string_view, LirModule>{"dc cvau unexpected result",
                                             make_aarch64_dc_cvau_intrinsic_module(
                                                 "aarch64-unknown-linux-gnu",
                                                 "ptr",
                                                 "ptr",
                                                 LirOperand("%cache"))},
      std::pair<std::string_view, LirModule>{"hint unsupported immediate",
                                             make_aarch64_hint_intrinsic_module(
                                                 "aarch64-unknown-linux-gnu", "i32", "0")},
      std::pair<std::string_view, LirModule>{"hint non-immediate",
                                             make_aarch64_hint_intrinsic_module(
                                                 "aarch64-unknown-linux-gnu", "i32", "%hint")},
      std::pair<std::string_view, LirModule>{"hint unexpected result",
                                             make_aarch64_hint_intrinsic_module(
                                                 "aarch64-unknown-linux-gnu",
                                                 "i32",
                                                 "1",
                                                 "i32",
                                                 LirOperand("%hint"))},
  };
  for (const auto& [case_name, module] : modules) {
    auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
    if (result.module.has_value()) {
      if (case_name.rfind("hint ", 0) == 0 &&
          find_intrinsic_call(
              *result.module,
              c4c::backend::bir::IntrinsicOperationKind::HintYield) == nullptr) {
        continue;
      }
      return fail((std::string("malformed or unsupported aarch64 semantic intrinsic case should "
                               "fail closed: ") +
                   std::string(case_name))
                      .c_str());
    }
    if (!contains_note(result.notes, "function", kFamily)) {
      return fail("missing aarch64 semantic intrinsic fail-closed note");
    }
  }
  return 0;
}

LirModule make_incoming_byval_param_boundary_module(
    lir::LirTypeRef signature_param_type_ref,
    std::string_view target_triple = "aarch64-unknown-linux-gnu") {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple(target_triple);
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.Payload = type { i32, i32 }");

  const c4c::StructNameId payload_id = module.struct_names.intern("%struct.Payload");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });

  LirFunction function;
  function.name = "incoming_byval_param_boundary";
  function.signature_text =
      "define void @incoming_byval_param_boundary(ptr byval(%struct.Payload) %payload)";
  c4c::TypeSpec payload_param_type{.base = c4c::TB_STRUCT};
  payload_param_type.tag_text_id = module.link_name_texts->intern("Payload");
  function.params.push_back({"%payload", payload_param_type});
  function.signature_params.push_back(
      lir::LirSignatureParam{.name = "%payload", .type = payload_param_type, .is_byval = true});
  function.signature_param_type_refs.push_back(std::move(signature_param_type_ref));

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_legacy_incoming_byval_param_boundary_module() {
  LirModule module = make_incoming_byval_param_boundary_module(lir::LirTypeRef(""));
  module.functions.front().signature_params.clear();
  module.functions.front().signature_param_type_refs.clear();
  return module;
}

int expect_structured_incoming_byval_param_materializes_from_type_ref() {
  LirModule module = make_incoming_byval_param_boundary_module(lir::LirTypeRef(""));
  const c4c::StructNameId payload_id = module.struct_names.find("%struct.Payload");
  module.functions.front().signature_param_type_refs.front() =
      lir::LirTypeRef::struct_type("ptr byval(%struct.Payload) align 8", payload_id);
  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("metadata-rich incoming byval parameter should lower through StructNameId");
  }
  if (result.module->functions.size() != 1) {
    return fail("incoming byval parameter fixture should lower one function");
  }

  const auto& function = result.module->functions.front();
  if (function.params.size() != 1 || !function.params.front().is_byval ||
      function.params.front().size_bytes != 8 || function.params.front().align_bytes != 4) {
    return fail("incoming byval parameter ABI metadata did not use structured layout");
  }

  bool saw_offset_zero = false;
  bool saw_offset_four = false;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
      if (load == nullptr || !load->address.has_value() ||
          load->address->base_kind !=
              c4c::backend::bir::MemoryAddress::BaseKind::PointerValue ||
          load->address->base_value.name != "%payload") {
        continue;
      }
      if (load->address->byte_offset == 0 && load->address->size_bytes == 4) {
        saw_offset_zero = true;
      }
      if (load->address->byte_offset == 4 && load->address->size_bytes == 4) {
        saw_offset_four = true;
      }
    }
  }
  if (!saw_offset_zero || !saw_offset_four) {
    return fail("incoming byval parameter materialization did not copy structured leaf slots");
  }
  return 0;
}

int expect_incoming_byval_param_with_missing_struct_layout_fails_closed() {
  LirModule module = make_incoming_byval_param_boundary_module(lir::LirTypeRef(""));
  const c4c::StructNameId payload_id = module.struct_names.find("%struct.Payload");
  module.functions.front().signature_param_type_refs.front() =
      lir::LirTypeRef::struct_type("ptr byval(%struct.Payload)", payload_id);
  module.struct_decls.clear();
  module.struct_decl_index.clear();

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("ID-bearing incoming byval parameter with missing structured layout should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in function-signature semantic family")) {
    return fail("missing function-signature failure for incoming byval parameter without structured layout");
  }
  return 0;
}

int expect_incoming_byval_param_with_mismatched_struct_id_fails_closed() {
  LirModule module = make_incoming_byval_param_boundary_module(lir::LirTypeRef(""));
  module.type_decls.push_back("%struct.OtherPayload = type { i32, i32 }");
  const c4c::StructNameId other_payload_id =
      module.struct_names.intern("%struct.OtherPayload");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = other_payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });
  module.functions.front().signature_param_type_refs.front() =
      lir::LirTypeRef::struct_type("ptr byval(%struct.Payload)", other_payload_id);

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("ID-bearing incoming byval parameter with mismatched StructNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in function-signature semantic family")) {
    return fail("missing function-signature failure for incoming byval parameter with mismatched StructNameId");
  }
  return 0;
}

int expect_incoming_byval_param_with_opaque_struct_layout_fails_closed() {
  LirModule module = make_incoming_byval_param_boundary_module(lir::LirTypeRef(""));
  const c4c::StructNameId payload_id = module.struct_names.find("%struct.Payload");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = payload_id,
      .is_opaque = true,
  });
  module.functions.front().signature_param_type_refs.front() =
      lir::LirTypeRef::struct_type("ptr byval(%struct.Payload)", payload_id);

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("ID-bearing incoming byval parameter with opaque structured layout should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in function-signature semantic family")) {
    return fail("missing function-signature failure for incoming byval parameter with opaque structured layout");
  }
  return 0;
}

int expect_metadata_rich_incoming_byval_param_without_struct_id_fails_closed() {
  auto result = try_lower_to_bir_with_options(
      make_incoming_byval_param_boundary_module(lir::LirTypeRef("ptr byval(%struct.Payload)")),
      BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("metadata-rich incoming byval parameter without StructNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in function-signature semantic family")) {
    return fail("missing function-signature failure for incoming byval parameter without StructNameId");
  }
  return 0;
}

int expect_metadata_rich_incoming_byval_param_without_byval_flag_fails_closed() {
  LirModule module = make_incoming_byval_param_boundary_module(lir::LirTypeRef(""));
  const c4c::StructNameId payload_id = module.struct_names.find("%struct.Payload");
  module.functions.front().signature_param_type_refs.front() =
      lir::LirTypeRef::struct_type("ptr byval(%struct.Payload)", payload_id);
  module.functions.front().signature_params.front().is_byval = false;

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("stale signature_text byval should not override structured byval metadata");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in local-memory semantic family")) {
    return fail("missing local-memory failure for incoming byval parameter without byval flag");
  }
  return 0;
}

int expect_non_aarch64_metadata_rich_incoming_byval_param_without_struct_id_uses_legacy_layout() {
  auto result = try_lower_to_bir_with_options(
      make_incoming_byval_param_boundary_module(lir::LirTypeRef("ptr byval(%struct.Payload)"),
                                                "x86_64-unknown-linux-gnu"),
      BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("non-AArch64 incoming byval parameter without StructNameId should keep legacy layout compatibility");
  }
  if (result.module->functions.empty() || result.module->functions.front().params.empty() ||
      !result.module->functions.front().params.front().is_byval ||
      result.module->functions.front().params.front().size_bytes != 8) {
    return fail("non-AArch64 metadata-rich incoming byval parameter did not use legacy layout");
  }
  return 0;
}

int expect_legacy_incoming_byval_param_without_signature_type_ref_uses_legacy_layout() {
  auto result = try_lower_to_bir_with_options(make_legacy_incoming_byval_param_boundary_module(),
                                              BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("legacy incoming byval parameter without signature refs should use legacy text layout");
  }
  if (result.module->functions.empty() || result.module->functions.front().params.empty() ||
      !result.module->functions.front().params.front().is_byval ||
      result.module->functions.front().params.front().size_bytes != 8) {
    return fail("legacy incoming byval parameter did not lower through legacy layout");
  }
  return 0;
}

LirModule make_mixed_scalar_and_aggregate_param_materialization_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.Payload = type { i32, i32 }");

  const c4c::StructNameId payload_id = module.struct_names.intern("%struct.Payload");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });

  c4c::TypeSpec float_type{};
  float_type.base = c4c::TB_FLOAT;
  c4c::TypeSpec payload_type{};
  payload_type.base = c4c::TB_STRUCT;
  payload_type.tag_text_id = module.link_name_texts->intern("Payload");

  LirFunction function;
  function.name = "mixed_scalar_and_aggregate_param_materialization";
  function.signature_text =
      "define void @mixed_scalar_and_aggregate_param_materialization(float %lane, %struct.Payload %payload)";
  function.signature_params.push_back(
      lir::LirSignatureParam{.name = "%lane", .type = float_type, .is_byval = false});
  function.signature_param_type_refs.push_back(lir::LirTypeRef("float"));
  function.signature_params.push_back(
      lir::LirSignatureParam{.name = "%payload", .type = payload_type, .is_byval = false});
  function.signature_param_type_refs.push_back(
      lir::LirTypeRef::struct_type("%struct.Payload", payload_id));

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.payload"),
      .type_str = "%struct.Payload",
      .align = 4,
  });
  entry.insts.push_back(LirStoreOp{
      .type_str = "%struct.Payload",
      .val = LirOperand("%payload"),
      .ptr = LirOperand("%lv.payload"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_mixed_scalar_and_aggregate_params_materialize_late_aggregate_param() {
  auto result = try_lower_to_bir_with_options(
      make_mixed_scalar_and_aggregate_param_materialization_module(),
      BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("mixed scalar plus aggregate params should materialize late aggregate param");
  }
  if (result.module->functions.empty() || result.module->functions.front().params.size() != 2) {
    return fail("mixed scalar plus aggregate fixture should lower both params");
  }
  const auto& function = result.module->functions.front();
  if (function.params[0].type != TypeKind::F32 ||
      function.params[1].type != TypeKind::Ptr ||
      !function.params[1].is_byval ||
      function.params[1].size_bytes != 8 ||
      function.params[1].align_bytes != 4) {
    return fail("late aggregate param after scalar lanes should keep byval ABI facts");
  }

  bool saw_offset_zero = false;
  bool saw_offset_four = false;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
      if (load == nullptr || !load->address.has_value() ||
          load->address->base_kind !=
              c4c::backend::bir::MemoryAddress::BaseKind::PointerValue ||
          load->address->base_value.name != "%payload") {
        continue;
      }
      if (load->address->byte_offset == 0 && load->address->size_bytes == 4) {
        saw_offset_zero = true;
      }
      if (load->address->byte_offset == 4 && load->address->size_bytes == 4) {
        saw_offset_four = true;
      }
    }
  }
  if (!saw_offset_zero || !saw_offset_four) {
    return fail("late aggregate param materialization should copy both structured leaves");
  }
  return 0;
}

LirModule make_aarch64_hfa_va_arg_aggregate_helper_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.Hfa = type { float }");

  const c4c::StructNameId hfa_id = module.struct_names.intern("%struct.Hfa");
  const lir::LirTypeRef hfa_ref = lir::LirTypeRef::struct_type("%struct.Hfa", hfa_id);
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = hfa_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("float")}},
  });

  LirFunction function;
  function.name = "aarch64_hfa_va_arg_aggregate_helper";
  function.signature_text = "define void @aarch64_hfa_va_arg_aggregate_helper(ptr %ap)";
  function.params.emplace_back("%ap", c4c::TypeSpec{.base = c4c::TB_VOID, .ptr_level = 1});

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirVaArgOp{
      .result = LirOperand("%hfa"),
      .ap_ptr = LirOperand("%ap"),
      .type_str = hfa_ref,
  });
  entry.insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.hfa"),
      .type_str = hfa_ref,
      .align = 4,
  });
  entry.insts.push_back(LirStoreOp{
      .type_str = hfa_ref,
      .val = LirOperand("%hfa"),
      .ptr = LirOperand("%lv.hfa"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_aarch64_hfa_va_arg_uses_aggregate_helper_handoff() {
  auto result = try_lower_to_bir_with_options(
      make_aarch64_hfa_va_arg_aggregate_helper_module(), BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("AArch64 HFA va_arg aggregate helper handoff should lower semantically");
  }
  if (result.module->functions.empty()) {
    return fail("AArch64 HFA va_arg aggregate helper fixture should lower one function");
  }

  bool saw_va_arg_aggregate = false;
  bool saw_hfa_store = false;
  const auto& function = result.module->functions.front();
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call != nullptr && call->callee == "llvm.va_arg.aggregate" &&
          call->arg_abi.size() == 2 &&
          call->arg_abi[0].sret_pointer &&
          call->arg_abi[0].size_bytes == 4 &&
          call->arg_abi[0].align_bytes == 4) {
        saw_va_arg_aggregate = true;
      }
      const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
      if (store != nullptr && store->slot_name == "%lv.hfa.0" &&
          store->value.type == TypeKind::F32) {
        saw_hfa_store = true;
      }
    }
  }
  if (!saw_va_arg_aggregate || !saw_hfa_store) {
    return fail("AArch64 HFA va_arg should use aggregate helper and remain storeable as aggregate");
  }
  return 0;
}

LirModule make_signature_return_boundary_module(lir::LirTypeRef signature_return_type_ref) {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.Payload = type { i32, i32 }");

  const c4c::StructNameId payload_id = module.struct_names.intern("%struct.Payload");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });

  LirFunction function;
  function.name = "signature_return_boundary";
  function.signature_text = "define %struct.Payload @signature_return_boundary()";
  function.signature_return_type_ref = std::move(signature_return_type_ref);

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "%struct.Payload",
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int expect_structured_signature_return_materializes_sret_from_type_ref() {
  LirModule module = make_signature_return_boundary_module(lir::LirTypeRef(""));
  const c4c::StructNameId payload_id = module.struct_names.find("%struct.Payload");
  module.functions.front().signature_return_type_ref =
      lir::LirTypeRef::struct_type("%struct.Payload", payload_id);

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!result.module.has_value()) {
    return fail("metadata-rich aggregate return should lower through StructNameId");
  }
  if (result.module->functions.empty() || result.module->functions.front().params.empty()) {
    return fail("aggregate return lowering did not materialize an sret parameter");
  }
  const auto& function = result.module->functions.front();
  if (function.return_type != TypeKind::Void || !function.params.front().is_sret ||
      function.params.front().size_bytes != 8 || function.params.front().align_bytes != 4) {
    return fail("aggregate return ABI metadata did not use structured layout");
  }
  return 0;
}

int expect_metadata_rich_signature_return_without_struct_id_fails_closed() {
  auto result = try_lower_to_bir_with_options(
      make_signature_return_boundary_module(lir::LirTypeRef("%struct.Payload")),
      BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("metadata-rich aggregate return without StructNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in function-signature semantic family")) {
    return fail("missing function-signature failure for aggregate return without StructNameId");
  }
  return 0;
}

int expect_signature_return_with_mismatched_struct_id_fails_closed() {
  LirModule module = make_signature_return_boundary_module(lir::LirTypeRef(""));
  module.type_decls.push_back("%struct.OtherPayload = type { i32, i32 }");
  const c4c::StructNameId other_payload_id = module.struct_names.intern("%struct.OtherPayload");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = other_payload_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i32")},
                 lir::LirStructField{lir::LirTypeRef("i32")}},
  });
  module.functions.front().signature_return_type_ref =
      lir::LirTypeRef::struct_type("%struct.Payload", other_payload_id);

  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return fail("aggregate return with mismatched StructNameId should fail closed");
  }
  if (!contains_note(result.notes,
                     "function",
                     "failed in function-signature semantic family")) {
    return fail("missing function-signature failure for aggregate return with mismatched StructNameId");
  }
  return 0;
}

LirModule make_bad_indirect_call_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_indirect_call";
  function.signature_text = "define void @bad_indirect_call()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("%callee"),
      .callee_type_suffix = "ptr",
      .args_str = "",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_call_return_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_call_return";
  function.signature_text = "define void @bad_call_return()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "{ i32, i32 }",
      .callee = LirOperand("@callee"),
      .callee_type_suffix = "()",
      .args_str = "",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_memcpy_runtime_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_memcpy_runtime";
  function.signature_text = "define void @bad_memcpy_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@memcpy"),
      .callee_type_suffix = "(ptr, ptr, i64)",
      .args_str = "ptr @dst, ptr @src, i64 -1",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_memset_runtime_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_memset_runtime";
  function.signature_text = "define void @bad_memset_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("@memset"),
      .callee_type_suffix = "(ptr, i8, i64)",
      .args_str = "ptr @dst, i8 7, i64 -1",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_variadic_runtime_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_variadic_runtime";
  function.signature_text = "define void @bad_variadic_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirVaArgOp{
      .result = LirOperand("@not_ssa"),
      .ap_ptr = LirOperand("%ap"),
      .type_str = "i32",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_stack_state_runtime_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_stack_state_runtime";
  function.signature_text = "define void @bad_stack_state_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStackSaveOp{
      .result = LirOperand("@not_ssa"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_abs_runtime_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_abs_runtime";
  function.signature_text = "define void @bad_abs_runtime()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirAbsOp{
      .result = LirOperand("@not_ssa"),
      .arg = LirOperand("%x"),
      .int_type = "i32",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_scalar_cast_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_scalar_cast";
  function.signature_text = "define void @bad_scalar_cast()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("@not_ssa"),
      .kind = LirCastKind::PtrToInt,
      .from_type = "ptr",
      .operand = LirOperand("@object"),
      .to_type = "i64",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_scalar_cast_lane_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "admitted_scalar_cast_lane";
  function.signature_text = "define i32 @admitted_scalar_cast_lane()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.slot.i"),
      .type_str = "i32",
      .count = LirOperand(""),
      .align = 4,
  });
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.slot.p"),
      .type_str = "ptr",
      .count = LirOperand(""),
      .align = 8,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{
      .type_str = "i32",
      .val = LirOperand("5"),
      .ptr = LirOperand("%lv.slot.i"),
  });
  entry.insts.push_back(LirStoreOp{
      .type_str = "ptr",
      .val = LirOperand("null"),
      .ptr = LirOperand("%lv.slot.p"),
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t0"),
      .type_str = "i32",
      .ptr = LirOperand("%lv.slot.i"),
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t1"),
      .kind = LirCastKind::SIToFP,
      .from_type = "i32",
      .operand = LirOperand("%t0"),
      .to_type = "float",
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t2"),
      .kind = LirCastKind::FPExt,
      .from_type = "float",
      .operand = LirOperand("%t1"),
      .to_type = "double",
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t3"),
      .kind = LirCastKind::FPTrunc,
      .from_type = "double",
      .operand = LirOperand("%t2"),
      .to_type = "float",
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t4"),
      .kind = LirCastKind::FPToSI,
      .from_type = "float",
      .operand = LirOperand("%t3"),
      .to_type = "i32",
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t5"),
      .type_str = "ptr",
      .ptr = LirOperand("%lv.slot.p"),
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t6"),
      .kind = LirCastKind::PtrToInt,
      .from_type = "ptr",
      .operand = LirOperand("%t5"),
      .to_type = "i32",
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t7"),
      .kind = LirCastKind::IntToPtr,
      .from_type = "i32",
      .operand = LirOperand("%t6"),
      .to_type = "ptr",
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t8"),
      .kind = LirCastKind::PtrToInt,
      .from_type = "ptr",
      .operand = LirOperand("%t7"),
      .to_type = "i32",
  });
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("%t9"),
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = "i32",
      .lhs = LirOperand("%t4"),
      .rhs = LirOperand("%t8"),
  });
  entry.terminator = LirRet{
      .value_str = std::string("%t9"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_scalar_local_memory_float_cmp_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "admitted_scalar_local_memory_float_cmp";
  function.signature_text = "define i32 @admitted_scalar_local_memory_float_cmp()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.a"),
      .type_str = "i32",
      .count = LirOperand(""),
      .align = 4,
  });
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.f"),
      .type_str = "float",
      .count = LirOperand(""),
      .align = 4,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{
      .type_str = "i32",
      .val = LirOperand("0"),
      .ptr = LirOperand("%lv.a"),
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t0"),
      .type_str = "i32",
      .ptr = LirOperand("%lv.a"),
  });
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("%t1"),
      .opcode = c4c::codegen::lir::LirBinaryOpcode::Add,
      .type_str = "i32",
      .lhs = LirOperand("%t0"),
      .rhs = LirOperand("1"),
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t2"),
      .kind = LirCastKind::SIToFP,
      .from_type = "i32",
      .operand = LirOperand("%t1"),
      .to_type = "float",
  });
  entry.insts.push_back(LirStoreOp{
      .type_str = "float",
      .val = LirOperand("%t2"),
      .ptr = LirOperand("%lv.f"),
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t3"),
      .type_str = "float",
      .ptr = LirOperand("%lv.f"),
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t4"),
      .type_str = "i32",
      .ptr = LirOperand("%lv.a"),
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t5"),
      .kind = LirCastKind::SIToFP,
      .from_type = "i32",
      .operand = LirOperand("%t4"),
      .to_type = "float",
  });
  entry.insts.push_back(LirCmpOp{
      .result = LirOperand("%t6"),
      .is_float = true,
      .predicate = "oeq",
      .type_str = "float",
      .lhs = LirOperand("%t3"),
      .rhs = LirOperand("%t5"),
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t7"),
      .kind = LirCastKind::ZExt,
      .from_type = "i1",
      .operand = LirOperand("%t6"),
      .to_type = "i32",
  });
  entry.terminator = LirRet{
      .value_str = std::string("%t7"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_float_une_compare_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "admitted_float_une_compare";
  function.signature_text = "define i32 @admitted_float_une_compare()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCmpOp{
      .result = LirOperand("%t0"),
      .is_float = true,
      .predicate = "une",
      .type_str = "double",
      .lhs = LirOperand("0x4028AE147AE147AE"),
      .rhs = LirOperand("0x404C63D70A3D70A4"),
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t1"),
      .kind = LirCastKind::ZExt,
      .from_type = "i1",
      .operand = LirOperand("%t0"),
      .to_type = "i32",
  });
  entry.terminator = LirRet{
      .value_str = std::string("%t1"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_null_indirect_call_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "admitted_null_indirect_call";
  function.signature_text = "define void @admitted_null_indirect_call()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCallOp{
      .result = LirOperand(""),
      .return_type = "void",
      .callee = LirOperand("null"),
      .callee_type_suffix = "()",
      .args_str = "",
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_alloca_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_alloca";
  function.signature_text = "define void @bad_alloca()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("@not_ssa"),
      .type_str = "i32",
      .count = LirOperand(""),
      .align = 0,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_function_signature_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_function_signature";
  function.signature_text = "define void @bad_function_signature()";
  c4c::TypeSpec int_type{};
  int_type.base = c4c::TB_INT;
  int_type.enum_underlying_base = c4c::TB_VOID;
  function.params.push_back({"%x", int_type});

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_scalar_control_flow_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_switch_label_scalar_control_flow";
  function.signature_text = "define i32 @bad_switch_label_scalar_control_flow()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirSwitch{
      .selector_name = "0",
      .selector_type = "i32",
      .default_label = "case_default",
      .cases = {{1, "case_one"}},
  };

  LirBlock case_one;
  case_one.label = "case_one";
  case_one.terminator = LirRet{
      .value_str = "i32 1",
      .type_str = "i32",
  };

  LirBlock case_default;
  case_default.label = "case_default";
  case_default.terminator = LirBr{
      .target_label = "join",
  };

  LirBlock join;
  join.label = "join";
  join.insts.push_back(LirPhiOp{
      .result = LirOperand("%merged"),
      .type_str = "i32",
      .incoming = {{"0", "case_default"}},
  });
  join.terminator = LirRet{
      .value_str = "%merged",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(case_one));
  function.blocks.push_back(std::move(case_default));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_return_global_pointer_symbol_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirGlobal global;
  global.name = "g";
  global.qualifier = "global ";
  global.llvm_type = "i32";
  global.init_text = "0";
  global.align_bytes = 4;
  module.globals.push_back(std::move(global));

  LirFunction function;
  function.name = "return_global_pointer_symbol";
  function.signature_text = "define ptr @return_global_pointer_symbol()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirCondBr{
      .cond_name = "true",
      .true_label = "left",
      .false_label = "right",
  };

  LirBlock left;
  left.label = "left";
  left.terminator = LirBr{
      .target_label = "join",
  };

  LirBlock right;
  right.label = "right";
  right.terminator = LirBr{
      .target_label = "join",
  };

  LirBlock join;
  join.label = "join";
  join.insts.push_back(LirPhiOp{
      .result = LirOperand("%merged"),
      .type_str = "ptr",
      .incoming = {
          {"@g", "left"},
          {"null", "right"},
      },
  });
  join.terminator = LirRet{
      .value_str = "%merged",
      .type_str = "ptr",
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(right));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_aggregate_phi_join_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.Pair = type { i32, i32 }");

  LirGlobal left_global;
  left_global.name = "left_pair";
  left_global.qualifier = "global ";
  left_global.llvm_type = "%struct.Pair";
  left_global.init_text = "%struct.Pair { i32 11, i32 22 }";
  left_global.align_bytes = 4;
  module.globals.push_back(std::move(left_global));

  LirGlobal right_global;
  right_global.name = "right_pair";
  right_global.qualifier = "global ";
  right_global.llvm_type = "%struct.Pair";
  right_global.init_text = "%struct.Pair { i32 33, i32 44 }";
  right_global.align_bytes = 4;
  module.globals.push_back(std::move(right_global));

  LirFunction function;
  function.name = "admitted_aggregate_phi_join";
  function.signature_text = "define i32 @admitted_aggregate_phi_join(i1 %p.pick_left)";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirCondBr{
      .cond_name = "%p.pick_left",
      .true_label = "left",
      .false_label = "right",
  };

  LirBlock left;
  left.label = "left";
  left.insts.push_back(LirLoadOp{
      .result = LirOperand("%left.value"),
      .type_str = "%struct.Pair",
      .ptr = LirOperand("@left_pair"),
  });
  left.terminator = LirBr{
      .target_label = "join",
  };

  LirBlock right;
  right.label = "right";
  right.insts.push_back(LirLoadOp{
      .result = LirOperand("%right.value"),
      .type_str = "%struct.Pair",
      .ptr = LirOperand("@right_pair"),
  });
  right.terminator = LirBr{
      .target_label = "join",
  };

  LirBlock join;
  join.label = "join";
  join.insts.push_back(LirPhiOp{
      .result = LirOperand("%merged"),
      .type_str = "%struct.Pair",
      .incoming = {
          {"%left.value", "left"},
          {"%right.value", "right"},
      },
  });
  join.insts.push_back(LirGepOp{
      .result = LirOperand("%merged.second.ptr"),
      .element_type = "%struct.Pair",
      .ptr = LirOperand("%merged"),
      .indices = {LirOperand("i32 0"), LirOperand("i32 1")},
  });
  join.insts.push_back(LirLoadOp{
      .result = LirOperand("%merged.second"),
      .type_str = "i32",
      .ptr = LirOperand("%merged.second.ptr"),
  });
  join.terminator = LirRet{
      .value_str = "%merged.second",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(right));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_scalar_float_globals_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirGlobal float_global;
  float_global.name = "gf";
  float_global.qualifier = "global ";
  float_global.llvm_type = "float";
  float_global.init_text = "0x3FF4000000000000";
  float_global.align_bytes = 4;
  module.globals.push_back(std::move(float_global));

  LirGlobal double_global;
  double_global.name = "gd";
  double_global.qualifier = "global ";
  double_global.llvm_type = "double";
  double_global.init_text = "0x4059000000000000";
  double_global.align_bytes = 8;
  module.globals.push_back(std::move(double_global));

  LirFunction function;
  function.name = "admitted_scalar_float_globals";
  function.signature_text = "define i32 @admitted_scalar_float_globals()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = "0",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_scalar_i16_globals_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirGlobal zero_global;
  zero_global.name = "gzero";
  zero_global.qualifier = "global ";
  zero_global.llvm_type = "i16";
  zero_global.init_text = "zeroinitializer";
  zero_global.align_bytes = 2;
  module.globals.push_back(std::move(zero_global));

  LirGlobal value_global;
  value_global.name = "gvalue";
  value_global.qualifier = "global ";
  value_global.llvm_type = "i16";
  value_global.init_text = "i16 17";
  value_global.align_bytes = 2;
  module.globals.push_back(std::move(value_global));

  LirFunction function;
  function.name = "admitted_scalar_i16_globals";
  function.signature_text = "define i32 @admitted_scalar_i16_globals()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = "0",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_aggregate_pointer_field_global_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.ptrpad = type { ptr, [1 x i8], [7 x i8] }");
  module.string_pool.push_back(c4c::codegen::lir::LirStringConst{
      .pool_name = "@.str0",
      .raw_bytes = "bugs\0",
      .byte_length = 5,
  });

  LirGlobal global;
  global.name = "gptrpad";
  global.qualifier = "global ";
  global.llvm_type = "%struct.ptrpad";
  global.init_text =
      "{ ptr getelementptr inbounds ([5 x i8], ptr @.str0, i64 0, i64 0), [1 x i8] [i8 99], "
      "[7 x i8] zeroinitializer }";
  global.align_bytes = 8;
  module.globals.push_back(std::move(global));

  LirFunction function;
  function.name = "admitted_aggregate_pointer_field_global";
  function.signature_text = "define i32 @admitted_aggregate_pointer_field_global()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = "0",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_aggregate_zero_sized_member_global_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.empty = type {}");
  module.type_decls.push_back("%struct.contains_empty = type { i8, %struct.empty, i8 }");

  LirGlobal global;
  global.name = "gzero_member";
  global.qualifier = "global ";
  global.llvm_type = "%struct.contains_empty";
  global.init_text = "{ i8 1, %struct.empty { }, i8 18 }";
  global.align_bytes = 1;
  module.globals.push_back(std::move(global));

  LirFunction function;
  function.name = "admitted_aggregate_zero_sized_member_global";
  function.signature_text = "define i32 @admitted_aggregate_zero_sized_member_global()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = "0",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_aggregate_string_array_field_global_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.T = type { [16 x i8], i8 }");

  LirGlobal global;
  global.name = "gstring_field";
  global.qualifier = "global ";
  global.llvm_type = "%struct.T";
  global.init_text = "{ [16 x i8] c\"hello\\00\\00\\00\\00\\00\\00\\00\\00\\00\\00\\00\", i8 42 }";
  global.align_bytes = 1;
  module.globals.push_back(std::move(global));

  LirFunction function;
  function.name = "admitted_aggregate_string_array_field_global";
  function.signature_text = "define i32 @admitted_aggregate_string_array_field_global()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = "0",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_aggregate_long_double_field_global_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.long_double_field = type { x86_fp80 }");

  LirGlobal global;
  global.name = "gld_field";
  global.qualifier = "global ";
  global.llvm_type = "%struct.long_double_field";
  global.init_text = "{ x86_fp80 0xK4003F8CCCCCCCCCCD000 }";
  global.align_bytes = 16;
  module.globals.push_back(std::move(global));

  LirFunction function;
  function.name = "admitted_aggregate_long_double_field_global";
  function.signature_text = "define i32 @admitted_aggregate_long_double_field_global()";

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = "0",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_local_memory_umbrella_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_local_memory_umbrella";
  function.signature_text = "define void @bad_local_memory_umbrella()";
  function.alloca_insts.push_back(LirPhiOp{
      .result = LirOperand("%t0"),
      .type_str = "i32",
      .incoming = {{"0", "entry"}},
  });

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_scalar_local_memory_umbrella_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_scalar_local_memory_umbrella";
  function.signature_text = "define void @bad_scalar_local_memory_umbrella()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirSelectOp{
      .result = LirOperand("%t0"),
      .type_str = "i32",
      .cond = LirOperand("%cond"),
      .true_val = LirOperand("1"),
      .false_val = LirOperand("0"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_scalar_binop_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_scalar_binop";
  function.signature_text = "define void @bad_scalar_binop()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("@not_ssa"),
      .opcode = "add",
      .type_str = "i32",
      .lhs = LirOperand("%lhs"),
      .rhs = LirOperand("%rhs"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_admitted_float_scalar_binop_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "admitted_float_scalar_binop";
  function.signature_text = "define float @admitted_float_scalar_binop()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_FLOAT};

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%lhs0"),
      .kind = LirCastKind::SIToFP,
      .from_type = "i32",
      .operand = LirOperand("3"),
      .to_type = "float",
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%rhs0"),
      .kind = LirCastKind::SIToFP,
      .from_type = "i32",
      .operand = LirOperand("4"),
      .to_type = "float",
  });
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("%t0"),
      .opcode = c4c::codegen::lir::LirBinaryOpcode::FAdd,
      .type_str = "float",
      .lhs = LirOperand("%lhs0"),
      .rhs = LirOperand("%rhs0"),
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%rhs1"),
      .kind = LirCastKind::SIToFP,
      .from_type = "i32",
      .operand = LirOperand("1"),
      .to_type = "float",
  });
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("%t1"),
      .opcode = c4c::codegen::lir::LirBinaryOpcode::FSub,
      .type_str = "float",
      .lhs = LirOperand("%t0"),
      .rhs = LirOperand("%rhs1"),
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%rhs2"),
      .kind = LirCastKind::SIToFP,
      .from_type = "i32",
      .operand = LirOperand("2"),
      .to_type = "float",
  });
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("%t2"),
      .opcode = c4c::codegen::lir::LirBinaryOpcode::FMul,
      .type_str = "float",
      .lhs = LirOperand("%t1"),
      .rhs = LirOperand("%rhs2"),
  });
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("%t3"),
      .opcode = c4c::codegen::lir::LirBinaryOpcode::FDiv,
      .type_str = "float",
      .lhs = LirOperand("%t2"),
      .rhs = LirOperand("%rhs2"),
  });
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("%t4"),
      .opcode = c4c::codegen::lir::LirBinaryOpcode::FNeg,
      .type_str = "float",
      .lhs = LirOperand("%t3"),
      .rhs = LirOperand(""),
  });
  entry.terminator = LirRet{
      .value_str = std::string("%t4"),
      .type_str = "float",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_f128_scalar_constant_binop_fails_closed_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");

  LirFunction function;
  function.name = "f128_scalar_constant_binop_fails_closed";
  function.signature_text = "define f128 @f128_scalar_constant_binop_fails_closed(f128 %lhs)";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_LONGDOUBLE};

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirBinOp{
      .result = LirOperand("%sum"),
      .opcode = c4c::codegen::lir::LirBinaryOpcode::FAdd,
      .type_str = "f128",
      .lhs = LirOperand("%lhs"),
      .rhs = LirOperand("0xL00000000000000000000000000000000"),
  });
  entry.terminator = LirRet{
      .value_str = std::string("%sum"),
      .type_str = "f128",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_gep_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_gep";
  function.signature_text = "define void @bad_gep()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("@not_ssa"),
      .element_type = "i32",
      .ptr = LirOperand("%ptr"),
      .indices = {"i32 0"},
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_local_aggregate_raw_i8_gep_byte_slice_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.s9 = type { [9 x i8] }");

  LirFunction function;
  function.name = "local_aggregate_raw_i8_gep_byte_slice";
  function.signature_text = "define void @local_aggregate_raw_i8_gep_byte_slice()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.scratch"),
      .type_str = "%struct.s9",
      .count = LirOperand(""),
      .align = 1,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t0"),
      .element_type = "i8",
      .ptr = LirOperand("%lv.scratch"),
      .indices = {"i64 8"},
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_local_aggregate_raw_float_leaf_byte_slice_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.hfa13 = type { float, float, float }");

  LirFunction function;
  function.name = "local_aggregate_raw_float_leaf_byte_slice";
  function.signature_text = "define void @local_aggregate_raw_float_leaf_byte_slice()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.scratch"),
      .type_str = "%struct.hfa13",
      .count = LirOperand(""),
      .align = 4,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t0"),
      .element_type = "i8",
      .ptr = LirOperand("%lv.scratch"),
      .indices = {"i64 8"},
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_local_aggregate_raw_float_tail_memcpy_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.hfa14 = type { float, float, float, float }");

  LirFunction function;
  function.name = "local_aggregate_raw_float_tail_memcpy";
  function.signature_text = "define void @local_aggregate_raw_float_tail_memcpy()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.dst"),
      .type_str = "%struct.hfa14",
      .count = LirOperand(""),
      .align = 4,
  });
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.src"),
      .type_str = "[2 x float]",
      .count = LirOperand(""),
      .align = 4,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t0"),
      .element_type = "i8",
      .ptr = LirOperand("%lv.dst"),
      .indices = {"i64 8"},
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t1"),
      .element_type = "[2 x float]",
      .ptr = LirOperand("%lv.src"),
      .indices = {"i64 0", "i64 0"},
  });
  entry.insts.push_back(c4c::codegen::lir::LirMemcpyOp{
      .dst = LirOperand("%t0"),
      .src = LirOperand("%t1"),
      .size = LirOperand("8"),
      .is_volatile = false,
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_local_gep_rejects_structured_mismatch_module() {
  LirModule module;
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
  module.type_decls.push_back("%struct.Pair = type { i64, i64 }");

  LirFunction function;
  function.name = "local_gep_rejects_structured_mismatch";
  function.signature_text = "define void @local_gep_rejects_structured_mismatch()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.pair"),
      .type_str = lir::LirTypeRef::struct_type("%struct.Pair", pair_id),
      .count = LirOperand(""),
      .align = 4,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%field1"),
      .element_type = lir::LirTypeRef::struct_type("%struct.Pair", pair_id),
      .ptr = LirOperand("%lv.pair"),
      .indices = {LirOperand("i32 0"), LirOperand("i32 1")},
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_local_gep_rejects_structured_opaque_legacy_fallback_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = pair_id,
      .is_opaque = true,
  });
  module.type_decls.push_back("%struct.Pair = type { i64, i64 }");

  LirFunction function;
  function.name = "local_gep_rejects_structured_opaque_legacy_fallback";
  function.signature_text =
      "define void @local_gep_rejects_structured_opaque_legacy_fallback()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.pair"),
      .type_str = lir::LirTypeRef::struct_type("%struct.Pair", pair_id),
      .count = LirOperand(""),
      .align = 8,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%field1"),
      .element_type = lir::LirTypeRef::struct_type("%struct.Pair", pair_id),
      .ptr = LirOperand("%lv.pair"),
      .indices = {LirOperand("i32 0"), LirOperand("i32 1")},
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_local_scalar_double_decimal_zero_store_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "local_scalar_double_decimal_zero_store";
  function.signature_text = "define void @local_scalar_double_decimal_zero_store()";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.scratch"),
      .type_str = "double",
      .count = LirOperand(""),
      .align = 8,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{
      .type_str = "double",
      .val = LirOperand("0.0"),
      .ptr = LirOperand("%lv.scratch"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_local_scalar_double_partial_float_memcpy_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "local_scalar_double_partial_float_memcpy";
  function.signature_text = "define void @local_scalar_double_partial_float_memcpy(ptr %p.src)";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.scratch"),
      .type_str = "double",
      .count = LirOperand(""),
      .align = 8,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{
      .type_str = "double",
      .val = LirOperand("0.0"),
      .ptr = LirOperand("%lv.scratch"),
  });
  entry.insts.push_back(c4c::codegen::lir::LirMemcpyOp{
      .dst = LirOperand("%lv.scratch"),
      .src = LirOperand("%p.src"),
      .size = LirOperand("4"),
      .is_volatile = false,
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t0"),
      .type_str = "double",
      .ptr = LirOperand("%lv.scratch"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_local_scalar_i64_partial_i8_memcpy_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "local_scalar_i64_partial_i8_memcpy";
  function.signature_text = "define void @local_scalar_i64_partial_i8_memcpy(ptr %p.src)";
  function.alloca_insts.push_back(LirAllocaOp{
      .result = LirOperand("%lv.scratch"),
      .type_str = "i64",
      .count = LirOperand(""),
      .align = 8,
  });

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{
      .type_str = "i64",
      .val = LirOperand("0"),
      .ptr = LirOperand("%lv.scratch"),
  });
  entry.insts.push_back(c4c::codegen::lir::LirMemcpyOp{
      .dst = LirOperand("%lv.scratch"),
      .src = LirOperand("%p.src"),
      .size = LirOperand("7"),
      .is_volatile = false,
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t0"),
      .type_str = "i64",
      .ptr = LirOperand("%lv.scratch"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_dynamic_indexed_gep_local_member_array_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.Pair = type { [3 x i32] }");

  LirFunction function;
  function.name = "dynamic_indexed_gep_local_member_array";
  function.signature_text =
      "define i32 @dynamic_indexed_gep_local_member_array(ptr %p.p, i32 %p.i)";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t0"),
      .element_type = "%struct.Pair",
      .ptr = LirOperand("%p.p"),
      .indices = {LirOperand("i32 0"), LirOperand("i32 0")},
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t1"),
      .kind = LirCastKind::SExt,
      .from_type = "i32",
      .operand = LirOperand("%p.i"),
      .to_type = "i64",
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t2"),
      .element_type = "[3 x i32]",
      .ptr = LirOperand("%t0"),
      .indices = {LirOperand("i64 0"), LirOperand("i64 0")},
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t3"),
      .element_type = "i32",
      .ptr = LirOperand("%t2"),
      .indices = {LirOperand("i64 %t1")},
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t4"),
      .type_str = "i32",
      .ptr = LirOperand("%t3"),
  });
  entry.terminator = LirRet{
      .value_str = std::string("%t4"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirGlobal make_dynamic_indexed_gep_global_cases() {
  LirGlobal global;
  global.name = "cases";
  global.qualifier = "global ";
  global.llvm_type = "[2 x %struct.Pair]";
  global.init_text =
      "[%struct.Pair { [3 x i64] [i64 7, i64 11, i64 13], i64 29 }, "
      "%struct.Pair { [3 x i64] [i64 17, i64 19, i64 23], i64 31 }]";
  global.align_bytes = 8;
  return global;
}

LirModule make_dynamic_indexed_gep_global_member_array_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.Pair = type { [3 x i64], i64 }");
  module.globals.push_back(make_dynamic_indexed_gep_global_cases());

  LirFunction function;
  function.name = "dynamic_indexed_gep_global_member_array";
  function.signature_text =
      "define i64 @dynamic_indexed_gep_global_member_array(i32 %p.j, i32 %p.i)";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_LONG};

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t0"),
      .element_type = "[2 x %struct.Pair]",
      .ptr = LirOperand("@cases"),
      .indices = {LirOperand("i64 0"), LirOperand("i64 0")},
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t1"),
      .kind = LirCastKind::SExt,
      .from_type = "i32",
      .operand = LirOperand("%p.j"),
      .to_type = "i64",
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t2"),
      .element_type = "%struct.Pair",
      .ptr = LirOperand("%t0"),
      .indices = {LirOperand("i64 %t1")},
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t3"),
      .element_type = "%struct.Pair",
      .ptr = LirOperand("%t2"),
      .indices = {LirOperand("i32 0"), LirOperand("i32 0")},
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t4"),
      .element_type = "[3 x i64]",
      .ptr = LirOperand("%t3"),
      .indices = {LirOperand("i64 0"), LirOperand("i64 0")},
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t5"),
      .kind = LirCastKind::SExt,
      .from_type = "i32",
      .operand = LirOperand("%p.i"),
      .to_type = "i64",
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t6"),
      .element_type = "i64",
      .ptr = LirOperand("%t4"),
      .indices = {LirOperand("i64 %t5")},
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t7"),
      .type_str = "i64",
      .ptr = LirOperand("%t6"),
  });
  entry.terminator = LirRet{
      .value_str = std::string("%t7"),
      .type_str = "i64",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_dynamic_indexed_gep_global_member_scalar_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.Pair = type { [3 x i64], i64 }");
  module.globals.push_back(make_dynamic_indexed_gep_global_cases());

  LirFunction function;
  function.name = "dynamic_indexed_gep_global_member_scalar";
  function.signature_text = "define i64 @dynamic_indexed_gep_global_member_scalar(i32 %p.j)";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_LONG};

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t0"),
      .element_type = "[2 x %struct.Pair]",
      .ptr = LirOperand("@cases"),
      .indices = {LirOperand("i64 0"), LirOperand("i64 0")},
  });
  entry.insts.push_back(LirCastOp{
      .result = LirOperand("%t1"),
      .kind = LirCastKind::SExt,
      .from_type = "i32",
      .operand = LirOperand("%p.j"),
      .to_type = "i64",
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t2"),
      .element_type = "%struct.Pair",
      .ptr = LirOperand("%t0"),
      .indices = {LirOperand("i64 %t1")},
  });
  entry.insts.push_back(LirGepOp{
      .result = LirOperand("%t3"),
      .element_type = "%struct.Pair",
      .ptr = LirOperand("%t2"),
      .indices = {LirOperand("i32 0"), LirOperand("i32 1")},
  });
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("%t4"),
      .type_str = "i64",
      .ptr = LirOperand("%t3"),
  });
  entry.terminator = LirRet{
      .value_str = std::string("%t4"),
      .type_str = "i64",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_store_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_store";
  function.signature_text = "define void @bad_store()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirStoreOp{
      .type_str = "i32",
      .val = LirOperand("%value"),
      .ptr = LirOperand(""),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_bad_load_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction function;
  function.name = "bad_load";
  function.signature_text = "define void @bad_load()";

  LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(LirLoadOp{
      .result = LirOperand("@not_ssa"),
      .type_str = "i32",
      .ptr = LirOperand("%ptr"),
  });
  entry.terminator = LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

LirModule make_structured_block_label_id_module() {
  LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  LirFunction switch_function;
  switch_function.name = "structured_block_label_ids";
  switch_function.signature_text = "define i32 @structured_block_label_ids(i32 %selector)";
  switch_function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  c4c::TypeSpec int_type{};
  int_type.base = c4c::TB_INT;
  int_type.enum_underlying_base = c4c::TB_VOID;
  switch_function.params.push_back({"%selector", int_type});

  LirBlock entry;
  entry.label = "entry";
  entry.terminator = LirSwitch{
      .selector_name = "%selector",
      .selector_type = "i32",
      .default_label = "case_default",
      .cases = {{1, "case_one"}, {2, "case_two"}},
  };

  LirBlock case_one;
  case_one.label = "case_one";
  case_one.terminator = LirBr{
      .target_label = "join",
  };

  LirBlock case_two;
  case_two.label = "case_two";
  case_two.terminator = LirBr{
      .target_label = "join",
  };

  LirBlock case_default;
  case_default.label = "case_default";
  case_default.terminator = LirBr{
      .target_label = "join",
  };

  LirBlock join;
  join.label = "join";
  join.insts.push_back(LirPhiOp{
      .result = LirOperand("%merged"),
      .type_str = "i32",
      .incoming = {
          {"11", "case_one"},
          {"22", "case_two"},
          {"33", "case_default"},
          {"44", "ghost"},
      },
  });
  join.terminator = LirRet{
      .value_str = "%merged",
      .type_str = "i32",
  };

  switch_function.blocks.push_back(std::move(entry));
  switch_function.blocks.push_back(std::move(case_one));
  switch_function.blocks.push_back(std::move(case_two));
  switch_function.blocks.push_back(std::move(case_default));
  switch_function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(switch_function));
  return module;
}

}  // namespace

int main() {
  constexpr std::string_view kModuleSummary =
      "currently admitted capability buckets covering function-signature, scalar-control-flow, scalar/local-memory (including scalar-cast/scalar-binop and alloca/gep/load/store local-memory), and local/global memory semantics, plus semantic call families (direct-call, indirect-call, and call-return) and explicit runtime or intrinsic families such as variadic, stack-state, absolute-value, memcpy, memset, and inline-asm placeholders";
  if (const int inline_asm_status = expect_failure_notes(
          "bad_inline_asm",
          make_unsupported_inline_asm_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'inline-asm placeholder family'",
          "latest function failure: semantic lir_to_bir function 'bad_inline_asm' failed in runtime/intrinsic family 'inline-asm placeholder family'",
          "missing module capability-bucket summary note",
          "missing specific runtime family function note",
          "missing module note carrying the runtime family failure");
      inline_asm_status != 0) {
    return inline_asm_status;
  }

  if (const int status = inline_asm_lir_lowering_preserves_structured_operand_metadata();
      status != 0) {
    return status;
  }

  if (const int direct_call_status = expect_failure_notes(
          "bad_direct_call",
          make_bad_direct_call_module(),
          kModuleSummary,
          "failed in semantic call family 'direct-call semantic family'",
          "latest function failure: semantic lir_to_bir function 'bad_direct_call' failed in semantic call family 'direct-call semantic family'",
          "missing module capability-bucket summary note",
          "missing specific semantic-call function note",
          "missing module note carrying the semantic-call family failure");
      direct_call_status != 0) {
    return direct_call_status;
  }

  auto link_name_aware_direct_call = make_bad_direct_call_module();
  assign_semantic_function_name(
      &link_name_aware_direct_call,
      "semantic_bad_direct_call",
      "corrupted_bad_direct_call");
  if (const int semantic_direct_call_status = expect_failure_notes(
          "semantic_bad_direct_call",
          link_name_aware_direct_call,
          kModuleSummary,
          "failed in semantic call family 'direct-call semantic family'",
          "latest function failure: semantic lir_to_bir function 'semantic_bad_direct_call' failed in semantic call family 'direct-call semantic family'",
          "missing module capability-bucket summary note for LinkNameId-backed direct-call failure",
          "missing semantic direct-call function note",
          "missing LinkNameId-backed module note carrying the semantic-call family failure");
      semantic_direct_call_status != 0) {
    return semantic_direct_call_status;
  }
  if (contains_note(try_lower_to_bir_with_options(link_name_aware_direct_call, BirLoweringOptions{}).notes,
                    "function",
                    "corrupted_bad_direct_call")) {
    return fail("backend failure notes should not trust a corrupted raw function name when LinkNameId is available");
  }

  if (const int legacy_byval_status =
          expect_legacy_byval_call_arg_without_type_refs_still_lowers();
      legacy_byval_status != 0) {
    return legacy_byval_status;
  }

  if (const int missing_byval_metadata_status =
          expect_metadata_rich_byval_call_arg_without_struct_id_fails_closed();
      missing_byval_metadata_status != 0) {
    return missing_byval_metadata_status;
  }

  if (const int mismatched_byval_metadata_status =
          expect_metadata_rich_byval_call_arg_mismatch_fails_closed();
      mismatched_byval_metadata_status != 0) {
    return mismatched_byval_metadata_status;
  }

  if (const int direct_structured_signature_status =
          expect_direct_call_prefers_structured_callee_signature_over_stale_suffix();
      direct_structured_signature_status != 0) {
    return direct_structured_signature_status;
  }
  if (const int direct_structured_byval_status =
          expect_direct_call_structured_byval_signature_materializes_aggregate_abi();
      direct_structured_byval_status != 0) {
    return direct_structured_byval_status;
  }
  if (const int direct_structured_byval_mismatch_status =
          expect_direct_call_structured_byval_signature_mismatch_fails_closed();
      direct_structured_byval_mismatch_status != 0) {
    return direct_structured_byval_mismatch_status;
  }
  if (const int aarch64_hfa_call_status =
          expect_aarch64_direct_hfa_call_uses_fp_lanes_not_byval();
      aarch64_hfa_call_status != 0) {
    return aarch64_hfa_call_status;
  }
  if (const int aarch64_variadic_hfa_call_status =
          expect_aarch64_variadic_hfa_call_uses_fp_lanes();
      aarch64_variadic_hfa_call_status != 0) {
    return aarch64_variadic_hfa_call_status;
  }

  if (const int missing_direct_link_name_status =
          expect_metadata_rich_direct_call_without_link_name_id_fails_closed();
      missing_direct_link_name_status != 0) {
    return missing_direct_link_name_status;
  }

  if (const int unknown_direct_link_name_status =
          expect_metadata_rich_direct_call_with_unknown_link_name_id_fails_closed();
      unknown_direct_link_name_status != 0) {
    return unknown_direct_link_name_status;
  }

  if (const int raw_direct_call_status =
          expect_raw_direct_call_without_metadata_still_lowers();
      raw_direct_call_status != 0) {
    return raw_direct_call_status;
  }

  if (const int missing_direct_signature_status =
          expect_metadata_rich_direct_call_without_signature_fails_closed();
      missing_direct_signature_status != 0) {
    return missing_direct_signature_status;
  }

  if (const int indirect_structured_signature_status =
          expect_indirect_call_prefers_structured_callee_signature_over_stale_suffix();
      indirect_structured_signature_status != 0) {
    return indirect_structured_signature_status;
  }

  if (const int indirect_structured_signature_mismatch_status =
          expect_indirect_call_signature_mismatch_fails_despite_stale_suffix_match();
      indirect_structured_signature_mismatch_status != 0) {
    return indirect_structured_signature_mismatch_status;
  }
  if (const int no_signature_structured_fabs_status =
          expect_no_signature_structured_fabs_intrinsic_infers_arg_type();
      no_signature_structured_fabs_status != 0) {
    return no_signature_structured_fabs_status;
  }
  if (const int aarch64_crc32w_status =
          expect_aarch64_crc32w_intrinsic_carries_bir_semantics();
      aarch64_crc32w_status != 0) {
    return aarch64_crc32w_status;
  }
  if (const int aarch64_vector_load_status =
          expect_aarch64_v16i8_vector_load_intrinsic_carries_bir_semantics();
      aarch64_vector_load_status != 0) {
    return aarch64_vector_load_status;
  }
  if (const int aarch64_vector_add_status =
          expect_aarch64_v16i8_vector_add_intrinsic_carries_bir_semantics();
      aarch64_vector_add_status != 0) {
    return aarch64_vector_add_status;
  }
  if (const int aarch64_dmb_status =
          expect_aarch64_dmb_intrinsic_carries_bir_semantics_without_selection();
      aarch64_dmb_status != 0) {
    return aarch64_dmb_status;
  }
  if (const int aarch64_dc_cvau_status =
          expect_aarch64_dc_cvau_intrinsic_carries_cache_semantics_without_selection();
      aarch64_dc_cvau_status != 0) {
    return aarch64_dc_cvau_status;
  }
  if (const int aarch64_hint_status =
          expect_aarch64_hint_intrinsic_carries_pause_hint_semantics_without_selection();
      aarch64_hint_status != 0) {
    return aarch64_hint_status;
  }
  if (const int aarch64_intrinsic_fail_closed_status =
          expect_aarch64_semantic_intrinsic_fail_closed_cases();
      aarch64_intrinsic_fail_closed_status != 0) {
    return aarch64_intrinsic_fail_closed_status;
  }

  if (const int structured_incoming_byval_status =
          expect_structured_incoming_byval_param_materializes_from_type_ref();
      structured_incoming_byval_status != 0) {
    return structured_incoming_byval_status;
  }

  if (const int missing_structured_incoming_byval_status =
          expect_incoming_byval_param_with_missing_struct_layout_fails_closed();
      missing_structured_incoming_byval_status != 0) {
    return missing_structured_incoming_byval_status;
  }

  if (const int mismatched_structured_incoming_byval_status =
          expect_incoming_byval_param_with_mismatched_struct_id_fails_closed();
      mismatched_structured_incoming_byval_status != 0) {
    return mismatched_structured_incoming_byval_status;
  }

  if (const int opaque_structured_incoming_byval_status =
          expect_incoming_byval_param_with_opaque_struct_layout_fails_closed();
      opaque_structured_incoming_byval_status != 0) {
    return opaque_structured_incoming_byval_status;
  }

  if (const int missing_incoming_byval_id_status =
          expect_metadata_rich_incoming_byval_param_without_struct_id_fails_closed();
      missing_incoming_byval_id_status != 0) {
    return missing_incoming_byval_id_status;
  }

  if (const int missing_incoming_byval_flag_status =
          expect_metadata_rich_incoming_byval_param_without_byval_flag_fails_closed();
      missing_incoming_byval_flag_status != 0) {
    return missing_incoming_byval_flag_status;
  }

  if (const int non_aarch64_missing_incoming_byval_id_status =
          expect_non_aarch64_metadata_rich_incoming_byval_param_without_struct_id_uses_legacy_layout();
      non_aarch64_missing_incoming_byval_id_status != 0) {
    return non_aarch64_missing_incoming_byval_id_status;
  }

  if (const int legacy_incoming_byval_status =
          expect_legacy_incoming_byval_param_without_signature_type_ref_uses_legacy_layout();
      legacy_incoming_byval_status != 0) {
    return legacy_incoming_byval_status;
  }

  if (const int mixed_scalar_aggregate_param_status =
          expect_mixed_scalar_and_aggregate_params_materialize_late_aggregate_param();
      mixed_scalar_aggregate_param_status != 0) {
    return mixed_scalar_aggregate_param_status;
  }

  if (const int hfa_va_arg_status =
          expect_aarch64_hfa_va_arg_uses_aggregate_helper_handoff();
      hfa_va_arg_status != 0) {
    return hfa_va_arg_status;
  }

  if (const int structured_signature_return_status =
          expect_structured_signature_return_materializes_sret_from_type_ref();
      structured_signature_return_status != 0) {
    return structured_signature_return_status;
  }

  if (const int missing_signature_return_id_status =
          expect_metadata_rich_signature_return_without_struct_id_fails_closed();
      missing_signature_return_id_status != 0) {
    return missing_signature_return_id_status;
  }

  if (const int mismatched_signature_return_id_status =
          expect_signature_return_with_mismatched_struct_id_fails_closed();
      mismatched_signature_return_id_status != 0) {
    return mismatched_signature_return_id_status;
  }

  if (const int indirect_call_status = expect_failure_notes(
          "bad_indirect_call",
          make_bad_indirect_call_module(),
          kModuleSummary,
          "failed in semantic call family 'indirect-call semantic family'",
          "latest function failure: semantic lir_to_bir function 'bad_indirect_call' failed in semantic call family 'indirect-call semantic family'",
          "missing module capability-bucket summary note",
          "missing specific indirect-call function note",
          "missing module note carrying the indirect-call semantic family failure");
      indirect_call_status != 0) {
    return indirect_call_status;
  }

  if (const int call_return_status = expect_failure_notes(
          "bad_call_return",
          make_bad_call_return_module(),
          kModuleSummary,
          "failed in semantic call family 'call-return semantic family'",
          "latest function failure: semantic lir_to_bir function 'bad_call_return' failed in semantic call family 'call-return semantic family'",
          "missing module capability-bucket summary note",
          "missing specific call-return function note",
          "missing module note carrying the call-return semantic family failure");
      call_return_status != 0) {
    return call_return_status;
  }

  if (const int memcpy_status = expect_failure_notes(
          "bad_memcpy_runtime",
          make_bad_memcpy_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'memcpy runtime family'",
          "latest function failure: semantic lir_to_bir function 'bad_memcpy_runtime' failed in runtime/intrinsic family 'memcpy runtime family'",
          "missing module capability-bucket summary note",
          "missing specific memcpy runtime function note",
          "missing module note carrying the memcpy runtime family failure");
      memcpy_status != 0) {
    return memcpy_status;
  }

  if (const int memset_status = expect_failure_notes(
          "bad_memset_runtime",
          make_bad_memset_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'memset runtime family'",
          "latest function failure: semantic lir_to_bir function 'bad_memset_runtime' failed in runtime/intrinsic family 'memset runtime family'",
          "missing module capability-bucket summary note",
          "missing specific memset runtime function note",
          "missing module note carrying the memset runtime family failure");
      memset_status != 0) {
    return memset_status;
  }

  if (const int variadic_status = expect_failure_notes(
          "bad_variadic_runtime",
          make_bad_variadic_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'variadic runtime family'",
          "latest function failure: semantic lir_to_bir function 'bad_variadic_runtime' failed in runtime/intrinsic family 'variadic runtime family'",
          "missing module capability-bucket summary note",
          "missing specific variadic runtime function note",
          "missing module note carrying the variadic runtime family failure");
      variadic_status != 0) {
    return variadic_status;
  }

  if (const int stack_state_status = expect_failure_notes(
          "bad_stack_state_runtime",
          make_bad_stack_state_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'stack-state runtime family'",
          "latest function failure: semantic lir_to_bir function 'bad_stack_state_runtime' failed in runtime/intrinsic family 'stack-state runtime family'",
          "missing module capability-bucket summary note",
          "missing specific stack-state runtime function note",
          "missing module note carrying the stack-state runtime family failure");
      stack_state_status != 0) {
    return stack_state_status;
  }

  if (const int abs_status = expect_failure_notes(
          "bad_abs_runtime",
          make_bad_abs_runtime_module(),
          kModuleSummary,
          "failed in runtime/intrinsic family 'absolute-value intrinsic family'",
          "latest function failure: semantic lir_to_bir function 'bad_abs_runtime' failed in runtime/intrinsic family 'absolute-value intrinsic family'",
          "missing module capability-bucket summary note",
          "missing specific absolute-value runtime function note",
          "missing module note carrying the absolute-value runtime family failure");
      abs_status != 0) {
    return abs_status;
  }

  if (const int scalar_cast_status = expect_failure_notes(
          "bad_scalar_cast",
          make_bad_scalar_cast_module(),
          kModuleSummary,
          "failed in scalar-cast semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_scalar_cast' failed in scalar-cast semantic family",
          "missing module capability-bucket summary note",
          "missing specific scalar-cast function note",
          "missing module note carrying the scalar-cast semantic family failure");
      scalar_cast_status != 0) {
    return scalar_cast_status;
  }

  if (const int admitted_scalar_cast_status = expect_success_without_function_note(
          "admitted_scalar_cast_lane",
          make_admitted_scalar_cast_lane_module(),
          "failed in scalar-cast semantic family",
          "latest function failure: semantic lir_to_bir function 'admitted_scalar_cast_lane' failed in scalar-cast semantic family",
          "memory-path scalar casts that already have BIR opcodes should not keep reporting the scalar-cast semantic family",
          "memory-path scalar casts that already have BIR opcodes should not keep the module on the scalar-cast semantic-family note");
      admitted_scalar_cast_status != 0) {
    return admitted_scalar_cast_status;
  }

  if (const int admitted_scalar_local_memory_float_cmp_status =
          expect_success_without_function_note(
              "admitted_scalar_local_memory_float_cmp",
              make_admitted_scalar_local_memory_float_cmp_module(),
              "failed in scalar/local-memory semantic family",
              "latest function failure: semantic lir_to_bir function "
              "'admitted_scalar_local_memory_float_cmp' failed in scalar/local-memory "
              "semantic family",
              "float local-memory compare lanes should not keep reporting the "
              "scalar/local-memory semantic family",
              "float local-memory compare lanes should not keep the module on the "
              "scalar/local-memory semantic-family note");
      admitted_scalar_local_memory_float_cmp_status != 0) {
    return admitted_scalar_local_memory_float_cmp_status;
  }

  if (const int admitted_float_une_compare_status = expect_success_without_function_note(
          "admitted_float_une_compare",
          make_admitted_float_une_compare_module(),
          "failed in scalar/local-memory semantic family",
          "latest function failure: semantic lir_to_bir function "
          "'admitted_float_une_compare' failed in scalar/local-memory semantic family",
          "float une compare lanes should not keep reporting the scalar/local-memory "
          "semantic family",
          "float une compare lanes should not keep the module on the "
          "scalar/local-memory semantic-family note");
      admitted_float_une_compare_status != 0) {
    return admitted_float_une_compare_status;
  }

  if (const int admitted_null_indirect_call_status = expect_success_without_function_note(
          "admitted_null_indirect_call",
          make_admitted_null_indirect_call_module(),
          "failed in scalar/local-memory semantic family",
          "latest function failure: semantic lir_to_bir function "
          "'admitted_null_indirect_call' failed in scalar/local-memory semantic family",
          "typed null indirect callees should not keep reporting the scalar/local-memory "
          "semantic family",
          "typed null indirect callees should not keep the module on the "
          "scalar/local-memory semantic-family note");
      admitted_null_indirect_call_status != 0) {
    return admitted_null_indirect_call_status;
  }

  if (const int alloca_status = expect_failure_notes(
          "bad_alloca",
          make_bad_alloca_module(),
          kModuleSummary,
          "failed in alloca local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_alloca' failed in alloca local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing specific alloca local-memory function note",
          "missing module note carrying the alloca local-memory semantic family failure");
      alloca_status != 0) {
    return alloca_status;
  }

  if (const int function_signature_status = expect_failure_notes(
          "bad_function_signature",
          make_bad_function_signature_module(),
          kModuleSummary,
          "failed in function-signature semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_function_signature' failed in function-signature semantic family",
          "missing module capability-bucket summary note",
          "missing function-signature umbrella function note",
          "missing module note carrying the function-signature umbrella failure");
      function_signature_status != 0) {
    return function_signature_status;
  }

  if (const int scalar_control_flow_status = expect_failure_notes(
          "bad_switch_label_scalar_control_flow",
          make_bad_scalar_control_flow_module(),
          kModuleSummary,
          "failed in scalar-control-flow semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_switch_label_scalar_control_flow' failed in scalar-control-flow semantic family",
          "missing module capability-bucket summary note",
          "missing switch/label scalar-control-flow umbrella function note",
          "missing module note carrying the scalar-control-flow umbrella failure");
      scalar_control_flow_status != 0) {
    return scalar_control_flow_status;
  }

  if (const int global_pointer_return_status = expect_success_without_function_note(
          "return_global_pointer_symbol",
          make_return_global_pointer_symbol_module(),
          "latest function failure: semantic lir_to_bir function 'return_global_pointer_symbol' failed in scalar-control-flow semantic family",
          "failed in scalar-control-flow semantic family",
          "unexpected global pointer-symbol return scalar-control-flow function failure note",
          "unexpected global pointer-symbol return scalar-control-flow module failure note");
      global_pointer_return_status != 0) {
    return global_pointer_return_status;
  }

  if (const int aggregate_phi_join_status = expect_success_without_function_note(
          "admitted_aggregate_phi_join",
          make_admitted_aggregate_phi_join_module(),
          "latest function failure: semantic lir_to_bir function 'admitted_aggregate_phi_join' failed in scalar-control-flow semantic family",
          "failed in scalar-control-flow semantic family",
          "aggregate phi joins should not keep reporting the scalar-control-flow semantic family",
          "aggregate phi joins should not keep the module on the scalar-control-flow semantic-family note");
      aggregate_phi_join_status != 0) {
    return aggregate_phi_join_status;
  }

  if (const int structured_block_label_id_status = expect_structured_block_label_ids();
      structured_block_label_id_status != 0) {
    return structured_block_label_id_status;
  }
  if (const int verifier_block_label_id_status = expect_bir_verifier_prefers_block_label_ids();
      verifier_block_label_id_status != 0) {
    return verifier_block_label_id_status;
  }

  if (const int scalar_float_globals_status = expect_admitted_scalar_float_globals();
      scalar_float_globals_status != 0) {
    return scalar_float_globals_status;
  }

  if (const int scalar_i16_globals_status = expect_admitted_scalar_i16_globals();
      scalar_i16_globals_status != 0) {
    return scalar_i16_globals_status;
  }

  if (const int aarch64_extern_data_status =
          expect_aarch64_extern_data_global_uses_got_policy();
      aarch64_extern_data_status != 0) {
    return aarch64_extern_data_status;
  }

  if (const int link_name_identity_status =
          expect_link_name_id_symbol_identity_survives_drifted_display_names();
      link_name_identity_status != 0) {
    return link_name_identity_status;
  }
  if (const int link_name_unresolved_global_status =
          expect_link_name_id_global_identity_rejects_unresolved_id();
      link_name_unresolved_global_status != 0) {
    return link_name_unresolved_global_status;
  }
  if (const int link_name_unresolved_extern_status =
          expect_link_name_id_extern_identity_rejects_unresolved_id();
      link_name_unresolved_extern_status != 0) {
    return link_name_unresolved_extern_status;
  }

  if (const int string_pool_link_name_status =
          expect_string_pool_direct_call_bridge_prefers_function_link_name_id();
      string_pool_link_name_status != 0) {
    return string_pool_link_name_status;
  }
  if (const int string_pool_text_id_status =
          expect_string_pool_direct_call_bridge_requires_text_id();
      string_pool_text_id_status != 0) {
    return string_pool_text_id_status;
  }

  if (const int link_name_mismatch_status =
          expect_bir_verifier_rejects_known_link_name_mismatches();
      link_name_mismatch_status != 0) {
    return link_name_mismatch_status;
  }

  if (const int local_slot_id_mismatch_status =
          expect_bir_verifier_rejects_local_slot_id_mismatches();
      local_slot_id_mismatch_status != 0) {
    return local_slot_id_mismatch_status;
  }

  if (const int aggregate_pointer_field_global_status =
          expect_admitted_aggregate_pointer_field_global();
      aggregate_pointer_field_global_status != 0) {
    return aggregate_pointer_field_global_status;
  }

  if (const int aggregate_zero_sized_member_global_status =
          expect_admitted_aggregate_zero_sized_member_global();
      aggregate_zero_sized_member_global_status != 0) {
    return aggregate_zero_sized_member_global_status;
  }

  if (const int aggregate_string_array_field_global_status =
          expect_admitted_aggregate_string_array_field_global();
      aggregate_string_array_field_global_status != 0) {
    return aggregate_string_array_field_global_status;
  }

  if (const int aggregate_long_double_field_global_status =
          expect_admitted_aggregate_long_double_field_global();
      aggregate_long_double_field_global_status != 0) {
    return aggregate_long_double_field_global_status;
  }

  if (const int local_memory_umbrella_status = expect_failure_notes(
          "bad_local_memory_umbrella",
          make_bad_local_memory_umbrella_module(),
          kModuleSummary,
          "failed in local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_local_memory_umbrella' failed in local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing local-memory umbrella function note",
          "missing module note carrying the local-memory umbrella failure");
      local_memory_umbrella_status != 0) {
    return local_memory_umbrella_status;
  }

  if (const int scalar_local_memory_umbrella_status = expect_failure_notes(
          "bad_scalar_local_memory_umbrella",
          make_bad_scalar_local_memory_umbrella_module(),
          kModuleSummary,
          "failed in scalar/local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_scalar_local_memory_umbrella' failed in scalar/local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing scalar/local-memory umbrella function note",
          "missing module note carrying the scalar/local-memory umbrella failure");
      scalar_local_memory_umbrella_status != 0) {
    return scalar_local_memory_umbrella_status;
  }

  if (const int scalar_binop_status = expect_failure_notes(
          "bad_scalar_binop",
          make_bad_scalar_binop_module(),
          kModuleSummary,
          "failed in scalar-binop semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_scalar_binop' failed in scalar-binop semantic family",
          "missing module capability-bucket summary note",
          "missing specific scalar-binop function note",
          "missing module note carrying the scalar-binop semantic family failure");
      scalar_binop_status != 0) {
    return scalar_binop_status;
  }

  if (const int admitted_float_scalar_binop_status = expect_success_without_function_note(
          "admitted_float_scalar_binop",
          make_admitted_float_scalar_binop_module(),
          "latest function failure: semantic lir_to_bir function 'admitted_float_scalar_binop' "
          "failed in scalar-binop semantic family",
          "failed in scalar-binop semantic family",
          "float scalar binops that already map into BIR should not keep reporting the "
          "scalar-binop semantic family",
          "float scalar binops that already map into BIR should not keep the module on the "
          "scalar-binop semantic-family note");
      admitted_float_scalar_binop_status != 0) {
    return admitted_float_scalar_binop_status;
  }

  if (const int f128_scalar_constant_status =
          expect_f128_scalar_constant_binop_fails_closed();
      f128_scalar_constant_status != 0) {
    return f128_scalar_constant_status;
  }

  if (const int gep_status = expect_failure_notes(
          "bad_gep",
          make_bad_gep_module(),
          kModuleSummary,
          "failed in gep local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_gep' failed in gep local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing specific gep local-memory function note",
          "missing module note carrying the gep local-memory semantic family failure");
      gep_status != 0) {
    return gep_status;
  }

  if (const int local_aggregate_raw_i8_gep_status = expect_success_without_function_note(
          "local_aggregate_raw_i8_gep_byte_slice",
          make_local_aggregate_raw_i8_gep_byte_slice_module(),
          "latest function failure: semantic lir_to_bir function "
          "'local_aggregate_raw_i8_gep_byte_slice' failed in gep local-memory semantic family",
          "failed in gep local-memory semantic family",
          "unexpected local aggregate raw i8 byte-slice gep function failure note",
          "unexpected local aggregate raw i8 byte-slice gep module failure note");
      local_aggregate_raw_i8_gep_status != 0) {
    return local_aggregate_raw_i8_gep_status;
  }

  if (const int local_aggregate_raw_float_leaf_gep_status = expect_success_without_function_note(
          "local_aggregate_raw_float_leaf_byte_slice",
          make_local_aggregate_raw_float_leaf_byte_slice_module(),
          "latest function failure: semantic lir_to_bir function "
          "'local_aggregate_raw_float_leaf_byte_slice' failed in gep local-memory semantic family",
          "failed in gep local-memory semantic family",
          "unexpected local aggregate raw float-leaf byte-slice gep function failure note",
          "unexpected local aggregate raw float-leaf byte-slice gep module failure note");
      local_aggregate_raw_float_leaf_gep_status != 0) {
    return local_aggregate_raw_float_leaf_gep_status;
  }

  if (const int local_aggregate_raw_float_tail_memcpy_status = expect_success_without_function_note(
          "local_aggregate_raw_float_tail_memcpy",
          make_local_aggregate_raw_float_tail_memcpy_module(),
          "latest function failure: semantic lir_to_bir function "
          "'local_aggregate_raw_float_tail_memcpy' failed in scalar/local-memory semantic family",
          "failed in scalar/local-memory semantic family",
          "unexpected local aggregate raw float-tail memcpy function failure note",
          "unexpected local aggregate raw float-tail memcpy module failure note");
      local_aggregate_raw_float_tail_memcpy_status != 0) {
    return local_aggregate_raw_float_tail_memcpy_status;
  }

  if (const int local_gep_structured_mismatch_status = expect_failure_notes(
          "local_gep_rejects_structured_mismatch",
          make_local_gep_rejects_structured_mismatch_module(),
          kModuleSummary,
          "failed in gep local-memory semantic family",
          "latest function failure: semantic lir_to_bir function "
          "'local_gep_rejects_structured_mismatch' failed in gep local-memory semantic family",
          "missing module capability-bucket summary note for structured local GEP parity "
          "mismatch rejection",
          "missing specific structured local GEP parity mismatch rejection function note",
          "missing module note carrying structured local GEP parity mismatch rejection failure");
      local_gep_structured_mismatch_status != 0) {
    return local_gep_structured_mismatch_status;
  }

  if (const int local_gep_structured_opaque_status = expect_failure_notes(
          "local_gep_rejects_structured_opaque_legacy_fallback",
          make_local_gep_rejects_structured_opaque_legacy_fallback_module(),
          kModuleSummary,
          "failed in gep local-memory semantic family",
          "latest function failure: semantic lir_to_bir function "
          "'local_gep_rejects_structured_opaque_legacy_fallback' failed in gep local-memory "
          "semantic family",
          "missing module capability-bucket summary note for structured local GEP fallback "
          "rejection",
          "missing specific structured local GEP fallback rejection function note",
          "missing module note carrying structured local GEP fallback rejection failure");
      local_gep_structured_opaque_status != 0) {
    return local_gep_structured_opaque_status;
  }

  if (const int local_scalar_double_decimal_zero_store_status = expect_success_without_function_note(
          "local_scalar_double_decimal_zero_store",
          make_local_scalar_double_decimal_zero_store_module(),
          "latest function failure: semantic lir_to_bir function "
          "'local_scalar_double_decimal_zero_store' failed in store local-memory semantic family",
          "failed in store local-memory semantic family",
          "unexpected local scalar double decimal-zero store function failure note",
          "unexpected local scalar double decimal-zero store module failure note");
      local_scalar_double_decimal_zero_store_status != 0) {
    return local_scalar_double_decimal_zero_store_status;
  }

  if (const int local_scalar_double_partial_float_memcpy_status = expect_success_without_function_note(
          "local_scalar_double_partial_float_memcpy",
          make_local_scalar_double_partial_float_memcpy_module(),
          "latest function failure: semantic lir_to_bir function "
          "'local_scalar_double_partial_float_memcpy' failed in scalar/local-memory semantic family",
          "failed in scalar/local-memory semantic family",
          "unexpected local scalar double partial-float memcpy function failure note",
          "unexpected local scalar double partial-float memcpy module failure note");
      local_scalar_double_partial_float_memcpy_status != 0) {
    return local_scalar_double_partial_float_memcpy_status;
  }

  if (const int local_scalar_i64_partial_i8_memcpy_status = expect_success_without_function_note(
          "local_scalar_i64_partial_i8_memcpy",
          make_local_scalar_i64_partial_i8_memcpy_module(),
          "latest function failure: semantic lir_to_bir function "
          "'local_scalar_i64_partial_i8_memcpy' failed in scalar/local-memory semantic family",
          "failed in scalar/local-memory semantic family",
          "unexpected local scalar i64 partial-i8 memcpy function failure note",
          "unexpected local scalar i64 partial-i8 memcpy module failure note");
      local_scalar_i64_partial_i8_memcpy_status != 0) {
    return local_scalar_i64_partial_i8_memcpy_status;
  }

  if (const int dynamic_gep_lane_status = expect_success_without_function_note(
          "dynamic_indexed_gep_local_member_array",
          make_dynamic_indexed_gep_local_member_array_module(),
          "latest function failure: semantic lir_to_bir function 'dynamic_indexed_gep_local_member_array' failed in gep local-memory semantic family",
          "failed in gep local-memory semantic family",
          "unexpected dynamic indexed gep local-memory lane function failure note",
          "unexpected dynamic indexed gep local-memory lane module failure note");
      dynamic_gep_lane_status != 0) {
    return dynamic_gep_lane_status;
  }

  if (const int dynamic_global_scalar_lane_status = expect_success_without_function_note(
          "dynamic_indexed_gep_global_member_scalar",
          make_dynamic_indexed_gep_global_member_scalar_module(),
          "latest function failure: semantic lir_to_bir function 'dynamic_indexed_gep_global_member_scalar' failed in gep local-memory semantic family",
          "failed in gep local-memory semantic family",
          "unexpected dynamic indexed global scalar-member gep function failure note",
          "unexpected dynamic indexed global scalar-member gep module failure note");
      dynamic_global_scalar_lane_status != 0) {
    return dynamic_global_scalar_lane_status;
  }

  if (const int dynamic_global_array_lane_status = expect_success_without_function_note(
          "dynamic_indexed_gep_global_member_array",
          make_dynamic_indexed_gep_global_member_array_module(),
          "latest function failure: semantic lir_to_bir function 'dynamic_indexed_gep_global_member_array' failed in gep local-memory semantic family",
          "failed in gep local-memory semantic family",
          "unexpected dynamic indexed global member-array gep function failure note",
          "unexpected dynamic indexed global member-array gep module failure note");
      dynamic_global_array_lane_status != 0) {
    return dynamic_global_array_lane_status;
  }

  if (const int dynamic_global_array_identity_status =
          expect_dynamic_global_scalar_array_loads_carry_link_name_id();
      dynamic_global_array_identity_status != 0) {
    return dynamic_global_array_identity_status;
  }

  if (const int dynamic_global_array_missing_identity_status =
          expect_dynamic_global_scalar_array_loads_reject_missing_link_name_spelling();
      dynamic_global_array_missing_identity_status != 0) {
    return dynamic_global_array_missing_identity_status;
  }

  if (const int dynamic_global_array_raw_identity_status =
          expect_dynamic_global_scalar_array_loads_keep_no_id_compatibility();
      dynamic_global_array_raw_identity_status != 0) {
    return dynamic_global_array_raw_identity_status;
  }

  if (const int pointer_initializer_identity_status =
          expect_pointer_initializer_symbol_names_carry_link_name_id();
      pointer_initializer_identity_status != 0) {
    return pointer_initializer_identity_status;
  }

  if (const int pointer_value_identity_status = expect_pointer_value_symbol_identity_carrier();
      pointer_value_identity_status != 0) {
    return pointer_value_identity_status;
  }

  if (const int addressed_global_pointer_identity_status =
          expect_addressed_global_pointer_provenance_uses_link_name_id_keys();
      addressed_global_pointer_identity_status != 0) {
    return addressed_global_pointer_identity_status;
  }

  if (const int store_status = expect_failure_notes(
          "bad_store",
          make_bad_store_module(),
          kModuleSummary,
          "failed in store local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_store' failed in store local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing specific store local-memory function note",
          "missing module note carrying the store local-memory semantic family failure");
      store_status != 0) {
    return store_status;
  }

  if (const int load_status = expect_failure_notes(
          "bad_load",
          make_bad_load_module(),
          kModuleSummary,
          "failed in load local-memory semantic family",
          "latest function failure: semantic lir_to_bir function 'bad_load' failed in load local-memory semantic family",
          "missing module capability-bucket summary note",
          "missing specific load local-memory function note",
          "missing module note carrying the load local-memory semantic family failure");
      load_status != 0) {
    return load_status;
  }
  return 0;
}
