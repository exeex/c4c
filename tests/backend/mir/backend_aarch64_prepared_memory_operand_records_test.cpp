#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/instruction.hpp"

#include <iostream>
#include <utility>
#include <variant>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
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

prepare::PreparedStoragePlanValue memory_register_storage(
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
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
  prepare::PreparedStoragePlanFunction storage;
  prepare::PreparedAddressingFunction addressing;
};

struct PreparedAddressFixture {
  prepare::PreparedNameTables names;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId target_label = c4c::kInvalidBlockLabel;
  c4c::ValueNameId direct_result_name = c4c::kInvalidValueName;
  c4c::ValueNameId got_result_name = c4c::kInvalidValueName;
  c4c::ValueNameId tls_result_name = c4c::kInvalidValueName;
  c4c::ValueNameId string_result_name = c4c::kInvalidValueName;
  c4c::ValueNameId label_result_name = c4c::kInvalidValueName;
  c4c::LinkNameId global_name = c4c::kInvalidLinkName;
  c4c::LinkNameId got_name = c4c::kInvalidLinkName;
  c4c::LinkNameId tls_name = c4c::kInvalidLinkName;
  c4c::TextId string_text_name = c4c::kInvalidText;
  prepare::PreparedValueLocationFunction locations;
  prepare::PreparedStoragePlanFunction storage;
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
  fixture.storage = prepare::PreparedStoragePlanFunction{
      .function_name = fixture.function_name,
      .values =
          {
              memory_register_storage(
                  prepare::PreparedValueId{10}, fixture.load_name, "w0"),
              memory_register_storage(
                  prepare::PreparedValueId{11}, fixture.stored_name, "w1"),
              memory_register_storage(
                  prepare::PreparedValueId{12}, fixture.pointer_name, "x2"),
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
                  .inst_index = 6,
                  .stored_value_name = fixture.stored_name,
                  .address_space = bir::AddressSpace::Fs,
                  .is_volatile = true,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                          .frame_slot_id = prepare::PreparedFrameSlotId{21},
                          .byte_offset = 12,
                          .size_bytes = 4,
                          .align_bytes = 4,
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

prepare::PreparedStoragePlanValue register_storage(prepare::PreparedValueId value_id,
                                                   c4c::ValueNameId value_name,
                                                   const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
  };
}

PreparedAddressFixture make_address_fixture() {
  PreparedAddressFixture fixture;
  fixture.function_name = fixture.names.function_names.intern("addr.f");
  fixture.block_label = fixture.names.block_labels.intern("entry");
  fixture.target_label = fixture.names.block_labels.intern("target");
  fixture.direct_result_name = fixture.names.value_names.intern("%direct.addr");
  fixture.got_result_name = fixture.names.value_names.intern("%got.addr");
  fixture.tls_result_name = fixture.names.value_names.intern("%tls.addr");
  fixture.string_result_name = fixture.names.value_names.intern("%string.addr");
  fixture.label_result_name = fixture.names.value_names.intern("%label.addr");
  fixture.global_name = fixture.names.link_names.intern("g.direct");
  fixture.got_name = fixture.names.link_names.intern("g.got");
  fixture.tls_name = fixture.names.link_names.intern("g.tls");
  fixture.string_text_name = fixture.names.texts.intern(".L.str0");
  fixture.locations = prepare::PreparedValueLocationFunction{
      .function_name = fixture.function_name,
      .value_homes =
          {
              register_home(prepare::PreparedValueId{20},
                            fixture.function_name,
                            fixture.direct_result_name,
                            "x3"),
              register_home(prepare::PreparedValueId{21},
                            fixture.function_name,
                            fixture.got_result_name,
                            "x4"),
              register_home(prepare::PreparedValueId{22},
                            fixture.function_name,
                            fixture.tls_result_name,
                            "x5"),
              register_home(prepare::PreparedValueId{23},
                            fixture.function_name,
                            fixture.string_result_name,
                            "x6"),
              register_home(prepare::PreparedValueId{24},
                            fixture.function_name,
                            fixture.label_result_name,
                            "x7"),
          },
  };
  fixture.storage = prepare::PreparedStoragePlanFunction{
      .function_name = fixture.function_name,
      .values =
          {
              register_storage(prepare::PreparedValueId{20}, fixture.direct_result_name, "x3"),
              register_storage(prepare::PreparedValueId{21}, fixture.got_result_name, "x4"),
              register_storage(prepare::PreparedValueId{22}, fixture.tls_result_name, "x5"),
              register_storage(prepare::PreparedValueId{23}, fixture.string_result_name, "x6"),
              register_storage(prepare::PreparedValueId{24}, fixture.label_result_name, "x7"),
          },
  };
  fixture.addressing = prepare::PreparedAddressingFunction{
      .function_name = fixture.function_name,
      .address_materializations =
          {
              prepare::PreparedAddressMaterialization{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 0,
                  .kind = prepare::PreparedAddressMaterializationKind::DirectGlobal,
                  .result_value_name = fixture.direct_result_name,
                  .symbol_name = fixture.global_name,
                  .address_materialization_policy =
                      bir::GlobalAddressMaterializationPolicy::Direct,
                  .byte_offset = 16,
              },
              prepare::PreparedAddressMaterialization{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 1,
                  .kind = prepare::PreparedAddressMaterializationKind::GotGlobal,
                  .result_value_name = fixture.got_result_name,
                  .symbol_name = fixture.got_name,
                  .address_materialization_policy =
                      bir::GlobalAddressMaterializationPolicy::GotRequired,
                  .byte_offset = 32,
              },
              prepare::PreparedAddressMaterialization{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 2,
                  .kind = prepare::PreparedAddressMaterializationKind::TlsGlobal,
                  .result_value_name = fixture.tls_result_name,
                  .symbol_name = fixture.tls_name,
                  .address_materialization_policy =
                      bir::GlobalAddressMaterializationPolicy::Direct,
                  .address_space = bir::AddressSpace::Tls,
                  .is_thread_local = true,
                  .has_tls_address_space = true,
                  .tls_model =
                      prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative,
                  .tls_thread_pointer_register =
                      prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0,
                  .tls_high_relocation = prepare::PreparedTlsRelocationKind::Aarch64TprelHi12,
                  .tls_low_relocation = prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc,
              },
              prepare::PreparedAddressMaterialization{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 3,
                  .kind = prepare::PreparedAddressMaterializationKind::StringConstant,
                  .result_value_name = fixture.string_result_name,
                  .text_name = fixture.string_text_name,
                  .byte_offset = 8,
                  .address_space = bir::AddressSpace::Gs,
              },
              prepare::PreparedAddressMaterialization{
                  .function_name = fixture.function_name,
                  .block_label = fixture.block_label,
                  .inst_index = 4,
                  .kind = prepare::PreparedAddressMaterializationKind::Label,
                  .result_value_name = fixture.label_result_name,
                  .target_label = fixture.target_label,
                  .byte_offset = 24,
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

  const auto selected =
      aarch64_codegen::make_prepared_frame_slot_load_memory_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          fixture.addressing,
          fixture.block_label,
          2,
          load);
  if (!selected.record.has_value() ||
      selected.error != aarch64_codegen::PreparedMemoryOperandRecordError::None) {
    return fail("expected frame-slot load machine record to select");
  }
  const auto& load_record = *selected.record;
  if (load_record.memory_kind != aarch64_codegen::MemoryInstructionKind::Load ||
      load_record.address.frame_slot_id != prepare::PreparedFrameSlotId{20} ||
      load_record.result_value_id != prepare::PreparedValueId{10} ||
      load_record.result_value_name != fixture.load_name ||
      !load_record.result_register.has_value() ||
      load_record.result_register->value_id != prepare::PreparedValueId{10} ||
      load_record.result_register->value_name != fixture.load_name ||
      load_record.result_register->occupied_registers.empty() ||
      load_record.result_register->occupied_registers.front() != "w0") {
    return fail("expected frame-slot load machine record to preserve result register");
  }

  const auto instruction = aarch64_codegen::make_memory_instruction(load_record);
  if (instruction.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.opcode != aarch64_codegen::MachineOpcode::Load ||
      instruction.side_effects.empty() ||
      instruction.side_effects.front() != aarch64_codegen::MachineSideEffectKind::MemoryRead ||
      instruction.defs.empty() ||
      instruction.defs.front().kind != aarch64_codegen::MachineEffectResourceKind::Register ||
      !instruction.defs.front().reg.has_value()) {
    return fail("expected selected frame-slot load instruction to define result register");
  }

  fixture.storage.values.clear();
  const auto missing_storage =
      aarch64_codegen::make_prepared_frame_slot_load_memory_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          fixture.addressing,
          fixture.block_label,
          2,
          load);
  if (missing_storage.record.has_value() ||
      missing_storage.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::MissingResultStorage ||
      aarch64_codegen::prepared_memory_operand_record_error_name(missing_storage.error) !=
          "missing_result_storage") {
    return fail("expected missing load destination storage to fail closed");
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

  const auto selected = aarch64_codegen::make_prepared_store_memory_instruction_record(
      fixture.names, fixture.locations, fixture.storage, fixture.addressing, fixture.block_label, 4, store);
  if (!selected.record.has_value() ||
      selected.error != aarch64_codegen::PreparedMemoryOperandRecordError::None) {
    return fail("expected pointer-value store machine record to select");
  }
  const auto& store_record = *selected.record;
  if (store_record.memory_kind != aarch64_codegen::MemoryInstructionKind::Store ||
      store_record.address.base_kind != aarch64_codegen::MemoryBaseKind::PointerValue ||
      !store_record.value.has_value() ||
      store_record.value->kind != aarch64_codegen::OperandKind::Register) {
    return fail("expected pointer-value store to carry structured register source");
  }
  const auto* stored_register =
      std::get_if<aarch64_codegen::RegisterOperand>(&store_record.value->payload);
  if (stored_register == nullptr ||
      stored_register->value_id != prepare::PreparedValueId{11} ||
      stored_register->value_name != fixture.stored_name ||
      stored_register->occupied_registers.empty() ||
      stored_register->occupied_registers.front() != "w1") {
    return fail("expected pointer-value store source register authority");
  }
  if (!store_record.address.base_register.has_value() ||
      store_record.address.base_register->value_id != prepare::PreparedValueId{12} ||
      store_record.address.base_register->value_name != fixture.pointer_name ||
      store_record.address.base_register->occupied_registers.empty() ||
      store_record.address.base_register->occupied_registers.front() != "x2") {
    return fail("expected pointer-value store base register authority");
  }
  const auto instruction = aarch64_codegen::make_memory_instruction(store_record);
  if (instruction.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.opcode != aarch64_codegen::MachineOpcode::Store ||
      instruction.side_effects.empty() ||
      instruction.side_effects.front() != aarch64_codegen::MachineSideEffectKind::MemoryWrite ||
      instruction.uses.size() < 2 ||
      instruction.uses.back().kind != aarch64_codegen::MachineEffectResourceKind::Register) {
    return fail("expected selected pointer-value store instruction to use source register");
  }
  return 0;
}

int frame_slot_store_conversion_selects_structured_register_source() {
  auto fixture = make_fixture();
  const bir::StoreLocalInst store{
      .slot_id = c4c::SlotNameId{5},
      .value = named_value(bir::TypeKind::I32, "%stored"),
      .byte_offset = 12,
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

  const auto selected = aarch64_codegen::make_prepared_store_memory_instruction_record(
      fixture.names, fixture.locations, fixture.storage, fixture.addressing, fixture.block_label, 6, store);
  if (!selected.record.has_value() ||
      selected.error != aarch64_codegen::PreparedMemoryOperandRecordError::None) {
    return fail("expected frame-slot store machine record to select");
  }
  const auto& store_record = *selected.record;
  if (store_record.address.base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      store_record.address.frame_slot_id != prepare::PreparedFrameSlotId{21} ||
      store_record.address.byte_offset != 12 || store_record.address.size_bytes != 4 ||
      store_record.address.align_bytes != 4 ||
      store_record.address.address_space != bir::AddressSpace::Fs ||
      !store_record.address.is_volatile ||
      !store_record.value.has_value() ||
      store_record.value->kind != aarch64_codegen::OperandKind::Register) {
    return fail("expected frame-slot store record to preserve address and source facts");
  }

  fixture.storage.values.clear();
  const auto missing_storage = aarch64_codegen::make_prepared_store_memory_instruction_record(
      fixture.names, fixture.locations, fixture.storage, fixture.addressing, fixture.block_label, 6, store);
  if (missing_storage.record.has_value() ||
      missing_storage.error !=
          aarch64_codegen::PreparedMemoryOperandRecordError::MissingStoredStorage ||
      aarch64_codegen::prepared_memory_operand_record_error_name(missing_storage.error) !=
          "missing_stored_storage") {
    return fail("expected missing stored-source storage to fail closed");
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

int address_materialization_records_preserve_prepared_carrier_facts() {
  auto fixture = make_address_fixture();
  const auto direct =
      aarch64_codegen::make_prepared_address_materialization_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          fixture.addressing,
          fixture.block_label,
          0);
  if (!direct.record.has_value() ||
      direct.error != aarch64_codegen::PreparedAddressMaterializationRecordError::None) {
    return fail("expected direct global address materialization record to select");
  }
  const auto direct_instruction =
      aarch64_codegen::make_address_materialization_instruction(*direct.record);
  if (direct_instruction.opcode != aarch64_codegen::MachineOpcode::AddressMaterialization ||
      direct_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      !std::holds_alternative<aarch64_codegen::AddressMaterializationRecord>(
          direct_instruction.payload)) {
    return fail("expected direct global address materialization to become selected machine node");
  }
  const auto& direct_record =
      std::get<aarch64_codegen::AddressMaterializationRecord>(direct_instruction.payload);
  if (direct_record.kind != aarch64_codegen::AddressMaterializationKind::DirectPageLow12 ||
      direct_record.prepared_kind != prepare::PreparedAddressMaterializationKind::DirectGlobal ||
      direct_record.result_value_id != prepare::PreparedValueId{20} ||
      direct_record.result_value_name != fixture.direct_result_name ||
      direct_record.result_home_kind != prepare::PreparedValueHomeKind::Register ||
      !direct_record.result_register.has_value() ||
      direct_record.result_register->value_id != prepare::PreparedValueId{20} ||
      direct_record.result_register->value_name != fixture.direct_result_name ||
      direct_record.symbol_name != fixture.global_name ||
      direct_record.text_name.has_value() ||
      direct_record.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      direct_record.byte_offset != 16 ||
      direct_record.source_materialization == nullptr) {
    return fail("expected direct global record to preserve result register and relocation identity");
  }

  const auto got = aarch64_codegen::make_prepared_address_materialization_instruction_record(
      fixture.names, fixture.locations, fixture.storage, fixture.addressing, fixture.block_label, 1);
  if (!got.record.has_value() ||
      got.error != aarch64_codegen::PreparedAddressMaterializationRecordError::None ||
      got.record->kind != aarch64_codegen::AddressMaterializationKind::GotPageLow12 ||
      got.record->prepared_kind != prepare::PreparedAddressMaterializationKind::GotGlobal ||
      got.record->symbol_name != fixture.got_name ||
      got.record->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired ||
      got.record->byte_offset != 32 ||
      got.record->result_value_id != prepare::PreparedValueId{21} ||
      got.record->result_value_name != fixture.got_result_name ||
      !got.record->result_register.has_value() ||
      got.record->result_register->value_name != fixture.got_result_name ||
      got.record->is_thread_local ||
      got.record->has_tls_address_space) {
    return fail("expected GOT global address materialization to preserve GOT policy facts");
  }
  const auto got_instruction =
      aarch64_codegen::make_address_materialization_instruction(*got.record);
  if (got_instruction.opcode != aarch64_codegen::MachineOpcode::AddressMaterialization ||
      got_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      got_instruction.selection.diagnostic != std::string_view{} ||
      got_instruction.operands.size() != 2 ||
      !std::holds_alternative<aarch64_codegen::AddressMaterializationRecord>(
          got_instruction.payload)) {
    return fail("expected GOT global address materialization to become selected machine node");
  }

  const auto tls = aarch64_codegen::make_prepared_address_materialization_instruction_record(
      fixture.names, fixture.locations, fixture.storage, fixture.addressing, fixture.block_label, 2);
  if (!tls.record.has_value() ||
      tls.record->kind != aarch64_codegen::AddressMaterializationKind::TlsRelative ||
      tls.record->symbol_name != fixture.tls_name ||
      tls.record->address_space != bir::AddressSpace::Tls ||
      !tls.record->is_thread_local ||
      !tls.record->has_tls_address_space ||
      tls.record->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      tls.record->tls_model !=
          prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
      tls.record->tls_thread_pointer_register !=
          prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0 ||
      tls.record->tls_high_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
      tls.record->tls_low_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc ||
      tls.record->result_value_id != prepare::PreparedValueId{22}) {
    return fail("expected TLS global address materialization to preserve TLS and result facts");
  }

  const auto string =
      aarch64_codegen::make_prepared_address_materialization_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          fixture.addressing,
          fixture.block_label,
          3);
  if (!string.record.has_value() ||
      string.record->kind != aarch64_codegen::AddressMaterializationKind::StringConstant ||
      string.record->text_name != fixture.string_text_name ||
      string.record->symbol_name.has_value() ||
      string.record->byte_offset != 8 ||
      string.record->address_space != bir::AddressSpace::Gs ||
      string.record->result_value_id != prepare::PreparedValueId{23}) {
    return fail("expected string address materialization to preserve text and result facts");
  }
  const auto label = aarch64_codegen::make_prepared_address_materialization_instruction_record(
      fixture.names, fixture.locations, fixture.storage, fixture.addressing, fixture.block_label, 4);
  if (!label.record.has_value() ||
      label.record->kind != aarch64_codegen::AddressMaterializationKind::LabelPageLow12 ||
      label.record->prepared_kind != prepare::PreparedAddressMaterializationKind::Label ||
      label.record->target_label != fixture.target_label ||
      label.record->target_label_name != "target" ||
      label.record->symbol_name.has_value() ||
      label.record->text_name.has_value() ||
      label.record->byte_offset != 24 ||
      label.record->result_value_id != prepare::PreparedValueId{24}) {
    return fail("expected label address materialization to preserve target label and result facts");
  }
  const auto label_instruction =
      aarch64_codegen::make_address_materialization_instruction(*label.record);
  if (label_instruction.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      label_instruction.operands.size() != 2 ||
      !std::holds_alternative<aarch64_codegen::AddressMaterializationRecord>(
          label_instruction.payload)) {
    return fail("expected label address materialization to become selected machine node");
  }

  return 0;
}

int address_materialization_policy_and_identity_fail_closed() {
  auto fixture = make_address_fixture();
  fixture.addressing.address_materializations[1].address_materialization_policy =
      bir::GlobalAddressMaterializationPolicy::Unspecified;
  const auto got = aarch64_codegen::make_prepared_address_materialization_instruction_record(
      fixture.names, fixture.locations, fixture.storage, fixture.addressing, fixture.block_label, 1);
  if (got.record.has_value() ||
      got.error !=
          aarch64_codegen::PreparedAddressMaterializationRecordError::
              MissingAddressMaterializationPolicy ||
      aarch64_codegen::prepared_address_materialization_record_error_name(got.error) !=
          "missing_address_materialization_policy") {
    return fail("expected GOT address materialization without explicit policy to fail closed");
  }

  fixture = make_address_fixture();
  fixture.addressing.address_materializations[1].address_materialization_policy =
      bir::GlobalAddressMaterializationPolicy::Direct;
  const auto got_policy_mismatch =
      aarch64_codegen::make_prepared_address_materialization_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          fixture.addressing,
          fixture.block_label,
          1);
  if (got_policy_mismatch.record.has_value() ||
      got_policy_mismatch.error !=
          aarch64_codegen::PreparedAddressMaterializationRecordError::
              AddressMaterializationPolicyMismatch ||
      aarch64_codegen::prepared_address_materialization_record_error_name(
          got_policy_mismatch.error) != "address_materialization_policy_mismatch") {
    return fail("expected GOT address materialization with direct policy to fail closed");
  }

  fixture = make_address_fixture();
  fixture.addressing.address_materializations.front().symbol_name = std::nullopt;
  const auto missing_symbol =
      aarch64_codegen::make_prepared_address_materialization_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          fixture.addressing,
          fixture.block_label,
          0);
  if (missing_symbol.record.has_value() ||
      missing_symbol.error !=
          aarch64_codegen::PreparedAddressMaterializationRecordError::MissingSymbolIdentity ||
      aarch64_codegen::prepared_address_materialization_record_error_name(
          missing_symbol.error) != "missing_symbol_identity") {
    return fail("expected missing global address symbol identity to fail closed");
  }

  fixture = make_address_fixture();
  fixture.addressing.address_materializations[2].tls_high_relocation =
      prepare::PreparedTlsRelocationKind::None;
  const auto missing_tls_fact =
      aarch64_codegen::make_prepared_address_materialization_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          fixture.addressing,
          fixture.block_label,
          2);
  if (missing_tls_fact.record.has_value() ||
      missing_tls_fact.error !=
          aarch64_codegen::PreparedAddressMaterializationRecordError::
              MissingTlsMaterializationFacts ||
      aarch64_codegen::prepared_address_materialization_record_error_name(
          missing_tls_fact.error) != "missing_tls_materialization_facts") {
    return fail("expected TLS address materialization without relocation facts to fail closed");
  }

  fixture = make_address_fixture();
  fixture.addressing.address_materializations[4].target_label = std::nullopt;
  const auto missing_label =
      aarch64_codegen::make_prepared_address_materialization_instruction_record(
          fixture.names,
          fixture.locations,
          fixture.storage,
          fixture.addressing,
          fixture.block_label,
          4);
  if (missing_label.record.has_value() ||
      missing_label.error !=
          aarch64_codegen::PreparedAddressMaterializationRecordError::MissingLabelIdentity ||
      aarch64_codegen::prepared_address_materialization_record_error_name(missing_label.error) !=
          "missing_label_identity") {
    return fail("expected missing label address identity to fail closed");
  }

  return 0;
}

int dispatch_selects_address_materialization_from_prepared_carrier() {
  auto fixture = make_address_fixture();
  prepare::PreparedBirModule prepared;
  prepared.names = fixture.names;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.addressing.functions.push_back(fixture.addressing);
  prepared.value_locations.functions.push_back(fixture.locations);
  prepared.storage_plans.functions.push_back(fixture.storage);

  bir::Function function;
  function.name = "addr.f";
  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = named_value(bir::TypeKind::Ptr, "%direct.addr"),
      .operand = named_value(bir::TypeKind::Ptr, "%not.a.symbol"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = named_value(bir::TypeKind::Ptr, "%got.addr"),
      .operand = named_value(bir::TypeKind::Ptr, "%not.a.got.symbol"),
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = fixture.function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = fixture.block_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
  });

  aarch64_module::FunctionLoweringContext function_context{
      .prepared = &prepared,
      .target_profile = &prepared.target_profile,
      .control_flow = &prepared.control_flow.functions.front(),
      .bir_function = &prepared.module.functions.front(),
      .value_locations = &prepared.value_locations.functions.front(),
      .storage_plan = &prepared.storage_plans.functions.front(),
  };
  const auto block_context = aarch64_codegen::make_block_lowering_context(
      function_context, prepared.control_flow.functions.front().blocks.front(), 0);
  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result = aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);
  if (!diagnostics.empty() || result.visited_operations != 2 ||
      block.instructions.size() != 3) {
    return fail("expected dispatch to select prepared address materialization nodes");
  }
  const auto* address =
      std::get_if<aarch64_codegen::AddressMaterializationRecord>(
          &block.instructions.front().target.payload);
  if (address == nullptr ||
      address->symbol_name != fixture.global_name ||
      address->result_value_id != prepare::PreparedValueId{20} ||
      address->result_value_name != fixture.direct_result_name ||
      address->kind != aarch64_codegen::AddressMaterializationKind::DirectPageLow12) {
    return fail("expected dispatch to consume prepared carrier rather than rendered BIR names");
  }
  const auto* got_address =
      std::get_if<aarch64_codegen::AddressMaterializationRecord>(
          &block.instructions[1].target.payload);
  if (got_address == nullptr ||
      got_address->symbol_name != fixture.got_name ||
      got_address->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired ||
      got_address->result_value_id != prepare::PreparedValueId{21} ||
      got_address->result_value_name != fixture.got_result_name ||
      got_address->kind != aarch64_codegen::AddressMaterializationKind::GotPageLow12) {
    return fail("expected dispatch to consume prepared GOT carrier without name inference");
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
  if (const int status = frame_slot_store_conversion_selects_structured_register_source();
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
  if (const int status = address_materialization_records_preserve_prepared_carrier_facts();
      status != 0) {
    return status;
  }
  if (const int status = address_materialization_policy_and_identity_fail_closed();
      status != 0) {
    return status;
  }
  if (const int status = dispatch_selects_address_materialization_from_prepared_carrier();
      status != 0) {
    return status;
  }
  return 0;
}
