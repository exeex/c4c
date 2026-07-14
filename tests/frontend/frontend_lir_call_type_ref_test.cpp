#include "arena.hpp"
#include "call_args_ops.hpp"
#include "hir_to_lir.hpp"
#include "hir_to_lir/lowering.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"
#include "target_profile.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
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

void expect_contains(std::string_view text, std::string_view needle,
                     const std::string& msg) {
  if (text.find(needle) == std::string_view::npos) {
    fail(msg + "\nMissing: " + std::string(needle));
  }
}

void expect_not_contains(std::string_view text, std::string_view needle,
                         const std::string& msg) {
  if (text.find(needle) != std::string_view::npos) {
    fail(msg + "\nUnexpected: " + std::string(needle));
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

c4c::codegen::lir::LirModule lower_lir_module_for_target(
    std::string_view source, std::string_view target_triple) {
  c4c::hir::Module hir_module = lower_hir_module(source);
  hir_module.target_profile =
      c4c::target_profile_from_triple(std::string(target_triple));
  c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);
  c4c::codegen::lir::verify_module(lir_module);
  return lir_module;
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

c4c::codegen::lir::LirCallOp& require_nth_indirect_call(
    c4c::codegen::lir::LirFunction& fn,
    std::size_t wanted_index) {
  std::size_t index = 0;
  for (auto& block : fn.blocks) {
    for (auto& inst : block.insts) {
      auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (!call ||
          call->callee.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        continue;
      }
      if (index == wanted_index) return *call;
      ++index;
    }
  }
  fail("fixture function should contain indirect call index " +
       std::to_string(wanted_index));
}

c4c::hir::Expr& require_member_expr(c4c::hir::Module& module,
                                    std::string_view field) {
  for (c4c::hir::Expr& expr : module.expr_pool) {
    auto* member = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
    if (member && member->field == field) return expr;
  }
  fail("fixture should contain member expression for field " + std::string(field));
}

c4c::hir::Function& require_hir_function(c4c::hir::Module& module,
                                         std::string_view name,
                                         bool is_declaration) {
  const auto it = std::find_if(module.functions.begin(), module.functions.end(),
                               [&](const c4c::hir::Function& fn) {
                                 return fn.name == name &&
                                        fn.blocks.empty() == is_declaration;
                               });
  expect_true(it != module.functions.end(),
              "fixture function should exist in HIR: " + std::string(name));
  return *it;
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

void expect_structured_call_arg_matches_rendered(
    const c4c::codegen::lir::LirCallOp& call,
    std::size_t index,
    const std::string& msg) {
  const auto parsed_args = c4c::codegen::lir::parse_lir_typed_call_args(call.args_str);
  expect_true(parsed_args.has_value(), msg + " rendered args should parse");
  expect_true(index < parsed_args->size(), msg + " rendered arg index should exist");
  expect_true(index < call.structured_args.size(),
              msg + " structured arg index should exist");
  expect_eq(call.structured_args[index].type, (*parsed_args)[index].type,
            msg + " structured type should match rendered argument type");
  expect_eq(call.structured_args[index].operand.str(), (*parsed_args)[index].operand,
            msg + " structured operand should match rendered argument operand");
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

void test_call_type_ref_rejects_stale_rendered_owner_miss() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct StaleCallCompat {
  int value;
};

struct StaleCallCompat make_stale(struct StaleCallCompat input) {
  return input;
}

struct StaleCallCompat call_stale(struct StaleCallCompat value) {
  return make_stale(value);
}
)c");

  c4c::Node missing_owner =
      make_record_owner("StaleCallCompat",
                        hir_module.link_name_texts->intern("MissingCallOwner"),
                        808);
  auto make_missing_owner_type = [&]() {
    c4c::TypeSpec ts{};
    ts.base = c4c::TB_STRUCT;
    ts.tag_text_id = missing_owner.unqualified_text_id;
    ts.namespace_context_id = missing_owner.namespace_context_id;
    ts.record_def = &missing_owner;
    ts.array_size = -1;
    ts.inner_rank = -1;
    return ts;
  };

  c4c::hir::Function& make_stale =
      require_hir_function(hir_module, "make_stale", false);
  make_stale.return_type.spec = make_missing_owner_type();
  make_stale.params[0].type.spec = make_missing_owner_type();

  c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);
  c4c::codegen::lir::LirCallOp& call =
      require_call_to(require_function(lir_module, "call_stale"), "@make_stale");

  expect_eq(call.return_type.str(), "%struct.StaleCallCompat",
            "owner-key miss should keep rendered call return text");
  expect_true(!call.return_type.has_struct_name_id(),
              "complete owner-key miss must not produce a stale return StructNameId");
  expect_true(call.callee_signature.has_value(),
              "metadata-rich direct call should still carry callee signature facts");
  expect_true(call.callee_signature->return_type_ref.has_value(),
              "callee signature should keep raw return type text after owner miss");
  expect_true(!call.callee_signature->return_type_ref->has_struct_name_id(),
              "complete owner-key miss must not produce a stale callee return StructNameId");
  expect_eq(std::to_string(call.callee_signature->fixed_param_type_refs.size()), "1",
            "callee signature should keep one fixed parameter type ref");
  expect_true(!call.callee_signature->fixed_param_type_refs[0].has_struct_name_id(),
              "complete owner-key miss must not produce a stale parameter StructNameId");
  expect_true(call.arg_type_refs.empty(),
              "call argument mirrors should fail closed after a complete owner-key miss");
  expect_eq(std::to_string(call.structured_args.size()), "1",
            "call should retain raw structured argument facts");
  expect_eq(call.structured_args[0].type, "%struct.StaleCallCompat",
            "call argument should keep rendered text after owner miss");
  expect_true(!call.structured_args[0].type_ref.has_struct_name_id(),
              "complete owner-key miss must not produce a stale argument StructNameId");
}

void test_call_type_ref_preserves_no_owner_compatibility_name_id() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct StaleNoOwnerCallCompat {
  int value;
};

struct StaleNoOwnerCallCompat no_owner_make(struct StaleNoOwnerCallCompat input) {
  return input;
}

struct StaleNoOwnerCallCompat call_no_owner(struct StaleNoOwnerCallCompat value) {
  return no_owner_make(value);
}
)c");

  c4c::TypeSpec no_owner_type{};
  no_owner_type.base = c4c::TB_STRUCT;
  no_owner_type.tag_text_id =
      hir_module.link_name_texts->intern("StaleNoOwnerCallCompat");
  no_owner_type.namespace_context_id = -1;
  c4c::TextId incomplete_qualifier[] = {c4c::kInvalidText};
  no_owner_type.qualifier_text_ids = incomplete_qualifier;
  no_owner_type.n_qualifier_segments = 1;
  no_owner_type.array_size = -1;
  no_owner_type.inner_rank = -1;

  c4c::hir::Function& no_owner_make =
      require_hir_function(hir_module, "no_owner_make", false);
  no_owner_make.return_type.spec = no_owner_type;
  no_owner_make.params[0].type.spec = no_owner_type;

  c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);
  c4c::codegen::lir::LirCallOp& call =
      require_call_to(require_function(lir_module, "call_no_owner"), "@no_owner_make");

  expect_struct_type_ref(lir_module, call.return_type,
                         "%struct.StaleNoOwnerCallCompat",
                         "no-owner call return compatibility mirror");
  expect_true(call.callee_signature.has_value(),
              "no-owner direct call should carry callee signature facts");
  expect_struct_type_ref(lir_module, *call.callee_signature->return_type_ref,
                         "%struct.StaleNoOwnerCallCompat",
                         "no-owner callee return compatibility mirror");
  expect_struct_type_ref(lir_module,
                         call.callee_signature->fixed_param_type_refs[0],
                         "%struct.StaleNoOwnerCallCompat",
                         "no-owner callee parameter compatibility mirror");
  expect_eq(std::to_string(call.arg_type_refs.size()), "1",
            "no-owner call should keep one argument mirror");
  expect_struct_type_ref(lir_module, call.arg_type_refs[0],
                         "%struct.StaleNoOwnerCallCompat",
                         "no-owner call argument compatibility mirror");
}

void test_rv64_direct_variadic_integer_extension_attrs() {
  c4c::codegen::lir::LirModule lir_module = lower_lir_module_for_target(R"c(
int printf(const char *, ...);
unsigned int vf(const char *, ...);

int main(void) {
  return printf("%d\n", 7) + (int)vf("%d\n", 7);
}
)c", "riscv64-linux-gnu");

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_contains(llvm_ir, "declare signext i32 @printf(ptr, ...)",
                  "RV64 variadic signed extern return declaration should carry signext");
  expect_contains(llvm_ir, "call signext i32 (ptr, ...) @printf(",
                  "RV64 variadic signed call result should carry signext");
  expect_contains(llvm_ir, "declare zeroext i32 @vf(ptr, ...)",
                  "RV64 variadic unsigned extern return declaration should carry zeroext");
  expect_contains(llvm_ir, "call zeroext i32 (ptr, ...) @vf(",
                  "RV64 variadic unsigned call result should carry zeroext");
  expect_contains(llvm_ir, "i32 noundef signext 7",
                  "RV64 variadic integer argument should carry signext");
}

void test_rv64_scalar_stdarg_uses_pointer_cursor() {
  c4c::codegen::lir::LirModule lir_module = lower_lir_module_for_target(R"c(
typedef __builtin_va_list va_list;

static int pick_first_int(int count, ...) {
  va_list ap;
  __builtin_va_start(ap, count);
  int value = __builtin_va_arg(ap, int);
  __builtin_va_end(ap);
  return value;
}

int main(void) {
  return pick_first_int(1, 7);
}
)c", "riscv64-linux-gnu");

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_contains(llvm_ir, "%lv.ap = alloca ptr, align 8",
                  "RV64 scalar stdarg should allocate va_list as pointer storage");
  expect_contains(llvm_ir, "call void @llvm.va_start.p0(ptr %lv.ap)",
                  "RV64 scalar stdarg should pass pointer storage to va_start");
  expect_contains(llvm_ir, "load ptr, ptr %lv.ap",
                  "RV64 scalar stdarg should load the cursor");
  expect_contains(llvm_ir, "getelementptr inbounds i8, ptr %t0, i64 8",
                  "RV64 scalar stdarg should advance the cursor by one slot");
  expect_contains(llvm_ir, "store ptr %t1, ptr %lv.ap",
                  "RV64 scalar stdarg should store the advanced cursor");
  expect_contains(llvm_ir, "load i32, ptr %t0",
                  "RV64 scalar stdarg should load the scalar from the original cursor");
  expect_not_contains(llvm_ir, "%struct.__va_list_tag_",
                      "RV64 scalar stdarg should not use the AArch64 structured va_list");
}

void test_aarch64_scalar_stdarg_preserves_structured_va_list() {
  c4c::codegen::lir::LirModule lir_module = lower_lir_module_for_target(R"c(
typedef __builtin_va_list va_list;

static int pick_first_int(int count, ...) {
  va_list ap;
  __builtin_va_start(ap, count);
  int value = __builtin_va_arg(ap, int);
  __builtin_va_end(ap);
  return value;
}

int main(void) {
  return pick_first_int(1, 7);
}
)c", "aarch64-linux-gnu");

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_contains(llvm_ir, "%struct.__va_list_tag_",
                  "AArch64 scalar stdarg should preserve structured va_list lowering");
}

c4c::codegen::lir::LirFunction make_identity_test_function(
    std::string name, c4c::codegen::lir::LirValueId id) {
  namespace lir = c4c::codegen::lir;
  lir::LirFunction function;
  function.name = std::move(name);
  function.signature_text = "define void @" + function.name + "() {";
  function.blocks.push_back(lir::LirBlock{});
  function.blocks.back().label = "entry";
  function.blocks.back().insts.push_back(
      lir::LirStackSaveOp{lir::LirOperand::ssa("@misleading-result", id)});
  function.blocks.back().insts.push_back(
      lir::LirStackRestoreOp{lir::LirOperand::ssa("7", id)});
  return function;
}

void expect_identity_verification_rejected(
    const c4c::codegen::lir::LirModule& module,
    const std::string& message) {
  try {
    c4c::codegen::lir::verify_module(module);
    fail(message);
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }
}

void test_structured_operand_identity_foundation() {
  namespace lir = c4c::codegen::lir;

  const lir::LirOperand ssa =
      lir::LirOperand::ssa("@not-an-ssa-spelling", lir::LirValueId{4});
  const lir::LirOperand same_ssa =
      lir::LirOperand::ssa("different-display", lir::LirValueId{4});
  const lir::LirOperand global =
      lir::LirOperand::global("%not-a-global-spelling", c4c::LinkNameId{17});
  const lir::LirOperand integer = lir::LirOperand::integer("@not-an-integer", 7);
  const lir::LirOperand compatibility("%legacy-text");

  expect_true(ssa.kind() == lir::LirOperandKind::SsaValue,
              "SSA factory should set kind without classifying display");
  expect_true(global.kind() == lir::LirOperandKind::Global,
              "global factory should set kind without classifying display");
  expect_true(integer.kind() == lir::LirOperandKind::Immediate,
              "integer factory should set kind without classifying display");
  expect_true(ssa.value_id() && ssa.value_id()->value == 4,
              "SSA factory should preserve native value identity");
  expect_true(global.link_name_id() && *global.link_name_id() == 17,
              "global factory should preserve native link-name identity");
  expect_true(integer.integer_immediate() &&
                  integer.integer_immediate()->value == 7,
              "integer factory should preserve native immediate payload");
  expect_true(ssa.same_authority_as(same_ssa).value_or(false),
              "semantic equality should use authority rather than display");
  expect_true(!ssa.same_authority_as(compatibility).has_value(),
              "semantic equality should be unavailable for compatibility text");
  expect_true(!compatibility.has_authority(),
              "existing text construction should remain monostate compatibility");

  lir::LirModule valid;
  valid.functions.push_back(make_identity_test_function("first", lir::LirValueId{4}));
  lir::verify_module(valid);

  lir::LirModule invalid_result;
  invalid_result.functions.push_back(
      make_identity_test_function("invalid", lir::LirValueId::invalid()));
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid result identity");

  lir::LirModule duplicate;
  duplicate.functions.push_back(make_identity_test_function("duplicate", lir::LirValueId{5}));
  duplicate.functions.back().blocks.back().insts.insert(
      duplicate.functions.back().blocks.back().insts.begin() + 1,
      lir::LirStackSaveOp{lir::LirOperand::ssa("unrelated-display",
                                               lir::LirValueId{5})});
  expect_identity_verification_rejected(
      duplicate, "verifier should reject duplicate current-function identity");

  lir::LirModule unknown_use;
  unknown_use.functions.push_back(make_identity_test_function("unknown", lir::LirValueId{6}));
  auto* restore = std::get_if<lir::LirStackRestoreOp>(
      &unknown_use.functions.back().blocks.back().insts.back());
  expect_true(restore != nullptr, "identity fixture should end with stackrestore");
  restore->saved_ptr = lir::LirOperand::ssa("@misleading-use", lir::LirValueId{9});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown current-function identity");

  lir::LirModule independent_functions;
  independent_functions.functions.push_back(
      make_identity_test_function("left", lir::LirValueId{8}));
  independent_functions.functions.push_back(
      make_identity_test_function("right", lir::LirValueId{8}));
  lir::verify_module(independent_functions);
}

c4c::LinkNameId add_identity_test_global(
    c4c::codegen::lir::LirModule& module, std::string name) {
  namespace lir = c4c::codegen::lir;
  if (!module.link_name_texts) {
    module.link_name_texts = std::make_shared<c4c::TextTable>();
    module.link_names.attach_text_table(module.link_name_texts.get());
  }
  const c4c::LinkNameId id = module.link_names.intern(name);
  lir::LirGlobal global;
  global.name = std::move(name);
  global.link_name_id = id;
  module.globals.push_back(std::move(global));
  return id;
}

c4c::codegen::lir::LirFunction make_store_test_function(
    c4c::codegen::lir::LirStoreOp store) {
  namespace lir = c4c::codegen::lir;
  lir::LirFunction function;
  function.name = "store_test";
  function.signature_text = "define void @store_test() {";
  function.blocks.push_back(lir::LirBlock{});
  function.blocks.back().label = "entry";
  function.blocks.back().insts.push_back(std::move(store));
  return function;
}

c4c::codegen::lir::LirFunction make_load_test_function(
    c4c::codegen::lir::LirLoadOp load) {
  namespace lir = c4c::codegen::lir;
  lir::LirFunction function;
  function.name = "load_test";
  function.signature_text = "define void @load_test() {";
  function.blocks.push_back(lir::LirBlock{});
  function.blocks.back().label = "entry";
  function.blocks.back().insts.push_back(std::move(load));
  return function;
}

c4c::codegen::lir::LirFunction make_gep_test_function(
    c4c::codegen::lir::LirGepOp gep) {
  namespace lir = c4c::codegen::lir;
  lir::LirFunction function;
  function.name = "gep_test";
  function.signature_text = "define void @gep_test() {";
  function.blocks.push_back(lir::LirBlock{});
  function.blocks.back().label = "entry";
  function.blocks.back().insts.push_back(std::move(gep));
  return function;
}

c4c::codegen::lir::LirFunction make_return_test_function(
    std::string name, c4c::codegen::lir::LirRet ret) {
  namespace lir = c4c::codegen::lir;
  lir::LirFunction function;
  function.name = std::move(name);
  function.signature_text = "define i32 @" + function.name + "() {";
  function.blocks.push_back(lir::LirBlock{});
  function.blocks.back().label = "entry";
  function.blocks.back().terminator = std::move(ret);
  return function;
}

void test_global_store_identity_contract() {
  namespace lir = c4c::codegen::lir;

  const lir::LirOperand misleading_integer =
      lir::LirOperand::integer("not-the-coerced-presentation", 7);
  const lir::LirOperand preserved_integer =
      lir::stmt_emitter_detail::integer_store_operand_after_coercion(
          misleading_integer, "coerced-presentation", true);
  expect_eq(preserved_integer.str(), "coerced-presentation",
            "representation-preserving coercion should use its output as presentation");
  expect_true(preserved_integer.integer_immediate() &&
                  preserved_integer.integer_immediate()->value == 7,
              "misleading source display must not discard native integer authority");
  const lir::LirOperand changed_integer =
      lir::stmt_emitter_detail::integer_store_operand_after_coercion(
          misleading_integer, "converted-presentation", false);
  expect_true(!changed_integer.has_authority(),
              "representation-changing coercion should discard integer authority");

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int identity_store_primary;
int identity_store_neighbor;

int main(void) {
  identity_store_primary = 7;
  identity_store_neighbor = 0;
  return 0;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& main = require_function(lowered, "main");
  std::vector<lir::LirStoreOp*> stores;
  for (auto& block : main.blocks) {
    for (auto& inst : block.insts) {
      if (auto* store = std::get_if<lir::LirStoreOp>(&inst)) {
        stores.push_back(store);
      }
    }
  }
  expect_eq(std::to_string(stores.size()), "2",
            "global-store fixture should lower exactly two stores");
  expect_eq(stores[0]->type_str.str(), "i32",
            "focused store should retain i32 display/type");
  expect_eq(stores[0]->val.str(), "7",
            "focused store should retain integer display");
  expect_eq(stores[0]->ptr.str(), "@identity_store_primary",
            "focused store should retain global display");
  expect_true(stores[0]->val.integer_immediate() &&
                  stores[0]->val.integer_immediate()->value == 7,
              "focused store should retain native integer authority");
  expect_true(stores[0]->ptr.link_name_id() != nullptr,
              "focused store should retain global LinkNameId authority");
  expect_eq(lowered.link_names.spelling(*stores[0]->ptr.link_name_id()),
            "identity_store_primary",
            "focused store authority should resolve to selected global");
  expect_true(stores[1]->val.integer_immediate() &&
                  stores[1]->val.integer_immediate()->value == 0,
              "nearby zero store should retain native integer authority");
  expect_eq(lowered.link_names.spelling(*stores[1]->ptr.link_name_id()),
            "identity_store_neighbor",
            "nearby store should retain its independently selected global");

  lir::LirModule misleading;
  const c4c::LinkNameId first_id =
      add_identity_test_global(misleading, "display_target");
  const c4c::LinkNameId second_id =
      add_identity_test_global(misleading, "authority_target");
  misleading.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(32), lir::LirOperand::integer("7", 7),
      lir::LirOperand::global("@display_target", second_id)}));
  lir::verify_module(misleading);
  const auto& misleading_store = std::get<lir::LirStoreOp>(
      misleading.functions[0].blocks[0].insts[0]);
  expect_true(first_id != second_id &&
                  misleading_store.ptr.link_name_id() &&
                  *misleading_store.ptr.link_name_id() == second_id,
              "misleading display must not redirect valid global authority");

  lir::LirModule missing_ptr_authority;
  const c4c::LinkNameId missing_ptr_id =
      add_identity_test_global(missing_ptr_authority, "missing_ptr");
  (void)missing_ptr_id;
  missing_ptr_authority.functions.push_back(make_store_test_function(
      lir::LirStoreOp{lir::LirTypeRef::integer(32),
                      lir::LirOperand::integer("7", 7),
                      lir::LirOperand("@missing_ptr")}));
  expect_identity_verification_rejected(
      missing_ptr_authority,
      "verifier should reject global store pointer without authority");

  lir::LirModule invalid_id;
  invalid_id.link_name_texts = std::make_shared<c4c::TextTable>();
  invalid_id.link_names.attach_text_table(invalid_id.link_name_texts.get());
  invalid_id.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(32), lir::LirOperand::integer("7", 7),
      lir::LirOperand::global("@invalid", c4c::kInvalidLinkName)}));
  expect_identity_verification_rejected(
      invalid_id, "verifier should reject invalid global LinkNameId");

  lir::LirModule unresolved;
  unresolved.link_name_texts = std::make_shared<c4c::TextTable>();
  unresolved.link_names.attach_text_table(unresolved.link_name_texts.get());
  unresolved.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(32), lir::LirOperand::integer("7", 7),
      lir::LirOperand::global("@unresolved", c4c::LinkNameId{99})}));
  expect_identity_verification_rejected(
      unresolved, "verifier should reject unresolved global LinkNameId");

  lir::LirModule function_only;
  function_only.link_name_texts = std::make_shared<c4c::TextTable>();
  function_only.link_names.attach_text_table(function_only.link_name_texts.get());
  const c4c::LinkNameId function_id =
      function_only.link_names.intern("function_only");
  function_only.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(32), lir::LirOperand::integer("7", 7),
      lir::LirOperand::global("@function_only", function_id)}));
  function_only.functions.back().link_name_id = function_id;
  expect_identity_verification_rejected(
      function_only, "verifier should reject function-only LinkNameId");

  lir::LirModule ownerless;
  ownerless.link_name_texts = std::make_shared<c4c::TextTable>();
  ownerless.link_names.attach_text_table(ownerless.link_name_texts.get());
  const c4c::LinkNameId ownerless_id = ownerless.link_names.intern("ownerless");
  ownerless.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(32), lir::LirOperand::integer("7", 7),
      lir::LirOperand::global("@ownerless", ownerless_id)}));
  expect_identity_verification_rejected(
      ownerless, "verifier should reject ownerless global LinkNameId");

  lir::LirModule ambiguous;
  const c4c::LinkNameId ambiguous_id =
      add_identity_test_global(ambiguous, "ambiguous");
  lir::LirGlobal duplicate_global = ambiguous.globals.front();
  duplicate_global.name = "ambiguous_duplicate";
  ambiguous.globals.push_back(std::move(duplicate_global));
  ambiguous.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(32), lir::LirOperand::integer("7", 7),
      lir::LirOperand::global("@ambiguous", ambiguous_id)}));
  expect_identity_verification_rejected(
      ambiguous, "verifier should reject ambiguous global ownership");

  lir::LirModule missing_immediate;
  const c4c::LinkNameId missing_immediate_id =
      add_identity_test_global(missing_immediate, "missing_immediate");
  missing_immediate.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(32), lir::LirOperand("7"),
      lir::LirOperand::global("@missing_immediate", missing_immediate_id)}));
  expect_identity_verification_rejected(
      missing_immediate,
      "verifier should reject integer immediate without authority");

  lir::LirModule wrong_alternative;
  const c4c::LinkNameId wrong_alternative_id =
      add_identity_test_global(wrong_alternative, "wrong_alternative");
  wrong_alternative.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@wrong_value", wrong_alternative_id),
      lir::LirOperand::global("@wrong_alternative", wrong_alternative_id)}));
  expect_identity_verification_rejected(
      wrong_alternative, "verifier should reject wrong store value authority");

  lir::LirModule wrong_pointer_alternative;
  wrong_pointer_alternative.functions.push_back(make_store_test_function(
      lir::LirStoreOp{lir::LirTypeRef::integer(32),
                      lir::LirOperand::integer("7", 7),
                      lir::LirOperand::integer("@not-a-pointer", 0)}));
  expect_identity_verification_rejected(
      wrong_pointer_alternative,
      "verifier should reject wrong store pointer authority");

  lir::LirModule out_of_range;
  const c4c::LinkNameId out_of_range_id =
      add_identity_test_global(out_of_range, "out_of_range");
  out_of_range.functions.push_back(make_store_test_function(lir::LirStoreOp{
      lir::LirTypeRef::integer(8), lir::LirOperand::integer("256", 256),
      lir::LirOperand::global("@out_of_range", out_of_range_id)}));
  expect_identity_verification_rejected(
      out_of_range,
      "verifier should reject immediate outside narrow integer range");

  lir::LirModule noninteger_compatibility;
  const c4c::LinkNameId noninteger_id =
      add_identity_test_global(noninteger_compatibility, "float_compatibility");
  noninteger_compatibility.functions.push_back(make_store_test_function(
      lir::LirStoreOp{lir::LirTypeRef("double"), lir::LirOperand("1.0"),
                      lir::LirOperand::global("@float_compatibility",
                                              noninteger_id)}));
  lir::verify_module(noninteger_compatibility);

  lir::LirModule special_token_compatibility;
  const c4c::LinkNameId special_token_id =
      add_identity_test_global(special_token_compatibility,
                               "special_token_compatibility");
  special_token_compatibility.functions.push_back(make_store_test_function(
      lir::LirStoreOp{lir::LirTypeRef::integer(32), lir::LirOperand("undef"),
                      lir::LirOperand::global("@special_token_compatibility",
                                              special_token_id)}));
  lir::verify_module(special_token_compatibility);
}

void test_global_load_identity_contract() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int g_counter;
int g_neighbor;

int read_counter(void) { return g_counter; }
int read_pair(int choose) {
  if (choose) return g_counter;
  return g_neighbor;
}
int read_counter_again(void) { return g_counter; }
)c", "x86_64-linux-gnu");

  const auto loads_in = [](lir::LirFunction& function) {
    std::vector<lir::LirLoadOp*> loads;
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        if (auto* load = std::get_if<lir::LirLoadOp>(&inst)) {
          loads.push_back(load);
        }
      }
    }
    return loads;
  };

  lir::LirFunction& read_counter = require_function(lowered, "read_counter");
  const std::vector<lir::LirLoadOp*> counter_loads = loads_in(read_counter);
  expect_eq(std::to_string(counter_loads.size()), "1",
            "focused global-load fixture should lower exactly one load");
  expect_eq(counter_loads[0]->result.str(), "%t0",
            "focused global load should retain its LLVM result display");
  expect_eq(counter_loads[0]->type_str.str(), "i32",
            "focused global load should retain its LLVM type display");
  expect_eq(counter_loads[0]->ptr.str(), "@g_counter",
            "focused global load should retain its pointer display");
  expect_true(counter_loads[0]->result.value_id() &&
                  counter_loads[0]->result.value_id()->valid(),
              "focused global load should own a valid function-local result ID");
  expect_true(counter_loads[0]->ptr.link_name_id() != nullptr,
              "focused global load should retain selected-global authority");
  expect_eq(lowered.link_names.spelling(*counter_loads[0]->ptr.link_name_id()),
            "g_counter",
            "focused load pointer authority should resolve to g_counter");

  lir::LirFunction& read_pair = require_function(lowered, "read_pair");
  const std::vector<lir::LirLoadOp*> pair_loads = loads_in(read_pair);
  expect_eq(std::to_string(pair_loads.size()), "2",
            "nearby conditional fixture should lower two global loads");
  expect_true(pair_loads[0]->result.value_id() &&
                  pair_loads[1]->result.value_id() &&
                  *pair_loads[0]->result.value_id() !=
                      *pair_loads[1]->result.value_id(),
              "interleaved displays must not collapse function-local load IDs");
  expect_eq(lowered.link_names.spelling(*pair_loads[0]->ptr.link_name_id()),
            "g_counter",
            "first nearby load should retain its selected global");
  expect_eq(lowered.link_names.spelling(*pair_loads[1]->ptr.link_name_id()),
            "g_neighbor",
            "second nearby load should retain its distinct selected global");

  lir::LirFunction& read_again =
      require_function(lowered, "read_counter_again");
  const std::vector<lir::LirLoadOp*> again_loads = loads_in(read_again);
  expect_true(again_loads.size() == 1 && again_loads[0]->result.value_id() &&
                  *again_loads[0]->result.value_id() ==
                      *counter_loads[0]->result.value_id(),
              "the same numeric load ID should remain legal in separate functions");
  lir::verify_module(lowered);

  lir::LirModule misleading;
  const c4c::LinkNameId display_id =
      add_identity_test_global(misleading, "load_display_target");
  const c4c::LinkNameId authority_id =
      add_identity_test_global(misleading, "load_authority_target");
  misleading.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("@misleading-result", lir::LirValueId{4}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@load_display_target", authority_id)}));
  lir::verify_module(misleading);
  const auto& misleading_load = std::get<lir::LirLoadOp>(
      misleading.functions[0].blocks[0].insts[0]);
  expect_true(display_id != authority_id && misleading_load.result.value_id() &&
                  misleading_load.result.value_id()->value == 4 &&
                  misleading_load.ptr.link_name_id() &&
                  *misleading_load.ptr.link_name_id() == authority_id,
              "misleading load displays must not redirect native authority");

  lir::LirModule missing_result;
  const c4c::LinkNameId missing_result_id =
      add_identity_test_global(missing_result, "missing_load_result");
  missing_result.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand("%missing"), lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@missing_load_result", missing_result_id)}));
  expect_identity_verification_rejected(
      missing_result, "verifier should reject global load without result authority");

  lir::LirModule wrong_result;
  const c4c::LinkNameId wrong_result_id =
      add_identity_test_global(wrong_result, "wrong_load_result");
  wrong_result.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::global("%wrong", wrong_result_id),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@wrong_load_result", wrong_result_id)}));
  expect_identity_verification_rejected(
      wrong_result, "verifier should reject wrong global-load result authority");

  lir::LirModule missing_pointer;
  const c4c::LinkNameId missing_pointer_id =
      add_identity_test_global(missing_pointer, "missing_load_pointer");
  (void)missing_pointer_id;
  missing_pointer.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("%load", lir::LirValueId{1}),
      lir::LirTypeRef::integer(32), lir::LirOperand("@missing_load_pointer")}));
  expect_identity_verification_rejected(
      missing_pointer, "verifier should reject global load without pointer authority");

  lir::LirModule wrong_pointer;
  wrong_pointer.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("%load", lir::LirValueId{1}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::integer("@wrong-load-pointer", 0)}));
  expect_identity_verification_rejected(
      wrong_pointer, "verifier should reject wrong global-load pointer authority");

  lir::LirModule invalid_pointer;
  invalid_pointer.link_name_texts = std::make_shared<c4c::TextTable>();
  invalid_pointer.link_names.attach_text_table(invalid_pointer.link_name_texts.get());
  invalid_pointer.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("%load", lir::LirValueId{1}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@invalid-load", c4c::kInvalidLinkName)}));
  expect_identity_verification_rejected(
      invalid_pointer, "verifier should reject invalid global-load LinkNameId");

  lir::LirModule unresolved_pointer;
  unresolved_pointer.link_name_texts = std::make_shared<c4c::TextTable>();
  unresolved_pointer.link_names.attach_text_table(
      unresolved_pointer.link_name_texts.get());
  unresolved_pointer.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("%load", lir::LirValueId{1}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@unresolved-load", c4c::LinkNameId{99})}));
  expect_identity_verification_rejected(
      unresolved_pointer, "verifier should reject unresolved global-load LinkNameId");

  lir::LirModule function_only;
  function_only.link_name_texts = std::make_shared<c4c::TextTable>();
  function_only.link_names.attach_text_table(function_only.link_name_texts.get());
  const c4c::LinkNameId function_id =
      function_only.link_names.intern("load_function_only");
  function_only.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("%load", lir::LirValueId{1}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@load_function_only", function_id)}));
  function_only.functions.back().link_name_id = function_id;
  expect_identity_verification_rejected(
      function_only, "verifier should reject function-only global-load LinkNameId");

  lir::LirModule ownerless;
  ownerless.link_name_texts = std::make_shared<c4c::TextTable>();
  ownerless.link_names.attach_text_table(ownerless.link_name_texts.get());
  const c4c::LinkNameId ownerless_id =
      ownerless.link_names.intern("ownerless_load");
  ownerless.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("%load", lir::LirValueId{1}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@ownerless_load", ownerless_id)}));
  expect_identity_verification_rejected(
      ownerless, "verifier should reject ownerless global-load LinkNameId");

  lir::LirModule ambiguous;
  const c4c::LinkNameId ambiguous_id =
      add_identity_test_global(ambiguous, "ambiguous_load");
  lir::LirGlobal duplicate_global = ambiguous.globals.front();
  duplicate_global.name = "ambiguous_load_duplicate";
  ambiguous.globals.push_back(std::move(duplicate_global));
  ambiguous.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("%load", lir::LirValueId{1}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@ambiguous_load", ambiguous_id)}));
  expect_identity_verification_rejected(
      ambiguous, "verifier should reject ambiguous global-load ownership");

  lir::LirModule duplicate_result;
  const c4c::LinkNameId duplicate_result_id =
      add_identity_test_global(duplicate_result, "duplicate_load_result");
  duplicate_result.functions.push_back(make_load_test_function(lir::LirLoadOp{
      lir::LirOperand::ssa("%first", lir::LirValueId{2}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@duplicate_load_result", duplicate_result_id)}));
  duplicate_result.functions[0].blocks[0].insts.push_back(lir::LirLoadOp{
      lir::LirOperand::ssa("%second", lir::LirValueId{2}),
      lir::LirTypeRef::integer(32),
      lir::LirOperand::global("@duplicate_load_result", duplicate_result_id)});
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate global-load result IDs");
}

void test_return_identity_contract() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int return_source;
void return_void_helper(void) {}

int return_immediate(void) { return 7; }
unsigned int return_same_width(void) { return 7; }
unsigned char return_narrowed(void) { return 7; }
int return_loaded(void) { return return_source; }
int return_synthesized_zero(void) { return; }
void return_void_expression(void) { return return_void_helper(); }
)c", "x86_64-linux-gnu");

  const auto require_return = [](lir::LirFunction& function) -> lir::LirRet& {
    for (auto& block : function.blocks) {
      if (auto* ret = std::get_if<lir::LirRet>(&block.terminator)) return *ret;
    }
    fail("fixture function should contain a return terminator");
  };

  lir::LirRet& immediate =
      require_return(require_function(lowered, "return_immediate"));
  expect_true(immediate.value_str &&
                  immediate.value_str->integer_immediate() &&
                  immediate.value_str->integer_immediate()->value == 7 &&
                  immediate.type_str.kind() == lir::LirTypeKind::Integer &&
                  immediate.type_str.integer_bit_width() == 32,
              "ordinary integer literal return should retain native payload and type");

  lir::LirRet& same_width =
      require_return(require_function(lowered, "return_same_width"));
  expect_true(same_width.value_str &&
                  same_width.value_str->integer_immediate() &&
                  same_width.value_str->integer_immediate()->value == 7,
              "same-representation signedness coercion should emit no cast and retain authority");

  lir::LirFunction& narrowed_function =
      require_function(lowered, "return_narrowed");
  lir::LirRet& narrowed = require_return(narrowed_function);
  expect_true(narrowed.value_str && !narrowed.value_str->has_authority(),
              "representation-changing return coercion must remain raw compatibility");
  bool saw_narrowing_cast = false;
  for (const auto& block : narrowed_function.blocks) {
    saw_narrowing_cast = saw_narrowing_cast ||
        std::any_of(block.insts.begin(), block.insts.end(), [](const lir::LirInst& inst) {
          return std::holds_alternative<lir::LirCastOp>(inst);
        });
  }
  expect_true(saw_narrowing_cast,
              "representation-changing return fixture should prove an emitted cast path");

  lir::LirFunction& loaded_function =
      require_function(lowered, "return_loaded");
  lir::LirLoadOp* load = nullptr;
  for (auto& block : loaded_function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* candidate = std::get_if<lir::LirLoadOp>(&inst)) load = candidate;
    }
  }
  lir::LirRet& loaded = require_return(loaded_function);
  expect_true(load && load->result.value_id() && loaded.value_str &&
                  loaded.value_str->value_id() &&
                  *load->result.value_id() == *loaded.value_str->value_id(),
              "scalar load return should reuse the defining function-local value ID");

  lir::LirRet& synthesized = require_return(
      require_function(lowered, "return_synthesized_zero"));
  expect_true(synthesized.value_str &&
                  synthesized.value_str->integer_immediate() &&
                  synthesized.value_str->integer_immediate()->value == 0,
              "synthesized scalar integer return should carry native zero authority");
  lir::LirRet& void_expression = require_return(
      require_function(lowered, "return_void_expression"));
  expect_true(!void_expression.value_str &&
                  void_expression.type_str.kind() == lir::LirTypeKind::Void,
              "void return expression should emit its side effect without a value");
  lir::verify_module(lowered);

  lir::LirModule misleading;
  lir::LirFunction misleading_function = make_return_test_function(
      "misleading_return",
      lir::LirRet{lir::LirOperand::ssa("@display-is-not-an-id",
                                       lir::LirValueId{4}),
                  lir::LirTypeRef::integer(32)});
  misleading_function.blocks[0].insts.push_back(lir::LirStackSaveOp{
      lir::LirOperand::ssa("7", lir::LirValueId{4})});
  misleading.functions.push_back(std::move(misleading_function));
  lir::verify_module(misleading);
  expect_contains(lir::print_llvm(misleading),
                  "ret i32 @display-is-not-an-id",
                  "return printer should preserve presentation after native-ID verification");

  lir::LirModule misleading_immediate;
  misleading_immediate.functions.push_back(make_return_test_function(
      "misleading_immediate",
      lir::LirRet{lir::LirOperand::integer("999", 7),
                  lir::LirTypeRef::integer(8)}));
  lir::verify_module(misleading_immediate);
  expect_contains(lir::print_llvm(misleading_immediate), "ret i8 999",
                  "return verifier should range-check native payload, while printer preserves display");

  lir::LirModule void_compatibility;
  void_compatibility.functions.push_back(make_return_test_function(
      "void_compatibility", lir::LirRet{std::nullopt, lir::LirTypeRef("void")}));
  lir::verify_module(void_compatibility);

  lir::LirModule raw_compatibility;
  raw_compatibility.functions.push_back(make_return_test_function(
      "raw_return", lir::LirRet{lir::LirOperand("%legacy"),
                                lir::LirTypeRef("i32")}));
  lir::verify_module(raw_compatibility);

  lir::LirModule void_with_value;
  void_with_value.functions.push_back(make_return_test_function(
      "void_with_value", lir::LirRet{lir::LirOperand::integer("0", 0),
                                     lir::LirTypeRef("void")}));
  expect_identity_verification_rejected(
      void_with_value, "verifier should reject a value on a void return");

  lir::LirModule missing_value;
  missing_value.functions.push_back(make_return_test_function(
      "missing_value", lir::LirRet{std::nullopt, lir::LirTypeRef::integer(32)}));
  expect_identity_verification_rejected(
      missing_value, "verifier should reject a missing non-void return value");

  lir::LirModule unsupported_authority;
  unsupported_authority.functions.push_back(make_return_test_function(
      "unsupported_authority",
      lir::LirRet{lir::LirOperand::global("7", c4c::LinkNameId{1}),
                  lir::LirTypeRef::integer(32)}));
  expect_identity_verification_rejected(
      unsupported_authority, "verifier should reject LinkNameId return authority");

  lir::LirModule out_of_range;
  out_of_range.functions.push_back(make_return_test_function(
      "out_of_range_return",
      lir::LirRet{lir::LirOperand::integer("0", 256),
                  lir::LirTypeRef::integer(8)}));
  expect_identity_verification_rejected(
      out_of_range, "verifier should reject out-of-range return immediate authority");

  lir::LirModule unknown_ssa;
  unknown_ssa.functions.push_back(make_return_test_function(
      "unknown_return",
      lir::LirRet{lir::LirOperand::ssa("7", lir::LirValueId{9}),
                  lir::LirTypeRef::integer(32)}));
  expect_identity_verification_rejected(
      unknown_ssa, "verifier should reject unknown current-function return ID");

  lir::LirModule cross_function;
  cross_function.functions.push_back(
      make_identity_test_function("return_owner", lir::LirValueId{7}));
  cross_function.functions.push_back(make_return_test_function(
      "cross_function_return",
      lir::LirRet{lir::LirOperand::ssa("%cross", lir::LirValueId{7}),
                  lir::LirTypeRef::integer(32)}));
  expect_identity_verification_rejected(
      cross_function, "verifier should reject cross-function return ID");

  lir::LirModule type_parity;
  type_parity.functions.push_back(make_return_test_function(
      "return_type_parity",
      lir::LirRet{lir::LirOperand::integer("7", 7),
                  lir::LirTypeRef("i32", lir::LirTypeKind::Integer, 64)}));
  expect_identity_verification_rejected(
      type_parity, "verifier should reject return type-ref mirror disagreement");
}

void test_global_array_gep_identity_contract() {
  namespace lir = c4c::codegen::lir;

  const auto zero_index = [] {
    return lir::LirGepIndex::typed(lir::LirTypeRef::integer(64),
                                   lir::LirOperand::integer("0", 0));
  };
  const auto authoritative_gep = [&](c4c::LinkNameId base_id,
                                     lir::LirOperand result,
                                     std::vector<lir::LirGepIndex> indices) {
    return lir::LirGepOp{std::move(result), lir::LirTypeRef("[1 x i32]"),
                         lir::LirOperand::global("@gep_base", base_id), false,
                         std::move(indices)};
  };

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_identity_array[1];
int lir_identity_array_neighbor[4];

int *array_address(void) { return lir_identity_array; }
int *neighbor_address(void) { return lir_identity_array_neighbor; }
)c", "x86_64-linux-gnu");

  lir::LirFunction& array_address = require_function(lowered, "array_address");
  lir::LirGepOp* first_gep = nullptr;
  bool cast_before_gep = false;
  for (auto& block : array_address.blocks) {
    for (auto& inst : block.insts) {
      if (!first_gep && std::holds_alternative<lir::LirCastOp>(inst)) {
        cast_before_gep = true;
      }
      if (!first_gep) first_gep = std::get_if<lir::LirGepOp>(&inst);
    }
  }
  expect_true(first_gep != nullptr,
              "global-array fixture should lower an ordinary decay GEP");
  expect_true(!cast_before_gep,
              "ordinary global-array decay should not insert a preceding cast");
  expect_eq(first_gep->result.str(), "%t0",
            "focused array GEP should retain its LLVM result display");
  expect_eq(first_gep->element_type.str(), "[1 x i32]",
            "focused array GEP should retain its aggregate type display");
  expect_eq(first_gep->ptr.str(), "@lir_identity_array",
            "focused array GEP should retain its global base display");
  expect_true(first_gep->result.value_id() &&
                  first_gep->result.value_id()->valid(),
              "focused array GEP should own a valid function-local result ID");
  expect_true(first_gep->ptr.link_name_id() &&
                  lowered.link_names.spelling(*first_gep->ptr.link_name_id()) ==
                      "lir_identity_array",
              "focused array GEP should retain selected-global authority");
  expect_eq(std::to_string(first_gep->indices.size()), "2",
            "ordinary array decay should carry two structured indices");
  for (const lir::LirGepIndex& index : first_gep->indices) {
    expect_true(index.is_authoritative() &&
                    index.type_ref().kind() == lir::LirTypeKind::Integer &&
                    index.type_ref().integer_bit_width() == 64 &&
                    index.value().integer_immediate() &&
                    index.value().integer_immediate()->value == 0,
                "array-decay index should carry typed native i64 zero authority");
  }
  expect_contains(lir::print_llvm(lowered),
                  "%t0 = getelementptr [1 x i32], ptr @lir_identity_array, i64 0, i64 0",
                  "typed GEP printer should preserve exact LLVM presentation");

  lir::LirFunction& neighbor_address =
      require_function(lowered, "neighbor_address");
  lir::LirGepOp* neighbor_gep = nullptr;
  for (auto& block : neighbor_address.blocks) {
    for (auto& inst : block.insts) {
      if (auto* gep = std::get_if<lir::LirGepOp>(&inst)) neighbor_gep = gep;
    }
  }
  expect_true(neighbor_gep && neighbor_gep->ptr.link_name_id() &&
                  neighbor_gep->element_type.str() == "[4 x i32]" &&
                  lowered.link_names.spelling(*neighbor_gep->ptr.link_name_id()) ==
                      "lir_identity_array_neighbor",
              "nearby array extent should retain independent structured GEP facts");
  lir::verify_module(lowered);

  lir::LirModule ssa_index;
  const c4c::LinkNameId ssa_base_id =
      add_identity_test_global(ssa_index, "ssa_index_base");
  lir::LirFunction ssa_function = make_gep_test_function(lir::LirGepOp{
      lir::LirOperand::ssa("@misleading-result", lir::LirValueId{4}),
      lir::LirTypeRef("[8 x i32]"),
      lir::LirOperand::global("%misleading-base", ssa_base_id), false,
      {lir::LirGepIndex::typed(
          lir::LirTypeRef::integer(64),
          lir::LirOperand::ssa("@misleading-index", lir::LirValueId{3}))}});
  ssa_function.blocks[0].insts.insert(
      ssa_function.blocks[0].insts.begin(),
      lir::LirStackSaveOp{
          lir::LirOperand::ssa("7", lir::LirValueId{3})});
  ssa_index.functions.push_back(std::move(ssa_function));
  lir::verify_module(ssa_index);

  lir::LirModule missing_result;
  const c4c::LinkNameId missing_result_id =
      add_identity_test_global(missing_result, "missing_gep_result");
  missing_result.functions.push_back(make_gep_test_function(authoritative_gep(
      missing_result_id, lir::LirOperand("%missing"),
      {zero_index(), zero_index()})));
  expect_identity_verification_rejected(
      missing_result, "verifier should reject authoritative GEP without result ID");

  lir::LirModule invalid_result;
  const c4c::LinkNameId invalid_result_id =
      add_identity_test_global(invalid_result, "invalid_gep_result");
  invalid_result.functions.push_back(make_gep_test_function(authoritative_gep(
      invalid_result_id,
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid()),
      {zero_index()})));
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid authoritative GEP result ID");

  lir::LirModule duplicate_result;
  const c4c::LinkNameId duplicate_id =
      add_identity_test_global(duplicate_result, "duplicate_gep_result");
  duplicate_result.functions.push_back(make_gep_test_function(authoritative_gep(
      duplicate_id, lir::LirOperand::ssa("%first", lir::LirValueId{5}),
      {zero_index()})));
  duplicate_result.functions[0].blocks[0].insts.push_back(authoritative_gep(
      duplicate_id, lir::LirOperand::ssa("%second", lir::LirValueId{5}),
      {zero_index()}));
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate authoritative GEP result IDs");

  lir::LirModule missing_base;
  const c4c::LinkNameId missing_base_id =
      add_identity_test_global(missing_base, "missing_gep_base");
  (void)missing_base_id;
  lir::LirGepOp missing_base_gep = authoritative_gep(
      missing_base_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {zero_index()});
  missing_base_gep.ptr = lir::LirOperand("@missing_gep_base");
  missing_base.functions.push_back(
      make_gep_test_function(std::move(missing_base_gep)));
  expect_identity_verification_rejected(
      missing_base, "verifier should reject authoritative GEP without base ID");

  lir::LirModule unresolved_base;
  unresolved_base.link_name_texts = std::make_shared<c4c::TextTable>();
  unresolved_base.link_names.attach_text_table(unresolved_base.link_name_texts.get());
  unresolved_base.functions.push_back(make_gep_test_function(authoritative_gep(
      c4c::LinkNameId{99}, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {zero_index()})));
  expect_identity_verification_rejected(
      unresolved_base, "verifier should reject unresolved authoritative GEP base ID");

  lir::LirModule function_only;
  function_only.link_name_texts = std::make_shared<c4c::TextTable>();
  function_only.link_names.attach_text_table(function_only.link_name_texts.get());
  const c4c::LinkNameId function_id =
      function_only.link_names.intern("gep_function_only");
  function_only.functions.push_back(make_gep_test_function(authoritative_gep(
      function_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {zero_index()})));
  function_only.functions.back().link_name_id = function_id;
  expect_identity_verification_rejected(
      function_only, "verifier should reject function-only authoritative GEP base ID");

  lir::LirModule ownerless;
  ownerless.link_name_texts = std::make_shared<c4c::TextTable>();
  ownerless.link_names.attach_text_table(ownerless.link_name_texts.get());
  const c4c::LinkNameId ownerless_id = ownerless.link_names.intern("ownerless_gep");
  ownerless.functions.push_back(make_gep_test_function(authoritative_gep(
      ownerless_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {zero_index()})));
  expect_identity_verification_rejected(
      ownerless, "verifier should reject ownerless authoritative GEP base ID");

  lir::LirModule ambiguous;
  const c4c::LinkNameId ambiguous_id =
      add_identity_test_global(ambiguous, "ambiguous_gep");
  lir::LirGlobal duplicate_global = ambiguous.globals.front();
  duplicate_global.name = "ambiguous_gep_duplicate";
  ambiguous.globals.push_back(std::move(duplicate_global));
  ambiguous.functions.push_back(make_gep_test_function(authoritative_gep(
      ambiguous_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {zero_index()})));
  expect_identity_verification_rejected(
      ambiguous, "verifier should reject ambiguous authoritative GEP base ownership");

  lir::LirModule empty_indices;
  const c4c::LinkNameId empty_id =
      add_identity_test_global(empty_indices, "empty_gep_indices");
  empty_indices.functions.push_back(make_gep_test_function(authoritative_gep(
      empty_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}), {})));
  expect_identity_verification_rejected(
      empty_indices, "verifier should reject empty authoritative GEP indices");

  lir::LirModule raw_index;
  const c4c::LinkNameId raw_id =
      add_identity_test_global(raw_index, "raw_gep_index");
  raw_index.functions.push_back(make_gep_test_function(authoritative_gep(
      raw_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {zero_index(), lir::LirGepIndex::raw("i64 0")})));
  expect_identity_verification_rejected(
      raw_index, "verifier should reject raw index inside authoritative GEP");

  lir::LirModule noninteger_index;
  const c4c::LinkNameId noninteger_id =
      add_identity_test_global(noninteger_index, "noninteger_gep_index");
  noninteger_index.functions.push_back(make_gep_test_function(authoritative_gep(
      noninteger_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {lir::LirGepIndex::typed(lir::LirTypeRef("double"),
                               lir::LirOperand::integer("0", 0))})));
  expect_identity_verification_rejected(
      noninteger_index, "verifier should reject noninteger authoritative GEP index");

  lir::LirModule global_index;
  const c4c::LinkNameId global_index_id =
      add_identity_test_global(global_index, "global_gep_index");
  global_index.functions.push_back(make_gep_test_function(authoritative_gep(
      global_index_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {lir::LirGepIndex::typed(
          lir::LirTypeRef::integer(64),
          lir::LirOperand::global("0", global_index_id))})));
  expect_identity_verification_rejected(
      global_index, "verifier should reject global authority as a GEP index");

  lir::LirModule overflow_index;
  const c4c::LinkNameId overflow_id =
      add_identity_test_global(overflow_index, "overflow_gep_index");
  overflow_index.functions.push_back(make_gep_test_function(authoritative_gep(
      overflow_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {lir::LirGepIndex::typed(lir::LirTypeRef::integer(8),
                               lir::LirOperand::integer("256", 256))})));
  expect_identity_verification_rejected(
      overflow_index, "verifier should reject unrepresentable GEP immediate");

  lir::LirModule unknown_index;
  const c4c::LinkNameId unknown_id =
      add_identity_test_global(unknown_index, "unknown_gep_index");
  unknown_index.functions.push_back(make_gep_test_function(authoritative_gep(
      unknown_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {lir::LirGepIndex::typed(
          lir::LirTypeRef::integer(64),
          lir::LirOperand::ssa("%unknown", lir::LirValueId{9}))})));
  expect_identity_verification_rejected(
      unknown_index, "verifier should reject unknown current-function GEP index ID");

  lir::LirModule cross_function;
  const c4c::LinkNameId cross_id =
      add_identity_test_global(cross_function, "cross_function_gep_index");
  cross_function.functions.push_back(
      make_identity_test_function("index_owner", lir::LirValueId{7}));
  cross_function.functions.push_back(make_gep_test_function(authoritative_gep(
      cross_id, lir::LirOperand::ssa("%gep", lir::LirValueId{1}),
      {lir::LirGepIndex::typed(
          lir::LirTypeRef::integer(64),
          lir::LirOperand::ssa("%cross", lir::LirValueId{7}))})));
  expect_identity_verification_rejected(
      cross_function, "verifier should reject cross-function GEP index ID");

  lir::LirModule raw_compatibility;
  raw_compatibility.functions.push_back(make_gep_test_function(lir::LirGepOp{
      lir::LirOperand("%raw"), lir::LirTypeRef("i8"),
      lir::LirOperand("%base"), false,
      {lir::LirGepIndex::raw("i64 0")}}));
  lir::verify_module(raw_compatibility);
}

void test_direct_scalar_result_call_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_direct_scalar_result_target(void) { return 7; }
int lir_direct_scalar_result_call_identity(void) {
  return lir_direct_scalar_result_target();
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& caller =
      require_function(lowered, "lir_direct_scalar_result_call_identity");
  std::vector<lir::LirCallOp*> calls;
  for (auto& block : caller.blocks) {
    for (auto& inst : block.insts) {
      if (auto* call = std::get_if<lir::LirCallOp>(&inst)) calls.push_back(call);
    }
  }
  expect_eq(std::to_string(calls.size()), "1",
            "scalar-result probe should lower exactly one call");
  const lir::LirCallOp& call = *calls[0];
  expect_true(call.direct_callee_link_name_id != c4c::kInvalidLinkName,
              "direct scalar call should retain native callee identity");
  const auto target = std::find_if(
      lowered.functions.begin(), lowered.functions.end(),
      [&](const lir::LirFunction& function) {
        return function.link_name_id == call.direct_callee_link_name_id;
      });
  expect_true(target != lowered.functions.end() &&
                  target->name == "lir_direct_scalar_result_target",
              "direct callee ID should resolve to the selected function");
  expect_true(call.return_type.kind() == lir::LirTypeKind::Integer &&
                  call.return_type.integer_bit_width() == 32,
              "scalar call should retain native i32 return type");
  expect_true(call.callee_signature &&
                  call.callee_signature->return_type_ref &&
                  call.callee_signature->return_type_ref->kind() ==
                      lir::LirTypeKind::Integer &&
                  call.callee_signature->return_type_ref->integer_bit_width() == 32 &&
                  call.callee_signature->fixed_param_type_refs.empty() &&
                  call.callee_signature->has_void_param_list &&
                  !call.callee_signature->is_variadic &&
                  !call.callee_signature->has_unspecified_params,
              "direct scalar call should retain its structured callee contract");
  lir::LirRet* return_value = nullptr;
  for (auto& block : caller.blocks) {
    if (auto* ret = std::get_if<lir::LirRet>(&block.terminator)) {
      return_value = ret;
    }
  }
  expect_true(call.result.kind() == lir::LirOperandKind::SsaValue &&
                  call.result.value_id() && call.result.value_id()->valid(),
              "direct scalar call result should own a valid native value ID");
  expect_true(return_value && return_value->value_str &&
                  return_value->value_str->value_id() &&
                  *return_value->value_str->value_id() ==
                      *call.result.value_id(),
              "direct scalar call return should preserve the exact result ID");

  const auto scalar_signature = [] {
    lir::LirCallSignature signature;
    signature.return_type_ref = lir::LirTypeRef::integer(32);
    signature.has_void_param_list = true;
    return signature;
  };
  const auto scalar_call = [&](lir::LirOperand result) {
    return lir::LirCallOp{std::move(result), lir::LirTypeRef::integer(32),
                          lir::LirOperand("@direct_scalar_target"),
                          c4c::LinkNameId{1}, "", "", {},
                          scalar_signature(), {}};
  };
  const auto scalar_function = [&](std::string name, lir::LirCallOp call_op,
                                   lir::LirOperand returned) {
    lir::LirFunction function;
    function.name = std::move(name);
    function.signature_text = "define i32 @" + function.name + "() {";
    function.blocks.push_back(lir::LirBlock{});
    function.blocks.back().label = "entry";
    function.blocks.back().insts.push_back(std::move(call_op));
    function.blocks.back().terminator =
        lir::LirRet{std::move(returned), lir::LirTypeRef::integer(32)};
    return function;
  };

  lir::LirModule misleading;
  misleading.functions.push_back(scalar_function(
      "misleading_call_result",
      scalar_call(lir::LirOperand::ssa("@not-result-display",
                                       lir::LirValueId{4})),
      lir::LirOperand::ssa("7", lir::LirValueId{4})));
  lir::verify_module(misleading);

  lir::LirModule missing_return_ref;
  lir::LirCallOp missing_return_ref_call =
      scalar_call(lir::LirOperand::ssa("%typed", lir::LirValueId{6}));
  missing_return_ref_call.callee_signature->return_type_ref.reset();
  missing_return_ref.functions.push_back(scalar_function(
      "missing_call_return_ref", std::move(missing_return_ref_call),
      lir::LirOperand::ssa("%typed-return", lir::LirValueId{6})));
  expect_identity_verification_rejected(
      missing_return_ref,
      "verifier should reject direct scalar call without structured return ref");

  lir::LirModule missing;
  missing.functions.push_back(scalar_function(
      "missing_call_result", scalar_call(lir::LirOperand("%missing")),
      lir::LirOperand::integer("0", 0)));
  expect_identity_verification_rejected(
      missing, "verifier should reject direct scalar call without result authority");

  lir::LirModule invalid;
  invalid.functions.push_back(scalar_function(
      "invalid_call_result",
      scalar_call(lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid())),
      lir::LirOperand::integer("0", 0)));
  expect_identity_verification_rejected(
      invalid, "verifier should reject invalid direct scalar call result ID");

  lir::LirModule duplicate;
  duplicate.functions.push_back(scalar_function(
      "duplicate_call_result",
      scalar_call(lir::LirOperand::ssa("%first", lir::LirValueId{5})),
      lir::LirOperand::ssa("%first-return", lir::LirValueId{5})));
  duplicate.functions[0].blocks[0].insts.push_back(
      scalar_call(lir::LirOperand::ssa("%second", lir::LirValueId{5})));
  expect_identity_verification_rejected(
      duplicate, "verifier should reject duplicate direct scalar call result IDs");

  lir::LirModule unknown_use;
  unknown_use.functions.push_back(scalar_function(
      "unknown_call_result_use",
      scalar_call(lir::LirOperand::ssa("%defined", lir::LirValueId{2})),
      lir::LirOperand::ssa("%unknown", lir::LirValueId{9})));
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown downstream call result ID");

  lir::LirModule cross_function;
  cross_function.functions.push_back(scalar_function(
      "call_result_owner",
      scalar_call(lir::LirOperand::ssa("%owned", lir::LirValueId{7})),
      lir::LirOperand::ssa("%owned-return", lir::LirValueId{7})));
  cross_function.functions.push_back(make_return_test_function(
      "cross_function_call_result",
      lir::LirRet{lir::LirOperand::ssa("%cross", lir::LirValueId{7}),
                  lir::LirTypeRef::integer(32)}));
  expect_identity_verification_rejected(
      cross_function, "verifier should reject cross-function call result use");

  lir::LirCallSignature void_signature;
  void_signature.return_type_ref = lir::LirTypeRef("void");
  void_signature.has_void_param_list = true;
  lir::LirFunction void_function;
  void_function.name = "void_result_authority";
  void_function.signature_text = "define void @void_result_authority() {";
  void_function.blocks.push_back(lir::LirBlock{});
  void_function.blocks.back().label = "entry";
  void_function.blocks.back().insts.push_back(lir::LirCallOp{
      lir::LirOperand::ssa("%forbidden", lir::LirValueId{1}),
      lir::LirTypeRef("void"), lir::LirOperand("@void_target"),
      c4c::LinkNameId{2}, "", "", {}, void_signature, {}});
  void_function.blocks.back().terminator = lir::LirRet{};
  lir::LirModule void_result;
  void_result.functions.push_back(std::move(void_function));
  expect_identity_verification_rejected(
      void_result, "verifier should reject result authority on a void call");
}

void test_direct_void_immediate_arg_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
void lir_direct_void_immediate_arg_target(int value);
void lir_direct_void_immediate_arg_identity(void) {
  lir_direct_void_immediate_arg_target(7);
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& caller =
      require_function(lowered, "lir_direct_void_immediate_arg_identity");
  std::vector<lir::LirCallOp*> calls;
  for (auto& block : caller.blocks) {
    for (auto& inst : block.insts) {
      if (auto* call = std::get_if<lir::LirCallOp>(&inst)) calls.push_back(call);
    }
  }
  expect_eq(std::to_string(calls.size()), "1",
            "immediate-argument probe should lower exactly one call");
  const lir::LirCallOp& call = *calls[0];
  expect_true(call.direct_callee_link_name_id != c4c::kInvalidLinkName &&
                  call.return_type.kind() == lir::LirTypeKind::Void,
              "void immediate call should retain native direct-target and return facts");
  expect_true(call.callee_signature &&
                  call.callee_signature->fixed_param_type_refs.size() == 1 &&
                  call.callee_signature->fixed_param_type_refs[0].kind() ==
                      lir::LirTypeKind::Integer &&
                  call.callee_signature->fixed_param_type_refs[0].integer_bit_width() == 32 &&
                  !call.callee_signature->is_variadic &&
                  !call.callee_signature->has_unspecified_params,
              "void immediate call should retain one fixed native i32 parameter");
  expect_true(call.arg_type_refs.size() == 1 &&
                  call.arg_type_refs[0].kind() == lir::LirTypeKind::Integer &&
                  call.arg_type_refs[0].integer_bit_width() == 32 &&
                  call.structured_args.size() == 1 &&
                  call.structured_args[0].type_ref == call.arg_type_refs[0] &&
                  call.structured_args[0].type_ref ==
                      call.callee_signature->fixed_param_type_refs[0],
              "fixed immediate argument should retain exact native i32 type authority");
  const lir::LirOperand& argument = call.structured_args[0].operand;
  expect_true(argument.kind() == lir::LirOperandKind::Immediate &&
                  argument.integer_immediate() &&
                  argument.integer_immediate()->value == 7 &&
                  call.structured_args[0].ext_attr == lir::LirExtAttr::None &&
                  call.result.empty() && !call.result.has_authority(),
              "fixed immediate call should retain payload authority without a result or extension");

  const auto require_focused_call = [](lir::LirModule& module) -> lir::LirCallOp& {
    lir::LirFunction& function =
        require_function(module, "lir_direct_void_immediate_arg_identity");
    lir::LirCallOp* found = nullptr;
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCallOp>(&inst)) {
          expect_true(found == nullptr,
                      "focused immediate fixture should contain only one call");
          found = candidate;
        }
      }
    }
    expect_true(found != nullptr,
                "focused immediate fixture should contain a structured call");
    return *found;
  };

  lir::LirModule misleading = lowered;
  lir::LirCallOp& misleading_call = require_focused_call(misleading);
  misleading_call.args_str = "rendered arguments are not authority";
  misleading_call.callee_type_suffix = "(rendered type is not authority)";
  misleading_call.structured_args[0].type = "rendered-arg-type";
  misleading_call.structured_args[0].operand.str() = "@rendered-not-immediate";
  misleading_call.callee_signature->fixed_param_types[0] = "rendered-param-type";
  lir::verify_module(misleading);

  lir::LirModule missing_payload = lowered;
  require_focused_call(missing_payload).structured_args[0].operand =
      lir::LirOperand("7");
  expect_identity_verification_rejected(
      missing_payload, "verifier should reject fixed immediate without payload authority");

  lir::LirModule wrong_alternative = lowered;
  require_focused_call(wrong_alternative).structured_args[0].operand =
      lir::LirOperand::global("7", c4c::LinkNameId{9});
  expect_identity_verification_rejected(
      wrong_alternative, "verifier should reject fixed immediate with global authority");

  lir::LirModule invalid_authority = lowered;
  require_focused_call(invalid_authority).structured_args[0].operand =
      lir::LirOperand::ssa("7", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_authority, "verifier should reject invalid argument value authority");

  lir::LirModule out_of_range = lowered;
  require_focused_call(out_of_range).structured_args[0].operand =
      lir::LirOperand::integer("7", 1LL << 40);
  expect_identity_verification_rejected(
      out_of_range, "verifier should reject immediate outside fixed parameter range");

  lir::LirModule type_conflict = lowered;
  require_focused_call(type_conflict).structured_args[0].type_ref =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      type_conflict, "verifier should reject structured argument type conflict");

  lir::LirModule signature_conflict = lowered;
  require_focused_call(signature_conflict)
      .callee_signature->fixed_param_type_refs[0] = lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      signature_conflict, "verifier should reject fixed signature type conflict");

  lir::LirModule count_conflict = lowered;
  lir::LirCallOp& count_call = require_focused_call(count_conflict);
  count_call.structured_args.push_back(count_call.structured_args[0]);
  expect_identity_verification_rejected(
      count_conflict, "verifier should reject extra structured immediate argument");

  lir::LirModule extension_conflict = lowered;
  require_focused_call(extension_conflict).structured_args[0].ext_attr =
      lir::LirExtAttr::SignExt;
  expect_identity_verification_rejected(
      extension_conflict, "verifier should reject extension on fixed immediate argument");

  lir::LirModule character_compatibility = lower_lir_module_for_target(R"c(
void lir_direct_void_character_compatibility_target(int value);
void lir_direct_void_character_compatibility(void) {
  lir_direct_void_character_compatibility_target('a');
}
)c", "x86_64-linux-gnu");
  lir::LirFunction& character_caller = require_function(
      character_compatibility, "lir_direct_void_character_compatibility");
  lir::LirCallOp* character_call = nullptr;
  for (auto& block : character_caller.blocks) {
    for (auto& inst : block.insts) {
      if (auto* candidate = std::get_if<lir::LirCallOp>(&inst)) {
        expect_true(character_call == nullptr,
                    "character compatibility fixture should contain one call");
        character_call = candidate;
      }
    }
  }
  expect_true(character_call && character_call->structured_args.size() == 1 &&
                  character_call->structured_args[0].operand.kind() ==
                      lir::LirOperandKind::Immediate &&
                  !character_call->structured_args[0].operand.has_authority() &&
                  character_call->structured_args[0].type_ref.empty() &&
                  character_call->arg_type_refs.empty(),
              "integer-looking character compatibility text must not enter the native immediate claim");
}

void test_direct_void_ssa_arg_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_direct_void_ssa_arg_source;
void lir_direct_void_ssa_arg_target(int value);
void lir_direct_void_ssa_arg_identity(void) {
  lir_direct_void_ssa_arg_target(lir_direct_void_ssa_arg_source);
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& caller =
      require_function(lowered, "lir_direct_void_ssa_arg_identity");
  std::vector<lir::LirLoadOp*> loads;
  std::vector<lir::LirCallOp*> calls;
  for (auto& block : caller.blocks) {
    for (auto& inst : block.insts) {
      if (auto* load = std::get_if<lir::LirLoadOp>(&inst)) loads.push_back(load);
      if (auto* call = std::get_if<lir::LirCallOp>(&inst)) calls.push_back(call);
    }
  }
  expect_true(loads.size() == 1 && calls.size() == 1,
              "SSA-argument probe should lower one selected-global load and one call");
  expect_true(loads[0]->result.value_id() &&
                  loads[0]->result.value_id()->valid() &&
                  loads[0]->ptr.link_name_id() &&
                  lowered.link_names.spelling(*loads[0]->ptr.link_name_id()) ==
                      "lir_direct_void_ssa_arg_source",
              "selected-global load should retain the closed CC-LOAD-1 authority");
  const lir::LirCallOp& call = *calls[0];
  expect_true(call.direct_callee_link_name_id != c4c::kInvalidLinkName &&
                  call.return_type.kind() == lir::LirTypeKind::Void &&
                  call.callee_signature &&
                  call.callee_signature->fixed_param_type_refs.size() == 1 &&
                  call.callee_signature->fixed_param_type_refs[0].kind() ==
                      lir::LirTypeKind::Integer &&
                  call.callee_signature->fixed_param_type_refs[0].integer_bit_width() == 32 &&
                  !call.callee_signature->is_variadic &&
                  !call.callee_signature->has_unspecified_params &&
                  call.arg_type_refs.size() == 1 &&
                  call.arg_type_refs[0].kind() == lir::LirTypeKind::Integer &&
                  call.arg_type_refs[0].integer_bit_width() == 32 &&
                  call.structured_args.size() == 1 &&
                  call.structured_args[0].type_ref == call.arg_type_refs[0] &&
                  call.structured_args[0].type_ref ==
                      call.callee_signature->fixed_param_type_refs[0],
              "SSA call should retain one exact fixed native i32 argument contract");
  const lir::LirOperand& argument = call.structured_args[0].operand;
  expect_true(argument.kind() == lir::LirOperandKind::SsaValue &&
                  argument.value_id() &&
                  *argument.value_id() == *loads[0]->result.value_id() &&
                  call.structured_args[0].ext_attr == lir::LirExtAttr::None &&
                  call.result.empty() && !call.result.has_authority(),
              "fixed SSA call argument should reuse the exact selected-global load result ID");
  lir::verify_module(lowered);

  const auto require_focused_call = [](lir::LirModule& module) -> lir::LirCallOp& {
    lir::LirFunction& function =
        require_function(module, "lir_direct_void_ssa_arg_identity");
    lir::LirCallOp* found = nullptr;
    for (auto& block : function.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCallOp>(&inst)) {
          expect_true(found == nullptr,
                      "focused SSA fixture should contain only one call");
          found = candidate;
        }
      }
    }
    expect_true(found != nullptr,
                "focused SSA fixture should contain a structured call");
    return *found;
  };

  lir::LirModule misleading = lowered;
  lir::LirCallOp& misleading_call = require_focused_call(misleading);
  misleading_call.args_str = "rendered arguments are not authority";
  misleading_call.callee_type_suffix = "(rendered type is not authority)";
  misleading_call.structured_args[0].type = "rendered-arg-type";
  misleading_call.structured_args[0].operand.str() = "@rendered-not-ssa";
  misleading_call.callee_signature->fixed_param_types[0] =
      "rendered-param-type";
  lir::verify_module(misleading);

  lir::LirModule missing_authority = lowered;
  require_focused_call(missing_authority).structured_args[0].operand =
      lir::LirOperand("%missing");
  expect_identity_verification_rejected(
      missing_authority,
      "verifier should reject fixed SSA argument without value authority");

  lir::LirModule wrong_alternative = lowered;
  require_focused_call(wrong_alternative).structured_args[0].operand =
      lir::LirOperand::global("%wrong", *loads[0]->ptr.link_name_id());
  expect_identity_verification_rejected(
      wrong_alternative,
      "verifier should reject fixed SSA argument with global authority");

  lir::LirModule invalid_authority = lowered;
  require_focused_call(invalid_authority).structured_args[0].operand =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_authority,
      "verifier should reject invalid fixed SSA argument value authority");

  lir::LirModule unknown_authority = lowered;
  require_focused_call(unknown_authority).structured_args[0].operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_authority,
      "verifier should reject unknown fixed SSA argument value authority");

  lir::LirModule cross_function = lowered;
  cross_function.functions.push_back(
      make_identity_test_function("ssa_argument_owner", lir::LirValueId{99}));
  require_focused_call(cross_function).structured_args[0].operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function,
      "verifier should reject cross-function fixed SSA argument authority");

  lir::LirModule type_conflict = lowered;
  require_focused_call(type_conflict).structured_args[0].type_ref =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      type_conflict, "verifier should reject structured SSA argument type conflict");

  lir::LirModule signature_conflict = lowered;
  require_focused_call(signature_conflict)
      .callee_signature->fixed_param_type_refs[0] = lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      signature_conflict, "verifier should reject fixed SSA signature type conflict");

  lir::LirModule count_conflict = lowered;
  lir::LirCallOp& count_call = require_focused_call(count_conflict);
  count_call.structured_args.push_back(count_call.structured_args[0]);
  expect_identity_verification_rejected(
      count_conflict, "verifier should reject extra structured SSA argument");

  lir::LirModule extension_conflict = lowered;
  require_focused_call(extension_conflict).structured_args[0].ext_attr =
      lir::LirExtAttr::SignExt;
  expect_identity_verification_rejected(
      extension_conflict, "verifier should reject extension on fixed SSA argument");
}

void test_scalar_ordinary_value_chain_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_scalar_ordinary_value_chain_source;
int lir_scalar_ordinary_value_chain_identity(void) {
  return (lir_scalar_ordinary_value_chain_source + 1) * 2;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_ordinary_value_chain_identity");
  std::vector<lir::LirLoadOp*> loads;
  std::vector<lir::LirBinOp*> binary_ops;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* load = std::get_if<lir::LirLoadOp>(&inst)) loads.push_back(load);
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
    }
  }
  expect_true(loads.size() == 1 && loads[0]->result.value_id() &&
                  loads[0]->result.value_id()->valid(),
              "scalar-chain source load should retain its neighboring native result ID");
  expect_eq(std::to_string(binary_ops.size()), "2",
            "scalar-chain probe should lower exactly two binary operations");
  expect_true(binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::Add &&
                  binary_ops[1]->opcode.typed() == lir::LirBinaryOpcode::Mul &&
                  binary_ops[0]->type_str.kind() == lir::LirTypeKind::Integer &&
                  binary_ops[0]->type_str.integer_bit_width() == 32 &&
                  binary_ops[1]->type_str.kind() == lir::LirTypeKind::Integer &&
                  binary_ops[1]->type_str.integer_bit_width() == 32,
              "scalar chain should retain native opcode and i32 type facts");
  expect_true(binary_ops[0]->result.kind() == lir::LirOperandKind::SsaValue &&
                  binary_ops[0]->result.value_id() &&
                  binary_ops[0]->result.value_id()->valid() &&
                  binary_ops[1]->lhs.kind() == lir::LirOperandKind::SsaValue &&
                  binary_ops[1]->lhs.value_id() &&
                  *binary_ops[1]->lhs.value_id() ==
                      *binary_ops[0]->result.value_id() &&
                  binary_ops[1]->result.kind() == lir::LirOperandKind::SsaValue &&
                  binary_ops[1]->result.value_id() &&
                  binary_ops[1]->result.value_id()->valid() &&
                  *binary_ops[1]->result.value_id() !=
                      *binary_ops[0]->result.value_id(),
              "scalar chain should preserve the exact first result ID into the later use");
  lir::verify_module(lowered);

  const auto require_focused_binary_ops =
      [](lir::LirModule& module) -> std::vector<lir::LirBinOp*> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_ordinary_value_chain_identity");
    std::vector<lir::LirBinOp*> found;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
          found.push_back(binary);
        }
      }
    }
    expect_eq(std::to_string(found.size()), "2",
              "focused scalar-chain fixture should contain two binary operations");
    return found;
  };

  lir::LirModule misleading = lowered;
  std::vector<lir::LirBinOp*> misleading_ops =
      require_focused_binary_ops(misleading);
  misleading_ops[0]->result.str() = "@rendered-not-result";
  misleading_ops[1]->lhs.str() = "7";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_binary_ops(invalid_result)[0]->result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid scalar binary result ID");

  lir::LirModule duplicate_result = lowered;
  std::vector<lir::LirBinOp*> duplicate_ops =
      require_focused_binary_ops(duplicate_result);
  duplicate_ops[1]->result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_ops[0]->result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate scalar binary result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_binary_ops(unknown_use)[1]->lhs =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown scalar binary operand ID");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_binary_owner", lir::LirValueId{99}));
  require_focused_binary_ops(cross_function_use)[1]->lhs =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function scalar binary operand ID");

  lir::LirModule invalid_opcode = lowered;
  require_focused_binary_ops(invalid_opcode)[0]->opcode =
      lir::LirBinaryOpcodeRef("not-a-binary-opcode");
  expect_identity_verification_rejected(
      invalid_opcode, "verifier should reject invalid scalar binary opcode");

  lir::LirModule missing_type = lowered;
  require_focused_binary_ops(missing_type)[0]->type_str = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_type, "verifier should reject missing scalar binary type authority");
}

void test_scalar_floating_binary_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
double lir_scalar_floating_binary_result_use_identity(void) {
  return (1.25 + 2.5) * 4.0;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function = require_function(
      lowered, "lir_scalar_floating_binary_result_use_identity");
  std::vector<lir::LirBinOp*> binary_ops;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
    }
  }
  expect_true(binary_ops.size() == 2,
              "scalar-floating probe should lower exactly two binary operations");
  expect_true(binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::FAdd &&
                  binary_ops[1]->opcode.typed() == lir::LirBinaryOpcode::FMul &&
                  binary_ops[0]->type_str.kind() == lir::LirTypeKind::Floating &&
                  binary_ops[0]->type_str.str() == "double" &&
                  binary_ops[1]->type_str.kind() == lir::LirTypeKind::Floating &&
                  binary_ops[1]->type_str.str() == "double" &&
                  binary_ops[0]->result.value_id() &&
                  binary_ops[0]->result.value_id()->valid() &&
                  binary_ops[1]->lhs.value_id() &&
                  *binary_ops[1]->lhs.value_id() ==
                      *binary_ops[0]->result.value_id() &&
                  binary_ops[1]->result.value_id() &&
                  binary_ops[1]->result.value_id()->valid() &&
                  *binary_ops[1]->result.value_id() !=
                      *binary_ops[0]->result.value_id(),
              "scalar floating chain should retain native opcodes, exact type, and result/use IDs");
  expect_true(!binary_ops[0]->lhs.has_authority() &&
                  !binary_ops[0]->rhs.has_authority() &&
                  !binary_ops[1]->rhs.has_authority(),
              "floating literals should remain honest monostate compatibility operands");
  lir::verify_module(lowered);

  const auto require_focused_binary_ops =
      [](lir::LirModule& module) -> std::vector<lir::LirBinOp*> {
    lir::LirFunction& focused = require_function(
        module, "lir_scalar_floating_binary_result_use_identity");
    std::vector<lir::LirBinOp*> found;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
          found.push_back(binary);
        }
      }
    }
    expect_true(found.size() == 2,
                "focused scalar-floating fixture should contain two binary operations");
    return found;
  };

  lir::LirModule misleading = lowered;
  std::vector<lir::LirBinOp*> misleading_ops =
      require_focused_binary_ops(misleading);
  misleading_ops[0]->result.str() = "@rendered-not-floating-result";
  misleading_ops[1]->lhs.str() = "7";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_binary_ops(invalid_result)[0]->result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result,
      "verifier should reject invalid scalar floating binary result ID");

  lir::LirModule duplicate_result = lowered;
  std::vector<lir::LirBinOp*> duplicate_ops =
      require_focused_binary_ops(duplicate_result);
  duplicate_ops[1]->result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_ops[0]->result.value_id());
  expect_identity_verification_rejected(
      duplicate_result,
      "verifier should reject duplicate scalar floating binary result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_binary_ops(unknown_use)[1]->lhs =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown scalar floating binary use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_floating_binary_owner",
                                  lir::LirValueId{99}));
  require_focused_binary_ops(cross_function_use)[1]->lhs =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function scalar floating binary use");

  lir::LirModule invalid_opcode = lowered;
  require_focused_binary_ops(invalid_opcode)[0]->opcode =
      lir::LirBinaryOpcodeRef("not-a-binary-opcode");
  expect_identity_verification_rejected(
      invalid_opcode,
      "verifier should reject invalid scalar floating binary opcode");

  lir::LirModule conflicting_opcode = lowered;
  require_focused_binary_ops(conflicting_opcode)[0]->opcode =
      lir::LirBinaryOpcode::Add;
  expect_identity_verification_rejected(
      conflicting_opcode,
      "verifier should reject integer opcode on authoritative floating binary type");

  lir::LirModule missing_type = lowered;
  require_focused_binary_ops(missing_type)[0]->type_str = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_type,
      "verifier should reject missing scalar floating binary type authority");

  lir::LirModule conflicting_type = lowered;
  require_focused_binary_ops(conflicting_type)[0]->type_str =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_type,
      "verifier should reject integer type on authoritative floating binary opcode");
}

void test_scalar_cast_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_scalar_cast_result_use_source;
long long lir_scalar_cast_result_use_identity(void) {
  return (long long)lir_scalar_cast_result_use_source + 1LL;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_cast_result_use_identity");
  std::vector<lir::LirLoadOp*> loads;
  std::vector<lir::LirCastOp*> casts;
  std::vector<lir::LirBinOp*> binary_ops;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* load = std::get_if<lir::LirLoadOp>(&inst)) loads.push_back(load);
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) casts.push_back(cast);
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
    }
  }
  expect_true(loads.size() == 1 && casts.size() == 1 &&
                  binary_ops.size() == 1,
              "scalar-cast probe should lower one load, cast, and later binary use");
  expect_true(casts[0]->kind == lir::LirCastKind::SExt &&
                  casts[0]->from_type.kind() == lir::LirTypeKind::Integer &&
                  casts[0]->from_type.integer_bit_width() == 32 &&
                  casts[0]->to_type.kind() == lir::LirTypeKind::Integer &&
                  casts[0]->to_type.integer_bit_width() == 64 &&
                  binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::Add &&
                  binary_ops[0]->type_str.kind() == lir::LirTypeKind::Integer &&
                  binary_ops[0]->type_str.integer_bit_width() == 64,
              "scalar-cast chain should retain native sext, endpoint, and Add facts");
  expect_true(loads[0]->result.value_id() && casts[0]->operand.value_id() &&
                  *casts[0]->operand.value_id() ==
                      *loads[0]->result.value_id() &&
                  casts[0]->result.value_id() &&
                  casts[0]->result.value_id()->valid() &&
                  binary_ops[0]->lhs.value_id() &&
                  *binary_ops[0]->lhs.value_id() ==
                      *casts[0]->result.value_id(),
              "later scalar use should preserve the exact authoritative cast result ID");
  lir::verify_module(lowered);

  const auto require_focused_cast = [](lir::LirModule& module)
      -> std::pair<lir::LirCastOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_cast_result_use_identity");
    lir::LirCastOp* cast = nullptr;
    lir::LirBinOp* binary = nullptr;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(cast == nullptr,
                      "focused scalar-cast fixture should contain one cast");
          cast = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          expect_true(binary == nullptr,
                      "focused scalar-cast fixture should contain one binary use");
          binary = candidate;
        }
      }
    }
    expect_true(cast && binary,
                "focused scalar-cast fixture should contain its cast/use pair");
    return {*cast, *binary};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_cast, misleading_use] = require_focused_cast(misleading);
  misleading_cast.result.str() = "@rendered-not-cast-result";
  misleading_use.lhs.str() = "7";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_cast(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid scalar cast result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_cast, duplicate_use] = require_focused_cast(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_cast.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate scalar cast result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_cast(unknown_use).second.lhs =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown scalar cast result use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_cast_owner", lir::LirValueId{99}));
  require_focused_cast(cross_function_use).second.lhs =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function scalar cast result use");

  lir::LirModule invalid_kind = lowered;
  require_focused_cast(invalid_kind).first.kind =
      static_cast<lir::LirCastKind>(255);
  expect_identity_verification_rejected(
      invalid_kind, "verifier should reject invalid native cast kind");

  lir::LirModule missing_from_type = lowered;
  require_focused_cast(missing_from_type).first.from_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_from_type, "verifier should reject missing cast source type authority");

  lir::LirModule missing_to_type = lowered;
  require_focused_cast(missing_to_type).first.to_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_to_type,
      "verifier should reject missing cast destination type authority");

  lir::LirModule conflicting_from_type = lowered;
  require_focused_cast(conflicting_from_type).first.from_type =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_from_type,
      "verifier should reject cast source type conflicting with extension kind");

  lir::LirModule conflicting_to_type = lowered;
  require_focused_cast(conflicting_to_type).first.to_type =
      lir::LirTypeRef::integer(16);
  expect_identity_verification_rejected(
      conflicting_to_type,
      "verifier should reject cast destination type conflicting with extension kind");
}

void test_scalar_fptrunc_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
float lir_scalar_fptrunc_result_use_identity(void) {
  return (float)(1.25 + 2.5) * 4.0f;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_fptrunc_result_use_identity");
  std::vector<lir::LirBinOp*> binary_ops;
  lir::LirCastOp* trunc = nullptr;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) trunc = cast;
    }
  }
  expect_true(binary_ops.size() == 2 && trunc &&
                  binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::FAdd &&
                  binary_ops[0]->type_str.kind() == lir::LirTypeKind::Floating &&
                  binary_ops[0]->type_str.str() == "double" &&
                  binary_ops[0]->result.value_id() &&
                  trunc->kind == lir::LirCastKind::FPTrunc &&
                  trunc->from_type.kind() == lir::LirTypeKind::Floating &&
                  trunc->from_type.str() == "double" &&
                  trunc->to_type.kind() == lir::LirTypeKind::Floating &&
                  trunc->to_type.str() == "float" &&
                  trunc->operand.value_id() &&
                  *trunc->operand.value_id() ==
                      *binary_ops[0]->result.value_id() &&
                  trunc->result.value_id() && trunc->result.value_id()->valid() &&
                  binary_ops[1]->opcode.typed() == lir::LirBinaryOpcode::FMul &&
                  binary_ops[1]->type_str.kind() == lir::LirTypeKind::Floating &&
                  binary_ops[1]->type_str.str() == "float" &&
                  binary_ops[1]->lhs.value_id() &&
                  *binary_ops[1]->lhs.value_id() == *trunc->result.value_id(),
              "explicit FPTrunc should consume and produce exact floating result/use IDs");
  lir::verify_module(lowered);

  const auto require_focused_cast = [](lir::LirModule& module)
      -> std::pair<lir::LirCastOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_fptrunc_result_use_identity");
    lir::LirCastOp* found_cast = nullptr;
    std::vector<lir::LirBinOp*> found_binary_ops;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(found_cast == nullptr,
                      "focused FPTrunc fixture should contain one cast");
          found_cast = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          found_binary_ops.push_back(candidate);
        }
      }
    }
    expect_true(found_cast && found_cast->result.value_id(),
                "focused FPTrunc fixture should contain an authoritative cast");
    lir::LirBinOp* found_use = nullptr;
    for (lir::LirBinOp* binary : found_binary_ops) {
      if (binary->lhs.value_id() &&
          *binary->lhs.value_id() == *found_cast->result.value_id()) {
        found_use = binary;
      }
    }
    expect_true(found_use,
                "focused FPTrunc fixture should contain its later floating use");
    return {*found_cast, *found_use};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_cast, misleading_use] = require_focused_cast(misleading);
  misleading_cast.operand.str() = "@rendered-not-fptrunc-source";
  misleading_cast.result.str() = "7";
  misleading_use.lhs.str() = "@rendered-not-fptrunc-result";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_cast(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid FPTrunc result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_cast, duplicate_use] = require_focused_cast(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_cast.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate FPTrunc result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_cast(unknown_use).first.operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown FPTrunc source use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_fptrunc_owner", lir::LirValueId{99}));
  require_focused_cast(cross_function_use).first.operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use, "verifier should reject cross-function FPTrunc source use");

  lir::LirModule wrong_kind = lowered;
  require_focused_cast(wrong_kind).first.kind = lir::LirCastKind::FPExt;
  expect_identity_verification_rejected(
      wrong_kind, "verifier should reject FPExt on the authoritative FPTrunc route");

  lir::LirModule missing_from_type = lowered;
  require_focused_cast(missing_from_type).first.from_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_from_type, "verifier should reject missing FPTrunc source type");

  lir::LirModule missing_to_type = lowered;
  require_focused_cast(missing_to_type).first.to_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_to_type, "verifier should reject missing FPTrunc destination type");

  lir::LirModule conflicting_endpoint = lowered;
  require_focused_cast(conflicting_endpoint).first.from_type =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_endpoint,
      "verifier should reject nonfloating FPTrunc endpoint authority");

  lir::LirModule conflicting_direction = lowered;
  auto [direction_cast, direction_use] =
      require_focused_cast(conflicting_direction);
  direction_cast.from_type = lir::LirTypeRef("float");
  direction_cast.to_type = lir::LirTypeRef("double");
  direction_use.type_str = lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_direction,
      "verifier should reject nonnarrowing authoritative FPTrunc endpoints");
}

void test_scalar_fpext_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
double lir_scalar_fpext_result_use_identity(void) {
  return (double)(1.25f + 2.5f) * 4.0;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_fpext_result_use_identity");
  std::vector<lir::LirBinOp*> binary_ops;
  lir::LirCastOp* ext = nullptr;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) ext = cast;
    }
  }
  expect_true(binary_ops.size() == 2 && ext &&
                  binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::FAdd &&
                  binary_ops[0]->type_str.kind() == lir::LirTypeKind::Floating &&
                  binary_ops[0]->type_str.str() == "float" &&
                  binary_ops[0]->result.value_id() &&
                  ext->kind == lir::LirCastKind::FPExt &&
                  ext->from_type.kind() == lir::LirTypeKind::Floating &&
                  ext->from_type.str() == "float" &&
                  ext->to_type.kind() == lir::LirTypeKind::Floating &&
                  ext->to_type.str() == "double" && ext->operand.value_id() &&
                  *ext->operand.value_id() ==
                      *binary_ops[0]->result.value_id() &&
                  ext->result.value_id() && ext->result.value_id()->valid() &&
                  binary_ops[1]->opcode.typed() == lir::LirBinaryOpcode::FMul &&
                  binary_ops[1]->type_str.kind() == lir::LirTypeKind::Floating &&
                  binary_ops[1]->type_str.str() == "double" &&
                  binary_ops[1]->lhs.value_id() &&
                  *binary_ops[1]->lhs.value_id() == *ext->result.value_id(),
              "explicit FPExt should consume and produce exact floating result/use IDs");
  lir::verify_module(lowered);

  const auto require_focused_cast = [](lir::LirModule& module)
      -> std::pair<lir::LirCastOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_fpext_result_use_identity");
    lir::LirCastOp* found_cast = nullptr;
    std::vector<lir::LirBinOp*> found_binary_ops;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(found_cast == nullptr,
                      "focused FPExt fixture should contain one cast");
          found_cast = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          found_binary_ops.push_back(candidate);
        }
      }
    }
    expect_true(found_cast && found_cast->result.value_id(),
                "focused FPExt fixture should contain an authoritative cast");
    lir::LirBinOp* found_use = nullptr;
    for (lir::LirBinOp* binary : found_binary_ops) {
      if (binary->lhs.value_id() &&
          *binary->lhs.value_id() == *found_cast->result.value_id()) {
        found_use = binary;
      }
    }
    expect_true(found_use,
                "focused FPExt fixture should contain its later floating use");
    return {*found_cast, *found_use};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_cast, misleading_use] = require_focused_cast(misleading);
  misleading_cast.operand.str() = "@rendered-not-fpext-source";
  misleading_cast.result.str() = "7";
  misleading_use.lhs.str() = "@rendered-not-fpext-result";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_cast(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid FPExt result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_cast, duplicate_use] = require_focused_cast(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_cast.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate FPExt result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_cast(unknown_use).first.operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown FPExt source use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_fpext_owner", lir::LirValueId{99}));
  require_focused_cast(cross_function_use).first.operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use, "verifier should reject cross-function FPExt source use");

  lir::LirModule wrong_kind = lowered;
  require_focused_cast(wrong_kind).first.kind = lir::LirCastKind::FPTrunc;
  expect_identity_verification_rejected(
      wrong_kind, "verifier should reject FPTrunc on the authoritative FPExt route");

  lir::LirModule missing_from_type = lowered;
  require_focused_cast(missing_from_type).first.from_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_from_type, "verifier should reject missing FPExt source type");

  lir::LirModule missing_to_type = lowered;
  require_focused_cast(missing_to_type).first.to_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_to_type, "verifier should reject missing FPExt destination type");

  lir::LirModule conflicting_endpoint = lowered;
  require_focused_cast(conflicting_endpoint).first.to_type =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_endpoint,
      "verifier should reject nonfloating FPExt endpoint authority");

  lir::LirModule conflicting_direction = lowered;
  auto [direction_cast, direction_use] =
      require_focused_cast(conflicting_direction);
  direction_cast.from_type = lir::LirTypeRef("double");
  direction_cast.to_type = lir::LirTypeRef("float");
  direction_use.type_str = lir::LirTypeRef("float");
  expect_identity_verification_rejected(
      conflicting_direction,
      "verifier should reject nonwidening authoritative FPExt endpoints");
}

void test_scalar_sitofp_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_scalar_sitofp_result_use_source;
double lir_scalar_sitofp_result_use_identity(void) {
  return (double)(lir_scalar_sitofp_result_use_source + 1) * 4.0;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_sitofp_result_use_identity");
  std::vector<lir::LirBinOp*> binary_ops;
  lir::LirCastOp* conversion = nullptr;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) conversion = cast;
    }
  }
  expect_true(
      binary_ops.size() == 2 && conversion &&
          binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::Add &&
          binary_ops[0]->type_str.kind() == lir::LirTypeKind::Integer &&
          binary_ops[0]->type_str.integer_bit_width() == 32 &&
          binary_ops[0]->result.value_id() &&
          conversion->kind == lir::LirCastKind::SIToFP &&
          conversion->from_type.kind() == lir::LirTypeKind::Integer &&
          conversion->from_type.integer_bit_width() == 32 &&
          conversion->to_type.kind() == lir::LirTypeKind::Floating &&
          conversion->to_type.str() == "double" &&
          conversion->operand.value_id() &&
          *conversion->operand.value_id() ==
              *binary_ops[0]->result.value_id() &&
          conversion->result.value_id() &&
          conversion->result.value_id()->valid() &&
          binary_ops[1]->opcode.typed() == lir::LirBinaryOpcode::FMul &&
          binary_ops[1]->type_str.kind() == lir::LirTypeKind::Floating &&
          binary_ops[1]->type_str.str() == "double" &&
          binary_ops[1]->lhs.value_id() &&
          *binary_ops[1]->lhs.value_id() ==
              *conversion->result.value_id(),
      "explicit SIToFP should preserve exact integer source and floating result/use IDs");
  lir::verify_module(lowered);

  const auto require_focused_cast = [](lir::LirModule& module)
      -> std::pair<lir::LirCastOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_sitofp_result_use_identity");
    lir::LirCastOp* found_cast = nullptr;
    std::vector<lir::LirBinOp*> found_binary_ops;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(found_cast == nullptr,
                      "focused SIToFP fixture should contain one cast");
          found_cast = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          found_binary_ops.push_back(candidate);
        }
      }
    }
    expect_true(found_cast && found_cast->result.value_id(),
                "focused SIToFP fixture should contain an authoritative cast");
    lir::LirBinOp* found_use = nullptr;
    for (lir::LirBinOp* binary : found_binary_ops) {
      if (binary->lhs.value_id() &&
          *binary->lhs.value_id() == *found_cast->result.value_id()) {
        found_use = binary;
      }
    }
    expect_true(found_use,
                "focused SIToFP fixture should contain its later floating use");
    return {*found_cast, *found_use};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_cast, misleading_use] = require_focused_cast(misleading);
  misleading_cast.operand.str() = "@rendered-not-sitofp-source";
  misleading_cast.result.str() = "7";
  misleading_use.lhs.str() = "@rendered-not-sitofp-result";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_cast(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid SIToFP result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_cast, duplicate_use] = require_focused_cast(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_cast.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate SIToFP result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_cast(unknown_use).first.operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown SIToFP source use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_sitofp_owner", lir::LirValueId{99}));
  require_focused_cast(cross_function_use).first.operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function SIToFP source use");

  lir::LirModule wrong_kind = lowered;
  require_focused_cast(wrong_kind).first.kind = lir::LirCastKind::FPToSI;
  expect_identity_verification_rejected(
      wrong_kind, "verifier should reject FPToSI on the authoritative SIToFP route");

  lir::LirModule missing_from_type = lowered;
  require_focused_cast(missing_from_type).first.from_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_from_type, "verifier should reject missing SIToFP source type");

  lir::LirModule missing_to_type = lowered;
  require_focused_cast(missing_to_type).first.to_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_to_type, "verifier should reject missing SIToFP destination type");

  lir::LirModule conflicting_from_type = lowered;
  require_focused_cast(conflicting_from_type).first.from_type =
      lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_from_type,
      "verifier should reject floating SIToFP source type authority");

  lir::LirModule conflicting_to_type = lowered;
  require_focused_cast(conflicting_to_type).first.to_type =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_to_type,
      "verifier should reject integer SIToFP destination type authority");
}

void test_scalar_uitofp_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
unsigned int lir_scalar_uitofp_result_use_source;
double lir_scalar_uitofp_result_use_identity(void) {
  return (double)(lir_scalar_uitofp_result_use_source + 1U) * 4.0;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_uitofp_result_use_identity");
  std::vector<lir::LirBinOp*> binary_ops;
  lir::LirCastOp* conversion = nullptr;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) conversion = cast;
    }
  }
  expect_true(
      binary_ops.size() == 2 && conversion &&
          binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::Add &&
          binary_ops[0]->type_str.kind() == lir::LirTypeKind::Integer &&
          binary_ops[0]->type_str.integer_bit_width() == 32 &&
          binary_ops[0]->result.value_id() &&
          conversion->kind == lir::LirCastKind::UIToFP &&
          conversion->from_type.kind() == lir::LirTypeKind::Integer &&
          conversion->from_type.integer_bit_width() == 32 &&
          conversion->to_type.kind() == lir::LirTypeKind::Floating &&
          conversion->to_type.str() == "double" &&
          conversion->operand.value_id() &&
          *conversion->operand.value_id() ==
              *binary_ops[0]->result.value_id() &&
          conversion->result.value_id() &&
          conversion->result.value_id()->valid() &&
          binary_ops[1]->opcode.typed() == lir::LirBinaryOpcode::FMul &&
          binary_ops[1]->type_str.kind() == lir::LirTypeKind::Floating &&
          binary_ops[1]->type_str.str() == "double" &&
          binary_ops[1]->lhs.value_id() &&
          *binary_ops[1]->lhs.value_id() ==
              *conversion->result.value_id(),
      "explicit UIToFP should preserve exact integer source and floating result/use IDs");
  lir::verify_module(lowered);

  const auto require_focused_cast = [](lir::LirModule& module)
      -> std::pair<lir::LirCastOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_uitofp_result_use_identity");
    lir::LirCastOp* found_cast = nullptr;
    std::vector<lir::LirBinOp*> found_binary_ops;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(found_cast == nullptr,
                      "focused UIToFP fixture should contain one cast");
          found_cast = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          found_binary_ops.push_back(candidate);
        }
      }
    }
    expect_true(found_cast && found_cast->result.value_id(),
                "focused UIToFP fixture should contain an authoritative cast");
    lir::LirBinOp* found_use = nullptr;
    for (lir::LirBinOp* binary : found_binary_ops) {
      if (binary->lhs.value_id() &&
          *binary->lhs.value_id() == *found_cast->result.value_id()) {
        found_use = binary;
      }
    }
    expect_true(found_use,
                "focused UIToFP fixture should contain its later floating use");
    return {*found_cast, *found_use};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_cast, misleading_use] = require_focused_cast(misleading);
  misleading_cast.operand.str() = "@rendered-not-uitofp-source";
  misleading_cast.result.str() = "7";
  misleading_use.lhs.str() = "@rendered-not-uitofp-result";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_cast(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid UIToFP result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_cast, duplicate_use] = require_focused_cast(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_cast.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate UIToFP result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_cast(unknown_use).first.operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown UIToFP source use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_uitofp_owner", lir::LirValueId{99}));
  require_focused_cast(cross_function_use).first.operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function UIToFP source use");

  lir::LirModule wrong_kind = lowered;
  require_focused_cast(wrong_kind).first.kind = lir::LirCastKind::FPToUI;
  expect_identity_verification_rejected(
      wrong_kind, "verifier should reject FPToUI on the authoritative UIToFP route");

  lir::LirModule missing_from_type = lowered;
  require_focused_cast(missing_from_type).first.from_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_from_type, "verifier should reject missing UIToFP source type");

  lir::LirModule missing_to_type = lowered;
  require_focused_cast(missing_to_type).first.to_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_to_type, "verifier should reject missing UIToFP destination type");

  lir::LirModule conflicting_from_type = lowered;
  require_focused_cast(conflicting_from_type).first.from_type =
      lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_from_type,
      "verifier should reject floating UIToFP source type authority");

  lir::LirModule conflicting_to_type = lowered;
  require_focused_cast(conflicting_to_type).first.to_type =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_to_type,
      "verifier should reject integer UIToFP destination type authority");
}

void test_scalar_fptosi_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_scalar_fptosi_result_use_identity(void) {
  return (int)(1.25 + 2.5) + 4;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_fptosi_result_use_identity");
  std::vector<lir::LirBinOp*> binary_ops;
  lir::LirCastOp* conversion = nullptr;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) conversion = cast;
    }
  }
  expect_true(
      binary_ops.size() == 2 && conversion &&
          binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::FAdd &&
          binary_ops[0]->type_str.kind() == lir::LirTypeKind::Floating &&
          binary_ops[0]->type_str.str() == "double" &&
          binary_ops[0]->result.value_id() &&
          conversion->kind == lir::LirCastKind::FPToSI &&
          conversion->from_type.kind() == lir::LirTypeKind::Floating &&
          conversion->from_type.str() == "double" &&
          conversion->to_type.kind() == lir::LirTypeKind::Integer &&
          conversion->to_type.integer_bit_width() == 32 &&
          conversion->operand.value_id() &&
          *conversion->operand.value_id() ==
              *binary_ops[0]->result.value_id() &&
          conversion->result.value_id() &&
          conversion->result.value_id()->valid() &&
          binary_ops[1]->opcode.typed() == lir::LirBinaryOpcode::Add &&
          binary_ops[1]->type_str.kind() == lir::LirTypeKind::Integer &&
          binary_ops[1]->type_str.integer_bit_width() == 32 &&
          binary_ops[1]->lhs.value_id() &&
          *binary_ops[1]->lhs.value_id() ==
              *conversion->result.value_id(),
      "explicit FPToSI should preserve exact floating source and integer result/use IDs");
  lir::verify_module(lowered);

  const auto require_focused_cast = [](lir::LirModule& module)
      -> std::pair<lir::LirCastOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_fptosi_result_use_identity");
    lir::LirCastOp* found_cast = nullptr;
    std::vector<lir::LirBinOp*> found_binary_ops;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(found_cast == nullptr,
                      "focused FPToSI fixture should contain one cast");
          found_cast = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          found_binary_ops.push_back(candidate);
        }
      }
    }
    expect_true(found_cast && found_cast->result.value_id(),
                "focused FPToSI fixture should contain an authoritative cast");
    lir::LirBinOp* found_use = nullptr;
    for (lir::LirBinOp* binary : found_binary_ops) {
      if (binary->lhs.value_id() &&
          *binary->lhs.value_id() == *found_cast->result.value_id()) {
        found_use = binary;
      }
    }
    expect_true(found_use,
                "focused FPToSI fixture should contain its later integer use");
    return {*found_cast, *found_use};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_cast, misleading_use] = require_focused_cast(misleading);
  misleading_cast.operand.str() = "@rendered-not-fptosi-source";
  misleading_cast.result.str() = "7";
  misleading_use.lhs.str() = "@rendered-not-fptosi-result";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_cast(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid FPToSI result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_cast, duplicate_use] = require_focused_cast(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_cast.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate FPToSI result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_cast(unknown_use).first.operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown FPToSI source use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_fptosi_owner", lir::LirValueId{99}));
  require_focused_cast(cross_function_use).first.operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function FPToSI source use");

  lir::LirModule wrong_kind = lowered;
  require_focused_cast(wrong_kind).first.kind = lir::LirCastKind::SIToFP;
  expect_identity_verification_rejected(
      wrong_kind, "verifier should reject SIToFP on the authoritative FPToSI route");

  lir::LirModule missing_from_type = lowered;
  require_focused_cast(missing_from_type).first.from_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_from_type, "verifier should reject missing FPToSI source type");

  lir::LirModule missing_to_type = lowered;
  require_focused_cast(missing_to_type).first.to_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_to_type, "verifier should reject missing FPToSI destination type");

  lir::LirModule conflicting_from_type = lowered;
  require_focused_cast(conflicting_from_type).first.from_type =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_from_type,
      "verifier should reject integer FPToSI source type authority");

  lir::LirModule conflicting_to_type = lowered;
  require_focused_cast(conflicting_to_type).first.to_type =
      lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_to_type,
      "verifier should reject floating FPToSI destination type authority");
}

void test_scalar_fptoui_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
unsigned int lir_scalar_fptoui_result_use_identity(void) {
  return (unsigned int)(1.25 + 2.5) + 4U;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_fptoui_result_use_identity");
  std::vector<lir::LirBinOp*> binary_ops;
  lir::LirCastOp* conversion = nullptr;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) conversion = cast;
    }
  }
  expect_true(
      binary_ops.size() == 2 && conversion &&
          binary_ops[0]->opcode.typed() == lir::LirBinaryOpcode::FAdd &&
          binary_ops[0]->type_str.kind() == lir::LirTypeKind::Floating &&
          binary_ops[0]->type_str.str() == "double" &&
          binary_ops[0]->result.value_id() &&
          conversion->kind == lir::LirCastKind::FPToUI &&
          conversion->from_type.kind() == lir::LirTypeKind::Floating &&
          conversion->from_type.str() == "double" &&
          conversion->to_type.kind() == lir::LirTypeKind::Integer &&
          conversion->to_type.integer_bit_width() == 32 &&
          conversion->operand.value_id() &&
          *conversion->operand.value_id() ==
              *binary_ops[0]->result.value_id() &&
          conversion->result.value_id() &&
          conversion->result.value_id()->valid() &&
          binary_ops[1]->opcode.typed() == lir::LirBinaryOpcode::Add &&
          binary_ops[1]->type_str.kind() == lir::LirTypeKind::Integer &&
          binary_ops[1]->type_str.integer_bit_width() == 32 &&
          binary_ops[1]->lhs.value_id() &&
          *binary_ops[1]->lhs.value_id() ==
              *conversion->result.value_id(),
      "explicit FPToUI should preserve exact floating source and integer result/use IDs");
  lir::verify_module(lowered);

  const auto require_focused_cast = [](lir::LirModule& module)
      -> std::pair<lir::LirCastOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_fptoui_result_use_identity");
    lir::LirCastOp* found_cast = nullptr;
    std::vector<lir::LirBinOp*> found_binary_ops;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(found_cast == nullptr,
                      "focused FPToUI fixture should contain one cast");
          found_cast = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          found_binary_ops.push_back(candidate);
        }
      }
    }
    expect_true(found_cast && found_cast->result.value_id(),
                "focused FPToUI fixture should contain an authoritative cast");
    lir::LirBinOp* found_use = nullptr;
    for (lir::LirBinOp* binary : found_binary_ops) {
      if (binary->lhs.value_id() &&
          *binary->lhs.value_id() == *found_cast->result.value_id()) {
        found_use = binary;
      }
    }
    expect_true(found_use,
                "focused FPToUI fixture should contain its later integer use");
    return {*found_cast, *found_use};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_cast, misleading_use] = require_focused_cast(misleading);
  misleading_cast.operand.str() = "@rendered-not-fptoui-source";
  misleading_cast.result.str() = "7";
  misleading_use.lhs.str() = "@rendered-not-fptoui-result";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_cast(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid FPToUI result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_cast, duplicate_use] = require_focused_cast(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_cast.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate FPToUI result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_cast(unknown_use).first.operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown FPToUI source use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_fptoui_owner", lir::LirValueId{99}));
  require_focused_cast(cross_function_use).first.operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function FPToUI source use");

  lir::LirModule wrong_kind = lowered;
  require_focused_cast(wrong_kind).first.kind = lir::LirCastKind::UIToFP;
  expect_identity_verification_rejected(
      wrong_kind, "verifier should reject UIToFP on the authoritative FPToUI route");

  lir::LirModule missing_from_type = lowered;
  require_focused_cast(missing_from_type).first.from_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_from_type, "verifier should reject missing FPToUI source type");

  lir::LirModule missing_to_type = lowered;
  require_focused_cast(missing_to_type).first.to_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_to_type, "verifier should reject missing FPToUI destination type");

  lir::LirModule conflicting_from_type = lowered;
  require_focused_cast(conflicting_from_type).first.from_type =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_from_type,
      "verifier should reject integer FPToUI source type authority");

  lir::LirModule conflicting_to_type = lowered;
  require_focused_cast(conflicting_to_type).first.to_type =
      lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_to_type,
      "verifier should reject floating FPToUI destination type authority");
}

void test_scalar_compare_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_scalar_compare_result_use_source;
int lir_scalar_compare_result_use_identity(void) {
  return lir_scalar_compare_result_use_source < 7;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_compare_result_use_identity");
  std::vector<lir::LirLoadOp*> loads;
  std::vector<lir::LirCmpOp*> comparisons;
  std::vector<lir::LirCastOp*> casts;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* load = std::get_if<lir::LirLoadOp>(&inst)) loads.push_back(load);
      if (auto* comparison = std::get_if<lir::LirCmpOp>(&inst)) {
        comparisons.push_back(comparison);
      }
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) casts.push_back(cast);
    }
  }
  expect_true(loads.size() == 1 && comparisons.size() == 1 && casts.size() == 1,
              "scalar-compare probe should lower one load, compare, and normalization cast");
  expect_true(!comparisons[0]->is_float &&
                  comparisons[0]->predicate.typed() ==
                      lir::LirCmpPredicate::Slt &&
                  comparisons[0]->type_str.kind() == lir::LirTypeKind::Integer &&
                  comparisons[0]->type_str.integer_bit_width() == 32 &&
                  casts[0]->kind == lir::LirCastKind::ZExt &&
                  casts[0]->from_type.kind() == lir::LirTypeKind::Integer &&
                  casts[0]->from_type.integer_bit_width() == 1 &&
                  casts[0]->to_type.kind() == lir::LirTypeKind::Integer &&
                  casts[0]->to_type.integer_bit_width() == 32,
              "scalar compare should retain native slt/i32 and coupled zext facts");
  expect_true(loads[0]->result.value_id() && comparisons[0]->lhs.value_id() &&
                  *comparisons[0]->lhs.value_id() ==
                      *loads[0]->result.value_id() &&
                  comparisons[0]->result.value_id() &&
                  comparisons[0]->result.value_id()->valid() &&
                  casts[0]->operand.value_id() &&
                  *casts[0]->operand.value_id() ==
                      *comparisons[0]->result.value_id() &&
                  !casts[0]->result.has_authority(),
              "normalization cast should consume the exact compare result ID without widening cast ownership");
  lir::verify_module(lowered);

  const auto require_focused_compare = [](lir::LirModule& module)
      -> std::pair<lir::LirCmpOp&, lir::LirCastOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_compare_result_use_identity");
    lir::LirCmpOp* comparison = nullptr;
    lir::LirCastOp* cast = nullptr;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCmpOp>(&inst)) {
          expect_true(comparison == nullptr,
                      "focused scalar-compare fixture should contain one compare");
          comparison = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(cast == nullptr,
                      "focused scalar-compare fixture should contain one cast use");
          cast = candidate;
        }
      }
    }
    expect_true(comparison && cast,
                "focused scalar-compare fixture should contain its result/use pair");
    return {*comparison, *cast};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_compare, misleading_use] =
      require_focused_compare(misleading);
  misleading_compare.result.str() = "@rendered-not-compare-result";
  misleading_use.operand.str() = "7";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_compare(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid scalar compare result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_compare, duplicate_use] =
      require_focused_compare(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_compare.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate scalar compare result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_compare(unknown_use).second.operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown scalar compare result use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_compare_owner", lir::LirValueId{99}));
  require_focused_compare(cross_function_use).second.operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function scalar compare result use");

  lir::LirModule invalid_predicate = lowered;
  require_focused_compare(invalid_predicate).first.predicate =
      lir::LirCmpPredicateRef("not-a-compare-predicate");
  expect_identity_verification_rejected(
      invalid_predicate, "verifier should reject invalid compare predicate");

  lir::LirModule missing_type = lowered;
  require_focused_compare(missing_type).first.type_str = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_type, "verifier should reject missing compare type authority");

  lir::LirModule conflicting_type = lowered;
  require_focused_compare(conflicting_type).first.type_str =
      lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_type,
      "verifier should reject floating type on authoritative integer compare");
}

void test_scalar_floating_compare_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_scalar_floating_compare_result_use_identity(void) {
  return 1.25 < 2.5;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function = require_function(
      lowered, "lir_scalar_floating_compare_result_use_identity");
  lir::LirCmpOp* comparison = nullptr;
  lir::LirCastOp* normalization = nullptr;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* candidate = std::get_if<lir::LirCmpOp>(&inst)) {
        comparison = candidate;
      }
      if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
        normalization = candidate;
      }
    }
  }
  expect_true(comparison && normalization && comparison->is_float &&
                  comparison->predicate.typed() == lir::LirCmpPredicate::OLt &&
                  comparison->type_str.kind() == lir::LirTypeKind::Floating &&
                  comparison->type_str.str() == "double" &&
                  comparison->result.value_id() &&
                  comparison->result.value_id()->valid() &&
                  !comparison->lhs.has_authority() &&
                  !comparison->rhs.has_authority() &&
                  normalization->kind == lir::LirCastKind::ZExt &&
                  normalization->from_type.kind() == lir::LirTypeKind::Integer &&
                  normalization->from_type.integer_bit_width() == 1 &&
                  normalization->to_type.kind() == lir::LirTypeKind::Integer &&
                  normalization->to_type.integer_bit_width() == 32 &&
                  normalization->operand.value_id() &&
                  *normalization->operand.value_id() ==
                      *comparison->result.value_id() &&
                  !normalization->result.has_authority(),
              "scalar floating compare should retain native mode/predicate/type and exact normalization use ID");
  lir::verify_module(lowered);

  const auto require_focused_compare = [](lir::LirModule& module)
      -> std::pair<lir::LirCmpOp&, lir::LirCastOp&> {
    lir::LirFunction& focused = require_function(
        module, "lir_scalar_floating_compare_result_use_identity");
    lir::LirCmpOp* found_comparison = nullptr;
    lir::LirCastOp* found_normalization = nullptr;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirCmpOp>(&inst)) {
          expect_true(found_comparison == nullptr,
                      "focused floating-compare fixture should contain one comparison");
          found_comparison = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(found_normalization == nullptr,
                      "focused floating-compare fixture should contain one normalization cast");
          found_normalization = candidate;
        }
      }
    }
    expect_true(found_comparison && found_comparison->result.value_id() &&
                    found_normalization,
                "focused floating-compare fixture should contain its result/use pair");
    return {*found_comparison, *found_normalization};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_compare, misleading_use] =
      require_focused_compare(misleading);
  misleading_compare.result.str() = "@rendered-not-floating-compare-result";
  misleading_use.operand.str() = "7";
  lir::verify_module(misleading);

  lir::LirModule invalid_result = lowered;
  require_focused_compare(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result,
      "verifier should reject invalid scalar floating compare result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_compare, duplicate_use] =
      require_focused_compare(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_compare.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result,
      "verifier should reject duplicate scalar floating compare result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_compare(unknown_use).second.operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown scalar floating compare use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_floating_compare_owner",
                                  lir::LirValueId{99}));
  require_focused_compare(cross_function_use).second.operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function scalar floating compare use");

  lir::LirModule invalid_predicate = lowered;
  require_focused_compare(invalid_predicate).first.predicate =
      lir::LirCmpPredicateRef("not-a-compare-predicate");
  expect_identity_verification_rejected(
      invalid_predicate,
      "verifier should reject invalid scalar floating compare predicate");

  lir::LirModule wrong_family_predicate = lowered;
  require_focused_compare(wrong_family_predicate).first.predicate =
      lir::LirCmpPredicate::Slt;
  expect_identity_verification_rejected(
      wrong_family_predicate,
      "verifier should reject integer predicate on floating compare mode");

  lir::LirModule missing_type = lowered;
  require_focused_compare(missing_type).first.type_str = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_type,
      "verifier should reject missing scalar floating compare type authority");

  lir::LirModule conflicting_type = lowered;
  require_focused_compare(conflicting_type).first.type_str =
      lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_type,
      "verifier should reject integer type on floating compare mode");

  lir::LirModule conflicting_mode = lowered;
  require_focused_compare(conflicting_mode).first.is_float = false;
  expect_identity_verification_rejected(
      conflicting_mode,
      "verifier should reject integer mode on authoritative floating comparison");
}

void test_scalar_select_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int lir_scalar_select_result_use_source;
int lir_scalar_select_result_use_identity(void) {
  return __builtin_ffs(lir_scalar_select_result_use_source) + 1;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_select_result_use_identity");
  std::vector<lir::LirSelectOp*> selects;
  std::vector<lir::LirBinOp*> binary_ops;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* select = std::get_if<lir::LirSelectOp>(&inst)) {
        selects.push_back(select);
      }
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
    }
  }
  expect_true(selects.size() == 1 && binary_ops.size() == 2,
              "scalar-select probe should lower one select and its builtin/later binary operations");
  lir::LirSelectOp& select = *selects[0];
  lir::LirBinOp* later_use = nullptr;
  for (lir::LirBinOp* binary : binary_ops) {
    if (select.result.value_id() && binary->lhs.value_id() &&
        *binary->lhs.value_id() == *select.result.value_id()) {
      later_use = binary;
    }
  }
  expect_true(select.type_str.kind() == lir::LirTypeKind::Integer &&
                  select.type_str.integer_bit_width() == 32 &&
                  select.result.value_id() && select.result.value_id()->valid() &&
                  select.cond.kind() == lir::LirOperandKind::SsaValue &&
                  !select.cond.has_authority() &&
                  select.true_val.integer_immediate() &&
                  select.true_val.integer_immediate()->value == 0 &&
                  select.false_val.kind() == lir::LirOperandKind::SsaValue &&
                  !select.false_val.has_authority(),
              "scalar select should retain exact i32 result type and honest available operand authority");
  expect_true(later_use && later_use->opcode.typed() == lir::LirBinaryOpcode::Add &&
                  later_use->type_str.kind() == lir::LirTypeKind::Integer &&
                  later_use->type_str.integer_bit_width() == 32 &&
                  later_use->lhs.value_id() &&
                  *later_use->lhs.value_id() == *select.result.value_id(),
              "later ordinary use should carry the exact scalar select result ID");
  lir::verify_module(lowered);

  const auto require_focused_select = [](lir::LirModule& module)
      -> std::pair<lir::LirSelectOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_select_result_use_identity");
    lir::LirSelectOp* found_select = nullptr;
    std::vector<lir::LirBinOp*> found_binary_ops;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirSelectOp>(&inst)) {
          expect_true(found_select == nullptr,
                      "focused scalar-select fixture should contain one select");
          found_select = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          found_binary_ops.push_back(candidate);
        }
      }
    }
    expect_true(found_select && found_select->result.value_id(),
                "focused scalar-select fixture should contain an authoritative select");
    lir::LirBinOp* found_use = nullptr;
    for (lir::LirBinOp* binary : found_binary_ops) {
      if (binary->lhs.value_id() &&
          *binary->lhs.value_id() == *found_select->result.value_id()) {
        found_use = binary;
      }
    }
    expect_true(found_use,
                "focused scalar-select fixture should contain its later ordinary use");
    return {*found_select, *found_use};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_select, misleading_use] = require_focused_select(misleading);
  misleading_select.result.str() = "@rendered-not-select-result";
  misleading_use.lhs.str() = "7";
  lir::verify_module(misleading);

  lir::LirModule missing_result = lowered;
  require_focused_select(missing_result).first.result =
      lir::LirOperand("%missing");
  expect_identity_verification_rejected(
      missing_result, "verifier should reject scalar select without result authority");

  lir::LirModule invalid_result = lowered;
  require_focused_select(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid scalar select result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_select, duplicate_use] = require_focused_select(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_select.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate scalar select result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_select(unknown_use).second.lhs =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown scalar select result use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_select_owner", lir::LirValueId{99}));
  require_focused_select(cross_function_use).second.lhs =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function scalar select result use");

  lir::LirModule missing_type = lowered;
  require_focused_select(missing_type).first.type_str = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_type, "verifier should reject missing scalar select type authority");

  lir::LirModule conflicting_type = lowered;
  require_focused_select(conflicting_type).first.type_str =
      lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_type,
      "verifier should reject noninteger type on authoritative scalar select");

  lir::LirModule missing_condition = lowered;
  require_focused_select(missing_condition).first.cond = lir::LirOperand{};
  expect_identity_verification_rejected(
      missing_condition, "verifier should reject missing scalar select condition");

  lir::LirModule wrong_condition_authority = lowered;
  require_focused_select(wrong_condition_authority).first.cond =
      lir::LirOperand::integer("1", 1);
  expect_identity_verification_rejected(
      wrong_condition_authority,
      "verifier should reject immediate authority on scalar select condition");
}

void test_wide_ffs_select_trunc_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
long long lir_wide_ffs_select_trunc_source;
int lir_wide_ffs_select_trunc_identity(void) {
  return __builtin_ffsll(lir_wide_ffs_select_trunc_source) + 1;
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_wide_ffs_select_trunc_identity");
  lir::LirCallOp* cttz = nullptr;
  lir::LirCmpOp* zero_cmp = nullptr;
  lir::LirSelectOp* select = nullptr;
  lir::LirCastOp* trunc = nullptr;
  std::vector<lir::LirBinOp*> binary_ops;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* call = std::get_if<lir::LirCallOp>(&inst)) cttz = call;
      if (auto* cmp = std::get_if<lir::LirCmpOp>(&inst)) zero_cmp = cmp;
      if (auto* candidate = std::get_if<lir::LirSelectOp>(&inst)) {
        select = candidate;
      }
      if (auto* cast = std::get_if<lir::LirCastOp>(&inst)) trunc = cast;
      if (auto* binary = std::get_if<lir::LirBinOp>(&inst)) {
        binary_ops.push_back(binary);
      }
    }
  }
  lir::LirBinOp* builtin_plus_one = nullptr;
  lir::LirBinOp* later_use = nullptr;
  for (lir::LirBinOp* binary : binary_ops) {
    if (trunc && trunc->result.value_id() && binary->lhs.value_id() &&
        *binary->lhs.value_id() == *trunc->result.value_id()) {
      later_use = binary;
    } else {
      builtin_plus_one = binary;
    }
  }
  expect_true(
      cttz && zero_cmp && select && trunc && builtin_plus_one && later_use &&
          binary_ops.size() == 2 && select->result.value_id() &&
          select->result.value_id()->valid() &&
          select->type_str.kind() == lir::LirTypeKind::Integer &&
          select->type_str.integer_bit_width() == 64 &&
          trunc->kind == lir::LirCastKind::Trunc &&
          trunc->from_type.kind() == lir::LirTypeKind::Integer &&
          trunc->from_type.integer_bit_width() == 64 &&
          trunc->to_type.kind() == lir::LirTypeKind::Integer &&
          trunc->to_type.integer_bit_width() == 32 &&
          trunc->operand.value_id() &&
          *trunc->operand.value_id() == *select->result.value_id() &&
          trunc->result.value_id() && trunc->result.value_id()->valid() &&
          later_use->opcode.typed() == lir::LirBinaryOpcode::Add &&
          later_use->type_str.kind() == lir::LirTypeKind::Integer &&
          later_use->type_str.integer_bit_width() == 32 &&
          later_use->lhs.value_id() &&
          *later_use->lhs.value_id() == *trunc->result.value_id(),
      "wide ffs should preserve exact select-to-trunc-to-use identities");
  expect_true(!cttz->result.has_authority() &&
                  !builtin_plus_one->result.has_authority() &&
                  !zero_cmp->result.has_authority() &&
                  select->cond.kind() == lir::LirOperandKind::SsaValue &&
                  !select->cond.has_authority() &&
                  select->false_val.kind() == lir::LirOperandKind::SsaValue &&
                  !select->false_val.has_authority(),
              "wide ffs internal producers should remain honest compatibility");
  lir::verify_module(lowered);

  struct FocusedChain {
    lir::LirSelectOp* select = nullptr;
    lir::LirCastOp* trunc = nullptr;
    lir::LirBinOp* later_use = nullptr;
  };
  const auto require_focused_chain = [](lir::LirModule& module) {
    lir::LirFunction& focused =
        require_function(module, "lir_wide_ffs_select_trunc_identity");
    FocusedChain chain;
    std::vector<lir::LirBinOp*> found_binary_ops;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirSelectOp>(&inst)) {
          expect_true(chain.select == nullptr,
                      "focused wide-ffs fixture should contain one select");
          chain.select = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirCastOp>(&inst)) {
          expect_true(chain.trunc == nullptr,
                      "focused wide-ffs fixture should contain one cast");
          chain.trunc = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          found_binary_ops.push_back(candidate);
        }
      }
    }
    expect_true(chain.select && chain.select->result.value_id() && chain.trunc &&
                    chain.trunc->result.value_id(),
                "focused wide-ffs fixture should contain authoritative select/cast results");
    for (lir::LirBinOp* binary : found_binary_ops) {
      if (binary->lhs.value_id() &&
          *binary->lhs.value_id() == *chain.trunc->result.value_id()) {
        chain.later_use = binary;
      }
    }
    expect_true(chain.later_use,
                "focused wide-ffs fixture should contain its later i32 use");
    return chain;
  };

  lir::LirModule misleading = lowered;
  FocusedChain misleading_chain = require_focused_chain(misleading);
  misleading_chain.select->result.str() = "@rendered-not-wide-select";
  misleading_chain.trunc->operand.str() = "7";
  misleading_chain.trunc->result.str() = "@rendered-not-wide-trunc";
  misleading_chain.later_use->lhs.str() = "8";
  lir::verify_module(misleading);

  lir::LirModule invalid_select_result = lowered;
  require_focused_chain(invalid_select_result).select->result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_select_result, "verifier should reject invalid wide-ffs select result ID");

  lir::LirModule invalid_cast_result = lowered;
  require_focused_chain(invalid_cast_result).trunc->result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_cast_result, "verifier should reject invalid wide-ffs trunc result ID");

  lir::LirModule duplicate_result = lowered;
  FocusedChain duplicate_chain = require_focused_chain(duplicate_result);
  duplicate_chain.trunc->result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_chain.select->result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate wide-ffs result IDs");

  lir::LirModule unknown_source = lowered;
  require_focused_chain(unknown_source).trunc->operand =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_source, "verifier should reject unknown wide-ffs trunc source ID");

  lir::LirModule cross_function_source = lowered;
  cross_function_source.functions.push_back(
      make_identity_test_function("wide_ffs_owner", lir::LirValueId{99}));
  require_focused_chain(cross_function_source).trunc->operand =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_source,
      "verifier should reject cross-function wide-ffs trunc source ID");

  lir::LirModule unknown_later_use = lowered;
  require_focused_chain(unknown_later_use).later_use->lhs =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_later_use, "verifier should reject unknown wide-ffs trunc result use");

  lir::LirModule wrong_kind = lowered;
  require_focused_chain(wrong_kind).trunc->kind = lir::LirCastKind::ZExt;
  expect_identity_verification_rejected(
      wrong_kind, "verifier should reject non-Trunc wide-ffs narrowing kind");

  lir::LirModule missing_from_type = lowered;
  require_focused_chain(missing_from_type).trunc->from_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_from_type, "verifier should reject missing wide-ffs trunc source type");

  lir::LirModule missing_to_type = lowered;
  require_focused_chain(missing_to_type).trunc->to_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_to_type, "verifier should reject missing wide-ffs trunc destination type");

  lir::LirModule conflicting_endpoint = lowered;
  require_focused_chain(conflicting_endpoint).trunc->to_type =
      lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_endpoint,
      "verifier should reject noninteger wide-ffs trunc destination type");

  lir::LirModule conflicting_direction = lowered;
  FocusedChain direction_chain = require_focused_chain(conflicting_direction);
  direction_chain.trunc->from_type = lir::LirTypeRef::integer(32);
  direction_chain.trunc->to_type = lir::LirTypeRef::integer(64);
  direction_chain.later_use->type_str = lir::LirTypeRef::integer(64);
  expect_identity_verification_rejected(
      conflicting_direction,
      "verifier should reject nonnarrowing wide-ffs Trunc endpoints");
}

void test_scalar_abs_result_use_identity_boundary() {
  namespace lir = c4c::codegen::lir;

  lir::LirModule lowered = lower_lir_module_for_target(R"c(
int abs(int);
long long llabs(long long);
int lir_scalar_abs_result_use_source;
int lir_scalar_abs_result_use_identity(void) {
  return abs(lir_scalar_abs_result_use_source) + 1;
}
int lir_scalar_abs_immediate_authority(void) {
  return abs(7);
}
long long lir_scalar_llabs_immediate_authority(void) {
  return llabs(7LL);
}
)c", "x86_64-linux-gnu");

  lir::LirFunction& function =
      require_function(lowered, "lir_scalar_abs_result_use_identity");
  lir::LirLoadOp* load = nullptr;
  lir::LirAbsOp* abs = nullptr;
  lir::LirBinOp* later_use = nullptr;
  for (auto& block : function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* candidate = std::get_if<lir::LirLoadOp>(&inst)) load = candidate;
      if (auto* candidate = std::get_if<lir::LirAbsOp>(&inst)) abs = candidate;
      if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
        later_use = candidate;
      }
    }
  }
  expect_true(load && abs && later_use && load->result.value_id() &&
                  abs->arg.value_id() &&
                  *abs->arg.value_id() == *load->result.value_id() &&
                  abs->result.value_id() && abs->result.value_id()->valid() &&
                  abs->int_type.kind() == lir::LirTypeKind::Integer &&
                  abs->int_type.integer_bit_width() == 32 &&
                  later_use->opcode.typed() == lir::LirBinaryOpcode::Add &&
                  later_use->type_str.kind() == lir::LirTypeKind::Integer &&
                  later_use->type_str.integer_bit_width() == 32 &&
                  later_use->lhs.value_id() &&
                  *later_use->lhs.value_id() == *abs->result.value_id(),
              "scalar abs should preserve its source and exact result IDs through a later ordinary use");

  lir::LirFunction& immediate_function =
      require_function(lowered, "lir_scalar_abs_immediate_authority");
  lir::LirAbsOp* immediate_abs = nullptr;
  for (auto& block : immediate_function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* candidate = std::get_if<lir::LirAbsOp>(&inst)) {
        immediate_abs = candidate;
      }
    }
  }
  expect_true(immediate_abs && immediate_abs->result.value_id() &&
                  immediate_abs->arg.integer_immediate() &&
                  immediate_abs->arg.integer_immediate()->value == 7 &&
                  immediate_abs->int_type.kind() == lir::LirTypeKind::Integer &&
                  immediate_abs->int_type.integer_bit_width() == 32,
              "scalar abs should preserve structurally available immediate argument authority");

  lir::LirFunction& immediate_ll_function =
      require_function(lowered, "lir_scalar_llabs_immediate_authority");
  lir::LirAbsOp* immediate_llabs = nullptr;
  for (auto& block : immediate_ll_function.blocks) {
    for (auto& inst : block.insts) {
      if (auto* candidate = std::get_if<lir::LirAbsOp>(&inst)) {
        immediate_llabs = candidate;
      }
    }
  }
  expect_true(immediate_llabs && immediate_llabs->result.value_id() &&
                  immediate_llabs->arg.integer_immediate() &&
                  immediate_llabs->arg.integer_immediate()->value == 7 &&
                  immediate_llabs->int_type.kind() == lir::LirTypeKind::Integer &&
                  immediate_llabs->int_type.integer_bit_width() == 64,
              "scalar llabs should retain exact i64 and immediate argument authority");
  lir::verify_module(lowered);

  const auto require_focused_abs = [](lir::LirModule& module)
      -> std::pair<lir::LirAbsOp&, lir::LirBinOp&> {
    lir::LirFunction& focused =
        require_function(module, "lir_scalar_abs_result_use_identity");
    lir::LirAbsOp* found_abs = nullptr;
    lir::LirBinOp* found_use = nullptr;
    for (auto& block : focused.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate = std::get_if<lir::LirAbsOp>(&inst)) {
          expect_true(found_abs == nullptr,
                      "focused scalar-abs fixture should contain one abs operation");
          found_abs = candidate;
        }
        if (auto* candidate = std::get_if<lir::LirBinOp>(&inst)) {
          expect_true(found_use == nullptr,
                      "focused scalar-abs fixture should contain one later binary use");
          found_use = candidate;
        }
      }
    }
    expect_true(found_abs && found_abs->result.value_id() && found_use,
                "focused scalar-abs fixture should contain its result/use pair");
    return {*found_abs, *found_use};
  };

  lir::LirModule misleading = lowered;
  auto [misleading_abs, misleading_use] = require_focused_abs(misleading);
  misleading_abs.arg.str() = "@rendered-not-abs-argument";
  misleading_abs.result.str() = "7";
  misleading_use.lhs.str() = "@rendered-not-abs-result";
  lir::verify_module(misleading);

  lir::LirModule missing_result = lowered;
  require_focused_abs(missing_result).first.result = lir::LirOperand("%missing");
  expect_identity_verification_rejected(
      missing_result, "verifier should reject scalar abs without result authority");

  lir::LirModule invalid_result = lowered;
  require_focused_abs(invalid_result).first.result =
      lir::LirOperand::ssa("%invalid", lir::LirValueId::invalid());
  expect_identity_verification_rejected(
      invalid_result, "verifier should reject invalid scalar abs result ID");

  lir::LirModule duplicate_result = lowered;
  auto [duplicate_abs, duplicate_use] = require_focused_abs(duplicate_result);
  duplicate_use.result = lir::LirOperand::ssa(
      "%duplicate", *duplicate_abs.result.value_id());
  expect_identity_verification_rejected(
      duplicate_result, "verifier should reject duplicate scalar abs result ID");

  lir::LirModule unknown_use = lowered;
  require_focused_abs(unknown_use).second.lhs =
      lir::LirOperand::ssa("%unknown", lir::LirValueId{99});
  expect_identity_verification_rejected(
      unknown_use, "verifier should reject unknown scalar abs result use");

  lir::LirModule cross_function_use = lowered;
  cross_function_use.functions.push_back(
      make_identity_test_function("scalar_abs_owner", lir::LirValueId{99}));
  require_focused_abs(cross_function_use).second.lhs =
      lir::LirOperand::ssa("%cross", lir::LirValueId{99});
  expect_identity_verification_rejected(
      cross_function_use,
      "verifier should reject cross-function scalar abs result use");

  lir::LirModule missing_type = lowered;
  require_focused_abs(missing_type).first.int_type = lir::LirTypeRef{};
  expect_identity_verification_rejected(
      missing_type, "verifier should reject missing scalar abs type authority");

  lir::LirModule conflicting_type = lowered;
  require_focused_abs(conflicting_type).first.int_type = lir::LirTypeRef("double");
  expect_identity_verification_rejected(
      conflicting_type,
      "verifier should reject noninteger type on authoritative scalar abs");

  lir::LirModule missing_argument = lowered;
  require_focused_abs(missing_argument).first.arg = lir::LirOperand{};
  expect_identity_verification_rejected(
      missing_argument, "verifier should reject missing scalar abs argument");

  lir::LirModule wrong_argument_authority = lowered;
  require_focused_abs(wrong_argument_authority).first.arg =
      lir::LirOperand::global("@wrong", c4c::LinkNameId{99});
  expect_identity_verification_rejected(
      wrong_argument_authority,
      "verifier should reject global authority on scalar abs argument");
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

int *leaf_ptr(int index) {
  static int values[4];
  return &values[index];
}

int *(*select_leaf_ptr(int seed))(int) {
  return leaf_ptr;
}

int read_nested_indirect_return(int *(*(*chooser)(int))(int)) {
  return *chooser(0)(2);
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
  expect_eq(std::to_string(direct_call.structured_args.size()), "1",
            "direct call should carry one structured argument");
  expect_structured_call_arg_matches_rendered(direct_call, 0, "direct call arg");
  expect_struct_type_ref(lir_module, direct_call.structured_args[0].type_ref,
                         "%struct.Pair", "direct call structured argument type ref");

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
  expect_eq(std::to_string(byval_call.structured_args.size()), "1",
            "fixed byval aggregate call should carry one structured argument");
  expect_structured_call_arg_matches_rendered(byval_call, 0, "fixed byval call arg");
  expect_struct_type_ref(lir_module, byval_call.structured_args[0].type_ref,
                         "%struct.Big", "fixed byval structured argument type ref");

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
  expect_eq(std::to_string(variadic_call.structured_args.size()), "2",
            "variadic aggregate call should still carry structured owned arguments");
  expect_structured_call_arg_matches_rendered(variadic_call, 1,
                                             "variadic aggregate tail arg");

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
  expect_eq(std::to_string(no_proto_call.structured_args.size()), "1",
            "direct no-prototype call should carry structured argument facts");
  expect_structured_call_arg_matches_rendered(no_proto_call, 0, "no-prototype call arg");
  expect_struct_type_ref(lir_module, no_proto_call.structured_args[0].type_ref,
                         "%struct.Pair",
                         "no-prototype structured argument type ref");

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
  expect_eq(std::to_string(indirect_int_call.structured_args.size()), "1",
            "metadata-rich indirect int call should carry one structured argument");
  expect_structured_call_arg_matches_rendered(indirect_int_call, 0,
                                             "indirect int call arg");
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
  expect_true(!indirect_unspecified_call.structured_args.empty(),
              "indirect unspecified call should carry structured argument facts");

  c4c::codegen::lir::LirFunction& read_nested_indirect_return =
      require_function(lir_module, "read_nested_indirect_return");
  c4c::codegen::lir::LirCallOp& nested_selector_call =
      require_nth_indirect_call(read_nested_indirect_return, 0);
  expect_eq(nested_selector_call.return_type.str(), "ptr",
            "nested function-pointer-returning indirect call should return ptr");
  expect_true(nested_selector_call.callee_signature.has_value(),
              "nested selector indirect call should carry callee signature");
  expect_true(nested_selector_call.callee_signature->return_type_ref.has_value(),
              "nested selector signature should carry return type ref");
  expect_eq(nested_selector_call.callee_signature->return_type_ref->str(), "ptr",
            "nested selector callee signature should preserve pointer return");
  const std::string nested_selector_formatted =
      c4c::codegen::lir::format_lir_call_site(nested_selector_call);
  expect_true(nested_selector_formatted.find("(i32) %") != std::string::npos,
              "nested selector call should preserve rendered function-pointer suffix");

  c4c::codegen::lir::LirCallOp raw_compat_call{};
  raw_compat_call.result = "%raw";
  raw_compat_call.return_type = c4c::codegen::lir::LirTypeRef("i32");
  raw_compat_call.callee = "@raw";
  raw_compat_call.callee_type_suffix = "(i32)";
  raw_compat_call.args_str = "i32 %x";
  expect_true(raw_compat_call.structured_args.empty(),
              "raw direct LirCallOp construction should remain no-carrier compatibility");

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
  test_call_type_ref_rejects_stale_rendered_owner_miss();
  test_call_type_ref_preserves_no_owner_compatibility_name_id();
  test_rv64_direct_variadic_integer_extension_attrs();
  test_rv64_scalar_stdarg_uses_pointer_cursor();
  test_aarch64_scalar_stdarg_preserves_structured_va_list();
  test_structured_operand_identity_foundation();
  test_global_store_identity_contract();
  test_global_load_identity_contract();
  test_return_identity_contract();
  test_global_array_gep_identity_contract();
  test_direct_scalar_result_call_identity_boundary();
  test_direct_void_immediate_arg_identity_boundary();
  test_direct_void_ssa_arg_identity_boundary();
  test_scalar_ordinary_value_chain_identity_boundary();
  test_scalar_floating_binary_result_use_identity_boundary();
  test_scalar_cast_result_use_identity_boundary();
  test_scalar_fptrunc_result_use_identity_boundary();
  test_scalar_fpext_result_use_identity_boundary();
  test_scalar_sitofp_result_use_identity_boundary();
  test_scalar_uitofp_result_use_identity_boundary();
  test_scalar_fptosi_result_use_identity_boundary();
  test_scalar_fptoui_result_use_identity_boundary();
  test_scalar_compare_result_use_identity_boundary();
  test_scalar_floating_compare_result_use_identity_boundary();
  test_scalar_select_result_use_identity_boundary();
  test_wide_ffs_select_trunc_result_use_identity_boundary();
  test_scalar_abs_result_use_identity_boundary();

  std::cout << "PASS: frontend_lir_call_type_ref\n";
  return 0;
}
