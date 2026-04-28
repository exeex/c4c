#include "src/backend/bir/lir_to_bir.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace {

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

LirModule make_admitted_scalar_float_globals_module();
LirModule make_admitted_scalar_i16_globals_module();
LirModule make_admitted_aggregate_pointer_field_global_module();
LirModule make_admitted_aggregate_zero_sized_member_global_module();
LirModule make_admitted_aggregate_string_array_field_global_module();
LirModule make_admitted_aggregate_long_double_field_global_module();
LirModule make_structured_block_label_id_module();
LirModule make_dynamic_indexed_gep_global_member_array_module();

int expect_link_name_id_symbol_identity_survives_drifted_display_names();
int expect_dynamic_global_scalar_array_loads_carry_link_name_id();
int expect_bir_verifier_rejects_known_link_name_mismatches();

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
      result.module->globals.front().name != "drifted_global_display" ||
      result.module->globals.front().link_name_id != global_id) {
    return fail("BIR global should preserve drifted display spelling beside LinkNameId");
  }
  const auto& lowered_function = result.module->functions.back();
  if (lowered_function.name != "drifted_user_display" ||
      lowered_function.link_name_id != user_id) {
    return fail("BIR function should preserve drifted display spelling beside LinkNameId");
  }

  const auto* store =
      std::get_if<c4c::backend::bir::StoreGlobalInst>(&lowered_function.blocks.front().insts[0]);
  if (store == nullptr || store->global_name != "semantic_global" ||
      store->global_name_id != global_id) {
    return fail("BIR global store should carry LinkNameId identity for semantic global refs");
  }
  const auto* call =
      std::get_if<c4c::backend::bir::CallInst>(&lowered_function.blocks.front().insts[1]);
  if (call == nullptr || call->callee != "stale_callee_display" ||
      call->callee_link_name_id != callee_id) {
    return fail("BIR direct call should carry LinkNameId identity despite stale callee spelling");
  }
  const auto* load =
      std::get_if<c4c::backend::bir::LoadGlobalInst>(&lowered_function.blocks.front().insts[2]);
  if (load == nullptr || load->global_name != "semantic_global" ||
      load->global_name_id != global_id) {
    return fail("BIR global load should carry LinkNameId identity for semantic global refs");
  }
  return 0;
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

int expect_bir_verifier_rejects_known_link_name_mismatches() {
  namespace bir = c4c::backend::bir;

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

  if (const int scalar_float_globals_status = expect_admitted_scalar_float_globals();
      scalar_float_globals_status != 0) {
    return scalar_float_globals_status;
  }

  if (const int scalar_i16_globals_status = expect_admitted_scalar_i16_globals();
      scalar_i16_globals_status != 0) {
    return scalar_i16_globals_status;
  }

  if (const int link_name_identity_status =
          expect_link_name_id_symbol_identity_survives_drifted_display_names();
      link_name_identity_status != 0) {
    return link_name_identity_status;
  }

  if (const int link_name_mismatch_status =
          expect_bir_verifier_rejects_known_link_name_mismatches();
      link_name_mismatch_status != 0) {
    return link_name_mismatch_status;
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
