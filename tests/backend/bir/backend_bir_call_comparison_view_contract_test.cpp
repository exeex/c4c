#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/bir_call_boundary_view.hpp"
#include "src/backend/bir/bir_comparison_view.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace {
namespace bir = c4c::backend::bir;

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

bir::CallInst call(const char* callee, const char* result_name) {
  bir::CallInst value;
  value.callee = callee;
  value.result = bir::Value::named(bir::TypeKind::I32, result_name);
  value.args.push_back(bir::Value::named(bir::TypeKind::I32, "%arg"));
  value.arg_sources.push_back(bir::CallArgumentSourceRelationship{
      .arg_index = 0,
      .source_value_name = std::string("%source"),
  });
  return value;
}

bir::BinaryInst comparison(const char* result_name) {
  return bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, result_name),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
      .rhs = bir::Value::immediate_i64(0),
  };
}

int call_statuses_and_facts() {
  bir::Block block;
  block.insts.emplace_back(call("callee", "%result"));
  auto incomplete_call = call("", "%unused");
  incomplete_call.callee_value.reset();
  block.insts.emplace_back(std::move(incomplete_call));
  auto ambiguous_call = call("duplicate", "%unused2");
  ambiguous_call.arg_sources.push_back(ambiguous_call.arg_sources.front());
  block.insts.emplace_back(std::move(ambiguous_call));
  const auto view = bir::make_bir_call_boundary_view(block);

  const auto available = bir::find_call_argument(view, 0U, 0U);
  const auto unavailable = bir::find_call(view, 99U);
  const auto incomplete = bir::find_call(view, 1U);
  const auto ambiguous = bir::find_call_argument(view, 2U, 0U);
  if (!available || available.callee != "callee" ||
      available.argument == nullptr || available.dependency_name != "%source" ||
      available.result == nullptr || available.argument_number != 0U ||
      unavailable.status != bir::BirViewStatus::Unavailable || unavailable ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete ||
      ambiguous.status != bir::BirViewStatus::Ambiguous || ambiguous) {
    return fail("call view did not expose narrow facts with four fail-closed statuses");
  }
  return 0;
}

int comparison_statuses_and_facts() {
  bir::Block available_block;
  available_block.insts.emplace_back(comparison("%condition"));
  available_block.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%condition"),
      .true_label = "yes",
      .false_label = "no",
  };
  const auto available = bir::find_comparison(
      bir::make_bir_comparison_view(available_block), 0U);

  bir::Block incomplete_block;
  incomplete_block.insts.emplace_back(comparison(""));
  const auto incomplete = bir::find_comparison(
      bir::make_bir_comparison_view(incomplete_block), 0U);

  bir::Block ambiguous_block;
  ambiguous_block.insts.emplace_back(comparison("%duplicate"));
  ambiguous_block.insts.emplace_back(comparison("%duplicate"));
  const auto ambiguous = bir::find_comparison(
      bir::make_bir_comparison_view(ambiguous_block), 0U);
  const auto unavailable = bir::find_comparison(bir::BirComparisonView{}, 0U);

  if (!available || available.comparison == nullptr ||
      available.predicate != bir::BinaryOpcode::Eq ||
      available.left_operand == nullptr || available.right_operand == nullptr ||
      available.materialized_condition == nullptr || !available.used_by_branch ||
      unavailable.status != bir::BirViewStatus::Unavailable || unavailable ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete ||
      ambiguous.status != bir::BirViewStatus::Ambiguous || ambiguous) {
    return fail("comparison view did not expose narrow facts with four fail-closed statuses");
  }
  return 0;
}

int headers_are_route_free_and_non_authoritative() {
  for (const char* path : {C4C_BIR_CALL_VIEW_HEADER,
                           C4C_BIR_COMPARISON_VIEW_HEADER}) {
    std::ifstream input(path);
    const std::string source((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
    for (const char* forbidden : {"Route", "route_", "index", "Index",
                                  "prepared", "Prepared", "ABI", "Abi",
                                  "target_label", "true_label", "false_label"}) {
      if (source.find(forbidden) != std::string::npos) {
        return fail("named call/comparison header exposed forbidden authority vocabulary");
      }
    }
  }
  return 0;
}
}  // namespace

int main() {
  if (const int status = call_statuses_and_facts(); status != 0) return status;
  if (const int status = comparison_statuses_and_facts(); status != 0) return status;
  return headers_are_route_free_and_non_authoritative();
}
