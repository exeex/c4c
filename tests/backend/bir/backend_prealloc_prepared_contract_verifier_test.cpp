#include "src/backend/prealloc/prepared_contract_verifier.hpp"
#include "src/backend/prealloc/calls.hpp"
#include "src/backend/prealloc/publication_plans.hpp"

#include "src/backend/bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

prepare::PreparedSelectedLocalStorageContractFacts coherent_local_storage() {
  return prepare::PreparedSelectedLocalStorageContractFacts{
      .function_name = c4c::FunctionNameId{7},
      .object_id = prepare::PreparedObjectId{3},
      .frame_slot_id = prepare::PreparedFrameSlotId{11},
      .byte_offset = 16,
      .size_bytes = 8,
      .align_bytes = 8,
      .has_extent = true,
      .has_alignment = true,
      .has_byte_range = true,
      .alias_authority_required = true,
      .has_alias_authority = true,
      .has_memory_provenance = true,
  };
}

prepare::PreparedSelectedObjectDataContractFacts coherent_object_data() {
  return prepare::PreparedSelectedObjectDataContractFacts{
      .object_label = c4c::LinkNameId{5},
      .object_size_bytes = 16,
      .emitted_byte_count = 8,
      .zero_fill_byte_count = 8,
      .has_object_label = true,
      .has_publication_identity = true,
      .requires_emitted_bytes = true,
      .has_emitted_bytes = true,
      .requires_zero_fill = true,
      .has_zero_fill = true,
      .requires_relocation = true,
      .has_relocation = true,
      .has_object_byte_range = true,
  };
}

prepare::PreparedCallArgumentSourceSelection coherent_frame_slot_address_route() {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
      .source_value_id = prepare::PreparedValueId{19},
      .source_value_name = c4c::ValueNameId{23},
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .source_slot_id = prepare::PreparedFrameSlotId{29},
      .source_stack_offset_bytes = std::size_t{32},
      .source_size_bytes = std::size_t{8},
      .source_align_bytes = std::size_t{8},
      .address_materialization_block_label = c4c::BlockLabelId{31},
      .address_materialization_inst_index = std::size_t{5},
      .address_materialization_frame_slot_id = prepare::PreparedFrameSlotId{29},
      .address_materialization_byte_offset = std::int64_t{32},
  };
}

prepare::PreparedCallArgumentSourceSelection coherent_frame_slot_value_route() {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
      .source_value_id = prepare::PreparedValueId{37},
      .source_value_name = c4c::ValueNameId{41},
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .source_slot_id = prepare::PreparedFrameSlotId{43},
      .source_stack_offset_bytes = std::size_t{48},
      .source_size_bytes = std::size_t{4},
      .source_align_bytes = std::size_t{4},
  };
}

prepare::PreparedCallArgumentSourceSelection
coherent_local_frame_address_materialization_route() {
  return prepare::PreparedCallArgumentSourceSelection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::
          LocalFrameAddressMaterialization,
      .source_value_id = prepare::PreparedValueId{53},
      .source_value_name = c4c::ValueNameId{59},
      .source_home_kind = prepare::PreparedValueHomeKind::Register,
      .source_slot_id = prepare::PreparedFrameSlotId{61},
      .source_stack_offset_bytes = std::size_t{64},
      .source_size_bytes = std::size_t{16},
      .source_align_bytes = std::size_t{8},
      .source_pointer_byte_delta = std::int64_t{0},
      .address_materialization_block_label = c4c::BlockLabelId{67},
      .address_materialization_inst_index = std::size_t{3},
      .address_materialization_frame_slot_id = prepare::PreparedFrameSlotId{61},
      .address_materialization_byte_offset = std::int64_t{64},
  };
}

prepare::PreparedValueHome coherent_rematerializable_integer_immediate_home() {
  return prepare::PreparedValueHome{
      .value_id = prepare::PreparedValueId{0},
      .function_name = c4c::FunctionNameId{73},
      .value_name = c4c::ValueNameId{79},
      .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
      .immediate_i32 = std::int64_t{511},
  };
}

prepare::PreparedValueHome coherent_pointer_base_plus_offset_home() {
  return prepare::PreparedValueHome{
      .value_id = prepare::PreparedValueId{1},
      .function_name = c4c::FunctionNameId{83},
      .value_name = c4c::ValueNameId{89},
      .kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset,
      .pointer_base_value_name = c4c::ValueNameId{97},
      .pointer_base_symbol_name = c4c::LinkNameId{101},
      .pointer_byte_delta = std::int64_t{12},
  };
}

prepare::PreparedValueHome coherent_va_start_stack_home() {
  return prepare::PreparedValueHome{
      .value_id = prepare::PreparedValueId{2},
      .function_name = c4c::FunctionNameId{107},
      .value_name = c4c::ValueNameId{109},
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{113},
      .offset_bytes = std::size_t{16},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
  };
}

prepare::PreparedValueHome coherent_va_start_address_home() {
  return prepare::PreparedValueHome{
      .value_id = prepare::PreparedValueId{3},
      .function_name = c4c::FunctionNameId{107},
      .value_name = c4c::ValueNameId{109},
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"x10"},
  };
}

int verify_call_argument_binary_producer_materialization_fact_query() {
  prepare::PreparedNameTables names;
  const auto block_label = names.block_labels.intern("fact.entry");
  const auto loaded_name = names.value_names.intern("%loaded");
  const auto sum_name = names.value_names.intern("%sum");
  const auto shifted_name = names.value_names.intern("%shifted");

  bir::Block block;
  block.label = "fact.entry";
  block.label_id = block_label;
  block.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
      .slot_name = "slot",
      .align_bytes = 4,
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%loaded"),
      .rhs = bir::Value::immediate_i32(9),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Shl,
      .result = bir::Value::named(bir::TypeKind::I32, "%shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .rhs = bir::Value::immediate_i32(1),
  });

  const auto* loaded = std::get_if<bir::LoadLocalInst>(&block.insts[0]);
  const auto* sum = std::get_if<bir::BinaryInst>(&block.insts[1]);
  const auto* shifted = std::get_if<bir::BinaryInst>(&block.insts[2]);
  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      loaded_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 0,
          .load_local = loaded,
      });
  source_producers.producers_by_value_name.emplace(
      sum_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 1,
          .binary = sum,
      });
  source_producers.producers_by_value_name.emplace(
      shifted_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 2,
          .binary = shifted,
      });

  const auto coherent =
      prepare::find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          3);
  auto stale_source_producers = source_producers;
  stale_source_producers.producers_by_value_name[sum_name].binary = shifted;

  if (!expect(coherent.has_value(),
              "coherent call-argument binary producer fact should be available") ||
      !expect(coherent->materializable,
              "coherent call-argument binary producer fact should be materializable") ||
      !expect(coherent->same_block_before_call,
              "coherent call-argument binary producer fact should carry scheduling authority") ||
      !expect(coherent->destination_value_name == sum_name,
              "call-argument binary producer fact should preserve destination identity") ||
      !expect(coherent->destination_value_type == bir::TypeKind::I32,
              "call-argument binary producer fact should preserve destination type") ||
      !expect(coherent->producer_block_label == block_label,
              "call-argument binary producer fact should preserve producer block") ||
      !expect(coherent->producer_instruction_index == 1,
              "call-argument binary producer fact should preserve producer instruction index") ||
      !expect(coherent->producer_kind ==
                  prepare::PreparedEdgePublicationSourceProducerKind::Binary,
              "call-argument binary producer fact should expose binary producer kind") ||
      !expect(coherent->producer_instruction == &block.insts[1],
              "call-argument binary producer fact should preserve instruction identity") ||
      !expect(coherent->binary == sum,
              "call-argument binary producer fact should preserve binary payload") ||
      !expect(coherent->binary_opcode == bir::BinaryOpcode::Add,
              "call-argument binary producer fact should expose binary opcode") ||
      !expect(coherent->lhs.name == "%loaded" &&
                  coherent->rhs.kind == bir::Value::Kind::Immediate &&
                  coherent->rhs.immediate == 9,
              "call-argument binary producer fact should expose operands") ||
      !expect(!prepare::
                   find_prepared_call_argument_binary_producer_materialization_fact(
                       names,
                       &source_producers,
                       block_label,
                       &block,
                       bir::Value::named(bir::TypeKind::I32, "%loaded"),
                       3)
                   .has_value(),
              "call-argument binary producer fact should reject cross-family load payload") ||
      !expect(!prepare::
                   find_prepared_call_argument_binary_producer_materialization_fact(
                       names,
                       &source_producers,
                       block_label,
                       &block,
                       bir::Value::named(bir::TypeKind::I32, "%shifted"),
                       3)
                   .has_value(),
              "call-argument binary producer fact should reject unsupported opcode") ||
      !expect(!prepare::
                   find_prepared_call_argument_binary_producer_materialization_fact(
                       names,
                       nullptr,
                       block_label,
                       &block,
                       bir::Value::named(bir::TypeKind::I32, "%sum"),
                       3)
                   .has_value(),
              "call-argument binary producer fact should reject missing lookup table") ||
      !expect(!prepare::
                   find_prepared_call_argument_binary_producer_materialization_fact(
                       names,
                       &source_producers,
                       block_label,
                       &block,
                       bir::Value::named(bir::TypeKind::I64, "%sum"),
                       3)
                   .has_value(),
              "call-argument binary producer fact should reject mismatched value type") ||
      !expect(!prepare::
                   find_prepared_call_argument_binary_producer_materialization_fact(
                       names,
                       &source_producers,
                       block_label,
                       &block,
                       bir::Value::named(bir::TypeKind::I32, "%sum"),
                       1)
                   .has_value(),
              "call-argument binary producer fact should reject future producer") ||
      !expect(!prepare::
                   find_prepared_call_argument_binary_producer_materialization_fact(
                       names,
                       &stale_source_producers,
                       block_label,
                       &block,
                       bir::Value::named(bir::TypeKind::I32, "%sum"),
                       3)
                   .has_value(),
              "call-argument binary producer fact should reject stale producer payload")) {
    return 1;
  }

  return 0;
}

int verify_call_argument_binary_producer_materialization_contract_reports() {
  prepare::PreparedNameTables names;
  const auto block_label = names.block_labels.intern("verify.entry");
  const auto sum_name = names.value_names.intern("%sum");

  bir::Block block;
  block.label = "verify.entry";
  block.label_id = block_label;
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
      .rhs = bir::Value::immediate_i32(9),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "%stale"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
      .rhs = bir::Value::immediate_i32(1),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Shl,
      .result = bir::Value::named(bir::TypeKind::I32, "%shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
      .rhs = bir::Value::immediate_i32(1),
  });

  const auto* sum = std::get_if<bir::BinaryInst>(&block.insts[0]);
  const auto* shifted = std::get_if<bir::BinaryInst>(&block.insts[2]);
  prepare::PreparedCallArgumentBinaryProducerMaterializationFact fact{
      .destination_value_name = sum_name,
      .destination_value_type = bir::TypeKind::I32,
      .producer_block_label = block_label,
      .producer_instruction_index = 0,
      .producer_kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
      .producer_instruction = &block.insts[0],
      .binary = sum,
      .binary_opcode = bir::BinaryOpcode::Add,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
      .rhs = bir::Value::immediate_i32(9),
      .same_block_before_call = true,
      .materializable = true,
  };

  const auto coherent =
      prepare::
          verify_prepared_call_argument_binary_producer_materialization_contract(
              &fact);
  const auto missing =
      prepare::
          verify_prepared_call_argument_binary_producer_materialization_contract(
              nullptr);

  auto cross_family = fact;
  cross_family.producer_kind =
      prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal;
  const auto cross_family_report =
      prepare::
          verify_prepared_call_argument_binary_producer_materialization_contract(
              &cross_family);

  auto stale = fact;
  stale.producer_instruction = &block.insts[1];
  const auto stale_report =
      prepare::
          verify_prepared_call_argument_binary_producer_materialization_contract(
              &stale);

  auto unsupported = fact;
  unsupported.producer_instruction = &block.insts[2];
  unsupported.binary = shifted;
  unsupported.binary_opcode = bir::BinaryOpcode::Shl;
  unsupported.lhs = shifted->lhs;
  unsupported.rhs = shifted->rhs;
  const auto unsupported_report =
      prepare::
          verify_prepared_call_argument_binary_producer_materialization_contract(
              &unsupported);

  auto contradictory_opcode = fact;
  contradictory_opcode.binary_opcode = bir::BinaryOpcode::Sub;
  const auto contradictory_opcode_report =
      prepare::
          verify_prepared_call_argument_binary_producer_materialization_contract(
              &contradictory_opcode);

  auto contradictory_operands = fact;
  contradictory_operands.rhs = bir::Value::immediate_i32(8);
  const auto contradictory_operands_report =
      prepare::
          verify_prepared_call_argument_binary_producer_materialization_contract(
              &contradictory_operands);

  auto missing_authority = fact;
  missing_authority.same_block_before_call = false;
  const auto missing_authority_report =
      prepare::
          verify_prepared_call_argument_binary_producer_materialization_contract(
              &missing_authority);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete call-argument binary producer fact should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent call-argument binary producer fact should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::ValueMaterializationFact,
              "call-argument binary producer fact should identify materialization family") ||
      !expect(coherent.value_name == sum_name,
              "coherent call-argument binary producer fact should preserve value identity") ||
      !expect(missing.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing call-argument binary producer fact should classify as producer missing") ||
      !expect(missing.fail_closed,
              "missing call-argument binary producer fact should fail closed") ||
      !expect(cross_family_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-family call-argument producer fact should classify as producer incoherent") ||
      !expect(stale_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "stale call-argument binary producer fact should classify as producer incoherent") ||
      !expect(unsupported_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "unsupported call-argument binary opcode should classify as producer incoherent") ||
      !expect(contradictory_opcode_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "contradictory call-argument binary opcode payload should classify as producer incoherent") ||
      !expect(contradictory_operands_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "contradictory call-argument binary operands should classify as producer incoherent") ||
      !expect(missing_authority_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing scheduling authority should classify as producer missing") ||
      !expect(prepare::
                  classify_prepared_call_argument_binary_producer_materialization_contract(
                      &unsupported) ==
                  prepare::
                      PreparedCallArgumentBinaryProducerMaterializationContractStatus::
                          UnsupportedBinaryOpcode,
              "unsupported binary opcode should have precise materialization status") ||
      !expect(prepare::
                  prepared_call_argument_binary_producer_materialization_contract_status_name(
                      prepare::
                          PreparedCallArgumentBinaryProducerMaterializationContractStatus::
                              ConflictingStaleInstruction) ==
                  std::string_view{"conflicting_stale_instruction"},
              "call-argument binary producer materialization status spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_variadic_va_start_helper_operand_home_reports() {
  auto destination = coherent_va_start_stack_home();
  auto address = coherent_va_start_address_home();
  prepare::PreparedVariadicEntryHelperOperandHomes coherent_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
      .block_index = 1,
      .instruction_index = 2,
      .destination_va_list = destination,
      .destination_va_list_address = address,
  };
  prepare::publish_prepared_variadic_va_start_operand_homes(coherent_homes);

  prepare::PreparedVariadicEntryHelperOperandHomes legacy_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
      .block_index = 1,
      .instruction_index = 2,
      .destination_va_list = destination,
      .destination_va_list_address = address,
  };

  prepare::PreparedVariadicEntryHelperOperandHomes wrong_helper_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
      .block_index = 1,
      .instruction_index = 2,
      .destination_va_list = destination,
      .destination_va_list_address = address,
  };
  prepare::publish_prepared_variadic_va_start_operand_homes(wrong_helper_homes);

  prepare::PreparedVariadicEntryHelperOperandHomes incomplete_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
      .block_index = 1,
      .instruction_index = 2,
      .destination_va_list = destination,
  };
  prepare::publish_prepared_variadic_va_start_operand_homes(incomplete_homes);

  const auto coherent =
      prepare::verify_prepared_variadic_entry_helper_operand_homes_contract(
          &coherent_homes,
          c4c::FunctionNameId{107},
          prepare::PreparedVariadicEntryHelperKind::VaStart,
          1,
          2);
  const auto legacy =
      prepare::verify_prepared_variadic_entry_helper_operand_homes_contract(
          &legacy_homes,
          c4c::FunctionNameId{107},
          prepare::PreparedVariadicEntryHelperKind::VaStart,
          1,
          2);
  const auto missing =
      prepare::verify_prepared_variadic_entry_helper_operand_homes_contract(
          nullptr,
          c4c::FunctionNameId{107},
          prepare::PreparedVariadicEntryHelperKind::VaStart,
          1,
          2);
  const auto wrong_helper =
      prepare::verify_prepared_variadic_entry_helper_operand_homes_contract(
          &wrong_helper_homes,
          c4c::FunctionNameId{107},
          prepare::PreparedVariadicEntryHelperKind::VaStart,
          1,
          2);
  const auto incomplete =
      prepare::verify_prepared_variadic_entry_helper_operand_homes_contract(
          &incomplete_homes,
          c4c::FunctionNameId{107},
          prepare::PreparedVariadicEntryHelperKind::VaStart,
          1,
          2);

  if (!expect(prepare::find_prepared_variadic_va_start_operand_homes(
                  coherent_homes) != nullptr,
              "coherent va_start helper homes should expose a typed payload") ||
      !expect(prepare::find_prepared_variadic_va_start_operand_homes(
                  legacy_homes) != nullptr,
              "complete legacy va_start optional fields should expose a typed payload") ||
      !expect(prepare::find_prepared_variadic_va_start_operand_homes(
                  wrong_helper_homes) == nullptr,
              "wrong-helper optional va_start fields should not expose a typed payload") ||
      !expect(prepare::find_prepared_variadic_va_start_operand_homes(
                  incomplete_homes) == nullptr,
              "incomplete va_start optional fields should not expose a typed payload") ||
      !expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "typed va_start helper homes should verify as coherent") ||
      !expect(!coherent.fail_closed,
              "typed va_start helper homes should not fail closed") ||
      !expect(legacy.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete legacy va_start optional fields should verify as coherent") ||
      !expect(!legacy.fail_closed,
              "complete legacy va_start optional fields should not fail closed") ||
      !expect(missing.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing va_start helper homes should classify as producer missing") ||
      !expect(missing.fail_closed,
              "missing va_start helper homes should fail closed") ||
      !expect(wrong_helper.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "wrong-helper va_start optional fields should classify as producer incoherent") ||
      !expect(wrong_helper.fail_closed,
              "wrong-helper va_start optional fields should fail closed") ||
      !expect(incomplete.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "incomplete va_start optional fields should classify as producer incoherent") ||
      !expect(incomplete.fail_closed,
              "incomplete va_start optional fields should fail closed")) {
    return 1;
  }

  return 0;
}

int verify_rematerializable_integer_immediate_contract_reports() {
  auto coherent_home = coherent_rematerializable_integer_immediate_home();
  const auto coherent =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(
          &coherent_home);

  auto missing_payload = coherent_rematerializable_integer_immediate_home();
  missing_payload.immediate_i32 = std::nullopt;
  const auto missing_payload_report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(
          &missing_payload);

  auto missing_identity = coherent_rematerializable_integer_immediate_home();
  missing_identity.value_name = c4c::kInvalidValueName;
  const auto missing_identity_report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(
          &missing_identity);

  auto wrong_home = coherent_rematerializable_integer_immediate_home();
  wrong_home.kind = prepare::PreparedValueHomeKind::Register;
  const auto wrong_home_report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(
          &wrong_home);

  auto cross_family = coherent_rematerializable_integer_immediate_home();
  cross_family.immediate_f128 =
      c4c::backend::bir::Value::F128Payload{.low_bits = 1, .high_bits = 2};
  const auto cross_family_report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(
          &cross_family);

  const auto missing_home_report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(
          nullptr);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete rematerializable integer immediate should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent rematerializable integer immediate should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::ValueMaterializationFact,
              "rematerializable integer immediate should identify materialization family") ||
      !expect(coherent.value_id == prepare::PreparedValueId{0} &&
                  coherent.function_name == c4c::FunctionNameId{73} &&
                  coherent.value_name == c4c::ValueNameId{79},
              "coherent rematerializable integer immediate should preserve identity") ||
      !expect(missing_payload_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing immediate payload should classify as producer missing") ||
      !expect(prepare::classify_prepared_rematerializable_integer_immediate_contract(
                  &missing_payload) ==
                  prepare::PreparedRematerializableIntegerImmediateContractStatus::
                      MissingImmediatePayload,
              "missing immediate payload should have precise status") ||
      !expect(missing_identity_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing value identity should classify as producer missing") ||
      !expect(wrong_home_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "wrong immediate home kind should classify as producer incoherent") ||
      !expect(cross_family_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-family immediate payload should classify as producer incoherent") ||
      !expect(missing_home_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "absent immediate value home should classify as producer missing") ||
      !expect(prepare::
                  prepared_rematerializable_integer_immediate_contract_status_name(
                      prepare::
                          PreparedRematerializableIntegerImmediateContractStatus::
                              ConflictingCrossFamilyPayload) ==
                  std::string_view{"conflicting_cross_family_payload"},
              "rematerializable immediate status spelling mismatch") ||
      !expect(prepare::prepared_contract_fact_family_name(
                  prepare::PreparedContractFactFamily::ValueMaterializationFact) ==
                  std::string_view{"value_materialization_fact"},
              "value materialization fact family spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_pointer_base_plus_offset_contract_reports() {
  auto coherent_home = coherent_pointer_base_plus_offset_home();
  const auto coherent =
      prepare::verify_prepared_pointer_base_plus_offset_contract(&coherent_home);

  auto missing_base = coherent_pointer_base_plus_offset_home();
  missing_base.pointer_base_value_name = std::nullopt;
  const auto missing_base_report =
      prepare::verify_prepared_pointer_base_plus_offset_contract(&missing_base);

  auto invalid_base = coherent_pointer_base_plus_offset_home();
  invalid_base.pointer_base_value_name = c4c::kInvalidValueName;
  const auto invalid_base_report =
      prepare::verify_prepared_pointer_base_plus_offset_contract(&invalid_base);

  auto missing_delta = coherent_pointer_base_plus_offset_home();
  missing_delta.pointer_byte_delta = std::nullopt;
  const auto missing_delta_report =
      prepare::verify_prepared_pointer_base_plus_offset_contract(&missing_delta);

  auto missing_identity = coherent_pointer_base_plus_offset_home();
  missing_identity.value_name = c4c::kInvalidValueName;
  const auto missing_identity_report =
      prepare::verify_prepared_pointer_base_plus_offset_contract(&missing_identity);

  auto wrong_home = coherent_pointer_base_plus_offset_home();
  wrong_home.kind = prepare::PreparedValueHomeKind::Register;
  const auto wrong_home_report =
      prepare::verify_prepared_pointer_base_plus_offset_contract(&wrong_home);

  auto cross_family_i32 = coherent_pointer_base_plus_offset_home();
  cross_family_i32.immediate_i32 = std::int64_t{7};
  const auto cross_family_i32_report =
      prepare::verify_prepared_pointer_base_plus_offset_contract(&cross_family_i32);

  auto cross_family_f128 = coherent_pointer_base_plus_offset_home();
  cross_family_f128.immediate_f128 =
      c4c::backend::bir::Value::F128Payload{.low_bits = 1, .high_bits = 2};
  const auto cross_family_f128_report =
      prepare::verify_prepared_pointer_base_plus_offset_contract(&cross_family_f128);

  const auto missing_home_report =
      prepare::verify_prepared_pointer_base_plus_offset_contract(nullptr);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete pointer base-plus-offset should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent pointer base-plus-offset should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::ValueMaterializationFact,
              "pointer base-plus-offset should identify materialization family") ||
      !expect(coherent.value_id == prepare::PreparedValueId{1} &&
                  coherent.function_name == c4c::FunctionNameId{83} &&
                  coherent.value_name == c4c::ValueNameId{89},
              "coherent pointer base-plus-offset should preserve identity") ||
      !expect(missing_base_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing pointer base should classify as producer missing") ||
      !expect(prepare::classify_prepared_pointer_base_plus_offset_contract(
                  &invalid_base) ==
                  prepare::PreparedPointerBasePlusOffsetContractStatus::
                      MissingPointerBase,
              "invalid pointer base should have precise missing-base status") ||
      !expect(missing_delta_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing pointer delta should classify as producer missing") ||
      !expect(prepare::classify_prepared_pointer_base_plus_offset_contract(
                  &missing_delta) ==
                  prepare::PreparedPointerBasePlusOffsetContractStatus::
                      MissingPointerByteDelta,
              "missing pointer delta should have precise status") ||
      !expect(missing_identity_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing pointer destination identity should classify as producer missing") ||
      !expect(wrong_home_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "wrong pointer home kind should classify as producer incoherent") ||
      !expect(cross_family_i32_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-family pointer i32 payload should classify as producer incoherent") ||
      !expect(cross_family_f128_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-family pointer f128 payload should classify as producer incoherent") ||
      !expect(missing_home_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "absent pointer value home should classify as producer missing") ||
      !expect(prepare::prepared_pointer_base_plus_offset_contract_status_name(
                  prepare::PreparedPointerBasePlusOffsetContractStatus::
                      ConflictingCrossFamilyPayload) ==
                  std::string_view{"conflicting_cross_family_payload"},
              "pointer materialization status spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_selected_local_storage_contract_reports() {
  const auto coherent =
      prepare::verify_prepared_selected_local_storage_contract(
          coherent_local_storage());

  auto missing = coherent_local_storage();
  missing.has_extent = false;
  const auto missing_report =
      prepare::verify_prepared_selected_local_storage_contract(missing);

  auto incoherent = coherent_local_storage();
  incoherent.conflicting_byte_range = true;
  const auto incoherent_report =
      prepare::verify_prepared_selected_local_storage_contract(incoherent);

  auto unsupported = coherent_local_storage();
  unsupported.unsupported_but_coherent = true;
  const auto unsupported_report =
      prepare::verify_prepared_selected_local_storage_contract(unsupported);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete local storage facts should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent local storage facts should not fail closed") ||
      !expect(missing_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing extent should classify as producer missing") ||
      !expect(missing_report.fact_family ==
                  prepare::PreparedContractFactFamily::StorageObjectExtent,
              "missing extent should identify storage object extent family") ||
      !expect(incoherent_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "conflicting byte range should classify as producer incoherent") ||
      !expect(incoherent_report.fact_family ==
                  prepare::PreparedContractFactFamily::StorageByteRange,
              "conflicting byte range should identify storage byte range family") ||
      !expect(unsupported_report.owner_class ==
                  prepare::PreparedContractOwnerClass::
                      TargetUnsupportedButCoherent,
              "complete unsupported storage form should classify as target unsupported") ||
      !expect(prepare::prepared_contract_fact_family_name(
                  prepare::PreparedContractFactFamily::MemoryAccessProvenance) ==
                  std::string_view{"memory_access_provenance"},
              "memory provenance family spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_selected_object_data_contract_reports() {
  const auto coherent =
      prepare::verify_prepared_selected_object_data_contract(coherent_object_data());

  auto missing = coherent_object_data();
  missing.has_relocation = false;
  const auto missing_report =
      prepare::verify_prepared_selected_object_data_contract(missing);

  auto incoherent = coherent_object_data();
  incoherent.conflicting_zero_fill = true;
  const auto incoherent_report =
      prepare::verify_prepared_selected_object_data_contract(incoherent);

  auto unsupported = coherent_object_data();
  unsupported.requires_unsupported_marker = true;
  unsupported.has_unsupported_marker = true;
  unsupported.unsupported_but_coherent = true;
  const auto unsupported_report =
      prepare::verify_prepared_selected_object_data_contract(unsupported);

  auto semantic_failure = coherent_object_data();
  semantic_failure.invalid_pre_prepared_initializer_semantics = true;
  const auto semantic_failure_report =
      prepare::verify_prepared_selected_object_data_contract(semantic_failure);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete object data facts should be coherent") ||
      !expect(missing_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing relocation should classify as producer missing") ||
      !expect(missing_report.fact_family ==
                  prepare::PreparedContractFactFamily::ObjectRelocation,
              "missing relocation should identify object relocation family") ||
      !expect(incoherent_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "conflicting zero-fill should classify as producer incoherent") ||
      !expect(incoherent_report.fact_family ==
                  prepare::PreparedContractFactFamily::ObjectZeroFill,
              "conflicting zero-fill should identify object zero-fill family") ||
      !expect(unsupported_report.owner_class ==
                  prepare::PreparedContractOwnerClass::
                      TargetUnsupportedButCoherent,
              "complete unsupported object form should classify as target unsupported") ||
      !expect(unsupported_report.fact_family ==
                  prepare::PreparedContractFactFamily::UnsupportedObjectDataMarker,
              "unsupported object form should identify marker family") ||
      !expect(semantic_failure_report.owner_class ==
                  prepare::PreparedContractOwnerClass::
                      PrePreparedSemanticFailure,
              "invalid initializer semantics should classify before prepared") ||
      !expect(prepare::prepared_selected_object_data_contract_status_name(
                  prepare::PreparedSelectedObjectDataContractStatus::
                      InvalidPrePreparedInitializerSemantics) ==
                  std::string_view{
                      "invalid_pre_prepared_initializer_semantics"},
              "semantic failure status spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_frame_slot_address_source_route_contract_reports() {
  auto coherent_route = coherent_frame_slot_address_route();
  const auto coherent =
      prepare::verify_prepared_frame_slot_address_source_route_contract(
          &coherent_route);

  auto missing = coherent_frame_slot_address_route();
  missing.source_stack_offset_bytes = std::nullopt;
  const auto missing_report =
      prepare::verify_prepared_frame_slot_address_source_route_contract(
          &missing);

  auto incoherent = coherent_frame_slot_address_route();
  incoherent.address_materialization_frame_slot_id =
      prepare::PreparedFrameSlotId{30};
  const auto incoherent_report =
      prepare::verify_prepared_frame_slot_address_source_route_contract(
          &incoherent);

  auto cross_route = coherent_frame_slot_address_route();
  cross_route.byval_lane_extent_bytes = std::size_t{8};
  const auto cross_route_report =
      prepare::verify_prepared_frame_slot_address_source_route_contract(
          &cross_route);

  const auto missing_route_report =
      prepare::verify_prepared_frame_slot_address_source_route_contract(nullptr);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete frame-slot address route should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent frame-slot address route should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::CallArgumentTypedRoute,
              "frame-slot address route should identify typed route family") ||
      !expect(missing_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing frame-slot address stack offset should classify as producer missing") ||
      !expect(missing_report.fail_closed,
              "missing frame-slot address route fact should fail closed") ||
      !expect(prepare::classify_prepared_frame_slot_address_source_route_contract(
                  &missing) ==
                  prepare::PreparedFrameSlotAddressSourceRouteContractStatus::
                      MissingStackOffset,
              "missing stack offset should have precise route status") ||
      !expect(incoherent_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "contradictory materialization slot should classify as producer incoherent") ||
      !expect(cross_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-route payload should classify as producer incoherent") ||
      !expect(missing_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "absent frame-slot address route should classify as producer missing") ||
      !expect(prepare::prepared_contract_fact_family_name(
                  prepare::PreparedContractFactFamily::CallArgumentTypedRoute) ==
                  std::string_view{"call_argument_typed_route"},
              "typed call-argument route family spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_frame_slot_value_source_route_contract_reports() {
  auto coherent_route = coherent_frame_slot_value_route();
  const auto coherent =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &coherent_route);

  auto missing = coherent_frame_slot_value_route();
  missing.source_value_name = std::nullopt;
  const auto missing_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &missing);

  auto wrong_home = coherent_frame_slot_value_route();
  wrong_home.source_home_kind = prepare::PreparedValueHomeKind::Register;
  const auto wrong_home_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &wrong_home);

  auto address_payload = coherent_frame_slot_value_route();
  address_payload.address_materialization_block_label = c4c::BlockLabelId{5};
  const auto address_payload_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &address_payload);

  auto cross_route = coherent_frame_slot_value_route();
  cross_route.byval_lane_extent_bytes = std::size_t{4};
  const auto cross_route_report =
      prepare::verify_prepared_frame_slot_value_source_route_contract(
          &cross_route);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete frame-slot value route should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent frame-slot value route should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::CallArgumentTypedRoute,
              "frame-slot value route should identify typed route family") ||
      !expect(missing_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing frame-slot value identity should classify as producer missing") ||
      !expect(prepare::classify_prepared_frame_slot_value_source_route_contract(
                  &missing) ==
                  prepare::PreparedFrameSlotValueSourceRouteContractStatus::
                      MissingSourceValueName,
              "missing value name should have precise route status") ||
      !expect(wrong_home_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "wrong frame-slot value home kind should classify as producer incoherent") ||
      !expect(address_payload_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "address materialization payload should classify as producer incoherent") ||
      !expect(cross_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-route frame-slot value payload should classify as producer incoherent") ||
      !expect(prepare::prepared_frame_slot_value_source_route_contract_status_name(
                  prepare::PreparedFrameSlotValueSourceRouteContractStatus::
                      ConflictingAddressMaterializationPayload) ==
                  std::string_view{"conflicting_address_materialization_payload"},
              "frame-slot value materialization status spelling mismatch")) {
    return 1;
  }

  return 0;
}

int verify_local_frame_address_materialization_source_route_contract_reports() {
  auto coherent_route = coherent_local_frame_address_materialization_route();
  const auto coherent =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &coherent_route);

  auto missing_delta = coherent_local_frame_address_materialization_route();
  missing_delta.source_pointer_byte_delta = std::nullopt;
  const auto missing_delta_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &missing_delta);

  auto missing_base = coherent_local_frame_address_materialization_route();
  missing_base.source_home_kind =
      prepare::PreparedValueHomeKind::PointerBasePlusOffset;
  missing_base.source_base_value_id = std::nullopt;
  const auto missing_base_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &missing_base);

  auto wrong_home = coherent_local_frame_address_materialization_route();
  wrong_home.source_home_kind = prepare::PreparedValueHomeKind::StackSlot;
  const auto wrong_home_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &wrong_home);

  auto missing_location = coherent_local_frame_address_materialization_route();
  missing_location.address_materialization_block_label = std::nullopt;
  const auto missing_location_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &missing_location);

  auto mismatched_slot = coherent_local_frame_address_materialization_route();
  mismatched_slot.address_materialization_frame_slot_id =
      prepare::PreparedFrameSlotId{62};
  const auto mismatched_slot_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &mismatched_slot);

  auto mismatched_offset = coherent_local_frame_address_materialization_route();
  mismatched_offset.address_materialization_byte_offset = std::int64_t{63};
  const auto mismatched_offset_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &mismatched_offset);

  auto cross_route = coherent_local_frame_address_materialization_route();
  cross_route.byval_lane_extent_bytes = std::size_t{8};
  const auto cross_route_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              &cross_route);

  const auto missing_route_report =
      prepare::
          verify_prepared_local_frame_address_materialization_source_route_contract(
              nullptr);

  if (!expect(coherent.owner_class == prepare::PreparedContractOwnerClass::Coherent,
              "complete local materialization route should be coherent") ||
      !expect(!coherent.fail_closed,
              "coherent local materialization route should not fail closed") ||
      !expect(coherent.fact_family ==
                  prepare::PreparedContractFactFamily::CallArgumentTypedRoute,
              "local materialization route should identify typed route family") ||
      !expect(missing_delta_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing pointer delta should classify as producer missing") ||
      !expect(prepare::
                  classify_prepared_local_frame_address_materialization_source_route_contract(
                      &missing_delta) ==
                  prepare::
                      PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
                          MissingPointerByteDelta,
              "missing pointer delta should have precise route status") ||
      !expect(missing_base_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing pointer base identity should classify as producer missing") ||
      !expect(prepare::
                  classify_prepared_local_frame_address_materialization_source_route_contract(
                      &missing_base) ==
                  prepare::
                      PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
                          MissingSourceBaseValueId,
              "missing pointer base identity should have precise route status") ||
      !expect(wrong_home_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "wrong local materialization home kind should classify as producer incoherent") ||
      !expect(missing_location_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "missing materialization location should classify as producer missing") ||
      !expect(mismatched_slot_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "contradictory local materialization slot should classify as producer incoherent") ||
      !expect(mismatched_offset_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "contradictory local materialization offset should classify as producer incoherent") ||
      !expect(cross_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerIncoherent,
              "cross-route local materialization payload should classify as producer incoherent") ||
      !expect(missing_route_report.owner_class ==
                  prepare::PreparedContractOwnerClass::ProducerMissing,
              "absent local materialization route should classify as producer missing") ||
      !expect(prepare::
                  prepared_local_frame_address_materialization_source_route_contract_status_name(
                      prepare::
                          PreparedLocalFrameAddressMaterializationSourceRouteContractStatus::
                              ConflictingMaterializationByteOffset) ==
                  std::string_view{"conflicting_materialization_byte_offset"},
              "local materialization offset status spelling mismatch")) {
    return 1;
  }

  return 0;
}

}  // namespace

int main() {
  if (const int rc = verify_call_argument_binary_producer_materialization_fact_query();
      rc != 0) {
    return rc;
  }
  if (const int rc =
          verify_call_argument_binary_producer_materialization_contract_reports();
      rc != 0) {
    return rc;
  }
  if (const int rc = verify_variadic_va_start_helper_operand_home_reports();
      rc != 0) {
    return rc;
  }
  if (const int rc = verify_rematerializable_integer_immediate_contract_reports();
      rc != 0) {
    return rc;
  }
  if (const int rc = verify_pointer_base_plus_offset_contract_reports(); rc != 0) {
    return rc;
  }
  if (const int rc = verify_selected_local_storage_contract_reports(); rc != 0) {
    return rc;
  }
  if (const int rc = verify_selected_object_data_contract_reports(); rc != 0) {
    return rc;
  }
  if (const int rc = verify_frame_slot_address_source_route_contract_reports();
      rc != 0) {
    return rc;
  }
  if (const int rc = verify_frame_slot_value_source_route_contract_reports();
      rc != 0) {
    return rc;
  }
  if (const int rc =
          verify_local_frame_address_materialization_source_route_contract_reports();
      rc != 0) {
    return rc;
  }
  return EXIT_SUCCESS;
}
