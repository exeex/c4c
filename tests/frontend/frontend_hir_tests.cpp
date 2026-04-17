#include "arena.hpp"
#include "hir_to_lir.hpp"
#include "hir/hir_ir.hpp"
#include "lir_printer.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "source_profile.hpp"
#include "backend/bir/lir_to_bir.hpp"

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
  c4c::Parser parser(tokens, arena, source_profile, "frontend_hir_tests.cpp");
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
  test_hir_to_lir_forwards_function_link_name_ids();
  test_lir_printer_resolves_link_names_at_emission_boundary();
  test_lir_printer_resolves_specialization_metadata_link_names();
  test_lir_printer_resolves_direct_call_link_names_at_emission_boundary();
  test_lir_to_bir_resolves_link_names_at_backend_boundary();
  test_lir_to_bir_analysis_resolves_link_names_at_backend_boundary();
  test_lir_to_bir_resolves_direct_call_link_names_at_backend_boundary();
  test_lir_to_bir_resolves_decl_backed_direct_call_link_names_at_backend_boundary();

  std::cout << "PASS: frontend_hir_tests\n";
  return 0;
}
