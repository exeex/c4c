#include "arena.hpp"
#include "hir_to_lir.hpp"
#include "hir_to_lir/lowering.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"
#include "codegen/shared/llvm_helpers.hpp"

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

c4c::hir::GlobalVar& require_hir_global(c4c::hir::Module& module,
                                        std::string_view name) {
  const auto it = std::find_if(module.globals.begin(), module.globals.end(),
                               [&](const c4c::hir::GlobalVar& global) {
                                 return global.name == name;
                               });
  expect_true(it != module.globals.end(),
              "fixture HIR global should exist: " + std::string(name));
  return *it;
}

c4c::Node make_record_owner(std::string_view rendered_name,
                            std::string_view owner_name, c4c::TextId text_id,
                            int namespace_context_id) {
  c4c::Node owner{};
  owner.kind = c4c::NK_STRUCT_DEF;
  owner.name = rendered_name.data();
  owner.unqualified_name = owner_name.data();
  owner.unqualified_text_id = text_id;
  owner.namespace_context_id = namespace_context_id;
  return owner;
}

c4c::TypeSpec make_owner_type(c4c::Node& owner) {
  c4c::TypeSpec type{};
  type.base = c4c::TB_STRUCT;
  type.tag_text_id = owner.unqualified_text_id;
  type.namespace_context_id = owner.namespace_context_id;
  type.record_def = &owner;
  type.array_size = -1;
  type.inner_rank = -1;
  return type;
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

void expect_type_ref_structured_equality_uses_name_id(
    const c4c::codegen::lir::LirModule& module) {
  const c4c::StructNameId pair_id = module.struct_names.find("%struct.Pair");
  const c4c::StructNameId slot_id = module.struct_names.find("%struct.Slot");
  expect_true(pair_id != c4c::kInvalidStructName,
              "fixture should declare Pair for equality collision checks");
  expect_true(slot_id != c4c::kInvalidStructName && slot_id != pair_id,
              "fixture should declare Slot for equality collision checks");

  const c4c::codegen::lir::LirTypeRef pair_ref =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.Pair", pair_id);
  const c4c::codegen::lir::LirTypeRef collision_ref =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.Pair", slot_id);
  expect_true(pair_ref != collision_ref,
              "global type-ref equality should reject same text with different StructNameId");

  expect_true(c4c::codegen::lir::LirTypeRef("%struct.Pair") ==
                  c4c::codegen::lir::LirTypeRef("%struct.Pair"),
              "global legacy no-id type refs should still compare by rendered text");
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

void test_global_type_ref_owner_key_precedes_stale_rendered_names() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct RealGlobalOwner {
  int value;
};

int owner_hit_global;
int owner_miss_global;
)c");

  const c4c::TextId hit_owner_text =
      hir_module.link_name_texts->intern("HitGlobalOwner");
  c4c::Node hit_owner =
      make_record_owner("StaleGlobalHitCompat", "HitGlobalOwner",
                        hit_owner_text, 601);
  c4c::hir::NamespaceQualifier hit_ns;
  hit_ns.context_id = hit_owner.namespace_context_id;
  hir_module.index_struct_def_owner(
      c4c::hir::make_hir_record_owner_key(hit_ns, hit_owner_text),
      "RealGlobalOwner", true);

  require_hir_global(hir_module, "owner_hit_global").type.spec =
      make_owner_type(hit_owner);

  const c4c::TextId missing_owner_text =
      hir_module.link_name_texts->intern("MissingGlobalOwner");
  c4c::Node missing_owner =
      make_record_owner("StaleGlobalMissCompat", "MissingGlobalOwner",
                        missing_owner_text, 602);
  require_hir_global(hir_module, "owner_miss_global").type.spec =
      make_owner_type(missing_owner);

  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);
  c4c::codegen::lir::verify_module(lir_module);

  const auto& hit = require_global(lir_module, "owner_hit_global");
  expect_eq(hit.llvm_type, "%struct.RealGlobalOwner",
            "complete owner-key hit should lower the global type through structured owner identity");
  expect_true(hit.llvm_type_ref.has_value(),
              "complete owner-key hit should keep a global type mirror");
  expect_eq(hit.llvm_type_ref->str(), "%struct.RealGlobalOwner",
            "complete owner-key hit mirror text should match the structured owner tag");
  expect_true(hit.llvm_type_ref->has_struct_name_id(),
              "complete owner-key hit should produce a structured name id");
  expect_eq(lir_module.struct_names.spelling(hit.llvm_type_ref->struct_name_id()),
            "%struct.RealGlobalOwner",
            "complete owner-key hit should prefer structured owner tag over stale rendered names");
  expect_true(lir_module.struct_names.find("%struct.StaleGlobalHitCompat") ==
                  c4c::kInvalidStructName,
              "owner-key hit should not intern stale compatibility name ids");

  const auto& miss = require_global(lir_module, "owner_miss_global");
  expect_eq(miss.llvm_type, "%struct.StaleGlobalMissCompat",
            "owner-key miss should preserve rendered global type text");
  expect_true(!miss.llvm_type_ref.has_value(),
              "complete owner-key miss must not return a stale compatibility type mirror");
  expect_true(lir_module.struct_names.find("%struct.StaleGlobalMissCompat") ==
                  c4c::kInvalidStructName,
              "complete owner-key miss should stop before interning stale rendered names");
}

void test_fp128_literals_match_llvm_text_order() {
  namespace helpers = c4c::codegen::llvm_helpers;
  helpers::set_active_target_triple("aarch64-unknown-linux-gnu");

  expect_eq(helpers::fp_literal(c4c::TB_LONGDOUBLE, 1.0),
            "0xL00000000000000003FFF000000000000",
            "fp128 literal text should match LLVM low-word/high-word order");
  expect_eq(helpers::fp_literal(c4c::TB_LONGDOUBLE, 0.5),
            "0xL00000000000000003FFE000000000000",
            "fp128 half literal text should match LLVM low-word/high-word order");
  expect_eq(helpers::fp_literal(c4c::TB_LONGDOUBLE, 1.0, "1.0L"),
            "0xL00000000000000003FFF000000000000",
            "decimal fp128 literal parser should use LLVM low-word/high-word order");
  expect_eq(helpers::fp_literal(c4c::TB_LONGDOUBLE, 0.0, "-0.0L"),
            "0xL00000000000000008000000000000000",
            "negative zero fp128 literal text should keep the sign bit in the high word");
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
  expect_type_ref_structured_equality_uses_name_id(lir_module);

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
  test_global_type_ref_owner_key_precedes_stale_rendered_names();
  test_fp128_literals_match_llvm_text_order();

  std::cout << "PASS: frontend_lir_global_type_ref\n";
  return 0;
}
