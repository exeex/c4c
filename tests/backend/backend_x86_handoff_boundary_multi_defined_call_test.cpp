#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;
using c4c::backend::BackendModuleInput;
using c4c::backend::BackendOptions;

const c4c::TargetProfile& target_profile_from_module_triple(std::string_view target_triple,
                                                            c4c::TargetProfile& storage) {
  storage = c4c::target_profile_from_triple(
      target_triple.empty() ? c4c::default_host_target_triple() : target_triple);
  return storage;
}

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

std::string narrow_abi_register(std::string_view wide_register) {
  if (wide_register == "rax") return "eax";
  if (wide_register == "rdi") return "edi";
  if (wide_register == "rsi") return "esi";
  if (wide_register == "rdx") return "edx";
  if (wide_register == "rcx") return "ecx";
  if (wide_register == "r8") return "r8d";
  if (wide_register == "r9") return "r9d";
  return std::string(wide_register);
}

std::string minimal_i32_return_register() {
  const auto abi =
      c4c::backend::lir_to_bir_detail::compute_function_return_abi(x86_target_profile(),
                                                                   bir::TypeKind::I32,
                                                                   false);
  if (!abi.has_value()) {
    throw std::runtime_error("missing canonical i32 return ABI for x86 handoff test");
  }
  const auto wide_register =
      prepare::call_result_destination_register_name(x86_target_profile(), *abi);
  if (!wide_register.has_value()) {
    throw std::runtime_error("missing canonical i32 return register for x86 handoff test");
  }
  return narrow_abi_register(*wide_register);
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
}

std::string expected_minimal_multi_defined_direct_call_lane_asm() {
  return asm_header("foo") + "    ret\n" +
         asm_header("actual_function") + "    mov " + minimal_i32_return_register() +
         ", 42\n    ret\n" +
         asm_header("main") + "    sub rsp, 24\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov DWORD PTR [rsp + 16], eax\n"
         "    mov eax, DWORD PTR [rsp + 16]\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov esi, eax\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov DWORD PTR [rsp + 20], eax\n"
         "    mov eax, DWORD PTR [rsp + 20]\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov esi, eax\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    mov eax, 0\n"
         "    add rsp, 24\n"
         "    ret\n"
         ".section .rodata\n"
         ".L.str0:\n"
         "    .asciz \"%i\\n\"\n";
}

std::string expected_multi_defined_direct_call_lane_contract_drift_asm() {
  return asm_header("foo") + "    ret\n" +
         asm_header("actual_function") + "    mov " + minimal_i32_return_register() +
         ", 42\n    ret\n" +
         asm_header("main") + "    sub rsp, 24\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov r10d, eax\n"
         "    mov DWORD PTR [rsp + 16], r10d\n"
         "    mov eax, DWORD PTR [rsp + 16]\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov edx, eax\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov DWORD PTR [rsp + 20], eax\n"
         "    mov eax, DWORD PTR [rsp + 20]\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov esi, eax\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    mov eax, 0\n"
         "    add rsp, 24\n"
         "    ret\n"
         ".section .rodata\n"
         ".L.str0:\n"
         "    .asciz \"%i\\n\"\n";
}

bir::Module make_x86_multi_defined_direct_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%i\n",
  });

  bir::Function foo;
  foo.name = "foo";
  foo.return_type = bir::TypeKind::Void;
  foo.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{},
  });

  bir::Function actual_function;
  actual_function.name = "actual_function";
  actual_function.return_type = bir::TypeKind::I32;
  actual_function.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(42)},
  });

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
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.function_pointer",
      .type = bir::TypeKind::Ptr,
      .align_bytes = 8,
  });
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.a",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.b",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "@actual_function"),
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.a",
      .value = bir::Value::named(bir::TypeKind::I32, "%t1"),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .slot_name = "%lv.a",
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t3")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "@actual_function"),
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.b",
      .value = bir::Value::named(bir::TypeKind::I32, "%t6"),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t8"),
      .slot_name = "%lv.b",
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t9"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t8")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(foo));
  module.functions.push_back(std::move(actual_function));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

int check_route_outputs(const bir::Module& module,
                        const std::string& expected_asm,
                        const std::string& expected_bir_fragment,
                        const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto prepared_bir_text = bir::print(prepared.module);

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
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

  const auto public_asm = c4c::backend::emit_target_bir_module(module, target_profile);
  if (public_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": public x86 BIR entry no longer routes through the x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
  if (generic_asm != public_asm) {
    return fail((std::string(failure_context) +
                 ": generic backend emit path no longer routes x86 BIR input through emit_target_bir_module")
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

int check_route_consumes_prepared_call_move_bundle_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_multi_defined_direct_call_lane_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "main");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing prepared value-location function");
  }

  auto* first_call_result_home =
      find_mutable_prepared_value_home(prepared, *function_locations, "%t1");
  if (first_call_result_home == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing first call result home");
  }
  first_call_result_home->kind = prepare::PreparedValueHomeKind::Register;
  first_call_result_home->register_name = "r10";
  first_call_result_home->slot_id.reset();
  first_call_result_home->offset_bytes.reset();
  first_call_result_home->immediate_i32.reset();

  auto* first_printf_arg_home =
      find_mutable_prepared_value_home(prepared, *function_locations, "%t3");
  if (first_printf_arg_home == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing first printf i32 argument home");
  }
  auto* first_printf_before_call = find_mutable_prepared_move_bundle(
      *function_locations, prepare::PreparedMovePhase::BeforeCall, 0, 3);
  if (first_printf_before_call == nullptr) {
    function_locations->move_bundles.push_back(prepare::PreparedMoveBundle{
        .function_name = function_locations->function_name,
        .phase = prepare::PreparedMovePhase::BeforeCall,
        .block_index = 0,
        .instruction_index = 3,
        .moves = {},
    });
    first_printf_before_call = &function_locations->move_bundles.back();
  }
  bool rewired_printf_arg = false;
  for (auto& move : first_printf_before_call->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        move.destination_abi_index != std::optional<std::size_t>{1}) {
      continue;
    }
    move.destination_register_name = "rdx";
    rewired_printf_arg = true;
    break;
  }
  if (!rewired_printf_arg) {
    first_printf_before_call->moves.push_back(prepare::PreparedMoveResolution{
        .from_value_id = first_printf_arg_home->value_id,
        .to_value_id = first_printf_arg_home->value_id,
        .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
        .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
        .destination_abi_index = std::size_t{1},
        .destination_register_name = std::string("rdx"),
        .block_index = 0,
        .instruction_index = 3,
        .uses_cycle_temp_source = false,
        .reason = "contract_call_arg_register_to_register",
    });
  }

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
        .reason = "contract_call_result_register_to_register",
    });
  }

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string("bounded multi-defined call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with exception: ") +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_multi_defined_direct_call_lane_contract_drift_asm()) {
    return fail("bounded multi-defined call contract drift route: x86 prepared-module consumer stopped following prepared BeforeCall/AfterCall call-lane metadata");
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_multi_defined_call_tests() {
  // Keep the bounded multi-defined same-module symbol-call lane isolated so
  // later Step 5 packets can move the remaining rejection-only boundary
  // coverage without re-threading this supported-path route.
  if (const auto status =
          check_route_outputs(make_x86_multi_defined_direct_call_lane_module(),
                              expected_minimal_multi_defined_direct_call_lane_asm(),
                              "bir.func @main() -> i32 {",
                              "bounded multi-defined-function same-module symbol-call prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status = check_route_consumes_prepared_call_move_bundle_contract(); status != 0) {
    return status;
  }

  return 0;
}
