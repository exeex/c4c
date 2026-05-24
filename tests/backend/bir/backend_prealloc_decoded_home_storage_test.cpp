#include "src/backend/prealloc/decoded_home_storage.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

namespace prepare = c4c::backend::prepare;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

prepare::PreparedRegisterPlacement sample_gpr_placement() {
  return prepare::PreparedRegisterPlacement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
      .slot_index = 2,
      .contiguous_width = 1,
  };
}

int verify_regalloc_decoding() {
  const auto function_name = static_cast<c4c::FunctionNameId>(11);
  const auto value_name = static_cast<c4c::ValueNameId>(21);
  const auto placement = sample_gpr_placement();
  prepare::PreparedRegallocFunction regalloc{
      .function_name = function_name,
      .values = {
          prepare::PreparedRegallocValue{
              .value_id = 1,
              .function_name = function_name,
              .value_name = value_name,
              .assigned_register = prepare::PreparedPhysicalRegisterAssignment{
                  .reg_class = prepare::PreparedRegisterClass::General,
                  .register_name = "r2",
                  .contiguous_width = 1,
                  .occupied_register_names = {"r2"},
                  .placement = placement,
              },
          },
          prepare::PreparedRegallocValue{
              .value_id = 2,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(22),
              .assigned_stack_slot = prepare::PreparedStackSlotAssignment{
                  .slot_id = 5,
                  .offset_bytes = 40,
                  .size_bytes = 8,
                  .align_bytes = 8,
              },
          },
          prepare::PreparedRegallocValue{
              .value_id = 3,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(23),
              .assigned_register = prepare::PreparedPhysicalRegisterAssignment{
                  .reg_class = prepare::PreparedRegisterClass::Float,
                  .register_name = "f0",
                  .contiguous_width = 1,
                  .occupied_register_names = {"f0"},
              },
          },
      },
  };

  const auto reg = prepare::decode_prepared_regalloc_assignment(&regalloc, 1);
  if (!expect(prepare::prepared_decoded_home_storage_available(reg),
              "regalloc register assignment should decode as available") ||
      !expect(reg.source == prepare::PreparedDecodedHomeStorageSource::RegallocAssignment,
              "regalloc register source mismatch") ||
      !expect(reg.kind == prepare::PreparedDecodedHomeStorageKind::Register,
              "regalloc register kind mismatch") ||
      !expect(reg.register_placement == placement,
              "regalloc register placement was not preserved") ||
      !expect(reg.register_class == prepare::PreparedRegisterClass::General,
              "regalloc register class was not preserved")) {
    return 1;
  }

  const auto stack = prepare::decode_prepared_regalloc_assignment(&regalloc, 2);
  if (!expect(stack.kind == prepare::PreparedDecodedHomeStorageKind::FrameSlot,
              "regalloc stack assignment kind mismatch") ||
      !expect(stack.slot_id == 5 && stack.stack_offset_bytes == 40,
              "regalloc stack slot facts were not preserved") ||
      !expect(stack.size_bytes == 8 && stack.align_bytes == 8,
              "regalloc stack size/align facts were not preserved")) {
    return 1;
  }

  const auto missing_placement = prepare::decode_prepared_regalloc_assignment(&regalloc, 3);
  if (!expect(missing_placement.kind == prepare::PreparedDecodedHomeStorageKind::Register,
              "regalloc missing-placement value should still decode as register") ||
      !expect(missing_placement.status ==
                  prepare::PreparedDecodedHomeStorageStatus::MissingRegisterPlacement,
              "regalloc missing-placement status mismatch")) {
    return 1;
  }

  return 0;
}

int verify_storage_plan_decoding() {
  const auto function_name = static_cast<c4c::FunctionNameId>(12);
  const auto placement = sample_gpr_placement();
  prepare::PreparedStoragePlanFunction storage_plan{
      .function_name = function_name,
      .values = {
          prepare::PreparedStoragePlanValue{
              .value_id = 10,
              .value_name = static_cast<c4c::ValueNameId>(30),
              .encoding = prepare::PreparedStorageEncodingKind::Register,
              .bank = prepare::PreparedRegisterBank::Gpr,
              .contiguous_width = 1,
              .register_name = "r2",
              .occupied_register_names = {"r2"},
              .register_placement = placement,
          },
          prepare::PreparedStoragePlanValue{
              .value_id = 11,
              .value_name = static_cast<c4c::ValueNameId>(31),
              .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
              .slot_id = 7,
              .stack_offset_bytes = 64,
              .spill_slot_placement = prepare::PreparedSpillSlotPlacement{
                  .slot_id = 7,
                  .offset_bytes = 64,
              },
          },
          prepare::PreparedStoragePlanValue{
              .value_id = 12,
              .value_name = static_cast<c4c::ValueNameId>(32),
              .encoding = prepare::PreparedStorageEncodingKind::Immediate,
              .immediate_i32 = -9,
          },
          prepare::PreparedStoragePlanValue{
              .value_id = 13,
              .value_name = static_cast<c4c::ValueNameId>(33),
              .encoding = prepare::PreparedStorageEncodingKind::SymbolAddress,
              .symbol_name = static_cast<c4c::LinkNameId>(44),
          },
          prepare::PreparedStoragePlanValue{
              .value_id = 14,
              .value_name = static_cast<c4c::ValueNameId>(34),
              .encoding = prepare::PreparedStorageEncodingKind::ComputedAddress,
          },
          prepare::PreparedStoragePlanValue{
              .value_id = 15,
              .value_name = static_cast<c4c::ValueNameId>(35),
              .encoding = prepare::PreparedStorageEncodingKind::Immediate,
          },
      },
  };

  const auto reg = prepare::decode_prepared_storage_plan_value(&storage_plan, 10);
  if (!expect(reg.kind == prepare::PreparedDecodedHomeStorageKind::Register,
              "storage-plan register kind mismatch") ||
      !expect(reg.register_placement == placement,
              "storage-plan register placement was not preserved")) {
    return 1;
  }

  const auto frame = prepare::decode_prepared_storage_plan_value(&storage_plan, 11);
  if (!expect(frame.kind == prepare::PreparedDecodedHomeStorageKind::FrameSlot,
              "storage-plan frame-slot kind mismatch") ||
      !expect(frame.spill_slot_placement.has_value(),
              "storage-plan spill-slot placement was not preserved")) {
    return 1;
  }

  const auto immediate = prepare::decode_prepared_storage_plan_value(&storage_plan, 12);
  const auto symbol = prepare::decode_prepared_storage_plan_value(&storage_plan, 13);
  const auto computed = prepare::decode_prepared_storage_plan_value(&storage_plan, 14);
  const auto missing_immediate =
      prepare::decode_prepared_storage_plan_value(&storage_plan, 15);
  if (!expect(immediate.kind == prepare::PreparedDecodedHomeStorageKind::ImmediateI32 &&
                  immediate.immediate_i32 == -9,
              "storage-plan immediate facts were not preserved") ||
      !expect(symbol.kind == prepare::PreparedDecodedHomeStorageKind::SymbolAddress &&
                  symbol.symbol_name == static_cast<c4c::LinkNameId>(44),
              "storage-plan symbol facts were not preserved") ||
      !expect(computed.kind == prepare::PreparedDecodedHomeStorageKind::ComputedAddress &&
                  computed.status ==
                      prepare::PreparedDecodedHomeStorageStatus::UnsupportedStorageEncoding,
              "storage-plan computed address should decode as unsupported") ||
      !expect(missing_immediate.status ==
                  prepare::PreparedDecodedHomeStorageStatus::MissingImmediatePayload,
              "storage-plan missing immediate status mismatch")) {
    return 1;
  }

  return 0;
}

int verify_value_home_and_combined_decoding() {
  const auto function_name = static_cast<c4c::FunctionNameId>(13);
  prepare::PreparedValueLocationFunction locations{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 20,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(50),
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = 8,
              .offset_bytes = 72,
              .size_bytes = 4,
              .align_bytes = 4,
          },
          prepare::PreparedValueHome{
              .value_id = 21,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(51),
              .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
              .immediate_i32 = 123,
          },
          prepare::PreparedValueHome{
              .value_id = 22,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(52),
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = "diagnostic-only",
          },
          prepare::PreparedValueHome{
              .value_id = 23,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(53),
              .kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset,
              .pointer_base_value_name = static_cast<c4c::ValueNameId>(54),
              .pointer_byte_delta = 16,
          },
          prepare::PreparedValueHome{
              .value_id = 24,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(55),
              .kind = prepare::PreparedValueHomeKind::StackSlot,
          },
      },
  };
  const auto lookups = prepare::make_prepared_value_home_lookups(&locations);

  const auto stack = prepare::decode_prepared_value_home(&locations, &lookups, 20);
  const auto immediate = prepare::decode_prepared_value_home(&locations, &lookups, 21);
  const auto reg = prepare::decode_prepared_value_home(&locations, &lookups, 22);
  const auto pointer = prepare::decode_prepared_value_home(&locations, &lookups, 23);
  const auto missing_slot = prepare::decode_prepared_value_home(&locations, &lookups, 24);
  const auto missing = prepare::decode_prepared_value_home(&locations, &lookups, 999);
  if (!expect(stack.kind == prepare::PreparedDecodedHomeStorageKind::FrameSlot &&
                  stack.slot_id == 8 && stack.stack_offset_bytes == 72,
              "value-home stack facts were not preserved") ||
      !expect(immediate.kind == prepare::PreparedDecodedHomeStorageKind::ImmediateI32 &&
                  immediate.immediate_i32 == 123,
              "value-home immediate facts were not preserved") ||
      !expect(reg.kind == prepare::PreparedDecodedHomeStorageKind::Register &&
                  reg.status ==
                      prepare::PreparedDecodedHomeStorageStatus::MissingRegisterPlacement,
              "value-home register should require typed register placement") ||
      !expect(pointer.kind ==
                  prepare::PreparedDecodedHomeStorageKind::PointerBasePlusOffset &&
                  pointer.status ==
                      prepare::PreparedDecodedHomeStorageStatus::UnsupportedValueHomeKind,
              "value-home pointer base plus offset should decode as unsupported") ||
      !expect(missing_slot.status ==
                  prepare::PreparedDecodedHomeStorageStatus::MissingFrameSlot,
              "value-home missing slot status mismatch") ||
      !expect(missing.status == prepare::PreparedDecodedHomeStorageStatus::MissingAuthority,
              "missing value-home should report no authority")) {
    return 1;
  }

  const auto placement = sample_gpr_placement();
  prepare::PreparedRegallocFunction regalloc{
      .function_name = function_name,
      .values = {
          prepare::PreparedRegallocValue{
              .value_id = 20,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(50),
              .assigned_register = prepare::PreparedPhysicalRegisterAssignment{
                  .reg_class = prepare::PreparedRegisterClass::General,
                  .register_name = "r2",
                  .contiguous_width = 1,
                  .occupied_register_names = {"r2"},
                  .placement = placement,
              },
          },
      },
  };
  const auto combined = prepare::decode_prepared_home_storage(
      prepare::PreparedHomeStorageDecodeInputs{
          .regalloc = &regalloc,
          .value_locations = &locations,
          .value_home_lookups = &lookups,
      },
      20);
  if (!expect(combined.source ==
                  prepare::PreparedDecodedHomeStorageSource::RegallocAssignment,
              "combined decoder should prefer regalloc assignment authority") ||
      !expect(combined.kind == prepare::PreparedDecodedHomeStorageKind::Register,
              "combined decoder should return regalloc register facts")) {
    return 1;
  }

  prepare::PreparedRegallocFunction empty_regalloc_authority{
      .function_name = function_name,
      .values = {
          prepare::PreparedRegallocValue{
              .value_id = 21,
              .function_name = function_name,
              .value_name = static_cast<c4c::ValueNameId>(51),
          },
      },
  };
  const auto empty_regalloc_combined = prepare::decode_prepared_home_storage(
      prepare::PreparedHomeStorageDecodeInputs{
          .regalloc = &empty_regalloc_authority,
          .value_locations = &locations,
          .value_home_lookups = &lookups,
      },
      21);
  if (!expect(empty_regalloc_combined.source ==
                  prepare::PreparedDecodedHomeStorageSource::RegallocAssignment,
              "combined decoder should not bypass a present empty regalloc value") ||
      !expect(empty_regalloc_combined.status ==
                  prepare::PreparedDecodedHomeStorageStatus::MissingAuthority,
              "present empty regalloc value should remain the authority result") ||
      !expect(empty_regalloc_combined.kind == prepare::PreparedDecodedHomeStorageKind::None,
              "present empty regalloc value should not fall through to value-home immediate")) {
    return 1;
  }

  prepare::PreparedStoragePlanFunction none_storage_authority{
      .function_name = function_name,
      .values = {
          prepare::PreparedStoragePlanValue{
              .value_id = 21,
              .value_name = static_cast<c4c::ValueNameId>(51),
              .encoding = prepare::PreparedStorageEncodingKind::None,
          },
      },
  };
  const auto none_storage_combined = prepare::decode_prepared_home_storage(
      prepare::PreparedHomeStorageDecodeInputs{
          .storage_plan = &none_storage_authority,
          .value_locations = &locations,
          .value_home_lookups = &lookups,
      },
      21);
  if (!expect(none_storage_combined.source ==
                  prepare::PreparedDecodedHomeStorageSource::StoragePlan,
              "combined decoder should not bypass a present empty storage-plan value") ||
      !expect(none_storage_combined.status ==
                  prepare::PreparedDecodedHomeStorageStatus::MissingAuthority,
              "present empty storage-plan value should remain the authority result") ||
      !expect(none_storage_combined.kind == prepare::PreparedDecodedHomeStorageKind::None,
              "present empty storage-plan value should not fall through to value-home immediate")) {
    return 1;
  }

  const auto true_fallback_combined = prepare::decode_prepared_home_storage(
      prepare::PreparedHomeStorageDecodeInputs{
          .regalloc = &empty_regalloc_authority,
          .storage_plan = &none_storage_authority,
          .value_locations = &locations,
          .value_home_lookups = &lookups,
      },
      20);
  if (!expect(true_fallback_combined.source ==
                  prepare::PreparedDecodedHomeStorageSource::ValueHome,
              "combined decoder should still fall through when higher authorities have no record") ||
      !expect(true_fallback_combined.kind ==
                  prepare::PreparedDecodedHomeStorageKind::FrameSlot,
              "true no-record fallback should decode the value-home record")) {
    return 1;
  }

  return 0;
}

int verify_diagnostic_builders() {
  const auto missing = prepare::build_prepared_decoded_home_storage_diagnostic(
      prepare::PreparedDecodedHomeStorage{
          .source = prepare::PreparedDecodedHomeStorageSource::None,
          .kind = prepare::PreparedDecodedHomeStorageKind::None,
          .status = prepare::PreparedDecodedHomeStorageStatus::MissingAuthority,
          .value_id = 900,
      });
  if (!expect(missing.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      MissingValueAuthority,
              "missing authority diagnostic category mismatch") ||
      !expect(missing.value_id == 900,
              "missing authority diagnostic should preserve value id") ||
      !expect(missing.message == "no typed prepared authority exists for value operand",
              "missing authority diagnostic message mismatch")) {
    return 1;
  }

  const auto regalloc_missing_placement =
      prepare::build_prepared_decoded_home_storage_diagnostic(
          prepare::PreparedDecodedHomeStorage{
              .source = prepare::PreparedDecodedHomeStorageSource::RegallocAssignment,
              .kind = prepare::PreparedDecodedHomeStorageKind::Register,
              .status =
                  prepare::PreparedDecodedHomeStorageStatus::MissingRegisterPlacement,
              .function_name = static_cast<c4c::FunctionNameId>(91),
              .value_id = 901,
              .value_name = static_cast<c4c::ValueNameId>(101),
          });
  if (!expect(regalloc_missing_placement.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      MissingTypedRegisterAuthority,
              "regalloc missing-placement diagnostic category mismatch") ||
      !expect(regalloc_missing_placement.function_name ==
                  static_cast<c4c::FunctionNameId>(91) &&
                  regalloc_missing_placement.value_name ==
                      static_cast<c4c::ValueNameId>(101),
              "regalloc missing-placement diagnostic should preserve identity") ||
      !expect(regalloc_missing_placement.message ==
                  "regalloc register assignment is missing typed register placement",
              "regalloc missing-placement diagnostic message mismatch")) {
    return 1;
  }

  const auto storage_missing_placement =
      prepare::build_prepared_decoded_home_storage_diagnostic(
          prepare::PreparedDecodedHomeStorage{
              .source = prepare::PreparedDecodedHomeStorageSource::StoragePlan,
              .kind = prepare::PreparedDecodedHomeStorageKind::Register,
              .status =
                  prepare::PreparedDecodedHomeStorageStatus::MissingRegisterPlacement,
              .value_id = 902,
          });
  if (!expect(storage_missing_placement.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      MissingTypedRegisterAuthority,
              "storage-plan missing-placement diagnostic category mismatch") ||
      !expect(storage_missing_placement.message ==
                  "storage-plan register value is missing typed register placement",
              "storage-plan missing-placement diagnostic message mismatch")) {
    return 1;
  }

  const auto value_home_missing_placement =
      prepare::build_prepared_decoded_home_storage_diagnostic(
          prepare::PreparedDecodedHomeStorage{
              .source = prepare::PreparedDecodedHomeStorageSource::ValueHome,
              .kind = prepare::PreparedDecodedHomeStorageKind::Register,
              .status =
                  prepare::PreparedDecodedHomeStorageStatus::MissingRegisterPlacement,
              .value_id = 903,
          });
  if (!expect(value_home_missing_placement.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      MissingTypedRegisterAuthority,
              "value-home missing-placement diagnostic category mismatch") ||
      !expect(value_home_missing_placement.message ==
                  "value-home register spelling is diagnostic-only until typed placement exists",
              "value-home missing-placement diagnostic message mismatch")) {
    return 1;
  }

  const auto unsupported_storage =
      prepare::build_prepared_decoded_home_storage_diagnostic(
          prepare::PreparedDecodedHomeStorage{
              .source = prepare::PreparedDecodedHomeStorageSource::StoragePlan,
              .kind = prepare::PreparedDecodedHomeStorageKind::ComputedAddress,
              .status =
                  prepare::PreparedDecodedHomeStorageStatus::UnsupportedStorageEncoding,
              .value_id = 904,
          });
  if (!expect(unsupported_storage.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      UnsupportedStoragePlanAuthority,
              "unsupported storage diagnostic category mismatch") ||
      !expect(unsupported_storage.message ==
                  "storage-plan value does not have a supported typed operand form",
              "unsupported storage diagnostic message mismatch")) {
    return 1;
  }

  const auto unsupported_home =
      prepare::build_prepared_decoded_home_storage_diagnostic(
          prepare::PreparedDecodedHomeStorage{
              .source = prepare::PreparedDecodedHomeStorageSource::ValueHome,
              .kind = prepare::PreparedDecodedHomeStorageKind::PointerBasePlusOffset,
              .status =
                  prepare::PreparedDecodedHomeStorageStatus::UnsupportedValueHomeKind,
              .value_id = 905,
          });
  if (!expect(unsupported_home.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      UnsupportedValueHomeAuthority,
              "unsupported value-home diagnostic category mismatch") ||
      !expect(unsupported_home.source ==
                  prepare::PreparedDecodedHomeStorageSource::ValueHome &&
                  unsupported_home.kind ==
                      prepare::PreparedDecodedHomeStorageKind::PointerBasePlusOffset &&
                  unsupported_home.status ==
                      prepare::PreparedDecodedHomeStorageStatus::UnsupportedValueHomeKind,
              "unsupported value-home diagnostic should preserve decoded facts") ||
      !expect(unsupported_home.message ==
                  "prepared value home does not have a supported typed operand form",
              "unsupported value-home diagnostic message mismatch")) {
    return 1;
  }

  if (!expect(prepare::prepared_decoded_home_storage_diagnostic_category_name(
                  unsupported_home.category) == "unsupported_value_home_authority",
              "diagnostic category name mismatch")) {
    return 1;
  }

  return 0;
}

}  // namespace

int main() {
  if (verify_regalloc_decoding() != 0) {
    return EXIT_FAILURE;
  }
  if (verify_storage_plan_decoding() != 0) {
    return EXIT_FAILURE;
  }
  if (verify_value_home_and_combined_decoding() != 0) {
    return EXIT_FAILURE;
  }
  if (verify_diagnostic_builders() != 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
