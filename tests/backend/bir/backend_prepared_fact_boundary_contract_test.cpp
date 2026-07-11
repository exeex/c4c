#include "src/backend/prealloc/publication_plans.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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
  auto conflicting_origin = fact;
  conflicting_origin.predecessor_label = c4c::BlockLabelId{3};
  conflicting_origin.publication_semantic_origin =
      prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
          PublicationSemanticOrigin::BirPhi;
  const auto consume = [&](const auto& candidates) {
    return prepare::query_prepared_current_block_join_routing_consumption(
        candidates, fact.successor_label, fact.routed_value_id,
        fact.routed_value_name, fact.role);
  };
  if (consume(std::vector<prepare::PreparedCurrentBlockJoinRoutingFact>{}).status !=
          prepare::PreparedFactBoundaryStatus::Missing ||
      consume(std::vector<prepare::PreparedCurrentBlockJoinRoutingFact>{
                  fact, conflicting_origin})
              .status !=
          prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return fail("result consumption should preserve missing and reject non-invariant answers");
  }
  return 0;
}

prepare::PreparedCurrentBlockJoinRoutingFact routing_fact() {
  return {
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
}

prepare::PreparedCurrentBlockJoinRoutingFact select_routing_fact(
    const std::vector<prepare::PreparedCurrentBlockJoinRoutingFact>& facts,
    const prepare::PreparedCurrentBlockJoinRoutingFact& key) {
  return prepare::select_prepared_current_block_join_routing_fact(
      facts, key.predecessor_label, key.successor_label,
      key.destination_value_id, key.destination_value_name, key.source_value_id,
      key.source_value_name, key.routed_value_id, key.routed_value_name,
      key.role);
}

prepare::PreparedCurrentBlockJoinRoutingConsumption consume_routing_facts(
    const std::vector<prepare::PreparedCurrentBlockJoinRoutingFact>& facts,
    const prepare::PreparedCurrentBlockJoinRoutingFact& key) {
  return prepare::query_prepared_current_block_join_routing_consumption(
      facts, key.successor_label, key.routed_value_id, key.routed_value_name,
      key.role);
}

int parallel_predecessors_remain_independently_available() {
  const auto first = routing_fact();
  auto second = first;
  second.predecessor_label = c4c::BlockLabelId{3};
  const std::vector<prepare::PreparedCurrentBlockJoinRoutingFact> facts{first,
                                                                       second};
  const auto consumed = consume_routing_facts(facts, first);
  if (!consumed || consumed.edge_fact_count != 2) {
    return fail("parallel predecessors must have one invariant result answer");
  }
  return 0;
}

int conflicting_parallel_destinations_are_explicitly_ambiguous() {
  const auto first = routing_fact();
  auto second = first;
  second.destination_value_id = prepare::PreparedValueId{11};
  second.destination_value_name = c4c::ValueNameId{11};
  const std::vector<prepare::PreparedCurrentBlockJoinRoutingFact> facts{first,
                                                                       second};
  const auto consumed = consume_routing_facts(facts, first);
  if (consumed.status != prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return fail("conflicting parallel destinations must fail closed as ambiguous");
  }
  return 0;
}

int wrong_successor_is_explicitly_mismatched() {
  const auto fact = routing_fact();
  auto wrong_successor = fact;
  wrong_successor.successor_label = c4c::BlockLabelId{4};
  if (consume_routing_facts({fact}, wrong_successor).status !=
      prepare::PreparedFactBoundaryStatus::Mismatched) {
    return fail("wrong successor must be explicitly mismatched");
  }
  return 0;
}

int duplicate_semantic_edge_is_explicitly_ambiguous() {
  const auto fact = routing_fact();
  if (consume_routing_facts({fact, fact}, fact).status !=
      prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return fail("duplicate semantic edge must be explicitly ambiguous");
  }
  return 0;
}

int result_level_key_loses_predecessor_before_destination_identity() {
  const prepare::PreparedCurrentBlockJoinRoutingFact first{
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = c4c::BlockLabelId{1},
      .successor_label = c4c::BlockLabelId{9},
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
  auto second = first;
  second.predecessor_label = c4c::BlockLabelId{2};
  second.destination_value_id = prepare::PreparedValueId{11};
  second.destination_value_name = c4c::ValueNameId{11};
  const std::vector<prepare::PreparedCurrentBlockJoinRoutingFact> facts{
      first, second};

  const auto select = [&](const auto& fact) {
    return prepare::select_prepared_current_block_join_routing_fact(
        facts,
        fact.predecessor_label,
        fact.successor_label,
        fact.destination_value_id,
        fact.destination_value_name,
        fact.source_value_id,
        fact.source_value_name,
        fact.routed_value_id,
        fact.routed_value_name,
        fact.role);
  };
  if (!select(first) || !select(second)) {
    return fail("baseline requires two independently valid edge-bound routing facts");
  }

  const auto same_result_level_key = [&](const auto& lhs, const auto& rhs) {
    return lhs.routed_value_id == rhs.routed_value_id &&
           lhs.routed_value_name == rhs.routed_value_name &&
           lhs.role == rhs.role && lhs.successor_label == rhs.successor_label;
  };
  if (!same_result_level_key(first, second) ||
      first.predecessor_label == second.predecessor_label ||
      first.destination_value_id == second.destination_value_id) {
    return fail("baseline fixture must share one result key across distinct edges");
  }

  const auto result_level_matches = std::count_if(
      facts.begin(), facts.end(), [&](const auto& fact) {
        return fact.status == prepare::PreparedFactBoundaryStatus::Available &&
               fact.routed_value_id == first.routed_value_id &&
               fact.routed_value_name == first.routed_value_name &&
               fact.role == first.role &&
               fact.successor_label == first.successor_label;
      });
  if (result_level_matches != 2) {
    return fail("first lost identity is predecessor_label; destination identity is also unavailable");
  }
  return 0;
}

int public_headers_have_only_inventoried_compatibility_payloads() {
  // Migrated prepared records must not expose route-numbered records, indexes,
  // status aliases, or query inputs.
  const std::unordered_map<std::string, std::size_t> expected_hits{};
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

int main(int argc, char** argv) {
  if (argc == 2) {
    const std::string_view probe = argv[1];
    if (probe == "parallel-predecessors") {
      return parallel_predecessors_remain_independently_available();
    }
    if (probe == "parallel-destinations") {
      return conflicting_parallel_destinations_are_explicitly_ambiguous();
    }
    if (probe == "wrong-successor") {
      return wrong_successor_is_explicitly_mismatched();
    }
    if (probe == "duplicate-semantic-edge") {
      return duplicate_semantic_edge_is_explicitly_ambiguous();
    }
    return fail("unknown prepared fact-boundary contract probe");
  }
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
  if (const int status =
          result_level_key_loses_predecessor_before_destination_identity();
      status != 0) {
    return status;
  }
  if (const int status = parallel_predecessors_remain_independently_available();
      status != 0) {
    return status;
  }
  if (const int status =
          conflicting_parallel_destinations_are_explicitly_ambiguous();
      status != 0) {
    return status;
  }
  if (const int status = wrong_successor_is_explicitly_mismatched();
      status != 0) {
    return status;
  }
  if (const int status = duplicate_semantic_edge_is_explicitly_ambiguous();
      status != 0) {
    return status;
  }
  return public_headers_have_only_inventoried_compatibility_payloads();
}
