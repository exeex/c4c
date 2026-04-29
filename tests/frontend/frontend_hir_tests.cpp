#include "arena.hpp"
#include "hir_to_lir.hpp"
#include "hir/hir_ir.hpp"
#include "hir/hir_printer.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"
#include "target_profile.hpp"
#include "backend/bir/lir_to_bir/lowering.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

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

c4c::hir::Module lower_hir_module(std::string_view source,
                                  c4c::SourceProfile source_profile =
                                      c4c::SourceProfile::CppSubset) {
  c4c::Lexer lexer(std::string(source), c4c::lex_profile_from(source_profile));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     source_profile, "frontend_hir_tests.cpp");
  c4c::Node* root = parser.parse();
  auto result = c4c::sema::analyze_program(
      root, c4c::sema_profile_from(source_profile));

  expect_true(result.validation.ok,
              "HIR fixture source should parse and validate successfully");
  expect_true(result.hir_module.has_value(),
              "HIR fixture source should lower to a module");
  return *result.hir_module;
}

void overwrite_signature_name(std::string* signature_text,
                              std::string_view replacement_name) {
  const std::size_t at_pos = signature_text->find('@');
  expect_true(at_pos != std::string::npos,
              "fixture signature should contain an LLVM global symbol");
  const std::size_t paren_pos = signature_text->find('(', at_pos);
  expect_true(paren_pos != std::string::npos,
              "fixture signature should contain an argument list");
  signature_text->replace(at_pos, paren_pos - at_pos,
                          "@" + std::string(replacement_name));
}

c4c::codegen::lir::LirModule make_link_name_aware_lir_module() {
  c4c::codegen::lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  return module;
}

const c4c::hir::Param* find_param_by_name(const c4c::hir::Function& fn,
                                          std::string_view name) {
  const auto it = std::find_if(fn.params.begin(), fn.params.end(),
                               [&](const c4c::hir::Param& param) {
                                 return param.name == name;
                               });
  return it == fn.params.end() ? nullptr : &(*it);
}

const c4c::hir::Function* find_function_with_param_names(
    const c4c::hir::Module& module,
    std::string_view first_name,
    std::string_view second_name) {
  const auto it = std::find_if(
      module.functions.begin(), module.functions.end(),
      [&](const c4c::hir::Function& fn) {
        return find_param_by_name(fn, first_name) != nullptr &&
               find_param_by_name(fn, second_name) != nullptr;
      });
  return it == module.functions.end() ? nullptr : &(*it);
}

void test_dense_id_map_tracks_assigned_dense_ids() {
  c4c::hir::DenseIdMap<c4c::hir::LocalId, std::string> names;

  names.insert(c4c::hir::LocalId{0}, "lhs");
  names.insert(c4c::hir::LocalId{3}, "rhs");

  expect_true(names.contains(c4c::hir::LocalId{0}),
              "DenseIdMap should report assigned IDs as present");
  expect_true(!names.contains(c4c::hir::LocalId{2}),
              "DenseIdMap should keep gaps unassigned until explicitly inserted");
  expect_true(names.at(c4c::hir::LocalId{3}) == "rhs",
              "DenseIdMap should preserve values at typed-ID indexes");
  expect_true(names.size() == 4,
              "DenseIdMap should grow storage to cover the highest dense ID");
}

void test_optional_dense_id_map_supports_sparse_occupancy() {
  c4c::hir::OptionalDenseIdMap<c4c::hir::ExprId, int> counts;

  expect_true(counts.find(c4c::hir::ExprId{1}) == nullptr,
              "OptionalDenseIdMap should report missing IDs through find()");

  int& first = counts.get_or_create(c4c::hir::ExprId{1}, [] { return 7; });
  int& second = counts.get_or_create(c4c::hir::ExprId{1}, [] { return 99; });
  expect_true(&first == &second && second == 7,
              "OptionalDenseIdMap should not recreate an occupied slot");
  counts.insert(c4c::hir::ExprId{4}, 11);

  expect_true(*counts.find(c4c::hir::ExprId{1}) == 7,
              "OptionalDenseIdMap should preserve earlier values after later growth");
  expect_true(counts.contains(c4c::hir::ExprId{4}),
              "OptionalDenseIdMap should track explicitly inserted sparse IDs");
  expect_true(*counts.find(c4c::hir::ExprId{4}) == 11,
              "OptionalDenseIdMap should return inserted sparse values");
  expect_true(counts.size() == 5,
              "OptionalDenseIdMap should size itself by dense typed-ID index");
}

void test_dense_id_map_supports_local_type_storage() {
  c4c::hir::DenseIdMap<c4c::hir::LocalId, c4c::TypeSpec> local_types;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  c4c::TypeSpec float_ptr_ts{};
  float_ptr_ts.base = c4c::TB_FLOAT;
  float_ptr_ts.ptr_level = 1;

  local_types.insert(c4c::hir::LocalId{0}, int_ts);
  local_types.insert(c4c::hir::LocalId{2}, float_ptr_ts);

  expect_true(local_types.contains(c4c::hir::LocalId{0}),
              "DenseIdMap should report stored local type entries by LocalId");
  expect_true(!local_types.contains(c4c::hir::LocalId{1}),
              "DenseIdMap should keep unassigned local type slots absent");
  expect_true(local_types.at(c4c::hir::LocalId{2}).base == c4c::TB_FLOAT &&
                  local_types.at(c4c::hir::LocalId{2}).ptr_level == 1,
              "DenseIdMap should preserve TypeSpec values at dense local slots");
}

void test_optional_dense_id_map_supports_function_id_counters() {
  c4c::hir::OptionalDenseIdMap<c4c::hir::FunctionId, int> expand_counts;

  expect_true(!expand_counts.contains(c4c::hir::FunctionId{2}),
              "OptionalDenseIdMap should leave untouched FunctionId slots absent");

  int& first = expand_counts.get_or_create(c4c::hir::FunctionId{2}, [] { return 0; });
  ++first;
  int& second = expand_counts.get_or_create(c4c::hir::FunctionId{2}, [] { return 99; });
  ++second;
  expand_counts.insert(c4c::hir::FunctionId{5}, 7);

  expect_true(first == 2 && &first == &second,
              "OptionalDenseIdMap should preserve FunctionId-backed counter slots");
  expect_true(*expand_counts.find(c4c::hir::FunctionId{5}) == 7,
              "OptionalDenseIdMap should support sparse FunctionId insertions");
  expect_true(expand_counts.size() == 6,
              "OptionalDenseIdMap should grow to the highest FunctionId index");
}

void test_hir_materializes_link_name_ids_for_emitted_symbols() {
  const c4c::hir::Module module = lower_hir_module(R"cpp(
template<typename T>
T id(T value) { return value; }

int global_value = 7;

int add_one(int value) { return value + 1; }

int use_template() { return id<int>(add_one(global_value)); }
)cpp");

  expect_true(module.link_name_texts != nullptr,
              "HIR modules should own a live text table for link-name interning");

  const auto global_it = module.global_index.find("global_value");
  expect_true(global_it != module.global_index.end(),
              "fixture global should be present in the HIR global index");
  const c4c::hir::GlobalVar* global = module.find_global(global_it->second);
  expect_true(global != nullptr,
              "fixture global should resolve through the HIR global lookup");
  expect_true(global->link_name_id != c4c::kInvalidLinkName,
              "emitted globals should materialize a stable LinkNameId");
  expect_eq(module.link_names.spelling(global->link_name_id), global->name,
            "global LinkNameId should resolve back to the emitted global spelling");

  const auto fn_it = module.fn_index.find("add_one");
  expect_true(fn_it != module.fn_index.end(),
              "fixture function should be present in the HIR function index");
  const c4c::hir::Function* function = module.find_function(fn_it->second);
  expect_true(function != nullptr,
              "fixture function should resolve through the HIR function lookup");
  expect_true(function->link_name_id != c4c::kInvalidLinkName,
              "emitted functions should materialize a stable LinkNameId");
  expect_eq(module.link_names.spelling(function->link_name_id), function->name,
            "function LinkNameId should resolve back to the emitted function spelling");

  const auto tpl_it = module.template_defs.find("id");
  expect_true(tpl_it != module.template_defs.end(),
              "template metadata should still be recorded in HIR");
  expect_true(tpl_it->second.name_text_id != c4c::kInvalidText,
              "template metadata should materialize a stable TextId for the template name");
  expect_eq(module.link_name_texts->lookup(tpl_it->second.name_text_id), tpl_it->second.name,
            "template metadata TextId should resolve through the HIR module text table");
  expect_true(tpl_it->second.template_param_text_ids.size() ==
                  tpl_it->second.template_params.size(),
              "template metadata should preserve one parallel TextId per template parameter");
  expect_true(!tpl_it->second.template_param_text_ids.empty(),
              "template metadata should materialize TextIds for preserved template parameter spellings");
  expect_true(tpl_it->second.template_param_text_ids.front() != c4c::kInvalidText,
              "template parameter metadata should preserve a valid TextId");
  expect_eq(module.link_name_texts->lookup(tpl_it->second.template_param_text_ids.front()),
            tpl_it->second.template_params.front(),
            "template parameter TextIds should resolve through the HIR module text table");
  expect_true(!tpl_it->second.instances.empty(),
              "template metadata should capture realized instantiations");
  const c4c::hir::HirTemplateInstantiation& instance = tpl_it->second.instances.front();
  expect_true(instance.mangled_link_name_id != c4c::kInvalidLinkName,
              "materialized template instantiations should record a LinkNameId");
  expect_eq(module.link_names.spelling(instance.mangled_link_name_id),
            instance.mangled_name,
            "template-instantiation LinkNameId should resolve to the mangled emitted name");

  const auto inst_fn_it = module.fn_index.find(instance.mangled_name);
  expect_true(inst_fn_it != module.fn_index.end(),
              "template-instantiation metadata should match a lowered HIR function");
  const c4c::hir::Function* inst_fn = module.find_function(inst_fn_it->second);
  expect_true(inst_fn != nullptr && inst_fn->link_name_id == instance.mangled_link_name_id,
              "template-instantiation metadata and lowered HIR functions should share one LinkNameId");
}

void test_hir_materializes_decl_ref_link_name_ids_for_emitted_refs() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int global_value = 7;

int helper(int value) { return value + global_value; }

int (*pick_helper(void))(int) { return helper; }
)cpp");

  const auto helper_it = hir_module.fn_index.find("helper");
  expect_true(helper_it != hir_module.fn_index.end(),
              "fixture helper should be present in the HIR function index");
  const c4c::hir::Function* helper = hir_module.find_function(helper_it->second);
  expect_true(helper != nullptr && helper->link_name_id != c4c::kInvalidLinkName,
              "fixture helper should carry a stable function LinkNameId");

  const auto picker_it = hir_module.fn_index.find("pick_helper");
  expect_true(picker_it != hir_module.fn_index.end(),
              "fixture picker should be present in the HIR function index");
  const c4c::hir::Function* picker = hir_module.find_function(picker_it->second);
  expect_true(picker != nullptr,
              "fixture picker should resolve through the HIR function lookup");

  const c4c::hir::DeclRef* helper_ref = nullptr;
  for (const auto& block : picker->blocks) {
    for (const auto& stmt : block.stmts) {
      const auto* ret = std::get_if<c4c::hir::ReturnStmt>(&stmt.payload);
      if (ret == nullptr || !ret->expr) {
        continue;
      }
      helper_ref = std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[ret->expr->value].payload);
      if (helper_ref != nullptr) {
        break;
      }
    }
    if (helper_ref != nullptr) {
      break;
    }
  }
  expect_true(helper_ref != nullptr,
              "fixture picker should retain the helper function designator as a decl-ref");
  expect_true(helper_ref->link_name_id == helper->link_name_id,
              "function designator decl-refs should copy the emitted function LinkNameId");

  const auto global_it = hir_module.global_index.find("global_value");
  expect_true(global_it != hir_module.global_index.end(),
              "fixture global should be present in the HIR global index");
  const c4c::hir::GlobalVar* global = hir_module.find_global(global_it->second);
  expect_true(global != nullptr && global->link_name_id != c4c::kInvalidLinkName,
              "fixture global should carry a stable global LinkNameId");

  const auto global_ref_it = std::find_if(
      hir_module.expr_pool.begin(), hir_module.expr_pool.end(),
      [&](const c4c::hir::Expr& expr) {
        const auto* ref = std::get_if<c4c::hir::DeclRef>(&expr.payload);
        return ref != nullptr && ref->global && ref->global->value == global->id.value;
      });
  expect_true(global_ref_it != hir_module.expr_pool.end(),
              "fixture helper should lower at least one global decl-ref");
  const auto* global_ref = std::get_if<c4c::hir::DeclRef>(&global_ref_it->payload);
  expect_true(global_ref != nullptr && global_ref->link_name_id == global->link_name_id,
              "global decl-refs should copy the emitted global LinkNameId");
}

void test_hir_function_params_preserve_text_ids() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
consteval int fold(int value, int scale) { return value * scale; }

struct Widget {
  Widget(int other) {}
};

int use_params(int input) { return input + 1; }
)cpp");

  const auto use_it = hir_module.fn_index.find("use_params");
  expect_true(use_it != hir_module.fn_index.end(),
              "fixture function should be present in the HIR function index");
  const c4c::hir::Function* use_fn = hir_module.find_function(use_it->second);
  expect_true(use_fn != nullptr,
              "fixture function should resolve through the HIR function lookup");
  const c4c::hir::Param* input_param = find_param_by_name(*use_fn, "input");
  expect_true(input_param != nullptr,
              "ordinary lowered functions should retain their parameter metadata");
  expect_true(input_param->name_text_id != c4c::kInvalidText,
              "ordinary lowered parameters should preserve a parallel TextId");
  expect_eq(hir_module.link_name_texts->lookup(input_param->name_text_id), "input",
            "ordinary lowered parameter TextIds should resolve through the HIR text table");

  const auto fold_it = hir_module.fn_index.find("fold");
  expect_true(fold_it != hir_module.fn_index.end(),
              "consteval fixture should be present in the HIR function index");
  const c4c::hir::Function* fold_fn = hir_module.find_function(fold_it->second);
  expect_true(fold_fn != nullptr && fold_fn->consteval_only,
              "consteval fixture should lower through the bodyless consteval path");
  const c4c::hir::Param* value_param = find_param_by_name(*fold_fn, "value");
  const c4c::hir::Param* scale_param = find_param_by_name(*fold_fn, "scale");
  expect_true(value_param != nullptr && scale_param != nullptr,
              "consteval lowering should retain all declared parameter metadata");
  expect_true(value_param->name_text_id != c4c::kInvalidText,
              "consteval parameters should preserve a parallel TextId");
  expect_true(scale_param->name_text_id != c4c::kInvalidText,
              "consteval parameters should preserve TextIds for later parameters too");
  expect_eq(hir_module.link_name_texts->lookup(value_param->name_text_id), "value",
            "consteval parameter TextIds should resolve through the HIR text table");
  expect_eq(hir_module.link_name_texts->lookup(scale_param->name_text_id), "scale",
            "consteval parameter TextIds should resolve through the HIR text table");

  const c4c::hir::Function* ctor_fn =
      find_function_with_param_names(hir_module, "this", "other");
  expect_true(ctor_fn != nullptr,
              "method fixture should lower at least one function carrying both this and other params");
  const c4c::hir::Param* this_param = find_param_by_name(*ctor_fn, "this");
  const c4c::hir::Param* other_param = find_param_by_name(*ctor_fn, "other");
  expect_true(this_param != nullptr && other_param != nullptr,
              "method lowering should retain both synthetic and declared params");
  expect_true(this_param->name_text_id != c4c::kInvalidText,
              "synthetic this parameters should preserve a parallel TextId");
  expect_true(other_param->name_text_id != c4c::kInvalidText,
              "method parameters should preserve a parallel TextId");
  expect_eq(hir_module.link_name_texts->lookup(this_param->name_text_id), "this",
            "synthetic this parameter TextIds should resolve through the HIR text table");
  expect_eq(hir_module.link_name_texts->lookup(other_param->name_text_id), "other",
            "method parameter TextIds should resolve through the HIR text table");
}

void test_hir_namespace_qualifiers_preserve_text_ids_for_qualified_decl_refs() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
namespace ns {
namespace inner {
int helper(int value) { return value + 1; }
}

int call_inner(int value) { return inner::helper(value); }
}
)cpp");

  const auto caller_it = hir_module.fn_index.find("ns::call_inner");
  expect_true(caller_it != hir_module.fn_index.end(),
              "fixture caller should be present in the HIR function index");
  const c4c::hir::Function* caller = hir_module.find_function(caller_it->second);
  expect_true(caller != nullptr,
              "fixture caller should resolve through the HIR function lookup");

  const c4c::hir::DeclRef* callee_ref = nullptr;
  for (const auto& block : caller->blocks) {
    for (const auto& stmt : block.stmts) {
      const auto* ret = std::get_if<c4c::hir::ReturnStmt>(&stmt.payload);
      if (ret == nullptr || !ret->expr) {
        continue;
      }
      const auto* call =
          std::get_if<c4c::hir::CallExpr>(&hir_module.expr_pool[ret->expr->value].payload);
      if (call == nullptr) {
        continue;
      }
      callee_ref = std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[call->callee.value].payload);
      if (callee_ref != nullptr) {
        break;
      }
    }
    if (callee_ref != nullptr) {
      break;
    }
  }
  expect_true(callee_ref != nullptr,
              "qualified calls should preserve a decl-ref callee in HIR");
  expect_true(callee_ref->name_text_id != c4c::kInvalidText,
              "qualified decl-refs should preserve a parallel base-name TextId");
  expect_eq(hir_module.link_name_texts->lookup(callee_ref->name_text_id), "helper",
            "qualified decl-ref base-name TextIds should resolve through the HIR text table");
  expect_true(callee_ref->ns_qual.segments.size() == 1,
              "qualified decl-refs should preserve one namespace qualifier segment");
  expect_eq(callee_ref->ns_qual.segments[0], "inner",
            "qualified decl-refs should preserve namespace qualifier spelling");
  expect_true(callee_ref->ns_qual.segment_text_ids.size() == 1,
              "qualified decl-refs should preserve parallel NamespaceQualifier TextIds");
  expect_true(callee_ref->ns_qual.segment_text_ids[0] != c4c::kInvalidText,
              "qualified decl-refs should materialize a valid NamespaceQualifier TextId");
  expect_true(hir_module.link_name_texts != nullptr,
              "HIR modules should own the text table used by NamespaceQualifier TextIds");
  expect_eq(hir_module.link_name_texts->lookup(callee_ref->ns_qual.segment_text_ids[0]), "inner",
            "qualified decl-ref NamespaceQualifier TextIds should resolve through the HIR text table");
}

void test_hir_decl_stmt_decl_refs_preserve_text_ids_for_ctor_and_dtor_routes() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
struct Widget {
  Widget() {}
  ~Widget() {}
};

int make_widget() {
  Widget widget;
  return 0;
}
)cpp");

  const auto fn_it = hir_module.fn_index.find("make_widget");
  expect_true(fn_it != hir_module.fn_index.end(),
              "fixture function should be present in the HIR function index");
  const c4c::hir::Function* function = hir_module.find_function(fn_it->second);
  expect_true(function != nullptr,
              "fixture function should resolve through the HIR function lookup");

  bool saw_local_widget_ref = false;
  bool saw_link_named_callee_ref = false;
  for (const auto& expr : hir_module.expr_pool) {
    const auto* ref = std::get_if<c4c::hir::DeclRef>(&expr.payload);
    if (ref == nullptr) {
      continue;
    }
    if (ref->local.has_value() && ref->name == "widget") {
      saw_local_widget_ref = true;
      expect_true(ref->name_text_id != c4c::kInvalidText,
                  "decl-stmt local decl-refs should preserve a parallel TextId");
      expect_eq(hir_module.link_name_texts->lookup(ref->name_text_id), "widget",
                "decl-stmt local decl-ref TextIds should resolve through the HIR text table");
    }
    if (ref->link_name_id != c4c::kInvalidLinkName) {
      saw_link_named_callee_ref = true;
      expect_true(ref->name_text_id != c4c::kInvalidText,
                  "decl-stmt constructor and destructor callees should preserve a parallel TextId");
      expect_eq(hir_module.link_name_texts->lookup(ref->name_text_id), ref->name,
                "decl-stmt callee TextIds should resolve to the same emitted spelling");
    }
  }

  expect_true(saw_local_widget_ref,
              "fixture should lower at least one local decl-ref for the constructed object");
  expect_true(saw_link_named_callee_ref,
              "fixture should lower at least one constructor or destructor callee decl-ref");
}

void test_hir_stmt_decl_refs_preserve_text_ids_for_this_param_and_ctor_callees() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
struct Member {
  Member(int value) {}
};

struct Widget {
  Member member;

  Widget(int other) : member(other) {}
  Widget() : Widget(7) {}
  Widget& operator=(const Widget& other) = default;
};
)cpp");

  bool saw_this_param_ref = false;
  bool saw_other_param_ref = false;
  bool saw_link_named_callee_ref = false;
  for (const auto& expr : hir_module.expr_pool) {
    const auto* ref = std::get_if<c4c::hir::DeclRef>(&expr.payload);
    if (ref == nullptr) {
      continue;
    }
    if (ref->param_index.has_value() && ref->name == "this") {
      saw_this_param_ref = true;
      expect_true(ref->name_text_id != c4c::kInvalidText,
                  "statement-lowered this decl-refs should preserve a parallel TextId");
      expect_eq(hir_module.link_name_texts->lookup(ref->name_text_id), "this",
                "statement-lowered this decl-ref TextIds should resolve through the HIR text table");
    }
    if (ref->param_index.has_value() && ref->name == "other") {
      saw_other_param_ref = true;
      expect_true(ref->name_text_id != c4c::kInvalidText,
                  "statement-lowered parameter decl-refs should preserve a parallel TextId");
      expect_eq(hir_module.link_name_texts->lookup(ref->name_text_id), "other",
                "statement-lowered parameter decl-ref TextIds should resolve through the HIR text table");
    }
    if (ref->link_name_id != c4c::kInvalidLinkName) {
      saw_link_named_callee_ref = true;
      expect_true(ref->name_text_id != c4c::kInvalidText,
                  "statement-lowered constructor callees should preserve a parallel TextId");
      expect_eq(hir_module.link_name_texts->lookup(ref->name_text_id), ref->name,
                "statement-lowered callee TextIds should resolve to the same emitted spelling");
    }
  }

  expect_true(saw_this_param_ref,
              "fixture should lower at least one statement-built this decl-ref");
  expect_true(saw_other_param_ref,
              "fixture should lower at least one statement-built parameter decl-ref");
  expect_true(saw_link_named_callee_ref,
              "fixture should lower at least one statement-built constructor callee decl-ref");
}

void test_hir_struct_defs_preserve_text_ids_for_tags_and_bases() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
struct Base {};
struct Derived : Base {
  int member;
};

template<typename T>
struct Box : Derived {
  T value;
};

Box<int> make_box() { return {}; }
)cpp");

  const auto derived_it = hir_module.struct_defs.find("Derived");
  expect_true(derived_it != hir_module.struct_defs.end(),
              "fixture should lower the non-template derived struct definition");
  c4c::hir::HirStructDef& derived = derived_it->second;
  expect_true(derived.tag_text_id != c4c::kInvalidText,
              "non-template struct defs should preserve a parallel tag TextId");
  expect_eq(hir_module.link_name_texts->lookup(derived.tag_text_id), "Derived",
            "non-template struct tag TextIds should resolve through the HIR text table");
  expect_true(derived.base_tags.size() == 1,
              "fixture should lower one direct base tag on the derived struct");
  expect_true(derived.base_tag_text_ids.size() == derived.base_tags.size(),
              "struct defs should preserve one base-tag TextId per lowered base tag");
  expect_true(derived.base_tag_text_ids.front() != c4c::kInvalidText,
              "non-template struct base tags should preserve parallel TextIds");
  expect_eq(hir_module.link_name_texts->lookup(derived.base_tag_text_ids.front()), "Base",
            "non-template struct base-tag TextIds should resolve through the HIR text table");
  expect_true(derived.fields.size() == 1,
              "fixture should lower one named field on the derived struct");
  expect_true(derived.fields.front().field_text_id != c4c::kInvalidText,
              "struct defs should preserve a parallel TextId for lowered field names");
  expect_eq(hir_module.link_name_texts->lookup(derived.fields.front().field_text_id), "member",
            "struct field TextIds should resolve through the HIR text table");

  derived.fields.front().name = "__corrupted_field_name__";
  const std::string rendered_hir = c4c::hir::format_hir(hir_module);
  expect_true(rendered_hir.find("field member: int") != std::string::npos,
              "HIR printer should prefer struct field TextIds over corrupted raw field strings");

  const auto instantiated_it = std::find_if(
      hir_module.struct_defs.begin(), hir_module.struct_defs.end(),
      [](const auto& entry) {
        return entry.first != "Base" && entry.first != "Derived" &&
               std::find(entry.second.base_tags.begin(), entry.second.base_tags.end(), "Derived") !=
                   entry.second.base_tags.end();
      });
  expect_true(instantiated_it != hir_module.struct_defs.end(),
              "fixture should instantiate a template struct definition that inherits from Derived");
  const c4c::hir::HirStructDef& instantiated = instantiated_it->second;
  expect_true(instantiated.tag_text_id != c4c::kInvalidText,
              "instantiated template struct defs should preserve a parallel tag TextId");
  expect_eq(hir_module.link_name_texts->lookup(instantiated.tag_text_id), instantiated.tag,
            "instantiated template struct tag TextIds should resolve to the lowered HIR tag");
  expect_true(instantiated.base_tag_text_ids.size() == instantiated.base_tags.size(),
              "instantiated template struct defs should preserve one base-tag TextId per lowered base");
  const auto derived_base_it =
      std::find(instantiated.base_tags.begin(), instantiated.base_tags.end(), "Derived");
  expect_true(derived_base_it != instantiated.base_tags.end(),
              "instantiated template struct should preserve its inherited base tag");
  const size_t derived_base_index =
      static_cast<size_t>(derived_base_it - instantiated.base_tags.begin());
  expect_true(instantiated.base_tag_text_ids[derived_base_index] != c4c::kInvalidText,
              "instantiated template struct base tags should preserve parallel TextIds");
  expect_eq(hir_module.link_name_texts->lookup(
                instantiated.base_tag_text_ids[derived_base_index]),
            "Derived",
            "instantiated template struct base-tag TextIds should resolve through the HIR text table");
  expect_true(instantiated.fields.size() == 1,
              "fixture should instantiate one named field on the template struct");
  expect_true(instantiated.fields.front().field_text_id != c4c::kInvalidText,
              "instantiated template struct fields should preserve parallel TextIds");
  expect_eq(hir_module.link_name_texts->lookup(instantiated.fields.front().field_text_id), "value",
            "instantiated template struct field TextIds should resolve through the HIR text table");

  c4c::hir::HirStructDef instantiated_copy = instantiated;
  instantiated_copy.fields.front().name = "__corrupted_instantiated_field_name__";
  hir_module.struct_defs[instantiated_it->first] = instantiated_copy;
  const std::string rendered_instantiated_hir = c4c::hir::format_hir(hir_module);
  expect_true(rendered_instantiated_hir.find("field value: int") != std::string::npos,
              "HIR printer should prefer instantiated struct field TextIds over corrupted raw field strings");
}

void test_hir_template_calls_preserve_text_ids_for_source_template_names() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
template<typename T>
T id(T value) { return value; }

int use_template() { return id<int>(7); }
)cpp");

  const auto caller_it = hir_module.fn_index.find("use_template");
  expect_true(caller_it != hir_module.fn_index.end(),
              "fixture caller should be present in the HIR function index");
  const c4c::hir::Function* caller = hir_module.find_function(caller_it->second);
  expect_true(caller != nullptr,
              "fixture caller should resolve through the HIR function lookup");

  const c4c::hir::TemplateCallInfo* template_info = nullptr;
  for (const auto& block : caller->blocks) {
    for (const auto& stmt : block.stmts) {
      const auto* ret = std::get_if<c4c::hir::ReturnStmt>(&stmt.payload);
      if (ret == nullptr || !ret->expr) {
        continue;
      }
      const auto* call =
          std::get_if<c4c::hir::CallExpr>(&hir_module.expr_pool[ret->expr->value].payload);
      if (call == nullptr || !call->template_info.has_value()) {
        continue;
      }
      template_info = &*call->template_info;
      break;
    }
    if (template_info != nullptr) {
      break;
    }
  }

  expect_true(template_info != nullptr,
              "fixture caller should retain template-call metadata on the lowered call");
  expect_true(template_info->source_template_text_id != c4c::kInvalidText,
              "template-call metadata should preserve a parallel TextId for the source template name");
  expect_eq(hir_module.link_name_texts->lookup(template_info->source_template_text_id),
            template_info->source_template,
            "template-call source-template TextIds should resolve through the HIR text table");
}

void test_hir_consteval_call_metadata_preserves_text_ids_for_function_names() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
consteval int fold(int value, int scale) { return value * scale; }

int use_consteval() { return fold(3, 4); }
)cpp");

  expect_true(hir_module.consteval_calls.size() == 1,
              "fixture should lower exactly one evaluated consteval call");
  const c4c::hir::ConstevalCallInfo& consteval_call = hir_module.consteval_calls.front();
  expect_true(consteval_call.fn_name_text_id != c4c::kInvalidText,
              "consteval-call metadata should preserve a parallel TextId for the function name");
  expect_eq(hir_module.link_name_texts->lookup(consteval_call.fn_name_text_id),
            consteval_call.fn_name,
            "consteval-call function-name TextIds should resolve through the HIR text table");
}

void test_hir_member_exprs_preserve_text_ids_for_field_names() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
struct Widget {
  int value;
};

int read_value(Widget* widget) { return widget->value; }
)cpp");

  const c4c::hir::MemberExpr* member_expr = nullptr;
  for (const auto& expr : hir_module.expr_pool) {
    member_expr = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
    if (member_expr != nullptr && member_expr->field == "value") {
      break;
    }
    member_expr = nullptr;
  }

  expect_true(member_expr != nullptr,
              "fixture should lower at least one MemberExpr for the selected field access");
  expect_true(member_expr->field_text_id != c4c::kInvalidText,
              "member expressions should preserve a parallel TextId for the field name");
  expect_eq(hir_module.link_name_texts->lookup(member_expr->field_text_id),
            member_expr->field,
            "member-expression field TextIds should resolve through the HIR text table");
}

void test_hir_global_init_designators_preserve_text_ids_for_field_names() {
  c4c::hir::Module hir_module = lower_hir_module(R"c(
struct Triple {
  int left;
  int middle;
  int right;
};

struct Triple triple = {.right = 7};
)c",
                                                  c4c::SourceProfile::C);

  const auto global_it = hir_module.global_index.find("triple");
  expect_true(global_it != hir_module.global_index.end(),
              "fixture global should be present in the HIR global index");
  c4c::hir::GlobalVar* global = hir_module.find_global(global_it->second);
  expect_true(global != nullptr,
              "fixture global should resolve through the mutable HIR global lookup");

  auto* init_list = std::get_if<c4c::hir::InitList>(&global->init);
  expect_true(init_list != nullptr && !init_list->items.empty(),
              "fixture global should lower to a non-empty designated init list");

  c4c::hir::InitListItem& item = init_list->items.front();
  expect_true(item.field_designator.has_value(),
              "designated init items should preserve the legacy field spelling during migration");
  expect_true(item.field_designator_text_id != c4c::kInvalidText,
              "designated init items should preserve a parallel TextId for the field name");
  expect_eq(hir_module.link_name_texts->lookup(item.field_designator_text_id), "right",
            "init-list field designator TextIds should resolve through the HIR text table");

  item.field_designator = "broken_right";

  const std::string hir_dump = c4c::hir::format_hir(hir_module);
  expect_true(hir_dump.find(".right = 7") != std::string::npos,
              "HIR printer should recover designated field spellings from TextId lookup");
  expect_true(hir_dump.find("broken_right") == std::string::npos,
              "HIR printer should not trust a corrupted raw field designator");

  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);
  const auto lir_global_it = std::find_if(
      lir_module.globals.begin(), lir_module.globals.end(),
      [](const c4c::codegen::lir::LirGlobal& global) { return global.name == "triple"; });
  expect_true(lir_global_it != lir_module.globals.end(),
              "fixture global should lower into a concrete LIR global carrier");
  expect_eq(lir_global_it->init_text, "{ i32 zeroinitializer, i32 zeroinitializer, i32 7 }",
            "LIR const-init lowering should recover the designated field from TextId lookup");
}

void test_hir_to_lir_forwards_function_link_name_ids() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
template<typename T>
T id(T value) { return value; }

int global_value = 7;

int add_one(int value) { return value + 1; }

int use_template() { return id<int>(add_one(global_value)); }
)cpp");
  const c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);

  const auto hir_fn_it = hir_module.fn_index.find("add_one");
  expect_true(hir_fn_it != hir_module.fn_index.end(),
              "fixture function should still be present in the HIR function index");
  const c4c::hir::Function* hir_function = hir_module.find_function(hir_fn_it->second);
  expect_true(hir_function != nullptr,
              "fixture function should resolve through the HIR lookup before lowering to LIR");
  expect_true(hir_function->link_name_id != c4c::kInvalidLinkName,
              "fixture function should carry a valid HIR LinkNameId before LIR lowering");

  const auto lir_fn_it = std::find_if(lir_module.functions.begin(), lir_module.functions.end(),
                                      [](const c4c::codegen::lir::LirFunction& fn) {
                                        return fn.name == "add_one";
                                      });
  expect_true(lir_fn_it != lir_module.functions.end(),
              "fixture function should lower into a concrete LIR function carrier");
  expect_true(lir_fn_it->link_name_id == hir_function->link_name_id,
              "LIR functions should forward the HIR LinkNameId on the direct emitted-function path");

  const auto hir_global_it = hir_module.global_index.find("global_value");
  expect_true(hir_global_it != hir_module.global_index.end(),
              "fixture global should still be present in the HIR global index");
  const c4c::hir::GlobalVar* hir_global = hir_module.find_global(hir_global_it->second);
  expect_true(hir_global != nullptr,
              "fixture global should resolve through the HIR lookup before lowering to LIR");
  expect_true(hir_global->link_name_id != c4c::kInvalidLinkName,
              "fixture global should carry a valid HIR LinkNameId before LIR lowering");

  const auto lir_global_it = std::find_if(lir_module.globals.begin(), lir_module.globals.end(),
                                          [](const c4c::codegen::lir::LirGlobal& global) {
                                            return global.name == "global_value";
                                          });
  expect_true(lir_global_it != lir_module.globals.end(),
              "fixture global should lower into a concrete LIR global carrier");
  expect_true(lir_global_it->link_name_id == hir_global->link_name_id,
              "LIR globals should forward the HIR LinkNameId on the direct emitted-global path");
}

void test_hir_to_lir_dead_internal_reachability_prefers_link_name_ids() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
static int helper(int value) { return value + 1; }

int call_helper(int value) { return helper(value); }
)cpp");

  const auto helper_it = hir_module.fn_index.find("helper");
  expect_true(helper_it != hir_module.fn_index.end(),
              "fixture helper should be present in the HIR function index");
  c4c::hir::Function* helper = hir_module.find_function(helper_it->second);
  expect_true(helper != nullptr && helper->link_name_id != c4c::kInvalidLinkName,
              "fixture static helper should carry a stable HIR LinkNameId");
  const c4c::LinkNameId helper_link_name_id = helper->link_name_id;

  helper->name = "broken_helper_definition_name";

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_helper_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id == helper_link_name_id;
      });
  expect_true(lir_helper_it != lir_module.functions.end(),
              "dead-internal elimination should keep a static helper reached by semantic LinkNameId");
  expect_true(lir_helper_it->can_elide_if_unreferenced,
              "fixture helper should still exercise the discardable-function reachability path");

  const auto lir_caller_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "call_helper";
      });
  expect_true(lir_caller_it != lir_module.functions.end(),
              "fixture caller should lower as the non-discardable reachability seed");

  const c4c::codegen::lir::LirCallOp* direct_call = nullptr;
  for (const auto& block : lir_caller_it->blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (call != nullptr &&
          call->direct_callee_link_name_id == helper_link_name_id) {
        direct_call = call;
        break;
      }
    }
    if (direct_call != nullptr) {
      break;
    }
  }
  expect_true(direct_call != nullptr,
              "fixture caller should preserve the static helper direct-call LinkNameId");
  expect_true(direct_call->callee == "@helper",
              "fixture should recover the direct-call spelling from semantic identity");
}

void test_hir_to_lir_direct_call_target_resolution_prefers_link_name_ids() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int helper(int value) { return value + 1; }

int call_helper(int value) { return helper(value); }
)cpp");

  const auto helper_it = hir_module.fn_index.find("helper");
  expect_true(helper_it != hir_module.fn_index.end(),
              "fixture helper should be present in the HIR function index");
  const c4c::hir::Function* helper = hir_module.find_function(helper_it->second);
  expect_true(helper != nullptr && helper->link_name_id != c4c::kInvalidLinkName,
              "fixture helper should carry a stable HIR LinkNameId before call lowering");

  const auto caller_it = hir_module.fn_index.find("call_helper");
  expect_true(caller_it != hir_module.fn_index.end(),
              "fixture caller should be present in the HIR function index");
  c4c::hir::Function* caller = hir_module.find_function(caller_it->second);
  expect_true(caller != nullptr,
              "fixture caller should resolve through the HIR function lookup");

  c4c::hir::DeclRef* callee_ref = nullptr;
  for (auto& block : caller->blocks) {
    for (auto& stmt : block.stmts) {
      auto* ret = std::get_if<c4c::hir::ReturnStmt>(&stmt.payload);
      if (ret == nullptr || !ret->expr) {
        continue;
      }
      auto* call = std::get_if<c4c::hir::CallExpr>(&hir_module.expr_pool[ret->expr->value].payload);
      if (call == nullptr) {
        continue;
      }
      callee_ref = std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[call->callee.value].payload);
      if (callee_ref != nullptr) {
        break;
      }
    }
    if (callee_ref != nullptr) {
      break;
    }
  }
  expect_true(callee_ref != nullptr,
              "fixture caller should retain a decl-ref callee before LIR lowering");
  expect_true(callee_ref->link_name_id == helper->link_name_id,
              "fixture decl-ref callee should already carry the helper LinkNameId");

  callee_ref->name = "broken_helper_declref_name";

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_caller_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "call_helper";
      });
  expect_true(lir_caller_it != lir_module.functions.end(),
              "fixture caller should still lower through the semantic LinkNameId route");

  const c4c::codegen::lir::LirCallOp* direct_call = nullptr;
  for (const auto& block : lir_caller_it->blocks) {
    auto inst_it = std::find_if(block.insts.begin(), block.insts.end(),
                                [](const c4c::codegen::lir::LirInst& inst) {
                                  return std::holds_alternative<c4c::codegen::lir::LirCallOp>(inst);
                                });
    if (inst_it == block.insts.end()) {
      continue;
    }
    direct_call = std::get_if<c4c::codegen::lir::LirCallOp>(&*inst_it);
    if (direct_call != nullptr) {
      break;
    }
  }
  expect_true(direct_call != nullptr,
              "fixture caller should still lower the direct call after the raw decl-ref name is corrupted");
  expect_true(direct_call->direct_callee_link_name_id == helper->link_name_id,
              "direct-call lowering should preserve the semantic LinkNameId from the HIR decl-ref");
  expect_true(direct_call->callee == "@helper",
              "direct-call lowering should recover the semantic callee symbol from LinkNameId");
  expect_true(direct_call->callee != "@broken_helper_declref_name",
              "direct-call lowering should not trust a corrupted raw decl-ref name when LinkNameId is available");
}

void test_hir_to_lir_decl_backed_function_designator_rvalues_prefer_link_name_ids() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
extern int helper(int value);

int (*pick_helper(void))(int) { return helper; }
)cpp");

  const auto helper_it = hir_module.fn_index.find("helper");
  expect_true(helper_it != hir_module.fn_index.end(),
              "fixture helper declaration should be present in the HIR function index");
  const c4c::hir::Function* helper = hir_module.find_function(helper_it->second);
  expect_true(helper != nullptr && helper->link_name_id != c4c::kInvalidLinkName,
              "fixture helper declaration should carry a stable HIR LinkNameId");

  const auto picker_it = hir_module.fn_index.find("pick_helper");
  expect_true(picker_it != hir_module.fn_index.end(),
              "fixture picker should be present in the HIR function index");
  c4c::hir::Function* picker = hir_module.find_function(picker_it->second);
  expect_true(picker != nullptr,
              "fixture picker should resolve through the HIR function lookup");

  c4c::hir::DeclRef* returned_ref = nullptr;
  for (auto& block : picker->blocks) {
    for (auto& stmt : block.stmts) {
      auto* ret = std::get_if<c4c::hir::ReturnStmt>(&stmt.payload);
      if (ret == nullptr || !ret->expr) {
        continue;
      }
      returned_ref = std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[ret->expr->value].payload);
      if (returned_ref != nullptr) {
        break;
      }
    }
    if (returned_ref != nullptr) {
      break;
    }
  }
  expect_true(returned_ref != nullptr,
              "fixture picker should return the helper through a decl-ref designator");
  expect_true(returned_ref->link_name_id == helper->link_name_id,
              "fixture decl-ref designator should already carry the helper LinkNameId");

  returned_ref->name = "broken_helper_designator_name";

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_picker_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "pick_helper";
      });
  expect_true(lir_picker_it != lir_module.functions.end(),
              "fixture picker should still lower through the semantic LinkNameId route");
  expect_true(!lir_picker_it->blocks.empty(),
              "fixture picker should still lower into a concrete LIR body");

  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&lir_picker_it->blocks.back().terminator);
  expect_true(ret != nullptr,
              "fixture picker should still end with a concrete LIR return terminator");
  expect_true(ret->type_str == "ptr",
              "function-designator return should still lower as a pointer-valued return");
  expect_true(ret->value_str.has_value() && *ret->value_str == "@helper",
              "function-designator lowering should recover the semantic helper symbol from LinkNameId");
  expect_true(!ret->value_str.has_value() || *ret->value_str != "@broken_helper_designator_name",
              "function-designator lowering should not trust a corrupted raw decl-ref name");
}

void test_hir_to_lir_decl_backed_call_result_inference_prefers_link_name_ids() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int helper(int value) { return value + 1; }

int (*pick_helper(void))(int) { return helper; }

int call_helper(int value) { return pick_helper()(value); }
)cpp");

  const auto picker_it = hir_module.fn_index.find("pick_helper");
  expect_true(picker_it != hir_module.fn_index.end(),
              "fixture picker should be present in the HIR function index");
  const c4c::hir::Function* picker = hir_module.find_function(picker_it->second);
  expect_true(picker != nullptr && picker->link_name_id != c4c::kInvalidLinkName,
              "fixture picker should carry a stable HIR LinkNameId");

  const auto caller_it = hir_module.fn_index.find("call_helper");
  expect_true(caller_it != hir_module.fn_index.end(),
              "fixture caller should be present in the HIR function index");
  c4c::hir::Function* caller = hir_module.find_function(caller_it->second);
  expect_true(caller != nullptr,
              "fixture caller should resolve through the HIR function lookup");

  c4c::hir::DeclRef* picker_ref = nullptr;
  for (auto& block : caller->blocks) {
    for (auto& stmt : block.stmts) {
      auto* ret = std::get_if<c4c::hir::ReturnStmt>(&stmt.payload);
      if (ret == nullptr || !ret->expr) {
        continue;
      }
      auto* outer_call =
          std::get_if<c4c::hir::CallExpr>(&hir_module.expr_pool[ret->expr->value].payload);
      if (outer_call == nullptr) {
        continue;
      }
      auto* inner_call =
          std::get_if<c4c::hir::CallExpr>(&hir_module.expr_pool[outer_call->callee.value].payload);
      if (inner_call == nullptr) {
        continue;
      }
      picker_ref = std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[inner_call->callee.value].payload);
      if (picker_ref != nullptr) {
        break;
      }
    }
    if (picker_ref != nullptr) {
      break;
    }
  }
  expect_true(picker_ref != nullptr,
              "fixture caller should retain the picker callee before LIR lowering");
  expect_true(picker_ref->link_name_id == picker->link_name_id,
              "fixture nested decl-ref callee should already carry the picker LinkNameId");

  picker_ref->name = "broken_pick_helper_declref_name";

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_caller_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "call_helper";
      });
  expect_true(lir_caller_it != lir_module.functions.end(),
              "fixture caller should still lower through the semantic LinkNameId route");

  const c4c::codegen::lir::LirCallOp* pick_call = nullptr;
  const c4c::codegen::lir::LirCallOp* helper_call = nullptr;
  for (const auto& block : lir_caller_it->blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (call == nullptr) {
        continue;
      }
      if (call->direct_callee_link_name_id == picker->link_name_id) {
        pick_call = call;
      } else if (call->direct_callee_link_name_id == c4c::kInvalidLinkName &&
                 !call->callee.empty() && call->callee.str().front() == '%') {
        helper_call = call;
      }
    }
  }

  expect_true(pick_call != nullptr,
              "fixture caller should still lower the picker call after the raw decl-ref name is corrupted");
  expect_true(pick_call->callee == "@pick_helper",
              "picker lowering should recover the semantic callee name from LinkNameId");
  expect_true(pick_call->callee != "@broken_pick_helper_declref_name",
              "picker lowering should not trust a corrupted raw decl-ref name");

  expect_true(helper_call != nullptr,
              "fixture caller should still lower the nested indirect helper call after call-result inference");
  expect_true(helper_call->return_type == "i32",
              "call-result inference should preserve the nested helper call return type");
  expect_true(!helper_call->callee_type_suffix.empty(),
              "call-result inference should preserve the nested helper function-pointer signature");
}

void test_hir_to_lir_object_helper_callees_prefer_link_name_ids() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
unsigned char g_storage[64];

void* operator_new(unsigned long size) { return (void*)g_storage; }

void operator_delete(void* ptr) {}

struct Box {
  int value;

  Box(int v) { value = v; }
};

int main() {
  Box* heap = new Box(5);
  delete heap;
  return 0;
}
)cpp");

  const auto user_it = hir_module.fn_index.find("main");
  expect_true(user_it != hir_module.fn_index.end(),
              "fixture user should be present in the HIR function index");
  c4c::hir::Function* user_fn = hir_module.find_function(user_it->second);
  expect_true(user_fn != nullptr,
              "fixture user should resolve through the HIR function lookup");

  c4c::hir::DeclRef* callee_ref = nullptr;
  for (auto& block : user_fn->blocks) {
    for (auto& stmt : block.stmts) {
      const auto* expr_stmt = std::get_if<c4c::hir::ExprStmt>(&stmt.payload);
      if (expr_stmt == nullptr || !expr_stmt->expr) {
        continue;
      }
      const auto* call =
          std::get_if<c4c::hir::CallExpr>(&hir_module.expr_pool[expr_stmt->expr->value].payload);
      if (call == nullptr) {
        continue;
      }
      callee_ref = std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[call->callee.value].payload);
      if (callee_ref != nullptr && callee_ref->name == "operator_delete") {
        break;
      }
    }
    if (callee_ref != nullptr) {
      break;
    }
  }
  expect_true(callee_ref != nullptr,
              "fixture object lowering should synthesize a decl-ref callee for operator_delete");
  expect_true(callee_ref->link_name_id != c4c::kInvalidLinkName,
              "object helper decl-ref callees should carry a semantic LinkNameId");

  const std::string semantic_name =
      std::string(hir_module.link_names.spelling(callee_ref->link_name_id));
  expect_true(!semantic_name.empty(),
              "object helper LinkNameId should resolve through the shared link-name table");
  callee_ref->name = "broken_operator_delete_declref_name";

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_user_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "main";
      });
  expect_true(lir_user_it != lir_module.functions.end(),
              "fixture user should still lower through the semantic LinkNameId route");

  const c4c::codegen::lir::LirCallOp* helper_call = nullptr;
  for (const auto& block : lir_user_it->blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (call == nullptr) {
        continue;
      }
      if (call->direct_callee_link_name_id == callee_ref->link_name_id) {
        helper_call = call;
        break;
      }
    }
    if (helper_call != nullptr) {
      break;
    }
  }

  expect_true(helper_call != nullptr,
              "object helper lowering should preserve the helper decl-ref into a direct LIR call");
  expect_true(helper_call->callee == "@" + semantic_name,
              "object helper lowering should recover the semantic helper spelling from LinkNameId");
  expect_true(helper_call->callee != "@broken_operator_delete_declref_name",
              "object helper lowering should not trust a corrupted raw decl-ref name");
}

void test_hir_to_lir_template_call_helper_callees_prefer_carrier_link_name_ids() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
template<typename T>
T id(T value) { return value; }

int use_template() { return id<int>(7); }
)cpp");

  const auto caller_it = hir_module.fn_index.find("use_template");
  expect_true(caller_it != hir_module.fn_index.end(),
              "fixture caller should be present in the HIR function index");
  c4c::hir::Function* caller = hir_module.find_function(caller_it->second);
  expect_true(caller != nullptr,
              "fixture caller should resolve through the HIR function lookup");

  c4c::hir::DeclRef* callee_ref = nullptr;
  for (auto& block : caller->blocks) {
    for (auto& stmt : block.stmts) {
      auto* ret = std::get_if<c4c::hir::ReturnStmt>(&stmt.payload);
      if (ret == nullptr || !ret->expr) {
        continue;
      }
      auto* call = std::get_if<c4c::hir::CallExpr>(&hir_module.expr_pool[ret->expr->value].payload);
      if (call == nullptr) {
        continue;
      }
      callee_ref = std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[call->callee.value].payload);
      if (callee_ref != nullptr) {
        break;
      }
    }
    if (callee_ref != nullptr) {
      break;
    }
  }
  expect_true(callee_ref != nullptr,
              "template-call lowering should synthesize a decl-ref callee");
  expect_true(callee_ref->link_name_id != c4c::kInvalidLinkName,
              "template-call helper decl-ref callees should copy a semantic carrier LinkNameId");

  const auto callee_it = hir_module.fn_index.find(callee_ref->name);
  expect_true(callee_it != hir_module.fn_index.end(),
              "fixture template callee should already be materialized in the HIR function index");
  const c4c::hir::Function* callee_fn = hir_module.find_function(callee_it->second);
  expect_true(callee_fn != nullptr && callee_fn->link_name_id == callee_ref->link_name_id,
              "template-call helper decl-ref callees should reuse the emitted function carrier LinkNameId");

  const std::string semantic_name =
      std::string(hir_module.link_names.spelling(callee_ref->link_name_id));
  expect_true(!semantic_name.empty(),
              "template-call helper LinkNameId should resolve through the shared link-name table");

  callee_ref->name = "broken_template_call_declref_name";

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_caller_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "use_template";
      });
  expect_true(lir_caller_it != lir_module.functions.end(),
              "fixture caller should still lower through the semantic LinkNameId route");

  const c4c::codegen::lir::LirCallOp* helper_call = nullptr;
  for (const auto& block : lir_caller_it->blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (call == nullptr) {
        continue;
      }
      if (call->direct_callee_link_name_id == callee_ref->link_name_id) {
        helper_call = call;
        break;
      }
    }
    if (helper_call != nullptr) {
      break;
    }
  }

  expect_true(helper_call != nullptr,
              "template-call lowering should preserve the helper decl-ref into a direct LIR call");
  expect_true(helper_call->callee == "@" + semantic_name,
              "template-call lowering should recover the semantic helper spelling from LinkNameId");
  expect_true(helper_call->callee != "@broken_template_call_declref_name",
              "template-call lowering should not trust a corrupted raw decl-ref name");
}

void test_hir_direct_call_builtin_alias_fallback_keeps_invalid_link_name_ids() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int main() {
  __builtin_abort();
  return 0;
}
)cpp");

  const auto main_it = hir_module.fn_index.find("main");
  expect_true(main_it != hir_module.fn_index.end(),
              "fixture main should be present in the HIR function index");
  c4c::hir::Function* main_fn = hir_module.find_function(main_it->second);
  expect_true(main_fn != nullptr,
              "fixture main should resolve through the HIR function lookup");

  const c4c::hir::DeclRef* callee_ref = nullptr;
  for (const auto& block : main_fn->blocks) {
    for (const auto& stmt : block.stmts) {
      const auto* expr_stmt = std::get_if<c4c::hir::ExprStmt>(&stmt.payload);
      if (expr_stmt == nullptr || !expr_stmt->expr) {
        continue;
      }
      const auto* call =
          std::get_if<c4c::hir::CallExpr>(&hir_module.expr_pool[expr_stmt->expr->value].payload);
      if (call == nullptr || call->builtin_id != c4c::BuiltinId::Abort) {
        continue;
      }
      callee_ref = std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[call->callee.value].payload);
      if (callee_ref != nullptr) {
        break;
      }
    }
    if (callee_ref != nullptr) {
      break;
    }
  }

  expect_true(callee_ref != nullptr,
              "builtin alias lowering should retain a decl-ref callee in HIR");
  expect_true(callee_ref->link_name_id == c4c::kInvalidLinkName,
              "builtin alias callees should stay on the invalid-id fallback when no semantic carrier exists");
}

void test_lir_printer_resolves_link_names_at_emission_boundary() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int global_value = 7;

int add_one(int value) { return value + global_value; }
)cpp");
  c4c::codegen::lir::LirModule lir_module =
      c4c::codegen::lir::lower(hir_module);

  auto lir_fn_it = std::find_if(lir_module.functions.begin(), lir_module.functions.end(),
                                [](const c4c::codegen::lir::LirFunction& fn) {
                                  return fn.link_name_id != c4c::kInvalidLinkName;
                                });
  expect_true(lir_fn_it != lir_module.functions.end(),
              "fixture should lower at least one emitted function with a LinkNameId");
  lir_fn_it->name = "broken_add_one_name";
  overwrite_signature_name(&lir_fn_it->signature_text, "broken_add_one_signature");

  auto lir_global_it = std::find_if(lir_module.globals.begin(), lir_module.globals.end(),
                                    [](const c4c::codegen::lir::LirGlobal& global) {
                                      return global.link_name_id != c4c::kInvalidLinkName;
                                    });
  expect_true(lir_global_it != lir_module.globals.end(),
              "fixture should lower at least one emitted global with a LinkNameId");
  lir_global_it->name = "broken_global_value";

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("@add_one(") != std::string::npos,
              "printer should recover the function spelling from LinkNameId lookup");
  expect_true(llvm_ir.find("@global_value = ") != std::string::npos,
              "printer should recover the global spelling from LinkNameId lookup");
  expect_true(llvm_ir.find("broken_add_one_signature") == std::string::npos,
              "printer should not trust a corrupted function signature name");
  expect_true(llvm_ir.find("broken_global_value") == std::string::npos,
              "printer should not trust a corrupted global name carrier");
}

void test_lir_printer_resolves_specialization_metadata_link_names() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
template<typename T>
T id(T value) { return value; }

int use_template() { return id<int>(7); }
)cpp");
  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto spec_it = std::find_if(
      lir_module.spec_entries.begin(), lir_module.spec_entries.end(),
      [](const c4c::codegen::lir::LirSpecEntry& entry) {
        return entry.mangled_link_name_id != c4c::kInvalidLinkName;
      });
  expect_true(spec_it != lir_module.spec_entries.end(),
              "fixture should lower at least one specialization metadata entry with a LinkNameId");

  const std::string resolved_name =
      std::string(lir_module.link_names.spelling(spec_it->mangled_link_name_id));
  expect_true(!resolved_name.empty(),
              "specialization metadata LinkNameId should resolve through the shared link-name table");

  auto broken_spec_it = spec_it;
  broken_spec_it->mangled_name = "broken_specialization_name";

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("!\"" + resolved_name + "\"") != std::string::npos,
              "printer should recover specialization metadata names from LinkNameId lookup");
  expect_true(llvm_ir.find("broken_specialization_name") == std::string::npos,
              "printer should not trust a corrupted specialization metadata name");
}

void test_lir_printer_resolves_direct_call_link_names_at_emission_boundary() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int helper(int value) { return value + 1; }

int call_helper(int value) { return helper(value); }
)cpp");
  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  auto caller_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "call_helper";
      });
  expect_true(caller_it != lir_module.functions.end(),
              "fixture should lower call_helper with a LinkNameId before printer emission");

  c4c::codegen::lir::LirCallOp* direct_call = nullptr;
  for (auto& block : caller_it->blocks) {
    auto inst_it = std::find_if(block.insts.begin(), block.insts.end(),
                                [](c4c::codegen::lir::LirInst& inst) {
                                  return std::holds_alternative<c4c::codegen::lir::LirCallOp>(inst);
                                });
    if (inst_it == block.insts.end()) {
      continue;
    }
    direct_call = std::get_if<c4c::codegen::lir::LirCallOp>(&*inst_it);
    if (direct_call != nullptr) {
      break;
    }
  }
  expect_true(direct_call != nullptr,
              "fixture should lower the helper call into a concrete direct-call carrier");
  expect_true(direct_call->direct_callee_link_name_id != c4c::kInvalidLinkName,
              "resolved direct calls should carry a LinkNameId into printer emission");
  expect_eq(lir_module.link_names.spelling(direct_call->direct_callee_link_name_id), "helper",
            "direct-call LinkNameId should resolve to the semantic callee spelling");
  direct_call->callee = c4c::codegen::lir::LirOperand("@broken_direct_call_callee");

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("@helper(") != std::string::npos,
              "printer should recover the direct callee spelling from LinkNameId lookup");
  expect_true(llvm_ir.find("broken_direct_call_callee") == std::string::npos,
              "printer should not trust a corrupted raw direct-call operand");
}

void test_lir_printer_resolves_extern_decl_link_names_at_emission_boundary() {
  c4c::codegen::lir::LirModule lir_module = make_link_name_aware_lir_module();

  c4c::codegen::lir::LirExternDecl decl;
  decl.name = "broken_extern_decl_name";
  decl.return_type_str = "i32";
  decl.return_type = "i32";
  decl.link_name_id = lir_module.link_names.intern("semantic_helper");
  expect_true(decl.link_name_id != c4c::kInvalidLinkName,
              "fixture should assign a real LinkNameId to the extern declaration carrier");
  lir_module.extern_decls.push_back(std::move(decl));

  const std::string llvm_ir = c4c::codegen::lir::print_llvm(lir_module);
  expect_true(llvm_ir.find("declare i32 @semantic_helper(...)") != std::string::npos,
              "printer should recover extern declaration spellings from LinkNameId lookup");
  expect_true(llvm_ir.find("broken_extern_decl_name") == std::string::npos,
              "printer should not trust a corrupted raw extern declaration name");
}

void test_lir_to_bir_resolves_link_names_at_backend_boundary() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int global_value = 7;

int add_one(int value) { return value + global_value; }
)cpp");
  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  auto lir_fn_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "add_one";
      });
  expect_true(lir_fn_it != lir_module.functions.end(),
              "fixture should lower add_one with a LinkNameId before backend emission");
  lir_fn_it->name = "broken_add_one_name";

  auto lir_global_it = std::find_if(
      lir_module.globals.begin(), lir_module.globals.end(),
      [&](const c4c::codegen::lir::LirGlobal& global) {
        return global.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(global.link_name_id) == "global_value";
      });
  expect_true(lir_global_it != lir_module.globals.end(),
              "fixture should lower global_value with a LinkNameId before backend emission");
  lir_global_it->name = "broken_global_value";

  const auto lowering =
      c4c::backend::try_lower_to_bir_with_options(lir_module, c4c::backend::BirLoweringOptions{});
  expect_true(lowering.module.has_value(),
              "backend lir_to_bir lowering should still succeed after raw LIR names are corrupted");

  const auto bir_fn_it = std::find_if(
      lowering.module->functions.begin(), lowering.module->functions.end(),
      [](const c4c::backend::bir::Function& function) { return function.name == "add_one"; });
  expect_true(bir_fn_it != lowering.module->functions.end(),
              "backend lir_to_bir should recover the function spelling from LinkNameId lookup");

  const auto bir_global_it = std::find_if(
      lowering.module->globals.begin(), lowering.module->globals.end(),
      [](const c4c::backend::bir::Global& global) { return global.name == "global_value"; });
  expect_true(bir_global_it != lowering.module->globals.end(),
              "backend lir_to_bir should recover the global spelling from LinkNameId lookup");

  expect_true(std::none_of(lowering.module->functions.begin(), lowering.module->functions.end(),
                           [](const c4c::backend::bir::Function& function) {
                             return function.name == "broken_add_one_name";
                           }),
              "backend lir_to_bir should not trust a corrupted raw function name carrier");
  expect_true(std::none_of(lowering.module->globals.begin(), lowering.module->globals.end(),
                           [](const c4c::backend::bir::Global& global) {
                             return global.name == "broken_global_value";
                           }),
              "backend lir_to_bir should not trust a corrupted raw global name carrier");
}

void test_lir_to_bir_analysis_resolves_link_names_at_backend_boundary() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int add_one(int value) { return value + 1; }
)cpp");
  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  auto lir_fn_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "add_one";
      });
  expect_true(lir_fn_it != lir_module.functions.end(),
              "fixture should lower add_one with a LinkNameId before backend analysis");
  lir_fn_it->name = "broken_analysis_function_name";

  c4c::backend::BirLoweringContext context =
      c4c::backend::make_lowering_context(lir_module, c4c::backend::BirLoweringOptions{});
  const c4c::backend::BirModuleAnalysis analysis = c4c::backend::analyze_module(context);

  const auto summary_it = std::find_if(
      analysis.functions.begin(), analysis.functions.end(),
      [](const c4c::backend::BirFunctionPreScan& summary) {
        return summary.function_name == "add_one";
      });
  expect_true(summary_it != analysis.functions.end(),
              "backend analysis should recover the semantic function name from LinkNameId lookup");
  expect_true(std::none_of(analysis.functions.begin(), analysis.functions.end(),
                           [](const c4c::backend::BirFunctionPreScan& summary) {
                             return summary.function_name == "broken_analysis_function_name";
                           }),
              "backend analysis should not trust a corrupted raw function name carrier");
}

void test_lir_to_bir_resolves_direct_call_link_names_at_backend_boundary() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int helper(int value) { return value + 1; }

int call_helper(int value) { return helper(value); }
)cpp");
  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  auto caller_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "call_helper";
      });
  expect_true(caller_it != lir_module.functions.end(),
              "fixture should lower call_helper with a LinkNameId before backend call lowering");

  c4c::codegen::lir::LirCallOp* direct_call = nullptr;
  for (auto& block : caller_it->blocks) {
    auto inst_it = std::find_if(block.insts.begin(), block.insts.end(),
                                [](c4c::codegen::lir::LirInst& inst) {
                                  return std::holds_alternative<c4c::codegen::lir::LirCallOp>(inst);
                                });
    if (inst_it == block.insts.end()) {
      continue;
    }
    direct_call = std::get_if<c4c::codegen::lir::LirCallOp>(&*inst_it);
    if (direct_call != nullptr) {
      break;
    }
  }
  expect_true(direct_call != nullptr,
              "fixture should lower the helper call into a concrete direct-call carrier");
  expect_true(direct_call->direct_callee_link_name_id != c4c::kInvalidLinkName,
              "resolved direct calls should forward a LinkNameId alongside the raw LIR callee");
  expect_eq(lir_module.link_names.spelling(direct_call->direct_callee_link_name_id), "helper",
            "direct-call LinkNameId should resolve to the semantic callee spelling");
  direct_call->callee = c4c::codegen::lir::LirOperand("@broken_direct_call_callee");

  const auto lowering =
      c4c::backend::try_lower_to_bir_with_options(lir_module, c4c::backend::BirLoweringOptions{});
  expect_true(lowering.module.has_value(),
              "backend lir_to_bir lowering should still succeed after a direct-call symbol is corrupted");

  const auto bir_fn_it = std::find_if(
      lowering.module->functions.begin(), lowering.module->functions.end(),
      [](const c4c::backend::bir::Function& function) {
        return !function.is_declaration && function.name == "call_helper";
      });
  expect_true(bir_fn_it != lowering.module->functions.end(),
              "backend lir_to_bir should lower the caller function with its semantic name");

  const c4c::backend::bir::CallInst* bir_call = nullptr;
  for (const auto& block : bir_fn_it->blocks) {
    auto inst_it = std::find_if(block.insts.begin(), block.insts.end(),
                                [](const c4c::backend::bir::Inst& inst) {
                                  return std::holds_alternative<c4c::backend::bir::CallInst>(inst);
                                });
    if (inst_it == block.insts.end()) {
      continue;
    }
    bir_call = std::get_if<c4c::backend::bir::CallInst>(&*inst_it);
    if (bir_call != nullptr) {
      break;
    }
  }
  expect_true(bir_call != nullptr,
              "backend lir_to_bir should preserve the direct call in the lowered BIR body");
  expect_true(!bir_call->is_indirect && bir_call->callee == "helper",
              "backend lir_to_bir should recover the semantic direct callee name from LinkNameId");
  expect_true(bir_call->callee != "broken_direct_call_callee",
              "backend lir_to_bir should not trust a corrupted raw direct-call symbol");
}

void test_lir_to_bir_resolves_extern_decl_link_names_at_backend_boundary() {
  c4c::codegen::lir::LirModule lir_module = make_link_name_aware_lir_module();

  c4c::codegen::lir::LirExternDecl decl;
  decl.name = "broken_backend_extern_decl_name";
  decl.return_type_str = "i32";
  decl.return_type = "i32";
  decl.link_name_id = lir_module.link_names.intern("semantic_backend_helper");
  expect_true(decl.link_name_id != c4c::kInvalidLinkName,
              "fixture should assign a real LinkNameId before backend extern-decl lowering");
  lir_module.extern_decls.push_back(std::move(decl));

  const auto lowering =
      c4c::backend::try_lower_to_bir_with_options(lir_module, c4c::backend::BirLoweringOptions{});
  expect_true(lowering.module.has_value(),
              "backend lir_to_bir lowering should succeed for an extern declaration with a LinkNameId");

  const auto bir_decl_it = std::find_if(
      lowering.module->functions.begin(), lowering.module->functions.end(),
      [](const c4c::backend::bir::Function& function) {
        return function.is_declaration && function.name == "semantic_backend_helper";
      });
  expect_true(bir_decl_it != lowering.module->functions.end(),
              "backend lir_to_bir should recover extern declaration names from LinkNameId lookup");
  expect_true(std::none_of(lowering.module->functions.begin(), lowering.module->functions.end(),
                           [](const c4c::backend::bir::Function& function) {
                             return function.name == "broken_backend_extern_decl_name";
                           }),
              "backend lir_to_bir should not trust a corrupted raw extern declaration name");
}

void test_lir_to_bir_resolves_decl_backed_direct_call_link_names_at_backend_boundary() {
  const c4c::hir::Module hir_module = lower_hir_module(R"cpp(
extern int helper(int value);

int call_helper(int value) { return helper(value); }
)cpp");
  c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  auto decl_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.is_declaration && fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "helper";
      });
  expect_true(decl_it != lir_module.functions.end(),
              "fixture should lower the extern helper declaration with a LinkNameId");
  decl_it->name = "broken_helper_decl_name";

  auto caller_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return !fn.is_declaration && fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "call_helper";
      });
  expect_true(caller_it != lir_module.functions.end(),
              "fixture should lower call_helper with a LinkNameId before backend call lowering");

  c4c::codegen::lir::LirCallOp* direct_call = nullptr;
  for (auto& block : caller_it->blocks) {
    auto inst_it = std::find_if(block.insts.begin(), block.insts.end(),
                                [](c4c::codegen::lir::LirInst& inst) {
                                  return std::holds_alternative<c4c::codegen::lir::LirCallOp>(inst);
                                });
    if (inst_it == block.insts.end()) {
      continue;
    }
    direct_call = std::get_if<c4c::codegen::lir::LirCallOp>(&*inst_it);
    if (direct_call != nullptr) {
      break;
    }
  }
  expect_true(direct_call != nullptr,
              "fixture should lower the helper call into a concrete decl-backed direct-call carrier");
  expect_true(direct_call->direct_callee_link_name_id != c4c::kInvalidLinkName,
              "decl-backed direct calls should still carry a LinkNameId into backend lowering");
  direct_call->callee = c4c::codegen::lir::LirOperand("@broken_decl_backed_direct_call");

  const auto lowering =
      c4c::backend::try_lower_to_bir_with_options(lir_module, c4c::backend::BirLoweringOptions{});
  expect_true(lowering.module.has_value(),
              "backend lir_to_bir lowering should still succeed after declaration-backed names are corrupted");

  const auto bir_decl_it = std::find_if(
      lowering.module->functions.begin(), lowering.module->functions.end(),
      [](const c4c::backend::bir::Function& function) {
        return function.is_declaration && function.name == "helper";
      });
  expect_true(bir_decl_it != lowering.module->functions.end(),
              "backend lir_to_bir should recover the semantic declaration name from LinkNameId");
  expect_true(std::none_of(lowering.module->functions.begin(), lowering.module->functions.end(),
                           [](const c4c::backend::bir::Function& function) {
                             return function.name == "broken_helper_decl_name";
                           }),
              "backend lir_to_bir should not trust a corrupted raw declaration name carrier");

  const auto bir_fn_it = std::find_if(
      lowering.module->functions.begin(), lowering.module->functions.end(),
      [](const c4c::backend::bir::Function& function) {
        return !function.is_declaration && function.name == "call_helper";
      });
  expect_true(bir_fn_it != lowering.module->functions.end(),
              "backend lir_to_bir should still lower the caller function with its semantic name");

  const c4c::backend::bir::CallInst* bir_call = nullptr;
  for (const auto& block : bir_fn_it->blocks) {
    auto inst_it = std::find_if(block.insts.begin(), block.insts.end(),
                                [](const c4c::backend::bir::Inst& inst) {
                                  return std::holds_alternative<c4c::backend::bir::CallInst>(inst);
                                });
    if (inst_it == block.insts.end()) {
      continue;
    }
    bir_call = std::get_if<c4c::backend::bir::CallInst>(&*inst_it);
    if (bir_call != nullptr) {
      break;
    }
  }
  expect_true(bir_call != nullptr,
              "backend lir_to_bir should preserve the decl-backed direct call in the lowered BIR body");
  expect_true(!bir_call->is_indirect && bir_call->callee == "helper",
              "backend lir_to_bir should recover the semantic decl-backed direct callee name from LinkNameId");
  expect_true(bir_call->callee != "broken_decl_backed_direct_call",
              "backend lir_to_bir should not trust a corrupted raw decl-backed direct-call symbol");
}

}  // namespace

int main() {
  test_dense_id_map_tracks_assigned_dense_ids();
  test_optional_dense_id_map_supports_sparse_occupancy();
  test_dense_id_map_supports_local_type_storage();
  test_optional_dense_id_map_supports_function_id_counters();
  test_hir_materializes_link_name_ids_for_emitted_symbols();
  test_hir_materializes_decl_ref_link_name_ids_for_emitted_refs();
  test_hir_function_params_preserve_text_ids();
  test_hir_namespace_qualifiers_preserve_text_ids_for_qualified_decl_refs();
  test_hir_decl_stmt_decl_refs_preserve_text_ids_for_ctor_and_dtor_routes();
  test_hir_stmt_decl_refs_preserve_text_ids_for_this_param_and_ctor_callees();
  test_hir_struct_defs_preserve_text_ids_for_tags_and_bases();
  test_hir_template_calls_preserve_text_ids_for_source_template_names();
  test_hir_consteval_call_metadata_preserves_text_ids_for_function_names();
  test_hir_member_exprs_preserve_text_ids_for_field_names();
  test_hir_global_init_designators_preserve_text_ids_for_field_names();
  test_hir_to_lir_forwards_function_link_name_ids();
  test_hir_to_lir_dead_internal_reachability_prefers_link_name_ids();
  test_hir_to_lir_direct_call_target_resolution_prefers_link_name_ids();
  test_hir_to_lir_decl_backed_function_designator_rvalues_prefer_link_name_ids();
  test_hir_to_lir_decl_backed_call_result_inference_prefers_link_name_ids();
  test_hir_to_lir_object_helper_callees_prefer_link_name_ids();
  test_hir_to_lir_template_call_helper_callees_prefer_carrier_link_name_ids();
  test_hir_direct_call_builtin_alias_fallback_keeps_invalid_link_name_ids();
  test_lir_printer_resolves_link_names_at_emission_boundary();
  test_lir_printer_resolves_specialization_metadata_link_names();
  test_lir_printer_resolves_direct_call_link_names_at_emission_boundary();
  test_lir_printer_resolves_extern_decl_link_names_at_emission_boundary();
  test_lir_to_bir_resolves_link_names_at_backend_boundary();
  test_lir_to_bir_analysis_resolves_link_names_at_backend_boundary();
  test_lir_to_bir_resolves_direct_call_link_names_at_backend_boundary();
  test_lir_to_bir_resolves_extern_decl_link_names_at_backend_boundary();
  test_lir_to_bir_resolves_decl_backed_direct_call_link_names_at_backend_boundary();

  std::cout << "PASS: frontend_hir_tests\n";
  return 0;
}
