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

c4c::codegen::lir::LirCallOp& require_indirect_call(
    c4c::codegen::lir::LirFunction& fn) {
  for (auto& block : fn.blocks) {
    for (auto& inst : block.insts) {
      auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (call && call->callee.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        return *call;
      }
    }
  }
  fail("fixture function should contain an indirect call");
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

void expect_indirect_int_signature(
    const c4c::codegen::lir::LirCallOp& call) {
  expect_true(call.callee_signature.has_value(),
              "metadata-rich indirect int call should carry callee signature");
  const c4c::codegen::lir::LirCallSignature& sig = *call.callee_signature;
  expect_true(sig.return_type_ref.has_value(),
              "indirect int call should carry structured return type ref");
  expect_eq(sig.return_type_ref->str(), "i32",
            "indirect int callee scalar return signature should match");
  expect_true(!sig.return_type_ref->has_struct_name_id(),
              "indirect int scalar return should not carry aggregate identity");
  expect_eq(std::to_string(sig.fixed_param_types.size()), "1",
            "indirect int call should carry one fixed signature param");
  expect_eq(sig.fixed_param_types[0], "i32",
            "indirect int call should retain fixed param ABI spelling");
  expect_eq(std::to_string(sig.fixed_param_type_refs.size()), "1",
            "indirect int call should carry one fixed signature param ref");
  expect_eq(sig.fixed_param_type_refs[0].str(), "i32",
            "indirect int call should carry fixed parameter type ref text");
  expect_true(!sig.fixed_param_type_refs[0].has_struct_name_id(),
              "indirect int parameter should not carry aggregate identity");
  expect_true(!sig.is_variadic, "indirect int call should not be variadic");
  expect_true(!sig.has_unspecified_params,
              "indirect int call should have a specified parameter list");
  expect_true(!sig.has_void_param_list,
              "indirect int call should not model a void parameter list");
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

int no_proto();

int call_no_proto(struct Pair value) {
  return no_proto(value);
}

int no_args(void);

int call_no_args(void) {
  return no_args();
}

int call_int_indirect(int (*fp)(int), int value) {
  return fp(value);
}

int call_variadic_indirect(int (*fp)(int, ...), struct Pair tail) {
  return fp(1, tail);
}

int call_unspecified_indirect(int (*fp)()) {
  return fp(1);
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
  expect_true(direct_call.callee_signature.has_value(),
              "metadata-rich direct call should carry structured callee signature");
  expect_true(direct_call.callee_signature->return_type_ref.has_value(),
              "direct call signature should carry structured return metadata");
  expect_struct_type_ref(lir_module, *direct_call.callee_signature->return_type_ref,
                         "%struct.Pair", "direct call signature return mirror");
  expect_eq(std::to_string(direct_call.callee_signature->fixed_param_types.size()), "1",
            "direct call signature should carry one fixed parameter");
  expect_struct_type_ref(lir_module, direct_call.callee_signature->fixed_param_type_refs[0],
                         "%struct.Pair", "direct call signature parameter mirror");
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
  expect_true(variadic_call.callee_signature.has_value(),
              "metadata-rich direct variadic call should carry callee signature");
  expect_true(variadic_call.callee_signature->is_variadic,
              "direct variadic call should carry variadic state");
  expect_eq(std::to_string(variadic_call.callee_signature->fixed_param_types.size()), "1",
            "direct variadic call should carry one fixed parameter");
  expect_eq(std::to_string(variadic_call.arg_type_refs.size()), "0",
            "variadic aggregate call should not carry argument mirrors when "
            "the call signature cannot parse against emitted ABI arguments");

  c4c::codegen::lir::LirFunction& call_no_proto =
      require_function(lir_module, "call_no_proto");
  c4c::codegen::lir::LirCallOp& no_proto_call =
      require_call_to(call_no_proto, "@no_proto");
  expect_true(no_proto_call.callee_signature.has_value(),
              "metadata-rich direct no-prototype call should carry callee signature");
  expect_true(no_proto_call.callee_signature->has_unspecified_params,
              "direct no-prototype call should carry unspecified-parameter-list state");
  expect_true(!no_proto_call.callee_signature->has_void_param_list,
              "direct no-prototype call should not be modeled as a void parameter list");
  expect_true(!no_proto_call.callee_signature->is_variadic,
              "direct no-prototype call should not be modeled as variadic");
  expect_true(no_proto_call.callee_signature->fixed_param_types.empty(),
              "direct no-prototype call should not invent fixed parameter mirrors");

  c4c::codegen::lir::LirFunction& call_no_args =
      require_function(lir_module, "call_no_args");
  c4c::codegen::lir::LirCallOp& no_args_call =
      require_call_to(call_no_args, "@no_args");
  expect_true(no_args_call.callee_signature.has_value(),
              "metadata-rich direct void-parameter call should carry callee signature");
  expect_true(no_args_call.callee_signature->has_void_param_list,
              "direct void-parameter call should keep void parameter list state");
  expect_true(!no_args_call.callee_signature->has_unspecified_params,
              "direct void-parameter call should not be modeled as unspecified");
  expect_true(no_args_call.callee_signature->fixed_param_types.empty(),
              "direct void-parameter call should not carry fixed parameter mirrors");

  c4c::codegen::lir::LirFunction& call_int_indirect =
      require_function(lir_module, "call_int_indirect");
  c4c::codegen::lir::LirCallOp& indirect_int_call =
      require_indirect_call(call_int_indirect);
  expect_indirect_int_signature(indirect_int_call);
  c4c::codegen::lir::verify_module(lir_module);

  const std::string indirect_formatted =
      c4c::codegen::lir::format_lir_call_site(indirect_int_call);
  expect_true(indirect_formatted.find("(i32) %") != std::string::npos,
              "indirect int call should preserve rendered function-pointer suffix");

  c4c::codegen::lir::LirModule stale_indirect_suffix = lir_module;
  c4c::codegen::lir::LirCallOp& stale_indirect_call =
      require_indirect_call(require_function(stale_indirect_suffix, "call_int_indirect"));
  stale_indirect_call.callee_type_suffix = "(ptr)";
  c4c::codegen::lir::verify_module(stale_indirect_suffix);

  c4c::codegen::lir::LirModule stale_direct_suffix = lir_module;
  c4c::codegen::lir::LirCallOp& stale_direct_call =
      require_call_to(require_function(stale_direct_suffix, "call_pair"), "@make_pair");
  stale_direct_call.callee_type_suffix = "(ptr)";
  c4c::codegen::lir::verify_module(stale_direct_suffix);

  c4c::codegen::lir::LirModule mismatched_indirect_sig = lir_module;
  c4c::codegen::lir::LirCallOp& mismatched_indirect_call =
      require_indirect_call(require_function(mismatched_indirect_sig, "call_int_indirect"));
  mismatched_indirect_call.callee_signature->fixed_param_types[0] = "ptr";
  mismatched_indirect_call.callee_signature->fixed_param_type_refs[0] =
      c4c::codegen::lir::LirTypeRef("ptr");
  try {
    c4c::codegen::lir::verify_module(mismatched_indirect_sig);
    fail("verifier should reject indirect callee signature that mismatches arguments");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirFunction& call_variadic_indirect =
      require_function(lir_module, "call_variadic_indirect");
  c4c::codegen::lir::LirCallOp& indirect_variadic_call =
      require_indirect_call(call_variadic_indirect);
  expect_true(indirect_variadic_call.callee_signature.has_value(),
              "metadata-rich indirect variadic call should carry callee signature");
  expect_true(indirect_variadic_call.callee_signature->is_variadic,
              "indirect variadic call should carry variadic state");
  expect_true(!indirect_variadic_call.callee_signature->has_unspecified_params,
              "indirect variadic call should not be modeled as unspecified");
  expect_eq(std::to_string(indirect_variadic_call.callee_signature->fixed_param_types.size()),
            "1",
            "indirect variadic call should carry one fixed parameter");
  expect_eq(indirect_variadic_call.callee_signature->fixed_param_types[0], "i32",
            "indirect variadic call should retain fixed i32 parameter type");

  c4c::codegen::lir::LirFunction& call_unspecified_indirect =
      require_function(lir_module, "call_unspecified_indirect");
  c4c::codegen::lir::LirCallOp& indirect_unspecified_call =
      require_indirect_call(call_unspecified_indirect);
  expect_true(indirect_unspecified_call.callee_signature.has_value(),
              "metadata-rich indirect unspecified call should carry callee signature");
  expect_true(indirect_unspecified_call.callee_signature->has_unspecified_params,
              "indirect unspecified call should carry unspecified-parameter-list state");
  expect_true(!indirect_unspecified_call.callee_signature->is_variadic,
              "indirect unspecified call should not be modeled as variadic");
  expect_true(indirect_unspecified_call.callee_signature->fixed_param_types.empty(),
              "indirect unspecified call should not invent fixed params");

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
