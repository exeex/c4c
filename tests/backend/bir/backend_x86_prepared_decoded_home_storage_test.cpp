#include "src/backend/mir/x86/prepared/prepared.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

namespace prepare = c4c::backend::prepare;
namespace x86_prepared = c4c::backend::x86::prepared;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

prepare::PreparedBirModule make_fixture() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("x86.decode");
  const auto regalloc_value_name = prepared.names.value_names.intern("regalloc_stack");
  const auto storage_value_name = prepared.names.value_names.intern("storage_immediate");
  const auto empty_storage_value_name = prepared.names.value_names.intern("empty_storage");
  const auto home_value_name = prepared.names.value_names.intern("home_stack");

  prepared.regalloc.functions.push_back(prepare::PreparedRegallocFunction{
      .function_name = function_name,
      .values = {prepare::PreparedRegallocValue{
          .value_id = 1,
          .function_name = function_name,
          .value_name = regalloc_value_name,
          .assigned_stack_slot = prepare::PreparedStackSlotAssignment{
              .slot_id = 4,
              .offset_bytes = 32,
              .size_bytes = 8,
              .align_bytes = 8,
          },
      }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {
          prepare::PreparedStoragePlanValue{
              .value_id = 1,
              .value_name = regalloc_value_name,
              .encoding = prepare::PreparedStorageEncodingKind::Immediate,
              .immediate_i32 = 111,
          },
          prepare::PreparedStoragePlanValue{
              .value_id = 2,
              .value_name = storage_value_name,
              .encoding = prepare::PreparedStorageEncodingKind::Immediate,
              .immediate_i32 = 42,
          },
          prepare::PreparedStoragePlanValue{
              .value_id = 3,
              .value_name = empty_storage_value_name,
              .encoding = prepare::PreparedStorageEncodingKind::None,
          },
      },
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 2,
              .function_name = function_name,
              .value_name = storage_value_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = 7,
              .offset_bytes = 56,
          },
          prepare::PreparedValueHome{
              .value_id = 3,
              .function_name = function_name,
              .value_name = empty_storage_value_name,
              .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
              .immediate_i32 = 7,
          },
          prepare::PreparedValueHome{
              .value_id = 4,
              .function_name = function_name,
              .value_name = home_value_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = 9,
              .offset_bytes = 72,
          },
      },
  });
  return prepared;
}

int check_query_exposes_decoded_home_storage_inputs() {
  const auto prepared = make_fixture();
  const auto query = x86_prepared::make_query(prepared, "x86.decode");

  if (!expect(query.regalloc() != nullptr, "x86 prepared query did not expose regalloc") ||
      !expect(query.storage_plan() != nullptr,
              "x86 prepared query did not expose storage plan") ||
      !expect(query.locations() != nullptr,
              "x86 prepared query did not expose value locations")) {
    return 1;
  }
  return 0;
}

int check_query_decodes_shared_home_storage_precedence() {
  const auto prepared = make_fixture();
  const auto query = x86_prepared::make_query(prepared, "x86.decode");

  const auto regalloc_stack = query.decode_home_storage(1);
  if (!expect(regalloc_stack.source ==
                  prepare::PreparedDecodedHomeStorageSource::RegallocAssignment,
              "x86 decoded home/storage should preserve regalloc precedence") ||
      !expect(regalloc_stack.kind == prepare::PreparedDecodedHomeStorageKind::FrameSlot,
              "x86 decoded regalloc assignment should preserve frame-slot facts") ||
      !expect(regalloc_stack.slot_id == 4 && regalloc_stack.stack_offset_bytes == 32,
              "x86 decoded regalloc assignment did not preserve stack facts")) {
    return 1;
  }

  const auto storage_immediate = query.decode_home_storage(2);
  if (!expect(storage_immediate.source ==
                  prepare::PreparedDecodedHomeStorageSource::StoragePlan,
              "x86 decoded home/storage should preserve storage-plan precedence") ||
      !expect(storage_immediate.kind ==
                  prepare::PreparedDecodedHomeStorageKind::ImmediateI32,
              "x86 decoded storage plan should preserve immediate facts") ||
      !expect(storage_immediate.immediate_i32 == 42,
              "x86 decoded storage plan did not preserve immediate payload")) {
    return 1;
  }

  const auto empty_storage = query.decode_home_storage(3);
  if (!expect(empty_storage.source == prepare::PreparedDecodedHomeStorageSource::StoragePlan,
              "x86 decoded empty storage authority should block value-home fallback") ||
      !expect(empty_storage.status ==
                  prepare::PreparedDecodedHomeStorageStatus::MissingAuthority,
              "x86 decoded empty storage authority should preserve missing-authority status") ||
      !expect(empty_storage.kind == prepare::PreparedDecodedHomeStorageKind::None,
              "x86 decoded empty storage authority should not become a value-home immediate")) {
    return 1;
  }

  const auto home_stack = query.decode_home_storage(4);
  if (!expect(home_stack.source == prepare::PreparedDecodedHomeStorageSource::ValueHome,
              "x86 decoded home/storage should fall through on true no-record authority") ||
      !expect(home_stack.kind == prepare::PreparedDecodedHomeStorageKind::FrameSlot,
              "x86 decoded value home should preserve stack facts") ||
      !expect(home_stack.slot_id == 9 && home_stack.stack_offset_bytes == 72,
              "x86 decoded value home did not preserve stack facts")) {
    return 1;
  }

  return 0;
}

int check_missing_query_reports_no_authority() {
  const auto prepared = make_fixture();
  const auto query = x86_prepared::make_query(prepared, "missing");
  const auto decoded = query.decode_home_storage(5);
  if (!expect(decoded.source == prepare::PreparedDecodedHomeStorageSource::None,
              "missing x86 prepared query should not report an authority source") ||
      !expect(decoded.status == prepare::PreparedDecodedHomeStorageStatus::MissingAuthority,
              "missing x86 prepared query should report missing authority")) {
    return 1;
  }
  return 0;
}

}  // namespace

int main() {
  if (const auto status = check_query_exposes_decoded_home_storage_inputs(); status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = check_query_decodes_shared_home_storage_precedence();
      status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = check_missing_query_reports_no_authority(); status != 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
