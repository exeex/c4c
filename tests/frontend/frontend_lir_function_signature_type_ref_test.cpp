#include "arena.hpp"
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
#include <utility>
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
                     "frontend_lir_function_signature_type_ref_test.c");
  c4c::Node* root = parser.parse();
  auto result =
      c4c::sema::analyze_program(root, c4c::sema_profile_from(c4c::SourceProfile::C));

  expect_true(result.validation.ok,
              "fixture source should parse and validate successfully");
  expect_true(result.hir_module.has_value(),
              "fixture source should lower to HIR");
  return *result.hir_module;
}

void expect_hir_rejects(std::string_view source,
                        std::string_view expected_diagnostic,
                        const std::string& msg) {
  try {
    (void)lower_hir_module(source);
    fail(msg + ": expected HIR lowering to reject fixture");
  } catch (const std::runtime_error& err) {
    const std::string actual = err.what();
    if (actual.find(expected_diagnostic) == std::string::npos) {
      fail(msg + "\nExpected diagnostic fragment: " +
           std::string(expected_diagnostic) + "\nActual: " + actual);
    }
  }
}

const c4c::codegen::lir::LirFunction& require_function(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name,
    bool is_declaration) {
  const auto it = std::find_if(module.functions.begin(), module.functions.end(),
                               [&](const c4c::codegen::lir::LirFunction& fn) {
                                 return fn.name == name &&
                                        fn.is_declaration == is_declaration;
                               });
  expect_true(it != module.functions.end(),
              "fixture function should lower into LIR: " + std::string(name));
  return *it;
}

c4c::codegen::lir::LirFunction& require_mutable_function(
    c4c::codegen::lir::LirModule& module,
    std::string_view name,
    bool is_declaration) {
  auto it = std::find_if(module.functions.begin(), module.functions.end(),
                         [&](const c4c::codegen::lir::LirFunction& fn) {
                           return fn.name == name &&
                                  fn.is_declaration == is_declaration;
                         });
  expect_true(it != module.functions.end(),
              "fixture function should lower into LIR: " + std::string(name));
  return *it;
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

void expect_verify_rejects(const c4c::codegen::lir::LirModule& module,
                           const std::string& msg) {
  try {
    c4c::codegen::lir::verify_module(module);
    fail(msg);
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }
}

void expect_print_rejects(const c4c::codegen::lir::LirModule& module,
                          const std::string& msg) {
  try {
    (void)c4c::codegen::lir::print_llvm(module);
    fail(msg);
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }
}

void expect_struct_type_ref(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirTypeRef& type_ref,
    std::string_view expected_text,
    const std::string& msg) {
  expect_eq(type_ref.str(), expected_text, msg + " text should match");
  expect_true(type_ref.has_struct_name_id(), msg + " should carry a StructNameId");
  expect_eq(module.struct_names.spelling(type_ref.struct_name_id()), expected_text,
            msg + " StructNameId should resolve to the mirrored signature text");
}

void expect_vrm_type_ref(const c4c::codegen::lir::LirTypeRef& type_ref,
                         unsigned expected_width,
                         const std::string& msg) {
  const std::string expected_text = "c4c.vrm" + std::to_string(expected_width);
  expect_eq(type_ref.str(), expected_text, msg + " text should match");
  expect_true(type_ref.kind() == c4c::codegen::lir::LirTypeKind::VrmRegister,
              msg + " should use the dedicated VRM type kind");
  expect_true(type_ref.kind() != c4c::codegen::lir::LirTypeKind::Integer,
              msg + " must not decay to an integer kind");
  expect_true(type_ref.kind() != c4c::codegen::lir::LirTypeKind::Pointer,
              msg + " must not decay to a pointer kind");
  expect_true(type_ref.kind() != c4c::codegen::lir::LirTypeKind::Struct,
              msg + " must not decay to an aggregate kind");
  expect_true(type_ref.kind() != c4c::codegen::lir::LirTypeKind::Vector,
              msg + " must not reuse GCC vector storage kind");
  expect_true(type_ref.vrm_width().has_value(),
              msg + " should carry structured VRM width metadata");
  expect_eq(std::to_string(*type_ref.vrm_width()),
            std::to_string(expected_width),
            msg + " VRM width should match");
  expect_true(!type_ref.has_struct_name_id(),
              msg + " should not carry aggregate StructNameId metadata");
}

void expect_type_ref_structured_equality_uses_name_id(
    const c4c::codegen::lir::LirModule& module) {
  const c4c::StructNameId pair_id = module.struct_names.find("%struct.Pair");
  const c4c::StructNameId big_id = module.struct_names.find("%struct.Big");
  expect_true(pair_id != c4c::kInvalidStructName,
              "fixture should declare Pair for equality collision checks");
  expect_true(big_id != c4c::kInvalidStructName && big_id != pair_id,
              "fixture should declare Big for equality collision checks");

  const c4c::codegen::lir::LirTypeRef pair_ref =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.Pair", pair_id);
  const c4c::codegen::lir::LirTypeRef collision_ref =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.Pair", big_id);
  expect_true(pair_ref != collision_ref,
              "signature type-ref equality should reject same text with different StructNameId");

  expect_true(c4c::codegen::lir::LirTypeRef("%struct.Pair") ==
                  c4c::codegen::lir::LirTypeRef("%struct.Pair"),
              "signature legacy no-id type refs should still compare by rendered text");
}

void expect_struct_signature_refs(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirFunction& fn) {
  expect_true(fn.signature_return_type_ref.has_value(),
              "function should carry a return type mirror");
  expect_struct_type_ref(module, *fn.signature_return_type_ref, "%struct.Pair",
                         "return type mirror");

  expect_eq(std::to_string(fn.signature_param_type_refs.size()), "1",
            "function should carry one emitted parameter mirror");
  expect_struct_type_ref(module, fn.signature_param_type_refs[0], "%struct.Pair",
                         "parameter type mirror");
}

void expect_signature_aggregate_registered(
    const c4c::hir::Module& hir_module,
    const c4c::codegen::lir::LirModule& lir_module,
    std::string_view tag,
    std::string_view expected_lir_name) {
  const auto it = hir_module.struct_defs.find(std::string(tag));
  expect_true(it != hir_module.struct_defs.end(),
              "fixture should contain HIR aggregate definition: " + std::string(tag));
  const std::optional<c4c::hir::HirAggregateRef> hir_ref =
      it->second.aggregate_ref;
  expect_true(hir_ref.has_value() && hir_ref->complete(),
              "fixture HIR aggregate ref should be populated: " + std::string(tag));
  const c4c::codegen::lir::LirAggregateRef lir_ref =
      lir_module.find_aggregate_ref(hir_module, *hir_ref);
  expect_true(lir_ref.valid(),
              "signature aggregate ref should be interned in the LIR store: " +
                  std::string(tag));
  const c4c::codegen::lir::LirAggregateStoreEntry* entry =
      lir_module.find_aggregate(lir_ref);
  expect_true(entry != nullptr,
              "signature aggregate store entry should be present: " + std::string(tag));
  expect_eq(lir_module.struct_names.spelling(entry->name_id), expected_lir_name,
            "signature aggregate store entry should retain the LIR declaration name");
}

void remove_aggregate_store_entry_by_name(
    c4c::codegen::lir::LirModule& module,
    c4c::StructNameId name_id) {
  std::vector<c4c::hir::HirAggregateRef> removed_hir_refs;
  for (const auto& entry : module.aggregate_store) {
    if (entry.name_id == name_id) {
      removed_hir_refs.push_back(entry.hir_ref);
    }
  }
  module.aggregate_store.erase(
      std::remove_if(module.aggregate_store.begin(), module.aggregate_store.end(),
                     [&](const c4c::codegen::lir::LirAggregateStoreEntry& entry) {
                       return entry.name_id == name_id;
                     }),
      module.aggregate_store.end());
  module.aggregate_ref_by_hir_ref.clear();
  for (std::size_t index = 0; index < module.aggregate_store.size(); ++index) {
    const auto& entry = module.aggregate_store[index];
    const auto removed = std::find_if(
        removed_hir_refs.begin(), removed_hir_refs.end(),
        [&](const c4c::hir::HirAggregateRef& ref) {
          return ref.module.value == entry.hir_ref.module.value &&
                 ref.aggregate.value == entry.hir_ref.aggregate.value;
        });
    if (removed != removed_hir_refs.end()) continue;
    module.aggregate_ref_by_hir_ref.emplace(
        c4c::codegen::lir::LirModule::aggregate_store_key(entry.hir_ref),
        c4c::codegen::lir::LirAggregateRef{static_cast<uint32_t>(index)});
  }
}

c4c::codegen::lir::LirAggregateStoreEntry& require_aggregate_store_entry_by_name(
    c4c::codegen::lir::LirModule& module,
    c4c::StructNameId name_id,
    const std::string& msg) {
  const auto it = std::find_if(
      module.aggregate_store.begin(), module.aggregate_store.end(),
      [&](const c4c::codegen::lir::LirAggregateStoreEntry& entry) {
        return entry.name_id == name_id;
      });
  expect_true(it != module.aggregate_store.end(), msg);
  return *it;
}

std::string aggregate_decl_prefix(std::string_view llvm_ir) {
  const std::size_t end = llvm_ir.find("\n\n");
  return end == std::string_view::npos ? std::string(llvm_ir)
                                       : std::string(llvm_ir.substr(0, end));
}

void expect_byval_signature_refs(
    const c4c::codegen::lir::LirFunction& fn,
    std::string_view expected_param_text) {
  expect_true(fn.signature_return_type_ref.has_value(),
              "byval fixture should carry a return type mirror");
  expect_eq(fn.signature_return_type_ref->str(), "i32",
            "byval fixture return mirror should keep the emitted return type");
  expect_true(!fn.signature_return_type_ref->has_struct_name_id(),
              "non-aggregate return mirror should not carry a StructNameId");

  expect_eq(std::to_string(fn.signature_param_type_refs.size()), "1",
            "byval fixture should carry one emitted parameter mirror");
  const auto& param_ref = fn.signature_param_type_refs[0];
  expect_eq(param_ref.str(), expected_param_text,
            "byval parameter mirror should keep the emitted parameter type fragment");
  expect_true(!param_ref.has_struct_name_id(),
              "byval parameter mirror should stay raw when text is not the aggregate name");
}

void expect_single_signature_param(const c4c::codegen::lir::LirFunction& fn,
                                   std::string_view expected_name,
                                   c4c::TypeBase expected_base,
                                   bool expected_byval,
                                   const std::string& msg) {
  expect_eq(std::to_string(fn.signature_params.size()), "1",
            msg + " should carry one structured signature parameter");
  expect_eq(fn.signature_params[0].name, expected_name,
            msg + " should carry the lowered parameter name");
  expect_true(fn.signature_params[0].type.base == expected_base,
              msg + " should carry the HIR parameter type");
  expect_true(fn.signature_params[0].is_byval == expected_byval,
              msg + " should carry explicit structured byval metadata");
}

const c4c::codegen::lir::LirFunctionSignatureStoreEntry&
expect_function_signature_store_entry(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirFunction& fn,
    const std::string& msg) {
  expect_true(fn.function_signature_ref.valid(),
              msg + " should carry a module-owned function signature ref");
  const auto* entry = module.find_function_signature(fn.function_signature_ref);
  expect_true(entry != nullptr,
              msg + " should resolve its module-owned function signature ref");
  expect_true(entry->return_type_ref == fn.signature_return_type_ref &&
                  (!entry->return_type_ref.has_value() ||
                   entry->return_type_ref->str() ==
                       fn.signature_return_type_ref->str()),
              msg + " stored return type should mirror the structured return fact");
  expect_true(entry->fixed_param_type_refs == fn.signature_param_type_refs,
              msg + " stored parameter types should mirror structured param facts");
  for (std::size_t index = 0; index < entry->fixed_param_type_refs.size(); ++index) {
    expect_true(entry->fixed_param_type_refs[index].str() ==
                    fn.signature_param_type_refs[index].str(),
                msg + " stored parameter text should mirror structured param facts");
  }
  expect_true(entry->is_variadic == fn.signature_is_variadic,
              msg + " stored variadic fact should mirror the function fact");
  expect_true(entry->has_void_param_list == fn.signature_has_void_param_list,
              msg + " stored void-list fact should mirror the function fact");
  expect_eq(std::to_string(entry->fixed_param_is_byval.size()),
            std::to_string(fn.signature_params.size()),
            msg + " stored byval fact count should mirror structured params");
  for (std::size_t index = 0; index < entry->fixed_param_is_byval.size(); ++index) {
    expect_true(entry->fixed_param_is_byval[index] ==
                    fn.signature_params[index].is_byval,
                msg + " stored byval fact should mirror structured param metadata");
  }
  return *entry;
}

void expect_single_logical_param(const c4c::codegen::lir::LirFunction& fn,
                                 c4c::TypeBase expected_base,
                                 int expected_ptr_level,
                                 const std::string& msg) {
  expect_eq(std::to_string(fn.params.size()), "1",
            msg + " should carry one HIR-owned logical parameter");
  expect_true(fn.params[0].second.base == expected_base &&
                  fn.params[0].second.ptr_level == expected_ptr_level,
              msg + " should preserve its logical TypeSpec independently of ABI receipt");
}

void test_owned_type_spec_rejects_stale_rendered_compatibility() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct StaleOwnedCompat {
  int value;
};

struct StaleOwnedCompat owned_return(void);
void owned_param(struct StaleOwnedCompat input);
)c");

  c4c::Node missing_owner{};
  missing_owner.kind = c4c::NK_STRUCT_DEF;
  missing_owner.name = "StaleOwnedCompat";
  missing_owner.unqualified_name = "MissingOwnedCompatOwner";
  missing_owner.unqualified_text_id =
      hir_module.link_name_texts->intern("MissingOwnedCompatOwner");
  missing_owner.namespace_context_id = 707;

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

  c4c::hir::Function& return_fn =
      require_hir_function(hir_module, "owned_return", true);
  return_fn.return_type.spec = make_missing_owner_type();
  return_fn.return_type.aggregate_ref.reset();
  c4c::hir::HirAggregateOwnerIdentity missing_identity{};
  missing_identity.namespace_context_id = missing_owner.namespace_context_id;
  missing_identity.declaration_text_id = missing_owner.unqualified_text_id;
  missing_identity.canonical_tag_text_id = missing_owner.unqualified_text_id;
  return_fn.return_type.aggregate_owner_identity = missing_identity;
  c4c::hir::Function& param_fn =
      require_hir_function(hir_module, "owned_param", true);
  param_fn.params[0].type.spec = make_missing_owner_type();
  param_fn.params[0].type.aggregate_ref.reset();
  param_fn.params[0].type.aggregate_owner_identity = missing_identity;

  try {
    (void)c4c::codegen::lir::lower(hir_module);
    fail("missing canonical aggregate ref and incoherent owner metadata should reject");
  } catch (const std::runtime_error& err) {
    const std::string actual = err.what();
    expect_true(actual.find("LIR-owned aggregate function type requires") !=
                    std::string::npos,
                "incoherent owned aggregate metadata should fail before stale compatibility can be re-interned");
  }
}

void expect_lir_lower_rejects_registered_hir_ref(
    c4c::hir::Module hir_module,
    std::string_view expected_diagnostic,
    const std::string& msg) {
  try {
    (void)c4c::codegen::lir::lower(hir_module);
    fail(msg + ": expected LIR lowering to reject fixture");
  } catch (const std::runtime_error& err) {
    const std::string actual = err.what();
    if (actual.find(expected_diagnostic) == std::string::npos) {
      fail(msg + "\nExpected diagnostic fragment: " +
           std::string(expected_diagnostic) + "\nActual: " + actual);
    }
  }
}

void test_owned_type_spec_rejects_corrupted_populated_hir_refs() {
  c4c::hir::Module valid_module = lower_hir_module(R"c(
struct CanonicalSigRef {
  int value;
};

struct CanonicalSigRef corrupt_return(struct CanonicalSigRef input);
void corrupt_param(struct CanonicalSigRef input);
)c");

  const auto def_it = valid_module.struct_defs.find("CanonicalSigRef");
  expect_true(def_it != valid_module.struct_defs.end(),
              "fixture should contain the aggregate definition");
  expect_true(def_it->second.aggregate_ref.has_value() &&
                  valid_module.owns_aggregate_ref(*def_it->second.aggregate_ref),
              "fixture aggregate definition should carry an owned canonical ref");

  c4c::hir::Function& return_fn =
      require_hir_function(valid_module, "corrupt_return", true);
  c4c::hir::Function& param_fn =
      require_hir_function(valid_module, "corrupt_param", true);
  expect_true(return_fn.return_type.aggregate_ref == def_it->second.aggregate_ref &&
                  return_fn.params[0].type.aggregate_ref == def_it->second.aggregate_ref &&
                  param_fn.params[0].type.aggregate_ref == def_it->second.aggregate_ref,
              "fixture function signatures should start from populated canonical refs");

  c4c::hir::Module stale_return_ref = valid_module;
  require_hir_function(stale_return_ref, "corrupt_return", true)
      .return_type.aggregate_ref = stale_return_ref.issue_aggregate_ref();
  expect_lir_lower_rejects_registered_hir_ref(
      std::move(stale_return_ref),
      "LIR-owned aggregate function type requires a registered HIR aggregate ref",
      "return occurrence with an unregistered but module-owned HIR ref must not recover by rendered text");

  c4c::hir::Module wrong_module_return_ref = valid_module;
  c4c::hir::Module foreign_module;
  require_hir_function(wrong_module_return_ref, "corrupt_return", true)
      .return_type.aggregate_ref = foreign_module.issue_aggregate_ref();
  expect_lir_lower_rejects_registered_hir_ref(
      std::move(wrong_module_return_ref),
      "LIR-owned aggregate function type requires a registered HIR aggregate ref",
      "return occurrence with a foreign HIR ref must not recover by owner key or tag");

  c4c::hir::Module stale_param_ref = valid_module;
  require_hir_function(stale_param_ref, "corrupt_param", true)
      .params[0]
      .type.aggregate_ref = stale_param_ref.issue_aggregate_ref();
  expect_lir_lower_rejects_registered_hir_ref(
      std::move(stale_param_ref),
      "LIR-owned aggregate function type requires a registered HIR aggregate ref",
      "parameter occurrence with an unregistered HIR ref must not recover by rendered text");
}

void test_signature_type_ref_preserves_no_owner_compatibility_name_id() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct StaleNoOwnerCompat {
  int value;
};

struct StaleNoOwnerCompat no_owner_return(void);
)c");

  c4c::TypeSpec no_owner_type{};
  no_owner_type.base = c4c::TB_STRUCT;
  no_owner_type.tag_text_id =
      hir_module.link_name_texts->intern("StaleNoOwnerCompat");
  no_owner_type.namespace_context_id = -1;
  c4c::TextId incomplete_qualifier[] = {c4c::kInvalidText};
  no_owner_type.qualifier_text_ids = incomplete_qualifier;
  no_owner_type.n_qualifier_segments = 1;
  no_owner_type.array_size = -1;
  no_owner_type.inner_rank = -1;

  c4c::hir::QualType& no_owner_return_type =
      require_hir_function(hir_module, "no_owner_return", true).return_type;
  no_owner_return_type.spec = no_owner_type;
  no_owner_return_type.aggregate_owner_identity.reset();
  no_owner_return_type.aggregate_ref.reset();

  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);
  const auto& lowered = require_function(lir_module, "no_owner_return", true);
  expect_true(lowered.signature_return_type_ref.has_value(),
              "no-owner signature should carry a return type mirror");
  expect_struct_type_ref(lir_module, *lowered.signature_return_type_ref,
                         "%struct.StaleNoOwnerCompat",
                         "no-owner signature compatibility mirror");
}

void test_vrm_signature_type_refs_preserve_carrier_identity() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
__c4c_builtin_vrm1 vrm1_identity(__c4c_builtin_vrm1 input);
typedef __c4c_builtin_vrm2 vrm2_t;
vrm2_t vrm2_identity(vrm2_t input);
__c4c_builtin_vrm4 vrm4_identity(__c4c_builtin_vrm4 input);
typedef __c4c_builtin_vrm8 vrm8_t;
vrm8_t vrm8_identity(vrm8_t input);
)c");

  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);
  c4c::codegen::lir::verify_module(lir_module);

  for (const unsigned width : {1u, 2u, 4u, 8u}) {
    const std::string name = "vrm" + std::to_string(width) + "_identity";
    const auto& fn = require_function(lir_module, name, true);
    expect_true(fn.signature_return_type_ref.has_value(),
                name + " should carry a return type mirror");
    expect_vrm_type_ref(*fn.signature_return_type_ref, width,
                        name + " return type mirror");

    expect_eq(std::to_string(fn.signature_param_type_refs.size()), "1",
              name + " should carry one parameter mirror");
    expect_vrm_type_ref(fn.signature_param_type_refs[0], width,
                        name + " parameter type mirror");
    expect_single_signature_param(fn, "%p.input", c4c::TB_VRM_REGISTER, false,
                                  name + " signature metadata");
    expect_eq(std::to_string(fn.signature_params[0].type.vrm_width),
              std::to_string(width),
              name + " structured signature parameter should carry VRM width");
  }

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  for (const unsigned width : {1u, 2u, 4u, 8u}) {
    const std::string type = "c4c.vrm" + std::to_string(width);
    const std::string name = "vrm" + std::to_string(width) + "_identity";
    expect_true(llvm_ir.find("declare " + type + " @" + name + "(" + type + ")") !=
                    std::string::npos,
                name + " printer output should keep the VRM carrier spelling");
  }

  c4c::codegen::lir::LirModule scalar_return = lir_module;
  require_mutable_function(scalar_return, "vrm1_identity", true)
      .signature_return_type_ref = c4c::codegen::lir::LirTypeRef::integer(64);
  expect_verify_rejects(
      scalar_return,
      "verifier should reject a VRM signature return lowered as scalar integer");

  c4c::codegen::lir::LirModule vector_param = lir_module;
  require_mutable_function(vector_param, "vrm2_identity", true)
      .signature_param_type_refs[0] = c4c::codegen::lir::LirTypeRef("<2 x i64>");
  expect_verify_rejects(
      vector_param,
      "verifier should reject a VRM signature parameter lowered as GCC vector");

  c4c::codegen::lir::LirModule wrong_width_param = lir_module;
  require_mutable_function(wrong_width_param, "vrm4_identity", true)
      .signature_param_type_refs[0] = c4c::codegen::lir::LirTypeRef::vrm_register(8);
  expect_verify_rejects(
      wrong_width_param,
      "verifier should reject a VRM signature parameter with the wrong width");
}

void test_vrm_call_boundaries_reject_non_expanded_carriers() {
  constexpr std::string_view boundary_diag =
      "non-expanded VRM register carrier cannot cross function-call ABI boundary";

  expect_hir_rejects(R"c(
void vrm2_sink(__c4c_builtin_vrm2 input);
void call_vrm2_param(__c4c_builtin_vrm2 input) {
  vrm2_sink(input);
}
)c",
                     boundary_diag,
                     "bare VRM function parameter call boundary should reject");

  expect_hir_rejects(R"c(
__c4c_builtin_vrm1 make_vrm1(void);
void call_vrm1_result(void) {
  make_vrm1();
}
)c",
                     boundary_diag,
                     "bare VRM call result boundary should reject");

  expect_hir_rejects(R"c(
__c4c_builtin_vrm4 return_vrm4(__c4c_builtin_vrm4 input) {
  return input;
}
)c",
                     boundary_diag,
                     "bare VRM function return boundary should reject");

  expect_hir_rejects(R"c(
void call_vrm8_fn_ptr(__c4c_builtin_vrm8 input,
                      void (*callee)(__c4c_builtin_vrm8)) {
  callee(input);
}
)c",
                     boundary_diag,
                     "bare VRM function pointer parameter boundary should reject");
}

void test_definition_logical_parameter_publication() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
int declared_zero();
int defined_zero() { return 1; }

int declared_explicit_void(void);
int defined_explicit_void(void) { return 2; }

int declared_plain(int misleading_i64, long long misleading_i32,
                   float misleading_double, double misleading_float);
int defined_plain(int misleading_i64, long long misleading_i32,
                  float misleading_double, double misleading_float) {
  return 3;
}

int declared_non_one_to_one(char narrow, int *pointer);
int defined_non_one_to_one(char narrow, int *pointer) { return 4; }
)c");
  const c4c::codegen::lir::LirModule module =
      c4c::codegen::lir::lower(hir_module);

  const auto expect_empty = [&](std::string_view name, bool declaration) {
    const auto& function = require_function(module, name, declaration);
    expect_true(function.params.empty() && function.signature_params.empty() &&
                    function.signature_param_type_refs.empty() &&
                    !function.signature_has_void_param_list,
                "empty parameter-list declaration/definition should keep all parameter tracks empty");
  };
  expect_empty("declared_zero", true);
  expect_empty("defined_zero", false);

  const auto expect_explicit_void = [&](std::string_view name,
                                        bool declaration) {
    const auto& function = require_function(module, name, declaration);
    expect_true(function.signature_has_void_param_list &&
                    function.params.size() == 1 &&
                    function.params[0].second.base == c4c::TB_VOID &&
                    function.signature_params.empty() &&
                    function.signature_param_type_refs.empty(),
                "explicit-void declaration/definition should retain one logical void fact and no emitted fixed parameter");
  };
  expect_explicit_void("declared_explicit_void", true);
  expect_explicit_void("defined_explicit_void", false);

  const std::vector<c4c::TypeBase> expected_bases = {
      c4c::TB_INT, c4c::TB_LONGLONG, c4c::TB_FLOAT, c4c::TB_DOUBLE};
  const std::vector<c4c::codegen::lir::LirTypeKind> expected_kinds = {
      c4c::codegen::lir::LirTypeKind::Integer,
      c4c::codegen::lir::LirTypeKind::Integer,
      c4c::codegen::lir::LirTypeKind::Floating,
      c4c::codegen::lir::LirTypeKind::Floating};
  const std::vector<std::string> expected_text = {
      "i32", "i64", "float", "double"};
  const auto expect_plain = [&](std::string_view name, bool declaration) {
    const auto& function = require_function(module, name, declaration);
    expect_true(function.params.size() == expected_bases.size() &&
                    function.signature_params.size() == expected_bases.size() &&
                    function.signature_param_type_refs.size() ==
                        expected_bases.size(),
                "plain declaration/definition should publish three one-to-one parameter tracks");
    for (std::size_t index = 0; index < expected_bases.size(); ++index) {
      const auto& logical = function.params[index].second;
      const auto& signature = function.signature_params[index].type;
      const bool exact_plain_shape =
          logical.base == signature.base &&
          logical.ptr_level == signature.ptr_level &&
          logical.is_lvalue_ref == signature.is_lvalue_ref &&
          logical.is_rvalue_ref == signature.is_rvalue_ref &&
          logical.array_size == signature.array_size &&
          logical.array_rank == signature.array_rank &&
          logical.is_ptr_to_array == signature.is_ptr_to_array &&
          logical.inner_rank == signature.inner_rank &&
          logical.is_const == signature.is_const &&
          logical.is_volatile == signature.is_volatile &&
          logical.is_vector == signature.is_vector &&
          logical.vector_lanes == signature.vector_lanes &&
          logical.vector_bytes == signature.vector_bytes &&
          logical.is_fn_ptr == signature.is_fn_ptr;
      expect_true(logical.base == expected_bases[index] &&
                      signature.base == expected_bases[index] &&
                      exact_plain_shape && logical.ptr_level == 0 &&
                      !logical.is_lvalue_ref && !logical.is_rvalue_ref &&
                      logical.array_rank == 0 && !logical.is_ptr_to_array &&
                      !logical.is_const && !logical.is_volatile &&
                      !logical.is_vector && !logical.is_fn_ptr &&
                      function.signature_param_type_refs[index].kind() ==
                          expected_kinds[index] &&
                      function.signature_param_type_refs[index].str() ==
                          expected_text[index] &&
                      !function.signature_params[index].is_byval,
                  "plain logical/signature/type-ref order must come from exact structured HIR facts");
    }
  };
  expect_plain("declared_plain", true);
  expect_plain("defined_plain", false);

  const auto expect_non_one_to_one_preserved = [&](std::string_view name,
                                                    bool declaration) {
    const auto& function = require_function(module, name, declaration);
    expect_true(function.params.size() == 2 &&
                    function.params[0].second.base == c4c::TB_CHAR &&
                    function.params[1].second.base == c4c::TB_INT &&
                    function.params[1].second.ptr_level == 1,
                "narrow and pointer parameters should retain their logical shapes");
    expect_true(function.signature_params.size() == 2 &&
                    function.signature_param_type_refs.size() == 2 &&
                    function.signature_params[0].type.base == c4c::TB_CHAR &&
                    function.signature_params[0].type.ptr_level == 0 &&
                    function.signature_param_type_refs[0].kind() ==
                        c4c::codegen::lir::LirTypeKind::Integer &&
                    function.signature_param_type_refs[0].str() == "i8" &&
                    function.signature_params[1].type.base == c4c::TB_INT &&
                    function.signature_params[1].type.ptr_level == 1 &&
                    function.signature_param_type_refs[1].kind() ==
                        c4c::codegen::lir::LirTypeKind::Pointer &&
                    function.signature_param_type_refs[1].str() == "ptr",
                "narrow and pointer ABI mirrors should remain structured but receiver-blocked pending extension and pointee authority");
    c4c::codegen::lir::verify_module(module);
  };
  expect_non_one_to_one_preserved("declared_non_one_to_one", true);
  expect_non_one_to_one_preserved("defined_non_one_to_one", false);

  c4c::codegen::lir::LirModule presentation_drift = module;
  auto& drifted =
      require_mutable_function(presentation_drift, "defined_plain", false);
  for (std::size_t index = 0; index < drifted.params.size(); ++index) {
    drifted.params[index].first = "%logical-display-" + std::to_string(index);
    drifted.signature_params[index].name =
        "%signature-display-" + std::to_string(index);
  }
  drifted.signature_text =
      "define i32 @defined_plain(i8 %wrong, ptr %wrong2) {";
  c4c::codegen::lir::verify_module(presentation_drift);
  expect_true(drifted.params[0].second.base == c4c::TB_INT &&
                  drifted.params[1].second.base == c4c::TB_LONGLONG &&
                  drifted.params[2].second.base == c4c::TB_FLOAT &&
                  drifted.params[3].second.base == c4c::TB_DOUBLE,
              "parameter names and signature rendering must remain presentation-only");

  const auto expect_plain_mutation_rejects = [&](auto mutate,
                                                  const std::string& message) {
    c4c::codegen::lir::LirModule candidate = module;
    auto& function =
        require_mutable_function(candidate, "defined_plain", false);
    mutate(function);
    expect_verify_rejects(candidate, message);
  };
  expect_plain_mutation_rejects(
      [](auto& function) { function.params.pop_back(); },
      "plain fixed scalar verification should reject a missing logical parameter");
  expect_plain_mutation_rejects(
      [](auto& function) { function.signature_params.pop_back(); },
      "plain fixed scalar verification should reject a missing signature parameter");
  expect_plain_mutation_rejects(
      [](auto& function) { function.signature_param_type_refs.pop_back(); },
      "plain fixed scalar verification should reject a missing typed mirror");
  expect_plain_mutation_rejects(
      [](auto& function) {
        std::swap(function.params[0], function.params[1]);
      },
      "plain fixed scalar verification should reject reordered logical authority");
  expect_plain_mutation_rejects(
      [](auto& function) {
        function.signature_params[0].type.base = c4c::TB_UINT;
      },
      "plain fixed scalar verification should reject a conflicting scalar type");
  expect_plain_mutation_rejects(
      [](auto& function) { function.params[0].second.align_bytes = 16; },
      "plain fixed scalar verification should reject a conflicting logical shape");
  expect_plain_mutation_rejects(
      [](auto& function) {
        function.signature_param_type_refs[0] =
            c4c::codegen::lir::LirTypeRef::integer(64);
      },
      "plain fixed scalar verification should reject a conflicting mirror width");
}

void test_target_profile_long_signature_publication() {
  const auto verify_profile = [](c4c::TargetArch arch, std::string_view expected) {
    c4c::hir::Module hir_module = lower_hir_module(R"c(
void long_params(long signed_value, unsigned long unsigned_value) {}
)c");
    hir_module.target_profile = c4c::default_target_profile(arch);
    const auto module = c4c::codegen::lir::lower(hir_module);
    const auto& function = require_function(module, "long_params", false);
    expect_true(function.signature_param_type_refs.size() == 2 &&
                    function.signature_param_type_refs[0].str() == expected &&
                    function.signature_param_type_refs[1].str() == expected,
                "long parameter mirrors must follow the structured target profile");
    c4c::codegen::lir::verify_module(module);
  };
  verify_profile(c4c::TargetArch::I686, "i32");
  verify_profile(c4c::TargetArch::X86_64, "i64");
}

void test_aarch64_hfa_parameter_classification() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct HfaPair {
  float first;
  float second;
};

void declared_hfa(struct HfaPair input);
void defined_hfa(struct HfaPair input) {}
)c");
  hir_module.target_profile =
      c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
  const c4c::codegen::lir::LirModule module =
      c4c::codegen::lir::lower(hir_module);

  const auto expect_hfa = [&](std::string_view name, bool declaration) {
    const auto& function = require_function(module, name, declaration);
    expect_single_logical_param(function, c4c::TB_STRUCT, 0,
                                "AArch64 HFA classification");
    expect_true(function.signature_params.size() == 2 &&
                    function.signature_param_type_refs.size() == 2,
                "one logical HFA aggregate should expand into two structured ABI lanes without claiming receiver support");
    for (std::size_t lane = 0; lane < 2; ++lane) {
      expect_true(function.signature_params[lane].type.base == c4c::TB_FLOAT &&
                      function.signature_params[lane].type.ptr_level == 0 &&
                      !function.signature_params[lane].is_byval &&
                      function.signature_param_type_refs[lane].kind() ==
                          c4c::codegen::lir::LirTypeKind::Floating &&
                      function.signature_param_type_refs[lane].str() == "float",
                  "each expanded HFA lane should retain structured float ABI and mirror authority");
    }
  };
  expect_hfa("declared_hfa", true);
  expect_hfa("defined_hfa", false);
  c4c::codegen::lir::verify_module(module);
}

void test_direct_scalar_parameter_authority_alias_boundary() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
typedef unsigned long long ull;
ull scalar_alias(ull x) { return x + 1; }
)c");
  const c4c::codegen::lir::LirModule module = c4c::codegen::lir::lower(hir_module);
  const auto& function = require_function(module, "scalar_alias", false);
  expect_true(function.native_body_parameter_definitions.size() == 1,
              "typedef-backed ull parameter must publish one DirectScalar authority");
  const auto& definition = function.native_body_parameter_definitions.front();
  expect_true(definition.value.valid() && definition.parameter_index == 0 &&
                  definition.owner == function.link_name_id &&
                  definition.type == function.signature_param_type_refs[0] &&
                  definition.abi == c4c::codegen::lir::LirNativeBodyParameterAbi::DirectScalar,
              "DirectScalar authority must publish typed current-function identity");
  c4c::codegen::lir::verify_module(module);

  const auto rejects = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    auto& candidate_function = require_mutable_function(candidate, "scalar_alias", false);
    mutate(candidate, candidate_function);
    expect_verify_rejects(candidate, message);
  };
  rejects([](auto&, auto& function) { function.native_body_parameter_definitions.clear(); },
          "DirectScalar verifier must reject missing parameter authority");
  rejects([](auto&, auto& function) {
            function.native_body_parameter_definitions.front().value =
                c4c::codegen::lir::LirValueId::invalid();
          },
          "DirectScalar verifier must reject missing parameter identity");
  rejects([](auto& candidate, auto& function) {
            function.native_body_parameter_definitions.front().owner =
                candidate.link_names.intern("foreign_scalar_parameter_owner");
          },
          "DirectScalar verifier must reject foreign parameter ownership");
  rejects([](auto&, auto& function) {
            function.native_body_parameter_definitions.front().type =
                c4c::codegen::lir::LirTypeRef::integer(32);
          },
          "DirectScalar verifier must reject a type-incoherent parameter authority");
}

void test_direct_scalar_unary_fneg_lhs_authority() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
double scalar_unary_fneg_lhs(double x) { return -x; }
)c");
  const c4c::codegen::lir::LirModule module = c4c::codegen::lir::lower(hir_module);
  const auto find_fneg = [](const auto& function) -> const c4c::codegen::lir::LirBinOp& {
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        if (const auto* binary = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
            binary != nullptr &&
            binary->opcode.typed() == c4c::codegen::lir::LirBinaryOpcode::FNeg) {
          return *binary;
        }
      }
    }
    fail("scalar unary fneg fixture should lower one fneg LirBinOp");
  };
  const auto& function = require_function(module, "scalar_unary_fneg_lhs", false);
  const auto& binary = find_fneg(function);
  expect_true(binary.rhs.empty(), "unary fneg must have an empty rhs");
  expect_true(binary.scalar_lhs_parameter_authority.has_value(),
              "direct scalar unary fneg LHS must publish native parameter authority");
  const auto& authority = *binary.scalar_lhs_parameter_authority;
  const auto& definition = function.native_body_parameter_definitions.front();
  expect_true(binary.lhs.value_id() && authority.value == *binary.lhs.value_id() &&
                  authority.value == definition.value &&
                  authority.parameter_index == definition.parameter_index &&
                  authority.type == binary.type_str && authority.type == definition.type &&
                  authority.owner == function.link_name_id && authority.owner == definition.owner &&
                  authority.abi == c4c::codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
                  authority.role == c4c::codegen::lir::LirScalarBinaryParameterRole::Lhs,
              "direct scalar unary fneg authority must mirror its current-function definition");
  c4c::codegen::lir::verify_module(module);

  const auto rejects = [&](auto mutate, const std::string& message) {
    auto candidate = module;
    auto& candidate_function =
        require_mutable_function(candidate, "scalar_unary_fneg_lhs", false);
    for (auto& block : candidate_function.blocks) {
      for (auto& inst : block.insts) {
        if (auto* candidate_binary = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
            candidate_binary != nullptr &&
            candidate_binary->opcode.typed() == c4c::codegen::lir::LirBinaryOpcode::FNeg) {
          mutate(*candidate_binary);
          expect_verify_rejects(candidate, message);
          return;
        }
      }
    }
    fail("mutable scalar unary fneg fixture should contain one fneg LirBinOp");
  };
  rejects([](auto& binary) { binary.scalar_lhs_parameter_authority.reset(); },
          "DirectScalar unary fneg verifier must reject omitted authority");
  rejects([](auto& binary) {
            binary.scalar_lhs_parameter_authority->type =
                c4c::codegen::lir::LirTypeRef("float");
          },
          "DirectScalar unary fneg verifier must reject mismatched authority tuple");
  rejects([](auto& binary) { binary.rhs = binary.lhs; },
          "unary fneg verifier must reject a populated rhs");
}
}  // namespace

int main() {
  test_owned_type_spec_rejects_stale_rendered_compatibility();
  test_owned_type_spec_rejects_corrupted_populated_hir_refs();
  test_signature_type_ref_preserves_no_owner_compatibility_name_id();
  test_vrm_signature_type_refs_preserve_carrier_identity();
  test_vrm_call_boundaries_reject_non_expanded_carriers();
  test_definition_logical_parameter_publication();
  test_target_profile_long_signature_publication();
  test_aarch64_hfa_parameter_classification();
  test_direct_scalar_parameter_authority_alias_boundary();
  test_direct_scalar_unary_fneg_lhs_authority();

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

struct Pair declared_pair(struct Pair input);
int declared_big(struct Big input);

struct Pair defined_pair(struct Pair input) {
  return input;
}

int defined_big(struct Big input) {
  return (int)input.a;
}

int declared_variadic(int fixed, ...);
int defined_variadic(int fixed, ...) {
  return fixed;
}

int declared_void_params(void);
int defined_void_params(void) {
  return 1;
}
)c");
  hir_module.target_profile =
      c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);
  expect_type_ref_structured_equality_uses_name_id(lir_module);
  expect_signature_aggregate_registered(hir_module, lir_module, "Pair",
                                        "%struct.Pair");
  expect_signature_aggregate_registered(hir_module, lir_module, "Big",
                                        "%struct.Big");

  const auto& declared_pair = require_function(lir_module, "declared_pair", true);
  expect_single_logical_param(declared_pair, c4c::TB_STRUCT, 0,
                              "declared receiver-blocked direct aggregate classification");
  expect_struct_signature_refs(lir_module, declared_pair);
  expect_single_signature_param(declared_pair, "%p.input", c4c::TB_STRUCT, false,
                                "declared aggregate signature metadata");
  expect_function_signature_store_entry(
      lir_module, declared_pair,
      "declared aggregate function signature store entry");
  expect_true(!declared_pair.signature_is_variadic,
              "non-variadic declaration should carry a structured variadic=false flag");
  expect_true(!declared_pair.signature_has_void_param_list,
              "aggregate declaration should not carry a void-param-list flag");

  const auto& defined_pair = require_function(lir_module, "defined_pair", false);
  expect_single_logical_param(defined_pair, c4c::TB_STRUCT, 0,
                              "defined receiver-blocked direct aggregate classification");
  expect_struct_signature_refs(lir_module, defined_pair);
  expect_single_signature_param(defined_pair, "%p.input", c4c::TB_STRUCT, false,
                                "defined aggregate signature metadata");
  expect_function_signature_store_entry(
      lir_module, defined_pair,
      "defined aggregate function signature store entry");
  expect_true(!defined_pair.signature_is_variadic,
              "non-variadic definition should carry a structured variadic=false flag");
  expect_true(!defined_pair.signature_has_void_param_list,
              "aggregate definition should not carry a void-param-list flag");

  const std::string byval_param_text = "ptr byval(%struct.Big) align 8";
  const auto& declared_big = require_function(lir_module, "declared_big", true);
  expect_single_logical_param(declared_big, c4c::TB_STRUCT, 0,
                              "declared receiver-blocked byval aggregate classification");
  expect_byval_signature_refs(declared_big, byval_param_text);
  expect_single_signature_param(declared_big, "%p.input", c4c::TB_STRUCT, true,
                                "declared byval signature metadata");
  const auto& declared_big_signature = expect_function_signature_store_entry(
      lir_module, declared_big,
      "declared byval function signature store entry");
  expect_true(declared_big_signature.fixed_param_is_byval[0],
              "declared byval function signature store should retain byval ABI fact");

  const auto& defined_big = require_function(lir_module, "defined_big", false);
  expect_single_logical_param(defined_big, c4c::TB_STRUCT, 0,
                              "defined receiver-blocked byval aggregate classification");
  expect_byval_signature_refs(defined_big, byval_param_text);
  expect_single_signature_param(defined_big, "%p.input", c4c::TB_STRUCT, true,
                                "defined byval signature metadata");
  const auto& defined_big_signature = expect_function_signature_store_entry(
      lir_module, defined_big,
      "defined byval function signature store entry");
  expect_true(defined_big_signature.fixed_param_is_byval[0],
              "defined byval function signature store should retain byval ABI fact");

  const auto& declared_variadic =
      require_function(lir_module, "declared_variadic", true);
  expect_single_logical_param(declared_variadic, c4c::TB_INT, 0,
                              "declared receiver-blocked variadic fixed-prefix classification");
  expect_true(declared_variadic.signature_is_variadic,
              "variadic declaration should carry a structured variadic flag");
  expect_true(!declared_variadic.signature_has_void_param_list,
              "variadic declaration should not carry a void-param-list flag");
  expect_single_signature_param(declared_variadic, "%p.fixed", c4c::TB_INT, false,
                                "declared variadic signature metadata");
  expect_eq(std::to_string(declared_variadic.signature_param_type_refs.size()), "1",
            "variadic declaration should mirror only fixed parameters");
  const auto& declared_variadic_signature =
      expect_function_signature_store_entry(
          lir_module, declared_variadic,
          "declared variadic function signature store entry");
  expect_true(declared_variadic_signature.is_variadic,
              "declared variadic function signature store should retain variadic fact");

  const auto& defined_variadic =
      require_function(lir_module, "defined_variadic", false);
  expect_single_logical_param(defined_variadic, c4c::TB_INT, 0,
                              "defined receiver-blocked variadic fixed-prefix classification");
  expect_true(defined_variadic.signature_is_variadic,
              "variadic definition should carry a structured variadic flag");
  expect_true(!defined_variadic.signature_has_void_param_list,
              "variadic definition should not carry a void-param-list flag");
  expect_single_signature_param(defined_variadic, "%p.fixed", c4c::TB_INT, false,
                                "defined variadic signature metadata");
  expect_eq(std::to_string(defined_variadic.signature_param_type_refs.size()), "1",
            "variadic definition should mirror only fixed parameters");
  const auto& defined_variadic_signature = expect_function_signature_store_entry(
      lir_module, defined_variadic,
      "defined variadic function signature store entry");
  expect_true(defined_variadic_signature.is_variadic,
              "defined variadic function signature store should retain variadic fact");

  const auto& declared_void_params =
      require_function(lir_module, "declared_void_params", true);
  expect_true(declared_void_params.signature_has_void_param_list,
              "void-parameter declaration should carry a structured void-param-list flag");
  expect_true(!declared_void_params.signature_is_variadic,
              "void-parameter declaration should not be variadic");
  expect_eq(std::to_string(declared_void_params.signature_params.size()), "0",
            "void-parameter declaration should not expose a fixed signature parameter");
  expect_eq(std::to_string(declared_void_params.signature_param_type_refs.size()), "0",
            "void-parameter declaration should not expose a fixed parameter mirror");
  const auto& declared_void_signature = expect_function_signature_store_entry(
      lir_module, declared_void_params,
      "declared void-list function signature store entry");
  expect_true(declared_void_signature.has_void_param_list,
              "declared void-list function signature store should retain void-list fact");

  const auto& defined_void_params =
      require_function(lir_module, "defined_void_params", false);
  expect_true(defined_void_params.signature_has_void_param_list,
              "void-parameter definition should carry a structured void-param-list flag");
  expect_true(!defined_void_params.signature_is_variadic,
              "void-parameter definition should not be variadic");
  expect_eq(std::to_string(defined_void_params.signature_params.size()), "0",
            "void-parameter definition should not expose a fixed signature parameter");
  expect_eq(std::to_string(defined_void_params.signature_param_type_refs.size()), "0",
            "void-parameter definition should not expose a fixed parameter mirror");
  const auto& defined_void_signature = expect_function_signature_store_entry(
      lir_module, defined_void_params,
      "defined void-list function signature store entry");
  expect_true(defined_void_signature.has_void_param_list,
              "defined void-list function signature store should retain void-list fact");

  c4c::codegen::lir::verify_module(lir_module);

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("declare %struct.Pair @declared_pair(%struct.Pair)") !=
                  std::string::npos,
              "printer should preserve declaration signature_text as final output spelling");
  expect_true(llvm_ir.find("define %struct.Pair @defined_pair(%struct.Pair %p.input)") !=
                  std::string::npos,
              "printer should preserve definition signature_text as final output spelling");
  expect_true(llvm_ir.find("declare i32 @declared_big(ptr byval(%struct.Big) align 8)") !=
                  std::string::npos,
              "printer should preserve byval declaration signature_text as final output spelling");
  expect_true(llvm_ir.find("define i32 @defined_big(ptr byval(%struct.Big) align 8 %p.input)") !=
                  std::string::npos,
              "printer should preserve byval definition signature_text as final output spelling");

  const c4c::StructNameId pair_id = lir_module.struct_names.find("%struct.Pair");
  const c4c::StructNameId big_id = lir_module.struct_names.find("%struct.Big");
  expect_true(pair_id != c4c::kInvalidStructName,
              "fixture should declare Pair for structured verifier checks");
  expect_true(big_id != c4c::kInvalidStructName,
              "fixture should declare Big for mismatch verifier checks");

  c4c::codegen::lir::LirModule reordered_struct_decls = lir_module;
  std::reverse(reordered_struct_decls.struct_decls.begin(),
               reordered_struct_decls.struct_decls.end());
  reordered_struct_decls.struct_decl_index.clear();
  for (std::size_t index = 0; index < reordered_struct_decls.struct_decls.size();
       ++index) {
    reordered_struct_decls.struct_decl_index.emplace(
        reordered_struct_decls.struct_decls[index].name_id, index);
  }
  expect_eq(aggregate_decl_prefix(c4c::codegen::lir::print_llvm(reordered_struct_decls)),
            aggregate_decl_prefix(llvm_ir),
            "printer should render aggregate declarations from canonical store order");

  c4c::codegen::lir::LirModule missing_printer_store = lir_module;
  remove_aggregate_store_entry_by_name(missing_printer_store, pair_id);
  expect_print_rejects(
      missing_printer_store,
      "printer should reject missing canonical store facts before falling back to struct_decls");

  c4c::codegen::lir::LirModule populated_store_legacy_decl = lir_module;
  const c4c::StructNameId legacy_shadow_id =
      populated_store_legacy_decl.struct_names.intern("%struct.LegacyShadow");
  c4c::codegen::lir::LirStructDecl legacy_shadow_decl;
  legacy_shadow_decl.name_id = legacy_shadow_id;
  legacy_shadow_decl.fields.push_back({c4c::codegen::lir::LirTypeRef::integer(32)});
  populated_store_legacy_decl.type_decls.push_back(
      "%struct.LegacyShadow = type { i32 }");
  populated_store_legacy_decl.record_struct_decl(std::move(legacy_shadow_decl));
  const std::string populated_store_legacy_ir =
      c4c::codegen::lir::print_llvm(populated_store_legacy_decl);
  expect_true(populated_store_legacy_ir.find("%struct.Pair = type { i32, i32 }") !=
                  std::string::npos,
              "populated-store printer should keep canonical aggregate declarations");
  expect_true(populated_store_legacy_ir.find("%struct.LegacyShadow = type { i32 }") !=
                  std::string::npos,
              "populated-store printer should preserve legacy no-owner declarations");

  c4c::codegen::lir::LirModule no_owner_struct_decl;
  no_owner_struct_decl.link_name_texts = std::make_shared<c4c::TextTable>();
  no_owner_struct_decl.struct_names.attach_text_table(
      no_owner_struct_decl.link_name_texts.get());
  const c4c::StructNameId legacy_id =
      no_owner_struct_decl.struct_names.intern("%struct.Legacy");
  c4c::codegen::lir::LirStructDecl legacy_decl;
  legacy_decl.name_id = legacy_id;
  c4c::codegen::lir::LirStructField legacy_field;
  legacy_field.type = c4c::codegen::lir::LirTypeRef("i32");
  legacy_decl.fields.push_back(std::move(legacy_field));
  no_owner_struct_decl.type_decls.push_back("%struct.Legacy = type { i32 }");
  no_owner_struct_decl.record_struct_decl(std::move(legacy_decl));
  expect_true(c4c::codegen::lir::print_llvm(no_owner_struct_decl)
                      .find("%struct.Legacy = type { i32 }") !=
                  std::string::npos,
              "printer should preserve no-owner structured declaration compatibility");

  c4c::codegen::lir::LirModule stale_return_text = lir_module;
  require_mutable_function(stale_return_text, "declared_pair", true)
      .signature_return_type_ref->str() = "%struct.StaleMirrorText";
  expect_verify_rejects(
      stale_return_text,
      "verifier should reject a function whose stored signature disagrees with stale return facts");

  c4c::codegen::lir::LirModule stale_param_text = lir_module;
  require_mutable_function(stale_param_text, "defined_pair", false)
      .signature_param_type_refs[0]
      .str() = "%struct.StaleMirrorText";
  expect_verify_rejects(
      stale_param_text,
      "verifier should reject a function whose stored signature disagrees with stale parameter facts");

  c4c::codegen::lir::LirModule stale_rendered_return_text = lir_module;
  require_mutable_function(stale_rendered_return_text, "declared_pair", true)
      .signature_text = "declare void @declared_pair(void)";
  c4c::codegen::lir::verify_module(stale_rendered_return_text);

  c4c::codegen::lir::LirModule stale_rendered_param_text = lir_module;
  require_mutable_function(stale_rendered_param_text, "defined_pair", false)
      .signature_text = "define void @defined_pair(void) {";
  c4c::codegen::lir::verify_module(stale_rendered_param_text);

  c4c::codegen::lir::LirModule missing_return_name = lir_module;
  require_mutable_function(missing_return_name, "declared_pair", true)
      .signature_return_type_ref = c4c::codegen::lir::LirTypeRef("%struct.Pair");
  expect_verify_rejects(
      missing_return_name,
      "verifier should reject a known struct signature return without StructNameId");

  c4c::codegen::lir::LirModule missing_param_name = lir_module;
  require_mutable_function(missing_param_name, "defined_pair", false)
      .signature_param_type_refs[0] =
      c4c::codegen::lir::LirTypeRef("%struct.Pair");
  expect_verify_rejects(
      missing_param_name,
      "verifier should reject a known struct signature parameter without StructNameId");

  c4c::codegen::lir::LirModule mismatched_return_name = lir_module;
  auto& mismatched_return_fn =
      require_mutable_function(mismatched_return_name, "declared_pair", true);
  mismatched_return_fn.signature_return_type_ref =
      mismatched_return_fn.signature_return_type_ref->with_struct_name_id(big_id);
  expect_verify_rejects(
      mismatched_return_name,
      "verifier should reject a signature return with mismatched StructNameId");

  c4c::codegen::lir::LirModule mismatched_param_name = lir_module;
  auto& mismatched_param_fn =
      require_mutable_function(mismatched_param_name, "defined_pair", false);
  mismatched_param_fn.signature_param_type_refs[0] =
      mismatched_param_fn.signature_param_type_refs[0].with_struct_name_id(big_id);
  expect_verify_rejects(
      mismatched_param_name,
      "verifier should reject a signature parameter with mismatched StructNameId");

  c4c::codegen::lir::LirModule missing_return_store = lir_module;
  remove_aggregate_store_entry_by_name(missing_return_store, pair_id);
  expect_verify_rejects(
      missing_return_store,
      "verifier should reject a direct aggregate signature return without a matching aggregate store entry");

  c4c::codegen::lir::LirModule empty_return_store = lir_module;
  empty_return_store.aggregate_store.clear();
  empty_return_store.aggregate_ref_by_hir_ref.clear();
  expect_verify_rejects(
      empty_return_store,
      "verifier should reject a direct aggregate signature return when canonical store facts are absent");

  c4c::codegen::lir::LirModule wrong_kind_return_store = lir_module;
  auto& wrong_return_entry = require_aggregate_store_entry_by_name(
      wrong_kind_return_store, pair_id,
      "fixture should carry a Pair aggregate store entry for return kind corruption");
  wrong_return_entry.is_union = true;
  wrong_return_entry.layout_kind =
      c4c::codegen::lir::LirAggregateLayoutKind::Union;
  expect_verify_rejects(
      wrong_kind_return_store,
      "verifier should reject a direct aggregate signature return with incoherent aggregate store kind");

  c4c::codegen::lir::LirModule missing_param_store = lir_module;
  remove_aggregate_store_entry_by_name(missing_param_store, pair_id);
  require_mutable_function(missing_param_store, "declared_pair", true)
      .signature_return_type_ref.reset();
  expect_verify_rejects(
      missing_param_store,
      "verifier should reject a direct aggregate signature parameter without a matching aggregate store entry");

  c4c::codegen::lir::LirModule empty_param_store = lir_module;
  empty_param_store.aggregate_store.clear();
  empty_param_store.aggregate_ref_by_hir_ref.clear();
  require_mutable_function(empty_param_store, "declared_pair", true)
      .signature_return_type_ref.reset();
  expect_verify_rejects(
      empty_param_store,
      "verifier should reject a direct aggregate signature parameter when canonical store facts are absent");

  c4c::codegen::lir::LirModule wrong_kind_param_store = lir_module;
  require_mutable_function(wrong_kind_param_store, "declared_pair", true)
      .signature_return_type_ref.reset();
  auto& wrong_param_entry = require_aggregate_store_entry_by_name(
      wrong_kind_param_store, pair_id,
      "fixture should carry a Pair aggregate store entry for parameter kind corruption");
  wrong_param_entry.is_union = true;
  wrong_param_entry.layout_kind =
      c4c::codegen::lir::LirAggregateLayoutKind::Union;
  expect_verify_rejects(
      wrong_kind_param_store,
      "verifier should reject a direct aggregate signature parameter with incoherent aggregate store kind");

  c4c::codegen::lir::LirModule rendered_param_text_drift = lir_module;
  auto& drifted_param_fn =
      require_mutable_function(rendered_param_text_drift, "defined_pair", false);
  drifted_param_fn.signature_text =
      "define %struct.Pair @defined_pair(%struct.NotDeclared %p.input) {";
  drifted_param_fn.signature_param_type_refs[0] =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.StaleMirrorText",
                                                 pair_id);
  expect_verify_rejects(
      rendered_param_text_drift,
      "verifier should reject function signature store disagreement despite stale rendered text");

  c4c::codegen::lir::LirModule stale_signature_ref = lir_module;
  require_mutable_function(stale_signature_ref, "declared_pair", true)
      .function_signature_ref =
      c4c::codegen::lir::LirFunctionSignatureRef{
          static_cast<uint32_t>(
              stale_signature_ref.function_signature_store.size())};
  expect_verify_rejects(
      stale_signature_ref,
      "verifier should reject a stale function signature ref");

  c4c::codegen::lir::LirModule wrong_store_signature = lir_module;
  auto& wrong_store_decl =
      require_mutable_function(wrong_store_signature, "declared_pair", true);
  wrong_store_signature
      .function_signature_store[wrong_store_decl.function_signature_ref.value]
      .is_variadic = true;
  expect_verify_rejects(
      wrong_store_signature,
      "verifier should reject wrong-module function signature store facts");

  c4c::codegen::lir::LirModule byval_text_fallback = lir_module;
  require_mutable_function(byval_text_fallback, "declared_big", true)
      .signature_param_type_refs[0] = c4c::codegen::lir::LirTypeRef("i8");
  expect_verify_rejects(
      byval_text_fallback,
      "verifier should reject a byval signature parameter mirror text mismatch");

  c4c::codegen::lir::LirModule missing_byval_flag = lir_module;
  require_mutable_function(missing_byval_flag, "declared_big", true)
      .signature_params[0]
      .is_byval = false;
  expect_verify_rejects(
      missing_byval_flag,
      "verifier should reject byval mirror text without explicit byval metadata");

  c4c::codegen::lir::LirModule invalid_byval_shape = lir_module;
  require_mutable_function(invalid_byval_shape, "declared_pair", true)
      .signature_params[0]
      .is_byval = true;
  expect_verify_rejects(
      invalid_byval_shape,
      "verifier should reject explicit byval metadata without a byval mirror");

  c4c::codegen::lir::LirModule aggregate_param_module;
  c4c::codegen::lir::LirFunction aggregate_param_decl;
  aggregate_param_decl.name = "takes_complex";
  aggregate_param_decl.is_declaration = true;
  aggregate_param_decl.signature_text =
      "declare void @takes_complex({ double, double }, ptr byval(%struct.Big) align 8)";
  aggregate_param_decl.signature_param_type_refs.push_back(
      c4c::codegen::lir::LirTypeRef("{ double, double }"));
  aggregate_param_decl.signature_param_type_refs.push_back(
      c4c::codegen::lir::LirTypeRef("ptr"));
  aggregate_param_module.functions.push_back(std::move(aggregate_param_decl));
  c4c::codegen::lir::verify_module(aggregate_param_module);

  std::cout << "PASS: frontend_lir_function_signature_type_ref\n";
  return 0;
}
