#include "src/backend/mir/riscv/codegen/emit.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;
namespace riscv = c4c::backend::riscv::codegen;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

struct FixtureIds {
  c4c::FunctionNameId function = c4c::kInvalidFunctionName;
  c4c::BlockLabelId predecessor = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId successor = c4c::kInvalidBlockLabel;
  c4c::ValueNameId source_name = c4c::kInvalidValueName;
  c4c::ValueNameId destination_name = c4c::kInvalidValueName;
};

FixtureIds intern_fixture_ids(prepare::PreparedBirModule& prepared) {
  return FixtureIds{
      .function = prepared.names.function_names.intern("join_regs"),
      .predecessor = prepared.names.block_labels.intern("left"),
      .successor = prepared.names.block_labels.intern("join"),
      .source_name = prepared.names.value_names.intern("%src"),
      .destination_name = prepared.names.value_names.intern("%dst"),
  };
}

prepare::PreparedBirModule make_register_edge_publication_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");
  prepared.module.target_triple = "riscv64-unknown-linux-gnu";
  const auto ids = intern_fixture_ids(prepared);

  bir::Function function;
  function.name = "join_regs";
  prepared.module.functions.push_back(function);

  prepare::PreparedControlFlowFunction control_flow;
  control_flow.function_name = ids.function;
  control_flow.join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = ids.function,
      .join_block_label = ids.successor,
      .result = bir::Value::named(bir::TypeKind::I32, "%dst"),
      .edge_transfers = {prepare::PreparedEdgeValueTransfer{
          .predecessor_label = ids.predecessor,
          .successor_label = ids.successor,
          .incoming_value = bir::Value::named(bir::TypeKind::I32, "%src"),
          .destination_value = bir::Value::named(bir::TypeKind::I32, "%dst"),
      }},
  });
  prepared.control_flow.functions.push_back(control_flow);

  prepare::PreparedValueLocationFunction locations;
  locations.function_name = ids.function;
  locations.value_homes = {
      prepare::PreparedValueHome{
          .value_id = 1,
          .function_name = ids.function,
          .value_name = ids.source_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a0"},
      },
      prepare::PreparedValueHome{
          .value_id = 2,
          .function_name = ids.function,
          .value_name = ids.destination_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"a1"},
      },
  };
  locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = ids.function,
      .phase = prepare::PreparedMovePhase::BlockEntry,
      .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .source_parallel_copy_predecessor_label = ids.predecessor,
      .source_parallel_copy_successor_label = ids.successor,
      .moves = {prepare::PreparedMoveResolution{
          .from_value_id = 1,
          .to_value_id = 2,
          .destination_kind = prepare::PreparedMoveDestinationKind::Value,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_register_name = std::string{"a1"},
          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
          .source_parallel_copy_predecessor_label = ids.predecessor,
          .source_parallel_copy_successor_label = ids.successor,
      }},
  });
  prepared.value_locations.functions.push_back(locations);
  return prepared;
}

prepare::PreparedFunctionLookups make_lookups(
    const prepare::PreparedBirModule& prepared) {
  return prepare::make_prepared_function_lookups(
      prepared, prepared.control_flow.functions.front());
}

int check_register_to_register_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };

  const auto asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(asm_text.find("mv a1, a0") != std::string::npos,
              "RISC-V prepared module should emit a register edge move")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept the shared register edge publication") ||
      !expect(intent.instruction_text == "mv a1, a0",
              "RISC-V helper should render target-local mv syntax")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V helper should not rediscover edge moves after shared lookup authority is removed")) {
    return 1;
  }
  return 0;
}

int check_immediate_to_register_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };
  auto& source_home = prepared.value_locations.functions.front().value_homes.front();
  source_home.kind = prepare::PreparedValueHomeKind::RematerializableImmediate;
  source_home.register_name.reset();
  source_home.slot_id.reset();
  source_home.offset_bytes.reset();
  source_home.immediate_i32 = 42;

  const auto asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(asm_text.find("li a1, 42") != std::string::npos,
              "RISC-V prepared module should emit an immediate edge move")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, ids.predecessor, ids.successor, 2);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept the shared immediate edge publication") ||
      !expect(intent.publication == publication && publication != nullptr,
              "RISC-V immediate helper should preserve shared publication authority") ||
      !expect(publication->source_home_kind ==
                  prepare::PreparedValueHomeKind::RematerializableImmediate,
              "RISC-V immediate helper should preserve source home kind") ||
      !expect(intent.source_value_id == 1 && intent.destination_value_id == 2,
              "RISC-V immediate helper should preserve prepared value ids") ||
      !expect(intent.source_immediate_i32 == 42 && intent.destination_register == "a1",
              "RISC-V immediate helper should record immediate and destination register") ||
      !expect(intent.instruction_text == "li a1, 42",
              "RISC-V helper should render target-local li syntax")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V immediate helper should not rediscover edge moves after shared lookup authority is removed")) {
    return 1;
  }
  return 0;
}

int check_stack_slot_to_register_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };
  auto& source_home = prepared.value_locations.functions.front().value_homes.front();
  source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  source_home.register_name.reset();
  source_home.slot_id = prepare::PreparedFrameSlotId{7};
  source_home.offset_bytes = 16;
  source_home.size_bytes = 4;
  source_home.immediate_i32.reset();

  const auto asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(asm_text.find("lw a1, 16(sp)") != std::string::npos,
              "RISC-V prepared module should emit an I32 stack-source edge load")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, ids.predecessor, ids.successor, 2);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept the shared stack-source edge publication") ||
      !expect(intent.publication == publication && publication != nullptr,
              "RISC-V stack-source helper should preserve shared publication authority") ||
      !expect(publication->source_home_kind == prepare::PreparedValueHomeKind::StackSlot,
              "RISC-V stack-source helper should preserve source home kind") ||
      !expect(intent.source_value_id == 1 && intent.destination_value_id == 2,
              "RISC-V stack-source helper should preserve prepared value ids") ||
      !expect(intent.source_stack_slot_id == prepare::PreparedFrameSlotId{7} &&
                  intent.source_stack_offset_bytes == 16 &&
                  intent.source_stack_size_bytes == 4 &&
                  intent.destination_register == "a1",
              "RISC-V stack-source helper should record stack provenance and destination register") ||
      !expect(intent.instruction_text == "lw a1, 16(sp)",
              "RISC-V helper should render target-local lw syntax")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V stack-source helper should not rediscover edge moves after shared lookup authority is removed")) {
    return 1;
  }
  return 0;
}

int check_missing_and_unsupported_homes_fail_closed() {
  auto prepared = make_register_edge_publication_module();
  const auto predecessor = prepared.names.block_labels.find("left");
  const auto successor = prepared.names.block_labels.find("join");

  auto lookups = make_lookups(prepared);
  auto intent = riscv::consume_edge_publication_move_intent(
      nullptr, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingSharedLookups,
              "RISC-V helper should fail closed without shared lookups")) {
    return 1;
  }

  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 999);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V helper should fail closed without a shared publication fact")) {
    return 1;
  }

  prepared.value_locations.functions.front().value_homes.front().kind =
      prepare::PreparedValueHomeKind::StackSlot;
  prepared.value_locations.functions.front().value_homes.front().register_name.reset();
  prepared.value_locations.functions.front().value_homes.front().offset_bytes = 16;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-source helper should reject stack sources without an I32 size")) {
    return 1;
  }

  prepared = make_register_edge_publication_module();
  prepared.value_locations.functions.front().value_homes.front().kind =
      prepare::PreparedValueHomeKind::StackSlot;
  prepared.value_locations.functions.front().value_homes.front().register_name.reset();
  prepared.value_locations.functions.front().value_homes.front().size_bytes = 4;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-source helper should reject stack sources without an offset")) {
    return 1;
  }

  prepared = make_register_edge_publication_module();
  prepared.value_locations.functions.front().value_homes.front().kind =
      prepare::PreparedValueHomeKind::StackSlot;
  prepared.value_locations.functions.front().value_homes.front().register_name.reset();
  prepared.value_locations.functions.front().value_homes.front().offset_bytes = 16;
  prepared.value_locations.functions.front().value_homes.front().size_bytes = 8;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-source helper should reject non-I32 stack-source widths")) {
    return 1;
  }

  prepared = make_register_edge_publication_module();
  prepared.value_locations.functions.front().value_homes.front().kind =
      prepare::PreparedValueHomeKind::PointerBasePlusOffset;
  prepared.value_locations.functions.front().value_homes.front().register_name.reset();
  prepared.value_locations.functions.front().value_homes.front().pointer_base_value_name =
      prepared.names.value_names.find("%src");
  prepared.value_locations.functions.front().value_homes.front().pointer_byte_delta = 4;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V register-destination slice should keep pointer-base sources unsupported")) {
    return 1;
  }

  prepared = make_register_edge_publication_module();
  prepared.value_locations.functions.front().value_homes.front().kind =
      prepare::PreparedValueHomeKind::RematerializableImmediate;
  prepared.value_locations.functions.front().value_homes.front().register_name.reset();
  prepared.value_locations.functions.front().value_homes.front().immediate_i32.reset();
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V helper should reject immediate homes without a value")) {
    return 1;
  }

  prepared = make_register_edge_publication_module();
  prepared.value_locations.functions.front().value_homes.back().kind =
      prepare::PreparedValueHomeKind::StackSlot;
  prepared.value_locations.functions.front().value_homes.back().register_name.reset();
  prepared.value_locations.functions.front().value_homes.back().offset_bytes = 24;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedDestinationHome,
              "RISC-V first slice should keep stack destinations unsupported")) {
    return 1;
  }

  prepared = make_register_edge_publication_module();
  prepared.value_locations.functions.front().move_bundles.front().moves.front().op_kind =
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedPublication,
              "RISC-V helper should reject non-move edge publications")) {
    return 1;
  }

  return 0;
}

}  // namespace

int main() {
  if (const int result = check_register_to_register_move_uses_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_immediate_to_register_move_uses_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_stack_slot_to_register_move_uses_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_missing_and_unsupported_homes_fail_closed();
      result != 0) {
    return result;
  }
  return EXIT_SUCCESS;
}
