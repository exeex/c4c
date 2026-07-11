#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/bir_control_flow_view.hpp"
#include "src/backend/bir/bir_return_view.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace {
namespace bir = c4c::backend::bir;
int fail(const char* message) { std::cerr << message << '\n'; return 1; }

int return_facts_and_statuses() {
  bir::Block available_block;
  available_block.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "%result")};
  const auto available = bir::find_return(
      bir::make_bir_return_view(available_block));

  bir::Block incomplete_block;
  incomplete_block.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "")};
  const auto incomplete = bir::find_return(
      bir::make_bir_return_view(incomplete_block));

  bir::Block ambiguous_block;
  ambiguous_block.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(1),
      .return_lanes = {bir::Value::immediate_i32(2)}};
  const auto ambiguous = bir::find_return(
      bir::make_bir_return_view(ambiguous_block));

  bir::Block branch_block;
  branch_block.terminator = bir::BranchTerminator{.target_label = "next"};
  const auto unavailable = bir::find_return(
      bir::make_bir_return_view(branch_block));
  if (!available || !available.return_identity || !available.returned_value ||
      available.provenance_value != available.returned_value ||
      !available.chain_complete ||
      unavailable.status != bir::BirViewStatus::Unavailable || unavailable ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete ||
      ambiguous.status != bir::BirViewStatus::Ambiguous || ambiguous)
    return fail("return view facts or four-state behavior failed");
  return 0;
}

bir::SelectInst selection(const char* result) {
  return bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, result),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%condition"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = bir::Value::immediate_i32(1),
      .false_value = bir::Value::immediate_i32(2)};
}

int control_facts_and_statuses() {
  bir::Function function;
  bir::Block from;
  from.label = "from";
  from.insts.emplace_back(selection("%selected"));
  from.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "%condition"),
      .true_label = "join", .false_label = "other"};
  bir::Block join;
  join.label = "join";
  join.insts.emplace_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%joined"),
      .incomings = {{.label = "from", .value = bir::Value::immediate_i32(1)}}});
  function.blocks = {from, join};
  const auto view = bir::make_bir_control_flow_view(function);
  const auto available_select = bir::find_selection(view, function.blocks[0], 0);
  const auto available_branch = bir::find_branch_condition(view, function.blocks[0]);
  const auto available_edge = bir::find_block_relationship(
      view, function.blocks[0], function.blocks[1]);
  const auto available_join = bir::find_join_source(
      view, function.blocks[1], 0, function.blocks[0]);
  const auto unavailable = bir::find_selection(view, function.blocks[0], 99);

  bir::Function incomplete_function;
  incomplete_function.blocks.emplace_back();
  incomplete_function.blocks[0].insts.emplace_back(selection(""));
  const auto incomplete = bir::find_selection(
      bir::make_bir_control_flow_view(incomplete_function),
      incomplete_function.blocks[0], 0);

  bir::Function ambiguous_function = function;
  ambiguous_function.blocks.push_back(ambiguous_function.blocks[1]);
  const auto ambiguous_view =
      bir::make_bir_control_flow_view(ambiguous_function);
  const auto ambiguous = bir::find_block_relationship(
      ambiguous_view, ambiguous_function.blocks[0], ambiguous_function.blocks[1]);
  if (!available_select || !available_select.selection ||
      !available_select.condition || !available_select.true_value ||
      !available_select.false_value || !available_branch ||
      !available_branch.condition || !available_edge ||
      available_edge.relationship_name != "join" || !available_join ||
      !available_join.join || !available_join.join_source ||
      unavailable.status != bir::BirViewStatus::Unavailable || unavailable ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete ||
      ambiguous.status != bir::BirViewStatus::Ambiguous || ambiguous)
    return fail("control-flow view facts or four-state behavior failed");
  return 0;
}

int headers_are_non_authoritative() {
  for (const char* path : {C4C_BIR_RETURN_VIEW_HEADER,
                           C4C_BIR_CONTROL_FLOW_VIEW_HEADER}) {
    std::ifstream input(path);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    for (const char* forbidden : {"Route", "route_", "prepared", "Prepared",
                                  "ABI", "Abi", "destination", "transfer",
                                  "stack_load", "target_label"})
      if (source.find(forbidden) != std::string::npos)
        return fail("return/control-flow header exposed forbidden authority vocabulary");
  }
  return 0;
}
}  // namespace

int main() {
  if (const int status = return_facts_and_statuses()) return status;
  if (const int status = control_facts_and_statuses()) return status;
  return headers_are_non_authoritative();
}
