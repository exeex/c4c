#include "hir/hir_ir.hpp"
#include "hir/compile_time_engine.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

c4c::hir::HirTemplateParameterBindingKey make_type_key(
    int owner_context, c4c::TextId owner_text_id, int parameter_index,
    c4c::TextId parameter_text_id) {
  c4c::hir::HirTemplateParameterBindingKey key{};
  key.parameter_kind = c4c::hir::HirTemplateParameterBindingKind::Type;
  key.owner_namespace_context_id = owner_context;
  key.owner_template_text_id = owner_text_id;
  key.parameter_index = parameter_index;
  key.parameter_text_id = parameter_text_id;
  return key;
}

void test_same_spelling_distinct_by_owner_and_index() {
  constexpr c4c::TextId kParamT = 11;
  auto owner_a_t0 = make_type_key(3, 101, 0, kParamT);
  auto owner_b_t0 = make_type_key(4, 202, 0, kParamT);
  auto owner_a_t1 = make_type_key(3, 101, 1, kParamT);

  expect_true(owner_a_t0 != owner_b_t0,
              "same-spelled parameters from different owners must be distinct");
  expect_true(owner_a_t0 != owner_a_t1,
              "same-spelled parameters at different indices must be distinct");

  std::set<c4c::hir::HirTemplateParameterBindingKey> ordered_keys;
  ordered_keys.insert(owner_a_t0);
  ordered_keys.insert(owner_b_t0);
  ordered_keys.insert(owner_a_t1);
  ordered_keys.insert(owner_a_t0);
  expect_true(ordered_keys.size() == 3,
              "ordering must preserve owner/index-distinct keys");
}

void test_complete_metadata_rejects_text_id_only_key() {
  c4c::hir::HirTemplateParameterBindingKey text_id_only{};
  text_id_only.parameter_kind = c4c::hir::HirTemplateParameterBindingKind::Type;
  text_id_only.parameter_text_id = 11;

  expect_true(
      !c4c::hir::hir_template_parameter_binding_key_has_complete_metadata(
          text_id_only),
      "raw parameter TextId alone must not count as complete binding metadata");

  auto complete = make_type_key(3, 101, 0, 11);
  expect_true(c4c::hir::hir_template_parameter_binding_key_has_complete_metadata(
                  complete),
              "owner text, parameter index, and parameter TextId should be complete");

  complete.owner_qualifier_segment_text_ids.push_back(c4c::kInvalidText);
  expect_true(
      !c4c::hir::hir_template_parameter_binding_key_has_complete_metadata(
          complete),
      "invalid owner qualifier TextIds must reject complete metadata");
}

void test_unordered_map_lookup_uses_structured_hash() {
  constexpr c4c::TextId kParamN = 17;
  c4c::hir::HirTemplateParameterBindingKey owner_a_n0 =
      make_type_key(8, 301, 0, kParamN);
  owner_a_n0.parameter_kind = c4c::hir::HirTemplateParameterBindingKind::NonType;
  c4c::hir::HirTemplateParameterBindingKey owner_b_n0 = owner_a_n0;
  owner_b_n0.owner_template_text_id = 302;
  c4c::hir::HirTemplateParameterBindingKey owner_a_n1 = owner_a_n0;
  owner_a_n1.parameter_index = 1;

  c4c::hir::HirTemplateNttpBindings bindings;
  bindings.emplace(owner_a_n0, 10);
  bindings.emplace(owner_b_n0, 20);
  bindings.emplace(owner_a_n1, 30);

  expect_true(bindings.size() == 3,
              "hash map must store owner/index-distinct NTTP bindings");
  expect_true(bindings.at(owner_a_n0) == 10,
              "hash lookup should find first owner binding");
  expect_true(bindings.at(owner_b_n0) == 20,
              "hash lookup should find second owner binding");
  expect_true(bindings.at(owner_a_n1) == 30,
              "hash lookup should find second-index binding");
}

void test_pack_element_identity_distinct_from_parameter_key() {
  c4c::hir::HirTemplateParameterBindingKey pack_param =
      make_type_key(8, 301, 2, 23);
  c4c::hir::HirTemplateParameterBindingKey pack_element_0 = pack_param;
  pack_element_0.pack_element_index = 0;
  c4c::hir::HirTemplateParameterBindingKey pack_element_1 = pack_param;
  pack_element_1.pack_element_index = 1;

  expect_true(pack_param != pack_element_0,
              "pack parameter key and pack element key must be distinct");
  expect_true(pack_element_0 != pack_element_1,
              "different pack elements must be distinct structured keys");

  c4c::hir::HirTemplateTypeBindings bindings;
  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  c4c::TypeSpec char_ts{};
  char_ts.base = c4c::TB_CHAR;
  bindings.emplace(pack_element_0, int_ts);
  bindings.emplace(pack_element_1, char_ts);

  expect_true(bindings.size() == 2,
              "hash map must preserve distinct pack element bindings");
  expect_true(bindings.at(pack_element_0).base == c4c::TB_INT,
              "pack element 0 binding should be preserved");
  expect_true(bindings.at(pack_element_1).base == c4c::TB_CHAR,
              "pack element 1 binding should be preserved");
}

void test_constructs_distinct_structured_keys_from_template_nodes() {
  c4c::Node owner_a{};
  owner_a.namespace_context_id = 8;
  owner_a.unqualified_text_id = 301;
  owner_a.n_template_params = 2;
  const char* owner_a_names[] = {"T", "T"};
  c4c::TextId owner_a_text_ids[] = {17, 17};
  owner_a.template_param_names = owner_a_names;
  owner_a.template_param_name_text_ids = owner_a_text_ids;

  c4c::Node owner_b = owner_a;
  owner_b.namespace_context_id = 9;
  owner_b.unqualified_text_id = 302;

  auto owner_a_t0 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner_a, 0, c4c::hir::HirTemplateParameterBindingKind::Type);
  auto owner_a_t1 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner_a, 1, c4c::hir::HirTemplateParameterBindingKind::Type);
  auto owner_b_t0 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner_b, 0, c4c::hir::HirTemplateParameterBindingKind::Type);

  expect_true(owner_a_t0.has_value(), "owner A index 0 should produce a key");
  expect_true(owner_a_t1.has_value(), "owner A index 1 should produce a key");
  expect_true(owner_b_t0.has_value(), "owner B index 0 should produce a key");
  expect_true(*owner_a_t0 != *owner_a_t1,
              "same-spelled template params on distinct indices must differ");
  expect_true(*owner_a_t0 != *owner_b_t0,
              "same-spelled template params on distinct owners must differ");

  c4c::hir::HirTemplateTypeBindings bindings;
  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  c4c::TypeSpec char_ts{};
  char_ts.base = c4c::TB_CHAR;
  c4c::TypeSpec long_ts{};
  long_ts.base = c4c::TB_LONG;
  bindings.emplace(*owner_a_t0, int_ts);
  bindings.emplace(*owner_a_t1, char_ts);
  bindings.emplace(*owner_b_t0, long_ts);
  expect_true(bindings.size() == 3,
              "structured type bindings must keep owner/index keys distinct");
  expect_true(bindings.at(*owner_a_t0).base == c4c::TB_INT,
              "owner A index 0 binding should be preserved");
  expect_true(bindings.at(*owner_a_t1).base == c4c::TB_CHAR,
              "owner A index 1 binding should be preserved");
  expect_true(bindings.at(*owner_b_t0).base == c4c::TB_LONG,
              "owner B index 0 binding should be preserved");
}

void test_call_binding_parity_observes_structured_maps() {
  c4c::Node owner{};
  owner.namespace_context_id = 8;
  owner.unqualified_text_id = 301;
  owner.n_template_params = 2;
  const char* names[] = {"T", "N"};
  c4c::TextId text_ids[] = {17, 19};
  bool is_nttp[] = {false, true};
  owner.template_param_names = names;
  owner.template_param_name_text_ids = text_ids;
  owner.template_param_is_nttp = is_nttp;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;

  c4c::hir::TypeBindings legacy_type_bindings;
  legacy_type_bindings.emplace("T", int_ts);
  c4c::hir::NttpBindings legacy_nttp_bindings;
  legacy_nttp_bindings.emplace("N", 42);

  auto type_key = c4c::hir::make_hir_template_parameter_binding_key(
      &owner, 0, c4c::hir::HirTemplateParameterBindingKind::Type);
  auto nttp_key = c4c::hir::make_hir_template_parameter_binding_key(
      &owner, 1, c4c::hir::HirTemplateParameterBindingKind::NonType);
  expect_true(type_key.has_value(), "type parameter should produce a key");
  expect_true(nttp_key.has_value(), "NTTP parameter should produce a key");

  c4c::hir::HirTemplateTypeBindings structured_type_bindings;
  structured_type_bindings.emplace(*type_key, int_ts);
  c4c::hir::HirTemplateNttpBindings structured_nttp_bindings;
  structured_nttp_bindings.emplace(*nttp_key, 42);

  auto parity = c4c::hir::observe_hir_template_call_binding_parity(
      &owner, legacy_type_bindings, structured_type_bindings,
      legacy_nttp_bindings, structured_nttp_bindings);
  expect_true(parity.ok(), "structured call bindings should match legacy maps");
  expect_true(parity.type_bindings_checked == 1,
              "parity should check the structured type binding");
  expect_true(parity.nttp_bindings_checked == 1,
              "parity should check the structured NTTP binding");

  structured_nttp_bindings[*nttp_key] = 7;
  parity = c4c::hir::observe_hir_template_call_binding_parity(
      &owner, legacy_type_bindings, structured_type_bindings,
      legacy_nttp_bindings, structured_nttp_bindings);
  expect_true(!parity.ok(), "parity should report structured/legacy mismatch");
  expect_true(parity.nttp_bindings_mismatched == 1,
              "parity should count mismatched NTTP values");
}

void test_legacy_name_dual_write_helpers_require_complete_owner_metadata() {
  c4c::Node owner{};
  owner.namespace_context_id = 8;
  owner.unqualified_text_id = 301;
  owner.n_template_params = 4;
  const char* names[] = {"T", "N", "Pack", "Nums"};
  c4c::TextId text_ids[] = {17, 19, 23, 29};
  bool is_nttp[] = {false, true, false, true};
  bool is_pack[] = {false, false, true, true};
  owner.template_param_names = names;
  owner.template_param_name_text_ids = text_ids;
  owner.template_param_is_nttp = is_nttp;
  owner.template_param_is_pack = is_pack;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  c4c::TypeSpec char_ts{};
  char_ts.base = c4c::TB_CHAR;

  c4c::hir::HirTemplateTypeBindings structured_type_bindings;
  c4c::hir::HirTemplateNttpBindings structured_nttp_bindings;
  expect_true(c4c::hir::add_hir_template_type_binding_by_legacy_name(
                  &owner, "T", int_ts, &structured_type_bindings),
              "type legacy-name binding should dual-write with complete metadata");
  expect_true(c4c::hir::add_hir_template_nttp_binding_by_legacy_name(
                  &owner, "N", 42, &structured_nttp_bindings),
              "NTTP legacy-name binding should dual-write with complete metadata");

  auto type_key = c4c::hir::make_hir_template_parameter_binding_key(
      &owner, 0, c4c::hir::HirTemplateParameterBindingKind::Type);
  auto nttp_key = c4c::hir::make_hir_template_parameter_binding_key(
      &owner, 1, c4c::hir::HirTemplateParameterBindingKind::NonType);
  expect_true(type_key.has_value(), "type parameter should produce a key");
  expect_true(nttp_key.has_value(), "NTTP parameter should produce a key");
  expect_true(structured_type_bindings.at(*type_key).base == c4c::TB_INT,
              "dual-written structured type binding should be keyed by owner");
  expect_true(structured_nttp_bindings.at(*nttp_key) == 42,
              "dual-written structured NTTP binding should be keyed by owner");

  expect_true(c4c::hir::add_hir_template_type_binding_by_legacy_name(
                  &owner, "Pack#0", int_ts, &structured_type_bindings),
              "type pack element 0 should dual-write with complete metadata");
  expect_true(c4c::hir::add_hir_template_type_binding_by_legacy_name(
                  &owner, "Pack#1", char_ts, &structured_type_bindings),
              "type pack element 1 should dual-write with complete metadata");
  expect_true(c4c::hir::add_hir_template_nttp_binding_by_legacy_name(
                  &owner, "Nums#0", 7, &structured_nttp_bindings),
              "NTTP pack element 0 should dual-write with complete metadata");
  expect_true(c4c::hir::add_hir_template_nttp_binding_by_legacy_name(
                  &owner, "Nums#1", 9, &structured_nttp_bindings),
              "NTTP pack element 1 should dual-write with complete metadata");

  auto type_pack_0 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner, 2, c4c::hir::HirTemplateParameterBindingKind::Type, 0);
  auto type_pack_1 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner, 2, c4c::hir::HirTemplateParameterBindingKind::Type, 1);
  auto nttp_pack_0 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner, 3, c4c::hir::HirTemplateParameterBindingKind::NonType, 0);
  auto nttp_pack_1 = c4c::hir::make_hir_template_parameter_binding_key(
      &owner, 3, c4c::hir::HirTemplateParameterBindingKind::NonType, 1);
  expect_true(type_pack_0.has_value(), "type pack element 0 should produce a key");
  expect_true(type_pack_1.has_value(), "type pack element 1 should produce a key");
  expect_true(nttp_pack_0.has_value(), "NTTP pack element 0 should produce a key");
  expect_true(nttp_pack_1.has_value(), "NTTP pack element 1 should produce a key");
  expect_true(structured_type_bindings.at(*type_pack_0).base == c4c::TB_INT,
              "type pack element 0 binding should be keyed by pack index");
  expect_true(structured_type_bindings.at(*type_pack_1).base == c4c::TB_CHAR,
              "type pack element 1 binding should be keyed by pack index");
  expect_true(structured_nttp_bindings.at(*nttp_pack_0) == 7,
              "NTTP pack element 0 binding should be keyed by pack index");
  expect_true(structured_nttp_bindings.at(*nttp_pack_1) == 9,
              "NTTP pack element 1 binding should be keyed by pack index");

  expect_true(!c4c::hir::add_hir_template_type_binding_by_legacy_name(
                  &owner, "Pack", int_ts, &structured_type_bindings),
              "pack parameters without an element suffix should not dual-write");

  owner.unqualified_text_id = c4c::kInvalidText;
  expect_true(!c4c::hir::add_hir_template_type_binding_by_legacy_name(
                  &owner, "T", int_ts, &structured_type_bindings),
              "incomplete owner metadata should prevent structured dual-write");
}

void test_specialization_argument_identity_uses_structured_parameter_key() {
  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;

  c4c::hir::SpecializationArgumentIdentity owner_a;
  owner_a.parameter_name = "T";
  owner_a.parameter_key = make_type_key(8, 301, 0, 17);
  owner_a.kind = c4c::hir::SpecializationArgumentKind::Type;
  owner_a.type = int_ts;

  c4c::hir::SpecializationArgumentIdentity owner_b = owner_a;
  owner_b.parameter_key = make_type_key(9, 302, 0, 17);
  c4c::hir::SpecializationArgumentIdentity different_text_id = owner_a;
  different_text_id.parameter_key = make_type_key(8, 301, 0, 19);
  c4c::hir::SpecializationArgumentIdentity pack_element_0 = owner_a;
  pack_element_0.parameter_key->pack_element_index = 0;
  c4c::hir::SpecializationArgumentIdentity pack_element_1 = owner_a;
  pack_element_1.parameter_key->pack_element_index = 1;
  c4c::hir::SpecializationArgumentIdentity same_key_different_display =
      owner_a;
  same_key_different_display.parameter_name = "DifferentDisplayName";

  expect_true(owner_a == same_key_different_display,
              "complete structured identities with the same key should ignore display names for equality");
  expect_true(!(owner_a < same_key_different_display),
              "same structured key must not order before a different display-name mirror");
  expect_true(!(same_key_different_display < owner_a),
              "different display-name mirror must not order before the same structured key");

  expect_true(owner_a != owner_b,
              "same rendered parameter name from different owners must not compare equal");
  expect_true(owner_a != different_text_id,
              "same rendered parameter name with different parameter TextId must not compare equal");
  expect_true(pack_element_0 != pack_element_1,
              "same rendered pack parameter name with different element indices must not compare equal");

  std::unordered_set<
      c4c::hir::SpecializationArgumentIdentity,
      c4c::hir::SpecializationArgumentIdentityHash>
      structured_identities;
  structured_identities.insert(owner_a);
  structured_identities.insert(same_key_different_display);
  structured_identities.insert(owner_b);
  structured_identities.insert(different_text_id);
  structured_identities.insert(pack_element_0);
  structured_identities.insert(pack_element_1);
  expect_true(structured_identities.size() == 5,
              "structured argument identity hash/equality must ignore display names but preserve distinct parameter keys");

  c4c::hir::SpecializationArgumentIdentity fallback_a;
  fallback_a.parameter_name = "T";
  fallback_a.kind = c4c::hir::SpecializationArgumentKind::Type;
  fallback_a.type = int_ts;
  c4c::hir::SpecializationArgumentIdentity fallback_b = fallback_a;
  expect_true(fallback_a == fallback_b,
              "fallback identities should preserve legacy parameter-name equality");

  std::unordered_set<
      c4c::hir::SpecializationArgumentIdentity,
      c4c::hir::SpecializationArgumentIdentityHash>
      fallback_identities;
  fallback_identities.insert(fallback_a);
  fallback_identities.insert(fallback_b);
  expect_true(fallback_identities.size() == 1,
              "fallback identity hash/equality should remain parameter-name compatible");
}

void test_pending_template_binding_identity_helpers_accept_structured_maps() {
  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  c4c::TypeSpec char_ts{};
  char_ts.base = c4c::TB_CHAR;

  auto owner_a_t = make_type_key(8, 301, 0, 17);
  auto owner_b_t = make_type_key(9, 302, 0, 17);
  c4c::hir::HirTemplateTypeBindings structured_type_bindings;
  structured_type_bindings.emplace(owner_a_t, int_ts);
  structured_type_bindings.emplace(owner_b_t, char_ts);

  auto type_identities =
      c4c::hir::make_pending_template_type_binding_identities(
          structured_type_bindings);
  expect_true(type_identities.size() == 2,
              "structured type binding helper should emit one identity per structured key");
  expect_true(type_identities[0] != type_identities[1],
              "structured type identities with same rendered name should remain distinct");
  expect_true(type_identities[0].has_complete_structured_parameter_identity(),
              "structured type identity should carry complete parameter metadata");
  expect_true(type_identities[1].has_complete_structured_parameter_identity(),
              "second structured type identity should carry complete parameter metadata");

  auto nttp_pack_0 = make_type_key(8, 301, 1, 19);
  nttp_pack_0.parameter_kind = c4c::hir::HirTemplateParameterBindingKind::NonType;
  nttp_pack_0.pack_element_index = 0;
  auto nttp_pack_1 = nttp_pack_0;
  nttp_pack_1.pack_element_index = 1;
  c4c::hir::HirTemplateNttpBindings structured_nttp_bindings;
  structured_nttp_bindings.emplace(nttp_pack_0, 7);
  structured_nttp_bindings.emplace(nttp_pack_1, 9);

  auto nttp_identities =
      c4c::hir::make_pending_template_nttp_binding_identities(
          structured_nttp_bindings);
  expect_true(nttp_identities.size() == 2,
              "structured NTTP binding helper should emit one identity per structured key");
  expect_true(nttp_identities[0] != nttp_identities[1],
              "structured NTTP pack element identities should remain distinct");
  expect_true(nttp_identities[0].has_complete_structured_parameter_identity(),
              "structured NTTP identity should carry complete parameter metadata");
  expect_true(nttp_identities[1].has_complete_structured_parameter_identity(),
              "second structured NTTP identity should carry complete parameter metadata");

  c4c::hir::TypeBindings legacy_type_bindings;
  legacy_type_bindings.emplace("T", int_ts);
  auto legacy_type_identities =
      c4c::hir::make_pending_template_type_binding_identities(
          legacy_type_bindings);
  expect_true(legacy_type_identities.size() == 1,
              "legacy type binding helper should keep old name-based output");
  expect_true(legacy_type_identities[0].parameter_name == "T",
              "legacy type binding identity should preserve parameter display name");
  expect_true(!legacy_type_identities[0].has_complete_structured_parameter_identity(),
              "legacy type binding identity should not invent structured metadata");

  c4c::Node pending_owner{};
  pending_owner.name = const_cast<char*>("Box");
  pending_owner.namespace_context_id = 4;
  pending_owner.unqualified_text_id = 401;
  c4c::TypeSpec pending_ts{};
  pending_ts.base = c4c::TB_STRUCT;
  pending_ts.tpl_struct_origin = "Box";
  c4c::hir::SourceSpan span{};
  span.begin.line = 12;
  span.end.line = 12;

  auto legacy_key = c4c::hir::make_pending_template_type_key(
      c4c::hir::PendingTemplateTypeKind::OwnerStruct, pending_ts,
      &pending_owner, legacy_type_bindings, c4c::hir::NttpBindings{},
      "test-structured-observation", span);
  auto structured_key = c4c::hir::make_pending_template_type_key(
      c4c::hir::PendingTemplateTypeKind::OwnerStruct, pending_ts,
      &pending_owner, structured_type_bindings, structured_nttp_bindings,
      "test-structured-observation", span);
  auto observation =
      c4c::hir::observe_pending_template_type_structured_identity(
          legacy_key, structured_key);

  expect_true(observation.structured_key_constructed,
              "structured pending key observation should report construction");
  expect_true(observation.static_context_matches,
              "structured pending key observation should preserve static pending context");
  expect_true(observation.structured_key_differs_from_legacy_key,
              "structured pending key should stay distinct from legacy fallback identity while legacy remains authoritative");
  expect_true(observation.structured_type_bindings == 2,
              "structured pending key should report structured type bindings");
  expect_true(observation.structured_nttp_bindings == 2,
              "structured pending key should report structured NTTP bindings");
  expect_true(observation.structured_bindings_complete(),
              "structured pending key helper should emit complete structured identities");
}

void test_pending_template_state_can_use_structured_identity_key() {
  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;

  c4c::Node pending_owner{};
  pending_owner.name = const_cast<char*>("Box");
  pending_owner.namespace_context_id = 4;
  pending_owner.unqualified_text_id = 401;

  c4c::TypeSpec pending_ts{};
  pending_ts.base = c4c::TB_STRUCT;
  pending_ts.tpl_struct_origin = "Box";

  c4c::hir::TypeBindings legacy_type_bindings;
  legacy_type_bindings.emplace("T", int_ts);
  c4c::hir::NttpBindings legacy_nttp_bindings;

  c4c::hir::HirTemplateTypeBindings structured_type_bindings;
  structured_type_bindings.emplace(make_type_key(8, 301, 0, 17), int_ts);
  c4c::hir::HirTemplateNttpBindings structured_nttp_bindings;

  c4c::hir::SourceSpan span{};
  span.begin.line = 21;
  span.end.line = 21;

  auto legacy_key = c4c::hir::make_pending_template_type_key(
      c4c::hir::PendingTemplateTypeKind::OwnerStruct, pending_ts,
      &pending_owner, legacy_type_bindings, legacy_nttp_bindings,
      "test-structured-state-key", span);
  auto structured_key = c4c::hir::make_pending_template_type_key(
      c4c::hir::PendingTemplateTypeKind::OwnerStruct, pending_ts,
      &pending_owner, structured_type_bindings, structured_nttp_bindings,
      "test-structured-state-key", span);
  auto observation =
      c4c::hir::observe_pending_template_type_structured_identity(
          legacy_key, structured_key);
  expect_true(c4c::hir::pending_template_structured_identity_can_key_state(
                  observation, legacy_type_bindings.size(),
                  legacy_nttp_bindings.size()),
              "complete structured pending metadata should be eligible for state-key authority");

  c4c::hir::CompileTimeState state;
  const bool recorded = state.record_pending_template_type_with_identity_key(
      c4c::hir::PendingTemplateTypeKind::OwnerStruct, pending_ts,
      &pending_owner, legacy_type_bindings, legacy_nttp_bindings, span,
      "test-structured-state-key", structured_key);
  expect_true(recorded,
              "structured pending state key should record a pending work item");
  expect_true(state.pending_template_type_count() == 1,
              "structured pending state key should record one pending item");
  const auto& item = state.pending_template_types().front();
  expect_true(item.identity_key == structured_key,
              "pending dedup/progress identity should use the structured key");
  expect_true(!(item.identity_key == legacy_key),
              "structured pending state key should not collapse to the legacy fallback key");
  expect_true(item.display_key.find("T=") != std::string::npos,
              "pending display key should preserve legacy rendered binding output");
  state.mark_pending_template_type_resolved(structured_key);
  expect_true(state.is_pending_template_type_resolved(structured_key),
              "pending resolved progress should be tracked by the structured key");
  expect_true(!state.is_pending_template_type_resolved(legacy_key),
              "legacy key should not become the resolved progress key when structured metadata is complete");
}

void test_pending_template_state_rejects_incomplete_structured_identity_key() {
  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;

  c4c::Node pending_owner{};
  pending_owner.name = const_cast<char*>("Box");
  pending_owner.namespace_context_id = 4;
  pending_owner.unqualified_text_id = 401;

  c4c::TypeSpec pending_ts{};
  pending_ts.base = c4c::TB_STRUCT;
  pending_ts.tpl_struct_origin = "Box";

  c4c::hir::TypeBindings legacy_type_bindings;
  legacy_type_bindings.emplace("T", int_ts);
  c4c::hir::HirTemplateTypeBindings incomplete_structured_type_bindings;
  auto incomplete_key = make_type_key(8, 301, 0, c4c::kInvalidText);
  incomplete_structured_type_bindings.emplace(incomplete_key, int_ts);

  c4c::hir::SourceSpan span{};
  span.begin.line = 22;
  span.end.line = 22;

  auto legacy_key = c4c::hir::make_pending_template_type_key(
      c4c::hir::PendingTemplateTypeKind::OwnerStruct, pending_ts,
      &pending_owner, legacy_type_bindings, c4c::hir::NttpBindings{},
      "test-incomplete-structured-state-key", span);
  auto incomplete_structured_key = c4c::hir::make_pending_template_type_key(
      c4c::hir::PendingTemplateTypeKind::OwnerStruct, pending_ts,
      &pending_owner, incomplete_structured_type_bindings,
      c4c::hir::HirTemplateNttpBindings{},
      "test-incomplete-structured-state-key", span);
  auto observation =
      c4c::hir::observe_pending_template_type_structured_identity(
          legacy_key, incomplete_structured_key);
  expect_true(!observation.structured_bindings_complete(),
              "incomplete structured pending metadata should be observable");
  expect_true(!c4c::hir::pending_template_structured_identity_can_key_state(
                  observation, legacy_type_bindings.size(), 0),
              "incomplete structured pending metadata should keep the legacy state-key path");
}

}  // namespace

int main() {
  test_same_spelling_distinct_by_owner_and_index();
  test_complete_metadata_rejects_text_id_only_key();
  test_unordered_map_lookup_uses_structured_hash();
  test_pack_element_identity_distinct_from_parameter_key();
  test_constructs_distinct_structured_keys_from_template_nodes();
  test_call_binding_parity_observes_structured_maps();
  test_legacy_name_dual_write_helpers_require_complete_owner_metadata();
  test_specialization_argument_identity_uses_structured_parameter_key();
  test_pending_template_binding_identity_helpers_accept_structured_maps();
  test_pending_template_state_can_use_structured_identity_key();
  test_pending_template_state_rejects_incomplete_structured_identity_key();
  std::cout << "PASS: cpp_hir_template_parameter_binding_key_test\n";
  return 0;
}
