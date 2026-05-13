#include "src/backend/mir/aarch64/codegen/records.hpp"

#include <iostream>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bir::Value named_value(bir::TypeKind type, const char* name) {
  return bir::Value::named(type, name);
}

prepare::PreparedValueHome register_home(prepare::PreparedValueId value_id,
                                         c4c::FunctionNameId function_name,
                                         c4c::ValueNameId value_name,
                                         const char* register_name) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = register_name,
  };
}

struct PreparedMemoryFixture {
  prepare::PreparedNameTables names;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  c4c::ValueNameId load_name = c4c::kInvalidValueName;
  c4c::ValueNameId stored_name = c4c::kInvalidValueName;
  c4c::ValueNameId pointer_name = c4c::kInvalidValueName;
  c4c::LinkNameId global_name = c4c::kInvalidLinkName;
  c4c::LinkNameId string_symbol_name = c4c::kInvalidLinkName;
  c4c::TextId string_text_name = c4c::kInvalidText;
  prepare::PreparedValueLocationFunction locations;
  prepare::PreparedAddressingFunction addressing;
};

PreparedMemoryFixture make_fixture() {
  PreparedMemoryFixture fixture;
  fixture.function_name = fixture.names.function_names.intern("f");
  fixture.block_label = fixture.names.block_labels.intern("entry");
  fixture.load_name = fixture.names.value_names.intern("%load");
  fixture.stored_name = fixture.names.value_names.intern("%stored");
  fixture.pointer_name = fixture.names.value_names.intern("%ptr");
  fixture.global_name = fixture.names.link_names.intern("g.counter");
  fixture.string_symbol_name = fixture.names.link_names.intern(".L.str0");
  fixture.string_text_name = fixture.names.texts.find(".L.str0");
  fixture.locations = prepare::PreparedValueLocationFunction{
      .function_name = fixture.function_name,
      .value_homes =
          {
              register_home(prepare::PreparedValueId{10},
                            fixture.function_name,
                            fixture.load_name,
                            "w0"),
              register_home(prepare::PreparedValueId{11},
                            fixture.function_name,
                            fixture.stored_name,
                            "w1"),
              register_home(prepare::PreparedValueId{12},
                            fixture.function_name,
                            fixture.pointer_name,
                            "x2"),
          },
  };
  fixture.addressing = prepare::PreparedAddressingFunction{
      .function_name = fixture.function_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 2,
                  .result_value_name = fixture.load_name,
                  .address_space = bir::AddressSpace::Fs,
                  .is_volatile = true,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                          .frame_slot_id = prepare::PreparedFrameSlotId{20},
                          .byte_offset = 8,
                          .size_bytes = 4,
                          .align_bytes = 4,
                          .can_use_base_plus_offset = true,
                      },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 3,
                  .stored_value_name = fixture.stored_name,
                  .address_space = bir::AddressSpace::Gs,
                  .is_volatile = true,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                          .symbol_name = fixture.global_name,
                          .byte_offset = 16,
                          .size_bytes = 4,
                          .align_bytes = 4,
                          .can_use_base_plus_offset = true,
                      },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 4,
                  .stored_value_name = fixture.stored_name,
                  .address_space = bir::AddressSpace::Tls,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
                          .pointer_value_name = fixture.pointer_name,
                          .byte_offset = 24,
                          .size_bytes = 8,
                          .align_bytes = 8,
                          .can_use_base_plus_offset = true,
                      },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 5,
                  .result_value_name = fixture.load_name,
                  .address_space = bir::AddressSpace::Gs,
                  .is_volatile = true,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::StringConstant,
                          .symbol_name = fixture.string_symbol_name,
                          .byte_offset = 4,
                          .size_bytes = 8,
                          .align_bytes = 8,
                          .can_use_base_plus_offset = true,
                      },
              },
          },
  };
  return fixture;
}

int frame_slot_load_conversion_preserves_prepared_and_bir_facts() {
  auto fixture = make_fixture();
  const bir::LoadLocalInst load{
      .result = named_value(bir::TypeKind::I32, "%load"),
      .slot_id = c4c::SlotNameId{5},
      .byte_offset = 8,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .byte_offset = 8,
              .size_bytes = 4,
              .align_bytes = 4,
              .address_space = bir::AddressSpace::Fs,
              .is_volatile = true,
              .base_slot_id = c4c::SlotNameId{5},
          },
  };

  const auto result = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names, fixture.locations, fixture.addressing, fixture.block_label, 2, load);
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedMemoryOperandRecordError::None) {
    return fail("expected frame-slot prepared memory conversion to succeed");
  }

  const auto& memory = *result.record;
  if (memory.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      memory.support != aarch64_codegen::MemoryOperandSupportKind::Prepared ||
      memory.function_name != fixture.function_name || memory.block_label != fixture.block_label ||
      memory.instruction_index != 2 ||
      memory.result_value_id != prepare::PreparedValueId{10} ||
      memory.result_value_name != fixture.load_name || memory.stored_value_id.has_value() ||
      memory.stored_value_name.has_value()) {
    return fail("expected frame-slot record to preserve prepared access identity");
  }
  if (memory.base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      memory.frame_slot_id != prepare::PreparedFrameSlotId{20} ||
      memory.byte_offset != 8 || memory.size_bytes != 4 || memory.align_bytes != 4 ||
      memory.address_space != bir::AddressSpace::Fs || !memory.is_volatile ||
      !memory.can_use_base_plus_offset) {
    return fail("expected frame-slot record to preserve address facts");
  }
  return 0;
}

int global_symbol_store_conversion_preserves_prepared_and_bir_facts() {
  auto fixture = make_fixture();
  const bir::StoreGlobalInst store{
      .global_name_id = fixture.global_name,
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 16,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .byte_offset = 16,
              .size_bytes = 4,
              .align_bytes = 4,
              .address_space = bir::AddressSpace::Gs,
              .is_volatile = true,
              .base_link_name_id = fixture.global_name,
          },
  };

  const auto result = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names, fixture.locations, fixture.addressing, fixture.block_label, 3, store);
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedMemoryOperandRecordError::None) {
    return fail("expected global-symbol prepared memory conversion to succeed");
  }

  const auto& memory = *result.record;
  if (memory.base_kind != aarch64_codegen::MemoryBaseKind::Symbol ||
      memory.symbol_name != fixture.global_name ||
      memory.stored_value_id != prepare::PreparedValueId{11} ||
      memory.stored_value_name != fixture.stored_name || memory.result_value_id.has_value() ||
      memory.result_value_name.has_value() || memory.byte_offset != 16 ||
      memory.size_bytes != 4 || memory.align_bytes != 4 ||
      memory.address_space != bir::AddressSpace::Gs || !memory.is_volatile ||
      !memory.can_use_base_plus_offset) {
    return fail("expected global-symbol record to preserve prepared and BIR facts");
  }
  return 0;
}

int pointer_value_store_conversion_preserves_prepared_and_bir_facts() {
  auto fixture = make_fixture();
  const bir::StoreLocalInst store{
      .slot_id = c4c::SlotNameId{5},
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 24,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
              .base_value = named_value(bir::TypeKind::Ptr, "%ptr"),
              .byte_offset = 24,
              .size_bytes = 8,
              .align_bytes = 8,
              .address_space = bir::AddressSpace::Tls,
          },
  };

  const auto result = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names, fixture.locations, fixture.addressing, fixture.block_label, 4, store);
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedMemoryOperandRecordError::None) {
    return fail("expected pointer-value prepared memory conversion to succeed");
  }

  const auto& memory = *result.record;
  if (memory.base_kind != aarch64_codegen::MemoryBaseKind::PointerValue ||
      memory.pointer_value_name != fixture.pointer_name ||
      memory.pointer_value_id != prepare::PreparedValueId{12} ||
      memory.stored_value_id != prepare::PreparedValueId{11} ||
      memory.stored_value_name != fixture.stored_name || memory.result_value_id.has_value() ||
      memory.result_value_name.has_value() || memory.byte_offset != 24 ||
      memory.size_bytes != 8 || memory.align_bytes != 8 ||
      memory.address_space != bir::AddressSpace::Tls || memory.is_volatile ||
      !memory.can_use_base_plus_offset) {
    return fail("expected pointer-value record to preserve prepared and BIR facts");
  }
  return 0;
}

int string_constant_load_conversion_preserves_prepared_and_bir_facts() {
  auto fixture = make_fixture();
  const bir::LoadGlobalInst load{
      .result = named_value(bir::TypeKind::I32, "%load"),
      .byte_offset = 4,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::StringConstant,
              .base_name = ".L.str0",
              .byte_offset = 4,
              .size_bytes = 8,
              .align_bytes = 8,
              .address_space = bir::AddressSpace::Gs,
              .is_volatile = true,
          },
  };

  const auto result = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names, fixture.locations, fixture.addressing, fixture.block_label, 5, load);
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedMemoryOperandRecordError::None) {
    return fail("expected string-constant prepared memory conversion to succeed");
  }

  const auto& memory = *result.record;
  if (memory.base_kind != aarch64_codegen::MemoryBaseKind::StringConstant ||
      memory.string_symbol_name != fixture.string_symbol_name ||
      memory.string_name != fixture.string_text_name ||
      memory.result_value_id != prepare::PreparedValueId{10} ||
      memory.result_value_name != fixture.load_name || memory.stored_value_id.has_value() ||
      memory.stored_value_name.has_value() || memory.byte_offset != 4 ||
      memory.size_bytes != 8 || memory.align_bytes != 8 ||
      memory.address_space != bir::AddressSpace::Gs || !memory.is_volatile ||
      !memory.can_use_base_plus_offset) {
    return fail("expected string-constant record to preserve prepared and BIR facts");
  }
  return 0;
}

int unsupported_or_mismatched_memory_facts_fail_closed() {
  auto fixture = make_fixture();
  const bir::LoadLocalInst load{
      .result = named_value(bir::TypeKind::I32, "%load"),
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .byte_offset = 8,
              .size_bytes = 4,
              .align_bytes = 4,
              .address_space = bir::AddressSpace::Fs,
              .is_volatile = true,
              .base_slot_id = c4c::SlotNameId{5},
          },
  };
  const auto missing = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names, fixture.locations, fixture.addressing, fixture.block_label, 99, load);
  if (missing.record.has_value() ||
      missing.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess ||
      aarch64_codegen::prepared_memory_operand_record_error_name(missing.error) !=
          "missing_prepared_memory_access") {
    return fail("expected missing prepared memory access to fail closed");
  }

  fixture.addressing.accesses[0].address.base_kind =
      prepare::PreparedAddressBaseKind::PointerValue;
  const auto unsupported = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names, fixture.locations, fixture.addressing, fixture.block_label, 2, load);
  if (unsupported.record.has_value() ||
      unsupported.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::MissingPointerValueName) {
    return fail("expected incomplete pointer base to fail closed in Step 4");
  }

  fixture = make_fixture();
  fixture.addressing.accesses[0].result_value_name = fixture.stored_name;
  const auto result_mismatch = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names, fixture.locations, fixture.addressing, fixture.block_label, 2, load);
  if (result_mismatch.record.has_value() ||
      result_mismatch.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::ResultValueMismatch) {
    return fail("expected mismatched load result identity to fail closed");
  }

  fixture = make_fixture();
  const bir::StoreGlobalInst store_mismatch{
      .global_name_id = c4c::LinkNameId{777},
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 16,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .byte_offset = 16,
              .size_bytes = 4,
              .align_bytes = 4,
              .address_space = bir::AddressSpace::Gs,
              .is_volatile = true,
              .base_link_name_id = c4c::LinkNameId{777},
          },
  };
  const auto symbol_mismatch = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      3,
      store_mismatch);
  if (symbol_mismatch.record.has_value() ||
      symbol_mismatch.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::SymbolMismatch) {
    return fail("expected mismatched global symbol identity to fail closed");
  }

  fixture = make_fixture();
  fixture.addressing.accesses[1].stored_value_name = fixture.load_name;
  const bir::StoreGlobalInst store{
      .global_name_id = fixture.global_name,
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 16,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .byte_offset = 16,
              .size_bytes = 4,
              .align_bytes = 4,
              .address_space = bir::AddressSpace::Gs,
              .is_volatile = true,
              .base_link_name_id = fixture.global_name,
          },
  };
  const auto stored_mismatch = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names, fixture.locations, fixture.addressing, fixture.block_label, 3, store);
  if (stored_mismatch.record.has_value() ||
      stored_mismatch.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::StoredValueMismatch) {
    return fail("expected mismatched store value identity to fail closed");
  }

  fixture = make_fixture();
  const bir::LoadLocalInst offset_mismatch_load{
      .result = named_value(bir::TypeKind::I32, "%load"),
      .slot_id = c4c::SlotNameId{5},
      .byte_offset = 8,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .byte_offset = 12,
              .size_bytes = 4,
              .align_bytes = 4,
              .address_space = bir::AddressSpace::Fs,
              .is_volatile = true,
              .base_slot_id = c4c::SlotNameId{5},
          },
  };
  const auto offset_mismatch = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      2,
      offset_mismatch_load);
  if (offset_mismatch.record.has_value() ||
      offset_mismatch.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::AddressFactMismatch ||
      aarch64_codegen::prepared_memory_operand_record_error_name(offset_mismatch.error) !=
          "address_fact_mismatch") {
    return fail("expected BIR/prepared offset mismatch to fail closed");
  }

  fixture = make_fixture();
  const bir::StoreGlobalInst address_space_mismatch_store{
      .global_name_id = fixture.global_name,
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 16,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .byte_offset = 16,
              .size_bytes = 4,
              .align_bytes = 4,
              .address_space = bir::AddressSpace::Default,
              .is_volatile = true,
              .base_link_name_id = fixture.global_name,
          },
  };
  const auto address_space_mismatch = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      3,
      address_space_mismatch_store);
  if (address_space_mismatch.record.has_value() ||
      address_space_mismatch.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::AddressFactMismatch) {
    return fail("expected BIR/prepared address-space mismatch to fail closed");
  }

  fixture = make_fixture();
  const bir::StoreGlobalInst volatility_mismatch_store{
      .global_name_id = fixture.global_name,
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 16,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .byte_offset = 16,
              .size_bytes = 4,
              .align_bytes = 4,
              .address_space = bir::AddressSpace::Gs,
              .is_volatile = false,
              .base_link_name_id = fixture.global_name,
          },
  };
  const auto volatility_mismatch = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      3,
      volatility_mismatch_store);
  if (volatility_mismatch.record.has_value() ||
      volatility_mismatch.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::AddressFactMismatch) {
    return fail("expected BIR/prepared volatility mismatch to fail closed");
  }

  fixture = make_fixture();
  const bir::StoreGlobalInst missing_structured_address_store{
      .global_name_id = fixture.global_name,
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 16,
      .align_bytes = 4,
  };
  const auto missing_structured_address = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      3,
      missing_structured_address_store);
  if (missing_structured_address.record.has_value() ||
      missing_structured_address.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::AddressFactMismatch) {
    return fail("expected missing structured volatility/address-space facts to fail closed");
  }

  fixture = make_fixture();
  fixture.locations.value_homes.pop_back();
  const bir::StoreLocalInst pointer_missing_home_store{
      .slot_id = c4c::SlotNameId{5},
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 24,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
              .base_value = named_value(bir::TypeKind::Ptr, "%ptr"),
              .byte_offset = 24,
              .size_bytes = 8,
              .align_bytes = 8,
              .address_space = bir::AddressSpace::Tls,
          },
  };
  const auto pointer_missing_home = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      4,
      pointer_missing_home_store);
  if (pointer_missing_home.record.has_value() ||
      pointer_missing_home.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::MissingPointerValueHome ||
      aarch64_codegen::prepared_memory_operand_record_error_name(pointer_missing_home.error) !=
          "missing_pointer_value_home") {
    return fail("expected missing pointer value home to fail closed");
  }

  fixture = make_fixture();
  fixture.locations.value_homes.push_back(register_home(
      prepare::PreparedValueId{13}, fixture.function_name, fixture.pointer_name, "x3"));
  const auto pointer_ambiguous_home = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      4,
      pointer_missing_home_store);
  if (pointer_ambiguous_home.record.has_value() ||
      pointer_ambiguous_home.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::AmbiguousPointerValueHome ||
      aarch64_codegen::prepared_memory_operand_record_error_name(pointer_ambiguous_home.error) !=
          "ambiguous_pointer_value_home") {
    return fail("expected ambiguous pointer value home to fail closed");
  }

  fixture = make_fixture();
  const bir::StoreLocalInst pointer_mismatch_store{
      .slot_id = c4c::SlotNameId{5},
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 24,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
              .base_value = named_value(bir::TypeKind::Ptr, "%other.ptr"),
              .byte_offset = 24,
              .size_bytes = 8,
              .align_bytes = 8,
              .address_space = bir::AddressSpace::Tls,
          },
  };
  const auto pointer_mismatch = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      4,
      pointer_mismatch_store);
  if (pointer_mismatch.record.has_value() ||
      pointer_mismatch.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::PointerValueMismatch) {
    return fail("expected BIR/prepared pointer value mismatch to fail closed");
  }

  fixture = make_fixture();
  const bir::LoadGlobalInst string_mismatch_load{
      .result = named_value(bir::TypeKind::I32, "%load"),
      .byte_offset = 4,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::StringConstant,
              .base_name = ".L.other",
              .byte_offset = 4,
              .size_bytes = 8,
              .align_bytes = 8,
              .address_space = bir::AddressSpace::Gs,
              .is_volatile = true,
          },
  };
  const auto string_mismatch = aarch64_codegen::make_prepared_memory_operand_record(
      fixture.names,
      fixture.locations,
      fixture.addressing,
      fixture.block_label,
      5,
      string_mismatch_load);
  if (string_mismatch.record.has_value() ||
      string_mismatch.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::StringIdentityMismatch ||
      aarch64_codegen::prepared_memory_operand_record_error_name(string_mismatch.error) !=
          "string_identity_mismatch") {
    return fail("expected BIR/prepared string identity mismatch to fail closed");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = frame_slot_load_conversion_preserves_prepared_and_bir_facts();
      status != 0) {
    return status;
  }
  if (const int status = global_symbol_store_conversion_preserves_prepared_and_bir_facts();
      status != 0) {
    return status;
  }
  if (const int status = pointer_value_store_conversion_preserves_prepared_and_bir_facts();
      status != 0) {
    return status;
  }
  if (const int status = string_constant_load_conversion_preserves_prepared_and_bir_facts();
      status != 0) {
    return status;
  }
  if (const int status = unsupported_or_mismatched_memory_facts_fail_closed(); status != 0) {
    return status;
  }
  return 0;
}
