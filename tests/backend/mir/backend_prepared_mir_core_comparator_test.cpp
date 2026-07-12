#include "src/backend/mir/prepared_view.hpp"
#include "src/backend/mir/query.hpp"
#include "src/backend/prealloc/module.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;
namespace prepared = c4c::backend::mir::prepared;
namespace mir = c4c::backend::mir;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

bir::Value named(bir::TypeKind type, const char* name) {
  return bir::Value::named(type, name);
}

prepare::PreparedBirModule make_fixture() {
  prepare::PreparedBirModule module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);
  module.module.target_triple = "x86_64-test-c4c";

  const auto function_name = module.names.function_names.intern("main");
  const auto entry_label = module.names.block_labels.intern("entry");
  const auto value_name = module.names.value_names.intern("%v");
  const auto global_name = module.module.names.link_names.intern("global_i32");
  const auto block_label_id = module.module.names.block_labels.intern("entry");

  module.module.globals.push_back(bir::Global{
      .name = "global_i32",
      .link_name_id = global_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "abc",
  });

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = named(bir::TypeKind::I32, "%v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.terminator = bir::ReturnTerminator{.value = named(bir::TypeKind::I32, "%v")};

  bir::Function main;
  main.name = "main";
  main.return_type = bir::TypeKind::I32;
  main.blocks.push_back(std::move(entry));
  module.module.functions.push_back(std::move(main));

  bir::Function declaration;
  declaration.name = "extern_fn";
  declaration.return_type = bir::TypeKind::I32;
  declaration.is_declaration = true;
  module.module.functions.push_back(std::move(declaration));

  bir::Function missing_facts;
  missing_facts.name = "missing_facts";
  missing_facts.return_type = bir::TypeKind::I32;
  missing_facts.blocks.push_back(bir::Block{.label = "entry"});
  module.module.functions.push_back(std::move(missing_facts));

  module.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  module.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{1},
          .function_name = function_name,
          .value_name = value_name,
          .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
          .immediate_i32 = 3,
      }},
  });
  module.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
  });

  module.completed_phases.push_back("phase-sentinel-not-core");
  module.notes.push_back(prepare::PrepareNote{
      .phase = "diagnostic-phase-sentinel",
      .message = "prepare-note-sentinel-not-core",
  });
  return module;
}

prepare::PreparedBirModule make_current_block_join_fixture(bool include_join_transfer) {
  prepare::PreparedBirModule module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);
  module.module.target_triple = "x86_64-test-c4c";

  const auto function_name = module.names.function_names.intern("current_join_query");
  const auto predecessor_label = module.names.block_labels.intern("current_join.pred");
  const auto successor_label = module.names.block_labels.intern("current_join.succ");
  const auto incoming_name = module.names.value_names.intern("%current.incoming");
  const auto destination_name = module.names.value_names.intern("%current.destination");

  const prepare::PreparedValueId incoming_id{101};
  const prepare::PreparedValueId destination_id{103};

  bir::Block block;
  block.label = "current_join.succ";
  block.label_id = successor_label;
  block.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%current.destination"),
      .incomings = {
          bir::PhiIncoming{
              .label = "current_join.pred",
              .value =
                  bir::Value::named(bir::TypeKind::I32, "%current.incoming"),
              .label_id = predecessor_label,
          },
      },
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%current.incoming"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(4),
      .rhs = bir::Value::immediate_i32(5),
  });

  bir::Function function;
  function.name = "current_join_query";
  function.return_type = bir::TypeKind::I32;
  function.blocks.push_back(std::move(block));
  module.module.functions.push_back(std::move(function));

  prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = successor_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
      .parallel_copy_bundles = {
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = predecessor_label,
              .successor_label = successor_label,
              .steps = {
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 0,
                  },
              },
          },
      },
  };
  if (include_join_transfer) {
    control_flow.join_transfers.push_back(prepare::PreparedJoinTransfer{
        .function_name = function_name,
        .join_block_label = successor_label,
        .kind = prepare::PreparedJoinTransferKind::PhiEdge,
        .edge_transfers = {
            prepare::PreparedEdgeValueTransfer{
                .predecessor_label = predecessor_label,
                .successor_label = successor_label,
                .incoming_value =
                    bir::Value::named(bir::TypeKind::I32, "%current.incoming"),
                .destination_value = bir::Value::named(
                    bir::TypeKind::I32, "%current.destination"),
            },
        },
    });
  }
  module.control_flow.functions.push_back(std::move(control_flow));

  module.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = incoming_id,
              .function_name = function_name,
              .value_name = incoming_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"r10d"},
          },
          prepare::PreparedValueHome{
              .value_id = destination_id,
              .function_name = function_name,
              .value_name = destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"r12d"},
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind =
                  prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 0,
              .source_parallel_copy_predecessor_label = predecessor_label,
              .source_parallel_copy_successor_label = successor_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .from_value_id = incoming_id,
                      .to_value_id = destination_id,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_register_name = std::string{"r12d"},
                      .source_parallel_copy_step_index = std::size_t{0},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
      },
  });
  module.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
  });
  return module;
}

bool contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

bool has_blocking_difference(const prepared::PreparedMirCoreComparisonReport& report,
                             std::string_view expected) {
  for (const auto& difference : report.blocking_differences) {
    if (difference == expected) {
      return true;
    }
  }
  return false;
}

int verify_core_view_canonical_dump() {
  const auto module = make_fixture();
  const prepared::PreparedMirCoreView view(module);
  const std::string dump = view.canonical_dump();
  const std::string repeat_dump = view.canonical_dump();

  if (dump != repeat_dump) {
    return fail("expected canonical core view dump to be deterministic");
  }
  if (!contains(dump, "prepared_mir_core_view schema=1\n") ||
      !contains(dump, "target triple=x86_64-test-c4c") ||
      !contains(dump, "arch=x86_64") ||
      !contains(dump, "module functions=3 defined_functions=2 globals=1 strings=1\n")) {
    return fail("expected canonical dump to include target and module core summary");
  }
  if (!contains(dump, "function index=0 name=main") ||
      !contains(dump, "declaration=0 blocks=1 view=1") ||
      !contains(dump, "function index=1 name=extern_fn") ||
      !contains(dump, "declaration=1 blocks=0 view=0") ||
      !contains(dump, "function index=2 name=missing_facts") ||
      !contains(dump, "declaration=0 blocks=1 view=0")) {
    return fail("expected canonical dump to include all/defined traversal and view admission");
  }
  if (!contains(dump, "global index=0 name=global_i32") ||
      !contains(dump, "string index=0 name=.str0 bytes=3 bytes_hex=616263 align=1") ||
      !contains(dump, "function_view name=main") ||
      !contains(dump, "control_flow function=main blocks=1") ||
      !contains(dump, "value_locations function=main homes=1 move_bundles=0") ||
      !contains(dump, "value_home function=main value_id=1 value_name=%v") ||
      !contains(dump, "addressing function=main accesses=0 materializations=0") ||
      !contains(dump, "lookups function=main") ||
      !contains(dump, "value_homes=1") ||
      !contains(dump, "block function=main index=0 label=entry") ||
      !contains(dump, "cursor function=main block_index=0 instruction_index=0 label=entry bound=1")) {
    return fail("expected canonical dump to include view-exposed core function facts");
  }
  if (contains(dump, "prepare-note-sentinel-not-core") ||
      contains(dump, "diagnostic-phase-sentinel") ||
      contains(dump, "phase-sentinel-not-core") ||
      contains(dump, "semantic_bir_shared")) {
    return fail("expected canonical dump to exclude diagnostic and prepared-history fields");
  }

  return 0;
}

int verify_core_view_structural_comparison() {
  const auto lhs_module = make_fixture();
  const auto rhs_module = make_fixture();
  const prepared::PreparedMirCoreView lhs_view(lhs_module);
  const prepared::PreparedMirCoreView rhs_view(rhs_module);

  const auto equal_report = prepared::compare_prepared_mir_core_views(lhs_view, rhs_view);
  if (!equal_report.equal() ||
      equal_report.status != prepared::PreparedMirCoreComparisonStatus::Equal ||
      !equal_report.blocking_differences.empty()) {
    return fail("expected identical core view snapshots to compare equal");
  }

  auto diagnostic_only_module = make_fixture();
  diagnostic_only_module.completed_phases.push_back("another-phase-not-core");
  diagnostic_only_module.notes.push_back(prepare::PrepareNote{
      .phase = "another-diagnostic-phase",
      .message = "another-prepare-note-not-core",
  });
  const prepared::PreparedMirCoreView diagnostic_only_view(diagnostic_only_module);
  const auto diagnostic_report =
      prepared::compare_prepared_mir_core_views(lhs_view, diagnostic_only_view);
  if (!diagnostic_report.equal()) {
    return fail("expected diagnostic and prepared-history changes to be excluded");
  }

  auto mutated_module = make_fixture();
  mutated_module.module.string_constants.front().bytes[1] = 'z';
  const prepared::PreparedMirCoreView mutated_view(mutated_module);
  const auto difference_report =
      prepared::compare_prepared_mir_core_views(lhs_view, mutated_view);
  if (difference_report.equal() ||
      difference_report.status != prepared::PreparedMirCoreComparisonStatus::Different ||
      !has_blocking_difference(difference_report, "string_constants")) {
    return fail("expected typed string-constant core fact mutation to block equality");
  }

  return 0;
}

int verify_direct_edge_publication_source_freshness_view() {
  const auto accepted_module = make_current_block_join_fixture(true);
  const prepared::PreparedMirCoreView accepted_core(accepted_module);
  const auto accepted_function = accepted_core.function_view("current_join_query");
  if (!accepted_function.has_value()) {
    return fail("expected current-block join fixture to produce a function view");
  }
  const auto accepted_query =
      accepted_function->current_block_direct_edge_publication_sources(0);
  if (accepted_query.status !=
          prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::Available ||
      accepted_query.sources.size() != 1 ||
      accepted_query.sources.front().status !=
          prepared::PreparedMirDirectEdgePublicationSourceStatus::Available ||
      accepted_query.sources.front().source_freshness_status !=
          prepare::PreparedValueFreshnessQueryStatus::Selected ||
      accepted_query.sources.front().source_freshness_candidate_count != 1 ||
      accepted_query.sources.front().freshness_use_kind !=
          prepare::PreparedValueFreshnessUseKind::DirectEdgePublicationSource ||
      accepted_query.sources.front().freshness_source_kind !=
          prepare::PreparedValueFreshnessSourceKind::DirectEdgePublication ||
      accepted_query.sources.front().freshness_proof_kind !=
          prepare::PreparedValueFreshnessProofKind::DirectEdgePublicationMove ||
      accepted_query.sources.front().freshness_rank !=
          prepare::PreparedValueFreshnessSourceRank::DirectEdgePublication ||
      accepted_query.sources.front().source_value_id !=
          std::optional<prepare::PreparedValueId>{prepare::PreparedValueId{101}} ||
      accepted_query.sources.front().source_value_name == c4c::kInvalidValueName ||
      accepted_query.sources.front().destination_value_id !=
          prepare::PreparedValueId{103} ||
      accepted_query.sources.front().destination_register_name !=
          std::optional<std::string>{"r12d"} ||
      accepted_query.sources.front().source_register_name !=
          std::optional<std::string>{"r10d"}) {
    return fail("expected selected direct-edge source freshness to become a typed view result");
  }

  const auto rejected_module = make_current_block_join_fixture(false);
  const prepared::PreparedMirCoreView rejected_core(rejected_module);
  const auto rejected_function = rejected_core.function_view("current_join_query");
  if (!rejected_function.has_value()) {
    return fail("expected rejected fixture to keep the core function view available");
  }
  const auto rejected_query =
      rejected_function->current_block_direct_edge_publication_sources(0);
  const auto route_discovery = mir::find_bir_current_block_join_source_identity(
      rejected_core.prepared_names(), rejected_query);
  if (route_discovery.status !=
      mir::BirCurrentBlockJoinSourceStatus::MissingPublication) {
    return fail("expected negative fixture to fail closed without Route 5 discovery");
  }
  if (rejected_query.status !=
          prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::Available ||
      rejected_query.sources.size() != 1 ||
      rejected_query.sources.front().status !=
          prepared::PreparedMirDirectEdgePublicationSourceStatus::MissingPublication ||
      rejected_query.sources.front().source_value_id.has_value() ||
      rejected_query.sources.front().destination_value_id != prepare::PreparedValueId{0} ||
      rejected_query.sources.front().destination_register_name.has_value() ||
      rejected_query.sources.front().source_freshness_candidate_count != 0 ||
      rejected_query.sources.front().freshness_use_kind !=
          prepare::PreparedValueFreshnessUseKind::Unknown) {
    return fail("expected prepared direct-edge query to reject missing authority without BIR route fallback");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = verify_core_view_canonical_dump(); status != 0) {
    return status;
  }
  if (const int status = verify_core_view_structural_comparison(); status != 0) {
    return status;
  }
  return verify_direct_edge_publication_source_freshness_view();
}
