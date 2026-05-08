#include "arena.hpp"
#include "hir_to_lir.hpp"
#include "hir_to_lir/lowering.hpp"
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
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

c4c::hir::Module lower_hir_module(std::string_view source) {
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::C));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::C,
                     "frontend_lir_global_type_ref_test.c");
  c4c::Node* root = parser.parse();
  auto result =
      c4c::sema::analyze_program(root, c4c::sema_profile_from(c4c::SourceProfile::C));

  expect_true(result.validation.ok,
              "fixture source should parse and validate successfully");
  expect_true(result.hir_module.has_value(),
              "fixture source should lower to HIR");
  return *result.hir_module;
}

const c4c::codegen::lir::LirGlobal& require_global(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  const auto it = std::find_if(module.globals.begin(), module.globals.end(),
                               [&](const c4c::codegen::lir::LirGlobal& global) {
                                 return global.name == name;
                               });
  expect_true(it != module.globals.end(),
              "fixture global should lower into LIR: " + std::string(name));
  return *it;
}

c4c::codegen::lir::LirGlobal& require_global(
    c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  const auto it = std::find_if(module.globals.begin(), module.globals.end(),
                               [&](const c4c::codegen::lir::LirGlobal& global) {
                                 return global.name == name;
                               });
  expect_true(it != module.globals.end(),
              "fixture global should lower into LIR: " + std::string(name));
  return *it;
}

void expect_global_type_ref(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirGlobal& global,
    std::string_view expected_type) {
  expect_eq(global.llvm_type, expected_type,
            "global should keep the legacy LLVM type text");
  expect_true(global.llvm_type_ref.has_value(),
              "top-level struct/union globals should carry a structured LIR type mirror");
  expect_eq(global.llvm_type_ref->str(), global.llvm_type,
            "global type mirror text should shadow llvm_type exactly");
  expect_true(global.llvm_type_ref->has_struct_name_id(),
              "global type mirror should carry a StructNameId");
  expect_eq(module.struct_names.spelling(global.llvm_type_ref->struct_name_id()),
            expected_type,
            "global StructNameId should resolve to the rendered aggregate type");
}

void test_lookup_structured_layout_rejects_stale_rendered_compatibility() {
  c4c::hir::Module hir_module;
  c4c::codegen::lir::LirModule lir_module;
  lir_module.link_name_texts = hir_module.link_name_texts;
  lir_module.struct_names.attach_text_table(lir_module.link_name_texts.get());

  c4c::Node real_owner{};
  real_owner.kind = c4c::NK_STRUCT_DEF;
  real_owner.name = "RealLirLayoutOwner";
  real_owner.unqualified_name = "RealLirLayoutOwner";
  real_owner.unqualified_text_id =
      hir_module.link_name_texts->intern("RealLirLayoutOwner");
  real_owner.namespace_context_id = 901;

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealLirLayoutOwner";
  real_def.tag_text_id = real_owner.unqualified_text_id;
  real_def.ns_qual.context_id = real_owner.namespace_context_id;
  real_def.size_bytes = 24;
  real_def.align_bytes = 8;
  c4c::hir::HirStructField real_field;
  real_field.name = "value";
  real_field.elem_type.base = c4c::TB_DOUBLE;
  real_field.size_bytes = 8;
  real_field.align_bytes = 8;
  real_def.fields.push_back(real_field);
  hir_module.index_struct_def_owner(c4c::hir::make_hir_record_owner_key(real_def),
                                    real_def.tag, true);
  hir_module.struct_defs[real_def.tag] = real_def;

  const c4c::StructNameId real_name_id = lir_module.struct_names.intern(
      c4c::codegen::llvm_helpers::llvm_struct_type_str(real_def.tag));
  c4c::codegen::lir::LirStructDecl real_lir_decl;
  real_lir_decl.name_id = real_name_id;
  lir_module.record_struct_decl(real_lir_decl);

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleLirLayoutCompat";
  stale_def.tag_text_id =
      hir_module.link_name_texts->intern("StaleLirLayoutCompat");
  stale_def.ns_qual.context_id = 901;
  stale_def.size_bytes = 64;
  stale_def.align_bytes = 16;
  c4c::hir::HirStructField stale_field;
  stale_field.name = "value";
  stale_field.elem_type.base = c4c::TB_FLOAT;
  stale_field.size_bytes = 4;
  stale_field.align_bytes = 4;
  stale_def.fields.push_back(stale_field);
  hir_module.struct_defs[stale_def.tag] = stale_def;

  const c4c::StructNameId stale_name_id = lir_module.struct_names.intern(
      c4c::codegen::llvm_helpers::llvm_struct_type_str(stale_def.tag));
  c4c::codegen::lir::LirStructDecl stale_lir_decl;
  stale_lir_decl.name_id = stale_name_id;
  lir_module.record_struct_decl(stale_lir_decl);

  c4c::TypeSpec owner_hit_query{};
  owner_hit_query.base = c4c::TB_STRUCT;
  owner_hit_query.tag_text_id = real_owner.unqualified_text_id;
  owner_hit_query.namespace_context_id = real_owner.namespace_context_id;
  owner_hit_query.record_def = &real_owner;
  owner_hit_query.array_size = -1;
  owner_hit_query.inner_rank = -1;

  const c4c::codegen::lir::stmt_emitter_detail::StructuredLayoutLookup hit =
      c4c::codegen::lir::stmt_emitter_detail::lookup_structured_layout(
          hir_module, &lir_module, owner_hit_query, "test-lir-layout-hit");
  expect_true(hit.legacy_decl && hit.legacy_decl->tag == real_def.tag,
              "complete owner-key hit should select the structured HIR layout");
  expect_true(hit.structured_decl && hit.structured_name_id == real_name_id,
              "complete owner-key hit should select the structured LIR layout");
  const auto hit_hfa =
      c4c::codegen::lir::stmt_emitter_detail::classify_aarch64_hfa(
          hir_module, owner_hit_query);
  expect_true(hit_hfa && hit_hfa->aggregate_size == real_def.size_bytes &&
                  hit_hfa->aggregate_align == real_def.align_bytes,
              "ABI aggregate layout lookup should use the structured owner-key layout");

  c4c::Node missing_owner{};
  missing_owner.kind = c4c::NK_STRUCT_DEF;
  missing_owner.name = "StaleLirLayoutCompat";
  missing_owner.unqualified_name = "StaleLirLayoutCompat";
  missing_owner.unqualified_text_id =
      hir_module.link_name_texts->intern("MissingLirLayoutOwner");
  missing_owner.namespace_context_id = stale_def.ns_qual.context_id;

  c4c::TypeSpec owner_miss_query{};
  owner_miss_query.base = c4c::TB_STRUCT;
  owner_miss_query.tag_text_id = missing_owner.unqualified_text_id;
  owner_miss_query.namespace_context_id = missing_owner.namespace_context_id;
  owner_miss_query.record_def = &missing_owner;
  owner_miss_query.array_size = -1;
  owner_miss_query.inner_rank = -1;

  const c4c::codegen::lir::stmt_emitter_detail::StructuredLayoutLookup miss =
      c4c::codegen::lir::stmt_emitter_detail::lookup_structured_layout(
          hir_module, &lir_module, owner_miss_query, "test-lir-layout-miss");
  expect_true(!miss.legacy_decl,
              "complete owner-key miss must not select stale rendered HIR layout compatibility");
  expect_true(!miss.structured_decl &&
                  miss.structured_name_id == c4c::kInvalidStructName,
              "complete owner-key miss must not select stale rendered LIR layout compatibility");
  const auto miss_hfa =
      c4c::codegen::lir::stmt_emitter_detail::classify_aarch64_hfa(
          hir_module, owner_miss_query);
  expect_true(!miss_hfa,
              "ABI aggregate layout lookup must stop after a complete owner-key miss");

  c4c::TypeSpec no_owner_query{};
  no_owner_query.base = c4c::TB_STRUCT;
  no_owner_query.tag_text_id = stale_def.tag_text_id;
  no_owner_query.namespace_context_id = -1;
  c4c::TextId incomplete_qualifier[] = {c4c::kInvalidText};
  no_owner_query.qualifier_text_ids = incomplete_qualifier;
  no_owner_query.n_qualifier_segments = 1;
  no_owner_query.array_size = -1;
  no_owner_query.inner_rank = -1;

  const c4c::codegen::lir::stmt_emitter_detail::StructuredLayoutLookup compat =
      c4c::codegen::lir::stmt_emitter_detail::lookup_structured_layout(
          hir_module, &lir_module, no_owner_query, "test-lir-layout-compat");
  expect_true(compat.legacy_decl && compat.legacy_decl->tag == stale_def.tag,
              "no-owner metadata should preserve rendered HIR layout compatibility");
  expect_true(compat.structured_decl && compat.structured_name_id == stale_name_id,
              "no-owner metadata should preserve rendered LIR layout compatibility");
  const auto compat_hfa =
      c4c::codegen::lir::stmt_emitter_detail::classify_aarch64_hfa(
          hir_module, no_owner_query);
  expect_true(compat_hfa && compat_hfa->aggregate_size == stale_def.size_bytes,
              "ABI aggregate layout lookup should preserve no-owner rendered compatibility");
}

}  // namespace

int main() {
  const c4c::hir::Module hir_module = lower_hir_module(R"c(
struct Pair {
  int left;
  int right;
};

union Slot {
  int int_value;
  float float_value;
};

struct Pair pair_global = {1, 2};
union Slot slot_global = {.int_value = 3};
)c");
  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);

  const auto& pair_global = require_global(lir_module, "pair_global");
  expect_global_type_ref(lir_module, pair_global, "%struct.Pair");

  const auto& slot_global = require_global(lir_module, "slot_global");
  expect_global_type_ref(lir_module, slot_global, "%struct.Slot");

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("@pair_global = global %struct.Pair ") != std::string::npos,
              "printer should keep using legacy llvm_type text for struct globals");
  expect_true(llvm_ir.find("@slot_global = global %struct.Slot ") != std::string::npos,
              "printer should keep using legacy llvm_type text for union globals");

  c4c::codegen::lir::LirModule structured_identity = lir_module;
  c4c::codegen::lir::LirGlobal& identity_pair =
      require_global(structured_identity, "pair_global");
  identity_pair.llvm_type_ref->str() = "%struct.StaleMirrorText";
  c4c::codegen::lir::verify_module(structured_identity);

  c4c::codegen::lir::LirModule text_fallback = lir_module;
  c4c::codegen::lir::LirGlobal& fallback_slot =
      require_global(text_fallback, "slot_global");
  fallback_slot.llvm_type_ref =
      c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(text_fallback);
    fail("verifier should reject a global mirror text mismatch without StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  test_lookup_structured_layout_rejects_stale_rendered_compatibility();

  std::cout << "PASS: frontend_lir_global_type_ref\n";
  return 0;
}
