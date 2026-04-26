#include <algorithm>
#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir/lowering.hpp"
#include "src/backend/mir/x86/abi/abi.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"  // Compatibility holdout for render_prepared_stack_memory_operand().
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
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
  return c4c::backend::x86::abi::narrow_i32_register_name(*wide_register);
}

bir::Module make_x86_param_passthrough_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "id_i32";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "p.x"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

std::string minimal_i32_param_register() {
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_passthrough_module(),
                                                        x86_target_profile());
  if (prepared.module.functions.empty() || prepared.module.functions.front().params.empty() ||
      !prepared.module.functions.front().params.front().abi.has_value()) {
    throw std::runtime_error("missing prepared i32 parameter ABI metadata for x86 handoff test");
  }
  const auto wide_register = prepare::call_arg_destination_register_name(
      x86_target_profile(), *prepared.module.functions.front().params.front().abi, 0);
  if (!wide_register.has_value()) {
    throw std::runtime_error("missing canonical i32 parameter register for x86 handoff test");
  }
  return c4c::backend::x86::abi::narrow_i32_register_name(*wide_register);
}

bir::Module make_x86_param_add_immediate_module();
bir::Module make_x86_immediate_add_param_module();
bir::Module make_x86_param_mul_immediate_module();
bir::Module make_x86_param_and_immediate_module();
bir::Module make_x86_immediate_or_param_module();
bir::Module make_x86_immediate_xor_param_module();
bir::Module make_x86_param_shl_immediate_module();
bir::Module make_x86_param_lshr_immediate_module();
bir::Module make_x86_param_ashr_immediate_module();
bir::Module make_x86_named_immediate_add_module();
bir::Module make_x86_named_same_module_global_return_module();
bir::Module make_x86_named_same_module_global_sub_return_module();

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
}

std::string expected_minimal_constant_return_asm(const char* function_name, int returned_value) {
  return asm_header(function_name) + "    mov " + minimal_i32_return_register() + ", " +
         std::to_string(returned_value) + "\n    ret\n";
}

std::string expected_minimal_param_passthrough_asm(const char* function_name) {
  return asm_header(function_name) + "    mov " + minimal_i32_return_register() + ", " +
         minimal_i32_param_register() + "\n    ret\n";
}

std::string expected_minimal_stack_home_passthrough_asm(const char* function_name,
                                                        std::size_t frame_size,
                                                        std::size_t byte_offset) {
  return asm_header(function_name) + "    sub rsp, " + std::to_string(frame_size) + "\n    mov " +
         minimal_i32_return_register() + ", " +
         c4c::backend::x86::render_prepared_stack_memory_operand(byte_offset, "DWORD") +
         "\n    add rsp, " + std::to_string(frame_size) + "\n    ret\n";
}

std::string expected_minimal_param_binary_asm(const char* function_name,
                                              const char* mnemonic,
                                              int immediate) {
  return asm_header(function_name) + "    mov r11d, " + minimal_i32_param_register() + "\n    " +
         mnemonic + " r11d, " + std::to_string(immediate) + "\n    mov " +
         minimal_i32_return_register() + ", r11d\n    ret\n";
}

std::string expected_minimal_stack_home_param_binary_asm(const char* function_name,
                                                         const char* mnemonic,
                                                         int immediate,
                                                         std::size_t frame_size,
                                                         std::size_t byte_offset) {
  return asm_header(function_name) + "    sub rsp, " + std::to_string(frame_size) + "\n    mov r11d, " +
         c4c::backend::x86::render_prepared_stack_memory_operand(byte_offset, "DWORD") + "\n    " +
         mnemonic + " r11d, " + std::to_string(immediate) + "\n    mov " +
         minimal_i32_return_register() + ", r11d\n    add rsp, " + std::to_string(frame_size) +
         "\n    ret\n";
}

std::string expected_minimal_same_module_global_named_return_asm(const char* function_name) {
  return asm_header(function_name)
         + "    mov DWORD PTR [rip + x], 0\n"
           "    mov eax, DWORD PTR [rip + x]\n"
           "    ret\n"
           ".bss\n.globl x\n.type x, @object\n.p2align 2\nx:\n"
           "    .long 0\n";
}

std::string expected_minimal_same_module_global_sub_return_asm(const char* function_name) {
  return asm_header(function_name)
         + "    mov DWORD PTR [rip + v], 1\n"
           "    mov DWORD PTR [rip + v + 4], 2\n"
           "    mov eax, DWORD PTR [rip + v]\n"
           "    mov ecx, eax\n"
           "    mov eax, 3\n"
           "    sub eax, ecx\n"
           "    mov r11d, eax\n"
           "    mov ecx, eax\n"
           "    mov eax, DWORD PTR [rip + v + 4]\n"
           "    mov ecx, eax\n"
           "    mov eax, r11d\n"
           "    sub eax, ecx\n"
           "    mov r12d, eax\n"
           "    ret\n"
           ".bss\n.globl v\n.type v, @object\n.p2align 2\nv:\n"
           "    .byte 0\n"
           "    .byte 0\n"
           "    .byte 0\n"
           "    .byte 0\n"
           "    .byte 0\n"
           "    .byte 0\n"
           "    .byte 0\n"
           "    .byte 0\n";
}

bir::Module make_x86_named_same_module_global_return_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "x",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(0),
  });

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "x",
      .value = bir::Value::immediate_i32(0),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.x"),
      .global_name = "x",
      .align_bytes = 4,
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "loaded.x"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_named_same_module_global_sub_return_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "v",
      .type = bir::TypeKind::I8,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer_elements = {
          bir::Value::immediate_i8(0),
          bir::Value::immediate_i8(0),
          bir::Value::immediate_i8(0),
          bir::Value::immediate_i8(0),
          bir::Value::immediate_i8(0),
          bir::Value::immediate_i8(0),
          bir::Value::immediate_i8(0),
          bir::Value::immediate_i8(0),
      },
  });

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "v",
      .value = bir::Value::immediate_i32(1),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "v",
      .value = bir::Value::immediate_i32(2),
      .byte_offset = 4,
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.x"),
      .global_name = "v",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "first.diff"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(3),
      .rhs = bir::Value::named(bir::TypeKind::I32, "loaded.x"),
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.y"),
      .global_name = "v",
      .byte_offset = 4,
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "final.diff"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "first.diff"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "loaded.y"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "final.diff"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int check_add_one_prepared_move_bundle_contract() {
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_add_immediate_module(),
                                                        x86_target_profile());
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, "add_one");
  if (function_locations == nullptr) {
    return fail("minimal i32 parameter add-immediate route: missing prepared value-location function");
  }

  const auto* sum_home = prepare::find_prepared_value_home(prepared.names, *function_locations, "sum");
  if (sum_home == nullptr || sum_home->kind != prepare::PreparedValueHomeKind::Register ||
      !sum_home->register_name.has_value() ||
      c4c::backend::x86::abi::narrow_i32_register_name(*sum_home->register_name) != "r11d") {
    return fail("minimal i32 parameter add-immediate route: prepared value-location contract lost the canonical result home");
  }

  const auto* before_instruction = prepare::find_prepared_move_bundle(
      *function_locations, prepare::PreparedMovePhase::BeforeInstruction, 0, 0);
  if (before_instruction == nullptr || before_instruction->moves.size() != 1 ||
      before_instruction->moves.front().destination_kind != prepare::PreparedMoveDestinationKind::Value ||
      before_instruction->moves.front().destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      before_instruction->moves.front().to_value_id != sum_home->value_id) {
    return fail("minimal i32 parameter add-immediate route: prepared move-bundle contract lost the instruction-0 result-home move");
  }

  const auto* before_return = prepare::find_prepared_move_bundle(
      *function_locations, prepare::PreparedMovePhase::BeforeReturn, 0, 1);
  if (before_return == nullptr || before_return->moves.size() != 1 ||
      before_return->moves.front().destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      !before_return->moves.front().destination_register_name.has_value() ||
      c4c::backend::x86::abi::narrow_i32_register_name(
          *before_return->moves.front().destination_register_name) !=
          minimal_i32_return_register()) {
    return fail("minimal i32 parameter add-immediate route: prepared move-bundle contract lost the return ABI move");
  }

  return 0;
}

int check_id_i32_prepared_value_location_contract() {
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_passthrough_module(),
                                                        x86_target_profile());
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, "id_i32");
  if (function_locations == nullptr) {
    return fail("minimal i32 parameter passthrough route: missing prepared value-location function");
  }

  const auto* param_home =
      prepare::find_prepared_value_home(prepared.names, *function_locations, "p.x");
  if (param_home == nullptr || param_home->kind != prepare::PreparedValueHomeKind::Register ||
      !param_home->register_name.has_value() ||
      c4c::backend::x86::abi::narrow_i32_register_name(*param_home->register_name) !=
          minimal_i32_param_register()) {
    std::string detail =
        "minimal i32 parameter passthrough route: prepared value-location contract lost the canonical parameter home";
    if (param_home == nullptr) {
      detail += " (missing home)";
    } else {
      detail += " (kind=" + std::string(prepare::prepared_value_home_kind_name(param_home->kind));
      detail += ", register=";
      detail += param_home->register_name.has_value() ? *param_home->register_name : "<none>";
      detail += ")";
    }
    return fail(detail.c_str());
  }

  const auto* before_return = prepare::find_prepared_move_bundle(
      *function_locations, prepare::PreparedMovePhase::BeforeReturn, 0, 0);
  if (before_return == nullptr || before_return->moves.size() != 1 ||
      before_return->moves.front().destination_kind !=
          prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      before_return->moves.front().from_value_id != param_home->value_id ||
      !before_return->moves.front().destination_register_name.has_value() ||
      c4c::backend::x86::abi::narrow_i32_register_name(
          *before_return->moves.front().destination_register_name) !=
          minimal_i32_return_register()) {
    return fail("minimal i32 parameter passthrough route: prepared value-location contract lost the return move bundle");
  }

  return 0;
}

prepare::PreparedValueLocationFunction* find_mutable_prepared_value_location_function(
    prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_name_id =
      prepare::resolve_prepared_function_name_id(prepared.names, function_name);
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

int check_id_i32_requires_authoritative_prepared_return_bundle() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_passthrough_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "id_i32");
  if (function_locations == nullptr) {
    return fail("minimal i32 parameter passthrough route: missing prepared value-location function");
  }

  erase_prepared_move_bundle(*function_locations, prepare::PreparedMovePhase::BeforeReturn, 0, 0);

  try {
    static_cast<void>(c4c::backend::x86::api::emit_prepared_module(prepared));
  } catch (const std::invalid_argument& ex) {
    if (std::string(ex.what()).find("canonical prepared-module handoff") !=
        std::string::npos) {
      return 0;
    }
    return fail((std::string("minimal i32 parameter passthrough route: x86 prepared-module consumer rejected the missing prepared return bundle with the wrong contract message: ") +
                 ex.what())
                    .c_str());
  } catch (const std::exception& ex) {
    return fail((std::string("minimal i32 parameter passthrough route: x86 prepared-module consumer rejected the missing prepared return bundle with the wrong exception type: ") +
                 ex.what())
                    .c_str());
  }

  return fail("minimal i32 parameter passthrough route: x86 prepared-module consumer reopened a local return fallback when the authoritative prepared return bundle was removed");
}

int check_id_i32_requires_authoritative_prepared_return_home() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_passthrough_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "id_i32");
  if (function_locations == nullptr) {
    return fail("minimal i32 parameter passthrough route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("minimal i32 parameter passthrough route: missing prepared parameter name id");
  }
  function_locations->value_homes.erase(
      std::remove_if(function_locations->value_homes.begin(),
                     function_locations->value_homes.end(),
                     [&](const prepare::PreparedValueHome& home) {
                       return home.value_name == *value_name_id;
                     }),
      function_locations->value_homes.end());

  try {
    static_cast<void>(c4c::backend::x86::api::emit_prepared_module(prepared));
  } catch (const std::invalid_argument& ex) {
    if (std::string(ex.what()).find("canonical prepared-module handoff") !=
        std::string::npos) {
      return 0;
    }
    return fail((std::string("minimal i32 parameter passthrough route: x86 prepared-module consumer rejected the missing prepared return home with the wrong contract message: ") +
                 ex.what())
                    .c_str());
  } catch (const std::exception& ex) {
    return fail((std::string("minimal i32 parameter passthrough route: x86 prepared-module consumer rejected the missing prepared return home with the wrong exception type: ") +
                 ex.what())
                    .c_str());
  }

  return fail("minimal i32 parameter passthrough route: x86 prepared-module consumer reopened a local return fallback when the authoritative prepared home was removed");
}

int check_named_immediate_prepared_value_location_contract() {
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_named_immediate_add_module(),
                                                        x86_target_profile());
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, "const_add");
  if (function_locations == nullptr) {
    return fail("minimal named immediate-add route: missing prepared value-location function");
  }

  const auto* sum_home =
      prepare::find_prepared_value_home(prepared.names, *function_locations, "sum");
  if (sum_home == nullptr ||
      sum_home->kind != prepare::PreparedValueHomeKind::RematerializableImmediate ||
      sum_home->immediate_i32 != std::optional<std::int64_t>{42}) {
    return fail("minimal named immediate-add route: prepared value-location contract lost the rematerializable immediate home");
  }

  return 0;
}

int check_id_i32_stack_home_passthrough_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_passthrough_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail("stack-backed i32 parameter passthrough route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 parameter passthrough route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 parameter passthrough route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_minimal_stack_home_passthrough_asm("id_i32", 4, 0)) {
    return fail("stack-backed i32 parameter passthrough route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_add_one_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_add_immediate_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail("stack-backed i32 add-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 add-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 add-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("add_one", "add", 1, 4, 0)) {
    return fail("stack-backed i32 add-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_immediate_add_param_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_immediate_add_param_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail(
        "stack-backed i32 commuted add-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 commuted add-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 commuted add-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("add_one_commuted", "add", 1, 4, 0)) {
    return fail("stack-backed i32 commuted add-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_immediate_xor_param_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_immediate_xor_param_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail(
        "stack-backed i32 commuted xor-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 commuted xor-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 commuted xor-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("flip_masked_bits_commuted", "xor", 10, 4, 0)) {
    return fail("stack-backed i32 commuted xor-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_immediate_or_param_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_immediate_or_param_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail(
        "stack-backed i32 commuted or-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 commuted or-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 commuted or-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("set_low_bits_commuted", "or", 12, 4, 0)) {
    return fail("stack-backed i32 commuted or-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_mul_three_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_mul_immediate_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail("stack-backed i32 mul-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 mul-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 mul-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("mul_three", "imul", 3, 4, 0)) {
    return fail("stack-backed i32 mul-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_mask_low_bits_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_and_immediate_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail("stack-backed i32 and-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 and-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 and-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("mask_low_bits", "and", 15, 4, 0)) {
    return fail("stack-backed i32 and-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_shift_left_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_shl_immediate_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail("stack-backed i32 shl-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 shl-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 shl-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("shift_left_three", "shl", 3, 4, 0)) {
    return fail("stack-backed i32 shl-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_logical_shift_right_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_lshr_immediate_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail("stack-backed i32 lshr-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 lshr-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 lshr-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("shift_right_two", "shr", 2, 4, 0)) {
    return fail("stack-backed i32 lshr-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

int check_arithmetic_shift_right_stack_home_consumes_prepared_value_location_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_param_ashr_immediate_module(),
                                                        x86_target_profile());
  if (prepared.value_locations.functions.empty()) {
    return fail("stack-backed i32 ashr-immediate route: missing prepared value-location function");
  }

  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail("stack-backed i32 ashr-immediate route: missing prepared parameter name id");
  }

  auto& function_locations = prepared.value_locations.functions.front();
  prepare::PreparedValueHome* param_home = nullptr;
  for (auto& candidate : function_locations.value_homes) {
    if (candidate.value_name == *value_name_id) {
      param_home = &candidate;
      break;
    }
  }
  if (param_home == nullptr) {
    return fail("stack-backed i32 ashr-immediate route: missing prepared parameter home");
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm !=
      expected_minimal_stack_home_param_binary_asm("shift_right_signed", "sar", 3, 4, 0)) {
    return fail("stack-backed i32 ashr-immediate route: x86 prepared-module consumer stopped following authoritative prepared stack homes");
  }

  return 0;
}

std::string expected_minimal_param_add_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "add", immediate);
}

std::string expected_minimal_param_sub_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "sub", immediate);
}

std::string expected_minimal_param_mul_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "imul", immediate);
}

std::string expected_minimal_param_and_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "and", immediate);
}

std::string expected_minimal_param_or_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "or", immediate);
}

std::string expected_minimal_param_xor_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "xor", immediate);
}

std::string expected_minimal_param_shl_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "shl", immediate);
}

std::string expected_minimal_param_lshr_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "shr", immediate);
}

std::string expected_minimal_param_ashr_immediate_asm(const char* function_name, int immediate) {
  return expected_minimal_param_binary_asm(function_name, "sar", immediate);
}

bir::Module make_x86_return_constant_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(7)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_add_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "add_one";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "sum"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_immediate_add_param_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "add_one_commuted";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "sum"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_sub_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "sub_one";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "diff"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "diff"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_mul_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "mul_three";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Mul,
      .result = bir::Value::named(bir::TypeKind::I32, "product"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "product"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_and_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "mask_low_bits";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::And,
      .result = bir::Value::named(bir::TypeKind::I32, "masked"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(15),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "masked"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_xor_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "flip_masked_bits";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "flipped"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(10),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "flipped"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_immediate_or_param_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "set_low_bits_commuted";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Or,
      .result = bir::Value::named(bir::TypeKind::I32, "masked"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(12),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "masked"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_immediate_xor_param_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "flip_masked_bits_commuted";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "masked"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(10),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "masked"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_or_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "set_low_bits";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Or,
      .result = bir::Value::named(bir::TypeKind::I32, "masked"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(12),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "masked"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_shl_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "shift_left_three";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Shl,
      .result = bir::Value::named(bir::TypeKind::I32, "shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "shifted"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_lshr_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "shift_right_two";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::LShr,
      .result = bir::Value::named(bir::TypeKind::I32, "shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "shifted"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_ashr_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "shift_right_signed";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::AShr,
      .result = bir::Value::named(bir::TypeKind::I32, "shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "shifted"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_named_immediate_add_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "const_add";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(40),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "sum"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
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
  if (!prepared.module.functions.empty() && !prepared.module.functions.front().params.empty() &&
      !prepared.module.functions.front().params.front().is_varargs &&
      !prepared.module.functions.front().params.front().is_sret &&
      !prepared.module.functions.front().params.front().is_byval &&
      !prepared.module.functions.front().params.front().abi.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared x86 handoff no longer carries canonical parameter ABI metadata")
                    .c_str());
  }

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

  const auto public_asm = c4c::backend::emit_x86_bir_module_entry(module, target_profile);
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

}  // namespace

int run_backend_x86_handoff_boundary_scalar_smoke_tests() {
  // Keep the minimal scalar smoke lane isolated so Step 5 can keep shrinking
  // the monolithic handoff file around the remaining local guard and helper
  // ownership seams.
  if (const auto status = check_add_one_prepared_move_bundle_contract(); status != 0) {
    return status;
  }
  if (const auto status = check_id_i32_prepared_value_location_contract(); status != 0) {
    return status;
  }
  if (const auto status = check_named_immediate_prepared_value_location_contract(); status != 0) {
    return status;
  }
  if (const auto status =
          check_id_i32_stack_home_passthrough_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_add_one_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_immediate_add_param_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_immediate_or_param_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_immediate_xor_param_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status = check_mul_three_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_mask_low_bits_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_shift_left_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_logical_shift_right_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_arithmetic_shift_right_stack_home_consumes_prepared_value_location_contract();
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_return_constant_module(),
                              expected_minimal_constant_return_asm("main", 7),
                              "bir.func @main() -> i32 {",
                              "minimal immediate return route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_named_immediate_add_module(),
                              expected_minimal_constant_return_asm("const_add", 42),
                              "bir.func @const_add() -> i32 {",
                              "minimal named immediate-add route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_named_same_module_global_return_module(),
                              expected_minimal_same_module_global_named_return_asm("main"),
                              "loaded.x = bir.load_global i32 @x",
                              "minimal named same-module global return route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_named_same_module_global_sub_return_module(),
                              expected_minimal_same_module_global_sub_return_asm("main"),
                              "final.diff = bir.sub i32 first.diff, loaded.y",
                              "minimal named same-module global sub return route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_passthrough_module(),
                              expected_minimal_param_passthrough_asm("id_i32"),
                              "bir.func @id_i32(i32 p.x) -> i32 {",
                              "minimal i32 parameter passthrough route");
      status != 0) {
    return status;
  }
  if (const auto status = check_id_i32_requires_authoritative_prepared_return_bundle();
      status != 0) {
    return status;
  }
  if (const auto status = check_id_i32_requires_authoritative_prepared_return_home();
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_add_immediate_module(),
                              expected_minimal_param_add_immediate_asm("add_one", 1),
                              "bir.func @add_one(i32 p.x) -> i32 {",
                              "minimal i32 parameter add-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_immediate_add_param_module(),
                              expected_minimal_param_add_immediate_asm("add_one_commuted", 1),
                              "bir.func @add_one_commuted(i32 p.x) -> i32 {",
                              "minimal i32 commuted add-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_sub_immediate_module(),
                              expected_minimal_param_sub_immediate_asm("sub_one", 1),
                              "bir.func @sub_one(i32 p.x) -> i32 {",
                              "minimal i32 parameter sub-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_mul_immediate_module(),
                              expected_minimal_param_mul_immediate_asm("mul_three", 3),
                              "bir.func @mul_three(i32 p.x) -> i32 {",
                              "minimal i32 parameter mul-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_and_immediate_module(),
                              expected_minimal_param_and_immediate_asm("mask_low_bits", 15),
                              "bir.func @mask_low_bits(i32 p.x) -> i32 {",
                              "minimal i32 parameter and-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_or_immediate_module(),
                              expected_minimal_param_or_immediate_asm("set_low_bits", 12),
                              "bir.func @set_low_bits(i32 p.x) -> i32 {",
                              "minimal i32 parameter or-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_immediate_or_param_module(),
                              expected_minimal_param_or_immediate_asm("set_low_bits_commuted", 12),
                              "bir.func @set_low_bits_commuted(i32 p.x) -> i32 {",
                              "minimal i32 commuted or-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_xor_immediate_module(),
                              expected_minimal_param_xor_immediate_asm("flip_masked_bits", 10),
                              "bir.func @flip_masked_bits(i32 p.x) -> i32 {",
                              "minimal i32 parameter xor-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_immediate_xor_param_module(),
                              expected_minimal_param_xor_immediate_asm(
                                  "flip_masked_bits_commuted", 10),
                              "bir.func @flip_masked_bits_commuted(i32 p.x) -> i32 {",
                              "minimal i32 commuted xor-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_shl_immediate_module(),
                              expected_minimal_param_shl_immediate_asm("shift_left_three", 3),
                              "bir.func @shift_left_three(i32 p.x) -> i32 {",
                              "minimal i32 parameter shl-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_lshr_immediate_module(),
                              expected_minimal_param_lshr_immediate_asm("shift_right_two", 2),
                              "bir.func @shift_right_two(i32 p.x) -> i32 {",
                              "minimal i32 parameter logical-right-shift-immediate route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_param_ashr_immediate_module(),
                              expected_minimal_param_ashr_immediate_asm("shift_right_signed", 3),
                              "bir.func @shift_right_signed(i32 p.x) -> i32 {",
                              "minimal i32 parameter arithmetic-right-shift-immediate route");
      status != 0) {
    return status;
  }

  return 0;
}
