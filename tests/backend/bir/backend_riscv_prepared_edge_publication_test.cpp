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

bool expect_unsupported_stack_destination(
    const riscv::EdgePublicationMoveIntent& intent,
    std::string_view message) {
  return expect(intent.status ==
                    riscv::EdgePublicationMoveIntentStatus::UnsupportedDestinationHome,
                message) &&
         expect(intent.instruction_text.empty(),
                "RISC-V stack-destination helper should not render an instruction "
                "for unsupported stack-destination source materialization");
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
  c4c::ValueNameId base_name = c4c::kInvalidValueName;
  c4c::ValueNameId destination_name = c4c::kInvalidValueName;
};

FixtureIds intern_fixture_ids(prepare::PreparedBirModule& prepared) {
  return FixtureIds{
      .function = prepared.names.function_names.intern("join_regs"),
      .predecessor = prepared.names.block_labels.intern("left"),
      .successor = prepared.names.block_labels.intern("join"),
      .source_name = prepared.names.value_names.intern("%src"),
      .base_name = prepared.names.value_names.intern("%base"),
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
      prepare::PreparedValueHome{
          .value_id = 3,
          .function_name = ids.function,
          .value_name = ids.base_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"s2"},
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

void set_edge_publication_value_types(prepare::PreparedBirModule& prepared,
                                      bir::TypeKind source_type,
                                      bir::TypeKind destination_type) {
  auto& transfer =
      prepared.control_flow.functions.front().join_transfers.front().edge_transfers.front();
  transfer.incoming_value.type = source_type;
  transfer.destination_value.type = destination_type;
  prepared.control_flow.functions.front().join_transfers.front().result.type =
      destination_type;
}

int check_register_to_register_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
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
      .base_name = prepared.names.value_names.find("%base"),
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
      .base_name = prepared.names.value_names.find("%base"),
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

int check_stack_slot_i64_to_register_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };
  auto& source_home = prepared.value_locations.functions.front().value_homes.front();
  source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  source_home.register_name.reset();
  source_home.slot_id = prepare::PreparedFrameSlotId{17};
  source_home.offset_bytes = 32;
  source_home.size_bytes = 8;
  source_home.immediate_i32.reset();

  const auto asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(asm_text.find("ld a1, 32(sp)") != std::string::npos,
              "RISC-V prepared module should emit an I64 stack-source edge load")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, ids.predecessor, ids.successor, 2);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept the shared I64 stack-source edge publication") ||
      !expect(intent.publication == publication && publication != nullptr,
              "RISC-V I64 stack-source helper should preserve shared publication authority") ||
      !expect(publication->source_home_kind == prepare::PreparedValueHomeKind::StackSlot,
              "RISC-V I64 stack-source helper should preserve source home kind") ||
      !expect(intent.source_value_id == 1 && intent.destination_value_id == 2,
              "RISC-V I64 stack-source helper should preserve prepared value ids") ||
      !expect(intent.source_stack_slot_id == prepare::PreparedFrameSlotId{17} &&
                  intent.source_stack_offset_bytes == 32 &&
                  intent.source_stack_size_bytes == 8 &&
                  intent.destination_register == "a1",
              "RISC-V I64 stack-source helper should record stack provenance and destination register") ||
      !expect(intent.instruction_text == "ld a1, 32(sp)",
              "RISC-V helper should render target-local ld syntax")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V I64 stack-source helper should not rediscover edge moves after shared lookup authority is removed")) {
    return 1;
  }
  return 0;
}

int check_large_offset_stack_slot_to_register_loads_use_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };

  auto set_stack_source = [&prepared](std::size_t offset,
                                      std::size_t size,
                                      prepare::PreparedFrameSlotId slot_id) {
    auto& source_home = prepared.value_locations.functions.front().value_homes.front();
    source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
    source_home.register_name.reset();
    source_home.slot_id = slot_id;
    source_home.offset_bytes = offset;
    source_home.size_bytes = size;
    source_home.immediate_i32.reset();
    source_home.pointer_base_value_name.reset();
    source_home.pointer_byte_delta.reset();
  };

  set_stack_source(4096, 4, prepare::PreparedFrameSlotId{21});
  const auto i32_asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(i32_asm_text.find("li t6, 4096\n    add t6, sp, t6\n    lw a1, 0(t6)") !=
                  std::string::npos,
              "RISC-V prepared module should materialize a large I32 stack-source address")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, ids.predecessor, ids.successor, 2);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept a large-offset I32 stack-source publication") ||
      !expect(intent.publication == publication && publication != nullptr,
              "RISC-V large-offset I32 helper should preserve shared publication authority") ||
      !expect(intent.source_stack_slot_id == prepare::PreparedFrameSlotId{21} &&
                  intent.source_stack_offset_bytes == 4096 &&
                  intent.source_stack_size_bytes == 4 &&
                  intent.destination_register == "a1",
              "RISC-V large-offset I32 helper should record stack provenance") ||
      !expect(intent.instruction_text ==
                  "li t6, 4096\n    add t6, sp, t6\n    lw a1, 0(t6)",
              "RISC-V helper should render target-local large-offset I32 load syntax")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V large-offset I32 helper should not rediscover edge moves after shared lookup authority is removed")) {
    return 1;
  }

  set_stack_source(8192, 8, prepare::PreparedFrameSlotId{22});
  const auto i64_asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(i64_asm_text.find("li t6, 8192\n    add t6, sp, t6\n    ld a1, 0(t6)") !=
                  std::string::npos,
              "RISC-V prepared module should materialize a large I64 stack-source address")) {
    return 1;
  }

  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept a large-offset I64 stack-source publication") ||
      !expect(intent.source_stack_slot_id == prepare::PreparedFrameSlotId{22} &&
                  intent.source_stack_offset_bytes == 8192 &&
                  intent.source_stack_size_bytes == 8 &&
                  intent.destination_register == "a1",
              "RISC-V large-offset I64 helper should record stack provenance") ||
      !expect(intent.instruction_text ==
                  "li t6, 8192\n    add t6, sp, t6\n    ld a1, 0(t6)",
              "RISC-V helper should render target-local large-offset I64 load syntax")) {
    return 1;
  }

  return 0;
}

int check_stack_source_fail_closed_forms() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };

  auto set_stack_source = [&prepared](std::optional<std::size_t> offset,
                                      std::optional<std::size_t> size) {
    auto& source_home = prepared.value_locations.functions.front().value_homes.front();
    source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
    source_home.register_name.reset();
    source_home.slot_id = prepare::PreparedFrameSlotId{17};
    source_home.offset_bytes = offset;
    source_home.size_bytes = size;
    source_home.immediate_i32.reset();
    source_home.pointer_base_value_name.reset();
    source_home.pointer_byte_delta.reset();
  };

  set_stack_source(std::nullopt, 8);
  auto lookups = make_lookups(prepared);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-source helper should reject I64 stack sources without an offset")) {
    return 1;
  }
  if (!expect(intent.instruction_text.empty() &&
                  !intent.source_stack_offset_bytes.has_value(),
              "RISC-V dynamic stack-source helper should not render a load "
              "without concrete stack-offset authority")) {
    return 1;
  }

  auto& dynamic_source_home =
      prepared.value_locations.functions.front().value_homes.front();
  dynamic_source_home.pointer_base_value_name = ids.base_name;
  dynamic_source_home.pointer_byte_delta = 12;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V dynamic StackSlot source should stay fail-closed even "
              "when pointer-base fields are present without a concrete offset") ||
      !expect(intent.instruction_text.empty() &&
                  !intent.source_pointer_byte_delta.has_value(),
              "RISC-V dynamic StackSlot source should not be reclassified as "
              "pointer-base materialization")) {
    return 1;
  }

  set_stack_source(32, 2);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-source helper should reject unsupported subword widths")) {
    return 1;
  }
  if (!expect(intent.instruction_text.empty(),
              "RISC-V subword stack-source helper should not emit lb/lbu/lh/lhu "
              "without prepared signedness authority")) {
    return 1;
  }

  set_edge_publication_value_types(prepared, bir::TypeKind::I16, bir::TypeKind::I16);
  set_stack_source(32, 2);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V typed subword stack-source helper should stay fail-closed "
              "without signedness authority") ||
      !expect(intent.instruction_text.find("lh") == std::string::npos &&
                  intent.instruction_text.find("lhu") == std::string::npos,
              "RISC-V typed subword stack-source helper should not choose signed "
              "or unsigned halfword opcodes from width alone")) {
    return 1;
  }

  set_edge_publication_value_types(prepared, bir::TypeKind::I32, bir::TypeKind::I64);
  set_stack_source(32, 4);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V unsigned-I32-shaped stack-source helper should stay "
              "fail-closed without zero-extension authority") ||
      !expect(intent.instruction_text.find("lwu") == std::string::npos,
              "RISC-V unsigned-I32-shaped stack-source helper should not emit "
              "lwu from size/type shape alone")) {
    return 1;
  }

  set_edge_publication_value_types(prepared, bir::TypeKind::F32, bir::TypeKind::F32);
  prepared.value_locations.functions.front().value_homes.at(1).register_name =
      std::string{"fa0"};
  set_stack_source(32, 4);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V F32 stack-source helper should stay fail-closed without "
              "prepared FPR destination authority") ||
      !expect(intent.instruction_text.find("flw") == std::string::npos &&
                  intent.instruction_text.find("lw fa0") == std::string::npos,
              "RISC-V F32 stack-source helper should not choose floating or "
              "integer load opcodes from raw register spelling")) {
    return 1;
  }

  set_edge_publication_value_types(prepared, bir::TypeKind::I32, bir::TypeKind::I32);
  prepared.value_locations.functions.front().value_homes.at(1).register_name =
      std::string{"a1"};

  set_stack_source(32, 16);
  const auto aggregate_asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(aggregate_asm_text.find("lw a1, 32(sp)") == std::string::npos &&
                  aggregate_asm_text.find("ld a1, 32(sp)") == std::string::npos,
              "RISC-V aggregate-width stack-source publication should not emit "
              "a scalar signed-12 stack load")) {
    return 1;
  }
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-source helper should reject aggregate-width stack sources") ||
      !expect(intent.instruction_text.empty() &&
                  !intent.source_stack_slot_id.has_value() &&
                  !intent.source_stack_offset_bytes.has_value() &&
                  !intent.source_stack_size_bytes.has_value(),
              "RISC-V aggregate-width stack-source helper should not record "
              "scalar stack-load provenance")) {
    return 1;
  }

  set_stack_source(4096, 16);
  const auto large_aggregate_asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(large_aggregate_asm_text.find("0(t6)") == std::string::npos &&
                  large_aggregate_asm_text.find("li t6, 4096") == std::string::npos,
              "RISC-V aggregate-width stack-source publication should not emit "
              "the scalar large-offset stack-load sequence")) {
    return 1;
  }
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V large-offset aggregate-width stack source should stay "
              "fail-closed without prepared aggregate copy authority") ||
      !expect(intent.instruction_text.empty() &&
                  !intent.source_stack_slot_id.has_value() &&
                  !intent.source_stack_offset_bytes.has_value() &&
                  !intent.source_stack_size_bytes.has_value(),
              "RISC-V large-offset aggregate-width stack-source helper should "
              "not record scalar stack-load provenance")) {
    return 1;
  }

  set_stack_source(2048, 2);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-source helper should keep large-offset subword sources unsupported")) {
    return 1;
  }

  set_stack_source(32, 8);
  prepared.value_locations.functions.front().move_bundles.front().moves.front().op_kind =
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedPublication,
              "RISC-V I64 stack-source helper should reject non-move edge publications")) {
    return 1;
  }

  return 0;
}

int check_pointer_base_to_register_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };
  auto& source_home = prepared.value_locations.functions.front().value_homes.front();
  source_home.kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset;
  source_home.register_name.reset();
  source_home.slot_id.reset();
  source_home.offset_bytes.reset();
  source_home.size_bytes.reset();
  source_home.immediate_i32.reset();
  source_home.pointer_base_value_name = ids.base_name;
  source_home.pointer_byte_delta = 12;

  const auto asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(asm_text.find("addi a1, s2, 12") != std::string::npos,
              "RISC-V prepared module should emit a pointer-base edge materialization")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, ids.predecessor, ids.successor, 2);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept the shared pointer-base edge publication") ||
      !expect(intent.publication == publication && publication != nullptr,
              "RISC-V pointer-base helper should preserve shared publication authority") ||
      !expect(publication->source_home_kind ==
                  prepare::PreparedValueHomeKind::PointerBasePlusOffset,
              "RISC-V pointer-base helper should preserve source home kind") ||
      !expect(intent.source_value_id == 1 && intent.destination_value_id == 2,
              "RISC-V pointer-base helper should preserve prepared value ids") ||
      !expect(intent.source_pointer_base_value_id == 3 &&
                  intent.source_pointer_base_register == "s2" &&
                  intent.source_pointer_byte_delta == 12 &&
                  intent.destination_register == "a1",
              "RISC-V pointer-base helper should record base register, delta, and destination register") ||
      !expect(intent.instruction_text == "addi a1, s2, 12",
              "RISC-V helper should render target-local addi syntax")) {
    return 1;
  }
  if (!expect(!intent.source_stack_offset_bytes.has_value() &&
                  intent.instruction_text.find("lw ") == std::string::npos &&
                  intent.instruction_text.find("ld ") == std::string::npos,
              "RISC-V pointer-base source should remain address-value "
              "materialization, not a dynamic stack-source load")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V pointer-base helper should not rediscover edge moves after shared lookup authority is removed")) {
    return 1;
  }

  source_home.pointer_byte_delta = 0;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept a zero pointer-base delta") ||
      !expect(intent.instruction_text == "mv a1, s2",
              "RISC-V helper should render zero pointer-base delta as a semantic move")) {
    return 1;
  }

  source_home.pointer_byte_delta = 4096;
  const auto large_delta_asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(large_delta_asm_text.find("li a1, 4096\n    add a1, s2, a1") !=
                  std::string::npos,
              "RISC-V prepared module should emit a large-delta pointer-base materialization")) {
    return 1;
  }
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept a large pointer-base delta with a distinct destination register") ||
      !expect(intent.source_pointer_base_register == "s2" &&
                  intent.source_pointer_byte_delta == 4096 &&
                  intent.destination_register == "a1",
              "RISC-V helper should record large-delta pointer-base fields") ||
      !expect(intent.instruction_text == "li a1, 4096\n    add a1, s2, a1",
              "RISC-V helper should render target-local large-delta materialization")) {
    return 1;
  }
  if (!expect(intent.instruction_text.find("0(t6)") == std::string::npos &&
                  intent.instruction_text.find("lw ") == std::string::npos &&
                  intent.instruction_text.find("ld ") == std::string::npos,
              "RISC-V large pointer-base materialization should not reuse the "
              "large-offset stack-source load sequence")) {
    return 1;
  }
  return 0;
}

int check_register_to_stack_slot_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };
  auto& destination_home = prepared.value_locations.functions.front().value_homes.at(1);
  destination_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  destination_home.register_name.reset();
  destination_home.slot_id = prepare::PreparedFrameSlotId{11};
  destination_home.offset_bytes = 24;
  destination_home.size_bytes = 4;

  const auto asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(asm_text.find("sw a0, 24(sp)") != std::string::npos,
              "RISC-V prepared module should emit an I32 stack-destination edge store")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, ids.predecessor, ids.successor, 2);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept the shared stack-destination edge publication") ||
      !expect(intent.publication == publication && publication != nullptr,
              "RISC-V stack-destination helper should preserve shared publication authority") ||
      !expect(publication->destination_home_kind == prepare::PreparedValueHomeKind::StackSlot,
              "RISC-V stack-destination helper should preserve destination home kind") ||
      !expect(intent.source_value_id == 1 && intent.destination_value_id == 2,
              "RISC-V stack-destination helper should preserve prepared value ids") ||
      !expect(intent.source_register == "a0" &&
                  intent.destination_stack_slot_id == prepare::PreparedFrameSlotId{11} &&
                  intent.destination_stack_offset_bytes == 24 &&
                  intent.destination_stack_size_bytes == 4,
              "RISC-V stack-destination helper should record source register and stack destination") ||
      !expect(intent.instruction_text == "sw a0, 24(sp)",
              "RISC-V helper should render target-local sw syntax")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V stack-destination helper should not rediscover edge moves after shared lookup authority is removed")) {
    return 1;
  }
  return 0;
}

int check_immediate_to_stack_slot_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };
  auto& source_home = prepared.value_locations.functions.front().value_homes.front();
  source_home.kind = prepare::PreparedValueHomeKind::RematerializableImmediate;
  source_home.register_name.reset();
  source_home.slot_id.reset();
  source_home.offset_bytes.reset();
  source_home.immediate_i32 = -17;

  auto& destination_home = prepared.value_locations.functions.front().value_homes.at(1);
  destination_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  destination_home.register_name.reset();
  destination_home.slot_id = prepare::PreparedFrameSlotId{11};
  destination_home.offset_bytes = 24;
  destination_home.size_bytes = 4;

  const auto asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(asm_text.find("li t0, -17\n    sw t0, 24(sp)") != std::string::npos,
              "RISC-V prepared module should emit an I32 immediate stack-destination edge store")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, ids.predecessor, ids.successor, 2);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept the shared immediate stack-destination edge publication") ||
      !expect(intent.publication == publication && publication != nullptr,
              "RISC-V immediate stack-destination helper should preserve shared publication authority") ||
      !expect(publication->source_home_kind ==
                  prepare::PreparedValueHomeKind::RematerializableImmediate,
              "RISC-V immediate stack-destination helper should preserve source home kind") ||
      !expect(publication->destination_home_kind == prepare::PreparedValueHomeKind::StackSlot,
              "RISC-V immediate stack-destination helper should preserve destination home kind") ||
      !expect(intent.source_value_id == 1 && intent.destination_value_id == 2,
              "RISC-V immediate stack-destination helper should preserve prepared value ids") ||
      !expect(intent.source_immediate_i32 == -17 &&
                  intent.destination_stack_slot_id == prepare::PreparedFrameSlotId{11} &&
                  intent.destination_stack_offset_bytes == 24 &&
                  intent.destination_stack_size_bytes == 4,
              "RISC-V immediate stack-destination helper should record immediate and stack destination") ||
      !expect(intent.instruction_text == "li t0, -17\n    sw t0, 24(sp)",
              "RISC-V helper should render t0 materialization followed by target-local sw syntax")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V immediate stack-destination helper should not rediscover edge moves after shared lookup authority is removed")) {
    return 1;
  }
  return 0;
}

int check_stack_slot_to_stack_slot_i32_move_uses_shared_lookup() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };
  auto& source_home = prepared.value_locations.functions.front().value_homes.front();
  source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  source_home.register_name.reset();
  source_home.slot_id = prepare::PreparedFrameSlotId{7};
  source_home.offset_bytes = 16;
  source_home.size_bytes = 4;
  source_home.immediate_i32.reset();

  auto& destination_home = prepared.value_locations.functions.front().value_homes.at(1);
  destination_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  destination_home.register_name.reset();
  destination_home.slot_id = prepare::PreparedFrameSlotId{11};
  destination_home.offset_bytes = 24;
  destination_home.size_bytes = 4;

  const auto asm_text = riscv::emit_prepared_module(prepared);
  if (!expect(asm_text.find("lw t0, 16(sp)\n    sw t0, 24(sp)") !=
                  std::string::npos,
              "RISC-V prepared module should emit an I32 stack-source load "
              "through t0 before the stack-destination store")) {
    return 1;
  }

  auto lookups = make_lookups(prepared);
  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, ids.predecessor, ids.successor, 2);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V helper should accept the shared I32 stack-to-stack edge publication") ||
      !expect(intent.publication == publication && publication != nullptr,
              "RISC-V stack-to-stack helper should preserve shared publication authority") ||
      !expect(publication->source_home_kind == prepare::PreparedValueHomeKind::StackSlot,
              "RISC-V stack-to-stack helper should preserve source home kind") ||
      !expect(publication->destination_home_kind == prepare::PreparedValueHomeKind::StackSlot,
              "RISC-V stack-to-stack helper should preserve destination home kind") ||
      !expect(intent.source_value_id == 1 && intent.destination_value_id == 2,
              "RISC-V stack-to-stack helper should preserve prepared value ids") ||
      !expect(intent.source_stack_slot_id == prepare::PreparedFrameSlotId{7} &&
                  intent.source_stack_offset_bytes == 16 &&
                  intent.source_stack_size_bytes == 4 &&
                  intent.destination_stack_slot_id == prepare::PreparedFrameSlotId{11} &&
                  intent.destination_stack_offset_bytes == 24 &&
                  intent.destination_stack_size_bytes == 4,
              "RISC-V stack-to-stack helper should record source and destination stack provenance") ||
      !expect(intent.instruction_text == "lw t0, 16(sp)\n    sw t0, 24(sp)",
              "RISC-V helper should render t0 load followed by target-local sw syntax")) {
    return 1;
  }

  lookups.edge_publications.publications_by_edge_destination.clear();
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::MissingPublication,
              "RISC-V stack-to-stack helper should not rediscover edge moves "
              "after shared lookup authority is removed")) {
    return 1;
  }
  return 0;
}

int check_stack_destination_fail_closed_forms() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };

  auto set_stack_destination = [&prepared](std::optional<std::size_t> offset,
                                           std::optional<std::size_t> size) {
    auto& destination_home = prepared.value_locations.functions.front().value_homes.at(1);
    destination_home.kind = prepare::PreparedValueHomeKind::StackSlot;
    destination_home.register_name.reset();
    destination_home.slot_id = prepare::PreparedFrameSlotId{11};
    destination_home.offset_bytes = offset;
    destination_home.size_bytes = size;
  };

  set_stack_destination(std::nullopt, 4);
  auto lookups = make_lookups(prepared);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedDestinationHome,
              "RISC-V stack-destination helper should reject destinations without an offset")) {
    return 1;
  }

  set_stack_destination(24, 8);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedDestinationHome,
              "RISC-V stack-destination helper should reject non-I32 stack destinations")) {
    return 1;
  }

  set_stack_destination(24, 4);
  auto& source_home = prepared.value_locations.functions.front().value_homes.front();
  source_home.kind = prepare::PreparedValueHomeKind::RematerializableImmediate;
  source_home.register_name.reset();
  source_home.immediate_i32 = 42;
  set_stack_destination(2048, 4);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect_unsupported_stack_destination(
          intent,
          "RISC-V immediate stack-destination helper should reject large offsets "
          "without address expansion support")) {
    return 1;
  }

  set_stack_destination(24, 4);
  source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  source_home.immediate_i32.reset();
  source_home.slot_id = prepare::PreparedFrameSlotId{7};
  source_home.offset_bytes = 32;
  source_home.size_bytes = 8;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect_unsupported_stack_destination(
          intent,
          "RISC-V stack-destination helper should reject non-I32 stack sources")) {
    return 1;
  }

  source_home.size_bytes = 4;
  source_home.offset_bytes = std::nullopt;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-destination helper should reject stack sources without offsets")) {
    return 1;
  }

  source_home.offset_bytes = 32;
  source_home.size_bytes = std::nullopt;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-destination helper should reject stack sources without sizes")) {
    return 1;
  }

  source_home.size_bytes = 2;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-destination helper should reject subword stack sources")) {
    return 1;
  }

  source_home.size_bytes = 16;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-destination helper should reject aggregate stack sources")) {
    return 1;
  }

  source_home.size_bytes = 4;
  source_home.offset_bytes = 2048;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect_unsupported_stack_destination(
          intent,
          "RISC-V stack-destination helper should reject large stack-source offsets")) {
    return 1;
  }

  source_home.offset_bytes = 24;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect_unsupported_stack_destination(
          intent,
          "RISC-V stack-destination helper should reject overlapping stack slots")) {
    return 1;
  }

  source_home.offset_bytes = 32;
  source_home.slot_id = prepare::PreparedFrameSlotId{11};
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect_unsupported_stack_destination(
          intent,
          "RISC-V stack-destination helper should reject same-slot stack moves")) {
    return 1;
  }

  source_home.kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset;
  source_home.slot_id.reset();
  source_home.offset_bytes.reset();
  source_home.size_bytes.reset();
  source_home.pointer_base_value_name = ids.base_name;
  source_home.pointer_byte_delta = 12;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect_unsupported_stack_destination(
          intent,
          "RISC-V stack-destination helper should keep pointer-base sources "
          "unsupported until scratch-backed source materialization is implemented")) {
    return 1;
  }

  prepared = make_register_edge_publication_module();
  set_stack_destination(24, 4);
  prepared.value_locations.functions.front().move_bundles.front().moves.front().op_kind =
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedPublication,
              "RISC-V stack-destination helper should reject non-move edge publications")) {
    return 1;
  }

  return 0;
}

int check_pointer_base_fail_closed_forms() {
  auto prepared = make_register_edge_publication_module();
  const auto ids = FixtureIds{
      .function = prepared.names.function_names.find("join_regs"),
      .predecessor = prepared.names.block_labels.find("left"),
      .successor = prepared.names.block_labels.find("join"),
      .source_name = prepared.names.value_names.find("%src"),
      .base_name = prepared.names.value_names.find("%base"),
      .destination_name = prepared.names.value_names.find("%dst"),
  };

  auto set_pointer_source = [&prepared](std::optional<c4c::ValueNameId> base_name,
                                        std::optional<std::int64_t> delta) {
    auto& source_home = prepared.value_locations.functions.front().value_homes.front();
    source_home.kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset;
    source_home.register_name.reset();
    source_home.slot_id.reset();
    source_home.offset_bytes.reset();
    source_home.size_bytes.reset();
    source_home.immediate_i32.reset();
    source_home.pointer_base_value_name = base_name;
    source_home.pointer_byte_delta = delta;
  };

  set_pointer_source(std::nullopt, 4);
  auto lookups = make_lookups(prepared);
  auto intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V pointer-base helper should reject homes without a base value name")) {
    return 1;
  }

  set_pointer_source(ids.base_name, std::nullopt);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V pointer-base helper should reject homes without a byte delta")) {
    return 1;
  }

  set_pointer_source(prepared.names.value_names.intern("%missing_base"), 4);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V pointer-base helper should reject unresolved base value names")) {
    return 1;
  }

  set_pointer_source(ids.base_name, 2048);
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::Available,
              "RISC-V pointer-base helper should accept positive large deltas with a distinct destination register")) {
    return 1;
  }

  set_pointer_source(ids.base_name, -2049);
  auto& destination_home = prepared.value_locations.functions.front().value_homes.at(1);
  destination_home.register_name = std::string{"s2"};
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedDestinationHome,
              "RISC-V pointer-base helper should reject large deltas when destination aliases the base register") ||
      !expect(intent.instruction_text.empty(),
              "RISC-V pointer-base helper should not render an aliasing large-delta materialization")) {
    return 1;
  }
  destination_home.register_name = std::string{"a1"};

  set_pointer_source(ids.base_name, 4);
  auto& base_home = prepared.value_locations.functions.front().value_homes.back();
  base_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  base_home.register_name.reset();
  base_home.offset_bytes = 32;
  base_home.size_bytes = 4;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V pointer-base helper should reject non-register base homes")) {
    return 1;
  }

  prepared = make_register_edge_publication_module();
  set_pointer_source(ids.base_name, 4);
  prepared.value_locations.functions.front().value_homes.back().kind =
      prepare::PreparedValueHomeKind::Register;
  prepared.value_locations.functions.front().value_homes.back().register_name = std::string{"s2"};
  prepared.value_locations.functions.front().value_homes.at(1).kind =
      prepare::PreparedValueHomeKind::StackSlot;
  prepared.value_locations.functions.front().value_homes.at(1).register_name.reset();
  prepared.value_locations.functions.front().value_homes.at(1).offset_bytes = 24;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, ids.predecessor, ids.successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedDestinationHome,
              "RISC-V pointer-base helper should keep stack destinations unsupported")) {
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
  prepared.value_locations.functions.front().value_homes.front().size_bytes = 16;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedSourceHome,
              "RISC-V stack-source helper should reject unsupported stack-source widths")) {
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
              "RISC-V pointer-base helper should reject self-referential non-register bases")) {
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
  prepared.value_locations.functions.front().value_homes.at(1).kind =
      prepare::PreparedValueHomeKind::StackSlot;
  prepared.value_locations.functions.front().value_homes.at(1).register_name.reset();
  prepared.value_locations.functions.front().value_homes.at(1).offset_bytes = 24;
  lookups = make_lookups(prepared);
  intent = riscv::consume_edge_publication_move_intent(
      &lookups, predecessor, successor, 2);
  if (!expect(intent.status == riscv::EdgePublicationMoveIntentStatus::UnsupportedDestinationHome,
              "RISC-V stack-destination helper should reject malformed stack destinations")) {
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
  if (const int result = check_stack_slot_i64_to_register_move_uses_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_large_offset_stack_slot_to_register_loads_use_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_stack_source_fail_closed_forms();
      result != 0) {
    return result;
  }
  if (const int result = check_pointer_base_to_register_move_uses_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_register_to_stack_slot_move_uses_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_immediate_to_stack_slot_move_uses_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_stack_slot_to_stack_slot_i32_move_uses_shared_lookup();
      result != 0) {
    return result;
  }
  if (const int result = check_stack_destination_fail_closed_forms();
      result != 0) {
    return result;
  }
  if (const int result = check_pointer_base_fail_closed_forms();
      result != 0) {
    return result;
  }
  if (const int result = check_missing_and_unsupported_homes_fail_closed();
      result != 0) {
    return result;
  }
  return EXIT_SUCCESS;
}
