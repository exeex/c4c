#include "hir/hir_ir.hpp"
#include "hir/compile_time_engine.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema/consteval.hpp"
#include "sema.hpp"
#include "source_profile.hpp"

#include <algorithm>
#include <cctype>
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
  const c4c::TextId legacy_fn_text = texts.intern("legacy_fn");
  const c4c::TextId incomplete_qualified_fn_text =
      texts.intern("incomplete_qualified_fn");
  const c4c::TextId link_fn_text = texts.intern("link_fn");
  const c4c::TextId structured_global_text = texts.intern("structured_global");
  const c4c::TextId legacy_global_text = texts.intern("legacy_global");
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
  legacy_fn_ref.name_text_id = legacy_fn_text;
  const c4c::hir::Function* legacy_fn = module.resolve_function_decl(legacy_fn_ref);
  expect_true(legacy_fn != nullptr && legacy_fn->id.value == 1,
              "function decl-ref without a structured key hit should keep rendered fallback");
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
  legacy_global_ref.name_text_id = legacy_global_text;
  const c4c::hir::GlobalVar* legacy_global =
      module.resolve_global_decl(legacy_global_ref);
  expect_true(legacy_global != nullptr && legacy_global->id.value == 1,
              "global decl-ref without a structured key hit should keep rendered fallback");
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
                  &stale_template,
              "template definition lookup should preserve rendered fallback when declaration key is absent");

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
                  &stale_struct,
              "template struct definition lookup should preserve rendered fallback when declaration key is absent");

  c4c::Node stale_struct_spec = make_compile_time_registry_node(
      c4c::NK_STRUCT_DEF, "StaleTemplateStruct<int>",
      texts.intern("StaleTemplateStruct"));
  c4c::Node structured_struct_spec = make_compile_time_registry_node(
      c4c::NK_STRUCT_DEF, "StructuredTemplateStruct<int>",
      texts.intern("StructuredTemplateStruct"));
  state.register_template_struct_specialization("StaleTemplateStruct",
                                                &stale_struct_spec);
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
  expect_true(fallback_specs && fallback_specs->size() == 1 &&
                  (*fallback_specs)[0] == &stale_struct_spec,
              "template struct specialization lookup should preserve rendered fallback when owner key is absent");

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
                                       "stale_consteval") == &stale_consteval,
              "consteval definition lookup should preserve rendered fallback when declaration key is absent");
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
  query.tag = "StaleRenderedLayout";
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
  query.tag = "StaleBuiltinLayout";
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

}  // namespace

int main() {
  test_module_decl_lookup_records_each_authority();
  test_stale_rendered_names_do_not_override_authoritative_decl_lookup();
  test_direct_call_callee_lookup_uses_authoritative_decl_identity();
  test_operator_callee_lookup_uses_authoritative_decl_identity();
  test_range_for_method_callee_lookup_uses_authoritative_decl_identity();
  test_struct_owner_key_lookup_detects_stale_rendered_member_and_method_maps();
  test_compile_time_state_structured_registry_lookup_wins_over_stale_rendered_names();
  test_pending_consteval_nttp_handoff_carries_text_id_bindings();
  test_template_call_nttp_handoff_carries_text_id_bindings();
  test_template_global_nttp_init_uses_text_id_function_ctx_binding();
  test_static_member_nttp_const_eval_prefers_text_id_binding();
  test_consteval_record_layout_prefers_hir_owner_key_over_stale_tag();
  test_builtin_record_layout_prefers_hir_owner_key_over_stale_tag();
  std::cout << "PASS: frontend_hir_lookup_tests\n";
  return 0;
}
