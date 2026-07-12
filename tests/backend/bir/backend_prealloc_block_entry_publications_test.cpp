#include "src/backend/prealloc/value_locations.hpp"
#include "src/backend/prealloc/control_flow.hpp"
#include "src/backend/prealloc/prepared_lookups.hpp"
#include "src/backend/mir/query.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

bool prepared_and_bir_available_block_entry_publication_identity_match(
    const prepare::PreparedCurrentBlockEntryPublication& prepared,
    const mir::BirBlockEntryPublicationIdentity& bir) {
  return prepared.status ==
             prepare::PreparedCurrentBlockEntryPublicationStatus::Available &&
         bir.available &&
         prepared.publication.destination_value_id ==
             prepared.destination_value_id &&
         bir.status ==
             prepare::PreparedCurrentBlockEntryPublicationStatus::Available &&
         bir.instruction_index == prepared.publication_bundle_instruction_index &&
         bir.destination_value_id == prepared.destination_value_id &&
         bir.destination_value_name_id == prepared.destination_value_name &&
         bir.destination_value_name == prepared.destination_value_name_text &&
         bir.destination_value_type == prepared.destination_value_type &&
         bir.successor_label == prepared.successor_label_text &&
         bir.successor_label_id == prepared.successor_label_id;
}

struct Fixture {
  c4c::FunctionNameId function_name = 1;
  c4c::BlockLabelId predecessor_label = 2;
  c4c::BlockLabelId successor_label = 3;
  c4c::BlockLabelId other_successor_label = 4;
  c4c::ValueNameId register_value_name = 11;
  c4c::ValueNameId fallback_register_value_name = 12;
  c4c::ValueNameId stack_value_name = 13;
  c4c::ValueNameId missing_register_value_name = 14;
  c4c::ValueNameId unpublished_register_value_name = 15;
  prepare::PreparedValueLocationFunction locations;
  prepare::PreparedRegallocFunction regalloc;
};

Fixture make_fixture() {
  Fixture fixture;
  fixture.locations.function_name = fixture.function_name;
  fixture.locations.value_homes = {
      prepare::PreparedValueHome{
          .value_id = 1,
          .function_name = fixture.function_name,
          .value_name = fixture.register_value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"r1"},
      },
      prepare::PreparedValueHome{
          .value_id = 2,
          .function_name = fixture.function_name,
          .value_name = fixture.fallback_register_value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"r2"},
      },
      prepare::PreparedValueHome{
          .value_id = 3,
          .function_name = fixture.function_name,
          .value_name = fixture.stack_value_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = 5,
          .offset_bytes = 40,
      },
      prepare::PreparedValueHome{
          .value_id = 4,
          .function_name = fixture.function_name,
          .value_name = fixture.missing_register_value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
      },
      prepare::PreparedValueHome{
          .value_id = 5,
          .function_name = fixture.function_name,
          .value_name = fixture.unpublished_register_value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"r7"},
      },
  };
  fixture.regalloc.function_name = fixture.function_name;
  for (const auto& home : fixture.locations.value_homes) {
    fixture.regalloc.values.push_back(prepare::PreparedRegallocValue{
        .value_id = home.value_id,
        .function_name = fixture.function_name,
        .value_name = home.value_name,
        .type = bir::TypeKind::I32,
    });
  }
  fixture.locations.move_bundles = {
      prepare::PreparedMoveBundle{
          .proof_attribution_id = 1,
          .function_name = fixture.function_name,
          .phase = prepare::PreparedMovePhase::BlockEntry,
          .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
          .block_index = 7,
          .source_parallel_copy_predecessor_label = fixture.predecessor_label,
          .source_parallel_copy_successor_label = fixture.successor_label,
          .moves = {
              prepare::PreparedMoveResolution{
                  .from_value_id = 20,
                  .to_value_id = 1,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"r9"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              },
              prepare::PreparedMoveResolution{
                  .from_value_id = 21,
                  .to_value_id = 2,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              },
              prepare::PreparedMoveResolution{
                  .from_value_id = 22,
                  .to_value_id = 3,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
                  .destination_stack_offset_bytes = std::size_t{40},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              },
              prepare::PreparedMoveResolution{
                  .from_value_id = 23,
                  .to_value_id = 4,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              },
              prepare::PreparedMoveResolution{
                  .from_value_id = 24,
                  .to_value_id = 99,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"r4"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              },
              prepare::PreparedMoveResolution{
                  .from_value_id = 25,
                  .to_value_id = 1,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"r5"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              },
              prepare::PreparedMoveResolution{
                  .from_value_id = 26,
                  .to_value_id = 1,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"r6"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              },
          },
      },
      prepare::PreparedMoveBundle{
          .function_name = fixture.function_name,
          .phase = prepare::PreparedMovePhase::BeforeInstruction,
          .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
          .source_parallel_copy_successor_label = fixture.successor_label,
          .moves = {prepare::PreparedMoveResolution{
              .to_value_id = 1,
              .destination_kind = prepare::PreparedMoveDestinationKind::Value,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_register_name = std::string{"wrong_phase"},
          }},
      },
      prepare::PreparedMoveBundle{
          .function_name = fixture.function_name,
          .phase = prepare::PreparedMovePhase::BlockEntry,
          .authority_kind = prepare::PreparedMoveAuthorityKind::None,
          .source_parallel_copy_successor_label = fixture.successor_label,
          .moves = {prepare::PreparedMoveResolution{
              .to_value_id = 1,
              .destination_kind = prepare::PreparedMoveDestinationKind::Value,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_register_name = std::string{"wrong_authority"},
          }},
      },
      prepare::PreparedMoveBundle{
          .function_name = fixture.function_name,
          .phase = prepare::PreparedMovePhase::BlockEntry,
          .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
          .source_parallel_copy_successor_label = fixture.other_successor_label,
          .moves = {prepare::PreparedMoveResolution{
              .to_value_id = 1,
              .destination_kind = prepare::PreparedMoveDestinationKind::Value,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_register_name = std::string{"wrong_label"},
          }},
      },
  };
  return fixture;
}

int check_collected_publication_facts() {
  const auto fixture = make_fixture();
  const auto publications = prepare::collect_prepared_block_entry_publications(
      &fixture.locations, fixture.successor_label);

  if (!expect(publications.size() == 7,
              "block-entry helper should collect only matching successor publications")) {
    return 1;
  }

  const auto& explicit_register = publications[0];
  if (!expect(prepare::prepared_block_entry_publication_available(explicit_register),
              "explicit register publication should be available") ||
      !expect(explicit_register.bundle == &fixture.locations.move_bundles.front() &&
                  explicit_register.move ==
                      &fixture.locations.move_bundles.front().moves.front(),
              "publication should preserve source Prepared bundle and move") ||
      !expect(explicit_register.home == &fixture.locations.value_homes.front(),
              "publication should preserve source Prepared value home") ||
      !expect(explicit_register.destination_value_id == 1 &&
                  explicit_register.destination_value_name == fixture.register_value_name,
              "publication should preserve destination value identity") ||
      !expect(explicit_register.destination_storage_kind ==
                  prepare::PreparedMoveStorageKind::Register,
              "publication should preserve destination storage kind") ||
      !expect(explicit_register.destination_register_name == std::optional<std::string>{"r9"},
              "publication should preserve move register name without target parsing")) {
    return 1;
  }

  const auto& home_register = publications[1];
  if (!expect(prepare::prepared_block_entry_publication_available(home_register),
              "home register fallback publication should be available") ||
      !expect(home_register.destination_register_name ==
                  std::optional<std::string>{"r2"},
              "publication should fall back to register value-home spelling")) {
    return 1;
  }

  const auto& stack_destination = publications[2];
  if (!expect(stack_destination.status ==
                  prepare::PreparedBlockEntryPublicationStatus::
                      UnsupportedDestinationStorage,
              "stack destination should be classified without target fallback") ||
      !expect(stack_destination.destination_storage_kind ==
                  prepare::PreparedMoveStorageKind::StackSlot,
              "stack destination should preserve storage kind") ||
      !expect(stack_destination.home == &fixture.locations.value_homes[2],
              "stack destination should preserve home authority")) {
    return 1;
  }

  const auto& missing_register = publications[3];
  if (!expect(missing_register.status ==
                  prepare::PreparedBlockEntryPublicationStatus::MissingRegisterName,
              "register publication without move or home register should report missing register") ||
      !expect(missing_register.home == &fixture.locations.value_homes[3],
              "missing-register publication should preserve home authority")) {
    return 1;
  }

  const auto& missing_home = publications[4];
  if (!expect(missing_home.status ==
                  prepare::PreparedBlockEntryPublicationStatus::MissingValueHome,
              "publication without value home should report missing home") ||
      !expect(missing_home.home == nullptr && missing_home.destination_value_id == 99,
              "missing-home publication should preserve destination value id")) {
    return 1;
  }

  const auto& unsupported_destination = publications[5];
  if (!expect(unsupported_destination.status ==
                  prepare::PreparedBlockEntryPublicationStatus::
                      UnsupportedDestinationKind,
              "non-value destination should report unsupported destination kind") ||
      !expect(unsupported_destination.destination_kind ==
                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              "non-value destination should preserve destination role")) {
    return 1;
  }

  const auto& unsupported_operation = publications[6];
  if (!expect(unsupported_operation.status ==
                  prepare::PreparedBlockEntryPublicationStatus::UnsupportedOperation,
              "non-move operation should report unsupported operation")) {
    return 1;
  }

  return 0;
}

int check_fallback_and_missing_inputs() {
  const auto fixture = make_fixture();

  if (!expect(prepare::collect_prepared_block_entry_publications(
                  &fixture.locations, fixture.other_successor_label)
                  .size() == 1,
              "wrong-label fixture should only match its own successor label")) {
    return 1;
  }

  if (!expect(prepare::collect_prepared_block_entry_publications(
                  &fixture.locations, c4c::BlockLabelId{99})
                  .empty(),
              "unmatched successor label should not fabricate publications") ||
      !expect(prepare::collect_prepared_block_entry_publications(
                  nullptr, fixture.successor_label)
                  .empty(),
              "missing value locations should not fabricate publications") ||
      !expect(prepare::collect_prepared_block_entry_publications(
                  &fixture.locations, c4c::kInvalidBlockLabel)
                  .empty(),
              "invalid successor label should not fabricate publications")) {
    return 1;
  }

  return 0;
}

int check_current_block_entry_publication_query() {
  auto fixture = make_fixture();
  const auto value_home_lookups =
      prepare::make_prepared_value_home_lookups(&fixture.locations);
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs query{
      .regalloc = &fixture.regalloc,
      .value_locations = &fixture.locations,
      .value_home_lookups = &value_home_lookups,
      .successor_label = fixture.successor_label,
  };

  const auto available = prepare::find_prepared_current_block_entry_publication(
      query, prepare::PreparedValueId{1});
  if (!expect(available.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::MissingNames,
              "unnamed current-block entry publication query should fail closed") ||
      !expect(available.destination_home == &fixture.locations.value_homes[0] &&
                  available.publication.home == &fixture.locations.value_homes[0],
              "current-block entry publication query should preserve value-home facts") ||
      !expect(available.publication.move ==
                  &fixture.locations.move_bundles[0].moves[0],
              "current-block entry publication query should preserve move fact") ||
      !expect(available.publication.destination_register_name ==
                  std::optional<std::string>{"r9"},
              "current-block entry publication query should not parse register spelling")) {
    return 1;
  }

  const auto unsupported_storage =
      prepare::find_prepared_current_block_entry_publication(
          query, prepare::PreparedValueId{3});
  if (!expect(unsupported_storage.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::
                      PublicationUnavailable,
              "unsupported storage should be reported as an unavailable publication") ||
      !expect(unsupported_storage.publication.status ==
                  prepare::PreparedBlockEntryPublicationStatus::
                      UnsupportedDestinationStorage,
              "unsupported storage should preserve collector classification") ||
      !expect(unsupported_storage.destination_home ==
                  &fixture.locations.value_homes[2],
              "unsupported storage should preserve destination home")) {
    return 1;
  }

  const auto missing_register =
      prepare::find_prepared_current_block_entry_publication(
          query, prepare::PreparedValueId{4});
  if (!expect(missing_register.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::
                      PublicationUnavailable,
              "missing register name should be reported as an unavailable publication") ||
      !expect(missing_register.publication.status ==
                  prepare::PreparedBlockEntryPublicationStatus::MissingRegisterName,
              "missing register name should preserve collector classification")) {
    return 1;
  }

  const auto missing_home = prepare::find_prepared_current_block_entry_publication(
      query, prepare::PreparedValueId{99});
  if (!expect(missing_home.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::
                      MissingDestinationHome,
              "publication query should require a prepared destination home identity")) {
    return 1;
  }

  const auto missing_publication =
      prepare::find_prepared_current_block_entry_publication(
          query, prepare::PreparedValueId{5});
  if (!expect(missing_publication.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::MissingPublication,
              "publication query should not fabricate current-block publications")) {
    return 1;
  }

  prepare::PreparedNameTables names;
  (void)names.block_labels.intern("unused");
  (void)names.block_labels.intern("predecessor");
  (void)names.block_labels.intern("successor");
  const auto published_value_name = names.value_names.intern("%published");
  const auto fallback_value_name = names.value_names.intern("%fallback");
  const auto stack_value_name = names.value_names.intern("%stack_destination");
  const auto bir_only_value_name = names.value_names.intern("%bir_only");
  fixture.locations.value_homes[0].value_name = published_value_name;
  fixture.locations.value_homes[1].value_name = fallback_value_name;
  fixture.locations.value_homes[2].value_name = stack_value_name;
  fixture.locations.value_homes[4].value_name = bir_only_value_name;
  fixture.regalloc.values[0].value_name = published_value_name;
  fixture.regalloc.values[1].value_name = fallback_value_name;
  fixture.regalloc.values[2].value_name = stack_value_name;
  fixture.regalloc.values[4].value_name = bir_only_value_name;
  const auto named_value_home_lookups =
      prepare::make_prepared_value_home_lookups(&fixture.locations);
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs named_query{
      .names = &names,
      .regalloc = &fixture.regalloc,
      .value_locations = &fixture.locations,
      .value_home_lookups = &named_value_home_lookups,
      .successor_label = fixture.successor_label,
  };
  const auto by_bir_value = prepare::find_prepared_current_block_entry_publication(
      named_query, bir::Value::named(bir::TypeKind::I32, "%published"));
  if (!expect(by_bir_value.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::MissingProof,
              "current-block entry publication query without proof should fail closed") ||
      !expect(by_bir_value.destination_value_id == prepare::PreparedValueId{1} &&
                  by_bir_value.destination_value_name == published_value_name,
              "BIR value query should preserve resolved prepared value identity") ||
      !expect(!by_bir_value.block_entry_publication_proof_attributed,
              "current-block entry publication should not claim block-entry publication proof without block-entry publication proof input")) {
    return 1;
  }

  bir::Block successor;
  successor.label = "successor";
  successor.label_id = fixture.successor_label;
  successor.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%published"),
      .incomings = {
          bir::PhiIncoming{
              .label = "predecessor",
              .value = bir::Value::named(bir::TypeKind::I32, "%source"),
              .label_id = fixture.predecessor_label,
          },
      },
  });
  successor.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%stack_destination"),
      .incomings = {
          bir::PhiIncoming{
              .label = "predecessor",
              .value = bir::Value::named(bir::TypeKind::I32, "%stack_source"),
              .label_id = fixture.predecessor_label,
          },
      },
  });
  successor.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%bir_only"),
      .incomings = {
          bir::PhiIncoming{
              .label = "predecessor",
              .value = bir::Value::named(bir::TypeKind::I32, "%bir_source"),
              .label_id = fixture.predecessor_label,
          },
      },
  });

  const auto& published_phi =
      std::get<bir::PhiInst>(successor.insts.front()).result;
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs block_entry_proof_query{
      .names = &names,
      .regalloc = &fixture.regalloc,
      .value_locations = &fixture.locations,
      .value_home_lookups = &named_value_home_lookups,
      .successor_label = fixture.successor_label,
      .block_entry_publication_proof_successor_block = &successor,
      .block_entry_publication_proof_destination_value = &published_phi,
  };
  const auto block_entry_proof_attributed =
      prepare::find_prepared_current_block_entry_publication(
          block_entry_proof_query, bir::Value::named(bir::TypeKind::I32, "%published"));
  if (!expect(block_entry_proof_attributed.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::Available,
              "block-entry publication proof agreement should preserve prepared availability") ||
      !expect(block_entry_proof_attributed.block_entry_publication_proof_attributed,
              "agreeing block-entry publication proof should attribute the available block-entry publication") ||
      !expect(block_entry_proof_attributed.block_entry_publication_proof_status ==
                  bir::BirViewStatus::Available,
              "agreeing block-entry publication proof should validate through the named publication view") ||
      !expect(block_entry_proof_attributed.block_entry_publication_proof_instruction_index ==
                  std::size_t{0},
              "block-entry publication proof should preserve the PHI instruction index") ||
      !expect(block_entry_proof_attributed.successor_label_text == "successor" &&
                  block_entry_proof_attributed.successor_label_id ==
                      fixture.successor_label &&
                  block_entry_proof_attributed.destination_value_name_text ==
                      "%published" &&
                  block_entry_proof_attributed.destination_value_name ==
                      published_value_name &&
                  block_entry_proof_attributed.destination_value_id ==
                      prepare::PreparedValueId{1} &&
                  block_entry_proof_attributed.destination_value_type ==
                      bir::TypeKind::I32 &&
                  block_entry_proof_attributed.publication_bundle_instruction_index ==
                      std::size_t{0},
              "attributed block-entry publication should own the complete stable semantic payload") ||
      !expect(block_entry_proof_attributed.publication.destination_register_name ==
                  std::optional<std::string>{"r9"},
              "block-entry publication proof should not rewrite prepared register spelling")) {
    return 1;
  }

  const auto bir_available = mir::find_bir_block_entry_publication_identity(
      block_entry_proof_attributed);
  if (!expect(prepared_and_bir_available_block_entry_publication_identity_match(
                  block_entry_proof_attributed, bir_available),
              "BIR block-entry publication identity should match prepared semantic destination fields for available PHI publication") ||
      !expect(bir_available.successor_label == successor.label &&
                  bir_available.successor_label_id == fixture.successor_label,
              "BIR block-entry publication identity should preserve successor identity")) {
    return 1;
  }

  const auto prepared_home_register_fallback =
      prepare::find_prepared_current_block_entry_publication(
          named_query, prepare::PreparedValueId{2});
  bir::Block no_phi_successor;
  no_phi_successor.label = "successor";
  no_phi_successor.label_id = fixture.successor_label;
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs missing_block_entry_proof_query{
      .names = &names,
      .regalloc = &fixture.regalloc,
      .value_locations = &fixture.locations,
      .value_home_lookups = &named_value_home_lookups,
      .successor_label = fixture.successor_label,
      .block_entry_publication_proof_successor_block = &no_phi_successor,
      .block_entry_publication_proof_destination_value = &published_phi,
  };
  const auto missing_block_entry_proof =
      prepare::find_prepared_current_block_entry_publication(
          missing_block_entry_proof_query, bir::Value::named(bir::TypeKind::I32, "%published"));
  if (!expect(missing_block_entry_proof.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::ProofUnavailable,
              "missing block-entry publication evidence should fail closed") ||
      !expect(!missing_block_entry_proof.block_entry_publication_proof_attributed,
              "missing block-entry publication proof should fall back without attribution")) {
    return 1;
  }

  const auto bir_missing_fallback_phi =
      mir::find_bir_block_entry_publication_identity(missing_block_entry_proof);
  if (!expect(prepared_home_register_fallback.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::MissingProof,
              "prepared home-register lookup without publication proof should fail closed") ||
      !expect(!bir_missing_fallback_phi &&
                  bir_missing_fallback_phi.status ==
                      prepare::PreparedCurrentBlockEntryPublicationStatus::ProofUnavailable,
              "prepared home/register readiness should not imply BIR PHI-entry identity")) {
    return 1;
  }

  const auto& stack_phi =
      std::get<bir::PhiInst>(successor.insts[1]).result;
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs mismatched_block_entry_proof_query{
      .names = &names,
      .regalloc = &fixture.regalloc,
      .value_locations = &fixture.locations,
      .value_home_lookups = &named_value_home_lookups,
      .successor_label = fixture.successor_label,
      .block_entry_publication_proof_successor_block = &successor,
      .block_entry_publication_proof_destination_value = &stack_phi,
  };
  const auto mismatched_block_entry_proof =
      prepare::find_prepared_current_block_entry_publication(
          mismatched_block_entry_proof_query, bir::Value::named(bir::TypeKind::I32, "%published"));
  if (!expect(mismatched_block_entry_proof.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::ProofMismatch,
              "mismatched block-entry publication proof should fail closed") ||
      !expect(!mismatched_block_entry_proof.block_entry_publication_proof_attributed,
              "mismatched block-entry publication proof should fall back without attribution") ||
      !expect(mismatched_block_entry_proof.block_entry_publication_proof_status ==
                  bir::BirViewStatus::Available,
              "mismatched block-entry publication proof should still record the available named fact")) {
    return 1;
  }

  auto wrong_successor_proof_block = successor;
  wrong_successor_proof_block.label_id = fixture.other_successor_label;
  const auto& wrong_successor_phi =
      std::get<bir::PhiInst>(wrong_successor_proof_block.insts.front()).result;
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs wrong_successor_block_entry_proof_query{
      .names = &names,
      .regalloc = &fixture.regalloc,
      .value_locations = &fixture.locations,
      .value_home_lookups = &named_value_home_lookups,
      .successor_label = fixture.successor_label,
      .block_entry_publication_proof_successor_block = &wrong_successor_proof_block,
      .block_entry_publication_proof_destination_value = &wrong_successor_phi,
  };
  const auto wrong_successor_block_entry_proof =
      prepare::find_prepared_current_block_entry_publication(
          wrong_successor_block_entry_proof_query,
          bir::Value::named(bir::TypeKind::I32, "%published"));
  if (!expect(wrong_successor_block_entry_proof.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::ProofMismatch,
              "wrong-successor block-entry publication proof should fail closed") ||
      !expect(!wrong_successor_block_entry_proof
                   .block_entry_publication_proof_attributed,
              "wrong-successor block-entry publication proof should fall back without attribution") ||
      !expect(wrong_successor_block_entry_proof
                      .block_entry_publication_proof_status ==
                  bir::BirViewStatus::Available,
              "wrong-successor block-entry publication proof should remain available before prepared agreement rejects it")) {
    return 1;
  }

  const auto wrong_type_proof_value =
      bir::Value::named(bir::TypeKind::I64, "%published");
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs wrong_type_block_entry_proof_query{
      .names = &names,
      .regalloc = &fixture.regalloc,
      .value_locations = &fixture.locations,
      .value_home_lookups = &named_value_home_lookups,
      .successor_label = fixture.successor_label,
      .block_entry_publication_proof_successor_block = &successor,
      .block_entry_publication_proof_destination_value = &wrong_type_proof_value,
  };
  const auto wrong_type_block_entry_proof =
      prepare::find_prepared_current_block_entry_publication(
          wrong_type_block_entry_proof_query,
          bir::Value::named(bir::TypeKind::I32, "%published"));
  if (!expect(wrong_type_block_entry_proof.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::ProofUnavailable,
              "wrong-key block-entry publication proof should fail closed") ||
      !expect(!wrong_type_block_entry_proof
                   .block_entry_publication_proof_attributed,
              "wrong-key block-entry publication proof should fall back without attribution") ||
      !expect(wrong_type_block_entry_proof
                      .block_entry_publication_proof_status ==
                  bir::BirViewStatus::Unavailable,
              "wrong-key block-entry publication proof should expose unavailable named evidence")) {
    return 1;
  }

  auto duplicate_proof_successor = successor;
  duplicate_proof_successor.insts.insert(duplicate_proof_successor.insts.begin(),
                                          duplicate_proof_successor.insts.front());
  const auto& duplicate_proof_phi =
      std::get<bir::PhiInst>(duplicate_proof_successor.insts.front()).result;
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs duplicate_block_entry_proof_query{
      .names = &names,
      .regalloc = &fixture.regalloc,
      .value_locations = &fixture.locations,
      .value_home_lookups = &named_value_home_lookups,
      .successor_label = fixture.successor_label,
      .block_entry_publication_proof_successor_block = &duplicate_proof_successor,
      .block_entry_publication_proof_destination_value = &duplicate_proof_phi,
  };
  const auto duplicate_block_entry_proof =
      prepare::find_prepared_current_block_entry_publication(
          duplicate_block_entry_proof_query,
          bir::Value::named(bir::TypeKind::I32, "%published"));
  if (!expect(duplicate_block_entry_proof.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::ProofAmbiguous,
              "duplicate block-entry publication proof should fail closed") ||
      !expect(!duplicate_block_entry_proof.block_entry_publication_proof_attributed,
              "duplicate block-entry publication proof should fall back without attribution") ||
      !expect(duplicate_block_entry_proof.block_entry_publication_proof_status ==
                  bir::BirViewStatus::Ambiguous,
              "duplicate block-entry publication proof should expose ambiguous named evidence")) {
    return 1;
  }

  const auto prepared_stack_destination =
      prepare::find_prepared_current_block_entry_publication(
          named_query, prepare::PreparedValueId{3});
  const auto bir_stack_destination =
      mir::find_bir_block_entry_publication_identity(prepared_stack_destination);
  if (!expect(prepared_stack_destination.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::
                      PublicationUnavailable,
              "prepared stack-destination publication should remain prepared-owned readiness") ||
      !expect(!bir_stack_destination,
              "common identity should fail closed for unavailable prepared publication")) {
    return 1;
  }

  const auto prepared_bir_only =
      prepare::find_prepared_current_block_entry_publication(
          named_query, prepare::PreparedValueId{5});
  const auto bir_only = mir::find_bir_block_entry_publication_identity(
      prepared_bir_only);
  if (!expect(prepared_bir_only.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::
                      MissingPublication,
              "prepared missing-publication should remain a prepared emission-readiness negative") ||
      !expect(!bir_only,
              "common identity should not reconstruct a missing prepared publication")) {
    return 1;
  }

  const auto unpublished_prepared =
      prepare::find_prepared_current_block_entry_publication(
          query, prepare::PreparedValueId{5});
  const auto bir_missing_publication =
      mir::find_bir_block_entry_publication_identity(unpublished_prepared);
  if (!expect(unpublished_prepared.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::
                      MissingPublication &&
                  !bir_missing_publication &&
                  bir_missing_publication.status ==
                      prepare::PreparedCurrentBlockEntryPublicationStatus::MissingPublication &&
                  bir_missing_publication.destination_value_id ==
                      unpublished_prepared.destination_value_id &&
                  bir_missing_publication.destination_value_name_id ==
                      unpublished_prepared.destination_value_name,
              "BIR block-entry publication identity should fail closed with prepared missing-publication category")) {
    return 1;
  }

  const auto wrong_successor_prepared =
      prepare::find_prepared_current_block_entry_publication(
          prepare::PreparedCurrentBlockEntryPublicationQueryInputs{
              .value_locations = &fixture.locations,
              .value_home_lookups = &value_home_lookups,
              .successor_label = c4c::BlockLabelId{99},
          },
          prepare::PreparedValueId{1});
  const auto bir_wrong_successor =
      mir::find_bir_block_entry_publication_identity(wrong_successor_prepared);
  if (!expect(wrong_successor_prepared.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::
                      MissingPublication &&
                  !bir_wrong_successor,
              "BIR block-entry publication identity should fail closed for wrong successor") ||
      !expect(bir_wrong_successor.status ==
                  prepare::PreparedCurrentBlockEntryPublicationStatus::MissingPublication,
              "BIR wrong-successor block-entry query should report missing successor identity")) {
    return 1;
  }

  const auto bir_wrong_value = mir::find_bir_block_entry_publication_identity(
      mismatched_block_entry_proof);
  if (!expect(!bir_wrong_value &&
                  bir_wrong_value.status ==
                      prepare::PreparedCurrentBlockEntryPublicationStatus::ProofMismatch,
              "BIR block-entry publication identity should fail closed for wrong destination value")) {
    return 1;
  }

  return 0;
}

int check_edge_publication_lookup_reuses_block_entry_publication_data() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("edge_publication");
  const auto predecessor_label = names.block_labels.intern("edge_publication.pred");
  const auto successor_label = names.block_labels.intern("edge_publication.succ");
  const auto source_name = names.value_names.intern("%edge.source");
  const auto destination_name = names.value_names.intern("%edge.destination");

  prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .join_transfers = {
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = successor_label,
              .result =
                  bir::Value::named(bir::TypeKind::I32, "%edge.destination"),
              .kind = prepare::PreparedJoinTransferKind::PhiEdge,
              .edge_transfers = {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge.source"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge.destination"),
                  },
              },
          },
      },
      .parallel_copy_bundles = {
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = predecessor_label,
              .successor_label = successor_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::SuccessorEntry,
              .execution_block_label = successor_label,
              .steps = {
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 0,
                  },
              },
          },
      },
  };
  prepare::PreparedValueLocationFunction locations{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 31,
              .function_name = function_name,
              .value_name = source_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"source_home"},
          },
          prepare::PreparedValueHome{
              .value_id = 32,
              .function_name = function_name,
              .value_name = destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"destination_home"},
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .proof_attribution_id = 1,
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 1,
              .source_parallel_copy_predecessor_label = predecessor_label,
              .source_parallel_copy_successor_label = successor_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .from_value_id = 31,
                      .to_value_id = 32,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_register_name = std::string{"publication_home"},
                      .source_parallel_copy_step_index = std::size_t{0},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
      },
  };

  const auto lookups = prepare::make_prepared_edge_publication_lookups(
      names, control_flow, &locations);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, successor_label, prepare::PreparedValueId{32});

  if (!expect(publication != nullptr,
              "edge-publication lookup should index block-entry destination")) {
    return 1;
  }
  if (!expect(publication->move == &locations.move_bundles[0].moves[0] &&
                  publication->move_bundle == &locations.move_bundles[0],
              "edge-publication lookup should link existing block-entry move publication") ||
      !expect(publication->parallel_copy_bundle == &control_flow.parallel_copy_bundles[0],
              "edge-publication lookup should link published parallel-copy ownership") ||
      !expect(publication->source_value_id == prepare::PreparedValueId{31} &&
                  publication->destination_value_id == prepare::PreparedValueId{32},
              "edge-publication lookup should preserve source and destination value ids") ||
      !expect(publication->source_value_kind == bir::Value::Kind::Named &&
                  publication->source_home == &locations.value_homes[0] &&
                  publication->source_home_kind ==
                      prepare::PreparedValueHomeKind::Register,
              "edge-publication lookup should preserve source classification and home") ||
      !expect(publication->parallel_copy_step_index == std::size_t{0} &&
                  publication->parallel_copy_step_kind ==
                      prepare::PreparedParallelCopyStepKind::Move &&
                  !publication->parallel_copy_step_uses_cycle_temp_source &&
                  publication->parallel_copy_execution_site ==
                      prepare::PreparedParallelCopyExecutionSite::SuccessorEntry &&
                  publication->parallel_copy_execution_block_label ==
                      std::optional<c4c::BlockLabelId>{successor_label},
              "edge-publication lookup should preserve parallel-copy execution metadata")) {
    return 1;
  }

  return 0;
}

}  // namespace

int main() {
  if (const auto status = check_collected_publication_facts(); status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = check_fallback_and_missing_inputs(); status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = check_current_block_entry_publication_query();
      status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status =
          check_edge_publication_lookup_reuses_block_entry_publication_data();
      status != 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
