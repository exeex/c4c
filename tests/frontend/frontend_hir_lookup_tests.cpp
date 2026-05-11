#include "hir/hir_ir.hpp"
#include "hir/compile_time_engine.hpp"
#include "hir/impl/hir_impl.hpp"
#include "codegen/shared/llvm_helpers.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema/consteval.hpp"
#include "sema.hpp"
#include "source_profile.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define private public
#include "hir/impl/lowerer.hpp"
#undef private

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void expect_eq_int(int actual, int expected, const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::to_string(expected) +
         "\nActual: " + std::to_string(actual));
  }
}

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

template <typename T>
auto has_legacy_tag_field(int)
    -> decltype((void)std::declval<T&>().tag, bool()) {
  return true;
}

template <typename>
bool has_legacy_tag_field(long) {
  return false;
}

bool has_typespec_legacy_tag() {
  return has_legacy_tag_field<c4c::TypeSpec>(0);
}

const c4c::Node* find_function_node_by_name(const c4c::Node* n,
                                            const char* name) {
  if (!n || !name) return nullptr;
  if (n->kind == c4c::NK_FUNCTION && n->name &&
      std::strcmp(n->name, name) == 0) {
    return n;
  }
  if (const c4c::Node* found = find_function_node_by_name(n->left, name)) return found;
  if (const c4c::Node* found = find_function_node_by_name(n->right, name)) return found;
  if (const c4c::Node* found = find_function_node_by_name(n->cond, name)) return found;
  if (const c4c::Node* found = find_function_node_by_name(n->then_, name)) return found;
  if (const c4c::Node* found = find_function_node_by_name(n->else_, name)) return found;
  if (const c4c::Node* found = find_function_node_by_name(n->body, name)) return found;
  if (const c4c::Node* found = find_function_node_by_name(n->init, name)) return found;
  if (const c4c::Node* found = find_function_node_by_name(n->update, name)) return found;
  for (int i = 0; i < n->n_children; ++i) {
    if (const c4c::Node* found = find_function_node_by_name(n->children[i], name)) {
      return found;
    }
  }
  return nullptr;
}

const c4c::Node* find_global_node_by_name(const c4c::Node* n,
                                          const char* name) {
  if (!n || !name) return nullptr;
  if (n->kind == c4c::NK_GLOBAL_VAR && n->name &&
      std::strcmp(n->name, name) == 0) {
    return n;
  }
  if (const c4c::Node* found = find_global_node_by_name(n->left, name)) return found;
  if (const c4c::Node* found = find_global_node_by_name(n->right, name)) return found;
  if (const c4c::Node* found = find_global_node_by_name(n->cond, name)) return found;
  if (const c4c::Node* found = find_global_node_by_name(n->then_, name)) return found;
  if (const c4c::Node* found = find_global_node_by_name(n->else_, name)) return found;
  if (const c4c::Node* found = find_global_node_by_name(n->body, name)) return found;
  if (const c4c::Node* found = find_global_node_by_name(n->init, name)) return found;
  if (const c4c::Node* found = find_global_node_by_name(n->update, name)) return found;
  for (int i = 0; i < n->n_children; ++i) {
    if (const c4c::Node* found = find_global_node_by_name(n->children[i], name)) {
      return found;
    }
  }
  return nullptr;
}

bool has_hit(const c4c::hir::Module& module,
             c4c::hir::ModuleDeclKind kind,
             c4c::hir::ModuleDeclLookupAuthority authority,
             const std::string& name,
             uint32_t resolved_id) {
  for (const auto& hit : module.decl_lookup_hits) {
    if (hit.kind == kind && hit.authority == authority &&
        hit.name == name && hit.resolved_id == resolved_id) {
      return true;
    }
  }
  return false;
}

bool has_mismatch(const c4c::hir::Module& module,
                  c4c::hir::ModuleDeclKind kind,
                  const std::string& name,
                  uint32_t structured_id,
                  uint32_t legacy_id) {
  for (const auto& mismatch : module.decl_lookup_parity_mismatches) {
    if (mismatch.kind == kind && mismatch.name == name &&
        mismatch.structured_id == structured_id &&
        mismatch.legacy_id == legacy_id) {
      return true;
    }
  }
  return false;
}

c4c::hir::NamespaceQualifier make_ns(c4c::TextTable& texts, std::string segment) {
  c4c::hir::NamespaceQualifier ns;
  ns.segments.push_back(segment);
  ns.segment_text_ids.push_back(texts.intern(segment));
  ns.context_id = 1;
  return ns;
}

void add_function(c4c::hir::Module& module,
                  c4c::hir::FunctionId id,
                  std::string name,
                  c4c::TextId name_text_id,
                  c4c::LinkNameId link_name_id = c4c::kInvalidLinkName,
                  c4c::hir::NamespaceQualifier ns = {},
                  c4c::TypeBase return_base = c4c::TB_VOID) {
  c4c::hir::Function fn;
  fn.id = id;
  fn.name = std::move(name);
  fn.name_text_id = name_text_id;
  fn.link_name_id = link_name_id;
  fn.ns_qual = std::move(ns);
  fn.return_type.spec.base = return_base;
  module.functions.push_back(fn);
  module.index_function_decl(module.functions.back());
}

void add_global(c4c::hir::Module& module,
                c4c::hir::GlobalId id,
                std::string name,
                c4c::TextId name_text_id,
                c4c::LinkNameId link_name_id = c4c::kInvalidLinkName,
                c4c::hir::NamespaceQualifier ns = {}) {
  c4c::hir::GlobalVar gv;
  gv.id = id;
  gv.name = std::move(name);
  gv.name_text_id = name_text_id;
  gv.link_name_id = link_name_id;
  gv.ns_qual = std::move(ns);
  module.globals.push_back(gv);
  module.index_global_decl(module.globals.back());
}

void test_module_decl_lookup_records_each_authority() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId structured_fn_text = texts.intern("structured_fn");
  const c4c::TextId incomplete_qualified_fn_text =
      texts.intern("incomplete_qualified_fn");
  const c4c::TextId link_fn_text = texts.intern("link_fn");
  const c4c::TextId structured_global_text = texts.intern("structured_global");
  const c4c::TextId incomplete_qualified_global_text =
      texts.intern("incomplete_qualified_global");
  const c4c::TextId concrete_global_text = texts.intern("concrete_global");
  const c4c::TextId link_global_text = texts.intern("link_global");

  const c4c::hir::NamespaceQualifier api_ns = make_ns(texts, "api");

  add_function(module, c4c::hir::FunctionId{0}, "api::structured_fn",
               structured_fn_text, c4c::kInvalidLinkName, api_ns);
  add_function(module, c4c::hir::FunctionId{1}, "legacy_fn",
               c4c::kInvalidText);
  add_function(module, c4c::hir::FunctionId{2}, "api::incomplete_qualified_fn",
               incomplete_qualified_fn_text, c4c::kInvalidLinkName, api_ns);
  add_function(module, c4c::hir::FunctionId{3}, "link_fn",
               link_fn_text, module.link_names.intern("link_fn"));

  add_global(module, c4c::hir::GlobalId{0}, "api::structured_global",
             structured_global_text, c4c::kInvalidLinkName, api_ns);
  add_global(module, c4c::hir::GlobalId{1}, "legacy_global",
             c4c::kInvalidText);
  add_global(module, c4c::hir::GlobalId{2}, "api::incomplete_qualified_global",
             incomplete_qualified_global_text, c4c::kInvalidLinkName, api_ns);
  add_global(module, c4c::hir::GlobalId{3}, "concrete_global",
             concrete_global_text);
  add_global(module, c4c::hir::GlobalId{4}, "link_global",
             link_global_text, module.link_names.intern("link_global"));

  c4c::hir::DeclRef structured_fn_ref;
  structured_fn_ref.name = "api::structured_fn";
  structured_fn_ref.name_text_id = structured_fn_text;
  structured_fn_ref.ns_qual = api_ns;
  const c4c::hir::Function* structured_fn =
      module.resolve_function_decl(structured_fn_ref);
  expect_true(structured_fn != nullptr && structured_fn->id.value == 0,
              "structured function decl-ref should resolve through the structured index");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "api::structured_fn", 0),
              "structured function lookup should record a structured hit");

  c4c::hir::DeclRef legacy_fn_ref;
  legacy_fn_ref.name = "legacy_fn";
  legacy_fn_ref.name_text_id = c4c::kInvalidText;
  const c4c::hir::Function* legacy_fn = module.resolve_function_decl(legacy_fn_ref);
  expect_true(legacy_fn != nullptr && legacy_fn->id.value == 1,
              "function decl-ref without declaration metadata should keep rendered fallback");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LegacyRendered,
                      "legacy_fn", 1),
              "legacy function lookup should record missing declaration metadata");

  c4c::hir::DeclRef incomplete_qualified_fn_ref;
  incomplete_qualified_fn_ref.name = "api::incomplete_qualified_fn";
  incomplete_qualified_fn_ref.name_text_id = incomplete_qualified_fn_text;
  incomplete_qualified_fn_ref.ns_qual.segments.push_back("api");
  const c4c::hir::Function* incomplete_qualified_fn =
      module.resolve_function_decl(incomplete_qualified_fn_ref);
  expect_true(incomplete_qualified_fn != nullptr &&
                  incomplete_qualified_fn->id.value == 2,
              "function decl-ref with incomplete qualifier metadata should keep rendered fallback");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LegacyRendered,
                      "api::incomplete_qualified_fn", 2),
              "legacy function lookup should record incomplete qualifier metadata");

  c4c::hir::DeclRef link_fn_ref;
  link_fn_ref.name = "link_fn";
  link_fn_ref.name_text_id = link_fn_text;
  link_fn_ref.link_name_id = module.link_names.find("link_fn");
  const c4c::hir::Function* link_fn = module.resolve_function_decl(link_fn_ref);
  expect_true(link_fn != nullptr && link_fn->id.value == 3,
              "function decl-ref with a LinkNameId should resolve through link-name authority");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LinkNameId,
                      "link_fn", 3),
              "function LinkNameId lookup should record a link-name hit");

  c4c::hir::DeclRef structured_global_ref;
  structured_global_ref.name = "api::structured_global";
  structured_global_ref.name_text_id = structured_global_text;
  structured_global_ref.ns_qual = api_ns;
  const c4c::hir::GlobalVar* structured_global =
      module.resolve_global_decl(structured_global_ref);
  expect_true(structured_global != nullptr && structured_global->id.value == 0,
              "structured global decl-ref should resolve through the structured index");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "api::structured_global", 0),
              "structured global lookup should record a structured hit");

  c4c::hir::DeclRef legacy_global_ref;
  legacy_global_ref.name = "legacy_global";
  legacy_global_ref.name_text_id = c4c::kInvalidText;
  const c4c::hir::GlobalVar* legacy_global =
      module.resolve_global_decl(legacy_global_ref);
  expect_true(legacy_global != nullptr && legacy_global->id.value == 1,
              "global decl-ref without declaration metadata should keep rendered fallback");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::LegacyRendered,
                      "legacy_global", 1),
              "legacy global lookup should record missing declaration metadata");

  c4c::hir::DeclRef incomplete_qualified_global_ref;
  incomplete_qualified_global_ref.name = "api::incomplete_qualified_global";
  incomplete_qualified_global_ref.name_text_id = incomplete_qualified_global_text;
  incomplete_qualified_global_ref.ns_qual.segments.push_back("api");
  const c4c::hir::GlobalVar* incomplete_qualified_global =
      module.resolve_global_decl(incomplete_qualified_global_ref);
  expect_true(incomplete_qualified_global != nullptr &&
                  incomplete_qualified_global->id.value == 2,
              "global decl-ref with incomplete qualifier metadata should keep rendered fallback");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::LegacyRendered,
                      "api::incomplete_qualified_global", 2),
              "legacy global lookup should record incomplete qualifier metadata");

  c4c::hir::DeclRef concrete_global_ref;
  concrete_global_ref.name = "concrete_global";
  concrete_global_ref.name_text_id = concrete_global_text;
  concrete_global_ref.global = c4c::hir::GlobalId{3};
  const c4c::hir::GlobalVar* concrete_global =
      module.resolve_global_decl(concrete_global_ref);
  expect_true(concrete_global != nullptr && concrete_global->id.value == 3,
              "global decl-ref with a concrete GlobalId should resolve by concrete id");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::ConcreteGlobalId,
                      "concrete_global", 3),
              "concrete GlobalId lookup should record a global-id hit");

  c4c::hir::DeclRef link_global_ref;
  link_global_ref.name = "link_global";
  link_global_ref.name_text_id = link_global_text;
  link_global_ref.link_name_id = module.link_names.find("link_global");
  const c4c::hir::GlobalVar* link_global =
      module.resolve_global_decl(link_global_ref);
  expect_true(link_global != nullptr && link_global->id.value == 4,
              "global decl-ref with a LinkNameId should resolve through link-name authority");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::LinkNameId,
                      "link_global", 4),
              "global LinkNameId lookup should record a link-name hit");
}

void test_stale_rendered_names_do_not_override_authoritative_decl_lookup() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId stale_fn_text = texts.intern("stale_function_name");
  const c4c::TextId structured_fn_text = texts.intern("structured_function");
  const c4c::TextId link_fn_text = texts.intern("link_function");
  const c4c::TextId stale_global_text = texts.intern("stale_global_name");
  const c4c::TextId structured_global_text = texts.intern("structured_global");
  const c4c::TextId concrete_global_text = texts.intern("concrete_global");
  const c4c::TextId link_global_text = texts.intern("link_global");

  add_function(module, c4c::hir::FunctionId{10}, "stale_function_name",
               stale_fn_text);
  add_function(module, c4c::hir::FunctionId{11}, "structured_function",
               structured_fn_text);
  add_function(module, c4c::hir::FunctionId{12}, "link_function",
               link_fn_text, module.link_names.intern("link_function"));

  add_global(module, c4c::hir::GlobalId{20}, "stale_global_name",
             stale_global_text);
  add_global(module, c4c::hir::GlobalId{21}, "structured_global",
             structured_global_text);
  add_global(module, c4c::hir::GlobalId{22}, "concrete_global",
             concrete_global_text);
  add_global(module, c4c::hir::GlobalId{23}, "link_global",
             link_global_text, module.link_names.intern("link_global"));

  c4c::hir::DeclRef structured_fn_ref;
  structured_fn_ref.name = "stale_function_name";
  structured_fn_ref.name_text_id = structured_fn_text;
  const c4c::hir::Function* structured_fn =
      module.resolve_function_decl(structured_fn_ref);
  expect_true(structured_fn != nullptr && structured_fn->id.value == 11,
              "stale function rendered name should not override structured lookup");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "stale_function_name", 11),
              "function disagreement should record structured authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "stale_function_name", 11, 10),
              "function disagreement should record legacy mismatch");

  c4c::hir::DeclRef link_fn_ref;
  link_fn_ref.name = "stale_function_name";
  link_fn_ref.name_text_id = stale_fn_text;
  link_fn_ref.link_name_id = module.link_names.find("link_function");
  const c4c::hir::Function* link_fn = module.resolve_function_decl(link_fn_ref);
  expect_true(link_fn != nullptr && link_fn->id.value == 12,
              "stale function rendered name should not override LinkNameId lookup");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LinkNameId,
                      "stale_function_name", 12),
              "function link-name disagreement should record link-name authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "stale_function_name", 12, 10),
              "function link-name disagreement should record legacy mismatch");

  c4c::hir::DeclRef structured_global_ref;
  structured_global_ref.name = "stale_global_name";
  structured_global_ref.name_text_id = structured_global_text;
  const c4c::hir::GlobalVar* structured_global =
      module.resolve_global_decl(structured_global_ref);
  expect_true(structured_global != nullptr && structured_global->id.value == 21,
              "stale global rendered name should not override structured lookup");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "stale_global_name", 21),
              "global disagreement should record structured authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Global,
                           "stale_global_name", 21, 20),
              "global disagreement should record legacy mismatch");

  c4c::hir::DeclRef concrete_global_ref;
  concrete_global_ref.name = "stale_global_name";
  concrete_global_ref.name_text_id = stale_global_text;
  concrete_global_ref.global = c4c::hir::GlobalId{22};
  const c4c::hir::GlobalVar* concrete_global =
      module.resolve_global_decl(concrete_global_ref);
  expect_true(concrete_global != nullptr && concrete_global->id.value == 22,
              "stale global rendered name should not override concrete GlobalId lookup");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::ConcreteGlobalId,
                      "stale_global_name", 22),
              "global concrete-id disagreement should record concrete-id authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Global,
                           "stale_global_name", 22, 20),
              "global concrete-id disagreement should record legacy mismatch");

  c4c::hir::DeclRef link_global_ref;
  link_global_ref.name = "stale_global_name";
  link_global_ref.name_text_id = stale_global_text;
  link_global_ref.link_name_id = module.link_names.find("link_global");
  const c4c::hir::GlobalVar* link_global =
      module.resolve_global_decl(link_global_ref);
  expect_true(link_global != nullptr && link_global->id.value == 23,
              "stale global rendered name should not override LinkNameId lookup");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::LinkNameId,
                      "stale_global_name", 23),
              "global link-name disagreement should record link-name authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Global,
                           "stale_global_name", 23, 20),
              "global link-name disagreement should record legacy mismatch");
}

void test_complete_decl_lookup_miss_rejects_stale_rendered_indexes() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId stale_fn_text = texts.intern("stale_rendered_fn");
  const c4c::TextId missing_fn_text = texts.intern("missing_structured_fn");
  const c4c::TextId stale_global_text = texts.intern("stale_rendered_global");
  const c4c::TextId missing_global_text =
      texts.intern("missing_structured_global");
  const c4c::hir::NamespaceQualifier other_ns = make_ns(texts, "other");

  add_function(module, c4c::hir::FunctionId{30}, "stale_rendered_fn",
               stale_fn_text);
  add_function(module, c4c::hir::FunctionId{31}, "other::missing_structured_fn",
               missing_fn_text, c4c::kInvalidLinkName, other_ns);
  add_global(module, c4c::hir::GlobalId{40}, "stale_rendered_global",
             stale_global_text);
  add_global(module, c4c::hir::GlobalId{41}, "other::missing_structured_global",
             missing_global_text, c4c::kInvalidLinkName, other_ns);

  c4c::hir::DeclRef fn_ref;
  fn_ref.name = "stale_rendered_fn";
  fn_ref.name_text_id = missing_fn_text;
  const c4c::hir::Function* fn = module.resolve_function_decl(fn_ref);
  expect_true(fn == nullptr,
              "complete function declaration-key miss must not use rendered fn_index");
  expect_true(!has_hit(module, c4c::hir::ModuleDeclKind::Function,
                       c4c::hir::ModuleDeclLookupAuthority::LegacyRendered,
                       "stale_rendered_fn", 30),
              "complete function declaration-key miss must not record legacy rendered authority");

  c4c::hir::DeclRef global_ref;
  global_ref.name = "stale_rendered_global";
  global_ref.name_text_id = missing_global_text;
  const c4c::hir::GlobalVar* global = module.resolve_global_decl(global_ref);
  expect_true(global == nullptr,
              "complete global declaration-key miss must not use rendered global_index");
  expect_true(!has_hit(module, c4c::hir::ModuleDeclKind::Global,
                       c4c::hir::ModuleDeclLookupAuthority::LegacyRendered,
                       "stale_rendered_global", 40),
              "complete global declaration-key miss must not record legacy rendered authority");
}

void test_direct_call_callee_lookup_uses_authoritative_decl_identity() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId stale_text = texts.intern("stale_direct_callee");
  const c4c::TextId structured_text = texts.intern("structured_direct_callee");
  const c4c::TextId linked_text = texts.intern("linked_direct_callee");
  const c4c::TextId stale_qualified_text =
      texts.intern("stale_qualified_direct_callee");
  const c4c::TextId structured_qualified_text =
      texts.intern("structured_qualified_direct_callee");
  const c4c::hir::NamespaceQualifier api_ns = make_ns(texts, "api");

  add_function(module, c4c::hir::FunctionId{30}, "stale_direct_callee",
               stale_text);
  add_function(module, c4c::hir::FunctionId{31}, "structured_direct_callee",
               structured_text);
  add_function(module, c4c::hir::FunctionId{32}, "linked_direct_callee",
               linked_text, module.link_names.intern("linked_direct_callee"));
  add_function(module, c4c::hir::FunctionId{33},
               "api::stale_qualified_direct_callee", stale_qualified_text,
               c4c::kInvalidLinkName, api_ns);
  add_function(module, c4c::hir::FunctionId{34},
               "api::structured_qualified_direct_callee",
               structured_qualified_text, c4c::kInvalidLinkName, api_ns);

  c4c::hir::DeclRef structured_ref;
  structured_ref.name = "stale_direct_callee";
  structured_ref.name_text_id = structured_text;
  const c4c::hir::Function* structured_fn =
      module.resolve_direct_call_callee(structured_ref);
  expect_true(structured_fn != nullptr && structured_fn->id.value == 31,
              "direct-call callee lookup should prefer structured identity over stale rendered name");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "stale_direct_callee", 31),
              "direct-call structured callee lookup should record structured authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "stale_direct_callee", 31, 30),
              "direct-call structured callee lookup should record stale-rendered mismatch");

  c4c::hir::DeclRef link_ref;
  link_ref.name = "stale_direct_callee";
  link_ref.name_text_id = stale_text;
  link_ref.link_name_id = module.link_names.find("linked_direct_callee");
  const c4c::hir::Function* link_fn =
      module.resolve_direct_call_callee(link_ref);
  expect_true(link_fn != nullptr && link_fn->id.value == 32,
              "direct-call callee lookup should prefer LinkNameId over stale rendered name");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LinkNameId,
                      "stale_direct_callee", 32),
              "direct-call link-name callee lookup should record link-name authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "stale_direct_callee", 32, 30),
              "direct-call link-name callee lookup should record stale-rendered mismatch");

  c4c::hir::DeclRef qualified_ref;
  qualified_ref.name = "api::stale_qualified_direct_callee";
  qualified_ref.name_text_id = structured_qualified_text;
  qualified_ref.ns_qual = api_ns;
  const c4c::hir::Function* qualified_fn =
      module.resolve_direct_call_callee(qualified_ref);
  expect_true(qualified_fn != nullptr && qualified_fn->id.value == 34,
              "qualified direct-call callee lookup should prefer structured identity over stale rendered name");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "api::stale_qualified_direct_callee", 34),
              "qualified direct-call structured lookup should record structured authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "api::stale_qualified_direct_callee", 34, 33),
              "qualified direct-call structured lookup should record stale-rendered mismatch");
}

void test_operator_callee_lookup_uses_authoritative_decl_identity() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId stale_text = texts.intern("stale_operator_callee");
  const c4c::TextId structured_text = texts.intern("structured_operator_callee");
  const c4c::TextId linked_text = texts.intern("linked_operator_callee");

  add_function(module, c4c::hir::FunctionId{40}, "stale_operator_callee",
               stale_text, c4c::kInvalidLinkName, {}, c4c::TB_INT);
  add_function(module, c4c::hir::FunctionId{41}, "structured_operator_callee",
               structured_text, c4c::kInvalidLinkName, {}, c4c::TB_LONG);
  add_function(module, c4c::hir::FunctionId{42}, "linked_operator_callee",
               linked_text, module.link_names.intern("linked_operator_callee"),
               {}, c4c::TB_BOOL);

  c4c::hir::DeclRef structured_ref;
  structured_ref.name = "stale_operator_callee";
  structured_ref.name_text_id = structured_text;
  const c4c::hir::Function* structured_fn =
      module.resolve_operator_callee(structured_ref);
  expect_true(structured_fn != nullptr && structured_fn->id.value == 41,
              "operator callee lookup should prefer structured identity over stale rendered name");
  expect_true(structured_fn != nullptr &&
                  structured_fn->return_type.spec.base == c4c::TB_LONG,
              "operator callee return metadata should come from structured authority");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "stale_operator_callee", 41),
              "operator structured callee lookup should record structured authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "stale_operator_callee", 41, 40),
              "operator structured callee lookup should record stale-rendered mismatch");

  c4c::hir::DeclRef link_ref;
  link_ref.name = "stale_operator_callee";
  link_ref.name_text_id = stale_text;
  link_ref.link_name_id = module.link_names.find("linked_operator_callee");
  const c4c::hir::Function* link_fn =
      module.resolve_operator_callee(link_ref);
  expect_true(link_fn != nullptr && link_fn->id.value == 42,
              "operator callee lookup should prefer LinkNameId over stale rendered name");
  expect_true(link_fn != nullptr &&
                  link_fn->return_type.spec.base == c4c::TB_BOOL,
              "operator callee return metadata should come from LinkNameId authority");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LinkNameId,
                      "stale_operator_callee", 42),
              "operator link-name callee lookup should record link-name authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "stale_operator_callee", 42, 40),
              "operator link-name callee lookup should record stale-rendered mismatch");

  const c4c::TextId legacy_text = texts.intern("legacy_operator_callee");
  add_function(module, c4c::hir::FunctionId{43}, "legacy_operator_callee",
               c4c::kInvalidText, c4c::kInvalidLinkName, {}, c4c::TB_LONGLONG);

  c4c::hir::DeclRef legacy_ref;
  legacy_ref.name = "legacy_operator_callee";
  legacy_ref.name_text_id = legacy_text;
  const c4c::hir::Function* legacy_fn =
      module.resolve_operator_callee(legacy_ref);
  expect_true(legacy_fn != nullptr && legacy_fn->id.value == 43,
              "operator callee lookup should preserve rendered-name fallback");
  expect_true(legacy_fn != nullptr &&
                  legacy_fn->return_type.spec.base == c4c::TB_LONGLONG,
              "operator callee return metadata should preserve rendered-name fallback");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LegacyRendered,
                      "legacy_operator_callee", 43),
              "operator legacy fallback should record rendered-name authority");
}

void test_range_for_method_callee_lookup_uses_authoritative_decl_identity() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId stale_text = texts.intern("stale_range_for_method");
  const c4c::TextId linked_text = texts.intern("linked_range_for_method");

  add_function(module, c4c::hir::FunctionId{50}, "stale_range_for_method",
               stale_text, c4c::kInvalidLinkName, {}, c4c::TB_INT);
  add_function(module, c4c::hir::FunctionId{51}, "linked_range_for_method",
               linked_text, module.link_names.intern("linked_range_for_method"),
               {}, c4c::TB_BOOL);

  c4c::hir::DeclRef link_ref;
  link_ref.name = "stale_range_for_method";
  link_ref.name_text_id = stale_text;
  link_ref.link_name_id = module.link_names.find("linked_range_for_method");
  const c4c::hir::Function* link_fn =
      module.resolve_range_for_method_callee(link_ref);
  expect_true(link_fn != nullptr && link_fn->id.value == 51,
              "range-for method lookup should prefer LinkNameId over stale rendered name");
  expect_true(link_fn != nullptr &&
                  link_fn->return_type.spec.base == c4c::TB_BOOL,
              "range-for method return metadata should come from LinkNameId authority");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LinkNameId,
                      "stale_range_for_method", 51),
              "range-for method link-name lookup should record link-name authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "stale_range_for_method", 51, 50),
              "range-for method link-name lookup should record stale-rendered mismatch");

  const c4c::TextId legacy_text = texts.intern("legacy_range_for_method");
  add_function(module, c4c::hir::FunctionId{52}, "legacy_range_for_method",
               c4c::kInvalidText, c4c::kInvalidLinkName, {}, c4c::TB_LONGLONG);

  c4c::hir::DeclRef legacy_ref;
  legacy_ref.name = "legacy_range_for_method";
  legacy_ref.name_text_id = legacy_text;
  const c4c::hir::Function* legacy_fn =
      module.resolve_range_for_method_callee(legacy_ref);
  expect_true(legacy_fn != nullptr && legacy_fn->id.value == 52,
              "range-for method lookup should preserve rendered-name fallback");
  expect_true(legacy_fn != nullptr &&
                  legacy_fn->return_type.spec.base == c4c::TB_LONGLONG,
              "range-for method return metadata should preserve rendered-name fallback");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::LegacyRendered,
                      "legacy_range_for_method", 52),
              "range-for method legacy fallback should record rendered-name authority");
}

void test_struct_owner_key_lookup_detects_stale_rendered_member_and_method_maps() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId owner_text = texts.intern("Owner");
  const c4c::TextId member_text = texts.intern("value");
  const c4c::TextId method_text = texts.intern("method");

  c4c::hir::NamespaceQualifier ns;
  ns.context_id = 3;
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(ns, owner_text);
  module.index_struct_def_owner(owner_key, "RenderedOwner", true);

  c4c::hir::HirStructMemberLookupKey member_key;
  member_key.owner_key = owner_key;
  member_key.member_text_id = member_text;

  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = owner_key;
  method_key.method_text_id = method_text;
  method_key.is_const_method = false;

  c4c::Node stale_member{};
  stale_member.kind = c4c::NK_DECL;
  stale_member.name = "value";
  stale_member.unqualified_name = "value";
  c4c::Node structured_member{};
  structured_member.kind = c4c::NK_DECL;
  structured_member.name = "value";
  structured_member.unqualified_name = "value";

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  const c4c::LinkNameId stale_method_link_name =
      module.link_names.intern("stale_method_mangled");
  const c4c::LinkNameId structured_method_link_name =
      module.link_names.intern("structured_method_mangled");
  c4c::TypeSpec stale_return_type{};
  stale_return_type.base = c4c::TB_INT;
  c4c::TypeSpec structured_return_type{};
  structured_return_type.base = c4c::TB_LONG;
  lowerer.struct_static_member_decls_["RenderedOwner"]["value"] = &stale_member;
  lowerer.struct_static_member_decls_by_owner_[member_key] = &structured_member;
  lowerer.struct_methods_["RenderedOwner::method"] = "stale_method_mangled";
  lowerer.struct_methods_by_owner_[method_key] = "structured_method_mangled";
  lowerer.struct_method_link_name_ids_["RenderedOwner::method"] =
      stale_method_link_name;
  lowerer.struct_method_link_name_ids_by_owner_[method_key] =
      structured_method_link_name;
  lowerer.struct_method_ret_types_["RenderedOwner::method"] = stale_return_type;
  lowerer.struct_method_ret_types_by_owner_[method_key] = structured_return_type;

  const c4c::Node* rendered_member =
      lowerer.find_struct_static_member_decl("RenderedOwner", "value");
  expect_true(rendered_member == &structured_member,
              "static-member lookup should prefer owner-key authority over stale rendered maps");
  expect_true(lowerer.struct_static_member_decl_lookup_parity_checks_ == 1,
              "static-member owner-key lookup should run a parity check");
  expect_true(lowerer.struct_static_member_decl_lookup_parity_mismatches_ == 1,
              "static-member owner-key lookup should detect stale rendered authority");

  const std::optional<std::string> rendered_method =
      lowerer.find_struct_method_mangled("RenderedOwner", "method", false);
  expect_true(rendered_method && *rendered_method == "structured_method_mangled",
              "method lookup should prefer owner-key authority over stale rendered maps");
  expect_true(lowerer.struct_method_mangled_lookup_parity_checks_ == 1,
              "method owner-key lookup should run a parity check");
  expect_true(lowerer.struct_method_mangled_lookup_parity_mismatches_ == 1,
              "method owner-key lookup should detect stale rendered authority");

  const std::optional<c4c::LinkNameId> rendered_method_link_name =
      lowerer.find_struct_method_link_name_id("RenderedOwner", "method", false);
  expect_true(rendered_method_link_name &&
                  *rendered_method_link_name == structured_method_link_name,
              "method link-name lookup should prefer owner-key authority over stale rendered maps");
  expect_true(lowerer.struct_method_link_name_lookup_parity_checks_ == 1,
              "method link-name owner-key lookup should run a parity check");
  expect_true(lowerer.struct_method_link_name_lookup_parity_mismatches_ == 1,
              "method link-name owner-key lookup should detect stale rendered authority");

  const std::optional<c4c::TypeSpec> rendered_method_return_type =
      lowerer.find_struct_method_return_type("RenderedOwner", "method", false);
  expect_true(rendered_method_return_type &&
                  rendered_method_return_type->base == c4c::TB_LONG,
              "method return-type lookup should prefer owner-key authority over stale rendered maps");
  expect_true(lowerer.struct_method_return_type_lookup_parity_checks_ == 1,
              "method return-type owner-key lookup should run a parity check");
  expect_true(lowerer.struct_method_return_type_lookup_parity_mismatches_ == 1,
              "method return-type owner-key lookup should detect stale rendered authority");
}

c4c::Node make_compile_time_registry_node(c4c::NodeKind kind,
                                           const char* name,
                                           c4c::TextId unqualified_text_id) {
  c4c::Node node{};
  node.kind = kind;
  node.name = name;
  node.unqualified_name = name;
  node.unqualified_text_id = unqualified_text_id;
  return node;
}

void test_compile_time_state_structured_registry_lookup_wins_over_stale_rendered_names() {
  c4c::TextTable texts;
  c4c::hir::CompileTimeState state;

  c4c::Node stale_template = make_compile_time_registry_node(
      c4c::NK_FUNCTION, "stale_template", texts.intern("stale_template"));
  stale_template.n_template_params = 1;
  c4c::Node structured_template = make_compile_time_registry_node(
      c4c::NK_FUNCTION, "structured_template",
      texts.intern("structured_template"));
  structured_template.n_template_params = 1;
  state.register_template_def("stale_template", &stale_template);
  state.register_template_def("structured_template", &structured_template);
  expect_true(state.find_template_def(&structured_template, "stale_template") ==
                  &structured_template,
              "template definition lookup should prefer declaration-key mirror over stale rendered name");
  c4c::Node unregistered_template = make_compile_time_registry_node(
      c4c::NK_FUNCTION, "unregistered_template",
      texts.intern("unregistered_template"));
  expect_true(state.find_template_def(&unregistered_template, "stale_template") ==
                  nullptr,
              "template definition lookup should fail closed after a complete declaration-key miss");
  expect_true(state.find_template_def(nullptr, "stale_template") ==
                  &stale_template,
              "template definition lookup should preserve rendered fallback for explicit no-metadata calls");

  c4c::Node stale_struct = make_compile_time_registry_node(
      c4c::NK_STRUCT_DEF, "StaleTemplateStruct",
      texts.intern("StaleTemplateStruct"));
  stale_struct.n_template_params = 1;
  c4c::Node structured_struct = make_compile_time_registry_node(
      c4c::NK_STRUCT_DEF, "StructuredTemplateStruct",
      texts.intern("StructuredTemplateStruct"));
  structured_struct.n_template_params = 1;
  state.register_template_struct_def("StaleTemplateStruct", &stale_struct);
  state.register_template_struct_def("StructuredTemplateStruct",
                                     &structured_struct);
  expect_true(state.find_template_struct_def(&structured_struct,
                                             "StaleTemplateStruct") ==
                  &structured_struct,
              "template struct definition lookup should prefer declaration-key mirror over stale rendered name");
  c4c::Node unregistered_struct = make_compile_time_registry_node(
      c4c::NK_STRUCT_DEF, "UnregisteredTemplateStruct",
      texts.intern("UnregisteredTemplateStruct"));
  unregistered_struct.n_template_params = 1;
  expect_true(state.find_template_struct_def(&unregistered_struct,
                                             "StaleTemplateStruct") ==
                  nullptr,
              "template struct definition lookup should fail closed after a complete declaration-key miss");
  expect_true(state.find_template_struct_def(nullptr,
                                             "StaleTemplateStruct") ==
                  &stale_struct,
              "template struct definition lookup should preserve rendered fallback for explicit no-metadata calls");

  c4c::Node stale_struct_spec = make_compile_time_registry_node(
      c4c::NK_STRUCT_DEF, "StaleTemplateStruct<int>",
      texts.intern("StaleTemplateStruct"));
  c4c::Node structured_struct_spec = make_compile_time_registry_node(
      c4c::NK_STRUCT_DEF, "StructuredTemplateStruct<int>",
      texts.intern("StructuredTemplateStruct"));
  state.register_template_struct_specialization_no_metadata_compat(
      "StaleTemplateStruct", &stale_struct_spec);
  state.register_template_struct_specialization(&structured_struct,
                                                &structured_struct_spec);
  const std::vector<const c4c::Node*>* structured_specs =
      state.find_template_struct_specializations(&structured_struct,
                                                 "StaleTemplateStruct");
  expect_true(structured_specs && structured_specs->size() == 1 &&
                  (*structured_specs)[0] == &structured_struct_spec,
              "template struct specialization lookup should prefer owner-key mirror over stale rendered primary name");
  const std::vector<const c4c::Node*>* fallback_specs =
      state.find_template_struct_specializations(&unregistered_struct,
                                                 "StaleTemplateStruct");
  expect_true(fallback_specs == nullptr,
              "template struct specialization lookup should fail closed after a complete owner-key miss");
  fallback_specs =
      state.find_template_struct_specializations_no_metadata_compat(
          "StaleTemplateStruct");
  expect_true(fallback_specs && fallback_specs->size() == 1 &&
                  (*fallback_specs)[0] == &stale_struct_spec,
              "template struct specialization lookup should preserve rendered fallback for explicit no-metadata calls");

  c4c::Node stale_consteval = make_compile_time_registry_node(
      c4c::NK_FUNCTION, "stale_consteval", texts.intern("stale_consteval"));
  stale_consteval.is_consteval = true;
  c4c::Node structured_consteval = make_compile_time_registry_node(
      c4c::NK_FUNCTION, "structured_consteval",
      texts.intern("structured_consteval"));
  structured_consteval.is_consteval = true;
  state.register_consteval_def("stale_consteval", &stale_consteval);
  state.register_consteval_def("structured_consteval", &structured_consteval);
  expect_true(state.find_consteval_def(&structured_consteval,
                                       "stale_consteval") ==
                  &structured_consteval,
              "consteval definition lookup should prefer declaration-key mirror over stale rendered name");
  c4c::Node unregistered_consteval = make_compile_time_registry_node(
      c4c::NK_FUNCTION, "unregistered_consteval",
      texts.intern("unregistered_consteval"));
  unregistered_consteval.is_consteval = true;
  expect_true(state.find_consteval_def(&unregistered_consteval,
                                       "stale_consteval") == nullptr,
              "consteval definition lookup should fail closed after a complete declaration-key miss");
  expect_true(state.find_consteval_def(nullptr, "stale_consteval") ==
                  &stale_consteval,
              "consteval definition lookup should preserve rendered fallback for explicit no-metadata calls");
}

void test_template_struct_primary_lookup_uses_record_def_before_stale_origin() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node stale_primary{};
  stale_primary.kind = c4c::NK_STRUCT_DEF;
  stale_primary.name = "StalePrimaryForCanonical";
  stale_primary.unqualified_name = "StalePrimaryForCanonical";
  stale_primary.n_template_params = 1;

  c4c::Node structured_primary{};
  structured_primary.kind = c4c::NK_STRUCT_DEF;
  structured_primary.name = "StructuredPrimaryForCanonical";
  structured_primary.unqualified_name = "StructuredPrimaryForCanonical";
  structured_primary.n_template_params = 1;

  lowerer.register_template_struct_primary("StalePrimaryForCanonical",
                                           &stale_primary);
  lowerer.register_template_struct_primary("StructuredPrimaryForCanonical",
                                           &structured_primary);

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "StalePrimaryForCanonical";
  ts.record_def = &structured_primary;

  expect_true(lowerer.canonical_template_struct_primary(ts) ==
                  &structured_primary,
              "template struct primary lookup should use record_def owner identity before stale rendered origin");
}

void test_template_struct_primary_record_def_miss_rejects_stale_origin() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node stale_primary{};
  stale_primary.kind = c4c::NK_STRUCT_DEF;
  stale_primary.name = "StalePrimaryAfterRecordMiss";
  stale_primary.unqualified_name = "StalePrimaryAfterRecordMiss";
  stale_primary.n_template_params = 1;
  lowerer.register_template_struct_primary("StalePrimaryAfterRecordMiss",
                                           &stale_primary);

  c4c::Node missing_primary{};
  missing_primary.kind = c4c::NK_STRUCT_DEF;
  missing_primary.name = "MissingStructuredPrimary";
  missing_primary.unqualified_name = "MissingStructuredPrimary";
  missing_primary.n_template_params = 1;

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "StalePrimaryAfterRecordMiss";
  ts.record_def = &missing_primary;

  expect_true(lowerer.canonical_template_struct_primary(ts) == nullptr,
              "template struct primary lookup must not use stale rendered origin after structured record_def miss");
}

void test_template_struct_primary_lookup_uses_origin_key_before_stale_tag() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("StaleOriginKeyPrimary");
  const c4c::TextId stale_inst_text = texts.intern("StaleOriginKeyPrimary_T0");
  const c4c::TextId structured_text = texts.intern("StructuredOriginKeyPrimary");

  c4c::Node stale_primary{};
  stale_primary.kind = c4c::NK_STRUCT_DEF;
  stale_primary.name = "StaleOriginKeyPrimary";
  stale_primary.unqualified_name = "StaleOriginKeyPrimary";
  stale_primary.unqualified_text_id = stale_text;
  stale_primary.namespace_context_id = 0;
  stale_primary.n_template_params = 1;

  c4c::Node structured_primary{};
  structured_primary.kind = c4c::NK_STRUCT_DEF;
  structured_primary.name = "StructuredOriginKeyPrimary";
  structured_primary.unqualified_name = "StructuredOriginKeyPrimary";
  structured_primary.unqualified_text_id = structured_text;
  structured_primary.namespace_context_id = 0;
  structured_primary.n_template_params = 1;

  lowerer.register_template_struct_primary("StaleOriginKeyPrimary",
                                           &stale_primary);
  lowerer.register_template_struct_primary("StructuredOriginKeyPrimary",
                                           &structured_primary);

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "StaleOriginKeyPrimary_T0";
  ts.tpl_struct_origin_key.context_id = 0;
  ts.tpl_struct_origin_key.base_text_id = structured_text;
  set_legacy_tag_if_present(ts, "StaleOriginKeyPrimary_T0", 0);
  ts.tag_text_id = stale_inst_text;

  expect_true(lowerer.canonical_template_struct_primary(ts) ==
                  &structured_primary,
              "template struct primary lookup should use structured origin key before stale rendered tag family fallback");
}

void test_inherited_static_member_eval_uses_base_record_def_before_stale_tag() {
  c4c::Node real_value{};
  real_value.kind = c4c::NK_INT_LIT;
  real_value.ival = 23;
  c4c::Node real_member{};
  real_member.kind = c4c::NK_DECL;
  real_member.is_static = true;
  real_member.name = "value";
  real_member.init = &real_value;
  c4c::Node* real_fields[] = {&real_member};
  c4c::Node real_base{};
  real_base.kind = c4c::NK_STRUCT_DEF;
  real_base.name = "RealInheritedStaticBase";
  real_base.fields = real_fields;
  real_base.n_fields = 1;

  c4c::Node stale_value{};
  stale_value.kind = c4c::NK_INT_LIT;
  stale_value.ival = 7;
  c4c::Node stale_member{};
  stale_member.kind = c4c::NK_DECL;
  stale_member.is_static = true;
  stale_member.name = "value";
  stale_member.init = &stale_value;
  c4c::Node* stale_fields[] = {&stale_member};
  c4c::Node stale_base{};
  stale_base.kind = c4c::NK_STRUCT_DEF;
  stale_base.name = "StaleInheritedStaticBase";
  stale_base.fields = stale_fields;
  stale_base.n_fields = 1;

  c4c::TypeSpec base_ts{};
  base_ts.array_size = -1;
  base_ts.inner_rank = -1;
  base_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(base_ts, "StaleInheritedStaticBase", 0);
  base_ts.record_def = &real_base;
  c4c::TypeSpec bases[] = {base_ts};
  c4c::Node derived{};
  derived.kind = c4c::NK_STRUCT_DEF;
  derived.name = "DerivedInheritedStatic";
  derived.base_types = bases;
  derived.n_bases = 1;

  std::unordered_map<std::string, const c4c::Node*> struct_defs;
  struct_defs["StaleInheritedStaticBase"] = &stale_base;
  long long value = 0;
  const bool resolved = c4c::hir::eval_struct_static_member_value_hir(
      &derived, struct_defs, "value", nullptr, &value);

  expect_true(resolved && value == 23,
              "inherited static member evaluation should use base record_def before stale rendered tag");
}

void test_inherited_static_member_eval_base_record_def_miss_rejects_stale_tag() {
  c4c::Node missing_base{};
  missing_base.kind = c4c::NK_STRUCT_DEF;
  missing_base.name = "MissingInheritedStaticBase";

  c4c::Node stale_value{};
  stale_value.kind = c4c::NK_INT_LIT;
  stale_value.ival = 7;
  c4c::Node stale_member{};
  stale_member.kind = c4c::NK_DECL;
  stale_member.is_static = true;
  stale_member.name = "value";
  stale_member.init = &stale_value;
  c4c::Node* stale_fields[] = {&stale_member};
  c4c::Node stale_base{};
  stale_base.kind = c4c::NK_STRUCT_DEF;
  stale_base.name = "StaleMissInheritedStaticBase";
  stale_base.fields = stale_fields;
  stale_base.n_fields = 1;

  c4c::TypeSpec base_ts{};
  base_ts.array_size = -1;
  base_ts.inner_rank = -1;
  base_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(base_ts, "StaleMissInheritedStaticBase", 0);
  base_ts.record_def = &missing_base;
  c4c::TypeSpec bases[] = {base_ts};
  c4c::Node derived{};
  derived.kind = c4c::NK_STRUCT_DEF;
  derived.name = "DerivedInheritedStaticMiss";
  derived.base_types = bases;
  derived.n_bases = 1;

  std::unordered_map<std::string, const c4c::Node*> struct_defs;
  struct_defs["StaleMissInheritedStaticBase"] = &stale_base;
  long long value = 0;
  const bool resolved = c4c::hir::eval_struct_static_member_value_hir(
      &derived, struct_defs, "value", nullptr, &value);

  expect_true(!resolved,
              "inherited static member evaluation must not use stale rendered base tag after record_def miss");
}

void test_compile_time_function_specialization_type_arg_uses_record_def_identity() {
  c4c::hir::InstantiationRegistry registry;

  const char* template_params[] = {"T"};
  c4c::Node primary{};
  primary.kind = c4c::NK_FUNCTION;
  primary.name = "pick";
  primary.template_param_names = template_params;
  primary.n_template_params = 1;

  c4c::Node shared_record{};
  shared_record.kind = c4c::NK_STRUCT_DEF;
  shared_record.name = "StructuredArg";

  c4c::TypeSpec spec_arg{};
  spec_arg.array_size = -1;
  spec_arg.inner_rank = -1;
  spec_arg.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(spec_arg, "StaleRenderedSpecArg", 0);
  spec_arg.record_def = &shared_record;

  c4c::TypeSpec binding_arg = spec_arg;
  set_legacy_tag_if_present(binding_arg, "StaleRenderedBindingArg", 0);

  c4c::Node specialization{};
  specialization.kind = c4c::NK_FUNCTION;
  specialization.name = "pick<StructuredArg>";
  specialization.template_arg_types = &spec_arg;
  specialization.n_template_args = 1;
  registry.register_function_specialization(&primary, &specialization);

  c4c::hir::TypeBindings bindings;
  bindings["T"] = binding_arg;
  const c4c::hir::SelectedFunctionTemplatePattern selected =
      registry.select_function_specialization(&primary, bindings, {},
                                              c4c::hir::SpecializationKey{});

  expect_true(selected.selected_pattern == &specialization,
              "function specialization TypeSpec arg matching should use shared record_def before stale rendered tags");
}

void test_compile_time_function_specialization_type_arg_record_def_mismatch_rejects_tag() {
  c4c::hir::InstantiationRegistry registry;

  const char* template_params[] = {"T"};
  c4c::Node primary{};
  primary.kind = c4c::NK_FUNCTION;
  primary.name = "reject";
  primary.template_param_names = template_params;
  primary.n_template_params = 1;

  c4c::Node spec_record{};
  spec_record.kind = c4c::NK_STRUCT_DEF;
  spec_record.name = "SpecRecord";
  c4c::Node binding_record{};
  binding_record.kind = c4c::NK_STRUCT_DEF;
  binding_record.name = "BindingRecord";

  c4c::TypeSpec spec_arg{};
  spec_arg.array_size = -1;
  spec_arg.inner_rank = -1;
  spec_arg.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(spec_arg, "SharedRenderedArg", 0);
  spec_arg.record_def = &spec_record;

  c4c::TypeSpec binding_arg = spec_arg;
  binding_arg.record_def = &binding_record;

  c4c::Node specialization{};
  specialization.kind = c4c::NK_FUNCTION;
  specialization.name = "reject<SharedRenderedArg>";
  specialization.template_arg_types = &spec_arg;
  specialization.n_template_args = 1;
  registry.register_function_specialization(&primary, &specialization);

  c4c::hir::TypeBindings bindings;
  bindings["T"] = binding_arg;
  const c4c::hir::SelectedFunctionTemplatePattern selected =
      registry.select_function_specialization(&primary, bindings, {},
                                              c4c::hir::SpecializationKey{});

  expect_true(selected.selected_pattern == &primary,
              "function specialization TypeSpec arg matching must reject mismatched record_def even when rendered tags match");
}

void test_compile_time_function_specialization_tag_only_nominal_args_do_not_match() {
  if (!has_typespec_legacy_tag()) return;

  c4c::hir::InstantiationRegistry registry;

  const char* template_params[] = {"T"};
  c4c::Node primary{};
  primary.kind = c4c::NK_FUNCTION;
  primary.name = "tag_only";
  primary.template_param_names = template_params;
  primary.n_template_params = 1;

  c4c::TypeSpec spec_arg{};
  spec_arg.array_size = -1;
  spec_arg.inner_rank = -1;
  spec_arg.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(spec_arg, "RenderedOnlyArg", 0);

  c4c::TypeSpec binding_arg = spec_arg;

  c4c::Node specialization{};
  specialization.kind = c4c::NK_FUNCTION;
  specialization.name = "tag_only<RenderedOnlyArg>";
  specialization.template_arg_types = &spec_arg;
  specialization.n_template_args = 1;
  registry.register_function_specialization(&primary, &specialization);

  c4c::hir::TypeBindings bindings;
  bindings["T"] = binding_arg;
  const c4c::hir::SelectedFunctionTemplatePattern selected =
      registry.select_function_specialization(&primary, bindings, {},
                                              c4c::hir::SpecializationKey{});

  expect_true(selected.selected_pattern == &primary,
              "tag-only nominal specialization args should not match without structured metadata");
}

void test_compile_time_function_specialization_primitive_args_still_match() {
  c4c::hir::InstantiationRegistry registry;

  const char* template_params[] = {"T"};
  c4c::Node primary{};
  primary.kind = c4c::NK_FUNCTION;
  primary.name = "primitive";
  primary.template_param_names = template_params;
  primary.n_template_params = 1;

  c4c::TypeSpec spec_arg{};
  spec_arg.array_size = -1;
  spec_arg.inner_rank = -1;
  spec_arg.base = c4c::TB_INT;

  c4c::TypeSpec binding_arg = spec_arg;

  c4c::Node specialization{};
  specialization.kind = c4c::NK_FUNCTION;
  specialization.name = "primitive<int>";
  specialization.template_arg_types = &spec_arg;
  specialization.n_template_args = 1;
  registry.register_function_specialization(&primary, &specialization);

  c4c::hir::TypeBindings bindings;
  bindings["T"] = binding_arg;
  const c4c::hir::SelectedFunctionTemplatePattern selected =
      registry.select_function_specialization(&primary, bindings, {},
                                              c4c::hir::SpecializationKey{});

  expect_true(selected.selected_pattern == &specialization,
              "primitive specialization args should continue matching by base shape");
}

void test_template_deduction_forwarding_consistency_uses_record_def_identity() {
  c4c::hir::Lowerer lowerer;
  c4c::TextTable texts;

  const char* template_params[] = {"T"};
  c4c::TextId template_param_text_ids[] = {texts.intern("T")};
  c4c::TypeSpec param_ts{};
  param_ts.array_size = -1;
  param_ts.inner_rank = -1;
  param_ts.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(param_ts, "T", 0);
  param_ts.tag_text_id = template_param_text_ids[0];
  param_ts.template_param_text_id = template_param_text_ids[0];
  param_ts.template_param_index = 0;
  param_ts.is_rvalue_ref = true;

  c4c::Node param_a{};
  param_a.kind = c4c::NK_VAR;
  param_a.type = param_ts;
  c4c::Node param_b = param_a;
  c4c::Node* params[] = {&param_a, &param_b};
  c4c::Node fn_def{};
  fn_def.kind = c4c::NK_FUNCTION;
  fn_def.name = "deduce_forward";
  fn_def.template_param_names = template_params;
  fn_def.template_param_name_text_ids = template_param_text_ids;
  fn_def.n_template_params = 1;
  fn_def.params = params;
  fn_def.n_params = 2;
  fn_def.unqualified_text_id = texts.intern("deduce_forward");
  param_ts.template_param_owner_text_id = fn_def.unqualified_text_id;
  param_a.type = param_ts;
  param_b.type = param_ts;

  c4c::Node shared_record{};
  shared_record.kind = c4c::NK_STRUCT_DEF;
  shared_record.name = "StructuredForwardArg";
  shared_record.unqualified_text_id = texts.intern("StructuredForwardArg");

  c4c::TypeSpec arg_a_ts{};
  arg_a_ts.array_size = -1;
  arg_a_ts.inner_rank = -1;
  arg_a_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(arg_a_ts, "StaleRenderedForwardA", 0);
  arg_a_ts.tag_text_id = shared_record.unqualified_text_id;
  arg_a_ts.record_def = &shared_record;
  c4c::TypeSpec arg_b_ts = arg_a_ts;
  set_legacy_tag_if_present(arg_b_ts, "StaleRenderedForwardB", 0);

  c4c::Node arg_a{};
  arg_a.kind = c4c::NK_CAST;
  arg_a.type = arg_a_ts;
  c4c::Node arg_b{};
  arg_b.kind = c4c::NK_CAST;
  arg_b.type = arg_b_ts;
  c4c::Node* args[] = {&arg_a, &arg_b};
  c4c::Node call{};
  call.kind = c4c::NK_CALL;
  call.children = args;
  call.n_children = 2;

  c4c::hir::TypeBindings deduced =
      lowerer.try_deduce_template_type_args(&call, &fn_def, nullptr);

  auto it = deduced.find("T");
  expect_true(it != deduced.end() && it->second.record_def == &shared_record,
              "forwarding repeated deduction should use shared record_def before stale rendered tags");
}

void test_template_deduction_repeated_type_param_record_def_mismatch_rejects_tag() {
  c4c::hir::Lowerer lowerer;
  c4c::TextTable texts;

  const char* template_params[] = {"T"};
  c4c::TextId template_param_text_ids[] = {texts.intern("T")};
  c4c::TypeSpec param_ts{};
  param_ts.array_size = -1;
  param_ts.inner_rank = -1;
  param_ts.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(param_ts, "T", 0);
  param_ts.tag_text_id = template_param_text_ids[0];
  param_ts.template_param_text_id = template_param_text_ids[0];
  param_ts.template_param_index = 0;

  c4c::Node param_a{};
  param_a.kind = c4c::NK_VAR;
  param_a.type = param_ts;
  c4c::Node param_b = param_a;
  c4c::Node* params[] = {&param_a, &param_b};
  c4c::Node fn_def{};
  fn_def.kind = c4c::NK_FUNCTION;
  fn_def.name = "deduce_reject";
  fn_def.template_param_names = template_params;
  fn_def.template_param_name_text_ids = template_param_text_ids;
  fn_def.n_template_params = 1;
  fn_def.params = params;
  fn_def.n_params = 2;
  fn_def.unqualified_text_id = texts.intern("deduce_reject");
  param_ts.template_param_owner_text_id = fn_def.unqualified_text_id;
  param_a.type = param_ts;
  param_b.type = param_ts;

  c4c::Node record_a{};
  record_a.kind = c4c::NK_STRUCT_DEF;
  record_a.name = "RecordA";
  record_a.unqualified_text_id = texts.intern("RecordA");
  c4c::Node record_b{};
  record_b.kind = c4c::NK_STRUCT_DEF;
  record_b.name = "RecordB";
  record_b.unqualified_text_id = texts.intern("RecordB");

  c4c::TypeSpec arg_a_ts{};
  arg_a_ts.array_size = -1;
  arg_a_ts.inner_rank = -1;
  arg_a_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(arg_a_ts, "SharedRenderedDeductionArg", 0);
  arg_a_ts.tag_text_id = record_a.unqualified_text_id;
  arg_a_ts.record_def = &record_a;
  c4c::TypeSpec arg_b_ts = arg_a_ts;
  arg_b_ts.record_def = &record_b;

  c4c::Node arg_a{};
  arg_a.kind = c4c::NK_CAST;
  arg_a.type = arg_a_ts;
  c4c::Node arg_b{};
  arg_b.kind = c4c::NK_CAST;
  arg_b.type = arg_b_ts;
  c4c::Node* args[] = {&arg_a, &arg_b};
  c4c::Node call{};
  call.kind = c4c::NK_CALL;
  call.children = args;
  call.n_children = 2;

  c4c::hir::TypeBindings deduced =
      lowerer.try_deduce_template_type_args(&call, &fn_def, nullptr);

  expect_true(deduced.empty(),
              "repeated type-parameter deduction should reject mismatched record_def despite matching rendered tags");
}

void test_template_deduction_structured_pack_matches_instantiated_record_origin() {
  c4c::hir::Lowerer lowerer;
  c4c::TextTable texts;

  const c4c::TextId fn_text = texts.intern("piecewise_ctor");
  const c4c::TextId args_text = texts.intern("Args1");
  const c4c::TextId tuple_text = texts.intern("tuple");
  const c4c::TextId stale_pattern_text = texts.intern("tuple_T_Args1");
  const c4c::TextId stale_inst_text = texts.intern("tuple_T_int");

  c4c::Node tuple_primary{};
  tuple_primary.kind = c4c::NK_STRUCT_DEF;
  tuple_primary.name = "tuple";
  tuple_primary.unqualified_name = "tuple";
  tuple_primary.unqualified_text_id = tuple_text;
  tuple_primary.namespace_context_id = 7;
  tuple_primary.n_template_params = 1;

  c4c::TypeSpec pattern_arg_ts{};
  pattern_arg_ts.array_size = -1;
  pattern_arg_ts.inner_rank = -1;
  pattern_arg_ts.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(pattern_arg_ts, "Args1", 0);
  pattern_arg_ts.tag_text_id = args_text;
  pattern_arg_ts.template_param_text_id = args_text;
  pattern_arg_ts.template_param_index = 0;
  pattern_arg_ts.template_param_owner_text_id = fn_text;
  pattern_arg_ts.template_param_owner_namespace_context_id = 7;

  c4c::TemplateArgRef pattern_arg{};
  pattern_arg.kind = c4c::TemplateArgKind::Type;
  pattern_arg.type = pattern_arg_ts;
  c4c::TemplateArgRef pattern_args[] = {pattern_arg};

  c4c::TypeSpec param_ts{};
  param_ts.array_size = -1;
  param_ts.inner_rank = -1;
  param_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(param_ts, "StaleTuplePatternRecord", 0);
  param_ts.tag_text_id = stale_pattern_text;
  param_ts.record_def = &tuple_primary;
  param_ts.tpl_struct_origin = "tuple";
  param_ts.tpl_struct_origin_key.context_id = 7;
  param_ts.tpl_struct_origin_key.base_text_id = tuple_text;
  param_ts.tpl_struct_args = c4c::TemplateArgRefList{pattern_args, 1};

  c4c::Node param{};
  param.kind = c4c::NK_VAR;
  param.type = param_ts;
  c4c::Node* params[] = {&param};

  const char* template_params[] = {"Args1"};
  c4c::TextId template_param_text_ids[] = {args_text};
  bool template_param_is_pack[] = {true};
  bool template_param_is_nttp[] = {false};
  c4c::Node fn_def{};
  fn_def.kind = c4c::NK_FUNCTION;
  fn_def.name = "piecewise_ctor";
  fn_def.unqualified_text_id = fn_text;
  fn_def.namespace_context_id = 7;
  fn_def.template_param_names = template_params;
  fn_def.template_param_name_text_ids = template_param_text_ids;
  fn_def.template_param_is_pack = template_param_is_pack;
  fn_def.template_param_is_nttp = template_param_is_nttp;
  fn_def.n_template_params = 1;
  fn_def.params = params;
  fn_def.n_params = 1;

  c4c::TypeSpec int_arg_ts{};
  int_arg_ts.array_size = -1;
  int_arg_ts.inner_rank = -1;
  int_arg_ts.base = c4c::TB_INT;
  c4c::TemplateArgRef int_arg{};
  int_arg.kind = c4c::TemplateArgKind::Type;
  int_arg.type = int_arg_ts;
  c4c::TemplateArgRef inst_args[] = {int_arg};

  c4c::Node tuple_int{};
  tuple_int.kind = c4c::NK_STRUCT_DEF;
  tuple_int.name = "tuple_T_int";
  tuple_int.unqualified_name = "tuple_T_int";
  tuple_int.unqualified_text_id = stale_inst_text;
  tuple_int.namespace_context_id = 7;
  tuple_int.template_origin_name = "tuple";

  c4c::TypeSpec arg_ts{};
  arg_ts.array_size = -1;
  arg_ts.inner_rank = -1;
  arg_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(arg_ts, "StaleTupleInstRecord", 0);
  arg_ts.tag_text_id = stale_inst_text;
  arg_ts.record_def = &tuple_int;
  arg_ts.tpl_struct_origin = "tuple";
  arg_ts.tpl_struct_origin_key.context_id = 7;
  arg_ts.tpl_struct_origin_key.base_text_id = tuple_text;
  arg_ts.tpl_struct_args = c4c::TemplateArgRefList{inst_args, 1};

  c4c::Node arg{};
  arg.kind = c4c::NK_CAST;
  arg.type = arg_ts;
  c4c::Node* args[] = {&arg};

  c4c::hir::TypeBindings deduced_types;
  c4c::hir::NttpBindings deduced_nttp;
  const bool deduced = lowerer.deduce_template_bindings_from_call_args(
      &fn_def, nullptr, args, 1, &deduced_types, &deduced_nttp);

  auto it = deduced_types.find("Args1#0");
  expect_true(deduced && it != deduced_types.end() &&
                  it->second.base == c4c::TB_INT,
              "structured deduction should bind Args1#0=int by tuple origin instead of mismatching concrete instantiated record names");
}

void test_template_materialization_substitutes_foreign_pack_ref_in_nested_owner() {
  c4c::hir::Lowerer lowerer;
  c4c::TextTable texts;

  const c4c::TextId pair_ctor_text = texts.intern("pair");
  const c4c::TextId args1_text = texts.intern("Args1");
  const c4c::TextId tuple_text = texts.intern("tuple");
  const c4c::TextId tuple_param_text = texts.intern("T");

  const char* tuple_param_names[] = {"T"};
  c4c::TextId tuple_param_text_ids[] = {tuple_param_text};
  bool tuple_param_is_pack[] = {false};
  bool tuple_param_is_nttp[] = {false};
  bool tuple_param_has_default[] = {false};
  c4c::TypeSpec tuple_param_default_types[1]{};
  long long tuple_param_default_values[] = {LLONG_MIN};

  c4c::Node tuple_primary{};
  tuple_primary.kind = c4c::NK_STRUCT_DEF;
  tuple_primary.name = "eastl::tuple";
  tuple_primary.unqualified_name = "tuple";
  tuple_primary.unqualified_text_id = tuple_text;
  tuple_primary.namespace_context_id = 7;
  tuple_primary.template_param_names = tuple_param_names;
  tuple_primary.template_param_name_text_ids = tuple_param_text_ids;
  tuple_primary.template_param_is_pack = tuple_param_is_pack;
  tuple_primary.template_param_is_nttp = tuple_param_is_nttp;
  tuple_primary.template_param_has_default = tuple_param_has_default;
  tuple_primary.template_param_default_types = tuple_param_default_types;
  tuple_primary.template_param_default_values = tuple_param_default_values;
  tuple_primary.n_template_params = 1;

  c4c::TypeSpec foreign_pack_ref{};
  foreign_pack_ref.array_size = -1;
  foreign_pack_ref.inner_rank = -1;
  foreign_pack_ref.base = c4c::TB_TYPEDEF;
  foreign_pack_ref.tag_text_id = args1_text;
  foreign_pack_ref.template_param_text_id = args1_text;
  foreign_pack_ref.template_param_index = 0;
  foreign_pack_ref.template_param_owner_text_id = pair_ctor_text;
  foreign_pack_ref.template_param_owner_namespace_context_id = 7;

  c4c::TemplateArgRef tuple_arg{};
  tuple_arg.kind = c4c::TemplateArgKind::Type;
  tuple_arg.type = foreign_pack_ref;
  tuple_arg.debug_text = "Args1";
  c4c::TemplateArgRef tuple_args[] = {tuple_arg};

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tpl_struct_origin = "eastl::tuple";
  owner_ts.tpl_struct_origin_key.context_id = 7;
  owner_ts.tpl_struct_origin_key.base_text_id = tuple_text;
  owner_ts.tpl_struct_args = c4c::TemplateArgRefList{tuple_args, 1};

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  c4c::hir::TypeBindings bindings;
  bindings["Args1#0"] = int_ts;

  const c4c::hir::ResolvedTemplateArgs resolved =
      lowerer.materialize_template_args(&tuple_primary, owner_ts, bindings, {});

  auto it = resolved.type_bindings.begin();
  for (; it != resolved.type_bindings.end(); ++it) {
    if (it->first == "T") break;
  }
  expect_true(it != resolved.type_bindings.end() &&
                  it->second.base == c4c::TB_INT &&
                  resolved.concrete_args.size() == 1 &&
                  !resolved.concrete_args[0].is_value &&
                  resolved.concrete_args[0].type.base == c4c::TB_INT,
              "template materialization should substitute foreign Args1#0=int instead of preserving the Args1 carrier");
}

void test_signature_substitution_preserves_nested_template_owner_identity() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId wrap_text = texts.intern("Wrap");
  const c4c::TextId t_text = texts.intern("T");

  const char* wrap_param_names[] = {"T"};
  c4c::TextId wrap_param_text_ids[] = {t_text};
  bool wrap_param_is_pack[] = {false};
  bool wrap_param_is_nttp[] = {false};
  bool wrap_param_has_default[] = {false};
  c4c::TypeSpec wrap_param_default_types[1]{};
  long long wrap_param_default_values[] = {LLONG_MIN};

  c4c::Node wrap_primary{};
  wrap_primary.kind = c4c::NK_STRUCT_DEF;
  wrap_primary.name = "canonical::Wrap";
  wrap_primary.unqualified_name = "Wrap";
  wrap_primary.unqualified_text_id = wrap_text;
  wrap_primary.namespace_context_id = 17;
  wrap_primary.template_param_names = wrap_param_names;
  wrap_primary.template_param_name_text_ids = wrap_param_text_ids;
  wrap_primary.template_param_is_pack = wrap_param_is_pack;
  wrap_primary.template_param_is_nttp = wrap_param_is_nttp;
  wrap_primary.template_param_has_default = wrap_param_has_default;
  wrap_primary.template_param_default_types = wrap_param_default_types;
  wrap_primary.template_param_default_values = wrap_param_default_values;
  wrap_primary.n_template_params = 1;
  lowerer.register_template_struct_primary("StaleRenderedWrap", &wrap_primary);

  c4c::TypeSpec t_ref{};
  t_ref.array_size = -1;
  t_ref.inner_rank = -1;
  t_ref.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(t_ref, "StaleRenderedT", 0);
  t_ref.tag_text_id = t_text;
  t_ref.template_param_text_id = t_text;

  c4c::TemplateArgRef inner_arg{};
  inner_arg.kind = c4c::TemplateArgKind::Type;
  inner_arg.type = t_ref;
  c4c::TemplateArgRef inner_args[] = {inner_arg};

  c4c::TypeSpec inner_wrap{};
  inner_wrap.array_size = -1;
  inner_wrap.inner_rank = -1;
  inner_wrap.base = c4c::TB_STRUCT;
  inner_wrap.tpl_struct_origin = "StaleRenderedWrap_T0";
  inner_wrap.tpl_struct_origin_key.context_id = 17;
  inner_wrap.tpl_struct_origin_key.base_text_id = wrap_text;
  inner_wrap.tpl_struct_args = c4c::TemplateArgRefList{inner_args, 1};

  c4c::TemplateArgRef outer_arg{};
  outer_arg.kind = c4c::TemplateArgKind::Type;
  outer_arg.type = inner_wrap;
  c4c::TemplateArgRef outer_args[] = {outer_arg};

  c4c::TypeSpec outer_wrap{};
  outer_wrap.array_size = -1;
  outer_wrap.inner_rank = -1;
  outer_wrap.base = c4c::TB_STRUCT;
  outer_wrap.tpl_struct_origin = "StaleRenderedWrap_T1";
  outer_wrap.tpl_struct_origin_key.context_id = 17;
  outer_wrap.tpl_struct_origin_key.base_text_id = wrap_text;
  outer_wrap.tpl_struct_args = c4c::TemplateArgRefList{outer_args, 1};

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  c4c::hir::TypeBindings bindings;
  bindings["T"] = int_ts;

  const c4c::TypeSpec resolved =
      lowerer.substitute_signature_template_type(outer_wrap, &bindings);

  expect_true(resolved.tpl_struct_origin &&
                  std::strcmp(resolved.tpl_struct_origin,
                              "canonical::Wrap") == 0,
              "signature substitution should replace stale outer rendered origin with structured owner primary");
  expect_true(resolved.tpl_struct_args.size == 1 &&
                  resolved.tpl_struct_args.data[0].kind ==
                      c4c::TemplateArgKind::Type,
              "signature substitution should preserve the nested template owner arg");
  const c4c::TypeSpec& nested = resolved.tpl_struct_args.data[0].type;
  expect_true(nested.tpl_struct_origin &&
                  std::strcmp(nested.tpl_struct_origin,
                              "canonical::Wrap") == 0,
              "signature substitution should replace stale nested rendered origin with structured owner primary");
  expect_true(nested.tpl_struct_args.size == 1 &&
                  nested.tpl_struct_args.data[0].kind ==
                      c4c::TemplateArgKind::Type &&
                  nested.tpl_struct_args.data[0].type.base == c4c::TB_INT,
              "signature substitution should substitute nested template owner args before template realization");
}

void test_signature_member_typedef_complete_owner_miss_rejects_rendered_split() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId ns_text = texts.intern("real_ns");
  const c4c::TextId owner_text = texts.intern("MissingOwner");
  const c4c::TextId member_text = texts.intern("value_type");

  c4c::Node stale_record{};
  stale_record.kind = c4c::NK_STRUCT_DEF;
  stale_record.n_member_typedefs = 1;
  stale_record.member_typedef_names = arena.alloc_array<const char*>(1);
  stale_record.member_typedef_names[0] = arena.strdup("value_type");
  stale_record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  stale_record.member_typedef_types[0].array_size = -1;
  stale_record.member_typedef_types[0].inner_rank = -1;
  stale_record.member_typedef_types[0].base = c4c::TB_LONG;

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["real_ns::MissingOwner"] = &stale_record;

  c4c::TextId qualifier_text_ids[] = {ns_text, owner_text};
  c4c::TypeSpec pending{};
  pending.array_size = -1;
  pending.inner_rank = -1;
  pending.base = c4c::TB_TYPEDEF;
  pending.namespace_context_id = 7;
  pending.qualifier_text_ids = qualifier_text_ids;
  pending.n_qualifier_segments = 2;
  pending.tag_text_id = member_text;

  const c4c::TypeSpec resolved =
      lowerer.substitute_signature_template_type(pending, nullptr);
  expect_true(resolved.base == c4c::TB_TYPEDEF &&
                  resolved.tag_text_id == member_text,
              "signature member typedef complete owner/member miss must not split rendered Owner::member text");
}

void test_signature_member_typedef_no_complete_metadata_keeps_rendered_split() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  c4c::Node legacy_record{};
  legacy_record.kind = c4c::NK_STRUCT_DEF;
  legacy_record.n_member_typedefs = 1;
  legacy_record.member_typedef_names = arena.alloc_array<const char*>(1);
  legacy_record.member_typedef_names[0] = arena.strdup("value_type");
  legacy_record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  legacy_record.member_typedef_types[0].array_size = -1;
  legacy_record.member_typedef_types[0].inner_rank = -1;
  legacy_record.member_typedef_types[0].base = c4c::TB_LONG;

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["LegacyOwner"] = &legacy_record;

  c4c::TypeSpec pending{};
  pending.array_size = -1;
  pending.inner_rank = -1;
  pending.base = c4c::TB_TYPEDEF;
  pending.tag_text_id = texts.intern("LegacyOwner::value_type");

  const c4c::TypeSpec resolved =
      lowerer.substitute_signature_template_type(pending, nullptr);
  expect_true(resolved.base == c4c::TB_LONG,
              "signature member typedef without complete owner/member metadata should keep rendered split compatibility");
}

void fill_type_member_typedef_record(c4c::Arena& arena,
                                     c4c::Node& record,
                                     c4c::TextId member_text,
                                     c4c::TypeBase result_base) {
  record.kind = c4c::NK_STRUCT_DEF;
  record.n_member_typedefs = 1;
  record.member_typedef_text_ids = arena.alloc_array<c4c::TextId>(1);
  record.member_typedef_text_ids[0] = member_text;
  record.member_typedef_names = arena.alloc_array<const char*>(1);
  record.member_typedef_names[0] = arena.strdup("type");
  record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  record.member_typedef_types[0].array_size = -1;
  record.member_typedef_types[0].inner_rank = -1;
  record.member_typedef_types[0].base = result_base;
}

c4c::TypeSpec make_signature_owner_type(c4c::TextId owner_text,
                                        c4c::TextId* qualifier_text_ids,
                                        int n_qualifier_segments,
                                        int namespace_context_id) {
  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tag_text_id = owner_text;
  ts.qualifier_text_ids = qualifier_text_ids;
  ts.n_qualifier_segments = n_qualifier_segments;
  ts.namespace_context_id = namespace_context_id;
  return ts;
}

void test_signature_member_typedef_owner_key_canonicalizes_parser_owner_ids() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId parser_ns_text_id = 1;
  const c4c::TextId parser_owner_text_id = 2;
  const c4c::TextId collision_ns_text =
      texts.intern("CollisionMemberTypedefNs");
  const c4c::TextId collision_owner_text =
      texts.intern("CollisionMemberTypedefOwner");
  expect_true(parser_ns_text_id == collision_ns_text &&
                  parser_owner_text_id == collision_owner_text,
              "member typedef fixture should force parser-owned owner TextId collisions");
  const c4c::TextId real_ns_text = texts.intern("RealMemberTypedefNs");
  const c4c::TextId real_owner_text =
      texts.intern("RealMemberTypedefOwner");
  const c4c::TextId member_text = texts.intern("type");

  c4c::Node real_record{};
  fill_type_member_typedef_record(arena, real_record, member_text, c4c::TB_LONG);
  c4c::Node collision_record{};
  fill_type_member_typedef_record(arena, collision_record, member_text,
                                  c4c::TB_INT);

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = 74;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real_owner_text);

  c4c::hir::NamespaceQualifier collision_ns;
  collision_ns.context_id = 74;
  collision_ns.segment_text_ids.push_back(collision_ns_text);
  const c4c::hir::HirRecordOwnerKey collision_key =
      c4c::hir::make_hir_record_owner_key(collision_ns, collision_owner_text);

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_by_owner_[real_key] = &real_record;
  lowerer.struct_def_nodes_by_owner_[collision_key] = &collision_record;

  const char* qualifier_segments[] = {"RealMemberTypedefNs",
                                      "RealMemberTypedefOwner"};
  c4c::TextId qualifier_text_ids[] = {parser_ns_text_id,
                                      parser_owner_text_id};
  c4c::TypeSpec pending{};
  pending.array_size = -1;
  pending.inner_rank = -1;
  pending.base = c4c::TB_TYPEDEF;
  pending.namespace_context_id = 74;
  pending.qualifier_segments = qualifier_segments;
  pending.qualifier_text_ids = qualifier_text_ids;
  pending.n_qualifier_segments = 2;
  pending.tag_text_id = member_text;

  const c4c::TypeSpec resolved =
      lowerer.substitute_signature_template_type(pending, nullptr);
  expect_true(resolved.base == c4c::TB_LONG,
              "signature member typedef owner-key lookup should canonicalize parser-owned qualifier/base ids through spelling carriers");
}

void test_signature_type_member_owner_key_canonicalizes_parser_qualifier_ids() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;

  const c4c::TextId parser_ns_text_id = 1;
  const c4c::TextId collision_ns_text =
      texts.intern("CollisionSignatureTypeNs");
  expect_true(parser_ns_text_id == collision_ns_text,
              "signature type fixture should force a parser-owned qualifier TextId collision");
  const c4c::TextId real_ns_text = texts.intern("RealSignatureTypeNs");
  const c4c::TextId owner_text = texts.intern("RealSignatureTypeOwner");
  const c4c::TextId type_text = texts.intern("type");

  c4c::Node real_record{};
  fill_type_member_typedef_record(arena, real_record, type_text, c4c::TB_LONG);
  c4c::Node collision_record{};
  fill_type_member_typedef_record(arena, collision_record, type_text,
                                  c4c::TB_INT);

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = 75;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, owner_text);

  c4c::hir::NamespaceQualifier collision_ns;
  collision_ns.context_id = 75;
  collision_ns.segment_text_ids.push_back(collision_ns_text);
  const c4c::hir::HirRecordOwnerKey collision_key =
      c4c::hir::make_hir_record_owner_key(collision_ns, owner_text);

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_by_owner_[real_key] = &real_record;
  lowerer.struct_def_nodes_by_owner_[collision_key] = &collision_record;

  const char* qualifier_segments[] = {"RealSignatureTypeNs"};
  c4c::TextId qualifier_text_ids[] = {parser_ns_text_id};
  c4c::TypeSpec ret_ts =
      make_signature_owner_type(owner_text, qualifier_text_ids, 1, 75);
  ret_ts.qualifier_segments = qualifier_segments;

  const c4c::TypeSpec resolved =
      lowerer.prepare_callable_return_type(ret_ts, nullptr, nullptr, nullptr,
                                           "signature-type-canonical", false);
  expect_true(resolved.base == c4c::TB_LONG,
              "signature ::type owner-key lookup should canonicalize parser-owned qualifier ids through spelling carriers");
}

void test_signature_return_type_complete_owner_miss_rejects_rendered_type_fallback() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId ns_text = texts.intern("real_ns");
  const c4c::TextId owner_text = texts.intern("MissingOwner");
  const c4c::TextId type_text = texts.intern("type");

  c4c::Node stale_record{};
  fill_type_member_typedef_record(arena, stale_record, type_text, c4c::TB_LONG);

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["real_ns::MissingOwner"] = &stale_record;

  c4c::TextId qualifier_text_ids[] = {ns_text};
  c4c::TypeSpec ret_ts =
      make_signature_owner_type(owner_text, qualifier_text_ids, 1, 7);

  const c4c::TypeSpec resolved =
      lowerer.prepare_callable_return_type(ret_ts, nullptr, nullptr, nullptr,
                                           "return-type-complete-miss", false);
  expect_true(resolved.base == c4c::TB_STRUCT &&
                  resolved.tag_text_id == owner_text,
              "signature return ::type complete owner/member miss must not use rendered owner compatibility");
}

void test_signature_return_type_no_complete_metadata_keeps_rendered_type_fallback() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId owner_text = texts.intern("LegacyOwner");
  const c4c::TextId type_text = texts.intern("type");

  c4c::Node legacy_record{};
  fill_type_member_typedef_record(arena, legacy_record, type_text, c4c::TB_LONG);

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["LegacyOwner"] = &legacy_record;

  c4c::TypeSpec ret_ts =
      make_signature_owner_type(owner_text, nullptr, 0, -1);

  const c4c::TypeSpec resolved =
      lowerer.prepare_callable_return_type(ret_ts, nullptr, nullptr, nullptr,
                                           "return-type-legacy", false);
  expect_true(resolved.base == c4c::TB_LONG,
              "signature return ::type without complete owner metadata should keep rendered owner compatibility");
}

void test_signature_parameter_type_complete_owner_miss_rejects_rendered_type_fallback() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId ns_text = texts.intern("real_ns");
  const c4c::TextId owner_text = texts.intern("MissingParamOwner");
  const c4c::TextId type_text = texts.intern("type");

  c4c::Node stale_record{};
  fill_type_member_typedef_record(arena, stale_record, type_text, c4c::TB_LONG);

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["real_ns::MissingParamOwner"] = &stale_record;

  c4c::TextId qualifier_text_ids[] = {ns_text};
  c4c::TypeSpec param_ts =
      make_signature_owner_type(owner_text, qualifier_text_ids, 1, 7);
  c4c::hir::Function fn{};
  c4c::hir::Lowerer::FunctionCtx ctx;

  lowerer.append_explicit_callable_param(
      fn, ctx, nullptr, "value", param_ts, nullptr, nullptr,
      "parameter-type-complete-miss", false);
  expect_true(fn.params.size() == 1 &&
                  fn.params[0].type.spec.base == c4c::TB_STRUCT &&
                  fn.params[0].type.spec.tag_text_id == owner_text,
              "signature parameter ::type complete owner/member miss must not use rendered owner compatibility");
}

void test_signature_parameter_type_no_complete_metadata_keeps_rendered_type_fallback() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId owner_text = texts.intern("LegacyParamOwner");
  const c4c::TextId type_text = texts.intern("type");

  c4c::Node legacy_record{};
  fill_type_member_typedef_record(arena, legacy_record, type_text, c4c::TB_LONG);

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_["LegacyParamOwner"] = &legacy_record;

  c4c::TypeSpec param_ts =
      make_signature_owner_type(owner_text, nullptr, 0, -1);
  c4c::hir::Function fn{};
  c4c::hir::Lowerer::FunctionCtx ctx;

  lowerer.append_explicit_callable_param(
      fn, ctx, nullptr, "value", param_ts, nullptr, nullptr,
      "parameter-type-legacy", false);
  expect_true(fn.params.size() == 1 &&
                  fn.params[0].type.spec.base == c4c::TB_LONG,
              "signature parameter ::type without complete owner metadata should keep rendered owner compatibility");
}

void test_pending_type_ref_uses_structured_debug_payload_not_tag() {
  c4c::TextTable texts;

  c4c::Node record{};
  record.kind = c4c::NK_STRUCT_DEF;
  record.name = "StructuredPendingRecord";
  record.unqualified_text_id = texts.intern("StructuredPendingRecord");

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(ts, "StalePendingRenderedTag", 0);
  ts.tag_text_id = texts.intern("StructuredPendingTag");
  ts.template_param_text_id = texts.intern("T");
  ts.deferred_member_type_text_id = texts.intern("type");
  ts.record_def = &record;
  ts.ptr_level = 1;

  const std::string encoded = c4c::hir::encode_pending_type_ref(ts);

  expect_true(encoded.find("StalePendingRenderedTag") == std::string::npos,
              "pending type refs should not depend on stale rendered TypeSpec tag spelling");
  expect_true(encoded.find("tag_text_id=") != std::string::npos &&
                  encoded.find("template_param_text_id=") != std::string::npos &&
                  encoded.find("record_def_text_id=") != std::string::npos,
              "pending type refs should expose structured debug payload ids");
}

void test_pending_type_ref_no_metadata_keeps_shape_payload() {
  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_TYPEDEF;
  ts.ptr_level = 2;

  const std::string encoded = c4c::hir::encode_pending_type_ref(ts);

  expect_true(encoded.find("base=") != std::string::npos &&
                  encoded.find("|ptr=2") != std::string::npos &&
                  encoded.find("tag_text_id=") != std::string::npos,
              "pending type refs without name metadata should retain structural shape payload");
}

void test_specialization_display_type_uses_structured_record_key_not_tag() {
  c4c::TextTable texts;

  c4c::Node record{};
  record.kind = c4c::NK_STRUCT_DEF;
  record.name = "CanonicalRecord";
  record.namespace_context_id = 42;
  record.unqualified_text_id = texts.intern("CanonicalRecord");

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(ts, "StaleCanonicalRenderedTag", 0);
  ts.tag_text_id = texts.intern("CanonicalRecord");
  ts.record_def = &record;
  ts.ptr_level = 1;

  const std::string encoded =
      c4c::hir::format_type_for_specialization_display_key(ts);

  expect_true(encoded.find("StaleCanonicalRenderedTag") == std::string::npos,
              "specialization display strings should not depend on stale rendered TypeSpec tag spelling");
  expect_true(encoded.find("struct.record.ctx42.text" +
                           std::to_string(record.unqualified_text_id)) !=
                  std::string::npos,
              "specialization display strings should expose structured record identity");
  expect_true(encoded.find("*") != std::string::npos,
              "specialization display strings should retain declarator shape");
}

void test_specialization_display_type_no_metadata_uses_explicit_unknown_name() {
  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(ts, "LegacyOnlyCanonicalTag", 0);

  const std::string encoded =
      c4c::hir::format_type_for_specialization_display_key(ts);

  expect_true(encoded == "struct.?",
              "specialization display strings without structured name metadata should use explicit unknown spelling");
}

void test_type_suffix_for_mangling_uses_record_def_not_stale_tag() {
  c4c::TextTable texts;

  c4c::Node record{};
  record.kind = c4c::NK_STRUCT_DEF;
  record.name = "StructuredMangleRecord";
  record.unqualified_name = "StructuredMangleRecord";
  record.unqualified_text_id = texts.intern("StructuredMangleRecord");
  record.namespace_context_id = 31;

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(ts, "StaleMangleTag", 0);
  ts.tag_text_id = record.unqualified_text_id;
  ts.record_def = &record;
  ts.ptr_level = 1;

  const std::string suffix = c4c::hir::type_suffix_for_mangling(ts);

  expect_true(suffix.find("StaleMangleTag") == std::string::npos,
              "type mangling suffix should not depend on stale rendered TypeSpec tag spelling");
  expect_true(suffix == "p1TStructuredMangleRecord",
              "type mangling suffix should preserve structured record spelling when available");
}

void test_type_suffix_for_mangling_no_metadata_is_explicit_unknown() {
  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(ts, "LegacyOnlyMangleTag", 0);

  const std::string suffix = c4c::hir::type_suffix_for_mangling(ts);

  expect_true(suffix == "unknown",
              "type mangling suffix without structured name metadata should use explicit unknown spelling");
}

void test_pending_consteval_nttp_handoff_carries_text_id_bindings() {
  constexpr std::string_view source = R"cpp(
template<int N>
consteval int plus_one() { return N + 1; }

int use_consteval_nttp() { return plus_one<41>(); }
)cpp";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_hir_lookup_tests.cpp");
  c4c::Node* root = parser.parse();
  auto sema_result = c4c::sema::analyze_program(
      root, c4c::sema_profile_from(c4c::SourceProfile::CppSubset));
  expect_true(sema_result.validation.ok,
              "pending consteval NTTP fixture should validate");

  auto initial = c4c::hir::build_initial_hir(
      root, &sema_result.canonical.resolved_types);
  const c4c::Node* plus_one_def =
      initial.ct_state->find_consteval_def("plus_one");
  expect_true(plus_one_def && plus_one_def->template_param_name_text_ids &&
                  plus_one_def->template_param_name_text_ids[0] != c4c::kInvalidText,
              "fixture NTTP parameter should carry parser TextId metadata");

  const c4c::hir::PendingConstevalExpr* pending = nullptr;
  for (const auto& expr : initial.module->expr_pool) {
    if (const auto* candidate =
            std::get_if<c4c::hir::PendingConstevalExpr>(&expr.payload)) {
      pending = candidate;
      break;
    }
  }
  expect_true(pending != nullptr,
              "initial HIR should retain a pending consteval call before reduction");
  expect_true(pending->nttp_bindings.size() == 1 &&
                  pending->nttp_bindings.at("N") == 41,
              "pending consteval should retain rendered NTTP compatibility bindings");

  const c4c::TextId n_text_id = plus_one_def->template_param_name_text_ids[0];
  const auto text_it = pending->nttp_bindings_by_text.find(n_text_id);
  expect_true(text_it != pending->nttp_bindings_by_text.end(),
              "pending consteval should carry TextId NTTP binding mirror");
  expect_true(text_it->second == 41,
              "TextId NTTP binding mirror should preserve the constant value");
}

void test_pending_consteval_nested_call_uses_compile_time_function_metadata() {
  constexpr std::string_view source = R"cpp(
consteval int inner() { return 5; }
consteval int outer() { return inner(); }

int use_nested_consteval() { return outer(); }
)cpp";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_hir_lookup_tests.cpp");
  c4c::Node* root = parser.parse();
  auto sema_result = c4c::sema::analyze_program(
      root, c4c::sema_profile_from(c4c::SourceProfile::CppSubset));
  expect_true(sema_result.validation.ok,
              "nested consteval HIR fixture should validate");

  auto initial = c4c::hir::build_initial_hir(
      root, &sema_result.canonical.resolved_types);
  const c4c::Node* inner_def =
      initial.ct_state->find_consteval_def("inner");
  const c4c::Node* outer_def =
      initial.ct_state->find_consteval_def("outer");
  expect_true(inner_def && outer_def,
              "compile-time state should register both nested consteval definitions");
  expect_true(inner_def->unqualified_text_id != c4c::kInvalidText &&
                  outer_def->unqualified_text_id != c4c::kInvalidText,
              "nested consteval definitions should carry TextId metadata");
  expect_true(initial.ct_state->consteval_fn_defs_by_text().count(
                  inner_def->unqualified_text_id) == 1 &&
                  initial.ct_state->consteval_fn_defs_by_text().count(
                      outer_def->unqualified_text_id) == 1,
              "compile-time state should expose consteval TextId function metadata");
  expect_true(!initial.ct_state->consteval_fn_defs_by_key().empty(),
              "compile-time state should expose consteval structured function metadata");

  bool saw_pending_before = false;
  for (const auto& expr : initial.module->expr_pool) {
    if (std::holds_alternative<c4c::hir::PendingConstevalExpr>(expr.payload)) {
      saw_pending_before = true;
      break;
    }
  }
  expect_true(saw_pending_before,
              "initial HIR should retain a pending outer consteval call before reduction");

  c4c::hir::run_compile_time_engine(
      *initial.module, initial.ct_state, initial.deferred_instantiate,
      initial.deferred_instantiate_type);

  bool found_reduced_nested_call = false;
  for (const auto& expr : initial.module->expr_pool) {
    expect_true(!std::holds_alternative<c4c::hir::PendingConstevalExpr>(expr.payload),
                "compile-time function metadata should let nested consteval calls reduce");
    const auto* literal = std::get_if<c4c::hir::IntLiteral>(&expr.payload);
    if (literal && literal->value == 5) found_reduced_nested_call = true;
  }
  expect_true(found_reduced_nested_call,
              "nested consteval call should reduce to the inner function result");
}

void test_template_call_nttp_handoff_carries_text_id_bindings() {
  constexpr std::string_view source = R"cpp(
template<int V>
consteval int lift() { return V + 3; }

template<int V>
int inner() { return lift<V>(); }

template<int V>
int outer() { return inner<V>(); }

int use_template_call_nttp() { return outer<39>(); }
)cpp";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_hir_lookup_tests.cpp");
  c4c::Node* root = parser.parse();
  auto sema_result = c4c::sema::analyze_program(
      root, c4c::sema_profile_from(c4c::SourceProfile::CppSubset));
  expect_true(sema_result.validation.ok,
              "template-call NTTP handoff fixture should validate");

  c4c::Node* inner_def =
      const_cast<c4c::Node*>(find_function_node_by_name(root, "inner"));
  expect_true(inner_def && inner_def->template_param_names &&
                  inner_def->template_param_name_text_ids &&
                  inner_def->template_param_name_text_ids[0] != c4c::kInvalidText,
              "fixture inner template should carry NTTP TextId metadata");
  inner_def->template_param_names[0] = arena.strdup("__stale_inner_rendered_nttp");

  auto initial = c4c::hir::build_initial_hir(
      root, &sema_result.canonical.resolved_types);
  const c4c::TextId inner_param_text_id =
      inner_def->template_param_name_text_ids[0];

  const c4c::hir::TemplateCallInfo* inner_call_info = nullptr;
  for (const auto& fn : initial.module->functions) {
    if (fn.template_origin != "outer") continue;
    for (const auto& block : fn.blocks) {
      for (const auto& stmt : block.stmts) {
        const auto* ret = std::get_if<c4c::hir::ReturnStmt>(&stmt.payload);
        if (!ret || !ret->expr) continue;
        const auto* call = std::get_if<c4c::hir::CallExpr>(
            &initial.module->expr_pool[ret->expr->value].payload);
        if (call && call->template_info &&
            call->template_info->source_template == "inner") {
          inner_call_info = &*call->template_info;
          break;
        }
      }
      if (inner_call_info) break;
    }
    if (inner_call_info) break;
  }
  expect_true(inner_call_info != nullptr,
              "initial HIR should retain template-call metadata for outer<39> -> inner<V>");
  expect_true(inner_call_info->nttp_args.size() == 1 &&
                  inner_call_info->nttp_args.at("__stale_inner_rendered_nttp") == 39,
              "template-call metadata should retain rendered NTTP compatibility bindings");
  const auto call_text_it =
      inner_call_info->nttp_args_by_text.find(inner_param_text_id);
  expect_true(call_text_it != inner_call_info->nttp_args_by_text.end(),
              "template-call metadata should carry TextId NTTP binding mirror");
  expect_true(call_text_it->second == 39,
              "template-call TextId NTTP binding mirror should preserve the value");

  c4c::hir::run_compile_time_engine(
      *initial.module, initial.ct_state, initial.deferred_instantiate,
      initial.deferred_instantiate_type);
  for (const auto& expr : initial.module->expr_pool) {
    expect_true(!std::holds_alternative<c4c::hir::PendingConstevalExpr>(expr.payload),
                "TextId NTTP env data should let deferred inner consteval calls reduce");
  }
  const auto tdef_it = initial.module->template_defs.find("inner");
  expect_true(tdef_it != initial.module->template_defs.end() &&
                  !tdef_it->second.instances.empty(),
              "compile-time engine should record the deferred inner instantiation");
  bool found_text_instance = false;
  for (const auto& inst : tdef_it->second.instances) {
    const auto inst_text_it = inst.nttp_bindings_by_text.find(inner_param_text_id);
    if (inst_text_it != inst.nttp_bindings_by_text.end() &&
        inst_text_it->second == 39) {
      found_text_instance = true;
      break;
    }
  }
  expect_true(found_text_instance,
              "deferred template-call instantiation should preserve TextId NTTP env data");
}

void test_template_global_nttp_init_uses_text_id_function_ctx_binding() {
  constexpr std::string_view source = R"cpp(
template<int V>
consteval int lift() { return V + 3; }

template<int V>
int value = lift<V>();

int use_template_global_nttp() { return value<39>; }
)cpp";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_hir_lookup_tests.cpp");
  c4c::Node* root = parser.parse();
  auto sema_result = c4c::sema::analyze_program(
      root, c4c::sema_profile_from(c4c::SourceProfile::CppSubset));
  expect_true(sema_result.validation.ok,
              "template-global NTTP FunctionCtx fixture should validate");

  c4c::Node* value_def =
      const_cast<c4c::Node*>(find_global_node_by_name(root, "value"));
  expect_true(value_def && value_def->template_param_names &&
                  value_def->template_param_name_text_ids &&
                  value_def->template_param_name_text_ids[0] != c4c::kInvalidText,
              "fixture template global should carry NTTP TextId metadata");
  value_def->template_param_names[0] = arena.strdup("__stale_global_rendered_nttp");

  auto initial = c4c::hir::build_initial_hir(
      root, &sema_result.canonical.resolved_types);
  c4c::hir::run_compile_time_engine(
      *initial.module, initial.ct_state, initial.deferred_instantiate,
      initial.deferred_instantiate_type);

  bool found_reduced_template_global = false;
  for (const auto& global : initial.module->globals) {
    if (global.name.find("value_") != 0) continue;
    const auto* init = std::get_if<c4c::hir::InitScalar>(&global.init);
    if (!init) continue;
    const auto* literal = std::get_if<c4c::hir::IntLiteral>(
        &initial.module->expr_pool[init->expr.value].payload);
    if (literal && literal->value == 42) {
      found_reduced_template_global = true;
      break;
    }
  }
  expect_true(found_reduced_template_global,
              "template-global initializer should reduce through TextId NTTP FunctionCtx binding despite stale rendered name");
}

void test_static_member_nttp_const_eval_prefers_text_id_binding() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId param_text_id = module.link_name_texts->intern("V");
  c4c::Node stale_ref{};
  stale_ref.kind = c4c::NK_VAR;
  stale_ref.name = "__stale_rendered_nttp";
  stale_ref.unqualified_name = "V";
  stale_ref.unqualified_text_id = param_text_id;

  c4c::hir::NttpBindings rendered_bindings;
  rendered_bindings["__stale_rendered_nttp"] = 7;
  c4c::hir::NttpTextBindings text_bindings;
  text_bindings[param_text_id] = 39;

  const long long value = lowerer.eval_const_int_with_nttp_bindings(
      &stale_ref, rendered_bindings, &text_bindings);
  expect_true(value == 39,
              "static-member NTTP const eval should prefer TextId bindings over stale rendered names");
}

void test_template_value_arg_static_member_rejects_rendered_owner_split() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node owner{};
  owner.kind = c4c::NK_STRUCT_DEF;
  owner.name = "StaleRenderedOwner";
  owner.template_origin_name = "is_signed";
  owner.n_template_args = 1;
  c4c::TypeSpec arg_type{};
  arg_type.base = c4c::TB_INT;
  arg_type.array_size = -1;
  arg_type.inner_rank = -1;
  owner.template_arg_types = &arg_type;
  bool arg_is_value = false;
  owner.template_arg_is_value = &arg_is_value;
  lowerer.struct_def_nodes_["StaleRenderedOwner"] = &owner;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "StaleRenderedOwner::value";

  long long value = 0;
  const bool evaluated =
      lowerer.try_eval_template_value_arg_expr(&ref, nullptr, &value);
  expect_true(!evaluated,
              "template value-arg const eval must not split rendered owner/member spelling");
}

void test_template_value_arg_static_member_uses_structured_owner_key() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId ns_text = module.link_name_texts->intern("RealNs");
  const c4c::TextId owner_text = module.link_name_texts->intern("RealOwner");
  const c4c::TextId member_text = module.link_name_texts->intern("value");
  c4c::hir::NamespaceQualifier owner_ns;
  owner_ns.context_id = 73;
  owner_ns.segment_text_ids.push_back(ns_text);
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(owner_ns, owner_text);
  const std::optional<c4c::hir::HirStructMemberLookupKey> member_key =
      lowerer.make_struct_member_lookup_key(owner_key, member_text);
  expect_true(member_key.has_value(),
              "fixture should build a structured static member lookup key");
  lowerer.struct_static_member_const_values_by_owner_[*member_key] = 123;

  c4c::TextId qualifier_text_ids[] = {ns_text, owner_text};
  const char* qualifier_segments[] = {"StaleRenderedNs", "StaleRenderedOwner"};
  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "StaleRenderedOwner::wrong";
  ref.unqualified_name = "stale_rendered_member";
  ref.unqualified_text_id = member_text;
  ref.qualifier_text_ids = qualifier_text_ids;
  ref.qualifier_segments = qualifier_segments;
  ref.n_qualifier_segments = 2;
  ref.namespace_context_id = owner_ns.context_id;

  long long value = 0;
  const bool evaluated =
      lowerer.try_eval_template_value_arg_expr(&ref, nullptr, &value);
  expect_true(evaluated && value == 123,
              "template value-arg const eval should use structured owner/member metadata");
}

void test_scalar_static_member_rejects_rendered_owner_split() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;

  c4c::Node rendered_decl{};
  rendered_decl.kind = c4c::NK_DECL;
  rendered_decl.name = "value";
  rendered_decl.unqualified_name = "value";
  rendered_decl.unqualified_text_id = module.link_name_texts->intern("value");
  rendered_decl.type = int_ts;
  lowerer.struct_static_member_decls_["StaleRenderedOwner"]["value"] =
      &rendered_decl;
  lowerer.struct_static_member_const_values_["StaleRenderedOwner"]["value"] =
      77;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "StaleRenderedOwner::value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id = rendered_decl.unqualified_text_id;
  ref.type = int_ts;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(nullptr, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* literal =
      expr ? std::get_if<c4c::hir::IntLiteral>(&expr->payload) : nullptr;
  expect_true(!(literal && literal->value == 77),
              "scalar static-member lowering must not split rendered owner/member spelling");
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref != nullptr,
              "rendered-only scalar static-member spelling should fall through as a normal reference");
}

void test_scalar_static_member_uses_member_text_after_structured_owner() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;

  const c4c::TextId ns_text = module.link_name_texts->intern("RealNs");
  const c4c::TextId owner_text = module.link_name_texts->intern("RealOwner");
  const c4c::TextId member_text = module.link_name_texts->intern("value");
  c4c::hir::NamespaceQualifier owner_ns;
  owner_ns.context_id = 91;
  owner_ns.segment_text_ids.push_back(ns_text);
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(owner_ns, owner_text);
  module.index_struct_def_owner(owner_key, "RealOwner", true);
  const std::optional<c4c::hir::HirStructMemberLookupKey> member_key =
      lowerer.make_struct_member_lookup_key(owner_key, member_text);
  expect_true(member_key.has_value(),
              "fixture should build a scalar static-member owner/member key");
  lowerer.struct_static_member_const_values_by_owner_[*member_key] = 123;

  c4c::TextId qualifier_text_ids[] = {ns_text, owner_text};
  const char* qualifier_segments[] = {"StaleRenderedNs", "StaleRenderedOwner"};
  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "StaleRenderedOwner::wrong";
  ref.unqualified_name = "stale_rendered_member";
  ref.unqualified_text_id = member_text;
  ref.qualifier_text_ids = qualifier_text_ids;
  ref.qualifier_segments = qualifier_segments;
  ref.n_qualifier_segments = 2;
  ref.namespace_context_id = owner_ns.context_id;
  ref.type = int_ts;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(nullptr, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* literal =
      expr ? std::get_if<c4c::hir::IntLiteral>(&expr->payload) : nullptr;
  expect_true(literal && literal->value == 123,
              "scalar static-member lowering should use member TextId only after structured owner authority exists");
}

void test_scalar_static_member_non_template_uses_owner_key_without_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;

  const c4c::TextId left_text = module.link_name_texts->intern("left");
  const c4c::TextId right_text = module.link_name_texts->intern("right");
  const c4c::TextId box_text = module.link_name_texts->intern("Box");
  const c4c::TextId value_text = module.link_name_texts->intern("value");

  c4c::hir::NamespaceQualifier left_ns;
  left_ns.context_id = 1;
  left_ns.segment_text_ids.push_back(left_text);
  c4c::hir::NamespaceQualifier right_ns;
  right_ns.context_id = 2;
  right_ns.segment_text_ids.push_back(right_text);
  const c4c::hir::HirRecordOwnerKey left_key =
      c4c::hir::make_hir_record_owner_key(left_ns, box_text);
  const c4c::hir::HirRecordOwnerKey right_key =
      c4c::hir::make_hir_record_owner_key(right_ns, box_text);

  const std::optional<c4c::hir::HirStructMemberLookupKey> left_member =
      lowerer.make_struct_member_lookup_key(left_key, value_text);
  const std::optional<c4c::hir::HirStructMemberLookupKey> right_member =
      lowerer.make_struct_member_lookup_key(right_key, value_text);
  expect_true(left_member.has_value() && right_member.has_value(),
              "fixture should build namespace-distinct static-member keys");
  lowerer.struct_static_member_const_values_by_owner_[*left_member] = 11;
  lowerer.struct_static_member_const_values_by_owner_[*right_member] = 23;

  const c4c::TextId stale_function_text = module.link_name_texts->intern("main");
  c4c::TextId qualifier_text_ids[] = {right_text, stale_function_text};
  const char* qualifier_segments[] = {"right", "Box"};
  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "right::Box::value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id = value_text;
  ref.qualifier_text_ids = qualifier_text_ids;
  ref.qualifier_segments = qualifier_segments;
  ref.n_qualifier_segments = 2;
  ref.namespace_context_id = right_ns.context_id;
  ref.type = int_ts;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(nullptr, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* literal =
      expr ? std::get_if<c4c::hir::IntLiteral>(&expr->payload) : nullptr;
  expect_true(literal && literal->value == 23,
              "non-template scalar static-member lookup should fall back from stale qualifier TextIds to structured qualifier segments without rendered owner recovery");
}

void test_consteval_record_layout_prefers_hir_owner_key_over_stale_tag() {
  c4c::hir::Module module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("RealLayout");
  record_node->unqualified_name = arena.strdup("RealLayout");
  record_node->unqualified_text_id = module.link_name_texts->intern("RealLayout");
  record_node->namespace_context_id = 17;

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealLayout";
  real_def.tag_text_id = record_node->unqualified_text_id;
  real_def.ns_qual.context_id = record_node->namespace_context_id;
  real_def.size_bytes = 24;
  real_def.align_bytes = 8;
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(real_def);
  module.index_struct_def_owner(owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleRenderedLayout";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleRenderedLayout");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.size_bytes = 4;
  stale_def.align_bytes = 4;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "StaleRenderedLayout", 0);
  query.tag_text_id = record_node->unqualified_text_id;
  query.namespace_context_id = record_node->namespace_context_id;
  query.record_def = record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  c4c::hir::ConstEvalEnv env;
  env.struct_defs = &module.struct_defs;
  env.struct_def_owner_index = &module.struct_def_owner_index;

  c4c::Node sizeof_node{};
  sizeof_node.kind = c4c::NK_SIZEOF_TYPE;
  sizeof_node.type = query;

  const c4c::hir::ConstEvalResult size =
      c4c::hir::evaluate_constant_expr(&sizeof_node, env);
  expect_true(size.ok() && size.as_int() == 24,
              "consteval sizeof should prefer structured HIR owner layout over stale rendered tag");

  c4c::Node alignof_node{};
  alignof_node.kind = c4c::NK_ALIGNOF_TYPE;
  alignof_node.type = query;

  const c4c::hir::ConstEvalResult align =
      c4c::hir::evaluate_constant_expr(&alignof_node, env);
  expect_true(align.ok() && align.as_int() == 8,
              "consteval alignof should prefer structured HIR owner layout over stale rendered tag");
}

void test_layout_type_lookup_prefers_structured_owner_over_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("RealLayoutOwner");
  record_node->unqualified_name = arena.strdup("RealLayoutOwner");
  record_node->unqualified_text_id =
      module.link_name_texts->intern("RealLayoutOwner");
  record_node->namespace_context_id = 22;

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealLayoutOwner";
  real_def.tag_text_id = record_node->unqualified_text_id;
  real_def.ns_qual.context_id = record_node->namespace_context_id;
  real_def.size_bytes = 32;
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(real_def);
  module.index_struct_def_owner(owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleLayoutOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleLayoutOwner");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.size_bytes = 4;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "StaleLayoutOwner", 0);
  query.tag_text_id = record_node->unqualified_text_id;
  query.namespace_context_id = record_node->namespace_context_id;
  query.record_def = record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  const c4c::hir::HirStructDef* layout =
      lowerer.find_struct_def_for_layout_type(query);
  expect_true(layout && layout->tag == "RealLayoutOwner" &&
                  layout->size_bytes == 32,
              "layout TypeSpec lookup should prefer structured owner metadata over stale rendered tag");
}

void test_layout_type_lookup_structured_owner_miss_rejects_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("MissingLayoutOwner");
  record_node->unqualified_name = arena.strdup("MissingLayoutOwner");
  record_node->unqualified_text_id =
      module.link_name_texts->intern("MissingLayoutOwner");
  record_node->namespace_context_id = 23;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleLayoutOwnerMiss";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleLayoutOwnerMiss");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.size_bytes = 4;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "StaleLayoutOwnerMiss", 0);
  query.tag_text_id = record_node->unqualified_text_id;
  query.namespace_context_id = record_node->namespace_context_id;
  query.record_def = record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  const c4c::hir::HirStructDef* layout =
      lowerer.find_struct_def_for_layout_type(query);
  expect_true(!layout,
              "layout TypeSpec lookup should reject stale rendered tag after structured owner miss");
}

void test_layout_type_lookup_no_owner_uses_tag_text_id_compatibility() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef compat_def;
  compat_def.tag = "CompatLayoutOwner";
  compat_def.tag_text_id = module.link_name_texts->intern("CompatLayoutOwner");
  compat_def.size_bytes = 48;
  compat_def.align_bytes = 16;
  module.struct_defs[compat_def.tag] = compat_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "StaleRenderedLayoutCompat", 0);
  query.tag_text_id = compat_def.tag_text_id;
  query.namespace_context_id = -1;
  query.array_size = -1;
  query.inner_rank = -1;

  const c4c::hir::HirStructDef* layout =
      lowerer.find_struct_def_for_layout_type(query);
  expect_true(layout && layout->tag == "CompatLayoutOwner" &&
                  layout->size_bytes == 48,
              "layout TypeSpec no-owner compatibility should use tag_text_id, not rendered tag");
}

void test_compute_struct_layout_field_uses_record_def_before_stale_tag() {
  c4c::hir::Module module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  *record_node = {};
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("RealFieldLayoutOwner");
  record_node->unqualified_name = arena.strdup("RealFieldLayoutOwner");
  record_node->unqualified_text_id =
      module.link_name_texts->intern("RealFieldLayoutOwner");
  record_node->namespace_context_id = 24;

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealFieldLayoutOwner";
  real_def.tag_text_id = record_node->unqualified_text_id;
  real_def.ns_qual.context_id = record_node->namespace_context_id;
  real_def.size_bytes = 32;
  real_def.align_bytes = 8;
  module.index_struct_def_owner(c4c::hir::make_hir_record_owner_key(real_def),
                                real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleFieldLayoutOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleFieldLayoutOwner");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.size_bytes = 4;
  stale_def.align_bytes = 4;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec field_ts{};
  field_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(field_ts, "StaleFieldLayoutOwner", 0);
  field_ts.tag_text_id = record_node->unqualified_text_id;
  field_ts.namespace_context_id = record_node->namespace_context_id;
  field_ts.record_def = record_node;
  field_ts.array_size = -1;
  field_ts.inner_rank = -1;

  c4c::hir::HirStructField field;
  field.name = "payload";
  field.elem_type = field_ts;

  c4c::hir::HirStructDef container;
  container.tag = "Container";
  container.tag_text_id = module.link_name_texts->intern("Container");
  container.fields.push_back(field);

  c4c::hir::compute_struct_layout(&module, container);

  expect_true(container.fields.front().size_bytes == 32,
              "struct layout field sizing should prefer record_def owner metadata over stale rendered tag");
  expect_true(container.fields.front().align_bytes == 8,
              "struct layout field alignment should prefer record_def owner metadata over stale rendered tag");
}

void test_compute_struct_layout_field_structured_miss_rejects_stale_tag() {
  c4c::hir::Module module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  *record_node = {};
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("MissingFieldLayoutOwner");
  record_node->unqualified_name = arena.strdup("MissingFieldLayoutOwner");
  record_node->unqualified_text_id =
      module.link_name_texts->intern("MissingFieldLayoutOwner");
  record_node->namespace_context_id = 25;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleFieldLayoutMiss";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleFieldLayoutMiss");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.size_bytes = 64;
  stale_def.align_bytes = 16;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec field_ts{};
  field_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(field_ts, "StaleFieldLayoutMiss", 0);
  field_ts.tag_text_id = record_node->unqualified_text_id;
  field_ts.namespace_context_id = record_node->namespace_context_id;
  field_ts.record_def = record_node;
  field_ts.array_size = -1;
  field_ts.inner_rank = -1;

  c4c::hir::HirStructField field;
  field.name = "payload";
  field.elem_type = field_ts;

  c4c::hir::HirStructDef container;
  container.tag = "ContainerMiss";
  container.tag_text_id = module.link_name_texts->intern("ContainerMiss");
  container.fields.push_back(field);

  c4c::hir::compute_struct_layout(&module, container);

  expect_true(container.fields.front().size_bytes == 4,
              "struct layout field sizing should reject stale rendered tag after structured owner miss");
  expect_true(container.fields.front().align_bytes == 4,
              "struct layout field alignment should reject stale rendered tag after structured owner miss");
}

void test_builtin_record_layout_prefers_hir_owner_key_over_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("RealBuiltinLayout");
  record_node->unqualified_name = arena.strdup("RealBuiltinLayout");
  record_node->unqualified_text_id = module.link_name_texts->intern("RealBuiltinLayout");
  record_node->namespace_context_id = 21;

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealBuiltinLayout";
  real_def.tag_text_id = record_node->unqualified_text_id;
  real_def.ns_qual.context_id = record_node->namespace_context_id;
  real_def.size_bytes = 40;
  real_def.align_bytes = 16;
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(real_def);
  module.index_struct_def_owner(owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleBuiltinLayout";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleBuiltinLayout");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.size_bytes = 4;
  stale_def.align_bytes = 4;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "StaleBuiltinLayout", 0);
  query.tag_text_id = record_node->unqualified_text_id;
  query.namespace_context_id = record_node->namespace_context_id;
  query.record_def = record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  c4c::Node sizeof_node{};
  sizeof_node.kind = c4c::NK_SIZEOF_TYPE;
  sizeof_node.type = query;
  const c4c::hir::ExprId sizeof_id =
      lowerer.lower_builtin_sizeof_type(nullptr, &sizeof_node);
  const c4c::hir::Expr* sizeof_expr = module.find_expr(sizeof_id);
  const auto* sizeof_lit =
      sizeof_expr ? std::get_if<c4c::hir::IntLiteral>(&sizeof_expr->payload) : nullptr;
  expect_true(sizeof_lit && sizeof_lit->value == 40,
              "HIR builtin sizeof should prefer structured owner layout over stale rendered tag");

  c4c::Node alignof_node{};
  alignof_node.kind = c4c::NK_ALIGNOF_TYPE;
  alignof_node.type = query;
  const c4c::hir::ExprId alignof_id =
      lowerer.lower_builtin_alignof_type(nullptr, &alignof_node);
  const c4c::hir::Expr* alignof_expr = module.find_expr(alignof_id);
  const auto* alignof_lit =
      alignof_expr ? std::get_if<c4c::hir::IntLiteral>(&alignof_expr->payload) : nullptr;
  expect_true(alignof_lit && alignof_lit->value == 16,
              "HIR builtin alignof should prefer structured owner layout over stale rendered tag");
}

void test_builtin_record_layout_structured_owner_miss_rejects_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("MissingBuiltinLayout");
  record_node->unqualified_name = arena.strdup("MissingBuiltinLayout");
  record_node->unqualified_text_id =
      module.link_name_texts->intern("MissingBuiltinLayout");
  record_node->namespace_context_id = 24;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleBuiltinLayoutMiss";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleBuiltinLayoutMiss");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.size_bytes = 64;
  stale_def.align_bytes = 32;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "StaleBuiltinLayoutMiss", 0);
  query.tag_text_id = record_node->unqualified_text_id;
  query.namespace_context_id = record_node->namespace_context_id;
  query.record_def = record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  c4c::Node sizeof_node{};
  sizeof_node.kind = c4c::NK_SIZEOF_TYPE;
  sizeof_node.type = query;
  const c4c::hir::ExprId sizeof_id =
      lowerer.lower_builtin_sizeof_type(nullptr, &sizeof_node);
  const c4c::hir::Expr* sizeof_expr = module.find_expr(sizeof_id);
  const auto* sizeof_lit =
      sizeof_expr ? std::get_if<c4c::hir::IntLiteral>(&sizeof_expr->payload) : nullptr;
  expect_true(sizeof_lit && sizeof_lit->value == 4,
              "HIR builtin sizeof should reject stale rendered tag after structured owner miss");

  c4c::Node alignof_node{};
  alignof_node.kind = c4c::NK_ALIGNOF_TYPE;
  alignof_node.type = query;
  const c4c::hir::ExprId alignof_id =
      lowerer.lower_builtin_alignof_type(nullptr, &alignof_node);
  const c4c::hir::Expr* alignof_expr = module.find_expr(alignof_id);
  const auto* alignof_lit =
      alignof_expr ? std::get_if<c4c::hir::IntLiteral>(&alignof_expr->payload) : nullptr;
  expect_true(alignof_lit && alignof_lit->value == 4,
              "HIR builtin alignof should reject stale rendered tag after structured owner miss");
}

void test_builtin_record_layout_no_owner_uses_tag_text_id_compatibility() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef compat_def;
  compat_def.tag = "CompatBuiltinLayout";
  compat_def.tag_text_id = module.link_name_texts->intern("CompatBuiltinLayout");
  compat_def.size_bytes = 24;
  compat_def.align_bytes = 8;
  module.struct_defs[compat_def.tag] = compat_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "StaleRenderedBuiltinCompat", 0);
  query.tag_text_id = compat_def.tag_text_id;
  query.namespace_context_id = -1;
  query.array_size = -1;
  query.inner_rank = -1;

  c4c::Node sizeof_node{};
  sizeof_node.kind = c4c::NK_SIZEOF_TYPE;
  sizeof_node.type = query;
  const c4c::hir::ExprId sizeof_id =
      lowerer.lower_builtin_sizeof_type(nullptr, &sizeof_node);
  const c4c::hir::Expr* sizeof_expr = module.find_expr(sizeof_id);
  const auto* sizeof_lit =
      sizeof_expr ? std::get_if<c4c::hir::IntLiteral>(&sizeof_expr->payload) : nullptr;
  expect_true(sizeof_lit && sizeof_lit->value == 24,
              "HIR builtin sizeof no-owner compatibility should use tag_text_id, not rendered tag");

  c4c::Node alignof_node{};
  alignof_node.kind = c4c::NK_ALIGNOF_TYPE;
  alignof_node.type = query;
  const c4c::hir::ExprId alignof_id =
      lowerer.lower_builtin_alignof_type(nullptr, &alignof_node);
  const c4c::hir::Expr* alignof_expr = module.find_expr(alignof_id);
  const auto* alignof_lit =
      alignof_expr ? std::get_if<c4c::hir::IntLiteral>(&alignof_expr->payload) : nullptr;
  expect_true(alignof_lit && alignof_lit->value == 8,
              "HIR builtin alignof no-owner compatibility should use tag_text_id, not rendered tag");
}

void test_codegen_aggregate_layout_complete_owner_miss_rejects_indexed_stale_tag() {
  c4c::hir::Module module;

  c4c::Node record_node{};
  record_node.kind = c4c::NK_STRUCT_DEF;
  record_node.name = "MissingCodegenLayout";
  record_node.unqualified_name = "MissingCodegenLayout";
  record_node.unqualified_text_id =
      module.link_name_texts->intern("MissingCodegenLayout");
  record_node.namespace_context_id = 74;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "IndexedStaleCodegenLayout";
  stale_def.tag_text_id =
      module.link_name_texts->intern("IndexedStaleCodegenLayout");
  stale_def.ns_qual.context_id = record_node.namespace_context_id;
  stale_def.size_bytes = 64;
  stale_def.align_bytes = 16;
  module.index_struct_def_owner(c4c::hir::make_hir_record_owner_key(stale_def),
                                stale_def.tag, true);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "IndexedStaleCodegenLayout", 0);
  query.tag_text_id = record_node.unqualified_text_id;
  query.namespace_context_id = record_node.namespace_context_id;
  query.record_def = &record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  const c4c::hir::HirStructDef* layout =
      c4c::codegen::llvm_helpers::find_typespec_aggregate_layout(module, query);
  expect_true(!layout,
              "codegen aggregate layout lookup must reject indexed rendered compatibility after complete owner miss");
}

void test_codegen_aggregate_layout_complete_typespec_owner_miss_rejects_stale_tag() {
  c4c::hir::Module module;

  const c4c::TextId missing_owner_text =
      module.link_name_texts->intern("MissingTypespecLayout");

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "IndexedStaleTypespecLayout";
  stale_def.tag_text_id =
      module.link_name_texts->intern("IndexedStaleTypespecLayout");
  stale_def.ns_qual.context_id = 75;
  stale_def.size_bytes = 128;
  stale_def.align_bytes = 32;
  module.index_struct_def_owner(c4c::hir::make_hir_record_owner_key(stale_def),
                                stale_def.tag, true);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "IndexedStaleTypespecLayout", 0);
  query.tag_text_id = missing_owner_text;
  query.namespace_context_id = stale_def.ns_qual.context_id;
  query.array_size = -1;
  query.inner_rank = -1;

  const c4c::hir::HirStructDef* layout =
      c4c::codegen::llvm_helpers::find_typespec_aggregate_layout(module, query);
  expect_true(!layout,
              "codegen aggregate layout lookup must reject rendered compatibility after complete TypeSpec owner miss");
}

void test_builtin_query_type_uses_template_param_text_id_binding() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node record_node{};
  record_node.kind = c4c::NK_STRUCT_DEF;
  record_node.name = "ResolvedBuiltinQuery";
  record_node.unqualified_name = "ResolvedBuiltinQuery";
  record_node.unqualified_text_id =
      module.link_name_texts->intern("ResolvedBuiltinQuery");
  record_node.namespace_context_id = 51;

  c4c::TypeSpec concrete{};
  concrete.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(concrete, "RenderedBindingPayload", 0);
  concrete.tag_text_id = record_node.unqualified_text_id;
  concrete.namespace_context_id = record_node.namespace_context_id;
  concrete.record_def = &record_node;
  concrete.ptr_level = 1;
  concrete.array_size = -1;
  concrete.inner_rank = -1;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.tpl_bindings["T"] = concrete;

  c4c::TypeSpec target{};
  target.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(target, "StaleRenderedTemplateParam", 0);
  target.template_param_text_id = module.link_name_texts->intern("T");
  target.ptr_level = 2;
  target.array_size = -1;
  target.inner_rank = -1;

  const c4c::TypeSpec resolved =
      lowerer.resolve_builtin_query_type(&ctx, target);
  expect_true(resolved.base == c4c::TB_STRUCT,
              "builtin query type should resolve through template parameter TextId");
  expect_true(resolved.record_def == &record_node &&
                  resolved.tag_text_id == record_node.unqualified_text_id,
              "builtin query type should copy structured binding metadata");
  expect_true(resolved.ptr_level == 3,
              "builtin query type should preserve existing pointer composition");
}

void test_builtin_query_type_uses_text_binding_when_module_text_missing() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec concrete{};
  concrete.base = c4c::TB_LONG;
  concrete.array_size = -1;
  concrete.inner_rank = -1;

  const c4c::TextId parser_only_text_id = 4096;
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.tpl_bindings["RenderedTemplateParam"] = concrete;
  ctx.tpl_bindings_by_text[parser_only_text_id] = concrete;

  c4c::TypeSpec target{};
  target.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(target, "StaleRenderedTemplateParam", 0);
  target.tag_text_id = parser_only_text_id;
  target.template_param_text_id = parser_only_text_id;
  target.array_size = -1;
  target.inner_rank = -1;

  const c4c::TypeSpec resolved =
      lowerer.resolve_builtin_query_type(&ctx, target);
  expect_true(resolved.base == c4c::TB_LONG,
              "builtin query type should use TextId bindings when module text lookup misses");
}

void test_builtin_query_type_structured_miss_rejects_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node record_node{};
  record_node.kind = c4c::NK_STRUCT_DEF;
  record_node.name = "WrongBuiltinQuery";
  record_node.unqualified_name = "WrongBuiltinQuery";
  record_node.unqualified_text_id = module.link_name_texts->intern("WrongBuiltinQuery");
  record_node.namespace_context_id = 52;

  c4c::TypeSpec concrete{};
  concrete.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(concrete, "StaleRenderedTemplateParam", 0);
  concrete.tag_text_id = record_node.unqualified_text_id;
  concrete.namespace_context_id = record_node.namespace_context_id;
  concrete.record_def = &record_node;
  concrete.array_size = -1;
  concrete.inner_rank = -1;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.tpl_bindings["StaleRenderedTemplateParam"] = concrete;

  c4c::TypeSpec target{};
  target.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(target, "StaleRenderedTemplateParam", 0);
  target.template_param_text_id = module.link_name_texts->intern("T");
  target.array_size = -1;
  target.inner_rank = -1;

  const c4c::TypeSpec resolved =
      lowerer.resolve_builtin_query_type(&ctx, target);
  expect_true(resolved.base == c4c::TB_TYPEDEF && resolved.record_def == nullptr,
              "builtin query type should not fall back to rendered tag after structured miss");
}

void test_builtin_query_type_no_metadata_keeps_compatibility_shape() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec concrete{};
  concrete.base = c4c::TB_INT;
  concrete.array_size = -1;
  concrete.inner_rank = -1;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.tpl_bindings["LegacyRenderedParam"] = concrete;

  c4c::TypeSpec target{};
  target.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(target, "LegacyRenderedParam", 0);
  target.array_size = -1;
  target.inner_rank = -1;

  const c4c::TypeSpec resolved =
      lowerer.resolve_builtin_query_type(&ctx, target);
  expect_true(resolved.base == c4c::TB_TYPEDEF,
              "builtin query type should not use rendered tag as semantic authority without metadata");
}

void test_lvalue_cast_uses_template_param_text_id_binding() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec text_bound{};
  text_bound.base = c4c::TB_INT;
  text_bound.is_lvalue_ref = true;
  text_bound.array_size = -1;
  text_bound.inner_rank = -1;

  c4c::TypeSpec stale_tag_bound{};
  stale_tag_bound.base = c4c::TB_INT;
  stale_tag_bound.array_size = -1;
  stale_tag_bound.inner_rank = -1;

  const c4c::TextId param_text_id = module.link_name_texts->intern("T");
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.tpl_bindings_by_text[param_text_id] = text_bound;
  ctx.tpl_bindings["StaleRenderedTemplateParam"] = stale_tag_bound;

  c4c::Node cast_node{};
  cast_node.kind = c4c::NK_CAST;
  cast_node.type.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(cast_node.type, "StaleRenderedTemplateParam", 0);
  cast_node.type.template_param_text_id = param_text_id;
  cast_node.type.array_size = -1;
  cast_node.type.inner_rank = -1;

  expect_true(lowerer.is_ast_lvalue(&cast_node, &ctx),
              "lvalue cast should prefer template_param_text_id binding over stale rendered tag");
}

void test_lvalue_cast_structured_miss_rejects_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec stale_tag_bound{};
  stale_tag_bound.base = c4c::TB_INT;
  stale_tag_bound.is_lvalue_ref = true;
  stale_tag_bound.array_size = -1;
  stale_tag_bound.inner_rank = -1;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.tpl_bindings["StaleRenderedTemplateParam"] = stale_tag_bound;

  c4c::Node cast_node{};
  cast_node.kind = c4c::NK_CAST;
  cast_node.type.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(cast_node.type, "StaleRenderedTemplateParam", 0);
  cast_node.type.template_param_text_id = module.link_name_texts->intern("T");
  cast_node.type.array_size = -1;
  cast_node.type.inner_rank = -1;

  expect_true(!lowerer.is_ast_lvalue(&cast_node, &ctx),
              "lvalue cast should reject stale rendered tag after structured binding miss");
}

c4c::hir::Param make_param_with_type(const char* name, c4c::TypeBase base) {
  c4c::hir::Param param{};
  param.name = name;
  param.type.spec.base = base;
  param.type.spec.array_size = -1;
  param.type.spec.inner_rank = -1;
  return param;
}

void test_var_expr_param_lookup_prefers_param_text_id_over_rendered_name() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId value_text_id = module.link_name_texts->intern("value");
  c4c::hir::Function fn;
  fn.params.push_back(make_param_with_type("value", c4c::TB_INT));
  fn.params.push_back(make_param_with_type("value", c4c::TB_LONG));

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.fn = &fn;
  ctx.params["value"] = 0;
  ctx.param_indices_by_text_id[value_text_id] = 1;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id = value_text_id;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && decl_ref->param_index &&
                  *decl_ref->param_index == 1,
              "source parameter value lookup should prefer TextId/index authority over rendered name");
}

void test_generic_type_param_lookup_prefers_param_text_id_over_rendered_name() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId value_text_id = module.link_name_texts->intern("value");
  c4c::hir::Function fn;
  fn.params.push_back(make_param_with_type("value", c4c::TB_INT));
  fn.params.push_back(make_param_with_type("value", c4c::TB_LONG));

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.fn = &fn;
  ctx.params["value"] = 0;
  ctx.param_indices_by_text_id[value_text_id] = 1;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id = value_text_id;

  const c4c::TypeSpec inferred =
      lowerer.infer_generic_ctrl_type(&ctx, &ref);
  expect_true(inferred.base == c4c::TB_LONG,
              "generic source parameter type inference should prefer TextId/index authority over rendered name");
}

void test_param_lookup_text_miss_rejects_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Function fn;
  fn.params.push_back(make_param_with_type("value", c4c::TB_INT));

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.fn = &fn;
  ctx.params["value"] = 0;

  c4c::TypeSpec fallback_ts{};
  fallback_ts.base = c4c::TB_CHAR;
  fallback_ts.array_size = -1;
  fallback_ts.inner_rank = -1;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id = module.link_name_texts->intern("different_source_param");
  ref.type = fallback_ts;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && !decl_ref->param_index,
              "complete source parameter TextId miss should not reopen rendered value lookup fallback");

  const c4c::TypeSpec inferred =
      lowerer.infer_generic_ctrl_type(&ctx, &ref);
  expect_true(inferred.base == c4c::TB_CHAR,
              "complete source parameter TextId miss should not reopen rendered type lookup fallback");
}

void test_param_lookup_no_metadata_uses_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Function fn;
  fn.params.push_back(make_param_with_type("value", c4c::TB_INT));

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.fn = &fn;
  ctx.params["value"] = 0;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id = c4c::kInvalidText;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && decl_ref->param_index &&
                  *decl_ref->param_index == 0,
              "no-metadata parameter value lookup should keep rendered compatibility fallback");

  const c4c::TypeSpec inferred =
      lowerer.infer_generic_ctrl_type(&ctx, &ref);
  expect_true(inferred.base == c4c::TB_INT,
              "no-metadata parameter type inference should keep rendered compatibility fallback");
}

void test_param_lookup_marked_generated_text_id_uses_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId generated_this_text_id =
      module.link_name_texts->intern("generated_this_source_carrier");
  c4c::hir::Function fn;
  fn.params.push_back(make_param_with_type("this", c4c::TB_STRUCT));

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.fn = &fn;
  ctx.params["this"] = 0;
  ctx.rendered_compat_param_names.insert("this");

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "this";
  ref.unqualified_name = "this";
  ref.unqualified_text_id = generated_this_text_id;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && decl_ref->param_index &&
                  *decl_ref->param_index == 0,
              "marked generated parameter value lookup should keep rendered compatibility fallback despite a valid TextId");

  const c4c::TypeSpec inferred =
      lowerer.infer_generic_ctrl_type(&ctx, &ref);
  expect_true(inferred.base == c4c::TB_STRUCT,
              "marked generated parameter type inference should keep rendered compatibility fallback despite a valid TextId");
}

void test_local_lookup_prefers_local_text_id_over_rendered_name() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId value_text_id = module.link_name_texts->intern("value");
  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  c4c::TypeSpec long_ts = int_ts;
  long_ts.base = c4c::TB_LONG;

  const c4c::hir::LocalId rendered_local{3};
  const c4c::hir::LocalId source_local{4};
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.locals["value"] = rendered_local;
  ctx.local_ids_by_text_id[value_text_id] = source_local;
  ctx.local_types.insert(rendered_local, int_ts);
  ctx.local_types.insert(source_local, long_ts);

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id = value_text_id;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && decl_ref->local &&
                  decl_ref->local->value == source_local.value,
              "source local value lookup should prefer TextId/LocalId authority over rendered name");

  const c4c::TypeSpec inferred =
      lowerer.infer_generic_ctrl_type(&ctx, &ref);
  expect_true(inferred.base == c4c::TB_LONG,
              "generic source local type inference should prefer TextId/LocalId authority over rendered name");
}

void test_local_lookup_text_miss_rejects_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  c4c::TypeSpec fallback_ts = int_ts;
  fallback_ts.base = c4c::TB_CHAR;

  const c4c::hir::LocalId rendered_local{5};
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.locals["value"] = rendered_local;
  ctx.local_types.insert(rendered_local, int_ts);

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id =
      module.link_name_texts->intern("different_source_local");
  ref.type = fallback_ts;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && !decl_ref->local,
              "complete source local TextId miss should not reopen rendered value lookup fallback");

  const c4c::TypeSpec inferred =
      lowerer.infer_generic_ctrl_type(&ctx, &ref);
  expect_true(inferred.base == c4c::TB_CHAR,
              "complete source local TextId miss should not reopen rendered type lookup fallback");
}

void test_local_lookup_no_metadata_and_marked_generated_use_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  const c4c::hir::LocalId rendered_local{6};

  c4c::hir::Lowerer::FunctionCtx no_metadata_ctx;
  no_metadata_ctx.locals["value"] = rendered_local;
  no_metadata_ctx.local_types.insert(rendered_local, int_ts);

  c4c::Node no_metadata_ref{};
  no_metadata_ref.kind = c4c::NK_VAR;
  no_metadata_ref.name = "value";
  no_metadata_ref.unqualified_name = "value";
  no_metadata_ref.unqualified_text_id = c4c::kInvalidText;

  const c4c::hir::ExprId no_metadata_expr_id =
      lowerer.lower_var_expr(&no_metadata_ctx, &no_metadata_ref);
  const c4c::hir::Expr* no_metadata_expr =
      module.find_expr(no_metadata_expr_id);
  const auto* no_metadata_decl_ref =
      no_metadata_expr
          ? std::get_if<c4c::hir::DeclRef>(&no_metadata_expr->payload)
          : nullptr;
  expect_true(no_metadata_decl_ref && no_metadata_decl_ref->local &&
                  no_metadata_decl_ref->local->value == rendered_local.value,
              "no-metadata local value lookup should keep rendered compatibility fallback");
  expect_true(lowerer.infer_generic_ctrl_type(&no_metadata_ctx,
                                              &no_metadata_ref).base == c4c::TB_INT,
              "no-metadata local type inference should keep rendered compatibility fallback");

  c4c::hir::Lowerer::FunctionCtx generated_ctx;
  generated_ctx.locals["__generated_local"] = rendered_local;
  generated_ctx.rendered_compat_local_names.insert("__generated_local");
  generated_ctx.local_types.insert(rendered_local, int_ts);

  c4c::Node generated_ref{};
  generated_ref.kind = c4c::NK_VAR;
  generated_ref.name = "__generated_local";
  generated_ref.unqualified_name = "__generated_local";
  generated_ref.unqualified_text_id =
      module.link_name_texts->intern("generated_source_carrier");

  const c4c::hir::ExprId generated_expr_id =
      lowerer.lower_var_expr(&generated_ctx, &generated_ref);
  const c4c::hir::Expr* generated_expr = module.find_expr(generated_expr_id);
  const auto* generated_decl_ref =
      generated_expr
          ? std::get_if<c4c::hir::DeclRef>(&generated_expr->payload)
          : nullptr;
  expect_true(generated_decl_ref && generated_decl_ref->local &&
                  generated_decl_ref->local->value == rendered_local.value,
              "marked generated local value lookup should keep rendered compatibility fallback despite a valid TextId");
  expect_true(lowerer.infer_generic_ctrl_type(&generated_ctx,
                                              &generated_ref).base == c4c::TB_INT,
              "marked generated local type inference should keep rendered compatibility fallback despite a valid TextId");
}

c4c::hir::FnPtrSig make_returning_fn_ptr_sig(c4c::TypeBase return_base) {
  c4c::hir::FnPtrSig sig{};
  sig.return_type.spec.base = return_base;
  sig.return_type.spec.array_size = -1;
  sig.return_type.spec.inner_rank = -1;
  return sig;
}

void test_param_fn_ptr_sig_lookup_prefers_param_text_id_over_rendered_name() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId callback_text_id =
      module.link_name_texts->intern("callback");
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.param_indices_by_text_id[callback_text_id] = 0;
  ctx.param_fn_ptr_sigs_by_index[0] =
      make_returning_fn_ptr_sig(c4c::TB_LONG);
  ctx.param_fn_ptr_sigs["callback"] =
      make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id = callback_text_id;

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(ret.has_value() && ret->base == c4c::TB_LONG,
              "param fn-ptr return inference should prefer parameter TextId/index authority over rendered name");
}

void test_param_fn_ptr_sig_text_miss_rejects_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.param_fn_ptr_sigs["callback"] =
      make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id =
      module.link_name_texts->intern("different_source_param");

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(!ret.has_value(),
              "complete param TextId miss should not reopen rendered param fn-ptr fallback");
}

void test_param_fn_ptr_sig_hit_without_signature_rejects_all_fallbacks() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId callback_text_id = texts.intern("callback");
  add_global(module, c4c::hir::GlobalId{120}, "callback", callback_text_id);
  module.globals.back().fn_ptr_sig =
      make_returning_fn_ptr_sig(c4c::TB_SHORT);
  add_global(module, c4c::hir::GlobalId{121}, "global_callback",
             texts.intern("global_callback"));
  module.globals.back().fn_ptr_sig =
      make_returning_fn_ptr_sig(c4c::TB_CHAR);
  add_function(module, c4c::hir::FunctionId{120}, "callback",
               callback_text_id, c4c::kInvalidLinkName, {}, c4c::TB_LONG);

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.param_indices_by_text_id[callback_text_id] = 0;
  ctx.param_fn_ptr_sigs["callback"] =
      make_returning_fn_ptr_sig(c4c::TB_INT);
  ctx.static_globals["callback"] = c4c::hir::GlobalId{120};
  ctx.static_global_ids_by_text_id[callback_text_id] =
      c4c::hir::GlobalId{121};

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id = callback_text_id;

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(!ret.has_value(),
              "complete param TextId hit without fn-ptr signature should not fall through to rendered, static, global, or function lookup");
}

void test_param_fn_ptr_sig_no_metadata_uses_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.param_fn_ptr_sigs["callback"] =
      make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id = c4c::kInvalidText;

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(ret.has_value() && ret->base == c4c::TB_INT,
              "no-metadata param fn-ptr calls should keep the rendered compatibility fallback");
}

void test_local_fn_ptr_sig_lookup_prefers_local_text_id_over_rendered_name() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId callback_text_id =
      module.link_name_texts->intern("callback");
  const c4c::hir::LocalId callback_local{7};
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.local_ids_by_text_id[callback_text_id] = callback_local;
  ctx.local_fn_ptr_sigs_by_id.insert(
      callback_local, make_returning_fn_ptr_sig(c4c::TB_LONG));
  ctx.local_fn_ptr_sigs["callback"] =
      make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id = callback_text_id;

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(ret.has_value() && ret->base == c4c::TB_LONG,
              "local fn-ptr return inference should prefer local TextId/LocalId authority over rendered name");
}

void test_local_fn_ptr_sig_lookup_shadows_same_spelled_param_sig() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId callback_text_id =
      module.link_name_texts->intern("callback");
  const c4c::hir::LocalId callback_local{7};
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.local_ids_by_text_id[callback_text_id] = callback_local;
  ctx.local_fn_ptr_sigs_by_id.insert(
      callback_local, make_returning_fn_ptr_sig(c4c::TB_LONG));
  ctx.param_indices_by_text_id[callback_text_id] = 0;
  ctx.param_fn_ptr_sigs_by_index[0] =
      make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id = callback_text_id;

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(ret.has_value() && ret->base == c4c::TB_LONG,
              "active local fn-ptr signature should shadow a same-spelled parameter signature");
}

void test_local_fn_ptr_sig_complete_local_shadow_blocks_param_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId callback_text_id =
      module.link_name_texts->intern("callback");
  const c4c::hir::LocalId callback_local{7};
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.local_ids_by_text_id[callback_text_id] = callback_local;
  ctx.param_indices_by_text_id[callback_text_id] = 0;
  ctx.param_fn_ptr_sigs_by_index[0] =
      make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id = callback_text_id;

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(!ret.has_value(),
              "complete local shadow without a fn-ptr signature should not fall through to a same-spelled parameter signature");
}

void test_local_fn_ptr_sig_text_miss_rejects_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.local_fn_ptr_sigs["callback"] =
      make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id =
      module.link_name_texts->intern("different_source_local");

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(!ret.has_value(),
              "complete local TextId miss should not reopen rendered local fn-ptr fallback");
}

void test_local_fn_ptr_sig_no_metadata_uses_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.local_fn_ptr_sigs["callback"] =
      make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::Node callee{};
  callee.kind = c4c::NK_VAR;
  callee.name = "callback";
  callee.unqualified_name = "callback";
  callee.unqualified_text_id = c4c::kInvalidText;

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &callee);
  expect_true(ret.has_value() && ret->base == c4c::TB_INT,
              "no-metadata local fn-ptr calls should keep the rendered compatibility fallback");
}

void test_callable_zero_sized_return_structured_miss_rejects_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node missing_record{};
  missing_record.kind = c4c::NK_STRUCT_DEF;
  missing_record.name = "MissingCallableReturn";
  missing_record.unqualified_name = "MissingCallableReturn";
  missing_record.unqualified_text_id =
      module.link_name_texts->intern("MissingCallableReturn");
  missing_record.namespace_context_id = 61;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleCallableZeroReturn";
  stale_def.tag_text_id =
      module.link_name_texts->intern("StaleCallableZeroReturn");
  stale_def.ns_qual.context_id = missing_record.namespace_context_id;
  stale_def.size_bytes = 0;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node literal{};
  literal.kind = c4c::NK_INT_LIT;
  literal.ival = 7;

  c4c::Node ret{};
  ret.kind = c4c::NK_RETURN;
  ret.left = &literal;

  c4c::Node fn{};
  fn.kind = c4c::NK_FUNCTION;
  fn.name = "callable_zero_return_miss";
  fn.unqualified_name = "callable_zero_return_miss";
  fn.unqualified_text_id =
      module.link_name_texts->intern("callable_zero_return_miss");
  fn.type.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(fn.type, "StaleCallableZeroReturn", 0);
  fn.type.tag_text_id = missing_record.unqualified_text_id;
  fn.type.namespace_context_id = missing_record.namespace_context_id;
  fn.type.record_def = &missing_record;
  fn.type.array_size = -1;
  fn.type.inner_rank = -1;
  fn.body = &ret;

  lowerer.lower_function(&fn);

  expect_true(!module.functions.empty(),
              "callable zero-sized return fixture should lower a function");
  expect_true(module.functions[0].return_type.spec.base == c4c::TB_STRUCT,
              "callable zero-sized return normalization must not use stale rendered tag after structured owner miss");
  expect_true(module.functions[0].return_type.spec.record_def == &missing_record,
              "callable zero-sized return should preserve structured owner metadata after miss");
}

void test_layout_type_lookup_canonicalizes_record_def_parser_owner_ids() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId parser_ns_text_id = 1;
  const c4c::TextId parser_owner_text_id = 2;
  const c4c::TextId collision_ns_text =
      module.link_name_texts->intern("CollisionLayoutNs");
  const c4c::TextId collision_owner_text =
      module.link_name_texts->intern("CollisionLayoutOwner");
  expect_true(parser_ns_text_id == collision_ns_text &&
                  parser_owner_text_id == collision_owner_text,
              "layout fixture should force parser-owned TextId collisions");
  const c4c::TextId real_ns_text =
      module.link_name_texts->intern("RealLayoutNs");
  const c4c::TextId real_owner_text =
      module.link_name_texts->intern("RealCanonicalLayoutOwner");

  const char* qualifier_segments[] = {"RealLayoutNs"};
  c4c::TextId qualifier_text_ids[] = {parser_ns_text_id};
  c4c::Node record_node{};
  record_node.kind = c4c::NK_STRUCT_DEF;
  record_node.name = "RealCanonicalLayoutOwner";
  record_node.unqualified_name = "RealCanonicalLayoutOwner";
  record_node.unqualified_text_id = parser_owner_text_id;
  record_node.namespace_context_id = 71;
  record_node.qualifier_segments = qualifier_segments;
  record_node.qualifier_text_ids = qualifier_text_ids;
  record_node.n_qualifier_segments = 1;

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = record_node.namespace_context_id;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real_owner_text);
  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealCanonicalLayoutOwner";
  real_def.tag_text_id = real_owner_text;
  real_def.ns_qual = real_ns;
  real_def.size_bytes = 32;
  module.index_struct_def_owner(real_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::NamespaceQualifier collision_ns;
  collision_ns.context_id = record_node.namespace_context_id;
  collision_ns.segment_text_ids.push_back(collision_ns_text);
  const c4c::hir::HirRecordOwnerKey collision_key =
      c4c::hir::make_hir_record_owner_key(collision_ns,
                                          collision_owner_text);
  c4c::hir::HirStructDef collision_def;
  collision_def.tag = "CollisionCanonicalLayoutOwner";
  collision_def.tag_text_id = collision_owner_text;
  collision_def.ns_qual = collision_ns;
  collision_def.size_bytes = 4;
  module.index_struct_def_owner(collision_key, collision_def.tag, true);
  module.struct_defs[collision_def.tag] = collision_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  query.tag_text_id = parser_owner_text_id;
  query.namespace_context_id = record_node.namespace_context_id;
  query.qualifier_segments = qualifier_segments;
  query.qualifier_text_ids = qualifier_text_ids;
  query.n_qualifier_segments = 1;
  query.record_def = &record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  const c4c::hir::HirStructDef* layout =
      lowerer.find_struct_def_for_layout_type(query);
  expect_true(layout && layout->tag == "RealCanonicalLayoutOwner" &&
                  layout->size_bytes == 32,
              "layout TypeSpec lookup should canonicalize record_def parser-owned owner ids through link_name_texts");
}

void test_callable_zero_return_canonicalizes_record_def_parser_owner_ids() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId parser_ns_text_id = 1;
  const c4c::TextId parser_owner_text_id = 2;
  const c4c::TextId collision_ns_text =
      module.link_name_texts->intern("CollisionCallableNs");
  const c4c::TextId collision_owner_text =
      module.link_name_texts->intern("CollisionCallableOwner");
  expect_true(parser_ns_text_id == collision_ns_text &&
                  parser_owner_text_id == collision_owner_text,
              "callable fixture should force parser-owned TextId collisions");
  const c4c::TextId real_ns_text =
      module.link_name_texts->intern("RealCallableNs");
  const c4c::TextId real_owner_text =
      module.link_name_texts->intern("RealCallableReturnOwner");

  const char* qualifier_segments[] = {"RealCallableNs"};
  c4c::TextId qualifier_text_ids[] = {parser_ns_text_id};
  c4c::Node record_node{};
  record_node.kind = c4c::NK_STRUCT_DEF;
  record_node.name = "RealCallableReturnOwner";
  record_node.unqualified_name = "RealCallableReturnOwner";
  record_node.unqualified_text_id = parser_owner_text_id;
  record_node.namespace_context_id = 72;
  record_node.qualifier_segments = qualifier_segments;
  record_node.qualifier_text_ids = qualifier_text_ids;
  record_node.n_qualifier_segments = 1;

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = record_node.namespace_context_id;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real_owner_text);
  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealCallableReturnOwner";
  real_def.tag_text_id = real_owner_text;
  real_def.ns_qual = real_ns;
  real_def.size_bytes = 0;
  module.index_struct_def_owner(real_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::NamespaceQualifier collision_ns;
  collision_ns.context_id = record_node.namespace_context_id;
  collision_ns.segment_text_ids.push_back(collision_ns_text);
  const c4c::hir::HirRecordOwnerKey collision_key =
      c4c::hir::make_hir_record_owner_key(collision_ns,
                                          collision_owner_text);
  c4c::hir::HirStructDef collision_def;
  collision_def.tag = "CollisionCallableReturnOwner";
  collision_def.tag_text_id = collision_owner_text;
  collision_def.ns_qual = collision_ns;
  collision_def.size_bytes = 8;
  module.index_struct_def_owner(collision_key, collision_def.tag, true);
  module.struct_defs[collision_def.tag] = collision_def;

  c4c::Node literal{};
  literal.kind = c4c::NK_INT_LIT;
  literal.ival = 17;

  c4c::Node ret{};
  ret.kind = c4c::NK_RETURN;
  ret.left = &literal;

  c4c::Node fn{};
  fn.kind = c4c::NK_FUNCTION;
  fn.name = "callable_zero_return_canonical";
  fn.unqualified_name = "callable_zero_return_canonical";
  fn.unqualified_text_id =
      module.link_name_texts->intern("callable_zero_return_canonical");
  fn.type.base = c4c::TB_STRUCT;
  fn.type.tag_text_id = parser_owner_text_id;
  fn.type.namespace_context_id = record_node.namespace_context_id;
  fn.type.qualifier_segments = qualifier_segments;
  fn.type.qualifier_text_ids = qualifier_text_ids;
  fn.type.n_qualifier_segments = 1;
  fn.type.record_def = &record_node;
  fn.type.array_size = -1;
  fn.type.inner_rank = -1;
  fn.body = &ret;

  lowerer.lower_function(&fn);

  expect_true(!module.functions.empty(),
              "canonical callable return fixture should lower a function");
  expect_true(module.functions[0].return_type.spec.base == c4c::TB_INT,
              "callable zero-sized return lookup should canonicalize record_def parser-owned owner ids before the structured miss decision");
}

void test_typespec_aggregate_owner_key_canonicalizes_record_def_parser_owner_ids() {
  c4c::hir::Module module;

  const c4c::TextId parser_ns_text_id = 1;
  const c4c::TextId parser_owner_text_id = 2;
  const c4c::TextId collision_ns_text =
      module.link_name_texts->intern("CollisionCodegenNs");
  const c4c::TextId collision_owner_text =
      module.link_name_texts->intern("CollisionCodegenOwner");
  expect_true(parser_ns_text_id == collision_ns_text &&
                  parser_owner_text_id == collision_owner_text,
              "codegen fixture should force parser-owned TextId collisions");
  const c4c::TextId real_ns_text =
      module.link_name_texts->intern("RealCodegenNs");
  const c4c::TextId real_owner_text =
      module.link_name_texts->intern("RealCodegenOwner");

  const char* qualifier_segments[] = {"RealCodegenNs"};
  c4c::TextId qualifier_text_ids[] = {parser_ns_text_id};
  c4c::Node record_node{};
  record_node.kind = c4c::NK_STRUCT_DEF;
  record_node.name = "RealCodegenOwner";
  record_node.unqualified_name = "RealCodegenOwner";
  record_node.unqualified_text_id = parser_owner_text_id;
  record_node.namespace_context_id = 73;
  record_node.qualifier_segments = qualifier_segments;
  record_node.qualifier_text_ids = qualifier_text_ids;
  record_node.n_qualifier_segments = 1;

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = record_node.namespace_context_id;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real_owner_text);

  c4c::hir::NamespaceQualifier collision_ns;
  collision_ns.context_id = record_node.namespace_context_id;
  collision_ns.segment_text_ids.push_back(collision_ns_text);
  const c4c::hir::HirRecordOwnerKey collision_key =
      c4c::hir::make_hir_record_owner_key(collision_ns,
                                          collision_owner_text);
  module.index_struct_def_owner(collision_key, "RealCodegenOwner", true);
  module.index_struct_def_owner(real_key, "RealCodegenOwner", true);

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  query.tag_text_id = parser_owner_text_id;
  query.namespace_context_id = record_node.namespace_context_id;
  query.qualifier_segments = qualifier_segments;
  query.qualifier_text_ids = qualifier_text_ids;
  query.n_qualifier_segments = 1;
  query.record_def = &record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      c4c::codegen::llvm_helpers::typespec_aggregate_owner_key(query, module);
  expect_true(owner_key.has_value() && *owner_key == real_key,
              "codegen TypeSpec owner-key lookup should prefer AST-node canonicalization over parser-owned id collisions");
}

void test_global_aggregate_init_normalization_prefers_hir_owner_key_over_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("RealAggregateLayout");
  record_node->unqualified_name = arena.strdup("RealAggregateLayout");
  record_node->unqualified_text_id =
      module.link_name_texts->intern("RealAggregateLayout");
  record_node->namespace_context_id = 31;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;

  auto make_field = [&](const char* name, int llvm_idx) {
    c4c::hir::HirStructField field;
    field.name = name;
    field.field_text_id = module.link_name_texts->intern(name);
    field.elem_type = int_ts;
    field.llvm_idx = llvm_idx;
    field.size_bytes = 4;
    field.align_bytes = 4;
    return field;
  };

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealAggregateLayout";
  real_def.tag_text_id = record_node->unqualified_text_id;
  real_def.ns_qual.context_id = record_node->namespace_context_id;
  real_def.size_bytes = 8;
  real_def.align_bytes = 4;
  real_def.fields.push_back(make_field("first", 0));
  real_def.fields.push_back(make_field("second", 1));
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(real_def);
  module.index_struct_def_owner(owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleAggregateLayout";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleAggregateLayout");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.size_bytes = 4;
  stale_def.align_bytes = 4;
  stale_def.fields.push_back(make_field("wrong", 0));
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec query{};
  query.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(query, "StaleAggregateLayout", 0);
  query.tag_text_id = record_node->unqualified_text_id;
  query.namespace_context_id = record_node->namespace_context_id;
  query.record_def = record_node;
  query.array_size = -1;
  query.inner_rank = -1;

  c4c::hir::InitList init;
  c4c::hir::InitListItem first;
  first.value = c4c::hir::InitScalar{c4c::hir::ExprId{0}};
  init.items.push_back(first);
  c4c::hir::InitListItem second;
  second.value = c4c::hir::InitScalar{c4c::hir::ExprId{1}};
  init.items.push_back(second);

  const c4c::hir::GlobalInit normalized =
      lowerer.normalize_global_init(query, c4c::hir::GlobalInit(init));
  const auto* normalized_list = std::get_if<c4c::hir::InitList>(&normalized);
  expect_true(normalized_list && normalized_list->items.size() == 2,
              "aggregate init normalization should use the structured owner field count");
  expect_true(normalized_list->items[0].field_designator &&
                  *normalized_list->items[0].field_designator == "first",
              "aggregate init normalization should map the first structured owner field");
  expect_true(normalized_list->items[1].field_designator &&
                  *normalized_list->items[1].field_designator == "second",
              "aggregate init normalization should map the second structured owner field");
}

void test_implicit_this_field_recovery_prefers_hir_owner_key_over_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  c4c::Arena arena;

  c4c::Node* record_node = arena.alloc_array<c4c::Node>(1);
  record_node->kind = c4c::NK_STRUCT_DEF;
  record_node->name = arena.strdup("RealThisLayout");
  record_node->unqualified_name = arena.strdup("RealThisLayout");
  record_node->unqualified_text_id = module.link_name_texts->intern("RealThisLayout");
  record_node->namespace_context_id = 41;
  lowerer.struct_def_nodes_["StaleThisLayout"] = record_node;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;

  const c4c::MemberSymbolId real_id =
      module.member_symbols.intern("RealThisLayout::field");
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleThisLayout::field");

  c4c::hir::HirStructField real_field;
  real_field.name = "field";
  real_field.field_text_id = module.link_name_texts->intern("field");
  real_field.elem_type = int_ts;
  real_field.member_symbol_id = real_id;

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealThisLayout";
  real_def.tag_text_id = record_node->unqualified_text_id;
  real_def.ns_qual.context_id = record_node->namespace_context_id;
  real_def.fields.push_back(real_field);
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(record_node);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for implicit-this recovery");
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructField stale_field = real_field;
  stale_field.member_symbol_id = stale_id;
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleThisLayout";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleThisLayout");
  stale_def.ns_qual.context_id = record_node->namespace_context_id;
  stale_def.fields.push_back(stale_field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* var = arena.alloc_array<c4c::Node>(1);
  var->kind = c4c::NK_VAR;
  var->name = arena.strdup("field");
  var->unqualified_name = arena.strdup("field");
  var->unqualified_text_id = real_field.field_text_id;
  var->type = int_ts;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.method_struct_tag = "StaleThisLayout";
  ctx.params["this"] = 0;

  const c4c::hir::ExprId field_id = lowerer.lower_var_expr(&ctx, var);
  const c4c::hir::Expr* field_expr = module.find_expr(field_id);
  const auto* member =
      field_expr ? std::get_if<c4c::hir::MemberExpr>(&field_expr->payload) : nullptr;
  expect_true(member != nullptr,
              "implicit-this field recovery should synthesize a member expression");
  expect_true(member->resolved_owner_tag == "RealThisLayout",
              "implicit-this field recovery should prefer structured owner layout");
  expect_true(member->member_symbol_id == real_id &&
                  member->member_symbol_id != stale_id,
              "implicit-this field recovery should not use stale rendered owner symbol");
}

void test_local_extern_global_lookup_prefers_structured_decl_over_stale_rendered_name() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("stale_extern_global");
  const c4c::TextId structured_text = texts.intern("structured_extern_global");
  c4c::hir::NamespaceQualifier local_ns;
  local_ns.context_id = 0;

  add_global(module, c4c::hir::GlobalId{50}, "stale_extern_global",
             stale_text, c4c::kInvalidLinkName, local_ns);
  add_global(module, c4c::hir::GlobalId{51}, "structured_extern_global",
             structured_text, c4c::kInvalidLinkName, local_ns);

  c4c::Node decl{};
  decl.kind = c4c::NK_DECL;
  decl.name = "stale_extern_global";
  decl.unqualified_name = "structured_extern_global";
  decl.unqualified_text_id = structured_text;
  decl.is_extern = true;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.locals["stale_extern_global"] = c4c::hir::LocalId{7};

  lowerer.lower_local_decl_stmt(ctx, &decl);

  const auto static_it = ctx.static_globals.find("stale_extern_global");
  expect_true(static_it != ctx.static_globals.end() &&
                  static_it->second.value == 51,
              "local extern lowering should prefer structured global metadata over stale rendered name");
  const auto structured_static_it =
      ctx.static_global_ids_by_text_id.find(structured_text);
  expect_true(structured_static_it != ctx.static_global_ids_by_text_id.end() &&
                  structured_static_it->second.value == 51,
              "local extern lowering should register the structured static/global bridge");
  expect_true(ctx.locals.find("stale_extern_global") == ctx.locals.end(),
              "local extern lowering should erase the shadowing local binding after global resolution");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "stale_extern_global", 51),
              "local extern structured global lookup should record structured authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Global,
                           "stale_extern_global", 51, 50),
              "local extern structured global lookup should record legacy rendered parity mismatch");
}

void test_local_extern_clears_stale_source_local_identity() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId bridge_text = texts.intern("bridge_extern");
  c4c::hir::NamespaceQualifier local_ns;
  local_ns.context_id = 0;
  add_global(module, c4c::hir::GlobalId{90}, "bridge_extern",
             bridge_text, c4c::kInvalidLinkName, local_ns);

  c4c::TypeSpec stale_ts{};
  stale_ts.base = c4c::TB_LONG;
  stale_ts.array_size = -1;
  stale_ts.inner_rank = -1;

  const c4c::hir::LocalId stale_local{12};
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.locals["bridge_extern"] = stale_local;
  ctx.local_ids_by_text_id[bridge_text] = stale_local;
  ctx.local_types.insert(stale_local, stale_ts);
  ctx.local_fn_ptr_sigs_by_id.insert(
      stale_local, make_returning_fn_ptr_sig(c4c::TB_LONG));
  ctx.local_fn_ptr_sigs["bridge_extern"] =
      make_returning_fn_ptr_sig(c4c::TB_LONG);

  c4c::Node decl{};
  decl.kind = c4c::NK_DECL;
  decl.name = "bridge_extern";
  decl.unqualified_name = "bridge_extern";
  decl.unqualified_text_id = bridge_text;
  decl.is_extern = true;

  lowerer.lower_local_decl_stmt(ctx, &decl);
  expect_true(ctx.local_ids_by_text_id.find(bridge_text) ==
                  ctx.local_ids_by_text_id.end(),
              "local extern bridge should clear stale source local identity");

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "bridge_extern";
  ref.unqualified_name = "bridge_extern";
  ref.unqualified_text_id = bridge_text;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && !decl_ref->local && decl_ref->global &&
                  decl_ref->global->value == 90,
              "local extern bridge should beat stale source local lookup");

  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &ref);
  expect_true(!ret.has_value(),
              "local extern bridge should fence stale source local function-pointer signature lookup");
}

void test_local_static_clears_stale_source_local_identity() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId bridge_text = texts.intern("bridge_static");
  c4c::TypeSpec stale_ts{};
  stale_ts.base = c4c::TB_CHAR;
  stale_ts.array_size = -1;
  stale_ts.inner_rank = -1;
  c4c::TypeSpec static_ts = stale_ts;
  static_ts.base = c4c::TB_LONG;

  c4c::hir::Function fn;
  fn.name = "owner";
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.fn = &fn;
  const c4c::hir::LocalId stale_local{13};
  ctx.locals["bridge_static"] = stale_local;
  ctx.local_ids_by_text_id[bridge_text] = stale_local;
  ctx.local_types.insert(stale_local, stale_ts);
  ctx.local_fn_ptr_sigs_by_id.insert(
      stale_local, make_returning_fn_ptr_sig(c4c::TB_CHAR));

  c4c::Node decl{};
  decl.kind = c4c::NK_DECL;
  decl.name = "bridge_static";
  decl.unqualified_name = "bridge_static";
  decl.unqualified_text_id = bridge_text;
  decl.is_static = true;
  decl.type = static_ts;

  lowerer.lower_local_decl_stmt(ctx, &decl);
  expect_true(ctx.local_ids_by_text_id.find(bridge_text) ==
                  ctx.local_ids_by_text_id.end(),
              "local static bridge should clear stale source local identity");

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "bridge_static";
  ref.unqualified_name = "bridge_static";
  ref.unqualified_text_id = bridge_text;

  const auto static_it = ctx.static_globals.find("bridge_static");
  expect_true(static_it != ctx.static_globals.end(),
              "local static lowering should register the static/global bridge");
  const auto structured_static_it =
      ctx.static_global_ids_by_text_id.find(bridge_text);
  expect_true(structured_static_it != ctx.static_global_ids_by_text_id.end() &&
                  structured_static_it->second.value == static_it->second.value,
              "local static lowering should register the source TextId static/global bridge");
  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && !decl_ref->local && decl_ref->global &&
                  decl_ref->global->value == static_it->second.value,
              "local static bridge should beat stale source local lookup");

  const c4c::TypeSpec inferred =
      lowerer.infer_generic_ctrl_type(&ctx, &ref);
  expect_true(inferred.base == c4c::TB_LONG,
              "local static bridge type should beat stale source local type lookup");
}

void test_static_global_bridge_source_text_id_beats_rendered_map() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId structured_text = texts.intern("structured_static_bridge");
  const c4c::TextId stale_text = texts.intern("stale_static_bridge");
  add_global(module, c4c::hir::GlobalId{100}, "stale_static_bridge",
             stale_text);
  module.globals.back().type.spec.base = c4c::TB_CHAR;
  module.globals.back().fn_ptr_sig = make_returning_fn_ptr_sig(c4c::TB_CHAR);
  add_global(module, c4c::hir::GlobalId{101}, "structured_static_bridge",
             structured_text);
  module.globals.back().type.spec.base = c4c::TB_LONG;
  module.globals.back().fn_ptr_sig = make_returning_fn_ptr_sig(c4c::TB_INT);

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.static_globals["stale_static_bridge"] = c4c::hir::GlobalId{100};
  ctx.static_global_ids_by_text_id[structured_text] = c4c::hir::GlobalId{101};

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "stale_static_bridge";
  ref.unqualified_name = "structured_static_bridge";
  ref.unqualified_text_id = structured_text;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && decl_ref->global &&
                  decl_ref->global->value == 101,
              "static/global bridge value lookup should prefer source TextId over rendered map");
  expect_true(lowerer.infer_generic_ctrl_type(&ctx, &ref).base == c4c::TB_LONG,
              "static/global bridge type lookup should prefer source TextId over rendered map");
  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &ref);
  expect_true(ret && ret->base == c4c::TB_INT,
              "static/global bridge function-pointer lookup should prefer source TextId over rendered map");
}

void test_static_global_bridge_text_miss_rejects_rendered_map() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.static_globals["stale_static_bridge"] = c4c::hir::GlobalId{100};

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "stale_static_bridge";
  ref.unqualified_name = "missing_static_bridge";
  ref.unqualified_text_id = texts.intern("missing_static_bridge");

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && !decl_ref->global,
              "complete static/global bridge miss should not use rendered map for value lookup");
  expect_true(lowerer.infer_generic_ctrl_type(&ctx, &ref).base != c4c::TB_LONG,
              "complete static/global bridge miss should not use rendered map for type lookup");
  expect_true(!lowerer.infer_call_result_type_from_callee(&ctx, &ref),
              "complete static/global bridge miss should not use rendered map for function-pointer lookup");
}

void test_static_global_bridge_rendered_compat_uses_rendered_map() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId generated_text = texts.intern("generated_static_carrier");
  add_global(module, c4c::hir::GlobalId{110}, "__generated_static",
             generated_text);
  module.globals.back().type.spec.base = c4c::TB_SHORT;
  module.globals.back().fn_ptr_sig = make_returning_fn_ptr_sig(c4c::TB_LONG);

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.static_globals["__generated_static"] = c4c::hir::GlobalId{110};
  ctx.rendered_compat_static_global_names.insert("__generated_static");

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "__generated_static";
  ref.unqualified_name = "__generated_static";
  ref.unqualified_text_id = generated_text;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(&ctx, &ref);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* decl_ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(decl_ref && decl_ref->global &&
                  decl_ref->global->value == 110,
              "explicit static/global rendered compatibility should use rendered map for value lookup");
  expect_true(lowerer.infer_generic_ctrl_type(&ctx, &ref).base == c4c::TB_SHORT,
              "explicit static/global rendered compatibility should use rendered map for type lookup");
  const std::optional<c4c::TypeSpec> ret =
      lowerer.infer_call_result_type_from_callee(&ctx, &ref);
  expect_true(ret && ret->base == c4c::TB_LONG,
              "explicit static/global rendered compatibility should use rendered map for function-pointer lookup");
}

void test_local_const_binding_insertion_populates_source_maps() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.is_const = true;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;

  c4c::Node init{};
  init.kind = c4c::NK_INT_LIT;
  init.ival = 37;
  init.type = int_ts;

  const c4c::TextId local_text = texts.intern("local_const");
  c4c::Node decl{};
  decl.kind = c4c::NK_DECL;
  decl.name = "local_const";
  decl.unqualified_name = "local_const";
  decl.unqualified_text_id = local_text;
  decl.type = int_ts;
  decl.init = &init;
  decl.is_constexpr = true;

  c4c::hir::Function fn;
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.fn = &fn;

  lowerer.lower_local_decl_stmt(ctx, &decl);

  const auto rendered_it = ctx.local_const_bindings.find("local_const");
  expect_true(rendered_it != ctx.local_const_bindings.end() &&
                  rendered_it->second == 37,
              "foldable local const insertion should retain rendered compatibility map");
  const auto text_it = ctx.local_const_bindings_by_text.find(local_text);
  expect_true(text_it != ctx.local_const_bindings_by_text.end() &&
                  text_it->second == 37,
              "foldable local const insertion should populate source TextId map");
  c4c::hir::ConstEvalStructuredNameKey key;
  key.base_text_id = local_text;
  const auto key_it = ctx.local_const_bindings_by_key.find(key);
  expect_true(key_it != ctx.local_const_bindings_by_key.end() &&
                  key_it->second == 37,
              "foldable local const insertion should populate source key map");
}

void test_local_const_binding_source_text_id_beats_rendered_map() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId structured_text = texts.intern("structured_local_const");
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.local_const_bindings["stale_local_const"] = 11;
  ctx.local_const_bindings_by_text[structured_text] = 29;
  c4c::hir::ConstEvalStructuredNameKey key;
  key.base_text_id = structured_text;
  ctx.local_const_bindings_by_key[key] = 29;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "stale_local_const";
  ref.unqualified_name = "structured_local_const";
  ref.unqualified_text_id = structured_text;

  c4c::hir::Lowerer::LowererConstEvalStructuredMaps structured_maps;
  c4c::hir::ConstEvalEnv env = lowerer.make_lowerer_consteval_env(
      structured_maps, &ctx.local_const_bindings,
      &ctx.local_const_bindings_by_text, &ctx.local_const_bindings_by_key);
  const c4c::hir::ConstEvalResult result =
      c4c::hir::evaluate_constant_expr(&ref, env);
  expect_true(result.ok() && result.as_int() == 29,
              "local const lookup should prefer source TextId/key over stale rendered map");
}

void test_local_const_binding_text_miss_rejects_rendered_map() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.local_const_bindings["stale_local_const"] = 11;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "stale_local_const";
  ref.unqualified_name = "missing_local_const";
  ref.unqualified_text_id = texts.intern("missing_local_const");

  c4c::hir::Lowerer::LowererConstEvalStructuredMaps structured_maps;
  c4c::hir::ConstEvalEnv env = lowerer.make_lowerer_consteval_env(
      structured_maps, &ctx.local_const_bindings,
      &ctx.local_const_bindings_by_text, &ctx.local_const_bindings_by_key);
  const c4c::hir::ConstEvalResult result =
      c4c::hir::evaluate_constant_expr(&ref, env);
  expect_true(!result.ok(),
              "complete local const source miss should not use rendered fallback");
}

void test_local_const_binding_no_metadata_uses_rendered_map() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.local_const_bindings["__generated_local_const"] = 41;

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "__generated_local_const";
  ref.unqualified_name = "__generated_local_const";
  ref.unqualified_text_id = c4c::kInvalidText;

  c4c::hir::Lowerer::LowererConstEvalStructuredMaps structured_maps;
  c4c::hir::ConstEvalEnv env = lowerer.make_lowerer_consteval_env(
      structured_maps, &ctx.local_const_bindings,
      &ctx.local_const_bindings_by_text, &ctx.local_const_bindings_by_key);
  const c4c::hir::ConstEvalResult result =
      c4c::hir::evaluate_constant_expr(&ref, env);
  expect_true(result.ok() && result.as_int() == 41,
              "no-metadata local const lookup should retain rendered compatibility");
}

void test_local_function_prototype_prefers_structured_decl_over_stale_rendered_name() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("stale_local_proto");
  const c4c::TextId structured_text = texts.intern("structured_local_proto");
  c4c::hir::NamespaceQualifier local_ns;
  local_ns.context_id = 0;

  add_function(module, c4c::hir::FunctionId{60}, "stale_local_proto",
               stale_text, c4c::kInvalidLinkName, local_ns);
  add_function(module, c4c::hir::FunctionId{61}, "structured_local_proto",
               structured_text, c4c::kInvalidLinkName, local_ns);

  c4c::Node decl{};
  decl.kind = c4c::NK_DECL;
  decl.name = "stale_local_proto";
  decl.unqualified_name = "structured_local_proto";
  decl.unqualified_text_id = structured_text;
  decl.n_params = 1;

  c4c::hir::Lowerer::FunctionCtx ctx;

  lowerer.lower_local_decl_stmt(ctx, &decl);

  expect_true(ctx.locals.find("stale_local_proto") == ctx.locals.end(),
              "local function prototype should not create a local when structured function metadata resolves");
  expect_true(module.expr_pool.empty(),
              "local function prototype skip should not append initializer expressions");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Function,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "stale_local_proto", 61),
              "local function prototype lookup should record structured authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Function,
                           "stale_local_proto", 61, 60),
              "local function prototype lookup should record legacy rendered parity mismatch");
}

void test_var_expr_global_fallback_prefers_structured_decl_over_stale_rendered_name() {
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId stale_text = texts.intern("stale_var_global");
  const c4c::TextId structured_text = texts.intern("structured_var_global");
  const c4c::LinkNameId stale_link =
      module.link_names.intern("stale_var_global_link");
  const c4c::LinkNameId structured_link =
      module.link_names.intern("structured_var_global_link");
  c4c::hir::NamespaceQualifier local_ns;
  local_ns.context_id = 0;

  add_global(module, c4c::hir::GlobalId{70}, "stale_var_global",
             stale_text, stale_link, local_ns);
  add_global(module, c4c::hir::GlobalId{71}, "structured_var_global",
             structured_text, structured_link, local_ns);

  c4c::Node var{};
  var.kind = c4c::NK_VAR;
  var.name = "stale_var_global";
  var.unqualified_name = "structured_var_global";
  var.unqualified_text_id = structured_text;
  var.type.base = c4c::TB_INT;

  const c4c::hir::ExprId expr_id = lowerer.lower_var_expr(nullptr, &var);
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  const auto* ref =
      expr ? std::get_if<c4c::hir::DeclRef>(&expr->payload) : nullptr;
  expect_true(ref != nullptr,
              "ordinary variable fallback should lower to a DeclRef");
  expect_true(ref->global && ref->global->value == 71,
              "ordinary variable fallback should prefer structured global metadata over stale rendered name");
  expect_true(ref->name == "structured_var_global",
              "ordinary variable fallback should rewrite DeclRef name to the resolved structured global");
  expect_true(ref->link_name_id == structured_link,
              "ordinary variable fallback should preserve the resolved global link-visible payload");
  expect_true(has_hit(module, c4c::hir::ModuleDeclKind::Global,
                      c4c::hir::ModuleDeclLookupAuthority::Structured,
                      "stale_var_global", 71),
              "ordinary variable fallback should record structured global authority");
  expect_true(has_mismatch(module, c4c::hir::ModuleDeclKind::Global,
                           "stale_var_global", 71, 70),
              "ordinary variable fallback should record legacy rendered parity mismatch");
}

void test_local_decl_direct_agg_structured_owner_miss_rejects_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* unresolved_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  unresolved_record->name = arena.strdup("UnresolvedDirectAggOwner");
  unresolved_record->unqualified_name = arena.strdup("UnresolvedDirectAggOwner");
  unresolved_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec missing_ts{};
  missing_ts.array_size = -1;
  missing_ts.inner_rank = -1;
  missing_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(missing_ts, arena.strdup("StaleDirectAggOwner"), 0);
  missing_ts.tag_text_id = module.link_name_texts->intern("StaleDirectAggOwner");
  missing_ts.record_def = unresolved_record;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleDirectAggOwner";
  stale_def.tag_text_id = missing_ts.tag_text_id;
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* array_source = parser.make_node(c4c::NK_VAR, 1);
  array_source->name = arena.strdup("source");
  array_source->unqualified_name = arena.strdup("source");
  array_source->type = missing_ts;
  c4c::Node* array_init = parser.make_node(c4c::NK_INIT_LIST, 1);
  array_init->children = arena.alloc_array<c4c::Node*>(1);
  array_init->children[0] = array_source;
  array_init->n_children = 1;

  c4c::TypeSpec array_ts = missing_ts;
  array_ts.array_rank = 1;
  array_ts.array_size = 1;
  array_ts.array_dims[0] = 1;
  c4c::Node* array_decl = parser.make_node(c4c::NK_DECL, 1);
  array_decl->name = arena.strdup("items");
  array_decl->unqualified_name = arena.strdup("items");
  array_decl->type = array_ts;
  array_decl->init = array_init;

  c4c::hir::Lowerer::FunctionCtx array_ctx;
  c4c::hir::Function array_fn;
  array_ctx.fn = &array_fn;
  array_ctx.current_block = c4c::hir::BlockId{0};
  const c4c::hir::LocalId source_id{42};
  array_ctx.locals["source"] = source_id;
  array_ctx.local_types.insert(source_id, missing_ts);
  lowerer.lower_local_decl_stmt(array_ctx, array_decl);

  bool used_array_direct_assign = false;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    const auto* assign = std::get_if<c4c::hir::AssignExpr>(&expr.payload);
    if (!assign) continue;
    const c4c::hir::Expr* lhs = module.find_expr(assign->lhs);
    const c4c::hir::Expr* rhs = module.find_expr(assign->rhs);
    const auto* index =
        lhs ? std::get_if<c4c::hir::IndexExpr>(&lhs->payload) : nullptr;
    const auto* ref =
        rhs ? std::get_if<c4c::hir::DeclRef>(&rhs->payload) : nullptr;
    if (index && ref && ref->name == "source") used_array_direct_assign = true;
  }
  expect_true(!used_array_direct_assign,
              "array aggregate direct assignment must not use stale rendered tags after structured owner miss");

  c4c::Node* outer_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  outer_record->name = arena.strdup("RealDirectAggOuter");
  outer_record->unqualified_name = arena.strdup("RealDirectAggOuter");
  outer_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> outer_key =
      lowerer.make_struct_def_node_owner_key(outer_record);
  expect_true(outer_key.has_value(),
              "fixture should build a structured owner key for direct aggregate outer");
  c4c::hir::HirStructField field;
  field.name = "field";
  field.field_text_id = module.link_name_texts->intern("field");
  field.elem_type = missing_ts;
  field.member_symbol_id =
      module.member_symbols.intern("RealDirectAggOuter::field");
  c4c::hir::HirStructDef outer_def;
  outer_def.tag = "RealDirectAggOuter";
  outer_def.tag_text_id = module.link_name_texts->intern("RealDirectAggOuter");
  outer_def.ns_qual.context_id = parser.current_namespace_context_id();
  outer_def.fields.push_back(field);
  module.index_struct_def_owner(*outer_key, outer_def.tag, true);
  module.struct_defs[outer_def.tag] = outer_def;

  c4c::TypeSpec outer_ts{};
  outer_ts.array_size = -1;
  outer_ts.inner_rank = -1;
  outer_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(outer_ts, arena.strdup("RealDirectAggOuter"), 0);
  outer_ts.record_def = outer_record;

  c4c::Node* field_source = parser.make_node(c4c::NK_VAR, 1);
  field_source->name = arena.strdup("field_source");
  field_source->unqualified_name = arena.strdup("field_source");
  field_source->type = missing_ts;
  c4c::Node* field_init = parser.make_node(c4c::NK_INIT_LIST, 1);
  field_init->children = arena.alloc_array<c4c::Node*>(1);
  field_init->children[0] = field_source;
  field_init->n_children = 1;
  c4c::Node* field_decl = parser.make_node(c4c::NK_DECL, 1);
  field_decl->name = arena.strdup("holder");
  field_decl->unqualified_name = arena.strdup("holder");
  field_decl->type = outer_ts;
  field_decl->init = field_init;

  c4c::hir::Lowerer::FunctionCtx field_ctx;
  c4c::hir::Function field_fn;
  field_ctx.fn = &field_fn;
  field_ctx.current_block = c4c::hir::BlockId{0};
  const c4c::hir::LocalId field_source_id{43};
  field_ctx.locals["field_source"] = field_source_id;
  field_ctx.local_types.insert(field_source_id, missing_ts);
  const size_t field_expr_start = module.expr_pool.size();
  lowerer.lower_local_decl_stmt(field_ctx, field_decl);

  bool used_field_direct_assign = false;
  for (size_t i = field_expr_start; i < module.expr_pool.size(); ++i) {
    const auto* assign =
        std::get_if<c4c::hir::AssignExpr>(&module.expr_pool[i].payload);
    if (!assign) continue;
    const c4c::hir::Expr* lhs = module.find_expr(assign->lhs);
    const c4c::hir::Expr* rhs = module.find_expr(assign->rhs);
    const auto* member =
        lhs ? std::get_if<c4c::hir::MemberExpr>(&lhs->payload) : nullptr;
    const auto* ref =
        rhs ? std::get_if<c4c::hir::DeclRef>(&rhs->payload) : nullptr;
    if (member && member->field == "field" && ref &&
        ref->name == "field_source") {
      used_field_direct_assign = true;
    }
  }
  expect_true(!used_field_direct_assign,
              "initializer-list aggregate direct assignment must not use stale rendered tags after structured owner miss");
}

void test_local_anonymous_aggregate_init_uses_record_owner_key() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::C);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* anon_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  anon_record->name = arena.strdup("_anon_0");
  anon_record->unqualified_name = arena.strdup("_anon_0");
  anon_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(anon_record);
  expect_true(owner_key.has_value(),
              "fixture should build an owner key for generated anonymous record");

  c4c::hir::HirStructField field;
  field.name = "value";
  field.field_text_id = module.link_name_texts->intern("value");
  field.elem_type.base = c4c::TB_INT;
  field.elem_type.array_size = -1;
  field.elem_type.inner_rank = -1;
  field.member_symbol_id = module.member_symbols.intern("_anon_0::value");

  c4c::hir::HirStructDef def;
  def.tag = "_anon_0";
  def.tag_text_id = module.link_name_texts->intern("_anon_0");
  def.ns_qual.context_id = parser.current_namespace_context_id();
  def.fields.push_back(field);
  module.index_struct_def_owner(*owner_key, def.tag, true);
  module.struct_defs[def.tag] = def;

  c4c::TypeSpec anon_ts{};
  anon_ts.array_size = -1;
  anon_ts.inner_rank = -1;
  anon_ts.base = c4c::TB_STRUCT;
  anon_ts.record_def = anon_record;

  c4c::Node* literal = parser.make_node(c4c::NK_INT_LIT, 1);
  literal->ival = 7;
  c4c::Node* init = parser.make_node(c4c::NK_INIT_LIST, 1);
  init->children = arena.alloc_array<c4c::Node*>(1);
  init->children[0] = literal;
  init->n_children = 1;

  c4c::Node* decl = parser.make_node(c4c::NK_DECL, 1);
  decl->name = arena.strdup("local_anon");
  decl->unqualified_name = arena.strdup("local_anon");
  decl->type = anon_ts;
  decl->init = init;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  lowerer.lower_local_decl_stmt(ctx, decl);

  bool stored_field = false;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    const auto* assign = std::get_if<c4c::hir::AssignExpr>(&expr.payload);
    if (!assign) continue;
    const c4c::hir::Expr* lhs = module.find_expr(assign->lhs);
    const c4c::hir::Expr* rhs = module.find_expr(assign->rhs);
    const auto* member =
        lhs ? std::get_if<c4c::hir::MemberExpr>(&lhs->payload) : nullptr;
    const auto* value =
        rhs ? std::get_if<c4c::hir::IntLiteral>(&rhs->payload) : nullptr;
    if (member && member->field == "value" &&
        member->resolved_owner_tag == "_anon_0" && value && value->value == 7) {
      stored_field = true;
    }
  }
  expect_true(stored_field,
              "anonymous aggregate init should lower field stores through record owner metadata");
}

void test_deferred_member_typedef_record_def_miss_rejects_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealOwnerWithoutAlias");

  c4c::Node* stale_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  stale_record->name = arena.strdup("StaleRenderedOwnerWithAlias");
  stale_record->n_member_typedefs = 1;
  stale_record->member_typedef_names = arena.alloc_array<const char*>(1);
  stale_record->member_typedef_names[0] = arena.strdup("value_type");
  stale_record->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  stale_record->member_typedef_types[0].array_size = -1;
  stale_record->member_typedef_types[0].inner_rank = -1;
  stale_record->member_typedef_types[0].base = c4c::TB_LONG;

  c4c::TypeSpec owner{};
  owner.array_size = -1;
  owner.inner_rank = -1;
  owner.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(
      owner, arena.strdup("StaleRenderedOwnerWithAlias"), 0);
  owner.record_def = real_record;
  owner.deferred_member_type_name = arena.strdup("value_type");

  c4c::hir::Lowerer lowerer;
  lowerer.struct_def_nodes_["StaleRenderedOwnerWithAlias"] = stale_record;
  const bool resolved = lowerer.resolve_struct_member_typedef_if_ready(&owner);

  expect_true(!resolved,
              "structured record_def member typedef miss must not fall back to stale rendered owner tag");
  expect_true(owner.base == c4c::TB_STRUCT &&
                  owner.deferred_member_type_name != nullptr,
              "failed structured member typedef lookup should leave the pending owner type intact");
}

void test_deferred_member_typedef_owner_key_miss_rejects_stale_node_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  const c4c::TextId ns_text = texts.intern("real_ns");
  const c4c::TextId owner_text = texts.intern("MissingOwner");
  const c4c::TextId member_text = texts.intern("value_type");

  c4c::Node stale_record{};
  stale_record.kind = c4c::NK_STRUCT_DEF;
  stale_record.unqualified_text_id = owner_text;
  stale_record.namespace_context_id = 7;
  stale_record.n_member_typedefs = 1;
  stale_record.member_typedef_text_ids = arena.alloc_array<c4c::TextId>(1);
  stale_record.member_typedef_text_ids[0] = member_text;
  stale_record.member_typedef_names = arena.alloc_array<const char*>(1);
  stale_record.member_typedef_names[0] = arena.strdup("value_type");
  stale_record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  stale_record.member_typedef_types[0].array_size = -1;
  stale_record.member_typedef_types[0].inner_rank = -1;
  stale_record.member_typedef_types[0].base = c4c::TB_LONG;

  c4c::TextId* qualifier_segments = arena.alloc_array<c4c::TextId>(1);
  qualifier_segments[0] = ns_text;
  c4c::TypeSpec pending{};
  pending.array_size = -1;
  pending.inner_rank = -1;
  pending.base = c4c::TB_STRUCT;
  pending.namespace_context_id = 7;
  pending.qualifier_text_ids = qualifier_segments;
  pending.n_qualifier_segments = 1;
  pending.deferred_member_type_owner_key =
      c4c::QualifiedNameKey{7, false, c4c::kInvalidNamePath, owner_text};
  pending.deferred_member_type_text_id = member_text;
  pending.deferred_member_type_name = arena.strdup("value_type");
  pending.tag_text_id = owner_text;

  c4c::hir::Lowerer lowerer;
  lowerer.struct_def_nodes_["real_ns::MissingOwner"] = &stale_record;

  const bool resolved = lowerer.resolve_struct_member_typedef_if_ready(&pending);
  expect_true(!resolved,
              "complete owner-key miss must not recover member typedef through stale struct_def_nodes_ tag scan");
  expect_true(pending.base == c4c::TB_STRUCT &&
                  pending.deferred_member_type_name != nullptr,
              "failed complete owner-key lookup should leave the pending member typedef intact");
}

void test_deferred_member_typedef_owner_key_miss_rejects_stale_module_tag() {
  c4c::Arena arena;
  c4c::hir::Module module;
  module.attach_link_name_texts(std::make_shared<c4c::TextTable>());
  c4c::TextTable& texts = *module.link_name_texts;
  const c4c::TextId ns_text = texts.intern("real_ns");
  const c4c::TextId owner_text = texts.intern("MissingOwner");
  const c4c::TextId member_text = texts.intern("value_type");

  c4c::Node stale_record{};
  stale_record.kind = c4c::NK_STRUCT_DEF;
  stale_record.unqualified_text_id = c4c::kInvalidText;
  stale_record.namespace_context_id = -1;
  stale_record.n_member_typedefs = 1;
  stale_record.member_typedef_text_ids = arena.alloc_array<c4c::TextId>(1);
  stale_record.member_typedef_text_ids[0] = member_text;
  stale_record.member_typedef_names = arena.alloc_array<const char*>(1);
  stale_record.member_typedef_names[0] = arena.strdup("value_type");
  stale_record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  stale_record.member_typedef_types[0].array_size = -1;
  stale_record.member_typedef_types[0].inner_rank = -1;
  stale_record.member_typedef_types[0].base = c4c::TB_LONG;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "real_ns::MissingOwner";
  stale_def.tag_text_id = owner_text;
  stale_def.ns_qual.context_id = 7;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TextId* qualifier_segments = arena.alloc_array<c4c::TextId>(1);
  qualifier_segments[0] = ns_text;
  c4c::TypeSpec pending{};
  pending.array_size = -1;
  pending.inner_rank = -1;
  pending.base = c4c::TB_STRUCT;
  pending.namespace_context_id = 7;
  pending.qualifier_text_ids = qualifier_segments;
  pending.n_qualifier_segments = 1;
  pending.deferred_member_type_owner_key =
      c4c::QualifiedNameKey{7, false, c4c::kInvalidNamePath, owner_text};
  pending.deferred_member_type_text_id = member_text;
  pending.deferred_member_type_name = arena.strdup("value_type");
  pending.tag_text_id = owner_text;

  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;
  lowerer.struct_def_nodes_[stale_def.tag] = &stale_record;

  const bool resolved = lowerer.resolve_struct_member_typedef_if_ready(&pending);
  expect_true(!resolved,
              "complete owner-key miss must not recover member typedef through stale module struct_defs tag scan");
  expect_true(pending.base == c4c::TB_STRUCT &&
                  pending.deferred_member_type_name != nullptr,
              "failed complete owner-key module lookup should leave the pending member typedef intact");
}

void test_deferred_member_typedef_uses_owner_key_and_member_text_id() {
  c4c::Arena arena;
  c4c::TextTable texts;
  const c4c::TextId ns_text = texts.intern("real_ns");
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId member_text = texts.intern("value_type");

  c4c::Node real_record{};
  real_record.kind = c4c::NK_STRUCT_DEF;
  real_record.n_member_typedefs = 1;
  real_record.member_typedef_text_ids = arena.alloc_array<c4c::TextId>(1);
  real_record.member_typedef_text_ids[0] = member_text;
  real_record.member_typedef_names = arena.alloc_array<const char*>(1);
  real_record.member_typedef_names[0] = arena.strdup("stale_name");
  real_record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  real_record.member_typedef_types[0].array_size = -1;
  real_record.member_typedef_types[0].inner_rank = -1;
  real_record.member_typedef_types[0].base = c4c::TB_LONG;

  c4c::hir::NamespaceQualifier owner_ns;
  owner_ns.context_id = 7;
  owner_ns.segment_text_ids.push_back(ns_text);
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(owner_ns, owner_text);

  c4c::hir::Lowerer lowerer;
  lowerer.struct_def_nodes_by_owner_[owner_key] = &real_record;

  c4c::TextId* qualifier_segments = arena.alloc_array<c4c::TextId>(1);
  qualifier_segments[0] = ns_text;
  c4c::TypeSpec pending{};
  pending.array_size = -1;
  pending.inner_rank = -1;
  pending.base = c4c::TB_STRUCT;
  pending.namespace_context_id = 7;
  pending.qualifier_text_ids = qualifier_segments;
  pending.n_qualifier_segments = 1;
  pending.deferred_member_type_owner_key =
      c4c::QualifiedNameKey{7, false, c4c::kInvalidNamePath, owner_text};
  pending.deferred_member_type_text_id = member_text;
  pending.deferred_member_type_name = arena.strdup("wrong_rendered_member");
  set_legacy_tag_if_present(pending, arena.strdup("stale_ns::Box"), 0);

  const bool resolved = lowerer.resolve_struct_member_typedef_if_ready(&pending);
  expect_true(resolved,
              "deferred member typedef should resolve through owner key and member TextId");
  expect_true(pending.base == c4c::TB_LONG,
              "stale rendered member spelling must not override member TextId");
}

void test_deferred_member_typedef_owner_key_beats_suffix_split_collision() {
  c4c::Arena arena;
  c4c::TextTable texts;
  const c4c::TextId real_ns_text = texts.intern("real_ns");
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId member_text = texts.intern("value_type");

  c4c::Node real_record{};
  real_record.kind = c4c::NK_STRUCT_DEF;
  real_record.name = "real_ns::Box";
  real_record.unqualified_name = "Box";
  real_record.unqualified_text_id = owner_text;
  real_record.namespace_context_id = 7;
  real_record.n_member_typedefs = 1;
  real_record.member_typedef_text_ids = arena.alloc_array<c4c::TextId>(1);
  real_record.member_typedef_text_ids[0] = member_text;
  real_record.member_typedef_names = arena.alloc_array<const char*>(1);
  real_record.member_typedef_names[0] = arena.strdup("value_type");
  real_record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  real_record.member_typedef_types[0].array_size = -1;
  real_record.member_typedef_types[0].inner_rank = -1;
  real_record.member_typedef_types[0].base = c4c::TB_LONG;

  c4c::Node stale_record{};
  stale_record.kind = c4c::NK_STRUCT_DEF;
  stale_record.name = "stale_ns::Box";
  stale_record.unqualified_name = "Box";
  stale_record.unqualified_text_id = owner_text;
  stale_record.namespace_context_id = 9;
  stale_record.n_member_typedefs = 1;
  stale_record.member_typedef_text_ids = arena.alloc_array<c4c::TextId>(1);
  stale_record.member_typedef_text_ids[0] = member_text;
  stale_record.member_typedef_names = arena.alloc_array<const char*>(1);
  stale_record.member_typedef_names[0] = arena.strdup("value_type");
  stale_record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  stale_record.member_typedef_types[0].array_size = -1;
  stale_record.member_typedef_types[0].inner_rank = -1;
  stale_record.member_typedef_types[0].base = c4c::TB_INT;

  c4c::hir::NamespaceQualifier real_owner_ns;
  real_owner_ns.context_id = 7;
  real_owner_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_owner_key =
      c4c::hir::make_hir_record_owner_key(real_owner_ns, owner_text);

  c4c::hir::Lowerer lowerer;
  lowerer.struct_def_nodes_by_owner_[real_owner_key] = &real_record;
  lowerer.struct_def_nodes_["stale_ns::Box"] = &stale_record;

  c4c::TextId* qualifier_segments = arena.alloc_array<c4c::TextId>(1);
  qualifier_segments[0] = real_ns_text;
  c4c::TypeSpec pending{};
  pending.array_size = -1;
  pending.inner_rank = -1;
  pending.base = c4c::TB_STRUCT;
  pending.namespace_context_id = 7;
  pending.qualifier_text_ids = qualifier_segments;
  pending.n_qualifier_segments = 1;
  pending.deferred_member_type_owner_key =
      c4c::QualifiedNameKey{7, false, c4c::kInvalidNamePath, owner_text};
  pending.deferred_member_type_text_id = member_text;
  pending.deferred_member_type_name = arena.strdup("value_type");
  set_legacy_tag_if_present(pending, arena.strdup("stale_ns::Box"), 0);
  pending.tag_text_id = owner_text;

  const bool resolved = lowerer.resolve_struct_member_typedef_if_ready(&pending);
  expect_true(resolved,
              "deferred member typedef should resolve despite a same-suffix stale rendered owner");
  expect_true(pending.base == c4c::TB_LONG,
              "deferred member typedef should use structured owner key before suffix-split rendered tag");
}

void test_pending_deferred_member_typedef_preserves_owner_qualifier_segments() {
  c4c::Arena arena;
  c4c::TextTable texts;
  const c4c::TextId ns_text = texts.intern("real_ns");
  const c4c::TextId owner_text = texts.intern("Box");
  const c4c::TextId member_text = texts.intern("value_type");

  c4c::Node real_record{};
  real_record.kind = c4c::NK_STRUCT_DEF;
  real_record.n_member_typedefs = 1;
  real_record.member_typedef_text_ids = arena.alloc_array<c4c::TextId>(1);
  real_record.member_typedef_text_ids[0] = member_text;
  real_record.member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  real_record.member_typedef_types[0].array_size = -1;
  real_record.member_typedef_types[0].inner_rank = -1;
  real_record.member_typedef_types[0].base = c4c::TB_LONG;

  c4c::hir::NamespaceQualifier qualified_owner_ns;
  qualified_owner_ns.context_id = 7;
  qualified_owner_ns.segment_text_ids.push_back(ns_text);
  const c4c::hir::HirRecordOwnerKey qualified_owner_key =
      c4c::hir::make_hir_record_owner_key(qualified_owner_ns, owner_text);

  c4c::hir::NamespaceQualifier segmentless_owner_ns;
  segmentless_owner_ns.context_id = 7;
  const c4c::hir::HirRecordOwnerKey segmentless_owner_key =
      c4c::hir::make_hir_record_owner_key(segmentless_owner_ns, owner_text);

  c4c::hir::Lowerer lowerer;
  lowerer.struct_def_nodes_by_owner_[qualified_owner_key] = &real_record;
  expect_true(lowerer.struct_def_nodes_by_owner_.count(segmentless_owner_key) == 0,
              "fixture should only register the qualified owner key");

  c4c::TextId* qualifier_segments = arena.alloc_array<c4c::TextId>(1);
  qualifier_segments[0] = ns_text;
  c4c::TypeSpec pending{};
  pending.array_size = -1;
  pending.inner_rank = -1;
  pending.base = c4c::TB_STRUCT;
  pending.namespace_context_id = 7;
  pending.qualifier_text_ids = qualifier_segments;
  pending.n_qualifier_segments = 1;
  pending.deferred_member_type_owner_key =
      c4c::QualifiedNameKey{7, false, c4c::kInvalidNamePath, owner_text};
  pending.deferred_member_type_text_id = member_text;
  pending.deferred_member_type_name = arena.strdup("wrong_rendered_member");

  c4c::hir::PendingTemplateTypeWorkItem work_item;
  work_item.kind = c4c::hir::PendingTemplateTypeKind::MemberTypedef;
  work_item.pending_type = pending;
  work_item.context_name = "qualified-owner-segment-test";

  const c4c::hir::DeferredTemplateTypeResult result =
      lowerer.resolve_deferred_member_typedef_type(work_item);
  expect_true(result.kind == c4c::hir::DeferredTemplateTypeResultKind::Resolved,
              "pending deferred member typedef should copy qualifier TextIds into the owner key");
}

void test_hir_template_arg_materialization_uses_nttp_domain_carrier() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Box");
  primary->unqualified_name = arena.strdup("Box");
  primary->unqualified_text_id = texts.intern("Box");
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("N");
  primary->template_param_name_text_ids = arena.alloc_array<c4c::TextId>(1);
  primary->template_param_name_text_ids[0] = texts.intern("N");
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = true;

  c4c::TypeSpec owner{};
  owner.array_size = -1;
  owner.inner_rank = -1;
  owner.base = c4c::TB_STRUCT;
  owner.tpl_struct_origin = arena.strdup("Box");
  owner.tpl_struct_args.size = 1;
  owner.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  owner.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
  owner.tpl_struct_args.data[0].value = 0;
  owner.tpl_struct_args.data[0].nttp_text_id = texts.intern("N");
  owner.tpl_struct_args.data[0].nttp_owner_text_id =
      primary->unqualified_text_id;
  owner.tpl_struct_args.data[0].nttp_owner_namespace_context_id =
      primary->namespace_context_id;
  owner.tpl_struct_args.data[0].nttp_param_index = 0;
  owner.tpl_struct_args.data[0].nttp_param_kind =
      c4c::TemplateParamDomainKind::NonType;
  owner.tpl_struct_args.data[0].debug_text = arena.strdup("RenderedN");

  c4c::hir::Lowerer lowerer;
  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  nttp_bindings["N"] = 13;
  nttp_bindings["RenderedN"] = 101;

  const c4c::hir::ResolvedTemplateArgs resolved =
      lowerer.materialize_template_args(primary, owner, type_bindings,
                                        nttp_bindings);

  expect_true(resolved.concrete_args.size() == 1 &&
                  resolved.concrete_args[0].is_value,
              "HIR NTTP materialization should accept a complete owner/index/kind carrier");
  expect_eq_int(static_cast<int>(resolved.concrete_args[0].value), 13,
                "HIR NTTP materialization should bind through the carrier's "
                "parameter domain instead of stale debug/rendered text");
  expect_true(resolved.nttp_bindings.size() == 1 &&
                  resolved.nttp_bindings[0].first == "N",
              "HIR NTTP materialization should write the primary parameter binding");
  expect_eq_int(static_cast<int>(resolved.nttp_bindings[0].second), 13,
                "HIR NTTP materialization should preserve the carrier-selected value");
}

void test_hir_template_arg_materialization_rejects_foreign_nttp_domain_carrier() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Node* inner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  inner->name = arena.strdup("Inner");
  inner->unqualified_name = arena.strdup("Inner");
  inner->unqualified_text_id = texts.intern("Inner");
  inner->namespace_context_id = parser.current_namespace_context_id();
  inner->n_template_params = 1;
  inner->template_param_names = arena.alloc_array<const char*>(1);
  inner->template_param_names[0] = arena.strdup("N");
  inner->template_param_name_text_ids = arena.alloc_array<c4c::TextId>(1);
  inner->template_param_name_text_ids[0] = texts.intern("N");
  inner->template_param_is_nttp = arena.alloc_array<bool>(1);
  inner->template_param_is_nttp[0] = true;

  c4c::Node* outer = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  outer->name = arena.strdup("Outer");
  outer->unqualified_name = arena.strdup("Outer");
  outer->unqualified_text_id = texts.intern("Outer");
  outer->namespace_context_id = parser.current_namespace_context_id();
  outer->n_template_params = 1;
  outer->template_param_names = arena.alloc_array<const char*>(1);
  outer->template_param_names[0] = arena.strdup("N");
  outer->template_param_name_text_ids = arena.alloc_array<c4c::TextId>(1);
  outer->template_param_name_text_ids[0] = texts.intern("N");
  outer->template_param_is_nttp = arena.alloc_array<bool>(1);
  outer->template_param_is_nttp[0] = true;

  c4c::TypeSpec owner{};
  owner.array_size = -1;
  owner.inner_rank = -1;
  owner.base = c4c::TB_STRUCT;
  owner.tpl_struct_origin = arena.strdup("Inner");
  owner.tpl_struct_args.size = 1;
  owner.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  owner.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
  owner.tpl_struct_args.data[0].value = 0;
  owner.tpl_struct_args.data[0].nttp_text_id = texts.intern("N");
  owner.tpl_struct_args.data[0].nttp_owner_text_id =
      outer->unqualified_text_id;
  owner.tpl_struct_args.data[0].nttp_owner_namespace_context_id =
      outer->namespace_context_id;
  owner.tpl_struct_args.data[0].nttp_param_index = 0;
  owner.tpl_struct_args.data[0].nttp_param_kind =
      c4c::TemplateParamDomainKind::NonType;
  owner.tpl_struct_args.data[0].debug_text = arena.strdup("N");

  c4c::hir::Lowerer lowerer;
  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  nttp_bindings["N"] = 99;

  const c4c::hir::ResolvedTemplateArgs resolved =
      lowerer.materialize_template_args(inner, owner, type_bindings,
                                        nttp_bindings);

  expect_true(resolved.concrete_args.empty() && resolved.nttp_bindings.empty(),
              "equal-spelling NTTP parameters in different owners must not "
              "bind through TemplateArgRef::nttp_text_id, debug_text, or the "
              "current primary's rendered parameter name");
}

void test_struct_def_owner_key_prefers_spelling_carriers_over_stale_text_ids() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId real_ns_text = module.link_name_texts->intern("RealNs");
  const c4c::TextId real_owner_text =
      module.link_name_texts->intern("RealOwner");
  const c4c::TextId stale_ns_text = module.link_name_texts->intern("StaleNs");
  const c4c::TextId stale_owner_text =
      module.link_name_texts->intern("StaleOwner");

  const char* qualifier_segments[] = {"RealNs"};
  c4c::TextId qualifier_text_ids[] = {stale_ns_text};
  c4c::Node sd{};
  sd.kind = c4c::NK_STRUCT_DEF;
  sd.name = "RenderedOwner";
  sd.unqualified_name = "RealOwner";
  sd.unqualified_text_id = stale_owner_text;
  sd.namespace_context_id = 41;
  sd.qualifier_segments = qualifier_segments;
  sd.qualifier_text_ids = qualifier_text_ids;
  sd.n_qualifier_segments = 1;

  lowerer.lower_struct_def(&sd);
  lowerer.register_struct_def_node_owner(&sd);

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = 41;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real_owner_text);
  c4c::hir::NamespaceQualifier stale_ns;
  stale_ns.context_id = 41;
  stale_ns.segment_text_ids.push_back(stale_ns_text);
  const c4c::hir::HirRecordOwnerKey stale_key =
      c4c::hir::make_hir_record_owner_key(stale_ns, stale_owner_text);

  const c4c::hir::HirStructDef* structured_def =
      module.find_struct_def_by_owner_structured(real_key);
  expect_true(structured_def != nullptr && structured_def->tag == "RenderedOwner",
              "struct definition owner indexing should prefer spelling carriers over stale TextIds");
  expect_true(module.find_struct_def_by_owner_structured(stale_key) == nullptr,
              "stale TextIds must not control the struct-definition owner key");
  expect_true(lowerer.struct_def_nodes_by_owner_.count(real_key) == 1,
              "struct definition node owner registration should use the structured owner key");
  expect_true(lowerer.struct_def_nodes_by_owner_.count(stale_key) == 0,
              "struct definition node registration should reject stale TextId owner keys");
}

void test_struct_def_owner_key_prefers_complete_text_ids_over_stale_spelling() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId real_ns_text = module.link_name_texts->intern("RealNs");
  const c4c::TextId real_owner_text =
      module.link_name_texts->intern("RealOwner");
  const c4c::TextId stale_ns_text = module.link_name_texts->intern("StaleNs");
  const c4c::TextId stale_owner_text =
      module.link_name_texts->intern("StaleOwner");

  const char* qualifier_segments[] = {"StaleNs"};
  c4c::TextId qualifier_text_ids[] = {real_ns_text};
  c4c::Node sd{};
  sd.kind = c4c::NK_STRUCT_DEF;
  sd.name = "RenderedOwner";
  sd.unqualified_name = "StaleOwner";
  sd.unqualified_text_id = real_owner_text;
  sd.namespace_context_id = 42;
  sd.qualifier_segments = qualifier_segments;
  sd.qualifier_text_ids = qualifier_text_ids;
  sd.n_qualifier_segments = 1;

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = 42;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real_owner_text);
  c4c::hir::NamespaceQualifier stale_ns;
  stale_ns.context_id = 42;
  stale_ns.segment_text_ids.push_back(stale_ns_text);
  const c4c::hir::HirRecordOwnerKey stale_key =
      c4c::hir::make_hir_record_owner_key(stale_ns, stale_owner_text);

  lowerer.struct_def_owner_by_node_[&sd] = real_key;
  lowerer.register_struct_def_node_owner(&sd);

  expect_true(lowerer.make_struct_def_node_owner_key(&sd) == real_key,
              "complete node-carried owner TextIds should be the struct-definition owner authority");
  expect_true(lowerer.struct_def_nodes_by_owner_.count(real_key) == 1,
              "struct definition node registration should use complete TextId identity before rendered repair");
  expect_true(lowerer.struct_def_nodes_by_owner_.count(stale_key) == 0,
              "stale rendered spelling must not control struct-definition owner registration when TextIds are complete");
}

void test_struct_def_owner_key_canonicalizes_parser_qualifier_text_ids() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId parser_ns_text_id = 1;
  const c4c::TextId parser_owner_text_id = 2;
  const c4c::TextId collision_ns_text =
      module.link_name_texts->intern("CollisionNs");
  const c4c::TextId collision_owner_text =
      module.link_name_texts->intern("CollisionOwner");
  expect_true(parser_ns_text_id == collision_ns_text &&
                  parser_owner_text_id == collision_owner_text,
              "test setup should force parser-owned TextId collisions in link_name_texts");
  const c4c::TextId real_ns_text = module.link_name_texts->intern("RealNs");
  const c4c::TextId real_owner_text =
      module.link_name_texts->intern("RealOwner");

  const char* qualifier_segments[] = {"RealNs"};
  c4c::TextId qualifier_text_ids[] = {parser_ns_text_id};
  c4c::Node sd{};
  sd.kind = c4c::NK_STRUCT_DEF;
  sd.name = "RenderedParserOwner";
  sd.unqualified_name = "RealOwner";
  sd.unqualified_text_id = parser_owner_text_id;
  sd.namespace_context_id = 43;
  sd.qualifier_segments = qualifier_segments;
  sd.qualifier_text_ids = qualifier_text_ids;
  sd.n_qualifier_segments = 1;

  lowerer.lower_struct_def(&sd);
  lowerer.register_struct_def_node_owner(&sd);

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = 43;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real_owner_text);
  c4c::hir::NamespaceQualifier collision_ns;
  collision_ns.context_id = 43;
  collision_ns.segment_text_ids.push_back(collision_ns_text);
  const c4c::hir::HirRecordOwnerKey collision_key =
      c4c::hir::make_hir_record_owner_key(
          collision_ns, collision_owner_text);

  expect_true(module.find_struct_def_by_owner_structured(real_key) != nullptr,
              "parser-owned qualifier TextIds should canonicalize into Module::link_name_texts");
  expect_true(module.find_struct_def_by_owner_structured(collision_key) == nullptr,
              "parser-owned raw TextId collisions must not become struct owner identity");
  expect_true(lowerer.struct_def_nodes_by_owner_.count(real_key) == 1,
              "node owner registration should use canonical link_name_texts TextIds");
  expect_true(lowerer.struct_def_nodes_by_owner_.count(collision_key) == 0,
              "node owner registration should not copy parser-owned TextIds directly");
}

void test_struct_def_owner_key_interns_first_use_spelling_carriers() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId parser_ns_text_id = 1;
  const c4c::TextId parser_owner_text_id = 2;
  const c4c::TextId collision_ns_text =
      module.link_name_texts->intern("FirstUseCollisionNs");
  const c4c::TextId collision_owner_text =
      module.link_name_texts->intern("FirstUseCollisionOwner");
  expect_true(parser_ns_text_id == collision_ns_text &&
                  parser_owner_text_id == collision_owner_text,
              "first-use fixture should force parser-owned TextId collisions");
  expect_true(module.link_name_texts->find("FirstUseRealNs") == c4c::kInvalidText,
              "first-use real namespace spelling should not be pre-interned");
  expect_true(module.link_name_texts->find("FirstUseRealOwner") == c4c::kInvalidText,
              "first-use real owner spelling should not be pre-interned");

  const char* qualifier_segments[] = {"FirstUseRealNs"};
  c4c::TextId qualifier_text_ids[] = {parser_ns_text_id};
  c4c::Node sd{};
  sd.kind = c4c::NK_STRUCT_DEF;
  sd.name = "RenderedFirstUseOwner";
  sd.unqualified_name = "FirstUseRealOwner";
  sd.unqualified_text_id = parser_owner_text_id;
  sd.namespace_context_id = 44;
  sd.qualifier_segments = qualifier_segments;
  sd.qualifier_text_ids = qualifier_text_ids;
  sd.n_qualifier_segments = 1;

  lowerer.lower_struct_def(&sd);
  lowerer.register_struct_def_node_owner(&sd);

  const c4c::TextId real_ns_text =
      module.link_name_texts->intern("FirstUseRealNs");
  const c4c::TextId real_owner_text =
      module.link_name_texts->intern("FirstUseRealOwner");
  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = 44;
  real_ns.segment_text_ids.push_back(real_ns_text);
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real_owner_text);
  c4c::hir::NamespaceQualifier collision_ns;
  collision_ns.context_id = 44;
  collision_ns.segment_text_ids.push_back(collision_ns_text);
  const c4c::hir::HirRecordOwnerKey collision_key =
      c4c::hir::make_hir_record_owner_key(
          collision_ns, collision_owner_text);

  expect_true(module.find_struct_def_by_owner_structured(real_key) != nullptr,
              "first-use spelling carriers should be interned into link_name_texts");
  expect_true(module.find_struct_def_by_owner_structured(collision_key) == nullptr,
              "first-use parser-owned raw TextId collisions must not become owner identity");
  expect_true(lowerer.struct_def_nodes_by_owner_.count(real_key) == 1,
              "first-use node owner registration should use interned spelling ids");
  expect_true(lowerer.struct_def_nodes_by_owner_.count(collision_key) == 0,
              "first-use node owner registration should reject raw parser ids");
}

void test_lower_struct_def_owner_miss_rejects_stale_rendered_existing_def() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructField stale_field;
  stale_field.name = "stale_field";
  stale_field.elem_type.base = c4c::TB_INT;
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "RenderedCollisionOwner";
  stale_def.tag_text_id =
      module.link_name_texts->intern("RenderedCollisionOwner");
  stale_def.ns_qual.context_id = 91;
  stale_def.fields.push_back(stale_field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node real{};
  real.kind = c4c::NK_STRUCT_DEF;
  real.name = "RenderedCollisionOwner";
  real.unqualified_name = "RealCollisionOwner";
  real.unqualified_text_id =
      module.link_name_texts->intern("RealCollisionOwner");
  real.namespace_context_id = 92;

  c4c::hir::NamespaceQualifier real_ns;
  real_ns.context_id = real.namespace_context_id;
  const c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(real_ns, real.unqualified_text_id);
  expect_true(module.find_struct_def_by_owner_structured(real_key) == nullptr,
              "fixture should start with a complete owner-key miss");

  lowerer.lower_struct_def(&real);

  const c4c::hir::HirStructDef* structured =
      module.find_struct_def_by_owner_structured(real_key);
  expect_true(structured != nullptr &&
                  structured->tag == "RenderedCollisionOwner",
              "lower_struct_def should index the complete owner key instead of treating a stale rendered struct_defs hit as the existing definition");
  expect_true(structured->fields.empty(),
              "stale rendered struct_defs payload must not make the complete owner-key miss look already populated");
}

void test_lower_struct_def_base_owner_miss_rejects_stale_rendered_base_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef stale_base;
  stale_base.tag = "StaleRenderedBase";
  stale_base.tag_text_id = module.link_name_texts->intern("StaleRenderedBase");
  stale_base.ns_qual.context_id = 101;
  module.struct_defs[stale_base.tag] = stale_base;

  c4c::Node unresolved_base{};
  unresolved_base.kind = c4c::NK_STRUCT_DEF;
  unresolved_base.name = "RealBase";
  unresolved_base.unqualified_name = "RealBase";
  unresolved_base.unqualified_text_id =
      module.link_name_texts->intern("RealBase");
  unresolved_base.namespace_context_id = 102;

  c4c::TypeSpec base_ts{};
  base_ts.base = c4c::TB_STRUCT;
  base_ts.array_size = -1;
  base_ts.inner_rank = -1;
  base_ts.tag_text_id = stale_base.tag_text_id;
  base_ts.record_def = &unresolved_base;

  c4c::Node derived{};
  derived.kind = c4c::NK_STRUCT_DEF;
  derived.name = "DerivedAfterBaseMiss";
  derived.unqualified_name = "DerivedAfterBaseMiss";
  derived.unqualified_text_id =
      module.link_name_texts->intern("DerivedAfterBaseMiss");
  derived.namespace_context_id = 103;
  derived.n_bases = 1;
  derived.base_types = &base_ts;

  lowerer.lower_struct_def(&derived);

  const auto derived_it = module.struct_defs.find("DerivedAfterBaseMiss");
  expect_true(derived_it != module.struct_defs.end(),
              "fixture should lower the derived struct");
  expect_true(!derived_it->second.base_tags.empty() &&
                  derived_it->second.base_tags[0] != "StaleRenderedBase",
              "complete base owner-key misses must not recover a base through a stale rendered struct_defs tag");
}

void test_out_of_class_nested_method_attach_uses_structured_owner_key() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId real_outer_text =
      module.link_name_texts->intern("RealOuter");
  const c4c::TextId stale_outer_text =
      module.link_name_texts->intern("StaleOuter");
  const c4c::TextId inner_text = module.link_name_texts->intern("Inner");
  const c4c::TextId method_text = module.link_name_texts->intern("run");

  c4c::hir::NamespaceQualifier real_owner_ns;
  real_owner_ns.context_id = 61;
  real_owner_ns.segment_text_ids.push_back(real_outer_text);
  const c4c::hir::HirRecordOwnerKey real_owner_key =
      c4c::hir::make_hir_record_owner_key(real_owner_ns, inner_text);
  module.index_struct_def_owner(real_owner_key, "RealOuter::Inner", true);

  c4c::hir::NamespaceQualifier stale_owner_ns;
  stale_owner_ns.context_id = 61;
  stale_owner_ns.segment_text_ids.push_back(stale_outer_text);
  const c4c::hir::HirRecordOwnerKey stale_owner_key =
      c4c::hir::make_hir_record_owner_key(stale_owner_ns, inner_text);
  module.index_struct_def_owner(stale_owner_key, "StaleOuter::Inner", true);
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleOuter::Inner";
  stale_def.tag_text_id = inner_text;
  stale_def.ns_qual = stale_owner_ns;
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::hir::HirStructMethodLookupKey real_method_key;
  real_method_key.owner_key = real_owner_key;
  real_method_key.method_text_id = method_text;
  real_method_key.is_const_method = false;
  lowerer.struct_methods_by_owner_[real_method_key] = "RealOuter_Inner__run";
  lowerer.struct_methods_["StaleOuter::Inner::run"] = "StaleOuter_Inner__run";

  c4c::Node real_decl{};
  real_decl.kind = c4c::NK_FUNCTION;
  real_decl.name = "run";
  c4c::Node stale_decl{};
  stale_decl.kind = c4c::NK_FUNCTION;
  stale_decl.name = "run";
  lowerer.pending_methods_.push_back(
      {"RealOuter_Inner__run", "RealOuter::Inner", &real_decl, {}, {}, {}});
  lowerer.pending_methods_.push_back(
      {"StaleOuter_Inner__run", "StaleOuter::Inner", &stale_decl, {}, {}, {}});

  const char* qualifier_segments[] = {"RealOuter", "Inner"};
  c4c::TextId qualifier_text_ids[] = {real_outer_text, inner_text};
  c4c::Node out_of_class{};
  out_of_class.kind = c4c::NK_FUNCTION;
  out_of_class.name = "StaleOuter::Inner::run";
  out_of_class.unqualified_name = "run";
  out_of_class.unqualified_text_id = method_text;
  out_of_class.qualifier_segments = qualifier_segments;
  out_of_class.qualifier_text_ids = qualifier_text_ids;
  out_of_class.n_qualifier_segments = 2;
  out_of_class.namespace_context_id = 61;
  c4c::Node body{};
  body.kind = c4c::NK_BLOCK;
  out_of_class.body = &body;

  std::vector<const c4c::Node*> items = {&out_of_class};
  lowerer.attach_out_of_class_struct_method_defs(items, module);

  expect_true(lowerer.pending_methods_[0].method_node == &out_of_class,
              "nested out-of-class method attach should use structured owner "
              "identity instead of stale rendered owner text");
  expect_true(lowerer.pending_methods_[1].method_node == &stale_decl,
              "stale rendered nested owner fallback must not capture the "
              "structured out-of-class method");
}

void test_out_of_class_method_attach_rejects_rendered_name_without_structured_key() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef rendered_def;
  rendered_def.tag = "RenderedOnlyOwner";
  rendered_def.tag_text_id = module.link_name_texts->intern("RenderedOnlyOwner");
  module.struct_defs[rendered_def.tag] = rendered_def;
  lowerer.struct_methods_["RenderedOnlyOwner::run"] = "RenderedOnlyOwner__run";

  c4c::Node stale_decl{};
  stale_decl.kind = c4c::NK_FUNCTION;
  stale_decl.name = "run";
  lowerer.pending_methods_.push_back(
      {"RenderedOnlyOwner__run", "RenderedOnlyOwner", &stale_decl, {}, {}, {}});

  c4c::Node fn{};
  fn.kind = c4c::NK_FUNCTION;
  fn.name = "RenderedOnlyOwner::run";
  fn.unqualified_name = "run";
  fn.unqualified_text_id = module.link_name_texts->intern("run");
  c4c::Node body{};
  body.kind = c4c::NK_BLOCK;
  fn.body = &body;

  std::vector<const c4c::Node*> items = {&fn};
  lowerer.attach_out_of_class_struct_method_defs(items, module);

  expect_true(lowerer.pending_methods_[0].method_node == &stale_decl,
              "out-of-class method attach must not use rendered Node::name splitting without a structured owner key");
}

void test_lower_non_method_complete_structured_method_miss_rejects_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  const c4c::TextId structured_owner_text =
      module.link_name_texts->intern("StructuredMethodMissOwner");
  const c4c::TextId method_text = module.link_name_texts->intern("run");

  c4c::hir::NamespaceQualifier owner_ns;
  owner_ns.context_id = 57;
  const c4c::hir::HirRecordOwnerKey structured_owner_key =
      c4c::hir::make_hir_record_owner_key(owner_ns, structured_owner_text);
  module.index_struct_def_owner(structured_owner_key,
                                "StructuredMethodMissOwner", true);
  c4c::hir::HirStructDef structured_def;
  structured_def.tag = "StructuredMethodMissOwner";
  structured_def.tag_text_id = structured_owner_text;
  structured_def.ns_qual = owner_ns;
  module.struct_defs[structured_def.tag] = structured_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "RenderedMethodOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("RenderedMethodOwner");
  stale_def.ns_qual = owner_ns;
  module.struct_defs[stale_def.tag] = stale_def;
  lowerer.struct_methods_["RenderedMethodOwner::run"] =
      "RenderedMethodOwner__run";

  const char* qualifier_segments[] = {"StructuredMethodMissOwner"};
  c4c::TextId qualifier_text_ids[] = {structured_owner_text};
  c4c::Node fn{};
  fn.kind = c4c::NK_FUNCTION;
  fn.name = "RenderedMethodOwner::run";
  fn.unqualified_name = "run";
  fn.unqualified_text_id = method_text;
  fn.qualifier_segments = qualifier_segments;
  fn.qualifier_text_ids = qualifier_text_ids;
  fn.n_qualifier_segments = 1;
  fn.namespace_context_id = 57;
  fn.type.array_size = -1;
  fn.type.inner_rank = -1;
  fn.type.base = c4c::TB_INT;

  std::vector<const c4c::Node*> items = {&fn};
  lowerer.lower_non_method_functions_and_globals(items, module);

  expect_eq_int(static_cast<int>(module.functions.size()), 1,
                "complete structured method miss should lower as a non-method instead of using stale rendered fallback");
  expect_true(module.functions[0].name == "RenderedMethodOwner::run",
              "non-method lowering should preserve the function definition after rejecting stale rendered method authority");
}

void test_lower_non_method_incomplete_structured_method_metadata_rejects_rendered_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef rendered_def;
  rendered_def.tag = "RenderedOnlyOwner";
  rendered_def.tag_text_id = module.link_name_texts->intern("RenderedOnlyOwner");
  module.struct_defs[rendered_def.tag] = rendered_def;
  lowerer.struct_methods_["RenderedOnlyOwner::run"] = "RenderedOnlyOwner__run";

  c4c::Node fn{};
  fn.kind = c4c::NK_FUNCTION;
  fn.name = "RenderedOnlyOwner::run";
  fn.unqualified_name = "run";
  fn.unqualified_text_id = module.link_name_texts->intern("run");
  fn.type.array_size = -1;
  fn.type.inner_rank = -1;
  fn.type.base = c4c::TB_INT;

  std::vector<const c4c::Node*> items = {&fn};
  lowerer.lower_non_method_functions_and_globals(items, module);

  expect_eq_int(static_cast<int>(module.functions.size()), 1,
                "incomplete structured method metadata should lower as a non-method instead of using rendered fallback");
  expect_true(module.functions[0].name == "RenderedOnlyOwner::run",
              "non-method lowering should preserve rendered names after rejecting rendered method authority");
}

}  // namespace

int main() {
  test_module_decl_lookup_records_each_authority();
  test_stale_rendered_names_do_not_override_authoritative_decl_lookup();
  test_complete_decl_lookup_miss_rejects_stale_rendered_indexes();
  test_direct_call_callee_lookup_uses_authoritative_decl_identity();
  test_operator_callee_lookup_uses_authoritative_decl_identity();
  test_range_for_method_callee_lookup_uses_authoritative_decl_identity();
  test_struct_owner_key_lookup_detects_stale_rendered_member_and_method_maps();
  test_compile_time_state_structured_registry_lookup_wins_over_stale_rendered_names();
  test_template_struct_primary_lookup_uses_record_def_before_stale_origin();
  test_template_struct_primary_record_def_miss_rejects_stale_origin();
  test_template_struct_primary_lookup_uses_origin_key_before_stale_tag();
  test_inherited_static_member_eval_uses_base_record_def_before_stale_tag();
  test_inherited_static_member_eval_base_record_def_miss_rejects_stale_tag();
  test_compile_time_function_specialization_type_arg_uses_record_def_identity();
  test_compile_time_function_specialization_type_arg_record_def_mismatch_rejects_tag();
  test_compile_time_function_specialization_tag_only_nominal_args_do_not_match();
  test_compile_time_function_specialization_primitive_args_still_match();
  test_template_deduction_forwarding_consistency_uses_record_def_identity();
  test_template_deduction_repeated_type_param_record_def_mismatch_rejects_tag();
  test_template_deduction_structured_pack_matches_instantiated_record_origin();
  test_template_materialization_substitutes_foreign_pack_ref_in_nested_owner();
  test_signature_substitution_preserves_nested_template_owner_identity();
  test_signature_member_typedef_complete_owner_miss_rejects_rendered_split();
  test_signature_member_typedef_no_complete_metadata_keeps_rendered_split();
  test_signature_member_typedef_owner_key_canonicalizes_parser_owner_ids();
  test_signature_type_member_owner_key_canonicalizes_parser_qualifier_ids();
  test_signature_return_type_complete_owner_miss_rejects_rendered_type_fallback();
  test_signature_return_type_no_complete_metadata_keeps_rendered_type_fallback();
  test_signature_parameter_type_complete_owner_miss_rejects_rendered_type_fallback();
  test_signature_parameter_type_no_complete_metadata_keeps_rendered_type_fallback();
  test_pending_type_ref_uses_structured_debug_payload_not_tag();
  test_pending_type_ref_no_metadata_keeps_shape_payload();
  test_specialization_display_type_uses_structured_record_key_not_tag();
  test_specialization_display_type_no_metadata_uses_explicit_unknown_name();
  test_type_suffix_for_mangling_uses_record_def_not_stale_tag();
  test_type_suffix_for_mangling_no_metadata_is_explicit_unknown();
  test_pending_consteval_nttp_handoff_carries_text_id_bindings();
  test_pending_consteval_nested_call_uses_compile_time_function_metadata();
  test_template_call_nttp_handoff_carries_text_id_bindings();
  test_template_global_nttp_init_uses_text_id_function_ctx_binding();
  test_static_member_nttp_const_eval_prefers_text_id_binding();
  test_template_value_arg_static_member_rejects_rendered_owner_split();
  test_template_value_arg_static_member_uses_structured_owner_key();
  test_scalar_static_member_rejects_rendered_owner_split();
  test_scalar_static_member_uses_member_text_after_structured_owner();
  test_scalar_static_member_non_template_uses_owner_key_without_tag();
  test_consteval_record_layout_prefers_hir_owner_key_over_stale_tag();
  test_layout_type_lookup_prefers_structured_owner_over_stale_tag();
  test_layout_type_lookup_structured_owner_miss_rejects_stale_tag();
  test_layout_type_lookup_no_owner_uses_tag_text_id_compatibility();
  test_compute_struct_layout_field_uses_record_def_before_stale_tag();
  test_compute_struct_layout_field_structured_miss_rejects_stale_tag();
  test_builtin_record_layout_prefers_hir_owner_key_over_stale_tag();
  test_builtin_record_layout_structured_owner_miss_rejects_stale_tag();
  test_builtin_record_layout_no_owner_uses_tag_text_id_compatibility();
  test_codegen_aggregate_layout_complete_owner_miss_rejects_indexed_stale_tag();
  test_codegen_aggregate_layout_complete_typespec_owner_miss_rejects_stale_tag();
  test_builtin_query_type_uses_template_param_text_id_binding();
  test_builtin_query_type_uses_text_binding_when_module_text_missing();
  test_builtin_query_type_structured_miss_rejects_stale_tag();
  test_builtin_query_type_no_metadata_keeps_compatibility_shape();
  test_lvalue_cast_uses_template_param_text_id_binding();
  test_lvalue_cast_structured_miss_rejects_stale_tag();
  test_var_expr_param_lookup_prefers_param_text_id_over_rendered_name();
  test_generic_type_param_lookup_prefers_param_text_id_over_rendered_name();
  test_param_lookup_text_miss_rejects_rendered_fallback();
  test_param_lookup_no_metadata_uses_rendered_fallback();
  test_param_lookup_marked_generated_text_id_uses_rendered_fallback();
  test_local_lookup_prefers_local_text_id_over_rendered_name();
  test_local_lookup_text_miss_rejects_rendered_fallback();
  test_local_lookup_no_metadata_and_marked_generated_use_rendered_fallback();
  test_param_fn_ptr_sig_lookup_prefers_param_text_id_over_rendered_name();
  test_param_fn_ptr_sig_text_miss_rejects_rendered_fallback();
  test_param_fn_ptr_sig_hit_without_signature_rejects_all_fallbacks();
  test_param_fn_ptr_sig_no_metadata_uses_rendered_fallback();
  test_local_fn_ptr_sig_lookup_prefers_local_text_id_over_rendered_name();
  test_local_fn_ptr_sig_lookup_shadows_same_spelled_param_sig();
  test_local_fn_ptr_sig_complete_local_shadow_blocks_param_fallback();
  test_local_fn_ptr_sig_text_miss_rejects_rendered_fallback();
  test_local_fn_ptr_sig_no_metadata_uses_rendered_fallback();
  test_callable_zero_sized_return_structured_miss_rejects_stale_tag();
  test_layout_type_lookup_canonicalizes_record_def_parser_owner_ids();
  test_callable_zero_return_canonicalizes_record_def_parser_owner_ids();
  test_typespec_aggregate_owner_key_canonicalizes_record_def_parser_owner_ids();
  test_global_aggregate_init_normalization_prefers_hir_owner_key_over_stale_tag();
  test_implicit_this_field_recovery_prefers_hir_owner_key_over_stale_tag();
  test_local_extern_global_lookup_prefers_structured_decl_over_stale_rendered_name();
  test_local_extern_clears_stale_source_local_identity();
  test_local_static_clears_stale_source_local_identity();
  test_static_global_bridge_source_text_id_beats_rendered_map();
  test_static_global_bridge_text_miss_rejects_rendered_map();
  test_static_global_bridge_rendered_compat_uses_rendered_map();
  test_local_const_binding_insertion_populates_source_maps();
  test_local_const_binding_source_text_id_beats_rendered_map();
  test_local_const_binding_text_miss_rejects_rendered_map();
  test_local_const_binding_no_metadata_uses_rendered_map();
  test_local_function_prototype_prefers_structured_decl_over_stale_rendered_name();
  test_var_expr_global_fallback_prefers_structured_decl_over_stale_rendered_name();
  test_local_decl_direct_agg_structured_owner_miss_rejects_stale_tag();
  test_local_anonymous_aggregate_init_uses_record_owner_key();
  test_deferred_member_typedef_record_def_miss_rejects_stale_tag();
  test_deferred_member_typedef_owner_key_miss_rejects_stale_node_tag();
  test_deferred_member_typedef_owner_key_miss_rejects_stale_module_tag();
  test_deferred_member_typedef_uses_owner_key_and_member_text_id();
  test_deferred_member_typedef_owner_key_beats_suffix_split_collision();
  test_pending_deferred_member_typedef_preserves_owner_qualifier_segments();
  test_hir_template_arg_materialization_uses_nttp_domain_carrier();
  test_hir_template_arg_materialization_rejects_foreign_nttp_domain_carrier();
  test_struct_def_owner_key_prefers_spelling_carriers_over_stale_text_ids();
  test_struct_def_owner_key_prefers_complete_text_ids_over_stale_spelling();
  test_struct_def_owner_key_canonicalizes_parser_qualifier_text_ids();
  test_struct_def_owner_key_interns_first_use_spelling_carriers();
  test_lower_struct_def_owner_miss_rejects_stale_rendered_existing_def();
  test_lower_struct_def_base_owner_miss_rejects_stale_rendered_base_tag();
  test_out_of_class_nested_method_attach_uses_structured_owner_key();
  test_out_of_class_method_attach_rejects_rendered_name_without_structured_key();
  test_lower_non_method_complete_structured_method_miss_rejects_rendered_fallback();
  test_lower_non_method_incomplete_structured_method_metadata_rejects_rendered_fallback();
  std::cout << "PASS: frontend_hir_lookup_tests\n";
  return 0;
}
