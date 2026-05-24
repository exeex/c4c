#include "src/backend/mir/x86/prepared/prepared.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
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
  const auto predecessor_label = prepared.names.block_labels.intern("entry");
  const auto successor_label = prepared.names.block_labels.intern("join");
  const auto other_successor_label = prepared.names.block_labels.intern("other");
  const auto regalloc_value_name = prepared.names.value_names.intern("regalloc_stack");
  const auto storage_value_name = prepared.names.value_names.intern("storage_immediate");
  const auto empty_storage_value_name = prepared.names.value_names.intern("empty_storage");
  const auto home_value_name = prepared.names.value_names.intern("home_stack");
  const auto block_entry_value_name = prepared.names.value_names.intern("block_entry_value");
  const auto formal_reg_name = prepared.names.value_names.intern("formal_reg");
  const auto formal_stack_name = prepared.names.value_names.intern("formal_stack");
  (void)prepared.names.value_names.intern("formal_missing_home");
  prepared.module.functions.push_back(bir::Function{
      .name = "x86.decode",
      .params = {
          bir::Param{
              .type = bir::TypeKind::I32,
              .name = "formal_reg",
              .size_bytes = 4,
              .align_bytes = 4,
              .abi = bir::CallArgAbiInfo{
                  .type = bir::TypeKind::I32,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .primary_class = bir::AbiValueClass::Integer,
                  .passed_in_register = true,
              },
          },
          bir::Param{
              .type = bir::TypeKind::I32,
              .name = "formal_stack",
              .size_bytes = 4,
              .align_bytes = 4,
              .abi = bir::CallArgAbiInfo{
                  .type = bir::TypeKind::I32,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .primary_class = bir::AbiValueClass::Integer,
                  .passed_on_stack = true,
              },
          },
          bir::Param{
              .type = bir::TypeKind::I32,
              .name = "formal_missing_home",
              .size_bytes = 4,
              .align_bytes = 4,
              .abi = bir::CallArgAbiInfo{
                  .type = bir::TypeKind::I32,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .primary_class = bir::AbiValueClass::Integer,
                  .passed_in_register = true,
              },
          },
          bir::Param{
              .type = bir::TypeKind::I32,
              .name = "formal_varargs",
              .size_bytes = 4,
              .align_bytes = 4,
              .is_varargs = true,
          },
      },
  });

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
          prepare::PreparedValueHome{
              .value_id = 5,
              .function_name = function_name,
              .value_name = block_entry_value_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"ebx"},
          },
          prepare::PreparedValueHome{
              .value_id = 10,
              .function_name = function_name,
              .value_name = formal_reg_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"eax"},
          },
          prepare::PreparedValueHome{
              .value_id = 11,
              .function_name = function_name,
              .value_name = formal_stack_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = 12,
              .offset_bytes = 96,
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .block_index = 0,
              .instruction_index = 3,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 2,
                  .to_value_id = 2,
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"eax"},
                  .destination_contiguous_width = 1,
                  .destination_occupied_register_names = {"eax"},
                  .block_index = 0,
                  .instruction_index = 3,
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .destination_register_placement = prepare::PreparedRegisterPlacement{
                      .bank = prepare::PreparedRegisterBank::Gpr,
                      .pool = prepare::PreparedRegisterSlotPool::CallArgument,
                      .slot_index = 0,
                      .contiguous_width = 1,
                  },
              }},
              .abi_bindings = {prepare::PreparedAbiBinding{
                  .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_abi_index = std::size_t{0},
                  .destination_register_name = std::string{"eax"},
                  .destination_contiguous_width = 1,
                  .destination_occupied_register_names = {"eax"},
                  .destination_register_placement = prepare::PreparedRegisterPlacement{
                      .bank = prepare::PreparedRegisterBank::Gpr,
                      .pool = prepare::PreparedRegisterSlotPool::CallArgument,
                      .slot_index = 0,
                      .contiguous_width = 1,
                  },
              }},
          },
          prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 1,
              .source_parallel_copy_predecessor_label = predecessor_label,
              .source_parallel_copy_successor_label = successor_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .from_value_id = 2,
                      .to_value_id = 5,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .from_value_id = 3,
                      .to_value_id = 99,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_register_name = std::string{"edx"},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
          prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 2,
              .source_parallel_copy_successor_label = other_successor_label,
              .moves = {prepare::PreparedMoveResolution{
                  .from_value_id = 2,
                  .to_value_id = 5,
                  .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"wrong_label"},
                  .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              }},
          },
      },
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 3,
          .arguments = {prepare::PreparedCallArgumentPlan{
              .instruction_index = 3,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::Immediate,
              .destination_register_name = std::string{"eax"},
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {"eax"},
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_register_placement = prepare::PreparedRegisterPlacement{
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .pool = prepare::PreparedRegisterSlotPool::CallArgument,
                  .slot_index = 0,
                  .contiguous_width = 1,
              },
          }},
      }},
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
              "x86 prepared query did not expose value locations") ||
      !expect(query.call_plans() != nullptr,
              "x86 prepared query did not expose call plans")) {
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

int check_query_reuses_shared_call_boundary_classification() {
  const auto prepared = make_fixture();
  const auto query = x86_prepared::make_query(prepared, "x86.decode");
  const auto* call_plans = query.call_plans();
  const auto* locations = query.locations();
  if (call_plans == nullptr || call_plans->calls.empty() || locations == nullptr ||
      locations->move_bundles.empty() || locations->move_bundles.front().moves.empty()) {
    return fail("x86 prepared query fixture did not expose call-boundary inputs");
  }

  const auto& call_plan = call_plans->calls.front();
  const auto& bundle = locations->move_bundles.front();
  const auto& move = bundle.moves.front();
  const auto classified =
      query.classify_call_boundary_move(call_plan, bundle, move);

  if (!expect(prepare::prepared_call_boundary_move_classification_available(classified),
              "x86 prepared query did not reuse available shared call-boundary classification") ||
      !expect(classified.phase == prepare::PreparedMovePhase::BeforeCall,
              "x86 prepared classification did not preserve phase") ||
      !expect(classified.destination_kind ==
                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              "x86 prepared classification did not preserve destination role") ||
      !expect(classified.storage_kind == prepare::PreparedMoveStorageKind::Register,
              "x86 prepared classification did not preserve storage kind") ||
      !expect(classified.abi_index == std::optional<std::size_t>{0},
              "x86 prepared classification did not preserve ABI index") ||
      !expect(classified.argument_plan == &call_plan.arguments.front(),
              "x86 prepared classification did not preserve argument plan authority") ||
      !expect(classified.abi_binding == &bundle.abi_bindings.front(),
              "x86 prepared classification did not preserve ABI binding authority") ||
      !expect(classified.move == &move && classified.bundle == &bundle &&
                  classified.call_plan == &call_plan,
              "x86 prepared classification did not preserve source Prepared records")) {
    return 1;
  }

  auto drifted_move = move;
  drifted_move.destination_register_name = std::string{"ecx"};
  const auto missing_binding =
      query.classify_call_boundary_move(call_plan, bundle, drifted_move);
  if (!expect(missing_binding.status ==
                  prepare::PreparedCallBoundaryMoveClassificationStatus::MissingAbiBinding,
              "x86 prepared classification should surface missing binding without target fallback") ||
      !expect(missing_binding.argument_plan == &call_plan.arguments.front(),
              "x86 missing-binding classification should still preserve argument authority")) {
    return 1;
  }

  return 0;
}

int check_query_reuses_shared_formal_publication_plans() {
  const auto prepared = make_fixture();
  const auto query = x86_prepared::make_query(prepared, "x86.decode");

  if (!expect(query.bir_function() != nullptr,
              "x86 prepared query did not expose prepared BIR function")) {
    return 1;
  }

  const auto plans = query.plan_formal_publications();
  if (!expect(plans.size() == 4,
              "x86 prepared query did not reuse shared formal-publication collection")) {
    return 1;
  }

  const auto reg_home = query.plan_formal_publication(0);
  if (!expect(prepare::prepared_formal_publication_available(reg_home),
              "x86 prepared query did not reuse available register-home publication plan") ||
      !expect(reg_home.action ==
                  prepare::PreparedFormalPublicationAction::IncomingRegisterToHome,
              "x86 formal publication did not preserve incoming-register action") ||
      !expect(reg_home.formal_index == 0 && reg_home.value_id.has_value() &&
                  *reg_home.value_id == 10,
              "x86 formal publication did not preserve formal index and prepared value id") ||
      !expect(reg_home.formal == &query.bir_function()->params[0],
              "x86 formal publication did not preserve source formal pointer") ||
      !expect(reg_home.home_kind == prepare::PreparedValueHomeKind::Register &&
                  reg_home.home != nullptr && reg_home.home->register_name == "eax",
              "x86 formal publication did not preserve register-home facts")) {
    return 1;
  }

  const auto stack_home = query.plan_formal_publication(1);
  if (!expect(prepare::prepared_formal_publication_available(stack_home),
              "x86 prepared query did not reuse available stack-home publication plan") ||
      !expect(stack_home.action ==
                  prepare::PreparedFormalPublicationAction::IncomingStackToHome,
              "x86 formal publication did not preserve incoming-stack action") ||
      !expect(stack_home.home_kind == prepare::PreparedValueHomeKind::StackSlot &&
                  stack_home.home != nullptr && stack_home.home->offset_bytes == 96,
              "x86 formal publication did not preserve stack-home facts")) {
    return 1;
  }

  const auto missing_home = query.plan_formal_publication(2);
  if (!expect(missing_home.status ==
                  prepare::PreparedFormalPublicationStatus::MissingValueHome,
              "x86 formal publication should surface missing value-home authority") ||
      !expect(missing_home.formal == &query.bir_function()->params[2],
              "x86 missing-home publication should preserve source formal facts")) {
    return 1;
  }

  const auto varargs = query.plan_formal_publication(3);
  if (!expect(varargs.status == prepare::PreparedFormalPublicationStatus::NoPublication,
              "x86 formal publication should preserve no-publication formals") ||
      !expect(varargs.action == prepare::PreparedFormalPublicationAction::NoPublication,
              "x86 no-publication formal should not select an entry-copy action")) {
    return 1;
  }

  return 0;
}

int check_query_reuses_shared_block_entry_publications() {
  const auto prepared = make_fixture();
  const auto query = x86_prepared::make_query(prepared, "x86.decode");
  const auto successor_label = prepared.names.block_labels.find("join");
  const auto other_successor_label = prepared.names.block_labels.find("other");
  if (!expect(successor_label != c4c::kInvalidBlockLabel &&
                  other_successor_label != c4c::kInvalidBlockLabel,
              "x86 prepared query fixture did not expose block labels")) {
    return 1;
  }

  const auto publications = query.collect_block_entry_publications(successor_label);
  if (!expect(publications.size() == 2,
              "x86 prepared query did not reuse shared block-entry publication collection")) {
    return 1;
  }

  const auto* locations = query.locations();
  if (locations == nullptr || locations->move_bundles.size() < 2U) {
    return fail("x86 prepared query fixture did not expose block-entry inputs");
  }
  const auto& block_entry_bundle = locations->move_bundles[1];

  const auto& available = publications[0];
  if (!expect(prepare::prepared_block_entry_publication_available(available),
              "x86 block-entry publication should be available") ||
      !expect(available.bundle == &block_entry_bundle &&
                  available.move == &block_entry_bundle.moves.front(),
              "x86 block-entry publication did not preserve source Prepared records") ||
      !expect(available.home != nullptr && available.home->value_id == 5,
              "x86 block-entry publication did not preserve value-home authority") ||
      !expect(available.destination_value_id == 5 &&
                  available.destination_value_name ==
                      prepared.names.value_names.find("block_entry_value"),
              "x86 block-entry publication did not preserve destination value facts") ||
      !expect(available.destination_register_name == std::optional<std::string>{"ebx"},
              "x86 block-entry publication did not preserve register-name availability")) {
    return 1;
  }

  const auto& missing_home = publications[1];
  if (!expect(missing_home.status ==
                  prepare::PreparedBlockEntryPublicationStatus::MissingValueHome,
              "x86 block-entry publication should surface missing home status") ||
      !expect(missing_home.home == nullptr && missing_home.destination_value_id == 99,
              "x86 missing-home block-entry publication should preserve value id") ||
      !expect(missing_home.destination_register_name == std::optional<std::string>{"edx"},
              "x86 missing-home block-entry publication should preserve move register fact")) {
    return 1;
  }

  const auto other_publications =
      query.collect_block_entry_publications(other_successor_label);
  if (!expect(other_publications.size() == 1 &&
                  other_publications.front().destination_register_name ==
                      std::optional<std::string>{"wrong_label"},
              "x86 block-entry publication query should filter by successor label")) {
    return 1;
  }

  return 0;
}

int check_query_reuses_shared_home_storage_diagnostics() {
  const auto prepared = make_fixture();
  const auto query = x86_prepared::make_query(prepared, "x86.decode");

  const auto missing = query.diagnose_home_storage(99);
  if (!expect(missing.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      MissingValueAuthority,
              "x86 prepared diagnostic should reuse missing-authority category") ||
      !expect(missing.value_id == 99,
              "x86 prepared diagnostic should preserve missing value id") ||
      !expect(missing.message == "no typed prepared authority exists for value operand",
              "x86 prepared diagnostic should reuse missing-authority message")) {
    return 1;
  }

  const auto empty_storage = query.diagnose_home_storage(3);
  if (!expect(empty_storage.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      UnsupportedStoragePlanAuthority,
              "x86 prepared diagnostic should reuse storage-plan category") ||
      !expect(empty_storage.source == prepare::PreparedDecodedHomeStorageSource::StoragePlan &&
                  empty_storage.status ==
                      prepare::PreparedDecodedHomeStorageStatus::MissingAuthority,
              "x86 prepared diagnostic should preserve decoded storage facts") ||
      !expect(empty_storage.message ==
                  "storage-plan value does not have a supported typed operand form",
              "x86 prepared diagnostic should reuse storage-plan message")) {
    return 1;
  }

  const auto value_home_register = query.diagnose_home_storage(5);
  if (!expect(value_home_register.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      MissingTypedRegisterAuthority,
              "x86 prepared diagnostic should reuse value-home typed-register category") ||
      !expect(value_home_register.source ==
                  prepare::PreparedDecodedHomeStorageSource::ValueHome &&
                  value_home_register.status ==
                      prepare::PreparedDecodedHomeStorageStatus::MissingRegisterPlacement,
              "x86 prepared diagnostic should preserve value-home register facts") ||
      !expect(value_home_register.message ==
                  "value-home register spelling is diagnostic-only until typed placement exists",
              "x86 prepared diagnostic should reuse value-home register message")) {
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
  const auto publication = query.plan_formal_publication(0);
  if (!expect(publication.status == prepare::PreparedFormalPublicationStatus::MissingInputs,
              "missing x86 prepared query should not report formal-publication authority") ||
      !expect(query.plan_formal_publications().empty(),
              "missing x86 prepared query should not collect formal-publication plans")) {
    return 1;
  }
  if (!expect(query.collect_block_entry_publications(c4c::BlockLabelId{1}).empty(),
              "missing x86 prepared query should not collect block-entry publications")) {
    return 1;
  }
  const auto missing_diagnostic = query.diagnose_home_storage(5);
  if (!expect(missing_diagnostic.category ==
                  prepare::PreparedDecodedHomeStorageDiagnosticCategory::
                      MissingValueAuthority,
              "missing x86 prepared query should still build shared missing-authority diagnostic")) {
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
  if (const auto status = check_query_reuses_shared_call_boundary_classification();
      status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = check_query_reuses_shared_formal_publication_plans();
      status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = check_query_reuses_shared_block_entry_publications();
      status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = check_query_reuses_shared_home_storage_diagnostics();
      status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = check_missing_query_reports_no_authority(); status != 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
