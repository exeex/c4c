#include "hir/hir_ir.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
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
                  c4c::hir::NamespaceQualifier ns = {}) {
  c4c::hir::Function fn;
  fn.id = id;
  fn.name = std::move(name);
  fn.name_text_id = name_text_id;
  fn.link_name_id = link_name_id;
  fn.ns_qual = std::move(ns);
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

  add_function(module, c4c::hir::FunctionId{30}, "stale_direct_callee",
               stale_text);
  add_function(module, c4c::hir::FunctionId{31}, "structured_direct_callee",
               structured_text);
  add_function(module, c4c::hir::FunctionId{32}, "linked_direct_callee",
               linked_text, module.link_names.intern("linked_direct_callee"));

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
}

}  // namespace

int main() {
  test_module_decl_lookup_records_each_authority();
  test_stale_rendered_names_do_not_override_authoritative_decl_lookup();
  test_direct_call_callee_lookup_uses_authoritative_decl_identity();
  std::cout << "PASS: frontend_hir_lookup_tests\n";
  return 0;
}
