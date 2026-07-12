#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/x86/x86.hpp"
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;
using c4c::backend::BackendModuleInput;
using c4c::backend::BackendOptions;

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
}

std::string expected_minimal_direct_extern_call_lane_asm() {
  return asm_header("main") + "    sub rsp, 8\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov r11d, eax\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov esi, r11d\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    mov r12d, eax\n"
         "    mov eax, 0\n"
         "    add rsp, 8\n"
         "    ret\n"
         ".section .rodata\n"
         ".L.str0:\n"
         "    .asciz \"%i\\n\"\n";
}

std::string expected_direct_extern_call_lane_contract_drift_asm() {
  return asm_header("main") + "    sub rsp, 8\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov r10d, eax\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov edx, r10d\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    mov r12d, eax\n"
         "    mov eax, 0\n"
         "    add rsp, 8\n"
         "    ret\n"
         ".section .rodata\n"
         ".L.str0:\n"
         "    .asciz \"%i\\n\"\n";
}

bir::Module make_x86_direct_extern_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%i\n",
  });

  bir::Function actual_function_decl;
  actual_function_decl.name = "actual_function";
  actual_function_decl.is_declaration = true;
  actual_function_decl.return_type = bir::TypeKind::I32;

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .callee = "actual_function",
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t0")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(actual_function_decl));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

int check_route_outputs(const bir::Module& module,
                        const std::string& expected_asm,
                        const std::string& expected_bir_fragment,
                        const char* failure_context) {
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(module, x86_target_profile());
  const auto prepared_bir_text = bir::print(prepared.module);

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the prepared handoff with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer did not emit the canonical asm")
                    .c_str());
  }

  const auto public_asm = c4c::backend::emit_x86_bir_module_entry(module, x86_target_profile());
  if (public_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": explicit x86 BIR entry no longer routes through the x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
  if (generic_asm != public_asm) {
    return fail((std::string(failure_context) +
                 ": generic backend emit path no longer routes x86 BIR input through emit_x86_bir_module_entry")
                    .c_str());
  }

  if (prepared_bir_text.find(expected_bir_fragment) == std::string::npos) {
    return fail((std::string(failure_context) +
                 ": test fixture no longer prepares the expected semantic BIR shape before routing into x86")
                    .c_str());
  }

  return 0;
}

prepare::PreparedValueLocationFunction* find_mutable_prepared_value_location_function(
    prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_name_id = prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_name_id.has_value()) {
    return nullptr;
  }
  for (auto& function_locations : prepared.value_locations.functions) {
    if (function_locations.function_name == *function_name_id) {
      return &function_locations;
    }
  }
  return nullptr;
}

bir::Function* find_mutable_bir_function(prepare::PreparedBirModule& prepared,
                                         std::string_view function_name) {
  for (auto& function : prepared.module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

prepare::PreparedValueHome* find_mutable_prepared_value_home(
    prepare::PreparedBirModule& prepared,
    prepare::PreparedValueLocationFunction& function_locations,
    std::string_view value_name) {
  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, value_name);
  if (!value_name_id.has_value()) {
    return nullptr;
  }
  for (auto& value_home : function_locations.value_homes) {
    if (value_home.value_name == *value_name_id) {
      return &value_home;
    }
  }
  return nullptr;
}

prepare::PreparedMoveBundle* find_mutable_prepared_move_bundle(
    prepare::PreparedValueLocationFunction& function_locations,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (auto& move_bundle : function_locations.move_bundles) {
    if (move_bundle.phase == phase && move_bundle.block_index == block_index &&
        move_bundle.instruction_index == instruction_index) {
      return &move_bundle;
    }
  }
  return nullptr;
}

void erase_prepared_move_bundle(prepare::PreparedValueLocationFunction& function_locations,
                                prepare::PreparedMovePhase phase,
                                std::size_t block_index,
                                std::size_t instruction_index) {
  function_locations.move_bundles.erase(
      std::remove_if(function_locations.move_bundles.begin(),
                     function_locations.move_bundles.end(),
                     [&](const prepare::PreparedMoveBundle& move_bundle) {
                       return move_bundle.phase == phase &&
                              move_bundle.block_index == block_index &&
                              move_bundle.instruction_index == instruction_index;
                     }),
      function_locations.move_bundles.end());
}

int check_route_consumes_prepared_direct_extern_call_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_direct_extern_call_lane_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "main");
  if (function_locations == nullptr) {
    return fail("bounded direct extern call contract drift route: missing prepared value-location function");
  }

  auto* first_call_result_home =
      find_mutable_prepared_value_home(prepared, *function_locations, "%t0");
  if (first_call_result_home == nullptr) {
    return fail("bounded direct extern call contract drift route: missing first call result home");
  }
  first_call_result_home->kind = prepare::PreparedValueHomeKind::Register;
  first_call_result_home->register_name = "r10";
  first_call_result_home->slot_id.reset();
  first_call_result_home->offset_bytes.reset();
  first_call_result_home->immediate_i32.reset();

  auto* printf_before_call = find_mutable_prepared_move_bundle(
      *function_locations, prepare::PreparedMovePhase::BeforeCall, 0, 1);
  if (printf_before_call == nullptr) {
    function_locations->move_bundles.push_back(prepare::PreparedMoveBundle{
        .function_name = function_locations->function_name,
        .phase = prepare::PreparedMovePhase::BeforeCall,
        .block_index = 0,
        .instruction_index = 1,
        .moves = {},
    });
    printf_before_call = &function_locations->move_bundles.back();
  }
  bool rewired_printf_arg = false;
  for (auto& move : printf_before_call->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        move.destination_abi_index != std::optional<std::size_t>{1}) {
      continue;
    }
    move.destination_register_name = "rdx";
    rewired_printf_arg = true;
    break;
  }
  if (!rewired_printf_arg) {
    printf_before_call->moves.push_back(prepare::PreparedMoveResolution{
        .from_value_id = first_call_result_home->value_id,
        .to_value_id = first_call_result_home->value_id,
        .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
        .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
        .destination_abi_index = std::size_t{1},
        .destination_register_name = std::string("rdx"),
        .block_index = 0,
        .instruction_index = 1,
        .uses_cycle_temp_source = false,
        .reason = "contract_direct_extern_call_arg_register_to_register",
    });
  }
  const auto main_function_id =
      prepare::resolve_prepared_function_name_id(prepared.names, "main");
  auto* call_plans = main_function_id.has_value()
                         ? const_cast<prepare::PreparedCallPlansFunction*>(
                               prepare::find_prepared_call_plans(prepared,
                                                                 *main_function_id))
                         : nullptr;
  if (call_plans == nullptr || call_plans->calls.size() != 2 ||
      call_plans->calls[1].arguments.size() != 2) {
    return fail("bounded direct extern call contract drift route: missing cursor-exact prepared call argument plan");
  }
  call_plans->calls[1].arguments[1].destination_register_name = "rdx";

  auto* first_call_after_call = find_mutable_prepared_move_bundle(
      *function_locations, prepare::PreparedMovePhase::AfterCall, 0, 0);
  if (first_call_after_call == nullptr) {
    function_locations->move_bundles.push_back(prepare::PreparedMoveBundle{
        .function_name = function_locations->function_name,
        .phase = prepare::PreparedMovePhase::AfterCall,
        .block_index = 0,
        .instruction_index = 0,
        .moves = {},
    });
    first_call_after_call = &function_locations->move_bundles.back();
  }
  bool saw_first_call_result_move = false;
  for (auto& move : first_call_after_call->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallResultAbi ||
        move.to_value_id != first_call_result_home->value_id) {
      continue;
    }
    move.destination_storage_kind = prepare::PreparedMoveStorageKind::Register;
    move.destination_register_name = "rax";
    saw_first_call_result_move = true;
    break;
  }
  if (!saw_first_call_result_move) {
    first_call_after_call->moves.push_back(prepare::PreparedMoveResolution{
        .from_value_id = first_call_result_home->value_id,
        .to_value_id = first_call_result_home->value_id,
        .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
        .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
        .destination_abi_index = std::nullopt,
        .destination_register_name = std::string("rax"),
        .block_index = 0,
        .instruction_index = 0,
        .uses_cycle_temp_source = false,
        .reason = "contract_direct_extern_call_result_register_to_register",
    });
  }

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string("bounded direct extern call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with exception: ") +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_direct_extern_call_lane_contract_drift_asm()) {
    return fail("bounded direct extern call contract drift route: x86 prepared-module consumer stopped following prepared BeforeCall/AfterCall metadata");
  }

  return 0;
}

const prepare::PreparedCallPlansFunction* find_call_plans(
    const prepare::PreparedBirModule& prepared, std::string_view function_name) {
  const auto function_id =
      prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_id.has_value()) {
    return nullptr;
  }
  return prepare::find_prepared_call_plans(prepared, *function_id);
}

int check_cursor_exact_semantic_operand_call_plans() {
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      make_x86_direct_extern_call_lane_module(), x86_target_profile());
  const auto* plans = find_call_plans(prepared, "main");
  if (plans == nullptr || plans->calls.size() != 2 ||
      plans->calls[0].block_index != 0 || plans->calls[0].instruction_index != 0 ||
      plans->calls[0].direct_callee_name !=
          std::optional<std::string>{"actual_function"} ||
      plans->calls[0].wrapper_kind !=
          prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      !plans->calls[0].arguments.empty() ||
      plans->calls[1].block_index != 0 || plans->calls[1].instruction_index != 1 ||
      plans->calls[1].direct_callee_name != std::optional<std::string>{"printf"} ||
      plans->calls[1].wrapper_kind !=
          prepare::PreparedCallWrapperKind::DirectExternVariadic ||
      plans->calls[1].arguments.size() != 2 ||
      plans->calls[1].arguments[0].instruction_index != 1 ||
      plans->calls[1].arguments[0].arg_index != 0 ||
      plans->calls[1].arguments[0].source_symbol_name !=
          std::optional<std::string>{"@.str0"} ||
      plans->calls[1].arguments[1].instruction_index != 1 ||
      plans->calls[1].arguments[1].arg_index != 1 ||
      !plans->calls[1].arguments[1].source_value_id.has_value()) {
    return fail("direct extern call plans: mixed adjacent calls lost exact cursor, callee, or argument identity");
  }
  const auto semantic_t0_id = plans->calls[1].arguments[1].source_value_id;

  auto unique = make_x86_direct_extern_call_lane_module();
  auto* unique_call = std::get_if<bir::CallInst>(&unique.functions.back().blocks.front().insts[1]);
  unique_call->arg_sources.push_back(bir::CallArgumentSourceRelationship{
      .arg_index = 1, .source_value_name = std::string{"%t0"}});
  auto unique_prepared = prepare::prepare_semantic_bir_module_with_options(
      std::move(unique), x86_target_profile());
  plans = find_call_plans(unique_prepared, "main");
  if (plans == nullptr || plans->calls.size() != 2 ||
      plans->calls[1].instruction_index != 1 || plans->calls[1].arguments.size() != 2) {
    return fail("direct extern call plans: unique compatible source relationship did not refine the semantic operand");
  }
  if (plans->calls[1].arguments[1].source_value_id != semantic_t0_id ||
      plans->calls[1].arguments[1].arg_index != 1 ||
      plans->calls[1].direct_callee_name != std::optional<std::string>{"printf"}) {
    return fail("direct extern call plans: unique refinement changed cursor, callee, or semantic argument identity");
  }

  const auto rejects = [](bir::Module module) {
    auto rejected = prepare::prepare_semantic_bir_module_with_options(
        std::move(module), x86_target_profile());
    const auto* rejected_plans = find_call_plans(rejected, "main");
    return rejected_plans != nullptr && rejected_plans->calls.size() == 1 &&
           rejected_plans->calls.front().instruction_index == 0;
  };

  auto duplicate = make_x86_direct_extern_call_lane_module();
  auto* duplicate_call =
      std::get_if<bir::CallInst>(&duplicate.functions.back().blocks.front().insts[1]);
  duplicate_call->arg_sources.push_back(bir::CallArgumentSourceRelationship{
      .arg_index = 1, .source_value_name = std::string{"%t0"}});
  duplicate_call->arg_sources.push_back(bir::CallArgumentSourceRelationship{
      .arg_index = 1, .source_value_name = std::string{"%t0"}});
  if (!rejects(std::move(duplicate))) {
    return fail("direct extern call plans: duplicate source relationships did not fail closed");
  }

  auto stale = make_x86_direct_extern_call_lane_module();
  auto* stale_call = std::get_if<bir::CallInst>(&stale.functions.back().blocks.front().insts[1]);
  stale_call->arg_sources.push_back(bir::CallArgumentSourceRelationship{
      .arg_index = 2, .source_value_name = std::string{"%t0"}});
  if (!rejects(std::move(stale))) {
    return fail("direct extern call plans: out-of-range source relationship did not fail closed");
  }

  auto contradictory = make_x86_direct_extern_call_lane_module();
  auto* contradictory_call =
      std::get_if<bir::CallInst>(&contradictory.functions.back().blocks.front().insts[1]);
  contradictory_call->arg_sources.push_back(bir::CallArgumentSourceRelationship{
      .arg_index = 1, .source_value_name = std::string{"%wrong"}});
  if (!rejects(std::move(contradictory))) {
    return fail("direct extern call plans: operand-contradicting source relationship did not fail closed");
  }
  return 0;
}

int check_route_requires_authoritative_prepared_before_call_bundle() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_direct_extern_call_lane_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "main");
  if (function_locations == nullptr) {
    return fail("bounded direct extern call contract drift route: missing prepared value-location function");
  }

  erase_prepared_move_bundle(*function_locations, prepare::PreparedMovePhase::BeforeCall, 0, 1);

  try {
    static_cast<void>(c4c::backend::x86::api::emit_prepared_module(prepared));
  } catch (const std::invalid_argument& ex) {
    if (std::string(ex.what()).find("authoritative prepared call-bundle handoff") !=
        std::string::npos) {
      return 0;
    }
    return fail((std::string("bounded direct extern call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with the wrong exception: ") +
                 ex.what())
                    .c_str());
  } catch (const std::exception& ex) {
    return fail((std::string("bounded direct extern call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with the wrong exception type: ") +
                 ex.what())
                    .c_str());
  }

  return fail("bounded direct extern call contract drift route: x86 prepared-module consumer reopened a local call-argument ABI fallback when the authoritative prepared BeforeCall bundle was removed");
}

int check_route_requires_authoritative_prepared_after_call_bundle() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_direct_extern_call_lane_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "main");
  if (function_locations == nullptr) {
    return fail("bounded direct extern call contract drift route: missing prepared value-location function");
  }

  auto* first_call_result_home =
      find_mutable_prepared_value_home(prepared, *function_locations, "%t0");
  if (first_call_result_home == nullptr) {
    return fail("bounded direct extern call contract drift route: missing first call result home");
  }
  first_call_result_home->kind = prepare::PreparedValueHomeKind::Register;
  first_call_result_home->register_name = "rax";
  first_call_result_home->slot_id.reset();
  first_call_result_home->offset_bytes.reset();
  first_call_result_home->immediate_i32.reset();

  erase_prepared_move_bundle(*function_locations, prepare::PreparedMovePhase::AfterCall, 0, 0);

  try {
    static_cast<void>(c4c::backend::x86::api::emit_prepared_module(prepared));
  } catch (const std::invalid_argument& ex) {
    if (std::string(ex.what()).find("authoritative prepared call-bundle handoff") !=
        std::string::npos) {
      return 0;
    }
    return fail((std::string("bounded direct extern call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with the wrong exception: ") +
                 ex.what())
                    .c_str());
  } catch (const std::exception& ex) {
    return fail((std::string("bounded direct extern call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with the wrong exception type: ") +
                 ex.what())
                    .c_str());
  }

  return fail("bounded direct extern call contract drift route: x86 prepared-module consumer reopened a local call-result ABI fallback when the authoritative prepared AfterCall bundle was removed");
}

int check_consumed_plans_require_cursor_exact_scalar_call_argument_authority() {
  const auto rejects = [](prepare::PreparedBirModule prepared) {
    try {
      static_cast<void>(c4c::backend::x86::api::emit_prepared_module(prepared));
    } catch (const std::invalid_argument& ex) {
      return std::string(ex.what()).find("prepared direct extern call argument") !=
             std::string::npos;
    }
    return false;
  };

  auto missing_source = prepare::prepare_semantic_bir_module_with_options(
      make_x86_direct_extern_call_lane_module(), x86_target_profile());
  auto* missing_plans = const_cast<prepare::PreparedCallPlansFunction*>(
      find_call_plans(missing_source, "main"));
  if (missing_plans == nullptr || missing_plans->calls.size() != 2 ||
      missing_plans->calls[1].arguments.size() != 2) {
    return fail("x86 prepared call-plan authority: malformed adjacent-call fixture");
  }
  missing_plans->calls[1].arguments[1].source_value_id.reset();
  if (!rejects(std::move(missing_source))) {
    return fail("x86 prepared call-plan authority: missing scalar source identity did not fail closed");
  }

  auto inconsistent_source = prepare::prepare_semantic_bir_module_with_options(
      make_x86_direct_extern_call_lane_module(), x86_target_profile());
  auto* inconsistent_main = find_mutable_bir_function(inconsistent_source, "main");
  if (inconsistent_main == nullptr || inconsistent_main->blocks.empty()) {
    return fail("x86 prepared call-plan authority: malformed scalar-source fixture");
  }
  auto* inconsistent_call =
      std::get_if<bir::CallInst>(&inconsistent_main->blocks.front().insts[1]);
  if (inconsistent_call == nullptr || inconsistent_call->args.size() != 2) {
    return fail("x86 prepared call-plan authority: malformed variadic-call fixture");
  }
  inconsistent_call->args[1].name = "%t1";
  if (!rejects(std::move(inconsistent_source))) {
    return fail("x86 prepared call-plan authority: operand/plan source disagreement did not fail closed");
  }

  auto inconsistent_destination = prepare::prepare_semantic_bir_module_with_options(
      make_x86_direct_extern_call_lane_module(), x86_target_profile());
  auto* destination_plans = const_cast<prepare::PreparedCallPlansFunction*>(
      find_call_plans(inconsistent_destination, "main"));
  if (destination_plans == nullptr || destination_plans->calls.size() != 2 ||
      destination_plans->calls[1].arguments.size() != 2) {
    return fail("x86 prepared call-plan authority: malformed destination fixture");
  }
  destination_plans->calls[1].arguments[1].destination_register_name = "rdx";
  if (!rejects(std::move(inconsistent_destination))) {
    return fail("x86 prepared call-plan authority: call-plan/bundle destination disagreement did not fail closed");
  }
  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_direct_extern_call_tests() {
  if (const auto status =
          check_route_outputs(make_x86_direct_extern_call_lane_module(),
                              expected_minimal_direct_extern_call_lane_asm(),
                              "bir.func @main() -> i32 {",
                              "bounded direct extern-call prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status = check_route_consumes_prepared_direct_extern_call_contract();
      status != 0) {
    return status;
  }
  if (const auto status = check_cursor_exact_semantic_operand_call_plans(); status != 0) {
    return status;
  }
  if (const auto status = check_route_requires_authoritative_prepared_before_call_bundle();
      status != 0) {
    return status;
  }
  if (const auto status = check_route_requires_authoritative_prepared_after_call_bundle();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_consumed_plans_require_cursor_exact_scalar_call_argument_authority();
      status != 0) {
    return status;
  }
  return 0;
}
