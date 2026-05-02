#include "arena.hpp"
#include "hir_to_lir.hpp"
#include "hir/hir_ir.hpp"
#include "hir/hir_printer.hpp"
#include "hir/compile_time_engine.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"
#include "target_profile.hpp"
#include "backend/bir/lir_to_bir/lowering.hpp"

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
#include <string_view>
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

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

void expect_eq_int(int actual, int expected, const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::to_string(expected) +
         "\nActual: " + std::to_string(actual));
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

void test_hir_to_lir_global_initializer_reachability_prefers_structured_function_ids() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
static int semantic_helper(int value) { return value + 1; }
static int rendered_shadow(int value) { return value - 1; }

int (*global_pick)(int) = semantic_helper;
int call_pick(int value) { return global_pick(value); }
)cpp");

  const auto helper_it = hir_module.fn_index.find("semantic_helper");
  expect_true(helper_it != hir_module.fn_index.end(),
              "fixture semantic helper should be present in the HIR function index");
  c4c::hir::Function* helper = hir_module.find_function(helper_it->second);
  expect_true(helper != nullptr && helper->link_name_id != c4c::kInvalidLinkName,
              "fixture semantic helper should carry a stable HIR LinkNameId");
  const c4c::LinkNameId helper_link_name_id = helper->link_name_id;

  const auto shadow_it = hir_module.fn_index.find("rendered_shadow");
  expect_true(shadow_it != hir_module.fn_index.end(),
              "fixture rendered-name shadow should be present in the HIR function index");
  const c4c::hir::Function* shadow = hir_module.find_function(shadow_it->second);
  expect_true(shadow != nullptr && shadow->link_name_id != c4c::kInvalidLinkName,
              "fixture rendered-name shadow should carry a stable HIR LinkNameId");
  const c4c::LinkNameId shadow_link_name_id = shadow->link_name_id;

  const auto global_it = hir_module.global_index.find("global_pick");
  expect_true(global_it != hir_module.global_index.end(),
              "fixture global function pointer should be present in the HIR global index");
  c4c::hir::GlobalVar* global_pick = hir_module.find_global(global_it->second);
  expect_true(global_pick != nullptr,
              "fixture global function pointer should resolve through the HIR global lookup");
  auto* init = std::get_if<c4c::hir::InitScalar>(&global_pick->init);
  expect_true(init != nullptr,
              "fixture global function pointer should use a scalar initializer");
  auto* init_ref =
      std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[init->expr.value].payload);
  expect_true(init_ref != nullptr && init_ref->link_name_id == helper_link_name_id,
              "fixture initializer should carry the semantic helper LinkNameId");

  helper->name = "broken_semantic_helper_definition_name";
  init_ref->name = "rendered_shadow";

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_global_it = std::find_if(
      lir_module.globals.begin(), lir_module.globals.end(),
      [&](const c4c::codegen::lir::LirGlobal& global) {
        return global.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(global.link_name_id) == "global_pick";
      });
  expect_true(lir_global_it != lir_module.globals.end(),
              "fixture global function pointer should lower into a concrete LIR global");
  expect_true(std::find(lir_global_it->initializer_function_link_name_ids.begin(),
                        lir_global_it->initializer_function_link_name_ids.end(),
                        helper_link_name_id) !=
                  lir_global_it->initializer_function_link_name_ids.end(),
              "global initializer lowering should preserve the structured helper identity");

  const auto lir_helper_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id == helper_link_name_id;
      });
  expect_true(lir_helper_it != lir_module.functions.end(),
              "dead-internal elimination should keep the helper reached by global initializer identity");
  expect_true(lir_helper_it->can_elide_if_unreferenced,
              "fixture semantic helper should exercise the discardable-function reachability path");

  const auto lir_shadow_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id == shadow_link_name_id;
      });
  expect_true(lir_shadow_it == lir_module.functions.end(),
              "dead-internal elimination should not keep the rendered-name collision");
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

void test_hir_to_lir_direct_call_lookup_rejects_rendered_name_collision_after_link_name_miss() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int rendered_shadow(int value) { return value + 100; }

int call_external(int value) { return rendered_shadow(value); }
)cpp");

  const c4c::LinkNameId semantic_external_id =
      hir_module.link_names.intern("semantic_external_helper");
  expect_true(semantic_external_id != c4c::kInvalidLinkName,
              "fixture should allocate a semantic external LinkNameId");

  const auto caller_it = hir_module.fn_index.find("call_external");
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
  expect_eq(callee_ref->name, "rendered_shadow",
            "fixture raw callee spelling should collide with a defined local function");

  callee_ref->link_name_id = semantic_external_id;

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_caller_it = std::find_if(
      lir_module.functions.begin(), lir_module.functions.end(),
      [&](const c4c::codegen::lir::LirFunction& fn) {
        return fn.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(fn.link_name_id) == "call_external";
      });
  expect_true(lir_caller_it != lir_module.functions.end(),
              "fixture caller should still lower after the call LinkNameId is drifted");

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
              "fixture caller should still lower the direct call after semantic/raw drift");
  expect_true(direct_call->direct_callee_link_name_id == semantic_external_id,
              "direct-call lowering should keep the semantic external LinkNameId");
  expect_true(direct_call->callee == "@semantic_external_helper",
              "direct-call lowering should recover the external callee from LinkNameId after a local-name miss");
  expect_true(direct_call->callee != "@rendered_shadow",
              "direct-call lowering should not bind the drifted rendered name to the local function");

  const auto extern_decl_it = std::find_if(
      lir_module.extern_decls.begin(), lir_module.extern_decls.end(),
      [&](const c4c::codegen::lir::LirExternDecl& decl) {
        return decl.link_name_id == semantic_external_id;
      });
  expect_true(extern_decl_it != lir_module.extern_decls.end(),
              "extern-call declaration finalization should preserve the semantic external LinkNameId");
  expect_eq(lir_module.link_names.spelling(extern_decl_it->link_name_id),
            "semantic_external_helper",
            "extern-call declaration should resolve through the semantic LinkNameId");
  expect_true(extern_decl_it->name != "rendered_shadow",
              "extern-call declaration should not use the drifted local collision as authority");
}

void test_hir_to_lir_global_initializer_lookup_prefers_semantic_global_identity() {
  c4c::hir::Module hir_module = lower_hir_module(R"cpp(
int semantic_global[2] = {1, 2};
int rendered_shadow = 11;
int *global_ptr = &semantic_global[1];
)cpp");

  const auto semantic_it = hir_module.global_index.find("semantic_global");
  expect_true(semantic_it != hir_module.global_index.end(),
              "fixture semantic global should be present in the HIR global index");
  c4c::hir::GlobalVar* semantic_global = hir_module.find_global(semantic_it->second);
  expect_true(semantic_global != nullptr &&
                  semantic_global->link_name_id != c4c::kInvalidLinkName &&
                  semantic_global->name_text_id != c4c::kInvalidText,
              "fixture semantic global should carry stable HIR identity");

  const auto shadow_it = hir_module.global_index.find("rendered_shadow");
  expect_true(shadow_it != hir_module.global_index.end(),
              "fixture shadow global should be present in the HIR global index");
  const c4c::hir::GlobalVar* shadow_global = hir_module.find_global(shadow_it->second);
  expect_true(shadow_global != nullptr,
              "fixture shadow global should resolve through the HIR global lookup");

  const auto ptr_it = hir_module.global_index.find("global_ptr");
  expect_true(ptr_it != hir_module.global_index.end(),
              "fixture pointer global should be present in the HIR global index");
  c4c::hir::GlobalVar* global_ptr = hir_module.find_global(ptr_it->second);
  expect_true(global_ptr != nullptr,
              "fixture pointer global should resolve through the HIR global lookup");
  auto* ptr_init = std::get_if<c4c::hir::InitScalar>(&global_ptr->init);
  expect_true(ptr_init != nullptr,
              "fixture pointer global should use a scalar initializer");

  auto* addr_of =
      std::get_if<c4c::hir::UnaryExpr>(&hir_module.expr_pool[ptr_init->expr.value].payload);
  expect_true(addr_of != nullptr && addr_of->op == c4c::hir::UnaryOp::AddrOf,
              "fixture pointer initializer should take a global address");
  auto* index_expr =
      std::get_if<c4c::hir::IndexExpr>(&hir_module.expr_pool[addr_of->operand.value].payload);
  expect_true(index_expr != nullptr,
              "fixture pointer initializer should index the semantic global array");
  auto* base_ref =
      std::get_if<c4c::hir::DeclRef>(&hir_module.expr_pool[index_expr->base.value].payload);
  expect_true(base_ref != nullptr && base_ref->global &&
                  base_ref->global->value == semantic_global->id.value,
              "fixture indexed base should carry a concrete semantic GlobalId");

  semantic_global->name = shadow_global->name;
  base_ref->name = shadow_global->name;
  base_ref->link_name_id = c4c::kInvalidLinkName;

  const c4c::codegen::lir::LirModule lir_module = c4c::codegen::lir::lower(hir_module);

  const auto lir_ptr_it = std::find_if(
      lir_module.globals.begin(), lir_module.globals.end(),
      [&](const c4c::codegen::lir::LirGlobal& global) {
        return global.link_name_id != c4c::kInvalidLinkName &&
               lir_module.link_names.spelling(global.link_name_id) == "global_ptr";
      });
  expect_true(lir_ptr_it != lir_module.globals.end(),
              "fixture pointer global should lower through semantic identity");
  expect_true(lir_ptr_it->init_text.find("@semantic_global") != std::string::npos,
              "global initializer lowering should recover the semantic global from GlobalId identity");
  expect_true(lir_ptr_it->init_text.find("@rendered_shadow") == std::string::npos,
              "global initializer lowering should not trust a drifted raw global-name collision");
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

void test_hir_template_arg_materialization_prefers_structured_value_payload() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Box");
  primary->n_template_params = 1;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("N");
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
  owner.tpl_struct_args.data[0].value = 7;
  owner.tpl_struct_args.data[0].debug_text = arena.strdup("RenderedN");

  c4c::hir::Lowerer lowerer;
  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  nttp_bindings["RenderedN"] = 101;

  const c4c::hir::ResolvedTemplateArgs resolved =
      lowerer.materialize_template_args(primary, owner, type_bindings,
                                        nttp_bindings);

  expect_true(resolved.concrete_args.size() == 1 &&
                  resolved.concrete_args[0].is_value,
              "HIR template materialization should keep the structured NTTP payload");
  expect_eq_int(static_cast<int>(resolved.concrete_args[0].value), 7,
                "stale TemplateArgRef debug_text must not select the NTTP value");
  expect_true(resolved.nttp_bindings.size() == 1 &&
                  resolved.nttp_bindings[0].first == "N",
              "HIR template materialization should bind the primary NTTP name");
  expect_eq_int(static_cast<int>(resolved.nttp_bindings[0].second), 7,
                "HIR template materialization should bind from the structured value");
}

void test_hir_template_arg_materialization_keeps_legacy_zero_value_fallback() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Box");
  primary->n_template_params = 2;
  primary->template_param_names = arena.alloc_array<const char*>(2);
  primary->template_param_names[0] = arena.strdup("N");
  primary->template_param_names[1] = arena.strdup("Flag");
  primary->template_param_is_nttp = arena.alloc_array<bool>(2);
  primary->template_param_is_nttp[0] = true;
  primary->template_param_is_nttp[1] = true;

  c4c::TypeSpec owner{};
  owner.array_size = -1;
  owner.inner_rank = -1;
  owner.base = c4c::TB_STRUCT;
  owner.tpl_struct_origin = arena.strdup("Box");
  owner.tpl_struct_args.size = 2;
  owner.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(2);
  owner.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
  owner.tpl_struct_args.data[0].value = 0;
  owner.tpl_struct_args.data[0].debug_text = arena.strdup("ForwardedN");
  owner.tpl_struct_args.data[1].kind = c4c::TemplateArgKind::Value;
  owner.tpl_struct_args.data[1].value = 0;
  owner.tpl_struct_args.data[1].debug_text = arena.strdup("ForwardedFlag");

  c4c::hir::Lowerer lowerer;
  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  nttp_bindings["ForwardedN"] = 42;
  nttp_bindings["ForwardedFlag"] = 0;

  const c4c::hir::ResolvedTemplateArgs resolved =
      lowerer.materialize_template_args(primary, owner, type_bindings,
                                        nttp_bindings);

  expect_true(resolved.concrete_args.size() == 2 &&
                  resolved.concrete_args[0].is_value &&
                  resolved.concrete_args[1].is_value,
              "legacy zero-valued NTTP refs should still materialize as values");
  expect_eq_int(static_cast<int>(resolved.concrete_args[0].value), 42,
                "zero-valued forwarded NTTP refs should use debug_text bindings");
  expect_eq_int(static_cast<int>(resolved.concrete_args[1].value), 0,
                "false forwarded NTTP refs should keep the zero binding");
}

void test_hir_pending_type_ref_keeps_legacy_zero_value_debug_text() {
  c4c::Arena arena;

  c4c::TypeSpec owner{};
  owner.array_size = -1;
  owner.inner_rank = -1;
  owner.base = c4c::TB_STRUCT;
  owner.tpl_struct_origin = arena.strdup("Box");
  owner.tpl_struct_args.size = 3;
  owner.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(3);
  owner.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
  owner.tpl_struct_args.data[0].value = 0;
  owner.tpl_struct_args.data[0].debug_text = arena.strdup("ForwardedN");
  owner.tpl_struct_args.data[1].kind = c4c::TemplateArgKind::Value;
  owner.tpl_struct_args.data[1].value = 0;
  owner.tpl_struct_args.data[1].debug_text = arena.strdup("ForwardedFlag");
  owner.tpl_struct_args.data[2].kind = c4c::TemplateArgKind::Value;
  owner.tpl_struct_args.data[2].value = 7;
  owner.tpl_struct_args.data[2].debug_text = arena.strdup("StaleRenderedN");

  const std::string encoded = c4c::hir::encode_pending_type_ref(owner);

  expect_true(encoded.find("|args=ForwardedN,ForwardedFlag,v:7|") !=
                  std::string::npos,
              "pending template type keys should preserve zero-valued legacy "
              "fallback refs while nonzero structured values win over stale text");
}

void test_hir_deferred_member_typedef_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Node* record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  record->name = arena.strdup("RealOwner");
  record->n_member_typedefs = 1;
  record->member_typedef_names = arena.alloc_array<const char*>(1);
  record->member_typedef_names[0] = arena.strdup("value_type");
  record->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  record->member_typedef_types[0].array_size = -1;
  record->member_typedef_types[0].inner_rank = -1;
  record->member_typedef_types[0].base = c4c::TB_INT;

  c4c::TypeSpec owner{};
  owner.array_size = -1;
  owner.inner_rank = -1;
  owner.base = c4c::TB_STRUCT;
  owner.tag = arena.strdup("StaleRenderedOwner");
  owner.record_def = record;
  owner.deferred_member_type_name = arena.strdup("value_type");

  c4c::hir::Lowerer lowerer;
  const bool resolved = lowerer.resolve_struct_member_typedef_if_ready(&owner);

  expect_true(resolved,
              "deferred member typedef resolution should use structured record_def when tag spelling is stale");
  expect_true(owner.base == c4c::TB_INT,
              "stale rendered owner tag must not block structured member typedef resolution");
}

void test_hir_member_owner_lookup_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  auto add_hir_struct = [&](const char* tag) {
    c4c::hir::HirStructDef def;
    def.tag = tag;
    def.tag_text_id = module.link_name_texts->intern(tag);
    def.ns_qual.context_id = parser.current_namespace_context_id();
    module.index_struct_def_owner(c4c::hir::make_hir_record_owner_key(def),
                                  def.tag, true);
    module.struct_defs[def.tag] = std::move(def);
  };
  add_hir_struct("RealOwner");
  add_hir_struct("StaleRenderedOwner");

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealOwner");
  real_record->unqualified_name = arena.strdup("RealOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  lowerer.struct_def_nodes_["RealOwner"] = real_record;
  lowerer.register_struct_def_node_owner(real_record);

  c4c::TypeSpec base{};
  base.array_size = -1;
  base.inner_rank = -1;
  base.base = c4c::TB_STRUCT;
  base.tag = arena.strdup("StaleRenderedOwner");
  base.record_def = real_record;

  std::optional<std::string> owner_tag =
      lowerer.resolve_member_lookup_owner_tag(
          base, false, nullptr, nullptr, nullptr, nullptr,
          "member-owner-record-def-test");

  expect_true(owner_tag.has_value() && *owner_tag == "RealOwner",
              "member owner lookup should prefer record_def over a stale rendered tag");
}

void test_hir_member_owner_lookup_prefers_template_origin_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef stale_primary_def;
  stale_primary_def.tag = "Box";
  stale_primary_def.tag_text_id = module.link_name_texts->intern("Box");
  stale_primary_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_primary_def.tag] = stale_primary_def;

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Box");
  primary->unqualified_name = arena.strdup("Box");
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("T");
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_pack = arena.alloc_array<bool>(1);
  primary->template_param_is_pack[0] = false;
  lowerer.register_template_struct_primary("Box", primary);

  c4c::TypeSpec base{};
  base.array_size = -1;
  base.inner_rank = -1;
  base.base = c4c::TB_STRUCT;
  base.tag = arena.strdup("Box");
  base.tpl_struct_origin = arena.strdup("Box");
  base.tpl_struct_args.size = 1;
  base.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  base.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Type;
  base.tpl_struct_args.data[0].type.array_size = -1;
  base.tpl_struct_args.data[0].type.inner_rank = -1;
  base.tpl_struct_args.data[0].type.base = c4c::TB_INT;

  std::optional<std::string> owner_tag =
      lowerer.resolve_member_lookup_owner_tag(
          base, false, nullptr, nullptr, nullptr, nullptr,
          "member-owner-template-origin-test");

  expect_true(owner_tag.has_value() && *owner_tag != "Box",
              "member owner lookup should realize template-origin carriers before stale rendered tags");
  expect_true(module.struct_defs.count(*owner_tag) != 0,
              "template-origin owner lookup should return a realized HIR struct tag");
}

void test_hir_template_struct_primary_lookup_prefers_record_def_over_stale_origin() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node stale_primary{};
  stale_primary.kind = c4c::NK_STRUCT_DEF;
  stale_primary.name = "StalePrimary";
  stale_primary.unqualified_name = "StalePrimary";
  stale_primary.n_template_params = 1;

  c4c::Node structured_primary{};
  structured_primary.kind = c4c::NK_STRUCT_DEF;
  structured_primary.name = "StructuredPrimary";
  structured_primary.unqualified_name = "StructuredPrimary";
  structured_primary.n_template_params = 1;

  lowerer.register_template_struct_primary("StalePrimary", &stale_primary);
  lowerer.register_template_struct_primary("StructuredPrimary",
                                           &structured_primary);

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = c4c::TB_STRUCT;
  ts.tpl_struct_origin = "StalePrimary";
  ts.record_def = &structured_primary;

  expect_true(lowerer.canonical_template_struct_primary(ts) ==
                  &structured_primary,
              "template struct primary lookup should prefer record_def identity over stale rendered origin");

  c4c::Node unregistered_primary{};
  unregistered_primary.kind = c4c::NK_STRUCT_DEF;
  unregistered_primary.name = "UnregisteredPrimary";
  unregistered_primary.unqualified_name = "UnregisteredPrimary";
  unregistered_primary.n_template_params = 1;
  ts.record_def = &unregistered_primary;
  expect_true(lowerer.canonical_template_struct_primary(ts) == nullptr,
              "template struct primary lookup should reject stale rendered fallback when record_def has structured owner metadata but no entry");
}

void test_hir_template_struct_specialization_lookup_prefers_owner_key_over_stale_primary_name() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node stale_primary{};
  stale_primary.kind = c4c::NK_STRUCT_DEF;
  stale_primary.name = "StalePrimary";
  stale_primary.unqualified_name = "StalePrimary";
  stale_primary.n_template_params = 1;

  c4c::Node structured_primary{};
  structured_primary.kind = c4c::NK_STRUCT_DEF;
  structured_primary.name = "StalePrimary";
  structured_primary.unqualified_name = "StructuredPrimary";
  structured_primary.n_template_params = 1;

  c4c::Node stale_spec{};
  stale_spec.kind = c4c::NK_STRUCT_DEF;
  stale_spec.name = "StalePrimary<int>";
  stale_spec.unqualified_name = "StalePrimary";
  c4c::Node structured_spec{};
  structured_spec.kind = c4c::NK_STRUCT_DEF;
  structured_spec.name = "StructuredPrimary<int>";
  structured_spec.unqualified_name = "StructuredPrimary";

  lowerer.register_template_struct_primary("StalePrimary", &stale_primary);
  lowerer.register_template_struct_primary("StructuredPrimary",
                                           &structured_primary);
  lowerer.register_template_struct_specialization(&stale_primary, &stale_spec);
  lowerer.register_template_struct_specialization(&structured_primary,
                                                  &structured_spec);

  const std::vector<const c4c::Node*>* specializations =
      lowerer.find_template_struct_specializations(&structured_primary);
  expect_true(specializations && specializations->size() == 1 &&
                  (*specializations)[0] == &structured_spec,
              "template struct specialization lookup should prefer owner-key identity over stale rendered primary name");
}

void test_hir_member_owner_lookup_record_def_failure_does_not_use_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleFailedOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleFailedOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* unresolved_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  unresolved_record->name = arena.strdup("UnresolvedOwner");
  unresolved_record->unqualified_name = arena.strdup("UnresolvedOwner");
  unresolved_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec base{};
  base.array_size = -1;
  base.inner_rank = -1;
  base.base = c4c::TB_STRUCT;
  base.tag = arena.strdup("StaleFailedOwner");
  base.record_def = unresolved_record;

  const std::optional<std::string> owner_tag =
      lowerer.resolve_member_lookup_owner_tag(
          base, false, nullptr, nullptr, nullptr, nullptr,
          "member-owner-record-def-failure-test");

  expect_true(!owner_tag.has_value(),
              "member owner lookup must not return a stale rendered tag when record_def identity fails to resolve");
}

void test_hir_member_owner_lookup_generated_record_def_without_key_keeps_tag_fallback() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::C);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef generated_def;
  generated_def.tag = "__anon_record_1";
  generated_def.tag_text_id = module.link_name_texts->intern("__anon_record_1");
  generated_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[generated_def.tag] = generated_def;

  c4c::Node* generated_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  generated_record->name = arena.strdup("_anon_0");
  generated_record->unqualified_name = arena.strdup("_anon_0");
  generated_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec base{};
  base.array_size = -1;
  base.inner_rank = -1;
  base.base = c4c::TB_UNION;
  base.tag = arena.strdup("__anon_record_1");
  base.record_def = generated_record;

  const std::optional<std::string> owner_tag =
      lowerer.resolve_member_lookup_owner_tag(
          base, false, nullptr, nullptr, nullptr, nullptr,
          "member-owner-generated-record-def-fallback-test");

  expect_true(owner_tag.has_value() && *owner_tag == "__anon_record_1",
              "generated anonymous aggregate record_def owners without an owner key should keep rendered tag compatibility");
}

void test_hir_member_owner_lookup_template_args_failure_does_not_use_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleTemplateArgsOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleTemplateArgsOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec base{};
  base.array_size = -1;
  base.inner_rank = -1;
  base.base = c4c::TB_STRUCT;
  base.tag = arena.strdup("StaleTemplateArgsOwner");
  base.tpl_struct_args.size = 1;
  base.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  base.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Type;
  base.tpl_struct_args.data[0].type.array_size = -1;
  base.tpl_struct_args.data[0].type.inner_rank = -1;
  base.tpl_struct_args.data[0].type.base = c4c::TB_INT;

  const std::optional<std::string> owner_tag =
      lowerer.resolve_member_lookup_owner_tag(
          base, false, nullptr, nullptr, nullptr, nullptr,
          "member-owner-template-args-failure-test");

  expect_true(!owner_tag.has_value(),
              "member owner lookup must not return a stale rendered tag when structured template args fail to resolve");
}

void test_hir_member_expr_owner_failure_does_not_use_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* unresolved_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  unresolved_record->name = arena.strdup("UnresolvedMemberExprOwner");
  unresolved_record->unqualified_name = arena.strdup("UnresolvedMemberExprOwner");
  unresolved_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleMemberExprOwner::field");

  c4c::hir::HirStructField stale_field;
  stale_field.name = "field";
  stale_field.field_text_id = module.link_name_texts->intern("field");
  stale_field.elem_type = int_ts;
  stale_field.member_symbol_id = stale_id;
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleMemberExprOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleMemberExprOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  stale_def.fields.push_back(stale_field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleMemberExprOwner");
  owner_ts.record_def = unresolved_record;

  c4c::Node* obj = parser.make_node(c4c::NK_VAR, 1);
  obj->name = arena.strdup("obj");
  obj->type = owner_ts;
  c4c::Node* member = parser.make_node(c4c::NK_MEMBER, 1);
  member->left = obj;
  member->name = arena.strdup("field");
  member->type = int_ts;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  const c4c::hir::LocalId obj_local{0};
  ctx.locals["obj"] = obj_local;
  ctx.local_types.insert(obj_local, owner_ts);

  const c4c::hir::ExprId expr_id = lowerer.lower_expr(&ctx, member);
  expect_true(expr_id.valid(), "member expression lowering should produce an expression");
  const c4c::hir::Expr* expr = module.find_expr(expr_id);
  expect_true(expr != nullptr, "member expression lowering should append an expression");
  const c4c::hir::MemberExpr* member_expr =
      expr ? std::get_if<c4c::hir::MemberExpr>(&expr->payload) : nullptr;
  expect_true(member_expr != nullptr,
              "member expression lowering should produce a MemberExpr payload");
  expect_true(member_expr->resolved_owner_tag != "StaleMemberExprOwner" &&
                  member_expr->member_symbol_id != stale_id,
              "member expression lowering must not select stale rendered owner metadata after structured owner failure");
}

void test_hir_static_member_const_lookup_prefers_template_owner_key_over_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef def;
  def.tag = "StaleRenderedBox_T_int";
  def.tag_text_id = module.link_name_texts->intern("Box");
  def.ns_qual.context_id = 9;
  c4c::hir::HirRecordOwnerTemplateIdentity identity;
  identity.primary_declaration_text_id = def.tag_text_id;
  identity.specialization_key = "type:int";
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_template_record_owner_key(def, std::move(identity));
  module.index_struct_def_owner(owner_key, def.tag, true);
  module.struct_defs[def.tag] = def;

  const c4c::TextId member_text_id = module.link_name_texts->intern("value");
  c4c::hir::HirStructMemberLookupKey member_key;
  member_key.owner_key = owner_key;
  member_key.member_text_id = member_text_id;

  lowerer.struct_static_member_const_values_[def.tag]["value"] = 11;
  lowerer.struct_static_member_const_values_by_owner_[member_key] = 23;

  const std::optional<long long> value =
      lowerer.find_struct_static_member_const_value(def.tag, "value");

  expect_true(value.has_value() && *value == 23,
              "static member const lookup should prefer template owner keys over stale rendered tags");
  expect_true(lowerer.struct_static_member_const_value_lookup_parity_checks_ == 1,
              "structured-first static member const lookup should still record rendered parity");
  expect_true(lowerer.struct_static_member_const_value_lookup_parity_mismatches_ == 1,
              "stale rendered static member const values should be detected when structured lookup wins");
}

void test_hir_static_member_const_lookup_keeps_rendered_fallback_without_owner_key() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  lowerer.struct_static_member_const_values_["LegacyRendered"]["value"] = 31;

  const std::optional<long long> value =
      lowerer.find_struct_static_member_const_value("LegacyRendered", "value");

  expect_true(value.has_value() && *value == 31,
              "static member const lookup should preserve rendered fallback when no owner key exists");
}

void test_hir_static_member_const_lookup_structured_miss_keeps_base_fallback() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::NamespaceQualifier ns;
  ns.context_id = 12;
  const c4c::TextId owner_text_id = module.link_name_texts->intern("Derived");
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(ns, owner_text_id);
  module.index_struct_def_owner(owner_key, "Derived", true);

  c4c::hir::HirStructDef base_def;
  base_def.tag = "Base";
  module.struct_defs[base_def.tag] = base_def;

  c4c::hir::HirStructDef derived_def;
  derived_def.tag = "Derived";
  derived_def.base_tags.push_back("Base");
  module.struct_defs[derived_def.tag] = derived_def;

  const c4c::TextId member_text_id = module.link_name_texts->intern("value");
  c4c::hir::HirStructMemberLookupKey member_key;
  member_key.owner_key = owner_key;
  member_key.member_text_id = member_text_id;

  lowerer.struct_static_member_const_values_["Base"]["value"] = 41;
  lowerer.struct_static_member_const_values_["StaleRendered"]["value"] = 7;

  const std::string stale_tag = "StaleRendered";
  const std::string rendered_member = "value";
  const std::optional<long long> value =
      lowerer.find_struct_static_member_const_value(
          member_key, &stale_tag, &rendered_member);

  expect_true(value.has_value() && *value == 41,
              "structured static member const miss should use owner-key base fallback before stale rendered spelling");
}

void test_hir_static_member_decl_lookup_prefers_template_owner_key_over_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef def;
  def.tag = "StaleRenderedBox_T_int";
  def.tag_text_id = module.link_name_texts->intern("Box");
  def.ns_qual.context_id = 9;
  c4c::hir::HirRecordOwnerTemplateIdentity identity;
  identity.primary_declaration_text_id = def.tag_text_id;
  identity.specialization_key = "type:int";
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_template_record_owner_key(def, std::move(identity));
  module.index_struct_def_owner(owner_key, def.tag, true);
  module.struct_defs[def.tag] = def;

  const c4c::TextId member_text_id = module.link_name_texts->intern("value");
  c4c::hir::HirStructMemberLookupKey member_key;
  member_key.owner_key = owner_key;
  member_key.member_text_id = member_text_id;

  c4c::Node stale_decl{};
  stale_decl.kind = c4c::NK_DECL;
  stale_decl.name = "value";
  c4c::Node structured_decl{};
  structured_decl.kind = c4c::NK_DECL;
  structured_decl.name = "value";

  lowerer.struct_static_member_decls_[def.tag]["value"] = &stale_decl;
  lowerer.struct_static_member_decls_by_owner_[member_key] = &structured_decl;

  const c4c::Node* decl =
      lowerer.find_struct_static_member_decl(def.tag, "value");

  expect_true(decl == &structured_decl,
              "static member decl lookup should prefer template owner keys over stale rendered tags");
  expect_true(lowerer.struct_static_member_decl_lookup_parity_checks_ == 1,
              "structured-first static member decl lookup should still record rendered parity");
  expect_true(lowerer.struct_static_member_decl_lookup_parity_mismatches_ == 1,
              "stale rendered static member decls should be detected when structured lookup wins");
}

void test_hir_static_member_decl_lookup_keeps_rendered_fallback_without_owner_key() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node rendered_decl{};
  rendered_decl.kind = c4c::NK_DECL;
  rendered_decl.name = "value";
  lowerer.struct_static_member_decls_["LegacyRendered"]["value"] = &rendered_decl;

  const c4c::Node* decl =
      lowerer.find_struct_static_member_decl("LegacyRendered", "value");

  expect_true(decl == &rendered_decl,
              "static member decl lookup should preserve rendered fallback when no owner key exists");
}

void test_hir_member_symbol_lookup_prefers_owner_key_over_stale_rendered_spelling() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node real_record{};
  real_record.kind = c4c::NK_STRUCT_DEF;
  real_record.name = "RealSymbolOwner";
  real_record.unqualified_name = "RealSymbolOwner";
  real_record.namespace_context_id = 6;

  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(&real_record);
  expect_true(owner_key.has_value(),
              "fixture real member-symbol owner should have structured identity");

  const c4c::TextId field_text_id = module.link_name_texts->intern("field");
  const c4c::hir::HirStructMemberLookupKey member_key =
      *lowerer.make_struct_member_lookup_key(*owner_key, field_text_id);
  const c4c::MemberSymbolId structured_id =
      module.member_symbols.intern("RealSymbolOwner::field");
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleRenderedOwner::stale_field");
  lowerer.struct_member_symbol_ids_by_owner_[member_key] = structured_id;

  const std::string stale_owner = "StaleRenderedOwner";
  const std::string stale_member = "stale_field";
  const c4c::MemberSymbolId direct_id =
      lowerer.find_struct_member_symbol_id(
          member_key, &stale_owner, &stale_member);

  expect_true(direct_id == structured_id && direct_id != stale_id,
              "direct member-symbol lookup should prefer owner/member keys over stale rendered spelling");
  expect_true(lowerer.struct_member_symbol_id_lookup_parity_checks_ == 1,
              "direct member-symbol owner-key lookup should record rendered parity");
  expect_true(lowerer.struct_member_symbol_id_lookup_parity_mismatches_ == 1,
              "direct member-symbol owner-key lookup should detect stale rendered spelling");

  c4c::TypeSpec owner_ts{};
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = stale_owner.c_str();
  owner_ts.record_def = &real_record;
  const c4c::MemberSymbolId typed_id =
      lowerer.find_struct_member_symbol_id(
          owner_ts, stale_owner, stale_member, field_text_id);

  expect_true(typed_id == structured_id,
              "TypeSpec-carried member-symbol lookup should prefer record_def/member TextId authority");
  expect_true(lowerer.struct_member_symbol_id_lookup_parity_checks_ == 2,
              "TypeSpec-carried member-symbol lookup should also record rendered parity");
  expect_true(lowerer.struct_member_symbol_id_lookup_parity_mismatches_ == 2,
              "TypeSpec-carried member-symbol lookup should also detect stale rendered spelling");
}

void test_hir_scoped_static_member_lowering_prefers_record_def_over_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node real_record{};
  real_record.kind = c4c::NK_STRUCT_DEF;
  real_record.name = "RealStaticOwner";
  real_record.unqualified_name = "RealStaticOwner";
  real_record.namespace_context_id = 4;

  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(&real_record);
  expect_true(owner_key.has_value(),
              "fixture real static owner should have structured owner identity");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealStaticOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealStaticOwner");
  real_def.ns_qual.context_id = real_record.namespace_context_id;
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;

  c4c::Node stale_decl{};
  stale_decl.kind = c4c::NK_DECL;
  stale_decl.name = "value";
  stale_decl.type = int_ts;
  c4c::Node real_decl{};
  real_decl.kind = c4c::NK_DECL;
  real_decl.name = "value";
  real_decl.type = int_ts;

  lowerer.struct_def_nodes_["StaleRenderedStaticOwner"] = &real_record;
  lowerer.struct_static_member_decls_["StaleRenderedStaticOwner"]["stale_value"] =
      &stale_decl;
  lowerer.struct_static_member_const_values_["StaleRenderedStaticOwner"]["stale_value"] =
      11;
  lowerer.register_struct_static_member_owner_lookup(*owner_key, &real_decl, 23);

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "StaleRenderedStaticOwner::stale_value";
  ref.unqualified_name = "value";
  ref.unqualified_text_id = module.link_name_texts->intern("value");
  ref.type = int_ts;

  const c4c::hir::ExprId value_id = lowerer.lower_var_expr(nullptr, &ref);
  const auto* literal =
      std::get_if<c4c::hir::IntLiteral>(&module.expr_pool[value_id.value].payload);
  expect_true(literal != nullptr,
              "scoped static member lookup should lower to a constexpr literal");
  expect_eq_int(static_cast<int>(literal->value), 23,
                "scoped static member lowering should prefer record_def/member identity over stale rendered spelling");
}

void test_hir_struct_method_lookup_prefers_template_owner_key_over_stale_tag() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef def;
  def.tag = "StaleRenderedBox_T_int";
  def.tag_text_id = module.link_name_texts->intern("Box");
  def.ns_qual.context_id = 9;
  c4c::hir::HirRecordOwnerTemplateIdentity identity;
  identity.primary_declaration_text_id = def.tag_text_id;
  identity.specialization_key = "type:int";
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_template_record_owner_key(def, std::move(identity));
  module.index_struct_def_owner(owner_key, def.tag, true);
  module.struct_defs[def.tag] = def;

  const c4c::TextId method_text_id = module.link_name_texts->intern("method");
  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = owner_key;
  method_key.method_text_id = method_text_id;
  method_key.is_const_method = false;

  const c4c::LinkNameId stale_link_name =
      module.link_names.intern("stale_method_mangled");
  const c4c::LinkNameId structured_link_name =
      module.link_names.intern("structured_method_mangled");
  c4c::TypeSpec stale_return_type{};
  stale_return_type.base = c4c::TB_INT;
  c4c::TypeSpec structured_return_type{};
  structured_return_type.base = c4c::TB_LONG;

  lowerer.struct_methods_[def.tag + "::method"] = "stale_method_mangled";
  lowerer.struct_methods_by_owner_[method_key] = "structured_method_mangled";
  lowerer.struct_method_link_name_ids_[def.tag + "::method"] = stale_link_name;
  lowerer.struct_method_link_name_ids_by_owner_[method_key] =
      structured_link_name;
  lowerer.struct_method_ret_types_[def.tag + "::method"] = stale_return_type;
  lowerer.struct_method_ret_types_by_owner_[method_key] = structured_return_type;

  const std::optional<std::string> mangled =
      lowerer.find_struct_method_mangled(def.tag, "method", false);
  expect_true(mangled.has_value() && *mangled == "structured_method_mangled",
              "method mangled lookup should prefer template owner keys over stale rendered tags");
  expect_true(lowerer.struct_method_mangled_lookup_parity_checks_ == 1,
              "structured-first method mangled lookup should still record rendered parity");
  expect_true(lowerer.struct_method_mangled_lookup_parity_mismatches_ == 1,
              "stale rendered method mangled names should be detected when structured lookup wins");

  const std::optional<c4c::LinkNameId> link_name =
      lowerer.find_struct_method_link_name_id(def.tag, "method", false);
  expect_true(link_name.has_value() && *link_name == structured_link_name,
              "method link-name lookup should prefer template owner keys over stale rendered tags");
  expect_true(lowerer.struct_method_link_name_lookup_parity_checks_ == 1,
              "structured-first method link-name lookup should still record rendered parity");
  expect_true(lowerer.struct_method_link_name_lookup_parity_mismatches_ == 1,
              "stale rendered method link names should be detected when structured lookup wins");

  const std::optional<c4c::TypeSpec> return_type =
      lowerer.find_struct_method_return_type(def.tag, "method", false);
  expect_true(return_type.has_value() && return_type->base == c4c::TB_LONG,
              "method return-type lookup should prefer template owner keys over stale rendered tags");
  expect_true(lowerer.struct_method_return_type_lookup_parity_checks_ == 1,
              "structured-first method return-type lookup should still record rendered parity");
  expect_true(lowerer.struct_method_return_type_lookup_parity_mismatches_ == 1,
              "stale rendered method return types should be detected when structured lookup wins");
}

void test_hir_struct_method_lookup_keeps_rendered_fallback_without_owner_key() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  lowerer.struct_methods_["LegacyRendered::method"] = "legacy_method_mangled";
  const c4c::LinkNameId legacy_link_name =
      module.link_names.intern("legacy_method_mangled");
  lowerer.struct_method_link_name_ids_["LegacyRendered::method"] =
      legacy_link_name;
  c4c::TypeSpec legacy_return_type{};
  legacy_return_type.base = c4c::TB_INT;
  lowerer.struct_method_ret_types_["LegacyRendered::method"] = legacy_return_type;

  const std::optional<std::string> mangled =
      lowerer.find_struct_method_mangled("LegacyRendered", "method", false);
  expect_true(mangled.has_value() && *mangled == "legacy_method_mangled",
              "method mangled lookup should preserve rendered fallback when no owner key exists");

  const std::optional<c4c::LinkNameId> link_name =
      lowerer.find_struct_method_link_name_id("LegacyRendered", "method", false);
  expect_true(link_name.has_value() && *link_name == legacy_link_name,
              "method link-name lookup should preserve rendered fallback when no owner key exists");

  const std::optional<c4c::TypeSpec> return_type =
      lowerer.find_struct_method_return_type("LegacyRendered", "method", false);
  expect_true(return_type.has_value() && return_type->base == c4c::TB_INT,
              "method return-type lookup should preserve rendered fallback when no owner key exists");
}

void test_hir_out_of_class_method_attachment_prefers_structured_owner_key() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::NamespaceQualifier owner_ns;
  owner_ns.context_id = 11;
  const c4c::TextId owner_text =
      module.link_name_texts->intern("StructuredOwner");
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(owner_ns, owner_text);
  c4c::hir::HirStructDef def;
  def.tag = "RenderedOwner";
  def.tag_text_id = owner_text;
  def.ns_qual = owner_ns;
  module.index_struct_def_owner(owner_key, def.tag, true);
  module.struct_defs[def.tag] = def;

  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = owner_key;
  method_key.method_text_id = module.link_name_texts->intern("method");
  lowerer.struct_methods_by_owner_[method_key] = "StructuredOwner__method";
  lowerer.struct_methods_["RenderedOwner::method"] = "RenderedOwner__method";

  c4c::Node stale_method{};
  stale_method.kind = c4c::NK_FUNCTION;
  stale_method.name = "method";
  stale_method.unqualified_name = "method";
  c4c::Node in_class_method{};
  in_class_method.kind = c4c::NK_FUNCTION;
  in_class_method.name = "method";
  in_class_method.unqualified_name = "method";
  lowerer.pending_methods_.push_back(
      {"RenderedOwner__method", "RenderedOwner", &stale_method, {}, {}});
  lowerer.pending_methods_.push_back(
      {"StructuredOwner__method", "RenderedOwner", &in_class_method, {}, {}});

  const char* qualifier_segments[] = {"StructuredOwner"};
  c4c::Node body{};
  c4c::Node out_of_class{};
  out_of_class.kind = c4c::NK_FUNCTION;
  out_of_class.name = "RenderedOwner::method";
  out_of_class.unqualified_name = "method";
  out_of_class.namespace_context_id = 11;
  out_of_class.qualifier_segments = qualifier_segments;
  out_of_class.n_qualifier_segments = 1;
  out_of_class.body = &body;

  std::vector<const c4c::Node*> items{&out_of_class};
  lowerer.attach_out_of_class_struct_method_defs(items, module);

  expect_true(lowerer.pending_methods_[0].method_node == &stale_method,
              "rendered compatibility key should not receive structured out-of-class body");
  expect_true(lowerer.pending_methods_[1].method_node == &out_of_class,
              "out-of-class method attachment should prefer structured owner/method key");
}

void test_hir_out_of_class_method_skip_prefers_structured_owner_key() {
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::NamespaceQualifier owner_ns;
  owner_ns.context_id = 13;
  const c4c::TextId owner_text =
      module.link_name_texts->intern("StructuredOwner");
  const c4c::hir::HirRecordOwnerKey owner_key =
      c4c::hir::make_hir_record_owner_key(owner_ns, owner_text);
  c4c::hir::HirStructDef def;
  def.tag = "RenderedOwner";
  def.tag_text_id = owner_text;
  def.ns_qual = owner_ns;
  module.index_struct_def_owner(owner_key, def.tag, true);
  module.struct_defs[def.tag] = def;

  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = owner_key;
  method_key.method_text_id = module.link_name_texts->intern("method");
  lowerer.struct_methods_by_owner_[method_key] = "StructuredOwner__method";

  const char* qualifier_segments[] = {"StructuredOwner"};
  c4c::Node out_of_class{};
  out_of_class.kind = c4c::NK_FUNCTION;
  out_of_class.name = "MisleadingRendered::method";
  out_of_class.unqualified_name = "method";
  out_of_class.namespace_context_id = 13;
  out_of_class.qualifier_segments = qualifier_segments;
  out_of_class.n_qualifier_segments = 1;

  std::vector<const c4c::Node*> items{&out_of_class};
  lowerer.lower_non_method_functions_and_globals(items, module);

  expect_true(module.functions.empty(),
              "structured out-of-class methods should not lower as ordinary functions");
}

void test_hir_range_for_method_owner_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealRange");
  real_record->unqualified_name = arena.strdup("RealRange");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for the real range record");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealRange";
  real_def.tag_text_id = module.link_name_texts->intern("RealRange");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleRenderedRange";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleRenderedRange");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  const c4c::TextId begin_text_id = module.link_name_texts->intern("begin");
  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = *owner_key;
  method_key.method_text_id = begin_text_id;
  method_key.is_const_method = false;
  lowerer.struct_methods_by_owner_[method_key] = "RealRange__begin";
  lowerer.struct_methods_["StaleRenderedRange::begin"] = "StaleRange__begin";

  c4c::TypeSpec range_ts{};
  range_ts.array_size = -1;
  range_ts.inner_rank = -1;
  range_ts.base = c4c::TB_STRUCT;
  range_ts.tag = arena.strdup("StaleRenderedRange");
  range_ts.record_def = real_record;

  const std::optional<std::string> stale_rendered_method =
      lowerer.find_struct_method_mangled("StaleRenderedRange", "begin", false);
  expect_true(stale_rendered_method.has_value() &&
                  *stale_rendered_method == "StaleRange__begin",
              "fixture should expose the stale rendered method fallback before owner repair");

  const std::string owner_tag = lowerer.resolve_struct_method_lookup_owner_tag(
      range_ts, false, nullptr, nullptr, nullptr, nullptr,
      "range-for-method-owner-test");
  expect_true(owner_tag == "RealRange",
              "range-for method owner resolution should prefer record_def over a stale rendered tag");

  const std::optional<std::string> resolved_method =
      lowerer.find_struct_method_mangled(owner_tag, "begin", false);
  expect_true(resolved_method.has_value() &&
                  *resolved_method == "RealRange__begin",
              "range-for method lookup should use the structured owner before rendered spelling");
}

void test_hir_operator_call_method_owner_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealOperatorOwner");
  real_record->unqualified_name = arena.strdup("RealOperatorOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for the real operator owner");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealOperatorOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealOperatorOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleRenderedOperatorOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleRenderedOperatorOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  const c4c::TextId operator_text_id =
      module.link_name_texts->intern("operator_deref");
  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = *owner_key;
  method_key.method_text_id = operator_text_id;
  method_key.is_const_method = false;
  lowerer.struct_methods_by_owner_[method_key] = "RealOperatorOwner__deref";
  lowerer.struct_methods_["StaleRenderedOperatorOwner::operator_deref"] =
      "StaleOperatorOwner__deref";

  c4c::TypeSpec structured_ret{};
  structured_ret.base = c4c::TB_LONG;
  c4c::TypeSpec stale_ret{};
  stale_ret.base = c4c::TB_INT;
  lowerer.struct_method_ret_types_by_owner_[method_key] = structured_ret;
  lowerer.struct_method_ret_types_["StaleRenderedOperatorOwner::operator_deref"] =
      stale_ret;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleRenderedOperatorOwner");
  owner_ts.record_def = real_record;

  c4c::Node* obj = parser.make_node(c4c::NK_VAR, 1);
  obj->name = arena.strdup("obj");
  obj->type = owner_ts;
  c4c::Node* result = parser.make_node(c4c::NK_DEREF, 1);
  result->left = obj;
  result->type = structured_ret;

  c4c::hir::Lowerer::FunctionCtx ctx;
  const c4c::hir::LocalId obj_local{0};
  ctx.locals["obj"] = obj_local;
  ctx.local_types.insert(obj_local, owner_ts);

  const std::optional<std::string> stale_rendered_method =
      lowerer.find_struct_method_mangled(
          "StaleRenderedOperatorOwner", "operator_deref", false);
  expect_true(stale_rendered_method.has_value() &&
                  *stale_rendered_method == "StaleOperatorOwner__deref",
              "fixture should expose the stale rendered operator fallback before owner repair");

  const c4c::hir::ExprId call_id = lowerer.try_lower_operator_call(
      &ctx, result, obj, "operator_deref", {});
  expect_true(call_id.valid(),
              "operator call lowering should find the structured-owner method");
  const c4c::hir::Expr* call_expr = module.find_expr(call_id);
  expect_true(call_expr != nullptr,
              "operator call lowering should append a call expression");
  const c4c::hir::CallExpr* call =
      std::get_if<c4c::hir::CallExpr>(&call_expr->payload);
  expect_true(call != nullptr,
              "operator call lowering should produce a CallExpr payload");
  const c4c::hir::Expr* callee_expr = module.find_expr(call->callee);
  expect_true(callee_expr != nullptr,
              "operator call lowering should append a callee expression");
  const c4c::hir::DeclRef* callee =
      std::get_if<c4c::hir::DeclRef>(&callee_expr->payload);
  expect_true(callee != nullptr && callee->name == "RealOperatorOwner__deref",
              "operator method lookup should use structured record_def owner before stale rendered spelling");
  expect_true(call_expr->type.spec.base == c4c::TB_LONG,
              "operator method return-type lookup should use the structured owner");
}

void test_hir_operator_call_method_owner_keeps_rendered_fallback() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::hir::HirStructDef legacy_def;
  legacy_def.tag = "LegacyRenderedOperatorOwner";
  legacy_def.tag_text_id = module.link_name_texts->intern("LegacyRenderedOperatorOwner");
  legacy_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[legacy_def.tag] = legacy_def;
  lowerer.struct_methods_["LegacyRenderedOperatorOwner::operator_deref"] =
      "LegacyOperatorOwner__deref";
  c4c::TypeSpec legacy_ret{};
  legacy_ret.base = c4c::TB_INT;
  lowerer.struct_method_ret_types_["LegacyRenderedOperatorOwner::operator_deref"] =
      legacy_ret;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("LegacyRenderedOperatorOwner");

  c4c::Node* obj = parser.make_node(c4c::NK_VAR, 1);
  obj->name = arena.strdup("obj");
  obj->type = owner_ts;
  c4c::Node* result = parser.make_node(c4c::NK_DEREF, 1);
  result->left = obj;
  result->type = legacy_ret;

  c4c::hir::Lowerer::FunctionCtx ctx;
  const c4c::hir::LocalId obj_local{0};
  ctx.locals["obj"] = obj_local;
  ctx.local_types.insert(obj_local, owner_ts);

  const c4c::hir::ExprId call_id = lowerer.try_lower_operator_call(
      &ctx, result, obj, "operator_deref", {});
  expect_true(call_id.valid(),
              "operator call lowering should retain rendered fallback when structured owner is absent");
  const c4c::hir::Expr* call_expr = module.find_expr(call_id);
  expect_true(call_expr != nullptr,
              "rendered fallback operator call should append a call expression");
  const c4c::hir::CallExpr* call =
      std::get_if<c4c::hir::CallExpr>(&call_expr->payload);
  expect_true(call != nullptr,
              "rendered fallback operator call should produce a CallExpr payload");
  const c4c::hir::Expr* callee_expr = module.find_expr(call->callee);
  expect_true(callee_expr != nullptr,
              "rendered fallback operator call should append a callee expression");
  const c4c::hir::DeclRef* callee =
      std::get_if<c4c::hir::DeclRef>(&callee_expr->payload);
  expect_true(callee != nullptr && callee->name == "LegacyOperatorOwner__deref",
              "operator method lookup should retain rendered fallback without structured owner identity");
}

void test_hir_generic_ctrl_deref_method_owner_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealGenericDerefOwner");
  real_record->unqualified_name = arena.strdup("RealGenericDerefOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for generic deref");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealGenericDerefOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealGenericDerefOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleGenericDerefOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleGenericDerefOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = *owner_key;
  method_key.method_text_id = module.link_name_texts->intern("operator_deref");
  method_key.is_const_method = false;
  c4c::TypeSpec structured_ret{};
  structured_ret.base = c4c::TB_LONG;
  c4c::TypeSpec stale_ret{};
  stale_ret.base = c4c::TB_INT;
  lowerer.struct_method_ret_types_by_owner_[method_key] = structured_ret;
  lowerer.struct_method_ret_types_["StaleGenericDerefOwner::operator_deref"] =
      stale_ret;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleGenericDerefOwner");
  owner_ts.record_def = real_record;

  c4c::Node* obj = parser.make_node(c4c::NK_VAR, 1);
  obj->name = arena.strdup("obj");
  obj->type = owner_ts;
  c4c::Node* deref = parser.make_node(c4c::NK_DEREF, 1);
  deref->left = obj;

  c4c::hir::Lowerer::FunctionCtx ctx;
  const c4c::hir::LocalId obj_local{0};
  ctx.locals["obj"] = obj_local;
  ctx.local_types.insert(obj_local, owner_ts);

  const c4c::TypeSpec inferred = lowerer.infer_generic_ctrl_type(&ctx, deref);
  expect_true(inferred.base == c4c::TB_LONG,
              "generic ctrl deref return-type lookup should use structured record_def owner before stale rendered spelling");
}

void test_hir_generic_ctrl_operator_call_method_owner_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealGenericCallOwner");
  real_record->unqualified_name = arena.strdup("RealGenericCallOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for generic operator_call");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealGenericCallOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealGenericCallOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleGenericCallOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleGenericCallOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = *owner_key;
  method_key.method_text_id = module.link_name_texts->intern("operator_call");
  method_key.is_const_method = false;
  c4c::TypeSpec structured_ret{};
  structured_ret.base = c4c::TB_LONG;
  c4c::TypeSpec stale_ret{};
  stale_ret.base = c4c::TB_INT;
  lowerer.struct_method_ret_types_by_owner_[method_key] = structured_ret;
  lowerer.struct_method_ret_types_["StaleGenericCallOwner::operator_call"] =
      stale_ret;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleGenericCallOwner");
  owner_ts.record_def = real_record;

  c4c::Node* obj = parser.make_node(c4c::NK_VAR, 1);
  obj->name = arena.strdup("obj");
  obj->type = owner_ts;
  c4c::Node* call = parser.make_node(c4c::NK_CALL, 1);
  call->left = obj;

  c4c::hir::Lowerer::FunctionCtx ctx;
  const c4c::hir::LocalId obj_local{0};
  ctx.locals["obj"] = obj_local;
  ctx.local_types.insert(obj_local, owner_ts);

  const c4c::TypeSpec inferred = lowerer.infer_generic_ctrl_type(&ctx, call);
  expect_true(inferred.base == c4c::TB_LONG,
              "generic ctrl operator_call return-type lookup should use structured record_def owner before stale rendered spelling");
}

void test_hir_member_call_method_owner_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealMemberCallOwner");
  real_record->unqualified_name = arena.strdup("RealMemberCallOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for member call");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealMemberCallOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealMemberCallOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleMemberCallOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleMemberCallOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = *owner_key;
  method_key.method_text_id = module.link_name_texts->intern("method");
  method_key.is_const_method = false;
  lowerer.struct_methods_by_owner_[method_key] = "RealMemberCallOwner__method";
  lowerer.struct_methods_["StaleMemberCallOwner::method"] =
      "StaleMemberCallOwner__method";

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleMemberCallOwner");
  owner_ts.record_def = real_record;

  c4c::Node* obj = parser.make_node(c4c::NK_VAR, 1);
  obj->name = arena.strdup("obj");
  obj->type = owner_ts;
  c4c::Node* member = parser.make_node(c4c::NK_MEMBER, 1);
  member->left = obj;
  member->name = arena.strdup("method");
  c4c::Node* call_node = parser.make_node(c4c::NK_CALL, 1);
  call_node->left = member;

  c4c::hir::Lowerer::FunctionCtx ctx;
  const c4c::hir::LocalId obj_local{0};
  ctx.locals["obj"] = obj_local;
  ctx.local_types.insert(obj_local, owner_ts);

  const std::optional<std::string> stale_rendered_method =
      lowerer.find_struct_method_mangled("StaleMemberCallOwner", "method", false);
  expect_true(stale_rendered_method.has_value() &&
                  *stale_rendered_method == "StaleMemberCallOwner__method",
              "fixture should expose the stale rendered member-call fallback before owner repair");

  const std::optional<c4c::hir::ExprId> call_id =
      lowerer.try_lower_member_call_expr(&ctx, call_node);
  expect_true(call_id.has_value() && call_id->valid(),
              "member call lowering should find the structured-owner method");
  const c4c::hir::Expr* call_expr = module.find_expr(*call_id);
  expect_true(call_expr != nullptr,
              "member call lowering should append a call expression");
  const c4c::hir::CallExpr* call =
      std::get_if<c4c::hir::CallExpr>(&call_expr->payload);
  expect_true(call != nullptr,
              "member call lowering should produce a CallExpr payload");
  const c4c::hir::Expr* callee_expr = module.find_expr(call->callee);
  expect_true(callee_expr != nullptr,
              "member call lowering should append a callee expression");
  const c4c::hir::DeclRef* callee =
      std::get_if<c4c::hir::DeclRef>(&callee_expr->payload);
  expect_true(callee != nullptr && callee->name == "RealMemberCallOwner__method",
              "member call method lookup should use structured record_def owner before stale rendered spelling");
}

void test_hir_direct_constructor_call_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealDirectCtorOwner");
  real_record->unqualified_name = arena.strdup("RealDirectCtorOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for direct constructor");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealDirectCtorOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealDirectCtorOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleDirectCtorOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleDirectCtorOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;

  c4c::Node* real_param = parser.make_node(c4c::NK_DECL, 0);
  real_param->type = int_ts;
  c4c::Node* real_ctor = parser.make_node(c4c::NK_FUNCTION, 0);
  real_ctor->name = arena.strdup("RealDirectCtorOwner");
  real_ctor->is_constructor = true;
  real_ctor->params = arena.alloc_array<c4c::Node*>(1);
  real_ctor->params[0] = real_param;
  real_ctor->n_params = 1;

  c4c::Node* stale_param = parser.make_node(c4c::NK_DECL, 0);
  stale_param->type = int_ts;
  c4c::Node* stale_ctor = parser.make_node(c4c::NK_FUNCTION, 0);
  stale_ctor->name = arena.strdup("StaleDirectCtorOwner");
  stale_ctor->is_constructor = true;
  stale_ctor->params = arena.alloc_array<c4c::Node*>(1);
  stale_ctor->params[0] = stale_param;
  stale_ctor->n_params = 1;

  lowerer.struct_constructors_["RealDirectCtorOwner"].push_back(
      {"RealDirectCtorOwner__ctor", real_ctor});
  lowerer.struct_constructors_["StaleDirectCtorOwner"].push_back(
      {"StaleDirectCtorOwner__ctor", stale_ctor});

  c4c::TypeSpec owner_ts{};
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleDirectCtorOwner");
  owner_ts.record_def = real_record;
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;

  c4c::Node* callee = parser.make_node(c4c::NK_VAR, 0);
  callee->name = arena.strdup("StaleDirectCtorOwner");
  callee->type = owner_ts;
  c4c::Node* arg = parser.make_node(c4c::NK_INT_LIT, 0);
  arg->ival = 7;
  arg->type = int_ts;
  c4c::Node* call_node = parser.make_node(c4c::NK_CALL, 1);
  call_node->left = callee;
  call_node->children = arena.alloc_array<c4c::Node*>(1);
  call_node->children[0] = arg;
  call_node->n_children = 1;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};

  const std::optional<c4c::hir::ExprId> result =
      lowerer.try_lower_direct_struct_constructor_call(&ctx, call_node);
  expect_true(result.has_value() && result->valid(),
              "direct constructor lowering should resolve through structured owner identity");

  const c4c::hir::DeclRef* ctor_callee = nullptr;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    const auto* call = std::get_if<c4c::hir::CallExpr>(&expr.payload);
    if (!call) continue;
    const c4c::hir::Expr* callee_expr = module.find_expr(call->callee);
    if (!callee_expr) continue;
    ctor_callee = std::get_if<c4c::hir::DeclRef>(&callee_expr->payload);
    if (ctor_callee) break;
  }
  expect_true(ctor_callee != nullptr &&
                  ctor_callee->name == "RealDirectCtorOwner__ctor",
              "direct constructor routing should not select the stale rendered constructor when record_def identity is available");
}

void test_hir_new_expr_method_owner_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealNewOwner");
  real_record->unqualified_name = arena.strdup("RealNewOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for new-expression owner");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealNewOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealNewOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleNewOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleNewOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = *owner_key;
  method_key.method_text_id = module.link_name_texts->intern("operator_new");
  method_key.is_const_method = false;
  lowerer.struct_methods_by_owner_[method_key] = "RealNewOwner__new";
  lowerer.struct_methods_["StaleNewOwner::operator_new"] = "StaleNewOwner__new";

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleNewOwner");
  owner_ts.record_def = real_record;

  c4c::Node* new_node = parser.make_node(c4c::NK_NEW_EXPR, 1);
  new_node->type = owner_ts;
  new_node->ival = 0;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};

  const std::string owner_tag = lowerer.resolve_struct_method_lookup_owner_tag(
      owner_ts, false, nullptr, nullptr, nullptr, new_node,
      "new-expression-method-owner-test");
  expect_true(owner_tag == "RealNewOwner",
              "new-expression owner resolution should prefer record_def over stale rendered spelling");
  const std::optional<std::string> resolved_method =
      lowerer.find_struct_method_mangled(owner_tag, "operator_new", false);
  expect_true(resolved_method.has_value() && *resolved_method == "RealNewOwner__new",
              "fixture should resolve structured operator_new before lowering new expression");

  const c4c::hir::ExprId new_id = lowerer.lower_new_expr(&ctx, new_node);
  expect_true(new_id.valid(), "new-expression lowering should produce a typed pointer");
  const c4c::hir::Expr* cast_expr = module.find_expr(new_id);
  expect_true(cast_expr != nullptr,
              "new-expression lowering should append the typed pointer cast");
  const c4c::hir::CastExpr* cast =
      std::get_if<c4c::hir::CastExpr>(&cast_expr->payload);
  expect_true(cast != nullptr,
              "new-expression lowering should return a CastExpr payload");
  const c4c::hir::Expr* call_expr = module.find_expr(cast->expr);
  expect_true(call_expr != nullptr,
              "new-expression lowering should append an allocator call expression");
  const c4c::hir::CallExpr* call =
      std::get_if<c4c::hir::CallExpr>(&call_expr->payload);
  expect_true(call != nullptr,
              "new-expression lowering should call an operator_new method");
  const c4c::hir::Expr* callee_expr = module.find_expr(call->callee);
  expect_true(callee_expr != nullptr,
              "new-expression lowering should append an allocator callee");
  const c4c::hir::DeclRef* callee =
      std::get_if<c4c::hir::DeclRef>(&callee_expr->payload);
  expect_true(callee != nullptr && callee->name == "RealNewOwner__new",
              "new-expression method lookup should use structured record_def owner before stale rendered spelling");
}

void test_hir_delete_expr_method_owner_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealDeleteOwner");
  real_record->unqualified_name = arena.strdup("RealDeleteOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for delete-expression owner");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealDeleteOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealDeleteOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleDeleteOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleDeleteOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::hir::HirStructMethodLookupKey method_key;
  method_key.owner_key = *owner_key;
  method_key.method_text_id = module.link_name_texts->intern("operator_delete");
  method_key.is_const_method = false;
  lowerer.struct_methods_by_owner_[method_key] = "RealDeleteOwner__delete";
  lowerer.struct_methods_["StaleDeleteOwner::operator_delete"] =
      "StaleDeleteOwner__delete";

  c4c::TypeSpec pointer_ts{};
  pointer_ts.array_size = -1;
  pointer_ts.inner_rank = -1;
  pointer_ts.base = c4c::TB_STRUCT;
  pointer_ts.tag = arena.strdup("StaleDeleteOwner");
  pointer_ts.record_def = real_record;
  pointer_ts.ptr_level = 1;

  c4c::Node* operand = parser.make_node(c4c::NK_VAR, 1);
  operand->name = arena.strdup("ptr");
  operand->type = pointer_ts;
  c4c::Node* delete_node = parser.make_node(c4c::NK_DELETE_EXPR, 1);
  delete_node->left = operand;
  delete_node->ival = 0;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  const c4c::hir::LocalId ptr_local{0};
  ctx.locals["ptr"] = ptr_local;
  ctx.local_types.insert(ptr_local, pointer_ts);

  const c4c::hir::ExprId delete_id = lowerer.lower_delete_expr(&ctx, delete_node);
  expect_true(delete_id.valid(), "delete-expression lowering should complete");
  expect_true(!fn.blocks.empty() && !fn.blocks.front().stmts.empty(),
              "delete-expression lowering should append an operator_delete statement");
  const c4c::hir::ExprStmt* expr_stmt =
      std::get_if<c4c::hir::ExprStmt>(&fn.blocks.front().stmts.back().payload);
  expect_true(expr_stmt != nullptr && expr_stmt->expr.has_value(),
              "delete-expression lowering should append an ExprStmt");
  const c4c::hir::Expr* call_expr = module.find_expr(*expr_stmt->expr);
  expect_true(call_expr != nullptr,
              "delete-expression lowering should append a deleter call expression");
  const c4c::hir::CallExpr* call =
      std::get_if<c4c::hir::CallExpr>(&call_expr->payload);
  expect_true(call != nullptr,
              "delete-expression lowering should call an operator_delete method");
  const c4c::hir::Expr* callee_expr = module.find_expr(call->callee);
  expect_true(callee_expr != nullptr,
              "delete-expression lowering should append a deleter callee");
  const c4c::hir::DeclRef* callee =
      std::get_if<c4c::hir::DeclRef>(&callee_expr->payload);
  expect_true(callee != nullptr && callee->name == "RealDeleteOwner__delete",
              "delete-expression method lookup should use structured record_def owner before stale rendered spelling");
}

void test_hir_init_list_member_symbol_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealInitListOwner");
  real_record->unqualified_name = arena.strdup("RealInitListOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for init-list owner");

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  c4c::TypeSpec int_ptr_ts = int_ts;
  int_ptr_ts.ptr_level = 1;

  const c4c::MemberSymbolId real_array_id =
      module.member_symbols.intern("RealInitListOwner::_M_array");
  const c4c::MemberSymbolId stale_array_id =
      module.member_symbols.intern("StaleInitListOwner::_M_array");
  const c4c::MemberSymbolId real_len_id =
      module.member_symbols.intern("RealInitListOwner::_M_len");
  module.member_symbols.intern("StaleInitListOwner::_M_len");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealInitListOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealInitListOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleInitListOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleInitListOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  c4c::hir::HirStructField array_field;
  array_field.name = "_M_array";
  array_field.field_text_id = module.link_name_texts->intern("_M_array");
  array_field.elem_type = int_ptr_ts;
  array_field.member_symbol_id = stale_array_id;
  stale_def.fields.push_back(array_field);
  c4c::hir::HirStructField len_field;
  len_field.name = "_M_len";
  len_field.field_text_id = module.link_name_texts->intern("_M_len");
  len_field.elem_type = int_ts;
  len_field.member_symbol_id = module.member_symbols.find("StaleInitListOwner::_M_len");
  stale_def.fields.push_back(len_field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleInitListOwner");
  owner_ts.record_def = real_record;

  c4c::Node* list_node = parser.make_node(c4c::NK_INIT_LIST, 1);
  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};

  const c4c::hir::ExprId tmp_id =
      lowerer.materialize_initializer_list_arg(&ctx, list_node, owner_ts);
  expect_true(tmp_id.valid(), "initializer-list materialization should produce a temp");

  const c4c::hir::MemberExpr* array_member = nullptr;
  const c4c::hir::MemberExpr* len_member = nullptr;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    if (const auto* member = std::get_if<c4c::hir::MemberExpr>(&expr.payload)) {
      if (member->field == "_M_array") array_member = member;
      if (member->field == "_M_len") len_member = member;
    }
  }
  expect_true(array_member != nullptr,
              "initializer-list materialization should lower _M_array member access");
  expect_true(array_member->resolved_owner_tag == "RealInitListOwner",
              "initializer-list member access should resolve structured owner tag first");
  expect_true(array_member->member_symbol_id == real_array_id &&
                  array_member->member_symbol_id != stale_array_id,
              "initializer-list member symbol should prefer record_def owner over stale tag");
  expect_true(len_member != nullptr && len_member->member_symbol_id == real_len_id,
              "initializer-list length member should use the structured owner symbol");
}

void test_hir_implicit_this_member_symbol_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealThisOwner");
  real_record->unqualified_name = arena.strdup("RealThisOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for implicit this");
  lowerer.struct_def_nodes_["StaleThisOwner"] = real_record;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  const c4c::MemberSymbolId real_id =
      module.member_symbols.intern("RealThisOwner::field");
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleThisOwner::field");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealThisOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealThisOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  c4c::hir::HirStructField real_field;
  real_field.name = "field";
  real_field.field_text_id = module.link_name_texts->intern("field");
  real_field.elem_type = int_ts;
  real_field.member_symbol_id = real_id;
  real_def.fields.push_back(real_field);
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleThisOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleThisOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  c4c::hir::HirStructField field;
  field.name = "field";
  field.field_text_id = module.link_name_texts->intern("field");
  field.elem_type = int_ts;
  field.member_symbol_id = stale_id;
  stale_def.fields.push_back(field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* var = parser.make_node(c4c::NK_VAR, 1);
  var->name = arena.strdup("field");
  var->type = int_ts;
  c4c::hir::Lowerer::FunctionCtx ctx;
  ctx.method_struct_tag = "StaleThisOwner";
  ctx.params["this"] = 0;

  const c4c::hir::ExprId field_id = lowerer.lower_var_expr(&ctx, var);
  const c4c::hir::Expr* field_expr = module.find_expr(field_id);
  expect_true(field_expr != nullptr,
              "implicit-this field lowering should append a member expression");
  const c4c::hir::MemberExpr* member =
      std::get_if<c4c::hir::MemberExpr>(&field_expr->payload);
  expect_true(member != nullptr,
              "implicit-this field lowering should produce a MemberExpr");
  expect_true(member->resolved_owner_tag == "RealThisOwner",
              "implicit-this member access should resolve structured owner tag first");
  expect_true(member->member_symbol_id == real_id &&
                  member->member_symbol_id != stale_id,
              "implicit-this member symbol should prefer record_def owner over stale tag");
}

void test_hir_ctor_init_member_symbol_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealCtorOwner");
  real_record->unqualified_name = arena.strdup("RealCtorOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for ctor init");
  lowerer.struct_def_nodes_["StaleCtorOwner"] = real_record;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  const c4c::MemberSymbolId real_id =
      module.member_symbols.intern("RealCtorOwner::field");
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleCtorOwner::field");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealCtorOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealCtorOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleCtorOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleCtorOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  c4c::hir::HirStructField field;
  field.name = "field";
  field.field_text_id = module.link_name_texts->intern("field");
  field.elem_type = int_ts;
  field.member_symbol_id = stale_id;
  stale_def.fields.push_back(field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* zero = parser.make_node(c4c::NK_INT_LIT, 1);
  zero->ival = 0;
  zero->type = int_ts;
  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 1);
  body->ival = 1;
  c4c::Node* ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  ctor->name = arena.strdup("StaleCtorOwner");
  ctor->unqualified_name = arena.strdup("StaleCtorOwner");
  ctor->is_constructor = true;
  ctor->type.base = c4c::TB_VOID;
  ctor->body = body;
  ctor->n_ctor_inits = 1;
  ctor->ctor_init_names = arena.alloc_array<const char*>(1);
  ctor->ctor_init_names[0] = arena.strdup("field");
  ctor->ctor_init_nargs = arena.alloc_array<int>(1);
  ctor->ctor_init_nargs[0] = 1;
  ctor->ctor_init_args = arena.alloc_array<c4c::Node**>(1);
  ctor->ctor_init_args[0] = arena.alloc_array<c4c::Node*>(1);
  ctor->ctor_init_args[0][0] = zero;

  lowerer.lower_struct_method("StaleCtorOwner__ctor", "StaleCtorOwner", ctor, nullptr, nullptr);

  const c4c::hir::MemberExpr* member = nullptr;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    if (const auto* candidate = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
        candidate && candidate->field == "field") {
      member = candidate;
      break;
    }
  }
  expect_true(member != nullptr,
              "constructor initializer should lower the initialized member access");
  expect_true(member->resolved_owner_tag == "RealCtorOwner",
              "constructor initializer member access should resolve structured owner tag first");
  expect_true(member->member_symbol_id == real_id &&
                  member->member_symbol_id != stale_id,
              "constructor initializer member symbol should prefer record_def owner over stale tag");
}

void test_hir_ctor_init_field_constructor_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_field_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_field_record->name = arena.strdup("RealFieldCtor");
  real_field_record->unqualified_name = arena.strdup("RealFieldCtor");
  real_field_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> field_owner_key =
      lowerer.make_struct_def_node_owner_key(real_field_record);
  expect_true(field_owner_key.has_value(),
              "fixture should build a structured owner key for field constructor");

  c4c::hir::HirStructDef real_field_def;
  real_field_def.tag = "RealFieldCtor";
  real_field_def.tag_text_id = module.link_name_texts->intern("RealFieldCtor");
  real_field_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*field_owner_key, real_field_def.tag, true);
  module.struct_defs[real_field_def.tag] = real_field_def;

  c4c::hir::HirStructDef stale_field_def;
  stale_field_def.tag = "StaleFieldCtor";
  stale_field_def.tag_text_id = module.link_name_texts->intern("StaleFieldCtor");
  stale_field_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_field_def.tag] = stale_field_def;

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  c4c::Node* real_param = parser.make_node(c4c::NK_DECL, 1);
  real_param->name = arena.strdup("value");
  real_param->type = int_ts;
  c4c::Node* real_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  real_ctor->name = arena.strdup("RealFieldCtor");
  real_ctor->unqualified_name = arena.strdup("RealFieldCtor");
  real_ctor->is_constructor = true;
  real_ctor->type.base = c4c::TB_VOID;
  real_ctor->n_params = 1;
  real_ctor->params = arena.alloc_array<c4c::Node*>(1);
  real_ctor->params[0] = real_param;
  lowerer.struct_constructors_["RealFieldCtor"].push_back(
      {"RealFieldCtor__ctor", real_ctor});

  c4c::Node* stale_param = parser.make_node(c4c::NK_DECL, 1);
  stale_param->name = arena.strdup("value");
  stale_param->type = int_ts;
  c4c::Node* stale_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  stale_ctor->name = arena.strdup("StaleFieldCtor");
  stale_ctor->unqualified_name = arena.strdup("StaleFieldCtor");
  stale_ctor->is_constructor = true;
  stale_ctor->type.base = c4c::TB_VOID;
  stale_ctor->n_params = 1;
  stale_ctor->params = arena.alloc_array<c4c::Node*>(1);
  stale_ctor->params[0] = stale_param;
  lowerer.struct_constructors_["StaleFieldCtor"].push_back(
      {"StaleFieldCtor__ctor", stale_ctor});

  c4c::TypeSpec field_ts{};
  field_ts.array_size = -1;
  field_ts.inner_rank = -1;
  field_ts.base = c4c::TB_STRUCT;
  field_ts.tag = arena.strdup("StaleFieldCtor");
  field_ts.record_def = real_field_record;

  c4c::hir::HirStructField field;
  field.name = "field";
  field.field_text_id = module.link_name_texts->intern("field");
  field.elem_type = field_ts;
  c4c::hir::HirStructDef outer_def;
  outer_def.tag = "OuterCtorDispatch";
  outer_def.tag_text_id = module.link_name_texts->intern("OuterCtorDispatch");
  outer_def.ns_qual.context_id = parser.current_namespace_context_id();
  outer_def.fields.push_back(field);
  module.struct_defs[outer_def.tag] = outer_def;

  c4c::Node* arg = parser.make_node(c4c::NK_INT_LIT, 1);
  arg->ival = 7;
  arg->type = int_ts;
  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 1);
  body->ival = 1;
  c4c::Node* ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  ctor->name = arena.strdup("OuterCtorDispatch");
  ctor->unqualified_name = arena.strdup("OuterCtorDispatch");
  ctor->is_constructor = true;
  ctor->type.base = c4c::TB_VOID;
  ctor->body = body;
  ctor->n_ctor_inits = 1;
  ctor->ctor_init_names = arena.alloc_array<const char*>(1);
  ctor->ctor_init_names[0] = arena.strdup("field");
  ctor->ctor_init_nargs = arena.alloc_array<int>(1);
  ctor->ctor_init_nargs[0] = 1;
  ctor->ctor_init_args = arena.alloc_array<c4c::Node**>(1);
  ctor->ctor_init_args[0] = arena.alloc_array<c4c::Node*>(1);
  ctor->ctor_init_args[0][0] = arg;

  lowerer.lower_struct_method(
      "OuterCtorDispatch__ctor", "OuterCtorDispatch", ctor, nullptr, nullptr);

  std::string lowered_callee;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    const auto* call = std::get_if<c4c::hir::CallExpr>(&expr.payload);
    if (!call || call->args.empty()) continue;
    const c4c::hir::Expr* callee = module.find_expr(call->callee);
    if (!callee) continue;
    if (const auto* ref = std::get_if<c4c::hir::DeclRef>(&callee->payload);
        ref && (ref->name == "RealFieldCtor__ctor" ||
                ref->name == "StaleFieldCtor__ctor")) {
      lowered_callee = ref->name;
      break;
    }
  }

  expect_eq(lowered_callee, "RealFieldCtor__ctor",
            "constructor initializer should use structured field owner before stale rendered tag");
}

void test_hir_local_decl_direct_constructor_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealLocalDirectCtor");
  real_record->unqualified_name = arena.strdup("RealLocalDirectCtor");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for direct local constructor");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealLocalDirectCtor";
  real_def.tag_text_id = module.link_name_texts->intern("RealLocalDirectCtor");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleLocalDirectCtor";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleLocalDirectCtor");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  c4c::Node* real_param = parser.make_node(c4c::NK_DECL, 1);
  real_param->type = int_ts;
  c4c::Node* real_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  real_ctor->is_constructor = true;
  real_ctor->n_params = 1;
  real_ctor->params = arena.alloc_array<c4c::Node*>(1);
  real_ctor->params[0] = real_param;
  lowerer.struct_constructors_["RealLocalDirectCtor"].push_back(
      {"RealLocalDirectCtor__ctor", real_ctor});

  c4c::Node* stale_param = parser.make_node(c4c::NK_DECL, 1);
  stale_param->type = int_ts;
  c4c::Node* stale_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  stale_ctor->is_constructor = true;
  stale_ctor->n_params = 1;
  stale_ctor->params = arena.alloc_array<c4c::Node*>(1);
  stale_ctor->params[0] = stale_param;
  lowerer.struct_constructors_["StaleLocalDirectCtor"].push_back(
      {"StaleLocalDirectCtor__ctor", stale_ctor});

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleLocalDirectCtor");
  owner_ts.record_def = real_record;

  c4c::Node* arg = parser.make_node(c4c::NK_INT_LIT, 1);
  arg->ival = 7;
  arg->type = int_ts;
  c4c::Node* decl = parser.make_node(c4c::NK_DECL, 1);
  decl->name = arena.strdup("local");
  decl->unqualified_name = arena.strdup("local");
  decl->type = owner_ts;
  decl->is_ctor_init = true;
  decl->children = arena.alloc_array<c4c::Node*>(1);
  decl->children[0] = arg;
  decl->n_children = 1;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  lowerer.lower_local_decl_stmt(ctx, decl);

  std::string lowered_callee;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    const auto* call = std::get_if<c4c::hir::CallExpr>(&expr.payload);
    if (!call) continue;
    const c4c::hir::Expr* callee = module.find_expr(call->callee);
    if (!callee) continue;
    if (const auto* ref = std::get_if<c4c::hir::DeclRef>(&callee->payload);
        ref && (ref->name == "RealLocalDirectCtor__ctor" ||
                ref->name == "StaleLocalDirectCtor__ctor")) {
      lowered_callee = ref->name;
      break;
    }
  }

  expect_eq(lowered_callee, "RealLocalDirectCtor__ctor",
            "local direct constructor init should use structured owner before stale rendered tag");
}

void test_hir_local_decl_default_constructor_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealLocalDefaultCtor");
  real_record->unqualified_name = arena.strdup("RealLocalDefaultCtor");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for default local constructor");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealLocalDefaultCtor";
  real_def.tag_text_id = module.link_name_texts->intern("RealLocalDefaultCtor");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleLocalDefaultCtor";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleLocalDefaultCtor");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* real_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  real_ctor->is_constructor = true;
  real_ctor->n_params = 0;
  lowerer.struct_constructors_["RealLocalDefaultCtor"].push_back(
      {"RealLocalDefaultCtor__ctor", real_ctor});
  c4c::Node* stale_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  stale_ctor->is_constructor = true;
  stale_ctor->n_params = 0;
  lowerer.struct_constructors_["StaleLocalDefaultCtor"].push_back(
      {"StaleLocalDefaultCtor__ctor", stale_ctor});

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleLocalDefaultCtor");
  owner_ts.record_def = real_record;

  c4c::Node* decl = parser.make_node(c4c::NK_DECL, 1);
  decl->name = arena.strdup("local");
  decl->unqualified_name = arena.strdup("local");
  decl->type = owner_ts;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  lowerer.lower_local_decl_stmt(ctx, decl);

  std::string lowered_callee;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    const auto* call = std::get_if<c4c::hir::CallExpr>(&expr.payload);
    if (!call) continue;
    const c4c::hir::Expr* callee = module.find_expr(call->callee);
    if (!callee) continue;
    if (const auto* ref = std::get_if<c4c::hir::DeclRef>(&callee->payload);
        ref && (ref->name == "RealLocalDefaultCtor__ctor" ||
                ref->name == "StaleLocalDefaultCtor__ctor")) {
      lowered_callee = ref->name;
      break;
    }
  }

  expect_eq(lowered_callee, "RealLocalDefaultCtor__ctor",
            "local implicit default constructor should use structured owner before stale rendered tag");
}

void test_hir_local_decl_copy_constructor_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealLocalCopyCtor");
  real_record->unqualified_name = arena.strdup("RealLocalCopyCtor");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for copy local constructor");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealLocalCopyCtor";
  real_def.tag_text_id = module.link_name_texts->intern("RealLocalCopyCtor");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleLocalCopyCtor";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleLocalCopyCtor");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleLocalCopyCtor");
  owner_ts.record_def = real_record;

  c4c::TypeSpec copy_param_ts = owner_ts;
  copy_param_ts.is_lvalue_ref = true;
  c4c::Node* real_param = parser.make_node(c4c::NK_DECL, 1);
  real_param->type = copy_param_ts;
  c4c::Node* real_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  real_ctor->is_constructor = true;
  real_ctor->n_params = 1;
  real_ctor->params = arena.alloc_array<c4c::Node*>(1);
  real_ctor->params[0] = real_param;
  lowerer.struct_constructors_["RealLocalCopyCtor"].push_back(
      {"RealLocalCopyCtor__copy_ctor", real_ctor});

  c4c::TypeSpec stale_param_ts = owner_ts;
  stale_param_ts.record_def = nullptr;
  stale_param_ts.is_lvalue_ref = true;
  c4c::Node* stale_param = parser.make_node(c4c::NK_DECL, 1);
  stale_param->type = stale_param_ts;
  c4c::Node* stale_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  stale_ctor->is_constructor = true;
  stale_ctor->n_params = 1;
  stale_ctor->params = arena.alloc_array<c4c::Node*>(1);
  stale_ctor->params[0] = stale_param;
  lowerer.struct_constructors_["StaleLocalCopyCtor"].push_back(
      {"StaleLocalCopyCtor__copy_ctor", stale_ctor});

  c4c::Node* init = parser.make_node(c4c::NK_VAR, 1);
  init->name = arena.strdup("source");
  init->type = owner_ts;
  c4c::Node* decl = parser.make_node(c4c::NK_DECL, 1);
  decl->name = arena.strdup("local");
  decl->unqualified_name = arena.strdup("local");
  decl->type = owner_ts;
  decl->init = init;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  const c4c::hir::LocalId source_id{17};
  ctx.locals["source"] = source_id;
  ctx.local_types.insert(source_id, owner_ts);
  lowerer.lower_local_decl_stmt(ctx, decl);

  std::string lowered_callee;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    const auto* call = std::get_if<c4c::hir::CallExpr>(&expr.payload);
    if (!call) continue;
    const c4c::hir::Expr* callee = module.find_expr(call->callee);
    if (!callee) continue;
    if (const auto* ref = std::get_if<c4c::hir::DeclRef>(&callee->payload);
        ref && (ref->name == "RealLocalCopyCtor__copy_ctor" ||
                ref->name == "StaleLocalCopyCtor__copy_ctor")) {
      lowered_callee = ref->name;
      break;
    }
  }

  expect_eq(lowered_callee, "RealLocalCopyCtor__copy_ctor",
            "local copy constructor should use structured owner before stale rendered tag");
}

void test_hir_local_decl_destructor_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealLocalDtor");
  real_record->unqualified_name = arena.strdup("RealLocalDtor");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for local destructor");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealLocalDtor";
  real_def.tag_text_id = module.link_name_texts->intern("RealLocalDtor");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleLocalDtor";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleLocalDtor");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* real_dtor = parser.make_node(c4c::NK_FUNCTION, 1);
  real_dtor->is_destructor = true;
  real_dtor->type.base = c4c::TB_VOID;
  lowerer.struct_destructors_["RealLocalDtor"] = {"RealLocalDtor__dtor", real_dtor};
  c4c::Node* stale_dtor = parser.make_node(c4c::NK_FUNCTION, 1);
  stale_dtor->is_destructor = true;
  stale_dtor->type.base = c4c::TB_VOID;
  lowerer.struct_destructors_["StaleLocalDtor"] = {"StaleLocalDtor__dtor",
                                                   stale_dtor};

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleLocalDtor");
  owner_ts.record_def = real_record;

  c4c::Node* decl = parser.make_node(c4c::NK_DECL, 1);
  decl->name = arena.strdup("local");
  decl->unqualified_name = arena.strdup("local");
  decl->type = owner_ts;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  lowerer.lower_local_decl_stmt(ctx, decl);

  expect_true(ctx.dtor_stack.size() == 1,
              "local declaration with destructor should be tracked for scope exit");
  expect_eq(ctx.dtor_stack.back().struct_tag, "RealLocalDtor",
            "local destructor tracking should use structured owner before stale rendered tag");
}

void test_hir_local_decl_member_dtor_tracking_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealLocalMemberDtor");
  real_record->unqualified_name = arena.strdup("RealLocalMemberDtor");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for local member dtor");

  c4c::TypeSpec field_ts{};
  field_ts.array_size = -1;
  field_ts.inner_rank = -1;
  field_ts.base = c4c::TB_STRUCT;
  field_ts.tag = arena.strdup("InnerLocalDtor");

  c4c::hir::HirStructDef inner_def;
  inner_def.tag = "InnerLocalDtor";
  inner_def.tag_text_id = module.link_name_texts->intern("InnerLocalDtor");
  inner_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[inner_def.tag] = inner_def;
  c4c::Node* inner_dtor = parser.make_node(c4c::NK_FUNCTION, 1);
  inner_dtor->is_destructor = true;
  inner_dtor->type.base = c4c::TB_VOID;
  lowerer.struct_destructors_["InnerLocalDtor"] = {"InnerLocalDtor__dtor",
                                                   inner_dtor};

  c4c::hir::HirStructField field;
  field.name = "field";
  field.field_text_id = module.link_name_texts->intern("field");
  field.elem_type = field_ts;
  field.member_symbol_id = module.member_symbols.intern("RealLocalMemberDtor::field");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealLocalMemberDtor";
  real_def.tag_text_id = module.link_name_texts->intern("RealLocalMemberDtor");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  real_def.fields.push_back(field);
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleLocalMemberDtor";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleLocalMemberDtor");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleLocalMemberDtor");
  owner_ts.record_def = real_record;

  c4c::Node* decl = parser.make_node(c4c::NK_DECL, 1);
  decl->name = arena.strdup("local");
  decl->unqualified_name = arena.strdup("local");
  decl->type = owner_ts;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  lowerer.lower_local_decl_stmt(ctx, decl);

  expect_true(ctx.dtor_stack.size() == 1,
              "local declaration with member dtors should be tracked for scope exit");
  expect_eq(ctx.dtor_stack.back().struct_tag, "RealLocalMemberDtor",
            "local member-dtor tracking should use structured owner before stale rendered tag");
}

void test_hir_defaulted_copy_member_symbol_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealDefaultedOwner");
  real_record->unqualified_name = arena.strdup("RealDefaultedOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for defaulted copy");
  lowerer.struct_def_nodes_["StaleDefaultedOwner"] = real_record;

  c4c::TypeSpec int_ts{};
  int_ts.base = c4c::TB_INT;
  const c4c::MemberSymbolId real_id =
      module.member_symbols.intern("RealDefaultedOwner::field");
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleDefaultedOwner::field");

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealDefaultedOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealDefaultedOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleDefaultedOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleDefaultedOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  c4c::hir::HirStructField field;
  field.name = "field";
  field.field_text_id = module.link_name_texts->intern("field");
  field.elem_type = int_ts;
  field.member_symbol_id = stale_id;
  stale_def.fields.push_back(field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec owner_ts{};
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleDefaultedOwner");
  owner_ts.ptr_level = 1;
  c4c::Node* param = parser.make_node(c4c::NK_DECL, 1);
  param->name = arena.strdup("other");
  param->type = owner_ts;
  c4c::Node* method = parser.make_node(c4c::NK_FUNCTION, 1);
  method->name = arena.strdup("StaleDefaultedOwner");
  method->unqualified_name = arena.strdup("StaleDefaultedOwner");
  method->is_constructor = true;
  method->is_defaulted = true;
  method->type.base = c4c::TB_VOID;
  method->n_params = 1;
  method->params = arena.alloc_array<c4c::Node*>(1);
  method->params[0] = param;

  lowerer.lower_struct_method(
      "StaleDefaultedOwner__copy_ctor", "StaleDefaultedOwner", method, nullptr,
      nullptr);

  const c4c::hir::MemberExpr* member = nullptr;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    if (const auto* candidate = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
        candidate && candidate->field == "field") {
      member = candidate;
      break;
    }
  }
  expect_true(member != nullptr,
              "defaulted copy should lower a generated member access");
  expect_true(member->resolved_owner_tag == "RealDefaultedOwner",
              "defaulted copy member access should resolve structured owner tag first");
  expect_true(member->member_symbol_id == real_id &&
                  member->member_symbol_id != stale_id,
              "defaulted copy member symbol should prefer record_def owner over stale tag");
}

void test_hir_member_dtor_member_symbol_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealDtorOwner");
  real_record->unqualified_name = arena.strdup("RealDtorOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for member dtor");
  lowerer.struct_def_nodes_["StaleDtorOwner"] = real_record;

  c4c::TypeSpec field_ts{};
  field_ts.base = c4c::TB_STRUCT;
  field_ts.tag = arena.strdup("InnerDtor");
  const c4c::MemberSymbolId real_id =
      module.member_symbols.intern("RealDtorOwner::field");
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleDtorOwner::field");

  c4c::hir::HirStructDef inner_def;
  inner_def.tag = "InnerDtor";
  inner_def.tag_text_id = module.link_name_texts->intern("InnerDtor");
  inner_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.struct_defs[inner_def.tag] = inner_def;
  lowerer.struct_destructors_["InnerDtor"] = {"InnerDtor__dtor", nullptr};

  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealDtorOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealDtorOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleDtorOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleDtorOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  c4c::hir::HirStructField field;
  field.name = "field";
  field.field_text_id = module.link_name_texts->intern("field");
  field.elem_type = field_ts;
  field.member_symbol_id = stale_id;
  stale_def.fields.push_back(field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* method = parser.make_node(c4c::NK_FUNCTION, 1);
  method->name = arena.strdup("~StaleDtorOwner");
  method->unqualified_name = arena.strdup("~StaleDtorOwner");
  method->is_destructor = true;
  method->is_defaulted = true;
  method->type.base = c4c::TB_VOID;

  lowerer.lower_struct_method(
      "StaleDtorOwner__dtor", "StaleDtorOwner", method, nullptr, nullptr);

  const c4c::hir::MemberExpr* member = nullptr;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    if (const auto* candidate = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
        candidate && candidate->field == "field") {
      member = candidate;
      break;
    }
  }
  expect_true(member != nullptr,
              "member destructor lowering should lower the destroyed member access");
  expect_true(member->resolved_owner_tag == "RealDtorOwner",
              "member destructor access should resolve structured owner tag first");
  expect_true(member->member_symbol_id == real_id &&
                  member->member_symbol_id != stale_id,
              "member destructor symbol should prefer record_def owner over stale tag");
}

void test_hir_local_decl_member_symbol_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealDeclOwner");
  real_record->unqualified_name = arena.strdup("RealDeclOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for local declaration");

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  const c4c::MemberSymbolId real_id =
      module.member_symbols.intern("RealDeclOwner::field");
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleDeclOwner::field");

  c4c::hir::HirStructField real_field;
  real_field.name = "field";
  real_field.field_text_id = module.link_name_texts->intern("field");
  real_field.elem_type = int_ts;
  real_field.member_symbol_id = real_id;
  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealDeclOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealDeclOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  real_def.fields.push_back(real_field);
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructField stale_field;
  stale_field.name = "wrong_field";
  stale_field.field_text_id = module.link_name_texts->intern("wrong_field");
  stale_field.elem_type = int_ts;
  stale_field.member_symbol_id = stale_id;
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleDeclOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleDeclOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  stale_def.fields.push_back(stale_field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleDeclOwner");
  owner_ts.record_def = real_record;

  c4c::Node* value = parser.make_node(c4c::NK_INT_LIT, 1);
  value->ival = 7;
  value->type = int_ts;
  c4c::Node* init = parser.make_node(c4c::NK_INIT_LIST, 1);
  init->children = arena.alloc_array<c4c::Node*>(1);
  init->children[0] = value;
  init->n_children = 1;
  c4c::Node* decl = parser.make_node(c4c::NK_DECL, 1);
  decl->name = arena.strdup("local");
  decl->unqualified_name = arena.strdup("local");
  decl->type = owner_ts;
  decl->init = init;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  lowerer.lower_local_decl_stmt(ctx, decl);

  const c4c::hir::MemberExpr* member = nullptr;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    if (const auto* candidate = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
        candidate && candidate->field == "field") {
      member = candidate;
      break;
    }
  }
  expect_true(member != nullptr,
              "local declaration aggregate init should lower the member access");
  expect_true(member->field == "field",
              "local declaration aggregate init must select fields from the structured owner");
  expect_true(member->resolved_owner_tag == "RealDeclOwner",
              "local declaration member access should resolve structured owner tag first");
  expect_true(member->member_symbol_id == real_id &&
                  member->member_symbol_id != stale_id,
              "local declaration member symbol should prefer record_def owner over stale tag");
}

void test_hir_local_decl_structured_owner_failure_does_not_use_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* unresolved_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  unresolved_record->name = arena.strdup("UnresolvedStructuredDeclOwner");
  unresolved_record->unqualified_name =
      arena.strdup("UnresolvedStructuredDeclOwner");
  unresolved_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleFailedDeclOwner::wrong_field");

  c4c::hir::HirStructField stale_field;
  stale_field.name = "wrong_field";
  stale_field.field_text_id = module.link_name_texts->intern("wrong_field");
  stale_field.elem_type = int_ts;
  stale_field.member_symbol_id = stale_id;
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleFailedDeclOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleFailedDeclOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  stale_def.fields.push_back(stale_field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::Node* stale_ctor = parser.make_node(c4c::NK_FUNCTION, 1);
  stale_ctor->is_constructor = true;
  stale_ctor->n_params = 0;
  lowerer.struct_constructors_["StaleFailedDeclOwner"].push_back(
      {"StaleFailedDeclOwner__ctor", stale_ctor});
  c4c::Node* stale_dtor = parser.make_node(c4c::NK_FUNCTION, 1);
  stale_dtor->is_destructor = true;
  stale_dtor->type.base = c4c::TB_VOID;
  lowerer.struct_destructors_["StaleFailedDeclOwner"] = {
      "StaleFailedDeclOwner__dtor", stale_dtor};

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleFailedDeclOwner");
  owner_ts.record_def = unresolved_record;

  c4c::Node* value = parser.make_node(c4c::NK_INT_LIT, 1);
  value->ival = 7;
  value->type = int_ts;
  c4c::Node* init = parser.make_node(c4c::NK_INIT_LIST, 1);
  init->children = arena.alloc_array<c4c::Node*>(1);
  init->children[0] = value;
  init->n_children = 1;
  c4c::Node* decl = parser.make_node(c4c::NK_DECL, 1);
  decl->name = arena.strdup("local");
  decl->unqualified_name = arena.strdup("local");
  decl->type = owner_ts;
  decl->init = init;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  lowerer.lower_local_decl_stmt(ctx, decl);

  bool used_stale_member = false;
  bool used_stale_ctor = false;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    if (const auto* member = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
        member && (member->resolved_owner_tag == "StaleFailedDeclOwner" ||
                   member->member_symbol_id == stale_id)) {
      used_stale_member = true;
    }
    const auto* call = std::get_if<c4c::hir::CallExpr>(&expr.payload);
    if (!call) continue;
    const c4c::hir::Expr* callee_expr = module.find_expr(call->callee);
    if (!callee_expr) continue;
    if (const auto* ref = std::get_if<c4c::hir::DeclRef>(&callee_expr->payload);
        ref && ref->name == "StaleFailedDeclOwner__ctor") {
      used_stale_ctor = true;
    }
  }

  expect_true(!used_stale_member,
              "failed structured declaration owner resolution must not use stale rendered fields");
  expect_true(ctx.dtor_stack.empty(),
              "failed structured declaration owner resolution must not use stale rendered destructors");

  c4c::Node* default_decl = parser.make_node(c4c::NK_DECL, 1);
  default_decl->name = arena.strdup("default_local");
  default_decl->unqualified_name = arena.strdup("default_local");
  default_decl->type = owner_ts;
  c4c::hir::Lowerer::FunctionCtx default_ctx;
  c4c::hir::Function default_fn;
  default_ctx.fn = &default_fn;
  default_ctx.current_block = c4c::hir::BlockId{0};
  lowerer.lower_local_decl_stmt(default_ctx, default_decl);

  for (const c4c::hir::Expr& expr : module.expr_pool) {
    const auto* call = std::get_if<c4c::hir::CallExpr>(&expr.payload);
    if (!call) continue;
    const c4c::hir::Expr* callee_expr = module.find_expr(call->callee);
    if (!callee_expr) continue;
    if (const auto* ref = std::get_if<c4c::hir::DeclRef>(&callee_expr->payload);
        ref && ref->name == "StaleFailedDeclOwner__ctor") {
      used_stale_ctor = true;
    }
  }
  expect_true(!used_stale_ctor,
              "failed structured declaration owner resolution must not use stale rendered constructors");
  expect_true(default_ctx.dtor_stack.empty(),
              "failed structured default declaration owner resolution must not use stale rendered destructors");
}

void test_hir_compound_literal_member_symbol_prefers_record_def_over_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* real_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_record->name = arena.strdup("RealCompoundOwner");
  real_record->unqualified_name = arena.strdup("RealCompoundOwner");
  real_record->namespace_context_id = parser.current_namespace_context_id();
  const std::optional<c4c::hir::HirRecordOwnerKey> owner_key =
      lowerer.make_struct_def_node_owner_key(real_record);
  expect_true(owner_key.has_value(),
              "fixture should build a structured owner key for compound literal");

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  const c4c::MemberSymbolId real_id =
      module.member_symbols.intern("RealCompoundOwner::field");
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleCompoundOwner::field");

  c4c::hir::HirStructField real_field;
  real_field.name = "field";
  real_field.field_text_id = module.link_name_texts->intern("field");
  real_field.elem_type = int_ts;
  real_field.member_symbol_id = real_id;
  c4c::hir::HirStructDef real_def;
  real_def.tag = "RealCompoundOwner";
  real_def.tag_text_id = module.link_name_texts->intern("RealCompoundOwner");
  real_def.ns_qual.context_id = parser.current_namespace_context_id();
  real_def.fields.push_back(real_field);
  module.index_struct_def_owner(*owner_key, real_def.tag, true);
  module.struct_defs[real_def.tag] = real_def;

  c4c::hir::HirStructField stale_field = real_field;
  stale_field.member_symbol_id = stale_id;
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleCompoundOwner";
  stale_def.tag_text_id = module.link_name_texts->intern("StaleCompoundOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  stale_def.fields.push_back(stale_field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleCompoundOwner");
  owner_ts.record_def = real_record;

  c4c::Node* value = parser.make_node(c4c::NK_INT_LIT, 1);
  value->ival = 7;
  value->type = int_ts;
  c4c::Node* init = parser.make_node(c4c::NK_INIT_LIST, 1);
  init->children = arena.alloc_array<c4c::Node*>(1);
  init->children[0] = value;
  init->n_children = 1;
  c4c::Node* compound = parser.make_node(c4c::NK_COMPOUND_LIT, 1);
  compound->type = owner_ts;
  compound->left = init;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  const c4c::hir::ExprId expr_id =
      lowerer.lower_compound_literal_expr(&ctx, compound);
  expect_true(expr_id.valid(), "compound literal lowering should produce a base expression");

  const c4c::hir::MemberExpr* member = nullptr;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    if (const auto* candidate = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
        candidate && candidate->field == "field") {
      member = candidate;
      break;
    }
  }
  expect_true(member != nullptr,
              "compound literal aggregate init should lower the member access");
  expect_true(member->resolved_owner_tag == "RealCompoundOwner",
              "compound literal member access should resolve structured owner tag first");
  expect_true(member->member_symbol_id == real_id &&
                  member->member_symbol_id != stale_id,
              "compound literal member symbol should prefer record_def owner over stale tag");
}

void test_hir_compound_literal_structured_owner_failure_does_not_use_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::hir::Module module;
  c4c::hir::Lowerer lowerer;
  lowerer.module_ = &module;

  c4c::Node* unresolved_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  unresolved_record->name = arena.strdup("UnresolvedCompoundOwner");
  unresolved_record->unqualified_name = arena.strdup("UnresolvedCompoundOwner");
  unresolved_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec int_ts{};
  int_ts.array_size = -1;
  int_ts.inner_rank = -1;
  int_ts.base = c4c::TB_INT;
  const c4c::MemberSymbolId stale_id =
      module.member_symbols.intern("StaleFailedCompoundOwner::wrong_field");

  c4c::hir::HirStructField stale_field;
  stale_field.name = "wrong_field";
  stale_field.field_text_id = module.link_name_texts->intern("wrong_field");
  stale_field.elem_type = int_ts;
  stale_field.member_symbol_id = stale_id;
  c4c::hir::HirStructDef stale_def;
  stale_def.tag = "StaleFailedCompoundOwner";
  stale_def.tag_text_id =
      module.link_name_texts->intern("StaleFailedCompoundOwner");
  stale_def.ns_qual.context_id = parser.current_namespace_context_id();
  stale_def.fields.push_back(stale_field);
  module.struct_defs[stale_def.tag] = stale_def;

  c4c::TypeSpec owner_ts{};
  owner_ts.array_size = -1;
  owner_ts.inner_rank = -1;
  owner_ts.base = c4c::TB_STRUCT;
  owner_ts.tag = arena.strdup("StaleFailedCompoundOwner");
  owner_ts.record_def = unresolved_record;

  c4c::Node* value = parser.make_node(c4c::NK_INT_LIT, 1);
  value->ival = 7;
  value->type = int_ts;
  c4c::Node* init = parser.make_node(c4c::NK_INIT_LIST, 1);
  init->children = arena.alloc_array<c4c::Node*>(1);
  init->children[0] = value;
  init->n_children = 1;
  c4c::Node* compound = parser.make_node(c4c::NK_COMPOUND_LIT, 1);
  compound->type = owner_ts;
  compound->left = init;

  c4c::hir::Lowerer::FunctionCtx ctx;
  c4c::hir::Function fn;
  ctx.fn = &fn;
  ctx.current_block = c4c::hir::BlockId{0};
  const c4c::hir::ExprId expr_id =
      lowerer.lower_compound_literal_expr(&ctx, compound);
  expect_true(expr_id.valid(), "compound literal lowering should produce a base expression");

  bool used_stale_member = false;
  for (const c4c::hir::Expr& expr : module.expr_pool) {
    if (const auto* member = std::get_if<c4c::hir::MemberExpr>(&expr.payload);
        member && (member->resolved_owner_tag == "StaleFailedCompoundOwner" ||
                   member->member_symbol_id == stale_id)) {
      used_stale_member = true;
    }
  }

  expect_true(!used_stale_member,
              "failed structured compound-literal owner resolution must not use stale rendered fields");
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
  test_hir_to_lir_global_initializer_reachability_prefers_structured_function_ids();
  test_hir_to_lir_direct_call_target_resolution_prefers_link_name_ids();
  test_hir_to_lir_direct_call_lookup_rejects_rendered_name_collision_after_link_name_miss();
  test_hir_to_lir_global_initializer_lookup_prefers_semantic_global_identity();
  test_hir_to_lir_decl_backed_function_designator_rvalues_prefer_link_name_ids();
  test_hir_to_lir_decl_backed_call_result_inference_prefers_link_name_ids();
  test_hir_to_lir_object_helper_callees_prefer_link_name_ids();
  test_hir_to_lir_template_call_helper_callees_prefer_carrier_link_name_ids();
  test_hir_direct_call_builtin_alias_fallback_keeps_invalid_link_name_ids();
  test_hir_template_arg_materialization_prefers_structured_value_payload();
  test_hir_template_arg_materialization_keeps_legacy_zero_value_fallback();
  test_hir_pending_type_ref_keeps_legacy_zero_value_debug_text();
  test_hir_deferred_member_typedef_prefers_record_def_over_stale_tag();
  test_hir_member_owner_lookup_prefers_record_def_over_stale_tag();
  test_hir_member_owner_lookup_prefers_template_origin_over_stale_tag();
  test_hir_template_struct_primary_lookup_prefers_record_def_over_stale_origin();
  test_hir_template_struct_specialization_lookup_prefers_owner_key_over_stale_primary_name();
  test_hir_member_owner_lookup_record_def_failure_does_not_use_stale_tag();
  test_hir_member_owner_lookup_generated_record_def_without_key_keeps_tag_fallback();
  test_hir_member_owner_lookup_template_args_failure_does_not_use_stale_tag();
  test_hir_member_expr_owner_failure_does_not_use_stale_tag();
  test_hir_static_member_const_lookup_prefers_template_owner_key_over_stale_tag();
  test_hir_static_member_const_lookup_keeps_rendered_fallback_without_owner_key();
  test_hir_static_member_const_lookup_structured_miss_keeps_base_fallback();
  test_hir_static_member_decl_lookup_prefers_template_owner_key_over_stale_tag();
  test_hir_static_member_decl_lookup_keeps_rendered_fallback_without_owner_key();
  test_hir_member_symbol_lookup_prefers_owner_key_over_stale_rendered_spelling();
  test_hir_scoped_static_member_lowering_prefers_record_def_over_stale_tag();
  test_hir_struct_method_lookup_prefers_template_owner_key_over_stale_tag();
  test_hir_struct_method_lookup_keeps_rendered_fallback_without_owner_key();
  test_hir_out_of_class_method_attachment_prefers_structured_owner_key();
  test_hir_out_of_class_method_skip_prefers_structured_owner_key();
  test_hir_range_for_method_owner_prefers_record_def_over_stale_tag();
  test_hir_operator_call_method_owner_prefers_record_def_over_stale_tag();
  test_hir_operator_call_method_owner_keeps_rendered_fallback();
  test_hir_generic_ctrl_deref_method_owner_prefers_record_def_over_stale_tag();
  test_hir_generic_ctrl_operator_call_method_owner_prefers_record_def_over_stale_tag();
  test_hir_member_call_method_owner_prefers_record_def_over_stale_tag();
  test_hir_direct_constructor_call_prefers_record_def_over_stale_tag();
  test_hir_new_expr_method_owner_prefers_record_def_over_stale_tag();
  test_hir_delete_expr_method_owner_prefers_record_def_over_stale_tag();
  test_hir_init_list_member_symbol_prefers_record_def_over_stale_tag();
  test_hir_implicit_this_member_symbol_prefers_record_def_over_stale_tag();
  test_hir_ctor_init_member_symbol_prefers_record_def_over_stale_tag();
  test_hir_ctor_init_field_constructor_prefers_record_def_over_stale_tag();
  test_hir_local_decl_direct_constructor_prefers_record_def_over_stale_tag();
  test_hir_local_decl_default_constructor_prefers_record_def_over_stale_tag();
  test_hir_local_decl_copy_constructor_prefers_record_def_over_stale_tag();
  test_hir_local_decl_destructor_prefers_record_def_over_stale_tag();
  test_hir_local_decl_member_dtor_tracking_prefers_record_def_over_stale_tag();
  test_hir_defaulted_copy_member_symbol_prefers_record_def_over_stale_tag();
  test_hir_member_dtor_member_symbol_prefers_record_def_over_stale_tag();
  test_hir_local_decl_member_symbol_prefers_record_def_over_stale_tag();
  test_hir_local_decl_structured_owner_failure_does_not_use_stale_tag();
  test_hir_compound_literal_member_symbol_prefers_record_def_over_stale_tag();
  test_hir_compound_literal_structured_owner_failure_does_not_use_stale_tag();
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
