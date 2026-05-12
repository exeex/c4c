#include "arena.hpp"
#include "call_args_ops.hpp"
#include "hir_to_lir.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"
#include "target_profile.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
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
                     "frontend_lir_call_type_ref_test.c");
  c4c::Node* root = parser.parse();
  auto result =
      c4c::sema::analyze_program(root, c4c::sema_profile_from(c4c::SourceProfile::C));

  expect_true(result.validation.ok,
              "fixture source should parse and validate successfully");
  expect_true(result.hir_module.has_value(),
              "fixture source should lower to HIR");
  return *result.hir_module;
}

c4c::codegen::lir::LirFunction& require_function(
    c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  const auto it = std::find_if(module.functions.begin(), module.functions.end(),
                               [&](const c4c::codegen::lir::LirFunction& fn) {
                                 return fn.name == name && !fn.is_declaration;
                               });
  expect_true(it != module.functions.end(),
              "fixture function should lower into LIR: " + std::string(name));
  return *it;
}

c4c::codegen::lir::LirCallOp& require_call_to(
    c4c::codegen::lir::LirFunction& fn,
    std::string_view callee) {
  for (auto& block : fn.blocks) {
    for (auto& inst : block.insts) {
      auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (call && call->callee == callee) return *call;
    }
  }
  fail("fixture function should contain call to " + std::string(callee));
}

c4c::hir::Expr& require_member_expr(c4c::hir::Module& module,
                                    std::string_view field) {
  for (c4c::hir::Expr& expr : module.expr_pool) {
    auto* member = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
    if (member && member->field == field) return expr;
  }
  fail("fixture should contain member expression for field " + std::string(field));
}

c4c::hir::Expr& require_expr(c4c::hir::Module& module, c4c::hir::ExprId id,
                             const std::string& msg) {
  c4c::hir::Expr* expr = module.find_expr(id);
  expect_true(expr != nullptr, msg);
  return *expr;
}

const c4c::codegen::lir::LirGepOp& require_first_gep(
    const c4c::codegen::lir::LirFunction& fn) {
  for (const auto& block : fn.blocks) {
    for (const auto& inst : block.insts) {
      const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst);
      if (gep) return *gep;
    }
  }
  fail("fixture function should contain a member GEP");
}

void expect_struct_type_ref(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirTypeRef& type_ref,
    std::string_view expected_text,
    const std::string& msg) {
  expect_eq(type_ref.str(), expected_text, msg + " text should match");
  expect_true(type_ref.has_struct_name_id(), msg + " should carry a StructNameId");
  expect_eq(module.struct_names.spelling(type_ref.struct_name_id()), expected_text,
            msg + " StructNameId should resolve to mirrored text");
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
              "call type-ref equality should reject same text with different StructNameId");

  expect_true(c4c::codegen::lir::LirTypeRef("%struct.Pair") ==
                  c4c::codegen::lir::LirTypeRef("%struct.Pair"),
              "call legacy no-id type refs should still compare by rendered text");
}

c4c::Node make_record_owner(std::string_view name, c4c::TextId text_id,
                            int namespace_context_id) {
  c4c::Node owner{};
  owner.kind = c4c::NK_STRUCT_DEF;
  owner.name = name.data();
  owner.unqualified_name = name.data();
  owner.unqualified_text_id = text_id;
  owner.namespace_context_id = namespace_context_id;
  return owner;
}

void append_compat_only_stale_member_layout(c4c::hir::Module& module) {
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleMemberOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleMemberOwner");
  stale_def.ns_qual.context_id = 77;
  stale_def.size_bytes = 4;
  stale_def.align_bytes = 4;
  c4c::hir::HirStructField stale_field;
  stale_field.name = "stale";
  stale_field.elem_type.base = c4c::TB_INT;
  stale_field.size_bytes = 4;
  stale_field.align_bytes = 4;
  stale_def.fields.push_back(stale_field);
  module.struct_defs[stale_def.tag] = stale_def;
}

void poison_member_base_type(c4c::hir::Module& module, c4c::hir::MemberExpr& member,
                             const c4c::TypeSpec& replacement) {
  c4c::hir::Expr& base = require_expr(module, member.base,
                                      "member base expression should exist");
  base.type.spec = replacement;
}

void test_member_access_owner_tag_recovery_uses_structured_owner_key() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct RealMemberOwner {
  int actual;
};

int read_actual(struct RealMemberOwner value) {
  return value.actual;
}
)c");
  append_compat_only_stale_member_layout(hir_module);

  c4c::hir::Expr& member_expr = require_member_expr(hir_module, "actual");
  auto& member = std::get<c4c::hir::MemberExpr>(member_expr.payload);
  member.resolved_owner_tag = "StaleMemberOwner";
  member.member_symbol_id = c4c::kInvalidMemberSymbol;

  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);
  const c4c::codegen::lir::LirFunction& fn = require_function(lir_module, "read_actual");
  const c4c::codegen::lir::LirGepOp& gep = require_first_gep(fn);
  expect_eq(gep.element_type.str(), "%struct.RealMemberOwner",
            "member owner recovery should use the structured owner-key tag");
  expect_true(gep.element_type.has_struct_name_id(),
              "member owner recovery GEP should carry structured LIR type identity");
  expect_eq(lir_module.struct_names.spelling(gep.element_type.struct_name_id()),
            "%struct.RealMemberOwner",
            "member owner recovery StructNameId should resolve to the structured tag");
}

void test_member_access_owner_tag_recovery_rejects_stale_rendered_miss() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct RealMemberOwner {
  int actual;
};

int read_actual(struct RealMemberOwner value) {
  return value.actual;
}
)c");
  append_compat_only_stale_member_layout(hir_module);

  const c4c::TextId missing_owner_text =
      hir_module.link_name_texts->intern("MissingMemberOwner");
  c4c::Node missing_owner =
      make_record_owner("StaleMemberOwner", missing_owner_text, 77);

  c4c::TypeSpec owner_miss_query{};
  owner_miss_query.base = c4c::TB_STRUCT;
  owner_miss_query.tag_text_id = missing_owner.unqualified_text_id;
  owner_miss_query.namespace_context_id = missing_owner.namespace_context_id;
  owner_miss_query.record_def = &missing_owner;
  owner_miss_query.array_size = -1;
  owner_miss_query.inner_rank = -1;

  c4c::hir::Expr& member_expr = require_member_expr(hir_module, "actual");
  auto& member = std::get<c4c::hir::MemberExpr>(member_expr.payload);
  member.field = "stale";
  member.field_text_id = hir_module.link_name_texts->intern("stale");
  member.resolved_owner_tag.clear();
  member.member_symbol_id = c4c::kInvalidMemberSymbol;
  member_expr.type.spec.base = c4c::TB_INT;
  poison_member_base_type(hir_module, member, owner_miss_query);

  try {
    (void)c4c::codegen::lir::lower(hir_module);
    fail("complete owner-key miss must not recover member access through stale rendered compatibility");
  } catch (const std::runtime_error& err) {
    expect_true(std::string(err.what()).find("MemberExpr base has no struct tag") !=
                    std::string::npos,
                "complete owner-key miss should stop before rendered member fallback");
  }
}

void test_member_access_owner_tag_recovery_preserves_no_owner_compatibility() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct RealMemberOwner {
  int actual;
};

struct StaleMemberOwner {
  int stale;
};

int read_actual(struct RealMemberOwner value) {
  return value.actual;
}
)c");

  c4c::TypeSpec no_owner_query{};
  no_owner_query.base = c4c::TB_STRUCT;
  no_owner_query.tag_text_id = hir_module.link_name_texts->intern("StaleMemberOwner");
  no_owner_query.namespace_context_id = -1;
  c4c::TextId incomplete_qualifier[] = {c4c::kInvalidText};
  no_owner_query.qualifier_text_ids = incomplete_qualifier;
  no_owner_query.n_qualifier_segments = 1;
  no_owner_query.array_size = -1;
  no_owner_query.inner_rank = -1;

  c4c::hir::Expr& member_expr = require_member_expr(hir_module, "actual");
  auto& member = std::get<c4c::hir::MemberExpr>(member_expr.payload);
  member.field = "stale";
  member.field_text_id = hir_module.link_name_texts->intern("stale");
  member.resolved_owner_tag.clear();
  member.member_symbol_id = c4c::kInvalidMemberSymbol;
  member_expr.type.spec.base = c4c::TB_INT;
  poison_member_base_type(hir_module, member, no_owner_query);

  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);
  const c4c::codegen::lir::LirFunction& fn = require_function(lir_module, "read_actual");
  const c4c::codegen::lir::LirGepOp& gep = require_first_gep(fn);
  expect_eq(gep.element_type.str(), "%struct.StaleMemberOwner",
            "no-owner member metadata should preserve rendered compatibility");
}

}  // namespace

int main() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct Pair {
  int left;
  int right;
};

struct Big {
  long long a;
  long long b;
  long long c;
};

struct Slot {
  int value;
};

struct Pair make_pair(struct Pair input) {
  return input;
}

struct Pair call_pair(struct Pair value) {
  return make_pair(value);
}

int consume_big(struct Big input) {
  return (int)input.a;
}

int call_big(struct Big value) {
  return consume_big(value);
}

int sink(int seed, ...);

int call_variadic(struct Pair tail) {
  return sink(1, tail);
}
)c");
  hir_module.target_profile =
      c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);
  expect_type_ref_structured_equality_uses_name_id(lir_module);
  c4c::codegen::lir::LirFunction& call_pair = require_function(lir_module, "call_pair");
  c4c::codegen::lir::LirCallOp& direct_call = require_call_to(call_pair, "@make_pair");

  expect_struct_type_ref(lir_module, direct_call.return_type, "%struct.Pair",
                         "call return mirror");
  expect_eq(std::to_string(direct_call.arg_type_refs.size()), "1",
            "direct call should carry one argument mirror");
  expect_struct_type_ref(lir_module, direct_call.arg_type_refs[0], "%struct.Pair",
                         "call argument mirror");

  const std::string formatted =
      c4c::codegen::lir::format_lir_call_site(direct_call);
  expect_true(formatted.find("@make_pair(%struct.Pair ") != std::string::npos,
              "format_lir_call_site should keep the existing call text shape");

  c4c::codegen::lir::LirFunction& call_big = require_function(lir_module, "call_big");
  c4c::codegen::lir::LirCallOp& byval_call =
      require_call_to(call_big, "@consume_big");
  expect_eq(std::to_string(byval_call.arg_type_refs.size()), "1",
            "fixed byval aggregate call should carry one argument mirror");
  expect_eq(byval_call.args_str.find("ptr byval(%struct.Big) align 8") ==
                    std::string::npos
                ? "missing"
                : "present",
            "present",
            "fixed byval aggregate call should keep the emitted ABI fragment");
  expect_struct_type_ref(lir_module, byval_call.arg_type_refs[0], "%struct.Big",
                         "fixed byval call argument mirror");

  c4c::codegen::lir::verify_module(lir_module);
  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("call %struct.Pair (%struct.Pair) @make_pair(%struct.Pair ") !=
                  std::string::npos,
              "printer should keep using formatted call-site text");
  expect_true(llvm_ir.find("call i32 (ptr) @consume_big(ptr byval(%struct.Big) align 8 ") !=
                  std::string::npos,
              "printer should keep using formatted byval call-site text");

  const c4c::StructNameId pair_id = direct_call.return_type.struct_name_id();
  const c4c::StructNameId slot_id = lir_module.struct_names.find("%struct.Slot");
  expect_true(slot_id != c4c::kInvalidStructName,
              "fixture should declare a second struct for mismatch checks");

  c4c::codegen::lir::LirModule stale_return_text = lir_module;
  c4c::codegen::lir::LirCallOp& stale_return_call =
      require_call_to(require_function(stale_return_text, "call_pair"), "@make_pair");
  stale_return_call.return_type.str() = "%struct.StaleMirrorText";
  c4c::codegen::lir::verify_module(stale_return_text);

  c4c::codegen::lir::LirModule stale_arg_text = lir_module;
  c4c::codegen::lir::LirCallOp& stale_arg_call =
      require_call_to(require_function(stale_arg_text, "call_pair"), "@make_pair");
  stale_arg_call.arg_type_refs[0].str() = "%struct.StaleMirrorText";
  c4c::codegen::lir::verify_module(stale_arg_text);

  c4c::codegen::lir::LirModule mismatched_return_name = lir_module;
  c4c::codegen::lir::LirCallOp& mismatched_return_call =
      require_call_to(require_function(mismatched_return_name, "call_pair"), "@make_pair");
  mismatched_return_call.return_type =
      mismatched_return_call.return_type.with_struct_name_id(slot_id);
  try {
    c4c::codegen::lir::verify_module(mismatched_return_name);
    fail("verifier should reject a call return with mismatched StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule mismatched_arg_name = lir_module;
  c4c::codegen::lir::LirCallOp& mismatched_arg_call =
      require_call_to(require_function(mismatched_arg_name, "call_pair"), "@make_pair");
  mismatched_arg_call.arg_type_refs[0] =
      mismatched_arg_call.arg_type_refs[0].with_struct_name_id(slot_id);
  try {
    c4c::codegen::lir::verify_module(mismatched_arg_name);
    fail("verifier should reject a call argument with mismatched StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule arg_text_fallback = lir_module;
  c4c::codegen::lir::LirCallOp& fallback_arg_call =
      require_call_to(require_function(arg_text_fallback, "call_pair"), "@make_pair");
  fallback_arg_call.args_str.replace(
      fallback_arg_call.args_str.find("%struct.Pair"),
      std::string("%struct.Pair").size(),
      "%struct.NotDeclared");
  fallback_arg_call.arg_type_refs[0] =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.StaleMirrorText",
                                                 pair_id);
  try {
    c4c::codegen::lir::verify_module(arg_text_fallback);
    fail("verifier should reject call argument mirror text mismatch without declared struct boundary");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirFunction& call_variadic =
      require_function(lir_module, "call_variadic");
  c4c::codegen::lir::LirCallOp& variadic_call =
      require_call_to(call_variadic, "@sink");
  expect_eq(std::to_string(variadic_call.arg_type_refs.size()), "0",
            "variadic aggregate call should not carry argument mirrors when "
            "the call signature cannot parse against emitted ABI arguments");

  c4c::codegen::lir::LirModule missing_return_name = lir_module;
  c4c::codegen::lir::LirCallOp& missing_return_call =
      require_call_to(require_function(missing_return_name, "call_pair"), "@make_pair");
  missing_return_call.return_type = c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(missing_return_name);
    fail("verifier should reject a known struct call return without StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule missing_arg_name = lir_module;
  c4c::codegen::lir::LirCallOp& missing_arg_call =
      require_call_to(require_function(missing_arg_name, "call_pair"), "@make_pair");
  missing_arg_call.arg_type_refs[0] = c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(missing_arg_name);
    fail("verifier should reject a known struct call argument without StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  test_member_access_owner_tag_recovery_uses_structured_owner_key();
  test_member_access_owner_tag_recovery_rejects_stale_rendered_miss();
  test_member_access_owner_tag_recovery_preserves_no_owner_compatibility();

  std::cout << "PASS: frontend_lir_call_type_ref\n";
  return 0;
}
