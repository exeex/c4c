#include "parser/ast.hpp"
#include "sema/canonical_symbol.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

void expect_false(bool condition, const std::string& msg) {
  if (condition) fail(msg);
}

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

template <typename T>
auto legacy_tag_if_present(const T& ts, int) -> decltype(ts.tag) {
  return ts.tag;
}

const char* legacy_tag_if_present(const c4c::TypeSpec&, long) {
  return nullptr;
}

c4c::Node make_record(std::string_view name,
                      int namespace_context_id = 7,
                      c4c::TextId unqualified_text_id = 42) {
  c4c::Node record{};
  record.kind = c4c::NK_STRUCT_DEF;
  record.name = name.data();
  record.unqualified_name = name.data();
  record.unqualified_text_id = unqualified_text_id;
  record.namespace_context_id = namespace_context_id;
  return record;
}

void test_canonicalize_type_prefers_record_def_over_stale_tag() {
  c4c::Node record = make_record("StructuredCanonical");

  c4c::TypeSpec ts{};
  ts.base = c4c::TB_STRUCT;
  ts.record_def = &record;
  set_legacy_tag_if_present(ts, "StaleCanonical", 0);

  const c4c::sema::CanonicalType ct = c4c::sema::canonicalize_type(ts);
  expect_eq(ct.user_spelling, "StructuredCanonical",
            "canonical leaf spelling should prefer record_def metadata");
}

void test_typespec_from_canonical_preserves_final_spelling_compatibility() {
  c4c::sema::CanonicalType ct{};
  ct.kind = c4c::sema::CanonicalTypeKind::Struct;
  ct.source_base = c4c::TB_STRUCT;
  ct.user_spelling = "RenderedCanonical";

  const c4c::TypeSpec ts = c4c::sema::typespec_from_canonical(ct);

  expect_true(ts.base == c4c::TB_STRUCT,
              "canonical struct should reconstruct a struct TypeSpec");
  const char* final_spelling = legacy_tag_if_present(ts, 0);
  if (final_spelling) {
    expect_eq(final_spelling, "RenderedCanonical",
              "TypeSpec reconstruction should retain canonical final spelling");
  }
}

c4c::sema::CanonicalTemplateParamIdentity make_template_param_identity(
    int owner_namespace_context_id,
    c4c::TextId owner_text_id,
    int index,
    c4c::TextId param_text_id) {
  c4c::sema::CanonicalTemplateParamIdentity identity{};
  identity.owner_namespace_context_id = owner_namespace_context_id;
  identity.owner_text_id = owner_text_id;
  identity.index = index;
  identity.param_text_id = param_text_id;
  identity.domain = c4c::TemplateParamDomainKind::Type;
  return identity;
}

c4c::sema::CanonicalType make_primitive_type(c4c::sema::CanonicalTypeKind kind,
                                             c4c::TypeBase source_base,
                                             std::string_view spelling) {
  c4c::sema::CanonicalType type{};
  type.kind = kind;
  type.source_base = source_base;
  type.user_spelling = spelling;
  return type;
}

c4c::sema::CanonicalType make_nominal_type(
    c4c::sema::CanonicalTypeKind kind,
    c4c::TypeBase source_base,
    std::string_view spelling,
    int namespace_context_id,
    c4c::TextId unqualified_text_id) {
  c4c::sema::CanonicalType type = make_primitive_type(kind, source_base, spelling);
  type.identity.nominal_name.namespace_context_id = namespace_context_id;
  type.identity.nominal_name.unqualified_text_id = unqualified_text_id;
  return type;
}

c4c::sema::CanonicalTemplateArg make_type_arg(c4c::sema::CanonicalType type) {
  c4c::sema::CanonicalTemplateArg arg{};
  arg.type = std::make_shared<c4c::sema::CanonicalType>(std::move(type));
  return arg;
}

void test_template_substitution_uses_owner_identity_before_name() {
  const c4c::sema::CanonicalTemplateParamIdentity first_owner =
      make_template_param_identity(1, 100, 0, 500);
  const c4c::sema::CanonicalTemplateParamIdentity second_owner =
      make_template_param_identity(2, 200, 0, 500);

  c4c::sema::CanonicalTemplateParam first_param{};
  first_param.name = "T";
  first_param.identity = first_owner;

  c4c::sema::CanonicalTemplateParam second_param{};
  second_param.name = "T";
  second_param.identity = second_owner;
  second_param.identity.is_pack = true;
  second_param.identity.has_default = true;

  c4c::sema::CanonicalType use_second_owner{};
  use_second_owner.kind = c4c::sema::CanonicalTypeKind::TypedefName;
  use_second_owner.user_spelling = "T";
  use_second_owner.identity.template_param = second_owner;

  const c4c::sema::CanonicalType substituted = c4c::sema::substitute_template_args(
      use_second_owner,
      {first_param, second_param},
      {make_type_arg(make_primitive_type(c4c::sema::CanonicalTypeKind::Int,
                                         c4c::TB_INT,
                                         "int")),
       make_type_arg(make_primitive_type(c4c::sema::CanonicalTypeKind::Double,
                                         c4c::TB_DOUBLE,
                                         "double"))});

  expect_true(substituted.kind == c4c::sema::CanonicalTypeKind::Double,
              "owner-aware template parameter substitution should ignore declaration-only "
              "carrier flags");

  c4c::sema::CanonicalType identity_only_use = use_second_owner;
  identity_only_use.user_spelling.clear();
  const c4c::sema::CanonicalType identity_only_substituted =
      c4c::sema::substitute_template_args(
          identity_only_use,
          {first_param, second_param},
          {make_type_arg(make_primitive_type(c4c::sema::CanonicalTypeKind::Int,
                                             c4c::TB_INT,
                                             "int")),
           make_type_arg(make_primitive_type(c4c::sema::CanonicalTypeKind::Double,
                                             c4c::TB_DOUBLE,
                                             "double"))});

  expect_true(identity_only_substituted.kind == c4c::sema::CanonicalTypeKind::Double,
              "owner-aware template parameter substitution should not require a name");
}

void test_template_substitution_rejects_name_fallback_after_identity_miss() {
  const c4c::sema::CanonicalTemplateParamIdentity declared_owner =
      make_template_param_identity(1, 100, 0, 500);
  const c4c::sema::CanonicalTemplateParamIdentity other_owner =
      make_template_param_identity(2, 200, 0, 500);

  c4c::sema::CanonicalTemplateParam declared_param{};
  declared_param.name = "T";
  declared_param.identity = declared_owner;

  c4c::sema::CanonicalType use_other_owner{};
  use_other_owner.kind = c4c::sema::CanonicalTypeKind::TypedefName;
  use_other_owner.user_spelling = "T";
  use_other_owner.identity.template_param = other_owner;

  const c4c::sema::CanonicalType substituted = c4c::sema::substitute_template_args(
      use_other_owner,
      {declared_param},
      {make_type_arg(make_primitive_type(c4c::sema::CanonicalTypeKind::Int,
                                         c4c::TB_INT,
                                         "int"))});

  expect_true(substituted.kind == c4c::sema::CanonicalTypeKind::TypedefName,
              "complete owner-aware identity miss should not use name fallback");
  expect_eq(substituted.user_spelling, "T",
            "identity-mismatched template parameter should remain unsubstituted");
}

void test_template_substitution_keeps_no_metadata_name_fallback() {
  c4c::sema::CanonicalTemplateParam param{};
  param.name = "T";

  c4c::sema::CanonicalType use_legacy_param{};
  use_legacy_param.kind = c4c::sema::CanonicalTypeKind::TypedefName;
  use_legacy_param.user_spelling = "T";

  const c4c::sema::CanonicalType substituted = c4c::sema::substitute_template_args(
      use_legacy_param,
      {param},
      {make_type_arg(make_primitive_type(c4c::sema::CanonicalTypeKind::Char,
                                         c4c::TB_CHAR,
                                         "char"))});

  expect_true(substituted.kind == c4c::sema::CanonicalTypeKind::Char,
              "no-metadata template parameter substitution should use name fallback");
}

void test_types_equal_uses_record_def_before_spelling() {
  c4c::Node first_record = make_record("SharedRenderedName", 7, 42);
  c4c::Node second_record = make_record("SharedRenderedName", 8, 42);

  c4c::sema::CanonicalType first{};
  first.kind = c4c::sema::CanonicalTypeKind::Struct;
  first.source_base = c4c::TB_STRUCT;
  first.user_spelling = "SharedRenderedName";
  first.identity.record_def = &first_record;

  c4c::sema::CanonicalType second = first;
  second.identity.record_def = &second_record;

  expect_false(c4c::sema::types_equal(first, second),
               "same-spelled structured records with different record_def metadata should differ");

  second.identity.record_def = &first_record;
  second.user_spelling = "StaleRenderedName";
  expect_true(c4c::sema::types_equal(first, second),
              "matching record_def metadata should not require matching rendered spelling");
}

void test_types_equal_uses_nominal_name_before_spelling() {
  c4c::sema::CanonicalType first = make_nominal_type(
      c4c::sema::CanonicalTypeKind::TypedefName, c4c::TB_TYPEDEF, "Alias", 7, 100);
  c4c::sema::CanonicalType second = make_nominal_type(
      c4c::sema::CanonicalTypeKind::TypedefName, c4c::TB_TYPEDEF, "Alias", 8, 100);

  expect_false(c4c::sema::types_equal(first, second),
               "same-spelled typedef leaves from different nominal domains should differ");

  second.identity.nominal_name.namespace_context_id = 7;
  second.user_spelling = "StaleAlias";
  expect_true(c4c::sema::types_equal(first, second),
              "matching nominal metadata should not require matching rendered spelling");
}

void test_types_equal_uses_template_param_identity_before_spelling() {
  c4c::sema::CanonicalType first{};
  first.kind = c4c::sema::CanonicalTypeKind::TypedefName;
  first.source_base = c4c::TB_TYPEDEF;
  first.user_spelling = "T";
  first.identity.template_param = make_template_param_identity(1, 100, 0, 500);

  c4c::sema::CanonicalType second = first;
  second.identity.template_param = make_template_param_identity(2, 200, 0, 500);

  expect_false(c4c::sema::types_equal(first, second),
               "same-spelled template parameters with different owner metadata should differ");

  second.identity.template_param = first.identity.template_param;
  second.user_spelling.clear();
  expect_true(c4c::sema::types_equal(first, second),
              "matching template parameter metadata should not require rendered spelling");
}

void test_types_equal_keeps_no_metadata_spelling_fallback() {
  c4c::sema::CanonicalType first_nominal{};
  first_nominal.kind = c4c::sema::CanonicalTypeKind::Struct;
  first_nominal.source_base = c4c::TB_STRUCT;
  first_nominal.user_spelling = "Legacy";

  c4c::sema::CanonicalType second_nominal = first_nominal;
  expect_true(c4c::sema::types_equal(first_nominal, second_nominal),
              "no-metadata nominal equality should keep rendered spelling fallback");

  c4c::sema::CanonicalType first_template_param{};
  first_template_param.kind = c4c::sema::CanonicalTypeKind::TypedefName;
  first_template_param.source_base = c4c::TB_TYPEDEF;
  first_template_param.user_spelling = "T";

  c4c::sema::CanonicalType second_template_param = first_template_param;
  expect_true(c4c::sema::types_equal(first_template_param, second_template_param),
              "no-metadata template-parameter equality should keep rendered spelling fallback");
}

}  // namespace

int main() {
  test_canonicalize_type_prefers_record_def_over_stale_tag();
  test_typespec_from_canonical_preserves_final_spelling_compatibility();
  test_template_substitution_uses_owner_identity_before_name();
  test_template_substitution_rejects_name_fallback_after_identity_miss();
  test_template_substitution_keeps_no_metadata_name_fallback();
  test_types_equal_uses_record_def_before_spelling();
  test_types_equal_uses_nominal_name_before_spelling();
  test_types_equal_uses_template_param_identity_before_spelling();
  test_types_equal_keeps_no_metadata_spelling_fallback();
  std::cout << "PASS: cpp_hir_sema_canonical_symbol_metadata_test\n";
  return 0;
}
