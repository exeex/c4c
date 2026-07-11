#include "src/backend/prealloc/publication_plans.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {

namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << '\n';
  return 1;
}

int statuses_are_explicit_and_fail_closed() {
  const prepare::PreparedFactBoundaryEvidence positive{
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .function_name = c4c::FunctionNameId{1},
      .block_label = c4c::BlockLabelId{2},
      .value_name = c4c::ValueNameId{3},
      .instruction_index = 4,
  };
  if (!positive || prepare::prepared_fact_boundary_status_name(positive.status) !=
                       "available") {
    return fail("expected a positive identity-bound prepared fact");
  }

  for (const auto status : {prepare::PreparedFactBoundaryStatus::Missing,
                            prepare::PreparedFactBoundaryStatus::Incomplete,
                            prepare::PreparedFactBoundaryStatus::Ambiguous,
                            prepare::PreparedFactBoundaryStatus::Mismatched,
                            prepare::PreparedFactBoundaryStatus::Unsupported}) {
    const prepare::PreparedFactBoundaryEvidence negative{.status = status};
    if (negative) {
      return fail("expected negative named input to fail closed");
    }
  }
  return 0;
}

int current_block_named_producer_evidence_is_independent_and_unique() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("named_join_evidence");
  const auto block_label = names.block_labels.intern("entry");
  const auto value_name = names.value_names.intern("%sum");
  const auto other_value_name = names.value_names.intern("%other");
  c4c::backend::bir::Function function;
  function.name = "named_join_evidence";
  c4c::backend::bir::Block block;
  block.label = "entry";
  block.label_id = block_label;
  block.insts.push_back(c4c::backend::bir::BinaryInst{
      .opcode = c4c::backend::bir::BinaryOpcode::Add,
      .result = c4c::backend::bir::Value::named(
          c4c::backend::bir::TypeKind::I32, "%sum"),
      .operand_type = c4c::backend::bir::TypeKind::I32,
      .lhs = c4c::backend::bir::Value::immediate_i32(1),
      .rhs = c4c::backend::bir::Value::immediate_i32(2),
  });
  function.blocks.push_back(std::move(block));
  const auto evidence =
      prepare::make_prepared_current_block_join_source_evidence(
          names, function_name, function);
  if (evidence.size() != 1 || !evidence.front() ||
      evidence.front().function_name != function_name ||
      evidence.front().block_label != block_label ||
      evidence.front().value_name != value_name ||
      evidence.front().instruction_index != 0) {
    return fail("named BIR producer query should create stable current-block evidence");
  }
  const auto selected =
      prepare::select_prepared_current_block_join_source_evidence(
          evidence, function_name, block_label, value_name, 0);
  const auto missing =
      prepare::select_prepared_current_block_join_source_evidence(
          {}, function_name, block_label, value_name, 0);
  const auto incomplete =
      prepare::select_prepared_current_block_join_source_evidence(
          {prepare::PreparedFactBoundaryEvidence{
              .status = prepare::PreparedFactBoundaryStatus::Incomplete}},
          function_name, block_label, value_name, 0);
  const auto ambiguous =
      prepare::select_prepared_current_block_join_source_evidence(
          {evidence.front(), evidence.front()},
          function_name, block_label, value_name, 0);
  const auto mismatched =
      prepare::select_prepared_current_block_join_source_evidence(
          evidence, function_name, block_label, other_value_name, 0);
  if (!selected ||
      missing.status != prepare::PreparedFactBoundaryStatus::Missing ||
      incomplete.status != prepare::PreparedFactBoundaryStatus::Incomplete ||
      ambiguous.status != prepare::PreparedFactBoundaryStatus::Ambiguous ||
      mismatched.status != prepare::PreparedFactBoundaryStatus::Mismatched) {
    return fail("current-block evidence selection should fail closed and uniquely");
  }
  return 0;
}

int current_block_routing_facts_are_edge_bound_and_unique() {
  const prepare::PreparedCurrentBlockJoinRoutingFact fact{
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = c4c::BlockLabelId{1},
      .successor_label = c4c::BlockLabelId{2},
      .destination_value_id = prepare::PreparedValueId{10},
      .destination_value_name = c4c::ValueNameId{10},
      .source_value_id = prepare::PreparedValueId{20},
      .source_value_name = c4c::ValueNameId{20},
      .routed_value_id = prepare::PreparedValueId{20},
      .routed_value_name = c4c::ValueNameId{20},
      .role = prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression,
      .publication_semantic_origin =
          prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
              PublicationSemanticOrigin::PreparedJoinTransfer,
  };
  auto select = [&](const std::vector<prepare::PreparedCurrentBlockJoinRoutingFact>& facts,
                    c4c::BlockLabelId predecessor,
                    c4c::BlockLabelId successor,
                    prepare::PreparedValueId destination) {
    return prepare::select_prepared_current_block_join_routing_fact(
        facts,
        predecessor,
        successor,
        destination,
        c4c::ValueNameId{10},
        prepare::PreparedValueId{20},
        c4c::ValueNameId{20},
        prepare::PreparedValueId{20},
        c4c::ValueNameId{20},
        prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression);
  };
  const auto selected = select({fact}, c4c::BlockLabelId{1},
                               c4c::BlockLabelId{2},
                               prepare::PreparedValueId{10});
  const auto parallel_edge = select({fact}, c4c::BlockLabelId{3},
                                    c4c::BlockLabelId{2},
                                    prepare::PreparedValueId{10});
  const auto wrong_successor = select({fact}, c4c::BlockLabelId{1},
                                      c4c::BlockLabelId{4},
                                      prepare::PreparedValueId{10});
  const auto wrong_destination = select({fact}, c4c::BlockLabelId{1},
                                        c4c::BlockLabelId{2},
                                        prepare::PreparedValueId{11});
  const auto duplicate = select({fact, fact}, c4c::BlockLabelId{1},
                                c4c::BlockLabelId{2},
                                prepare::PreparedValueId{10});
  if (!selected ||
      parallel_edge.status != prepare::PreparedFactBoundaryStatus::Mismatched ||
      wrong_successor.status != prepare::PreparedFactBoundaryStatus::Mismatched ||
      wrong_destination.status != prepare::PreparedFactBoundaryStatus::Mismatched ||
      duplicate.status != prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return fail("current-block routing facts should be unique and edge-bound");
  }
  return 0;
}

int public_headers_have_only_inventoried_compatibility_payloads() {
  // Step 1 inventory: these are legacy public compatibility/proof payloads.
  // The guard makes additions fail while their owning producer seams migrate.
  const std::unordered_map<std::string, std::size_t> expected_hits{
      {"publication_plans.hpp", 6},
      {"value_locations.hpp", 6},
  };
  const std::regex forbidden(
      R"(\b(Route[0-9][A-Za-z0-9_]*(Record|Index)|route[0-9]_[A-Za-z0-9_]+)\b)");
  std::unordered_map<std::string, std::size_t> actual_hits;

  for (const auto& entry : std::filesystem::recursive_directory_iterator(
           C4C_PREALLOC_SOURCE_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".hpp") {
      continue;
    }
    std::ifstream input(entry.path());
    const std::string source((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
    actual_hits[entry.path().filename().string()] +=
        std::distance(std::sregex_iterator(source.begin(), source.end(), forbidden),
                      std::sregex_iterator());
  }
  for (const auto& [file, count] : actual_hits) {
    const auto expected = expected_hits.find(file);
    if (count != 0 && (expected == expected_hits.end() || expected->second != count)) {
      return fail("public prepared header gained an uninventoried route record or index");
    }
  }
  for (const auto& [file, count] : expected_hits) {
    if (actual_hits[file] != count) {
      return fail("prepared route compatibility inventory changed; update by migration");
    }
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = statuses_are_explicit_and_fail_closed(); status != 0) {
    return status;
  }
  if (const int status =
          current_block_named_producer_evidence_is_independent_and_unique();
      status != 0) {
    return status;
  }
  if (const int status = current_block_routing_facts_are_edge_bound_and_unique();
      status != 0) {
    return status;
  }
  return public_headers_have_only_inventoried_compatibility_payloads();
}
