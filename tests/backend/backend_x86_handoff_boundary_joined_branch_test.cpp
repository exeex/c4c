#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir/lowering.hpp"
#include "src/backend/mir/x86/abi/abi.hpp"
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <cstdlib>
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

const prepare::PreparedControlFlowFunction* find_control_flow_function(
    const prepare::PreparedBirModule& prepared,
    const char* function_name) {
  for (const auto& function : prepared.control_flow.functions) {
    if (prepare::prepared_function_name(prepared.names, function.function_name) ==
        function_name) {
      return &function;
    }
  }
  return nullptr;
}

prepare::PreparedControlFlowFunction* find_control_flow_function(prepare::PreparedBirModule& prepared,
                                                                 const char* function_name) {
  for (auto& function : prepared.control_flow.functions) {
    if (prepare::prepared_function_name(prepared.names, function.function_name) ==
        function_name) {
      return &function;
    }
  }
  return nullptr;
}

bir::Block* find_block(bir::Function& function, const char* label) {
  for (auto& block : function.blocks) {
    if (block.label == label) {
      return &block;
    }
  }
  return nullptr;
}

std::string_view block_label(const prepare::PreparedBirModule& prepared,
                             c4c::BlockLabelId label) {
  return prepare::prepared_block_label(prepared.names, label);
}

c4c::BlockLabelId intern_block_label(prepare::PreparedBirModule& prepared,
                                     std::string_view label) {
  return prepared.names.block_labels.intern(label);
}

c4c::BlockLabelId find_block_label_id(const prepare::PreparedBirModule& prepared,
                                      std::string_view label) {
  return prepared.names.block_labels.find(label);
}

c4c::SlotNameId intern_slot_name(prepare::PreparedBirModule& prepared,
                                 std::string_view name) {
  return prepared.names.slot_names.intern(name);
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

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
}

std::string expected_zero_branch_prefix(const char* function_name,
                                        const char* false_label,
                                        const char* false_branch_opcode) {
  const auto param_register = minimal_i32_param_register();
  return asm_header(function_name) + "    test " + param_register + ", " + param_register +
         "\n    " + false_branch_opcode + " .L" + function_name + "_" + false_label + "\n";
}

std::string expected_branch_prefix(const char* function_name, const char* false_label) {
  return expected_zero_branch_prefix(function_name, false_label, "jne");
}

std::string expected_branch_prefix_with_source(const char* function_name,
                                               const char* false_label,
                                               const char* source_register) {
  return asm_header(function_name) + "    test " + std::string(source_register) + ", " +
         std::string(source_register) + "\n    jne .L" + function_name + "_" + false_label + "\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    add " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_with_source_asm(
    const char* function_name,
    const char* false_label,
    const char* source_register,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix_with_source(function_name, false_label, source_register) + "    mov " +
         minimal_i32_return_register() + ", " + source_register + "\n    add " +
         minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + source_register + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
    const char* function_name,
    const char* false_label,
    int true_selected_immediate,
    int false_selected_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + std::to_string(true_selected_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + std::to_string(false_selected_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
    const char* function_name,
    const char* false_label,
    const char* true_global_name,
    const char* false_global_name,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", DWORD PTR [rip + " + std::string(true_global_name) +
         "]\n    xor " + minimal_i32_return_register() + ", " +
         std::to_string(joined_immediate) + "\n    ret\n.L" + function_name + "_" +
         false_label + ":\n    mov " + minimal_i32_return_register() + ", DWORD PTR [rip + " +
         std::string(false_global_name) + "]\n    xor " + minimal_i32_return_register() + ", " +
         std::to_string(joined_immediate) + "\n    ret\n.data\n.globl " +
         std::string(true_global_name) + "\n.type " + std::string(true_global_name) +
         ", @object\n.p2align 2\n" + std::string(true_global_name) + ":\n    .long 5\n.globl " +
         std::string(false_global_name) + "\n.type " + std::string(false_global_name) +
         ", @object\n.p2align 2\n" + std::string(false_global_name) + ":\n    .long 1\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
    const char* function_name,
    const char* false_label,
    const char* true_global_name,
    int true_chain_immediate,
    const char* false_global_name,
    int false_chain_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", DWORD PTR [rip + " + std::string(true_global_name) +
         "]\n    add " + minimal_i32_return_register() + ", " +
         std::to_string(true_chain_immediate) + "\n    xor " + minimal_i32_return_register() +
         ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", DWORD PTR [rip + " +
         std::string(false_global_name) + "]\n    sub " + minimal_i32_return_register() + ", " +
         std::to_string(false_chain_immediate) + "\n    xor " + minimal_i32_return_register() +
         ", " + std::to_string(joined_immediate) +
         "\n    ret\n.data\n.globl " + std::string(true_global_name) + "\n.type " +
         std::string(true_global_name) + ", @object\n.p2align 2\n" +
         std::string(true_global_name) + ":\n    .long 5\n.globl " +
         std::string(false_global_name) + "\n.type " + std::string(false_global_name) +
         ", @object\n.p2align 2\n" + std::string(false_global_name) + ":\n    .long 1\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
    const char* function_name,
    const char* false_label,
    const char* true_root_name,
    const char* true_global_name,
    const char* false_root_name,
    const char* false_global_name,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov rax, QWORD PTR [rip + " +
         std::string(true_root_name) + "]\n    mov " + minimal_i32_return_register() +
         ", DWORD PTR [rip + " + std::string(true_global_name) + "]\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov rax, QWORD PTR [rip + " +
         std::string(false_root_name) + "]\n    mov " + minimal_i32_return_register() +
         ", DWORD PTR [rip + " + std::string(false_global_name) + "]\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.data\n.globl " + std::string(true_root_name) + "\n.type " +
         std::string(true_root_name) + ", @object\n.p2align 3\n" + std::string(true_root_name) +
         ":\n    # global data emission deferred to behavior-recovery packet\n.globl " +
         std::string(true_global_name) + "\n.type " + std::string(true_global_name) +
         ", @object\n.p2align 2\n" + std::string(true_global_name) + ":\n    .long 5\n.globl " +
         std::string(false_root_name) + "\n.type " + std::string(false_root_name) +
         ", @object\n.p2align 3\n" + std::string(false_root_name) +
         ":\n    # global data emission deferred to behavior-recovery packet\n.globl " +
         std::string(false_global_name) +
         "\n.type " + std::string(false_global_name) + ", @object\n.p2align 2\n" +
         std::string(false_global_name) + ":\n    .long 1\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
    const char* function_name,
    const char* false_label,
    const char* true_root_name,
    const char* true_global_name,
    int true_chain_immediate,
    const char* false_root_name,
    const char* false_global_name,
    int false_chain_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov rax, QWORD PTR [rip + " +
         std::string(true_root_name) + "]\n    mov " + minimal_i32_return_register() +
         ", DWORD PTR [rip + " + std::string(true_global_name) + "]\n    add " +
         minimal_i32_return_register() + ", " + std::to_string(true_chain_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " +
         std::to_string(joined_immediate) + "\n    ret\n.L" + function_name + "_" +
         false_label + ":\n    mov rax, QWORD PTR [rip + " + std::string(false_root_name) +
         "]\n    mov " + minimal_i32_return_register() + ", DWORD PTR [rip + " +
         std::string(false_global_name) + "]\n    sub " + minimal_i32_return_register() +
         ", " + std::to_string(false_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.data\n.globl " + std::string(true_root_name) + "\n.type " +
         std::string(true_root_name) + ", @object\n.p2align 3\n" + std::string(true_root_name) +
         ":\n    # global data emission deferred to behavior-recovery packet\n.globl " +
         std::string(true_global_name) + "\n.type " + std::string(true_global_name) +
         ", @object\n.p2align 2\n" + std::string(true_global_name) + ":\n    .long 5\n.globl " +
         std::string(false_root_name) + "\n.type " + std::string(false_root_name) +
         ", @object\n.p2align 3\n" + std::string(false_root_name) +
         ":\n    # global data emission deferred to behavior-recovery packet\n.globl " +
         std::string(false_global_name) +
         "\n.type " + std::string(false_global_name) + ", @object\n.p2align 2\n" +
         std::string(false_global_name) + ":\n    .long 1\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
    const char* function_name,
    const char* false_label,
    const char* true_root_name,
    const char* true_global_name,
    int true_global_offset,
    const char* false_root_name,
    const char* false_global_name,
    int false_global_offset,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov rax, QWORD PTR [rip + " +
         std::string(true_root_name) + "]\n    mov " + minimal_i32_return_register() +
         ", DWORD PTR [rip + " + std::string(true_global_name) + " + " +
         std::to_string(true_global_offset) + "]\n    xor " + minimal_i32_return_register() +
         ", " + std::to_string(joined_immediate) + "\n    ret\n.L" + function_name + "_" +
         false_label + ":\n    mov rax, QWORD PTR [rip + " + std::string(false_root_name) +
         "]\n    mov " + minimal_i32_return_register() + ", DWORD PTR [rip + " +
         std::string(false_global_name) + " + " + std::to_string(false_global_offset) +
         "]\n    xor " + minimal_i32_return_register() + ", " +
         std::to_string(joined_immediate) + "\n    ret\n.data\n.globl " +
         std::string(true_root_name) + "\n.type " + std::string(true_root_name) +
         ", @object\n.p2align 3\n" + std::string(true_root_name) +
         ":\n    # global data emission deferred to behavior-recovery packet\n.globl " +
         std::string(true_global_name) +
         "\n.type " + std::string(true_global_name) + ", @object\n.p2align 2\n" +
         std::string(true_global_name) + ":\n    .long 0\n    .long 5\n.globl " +
         std::string(false_root_name) + "\n.type " + std::string(false_root_name) +
         ", @object\n.p2align 3\n" + std::string(false_root_name) +
         ":\n    # global data emission deferred to behavior-recovery packet\n.globl " +
         std::string(false_global_name) +
         "\n.type " + std::string(false_global_name) + ", @object\n.p2align 2\n" +
         std::string(false_global_name) + ":\n    .long 0\n    .long 1\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
    const char* function_name,
    const char* false_label,
    const char* true_root_name,
    const char* true_global_name,
    int true_global_offset,
    int true_chain_immediate,
    const char* false_root_name,
    const char* false_global_name,
    int false_global_offset,
    int false_chain_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov rax, QWORD PTR [rip + " +
         std::string(true_root_name) + "]\n    mov " + minimal_i32_return_register() +
         ", DWORD PTR [rip + " + std::string(true_global_name) + " + " +
         std::to_string(true_global_offset) + "]\n    add " + minimal_i32_return_register() +
         ", " + std::to_string(true_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov rax, QWORD PTR [rip + " +
         std::string(false_root_name) + "]\n    mov " + minimal_i32_return_register() +
         ", DWORD PTR [rip + " + std::string(false_global_name) + " + " +
         std::to_string(false_global_offset) + "]\n    sub " + minimal_i32_return_register() +
         ", " + std::to_string(false_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.data\n.globl " + std::string(true_root_name) + "\n.type " +
         std::string(true_root_name) + ", @object\n.p2align 3\n" +
         std::string(true_root_name) +
         ":\n    # global data emission deferred to behavior-recovery packet\n.globl " +
         std::string(true_global_name) + "\n.type " +
         std::string(true_global_name) + ", @object\n.p2align 2\n" +
         std::string(true_global_name) + ":\n    .long 0\n    .long 5\n.globl " +
         std::string(false_root_name) + "\n.type " + std::string(false_root_name) +
         ", @object\n.p2align 3\n" + std::string(false_root_name) +
         ":\n    # global data emission deferred to behavior-recovery packet\n.globl " +
         std::string(false_global_name) +
         "\n.type " + std::string(false_global_name) + ", @object\n.p2align 2\n" +
         std::string(false_global_name) + ":\n    .long 0\n    .long 1\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
    const char* function_name,
    const char* false_label,
    const char* true_global_name,
    int true_global_offset,
    const char* false_global_name,
    int false_global_offset,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", DWORD PTR [rip + " + std::string(true_global_name) +
         " + " + std::to_string(true_global_offset) + "]\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", DWORD PTR [rip + " +
         std::string(false_global_name) + " + " + std::to_string(false_global_offset) +
         "]\n    xor " + minimal_i32_return_register() + ", " +
         std::to_string(joined_immediate) + "\n    ret\n.data\n.globl " +
         std::string(true_global_name) + "\n.type " + std::string(true_global_name) +
         ", @object\n.p2align 2\n" + std::string(true_global_name) +
         ":\n    .long 0\n    .long 5\n.globl " + std::string(false_global_name) +
         "\n.type " + std::string(false_global_name) + ", @object\n.p2align 2\n" +
         std::string(false_global_name) + ":\n    .long 0\n    .long 1\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
    const char* function_name,
    const char* false_label,
    const char* true_global_name,
    int true_global_offset,
    int true_chain_immediate,
    const char* false_global_name,
    int false_global_offset,
    int false_chain_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", DWORD PTR [rip + " + std::string(true_global_name) +
         " + " + std::to_string(true_global_offset) + "]\n    add " +
         minimal_i32_return_register() + ", " + std::to_string(true_chain_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " +
         std::to_string(joined_immediate) + "\n    ret\n.L" + function_name + "_" +
         false_label + ":\n    mov " + minimal_i32_return_register() + ", DWORD PTR [rip + " +
         std::string(false_global_name) + " + " + std::to_string(false_global_offset) +
         "]\n    sub " + minimal_i32_return_register() + ", " +
         std::to_string(false_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.data\n.globl " + std::string(true_global_name) + "\n.type " +
         std::string(true_global_name) + ", @object\n.p2align 2\n" +
         std::string(true_global_name) + ":\n    .long 0\n    .long 5\n.globl " +
         std::string(false_global_name) + "\n.type " + std::string(false_global_name) +
         ", @object\n.p2align 2\n" + std::string(false_global_name) +
         ":\n    .long 0\n    .long 1\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
    const char* function_name,
    const char* false_label,
    int true_base_immediate,
    int true_chain_immediate,
    int false_base_immediate,
    int false_chain_immediate,
    int selected_chain_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + std::to_string(true_base_immediate) +
         "\n    add " + minimal_i32_return_register() + ", " +
         std::to_string(true_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " +
         std::to_string(selected_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + std::to_string(false_base_immediate) +
         "\n    sub " + minimal_i32_return_register() + ", " +
         std::to_string(false_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " +
         std::to_string(selected_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_chained_add_or_sub_then_xor_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int selected_chain_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " +
         std::to_string(selected_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " +
         std::to_string(selected_chain_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    and " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    and " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    or " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    or " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    imul " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    imul " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    shl " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    shl " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    shr " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    shr " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    sar " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    sar " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) + "\n    ret\n";
}

std::string expected_minimal_param_ne_zero_branch_joined_add_or_sub_asm(
    const char* function_name,
    const char* false_label,
    int true_immediate,
    int false_immediate) {
  return expected_zero_branch_prefix(function_name, false_label, "je") + "    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() +
         "\n    sub " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    add " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    ret\n";
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_ne_zero_branch_joined_sub_or_add_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_nonzero";
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
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_nonzero",
      .false_label = "is_zero",
  };

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
      },
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_then_add";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(2),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_then_xor";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(3),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_immediates_then_xor_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_immediate_then_xor";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::immediate_i32(5),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::immediate_i32(1),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(3),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_globals_then_xor_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "selected_zero",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(5),
  });
  module.globals.push_back(bir::Global{
      .name = "selected_nonzero",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(1),
  });

  bir::Function function;
  function.name = "branch_join_global_then_xor";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::immediate_i32(5),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::immediate_i32(1),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(3),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "selected_zero_root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("selected_zero_backing"),
  });
  module.globals.push_back(bir::Global{
      .name = "selected_zero_backing",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(5),
  });
  module.globals.push_back(bir::Global{
      .name = "selected_nonzero_root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("selected_nonzero_backing"),
  });
  module.globals.push_back(bir::Global{
      .name = "selected_nonzero_backing",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(1),
  });

  bir::Function function;
  function.name = "branch_join_pointer_backed_global_then_xor";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::immediate_i32(5),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::immediate_i32(1),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(3),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "selected_zero_pair",
      .type = bir::TypeKind::I32,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer_elements = {bir::Value::immediate_i32(0), bir::Value::immediate_i32(5)},
  });
  module.globals.push_back(bir::Global{
      .name = "selected_nonzero_pair",
      .type = bir::TypeKind::I32,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer_elements = {bir::Value::immediate_i32(0), bir::Value::immediate_i32(1)},
  });

  bir::Function function;
  function.name = "branch_join_offset_global_then_xor";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::immediate_i32(5),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::immediate_i32(1),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(3),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "selected_zero_root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("selected_zero_pair"),
  });
  module.globals.push_back(bir::Global{
      .name = "selected_zero_pair",
      .type = bir::TypeKind::I32,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer_elements = {bir::Value::immediate_i32(0), bir::Value::immediate_i32(5)},
  });
  module.globals.push_back(bir::Global{
      .name = "selected_nonzero_root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("selected_nonzero_pair"),
  });
  module.globals.push_back(bir::Global{
      .name = "selected_nonzero_pair",
      .type = bir::TypeKind::I32,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer_elements = {bir::Value::immediate_i32(0), bir::Value::immediate_i32(1)},
  });

  bir::Function function;
  function.name = "branch_join_offset_pointer_backed_global_then_xor";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::immediate_i32(5),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::immediate_i32(1),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Xor,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(3),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_then_and";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::And,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(15),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_then_or";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Or,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(12),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_then_mul";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Mul,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(3),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_then_shl";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Shl,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(2),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_then_lshr";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::LShr,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(2),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_join_adjust_then_ashr";
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
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::AShr,
      .result = bir::Value::named(bir::TypeKind::I32, "joined"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(2),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "joined"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));
  return module;
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

void refresh_prepared_i32_return_bundle(prepare::PreparedBirModule& prepared,
                                        const c4c::TargetProfile& target_profile,
                                        bir::Function& function,
                                        const bir::Block& return_block,
                                        std::string_view function_name) {
  if (return_block.terminator.kind != bir::TerminatorKind::Return ||
      !return_block.terminator.value.has_value() ||
      return_block.terminator.value->kind != bir::Value::Kind::Named) {
    return;
  }

  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, function_name);
  if (function_locations == nullptr) {
    return;
  }

  const auto return_value_name =
      prepared.names.value_names.intern(return_block.terminator.value->name);
  auto* return_home =
      find_mutable_prepared_value_home(
          prepared, *function_locations, return_block.terminator.value->name);
  if (return_home == nullptr) {
    prepare::PreparedValueId next_value_id = 0;
    for (const auto& home : function_locations->value_homes) {
      next_value_id = std::max(next_value_id, home.value_id + 1);
    }
    function_locations->value_homes.push_back(prepare::PreparedValueHome{
        .value_id = next_value_id,
        .function_name = function_locations->function_name,
        .value_name = return_value_name,
        .kind = prepare::PreparedValueHomeKind::Register,
        .register_name = minimal_i32_return_register(),
    });
    return_home = &function_locations->value_homes.back();
  }

  std::optional<std::size_t> return_block_index;
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    if (&function.blocks[block_index] == &return_block) {
      return_block_index = block_index;
      break;
    }
  }
  if (!return_block_index.has_value()) {
    return;
  }

  const auto return_register =
      function.return_abi.has_value()
          ? prepare::call_result_destination_register_name(target_profile, *function.return_abi)
          : std::optional<std::string>{minimal_i32_return_register()};
  if (!return_register.has_value()) {
    return;
  }

  auto return_bundle_it =
      std::find_if(function_locations->move_bundles.begin(),
                   function_locations->move_bundles.end(),
                   [](const prepare::PreparedMoveBundle& bundle) {
                     return bundle.phase == prepare::PreparedMovePhase::BeforeReturn;
                   });
  if (return_bundle_it == function_locations->move_bundles.end()) {
    function_locations->move_bundles.push_back(prepare::PreparedMoveBundle{
        .function_name = function_locations->function_name,
        .phase = prepare::PreparedMovePhase::BeforeReturn,
    });
    return_bundle_it = function_locations->move_bundles.end() - 1;
  }

  return_bundle_it->function_name = function_locations->function_name;
  return_bundle_it->phase = prepare::PreparedMovePhase::BeforeReturn;
  return_bundle_it->authority_kind = prepare::PreparedMoveAuthorityKind::None;
  return_bundle_it->block_index = *return_block_index;
  return_bundle_it->instruction_index = return_block.insts.size();
  return_bundle_it->source_parallel_copy_predecessor_label.reset();
  return_bundle_it->source_parallel_copy_successor_label.reset();
  return_bundle_it->moves = {
      prepare::PreparedMoveResolution{
          .from_value_id = return_home->value_id,
          .to_value_id = return_home->value_id,
          .destination_kind = prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_register_name = return_register,
          .block_index = *return_block_index,
          .instruction_index = return_block.insts.size(),
          .coalesced_by_assigned_storage =
              return_home->kind == prepare::PreparedValueHomeKind::Register &&
              return_home->register_name == return_register,
          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          .authority_kind = prepare::PreparedMoveAuthorityKind::None,
          .reason = "return_register_to_register",
      },
  };
}

int check_route_outputs(const bir::Module& module,
                        const std::string& expected_asm,
                        const std::string& expected_bir_fragment,
                        const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(
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

int check_join_route_consumes_prepared_control_flow_impl(const bir::Module& module,
                                                         const std::string& expected_asm,
                                                         const char* function_name,
                                                         const char* failure_context,
                                                         bool use_edge_store_slot_carrier,
                                                         bool use_selected_value_chain,
                                                         bool use_immediate_selected_value_chain,
                                                         bool use_global_selected_value_roots,
                                                         bool use_global_selected_value_chain,
                                                         bool use_fixed_offset_global_selected_value_roots,
                                                         bool use_pointer_backed_global_selected_value_roots,
                                                         bool add_unreachable_block,
                                                         bool add_true_lane_passthrough_block = false,
                                                         bool add_false_lane_passthrough_block =
                                                             false) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the joined branch control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "join");
  if (entry_block == nullptr || join_block == nullptr || entry_block->insts.empty() ||
      join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared joined branch fixture no longer has the expected entry/join shape")
                    .c_str());
  }

  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.front());
  std::size_t join_select_index = join_block->insts.size() - 1;
  auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
  if (join_select == nullptr && join_block->insts.size() >= 2) {
    join_select_index = join_block->insts.size() - 2;
    join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
  }
  if (entry_compare == nullptr || join_select == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared joined branch fixture no longer exposes the bounded compare/select carrier")
                    .c_str());
  }
  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.size() != 1 ||
      mutable_control_flow->branch_conditions.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared joined branch fixture lost its mutable control-flow contract")
                    .c_str());
  }
  auto& branch_condition = mutable_control_flow->branch_conditions.front();
  auto& join_transfer = mutable_control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() < 2 ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value() ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared joined branch contract no longer publishes authoritative true/false join ownership")
                    .c_str());
  }
  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  const auto join_block_label_id = join_transfer.join_block_label;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared joined branch contract published invalid true/false join ownership indices")
                    .c_str());
  }
  const auto original_true_edge_predecessor =
      join_transfer.edge_transfers[true_transfer_index].predecessor_label;
  const auto original_false_edge_predecessor =
      join_transfer.edge_transfers[false_transfer_index].predecessor_label;

  const std::string renamed_true_label = "carrier.zero";
  const std::string renamed_false_label = "carrier.nonzero";
  const auto original_true_label_id = branch_condition.true_label;
  const auto original_false_label_id = branch_condition.false_label;
  const std::string original_false_label = entry_block->terminator.false_label;
  auto* true_carrier = find_block(function, entry_block->terminator.true_label.c_str());
  auto* false_carrier = find_block(function, entry_block->terminator.false_label.c_str());
  if (true_carrier == nullptr || false_carrier == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared joined branch fixture no longer exposes both carrier blocks")
                    .c_str());
  }
  true_carrier->label = renamed_true_label;
  false_carrier->label = renamed_false_label;
  true_carrier->label_id = prepared.module.names.block_labels.intern(renamed_true_label);
  false_carrier->label_id = prepared.module.names.block_labels.intern(renamed_false_label);
  branch_condition.true_label = intern_block_label(prepared, renamed_true_label);
  branch_condition.false_label = intern_block_label(prepared, renamed_false_label);
  for (auto& prepared_block : mutable_control_flow->blocks) {
    if (prepared_block.block_label == original_true_label_id) {
      prepared_block.block_label = branch_condition.true_label;
    } else if (prepared_block.block_label == original_false_label_id) {
      prepared_block.block_label = branch_condition.false_label;
    }
    if (prepared_block.true_label == original_true_label_id) {
      prepared_block.true_label = branch_condition.true_label;
    }
    if (prepared_block.false_label == original_false_label_id) {
      prepared_block.false_label = branch_condition.false_label;
    }
    if (prepared_block.branch_target_label == original_true_label_id) {
      prepared_block.branch_target_label = branch_condition.true_label;
    } else if (prepared_block.branch_target_label == original_false_label_id) {
      prepared_block.branch_target_label = branch_condition.false_label;
    }
  }
  entry_block->terminator.true_label = "carrier.raw.zero";
  entry_block->terminator.false_label = "carrier.raw.nonzero";

  join_transfer.source_true_incoming_label = intern_block_label(prepared, "contract.true_lane");
  join_transfer.source_false_incoming_label = intern_block_label(prepared, "contract.false_lane");
  join_transfer.edge_transfers[true_transfer_index].predecessor_label =
      *join_transfer.source_true_incoming_label;
  join_transfer.edge_transfers[false_transfer_index].predecessor_label =
      *join_transfer.source_false_incoming_label;
  for (auto& bundle : mutable_control_flow->parallel_copy_bundles) {
    if (bundle.predecessor_label == original_true_edge_predecessor &&
        bundle.successor_label == join_transfer.join_block_label) {
      bundle.predecessor_label = *join_transfer.source_true_incoming_label;
      bundle.execution_block_label = branch_condition.true_label;
    } else if (bundle.predecessor_label == original_false_edge_predecessor &&
               bundle.successor_label == join_transfer.join_block_label) {
      bundle.predecessor_label = *join_transfer.source_false_incoming_label;
      bundle.execution_block_label = branch_condition.false_label;
    }
  }
  if (auto* function_locations =
          find_mutable_prepared_value_location_function(prepared, function_name);
      function_locations != nullptr) {
    for (auto& bundle : function_locations->move_bundles) {
      if (bundle.source_parallel_copy_successor_label != join_transfer.join_block_label) {
        continue;
      }
      if (bundle.source_parallel_copy_predecessor_label == original_true_edge_predecessor) {
        bundle.source_parallel_copy_predecessor_label =
            *join_transfer.source_true_incoming_label;
      } else if (bundle.source_parallel_copy_predecessor_label ==
                 original_false_edge_predecessor) {
        bundle.source_parallel_copy_predecessor_label =
            *join_transfer.source_false_incoming_label;
      } else {
        continue;
      }
      for (auto& move : bundle.moves) {
        if (move.source_parallel_copy_successor_label != join_transfer.join_block_label) {
          continue;
        }
        if (move.source_parallel_copy_predecessor_label == original_true_edge_predecessor) {
          move.source_parallel_copy_predecessor_label =
              *join_transfer.source_true_incoming_label;
        } else if (move.source_parallel_copy_predecessor_label ==
                   original_false_edge_predecessor) {
          move.source_parallel_copy_predecessor_label =
              *join_transfer.source_false_incoming_label;
        }
      }
    }
  }
  join_transfer.incomings.push_back(
      bir::PhiIncoming{.label = "contract.extra_lane", .value = bir::Value::immediate_i32(77)});
  join_transfer.edge_transfers.push_back(prepare::PreparedEdgeValueTransfer{
      .predecessor_label = intern_block_label(prepared, "contract.extra_lane"),
      .successor_label = join_transfer.join_block_label,
      .incoming_value = bir::Value::immediate_i32(77),
      .destination_value = join_transfer.result,
      .storage_name = std::nullopt,
  });
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == block_label(prepared, *join_transfer.source_true_incoming_label) ||
        incoming.label == renamed_true_label) {
      incoming.label = std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
    } else if (incoming.label == block_label(prepared, *join_transfer.source_false_incoming_label) ||
               incoming.label == renamed_false_label) {
      incoming.label =
          std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
    }
  }

  entry_compare->opcode = bir::BinaryOpcode::Ne;
  entry_compare->lhs = bir::Value::immediate_i32(9);
  entry_compare->rhs = bir::Value::immediate_i32(3);
  const std::string original_carrier_result_name = join_select->result.name;
  const auto renamed_carrier_result =
      bir::Value::named(bir::TypeKind::I32, "carrier.join.contract.result");
  if (auto* function_locations =
          find_mutable_prepared_value_location_function(prepared, function_name);
      function_locations != nullptr) {
    if (auto* result_home = find_mutable_prepared_value_home(
            prepared, *function_locations, original_carrier_result_name);
        result_home != nullptr) {
      result_home->value_name = prepared.names.value_names.intern(renamed_carrier_result.name);
    }
  }
  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.join.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    join_block->insts[join_select_index] = bir::LoadLocalInst{
        .result = renamed_carrier_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
  } else {
    join_select->result = renamed_carrier_result;
    join_select->predicate = bir::BinaryOpcode::Ne;
    join_select->lhs = bir::Value::immediate_i32(4);
    join_select->rhs = bir::Value::immediate_i32(1);
  }
  for (auto& inst : join_block->insts) {
    if (auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
      if (binary->lhs.kind == bir::Value::Kind::Named &&
          binary->lhs.name == original_carrier_result_name) {
        binary->lhs = renamed_carrier_result;
      }
      if (binary->rhs.kind == bir::Value::Kind::Named &&
          binary->rhs.name == original_carrier_result_name) {
        binary->rhs = renamed_carrier_result;
      }
    }
  }
  if (join_block->terminator.value.has_value() &&
      join_block->terminator.value->kind == bir::Value::Kind::Named &&
      join_block->terminator.value->name == original_carrier_result_name) {
    join_block->terminator.value = renamed_carrier_result;
  }
  const auto current_true_label_id = branch_condition.true_label;
  const auto current_false_label_id = branch_condition.false_label;
  if (join_block->terminator.kind == bir::TerminatorKind::Return &&
      join_block->terminator.value.has_value() &&
      join_block->terminator.value->kind == bir::Value::Kind::Named) {
    auto* function_locations =
        find_mutable_prepared_value_location_function(prepared, function_name);
    auto* return_home =
        function_locations == nullptr
            ? nullptr
            : find_mutable_prepared_value_home(
                  prepared, *function_locations, join_block->terminator.value->name);
    const auto return_value_name =
        prepared.names.value_names.intern(join_block->terminator.value->name);
    if (function_locations != nullptr && return_home == nullptr) {
      prepare::PreparedValueId next_value_id = 0;
      for (const auto& home : function_locations->value_homes) {
        next_value_id = std::max(next_value_id, home.value_id + 1);
      }
      function_locations->value_homes.push_back(prepare::PreparedValueHome{
          .value_id = next_value_id,
          .function_name = function_locations->function_name,
          .value_name = return_value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = minimal_i32_return_register(),
      });
      return_home = &function_locations->value_homes.back();
    }
    std::optional<std::size_t> join_block_index;
    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      if (&function.blocks[block_index] == join_block) {
        join_block_index = block_index;
        break;
      }
    }
    if (function_locations != nullptr && return_home != nullptr &&
        join_block_index.has_value() &&
        prepare::find_prepared_move_bundle(*function_locations,
                                           prepare::PreparedMovePhase::BeforeReturn,
                                           *join_block_index,
                                           join_block->insts.size()) == nullptr &&
        function.return_abi.has_value()) {
      const auto return_register =
          prepare::call_result_destination_register_name(target_profile, *function.return_abi);
      if (return_register.has_value()) {
        function_locations->move_bundles.push_back(prepare::PreparedMoveBundle{
            .function_name = function_locations->function_name,
            .phase = prepare::PreparedMovePhase::BeforeReturn,
            .block_index = *join_block_index,
            .instruction_index = join_block->insts.size(),
            .moves =
                {
                    prepare::PreparedMoveResolution{
                        .from_value_id = return_home->value_id,
                        .to_value_id = return_home->value_id,
                        .destination_kind =
                            prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                        .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                        .destination_register_name = return_register,
                        .block_index = *join_block_index,
                        .instruction_index = join_block->insts.size(),
                        .coalesced_by_assigned_storage =
                            return_home->kind == prepare::PreparedValueHomeKind::Register &&
                            return_home->register_name == return_register,
                        .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                        .authority_kind = prepare::PreparedMoveAuthorityKind::None,
                        .reason = "return_register_to_register",
                    },
                },
        });
      }
    }
  }

  if (use_selected_value_chain) {
    const auto true_chain_root =
        bir::Value::named(bir::TypeKind::I32, "contract.true.selected.root");
    const auto true_chain_selected =
        bir::Value::named(bir::TypeKind::I32, "contract.true.selected.chain");
    const auto false_chain_root =
        bir::Value::named(bir::TypeKind::I32, "contract.false.selected.root");
    const auto false_chain_selected =
        bir::Value::named(bir::TypeKind::I32, "contract.false.selected.chain");
    if (use_immediate_selected_value_chain) {
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Add,
              .result = true_chain_root,
              .operand_type = bir::TypeKind::I32,
              .lhs = bir::Value::immediate_i32(5),
              .rhs = bir::Value::immediate_i32(4),
          });
      ++join_select_index;
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Xor,
              .result = true_chain_selected,
              .operand_type = bir::TypeKind::I32,
              .lhs = true_chain_root,
              .rhs = bir::Value::immediate_i32(0),
          });
      ++join_select_index;
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Sub,
              .result = false_chain_root,
              .operand_type = bir::TypeKind::I32,
              .lhs = bir::Value::immediate_i32(7),
              .rhs = bir::Value::immediate_i32(6),
          });
      ++join_select_index;
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Xor,
              .result = false_chain_selected,
              .operand_type = bir::TypeKind::I32,
              .lhs = false_chain_root,
              .rhs = bir::Value::immediate_i32(0),
          });
    } else {
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Add,
              .result = true_chain_root,
              .operand_type = bir::TypeKind::I32,
              .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
              .rhs = bir::Value::immediate_i32(5),
          });
      ++join_select_index;
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Xor,
              .result = true_chain_selected,
              .operand_type = bir::TypeKind::I32,
              .lhs = true_chain_root,
              .rhs = bir::Value::immediate_i32(0),
          });
      ++join_select_index;
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Sub,
              .result = false_chain_root,
              .operand_type = bir::TypeKind::I32,
              .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
              .rhs = bir::Value::immediate_i32(1),
          });
      ++join_select_index;
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Xor,
              .result = false_chain_selected,
              .operand_type = bir::TypeKind::I32,
              .lhs = false_chain_root,
              .rhs = bir::Value::immediate_i32(0),
          });
    }
    join_transfer.edge_transfers[true_transfer_index].incoming_value = true_chain_selected;
    join_transfer.edge_transfers[false_transfer_index].incoming_value = false_chain_selected;
    for (auto& incoming : join_transfer.incomings) {
      if (incoming.label == block_label(prepared, *join_transfer.source_true_incoming_label)) {
        incoming.value = true_chain_selected;
      } else if (incoming.label ==
                 block_label(prepared, *join_transfer.source_false_incoming_label)) {
        incoming.value = false_chain_selected;
      }
    }
  } else if (use_global_selected_value_roots) {
    const char* true_global_name =
        use_fixed_offset_global_selected_value_roots ? "selected_zero_pair"
        : use_pointer_backed_global_selected_value_roots ? "selected_zero_backing"
                                                         : "selected_zero";
    const char* false_global_name =
        use_fixed_offset_global_selected_value_roots ? "selected_nonzero_pair"
        : use_pointer_backed_global_selected_value_roots ? "selected_nonzero_backing"
                                                         : "selected_nonzero";
    const std::size_t global_byte_offset =
        use_fixed_offset_global_selected_value_roots ? 4 : 0;
    const auto true_global_root =
        bir::Value::named(bir::TypeKind::I32, "contract.true.global.root");
    const auto false_global_root =
        bir::Value::named(bir::TypeKind::I32, "contract.false.global.root");
    join_block->insts.insert(
        join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
        bir::LoadGlobalInst{
            .result = true_global_root,
            .global_name = true_global_name,
            .byte_offset = global_byte_offset,
            .align_bytes = 4,
            .address = use_pointer_backed_global_selected_value_roots
                           ? std::optional<bir::MemoryAddress>(bir::MemoryAddress{
                                 .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                                 .base_name = "selected_zero_root",
                                 .size_bytes = 4,
                                 .align_bytes = 4,
                             })
                           : std::nullopt,
        });
    ++join_select_index;
    join_block->insts.insert(
        join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
        bir::LoadGlobalInst{
            .result = false_global_root,
            .global_name = false_global_name,
            .byte_offset = global_byte_offset,
            .align_bytes = 4,
            .address = use_pointer_backed_global_selected_value_roots
                           ? std::optional<bir::MemoryAddress>(bir::MemoryAddress{
                                 .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                                 .base_name = "selected_nonzero_root",
                                 .size_bytes = 4,
                                 .align_bytes = 4,
                             })
                           : std::nullopt,
        });
    if (use_global_selected_value_chain) {
      const auto true_global_selected =
          bir::Value::named(bir::TypeKind::I32, "contract.true.global.chain");
      const auto false_global_selected =
          bir::Value::named(bir::TypeKind::I32, "contract.false.global.chain");
      ++join_select_index;
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Add,
              .result = true_global_selected,
              .operand_type = bir::TypeKind::I32,
              .lhs = true_global_root,
              .rhs = bir::Value::immediate_i32(4),
          });
      ++join_select_index;
      join_block->insts.insert(
          join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
          bir::BinaryInst{
              .opcode = bir::BinaryOpcode::Sub,
              .result = false_global_selected,
              .operand_type = bir::TypeKind::I32,
              .lhs = false_global_root,
              .rhs = bir::Value::immediate_i32(1),
          });
      join_transfer.edge_transfers[true_transfer_index].incoming_value = true_global_selected;
      join_transfer.edge_transfers[false_transfer_index].incoming_value = false_global_selected;
    } else {
      join_transfer.edge_transfers[true_transfer_index].incoming_value = true_global_root;
      join_transfer.edge_transfers[false_transfer_index].incoming_value = false_global_root;
    }
    for (auto& incoming : join_transfer.incomings) {
      if (incoming.label == block_label(prepared, *join_transfer.source_true_incoming_label)) {
        incoming.value = join_transfer.edge_transfers[true_transfer_index].incoming_value;
      } else if (incoming.label ==
                 block_label(prepared, *join_transfer.source_false_incoming_label)) {
        incoming.value = join_transfer.edge_transfers[false_transfer_index].incoming_value;
      }
    }
  }
  if (join_block->terminator.kind == bir::TerminatorKind::Return &&
      join_block->terminator.value.has_value() &&
      join_block->terminator.value->kind == bir::Value::Kind::Named) {
    auto* function_locations =
        find_mutable_prepared_value_location_function(prepared, function_name);
    auto* return_home =
        function_locations == nullptr
            ? nullptr
            : find_mutable_prepared_value_home(
                  prepared, *function_locations, join_block->terminator.value->name);
    const auto return_value_name =
        prepared.names.value_names.intern(join_block->terminator.value->name);
    if (function_locations != nullptr && return_home == nullptr) {
      prepare::PreparedValueId next_value_id = 0;
      for (const auto& home : function_locations->value_homes) {
        next_value_id = std::max(next_value_id, home.value_id + 1);
      }
      function_locations->value_homes.push_back(prepare::PreparedValueHome{
          .value_id = next_value_id,
          .function_name = function_locations->function_name,
          .value_name = return_value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = minimal_i32_return_register(),
      });
      return_home = &function_locations->value_homes.back();
    }
    std::optional<std::size_t> join_block_index;
    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      if (&function.blocks[block_index] == join_block) {
        join_block_index = block_index;
        break;
      }
    }
    if (function_locations != nullptr && return_home != nullptr &&
        join_block_index.has_value() &&
        prepare::find_prepared_move_bundle(*function_locations,
                                           prepare::PreparedMovePhase::BeforeReturn,
                                           *join_block_index,
                                           join_block->insts.size()) == nullptr &&
        function.return_abi.has_value()) {
      const auto return_register =
          prepare::call_result_destination_register_name(target_profile, *function.return_abi);
      if (return_register.has_value()) {
        function_locations->move_bundles.push_back(prepare::PreparedMoveBundle{
            .function_name = function_locations->function_name,
            .phase = prepare::PreparedMovePhase::BeforeReturn,
            .block_index = *join_block_index,
            .instruction_index = join_block->insts.size(),
            .moves =
                {
                    prepare::PreparedMoveResolution{
                        .from_value_id = return_home->value_id,
                        .to_value_id = return_home->value_id,
                        .destination_kind =
                            prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                        .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                        .destination_register_name = return_register,
                        .block_index = *join_block_index,
                        .instruction_index = join_block->insts.size(),
                        .coalesced_by_assigned_storage =
                            return_home->kind == prepare::PreparedValueHomeKind::Register &&
                            return_home->register_name == return_register,
                        .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                        .authority_kind = prepare::PreparedMoveAuthorityKind::None,
                        .reason = "return_register_to_register",
                    },
                },
        });
      }
    }
  }

  mutable_control_flow->branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = prepared.names.function_names.intern(function_name),
      .block_label = intern_block_label(prepared, "dead.cond"),
      .kind = prepare::PreparedBranchConditionKind::MaterializedBool,
      .condition_value = bir::Value::named(bir::TypeKind::I32, "dead.cond"),
      .true_label = intern_block_label(prepared, "dead.true"),
      .false_label = intern_block_label(prepared, "dead.false"),
  });
  mutable_control_flow->blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = intern_block_label(prepared, "dead.cond"),
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = intern_block_label(prepared, "dead.true"),
      .false_label = intern_block_label(prepared, "dead.false"),
  });
  mutable_control_flow->blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = intern_block_label(prepared, "dead.true"),
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = intern_block_label(prepared, "dead.join"),
  });
  mutable_control_flow->blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = intern_block_label(prepared, "dead.false"),
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = intern_block_label(prepared, "dead.join"),
  });
  mutable_control_flow->blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = intern_block_label(prepared, "dead.join"),
      .terminator_kind = bir::TerminatorKind::Return,
  });
  mutable_control_flow->join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = prepared.names.function_names.intern(function_name),
      .join_block_label = intern_block_label(prepared, "dead.join"),
      .result = bir::Value::named(bir::TypeKind::I32, "dead.result"),
      .kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot,
  });
  function.blocks.push_back(bir::Block{
      .label = "dead.cond",
      .terminator = bir::CondBranchTerminator{
          .condition = bir::Value::named(bir::TypeKind::I32, "dead.cond"),
          .true_label = "dead.true",
          .false_label = "dead.false",
          .true_label_id = prepared.module.names.block_labels.intern("dead.true"),
          .false_label_id = prepared.module.names.block_labels.intern("dead.false"),
      },
      .label_id = prepared.module.names.block_labels.intern("dead.cond"),
  });
  function.blocks.push_back(bir::Block{
      .label = "dead.true",
      .terminator = bir::BranchTerminator{
          .target_label = "dead.join",
          .target_label_id = prepared.module.names.block_labels.intern("dead.join"),
      },
      .label_id = prepared.module.names.block_labels.intern("dead.true"),
  });
  function.blocks.push_back(bir::Block{
      .label = "dead.false",
      .terminator = bir::BranchTerminator{
          .target_label = "dead.join",
          .target_label_id = prepared.module.names.block_labels.intern("dead.join"),
      },
      .label_id = prepared.module.names.block_labels.intern("dead.false"),
  });
  function.blocks.push_back(bir::Block{
      .label = "dead.join",
      .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)},
      .label_id = prepared.module.names.block_labels.intern("dead.join"),
  });
  if (add_true_lane_passthrough_block) {
    const auto true_bridge_label = intern_block_label(prepared, "contract.true.bridge");
    const auto true_bridge_bir_label =
        prepared.module.names.block_labels.intern("contract.true.bridge");
    true_carrier->terminator = bir::BranchTerminator{
        .target_label = "contract.true.bridge",
        .target_label_id = true_bridge_bir_label,
    };
    for (auto& prepared_block : mutable_control_flow->blocks) {
      if (prepared_block.block_label == current_true_label_id) {
        prepared_block.terminator_kind = bir::TerminatorKind::Branch;
        prepared_block.branch_target_label = true_bridge_label;
        prepared_block.true_label = c4c::kInvalidBlockLabel;
        prepared_block.false_label = c4c::kInvalidBlockLabel;
      }
    }
    mutable_control_flow->blocks.push_back(prepare::PreparedControlFlowBlock{
        .block_label = true_bridge_label,
        .terminator_kind = bir::TerminatorKind::Branch,
        .branch_target_label = join_block_label_id,
    });
    function.blocks.push_back(bir::Block{
        .label = "contract.true.bridge",
        .terminator = bir::BranchTerminator{.target_label = join_block->label},
        .label_id = true_bridge_bir_label,
    });
    function.blocks.back().terminator.target_label_id = join_block->label_id;
  }
  if (add_false_lane_passthrough_block) {
    const auto false_bridge_label = intern_block_label(prepared, "contract.false.bridge");
    const auto false_bridge_bir_label =
        prepared.module.names.block_labels.intern("contract.false.bridge");
    false_carrier->terminator = bir::BranchTerminator{
        .target_label = "contract.false.bridge",
        .target_label_id = false_bridge_bir_label,
    };
    for (auto& prepared_block : mutable_control_flow->blocks) {
      if (prepared_block.block_label == current_false_label_id) {
        prepared_block.terminator_kind = bir::TerminatorKind::Branch;
        prepared_block.branch_target_label = false_bridge_label;
        prepared_block.true_label = c4c::kInvalidBlockLabel;
        prepared_block.false_label = c4c::kInvalidBlockLabel;
      }
    }
    mutable_control_flow->blocks.push_back(prepare::PreparedControlFlowBlock{
        .block_label = false_bridge_label,
        .terminator_kind = bir::TerminatorKind::Branch,
        .branch_target_label = join_block_label_id,
    });
    function.blocks.push_back(bir::Block{
        .label = "contract.false.bridge",
        .terminator = bir::BranchTerminator{.target_label = join_block->label},
        .label_id = false_bridge_bir_label,
    });
    function.blocks.back().terminator.target_label_id = join_block->label_id;
  }
  if (add_unreachable_block) {
    function.blocks.push_back(bir::Block{
        .label = "contract.dead.unreachable",
        .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(99)},
        .label_id = prepared.module.names.block_labels.intern("contract.dead.unreachable"),
    });
  }

  std::string mutated_expected_asm = expected_asm;
  const std::string original_branch_label =
      std::string(".L") + function_name + "_" + original_false_label;
  const std::string renamed_branch_label =
      std::string(".L") + function_name + "_" + renamed_false_label;
  if (const auto label_pos = mutated_expected_asm.find(original_branch_label);
      label_pos != std::string::npos) {
    mutated_expected_asm.replace(label_pos, original_branch_label.size(), renamed_branch_label);
    if (const auto second_label_pos =
            mutated_expected_asm.find(original_branch_label, label_pos + renamed_branch_label.size());
        second_label_pos != std::string::npos) {
      mutated_expected_asm.replace(
          second_label_pos,
          original_branch_label.size(),
          renamed_branch_label);
    }
  }

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != mutated_expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared control-flow contract")
                    .c_str());
  }

  return 0;
}

int check_join_route_consumes_prepared_control_flow(const bir::Module& module,
                                                    const std::string& expected_asm,
                                                    const char* function_name,
                                                    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false);
}

int check_join_route_with_unreachable_block_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false);
}

int check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true,
      false);
}

int check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      false);
}

int check_join_route_immediate_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      true,
      true,
      false,
      false,
      false,
      false,
      false);
}

int check_join_route_edge_store_slot_immediate_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      true,
      true,
      false,
      false,
      false,
      false,
      false);
}

int check_join_route_immediate_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_immediate_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      true,
      false);
}

int check_join_route_immediate_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_immediate_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      false,
      false,
      false);
}

int check_join_route_edge_store_slot_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      false,
      false,
      false);
}

int check_join_route_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      false,
      false,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      false,
      false,
      false,
      true,
      false);
}

int check_join_route_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      false,
      false,
      false);
}

int check_join_route_edge_store_slot_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      false,
      false,
      false);
}

int check_join_route_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      false,
      false,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      false,
      false,
      false,
      true,
      false);
}

int check_join_route_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      false,
      false,
      false,
      false,
      true);
}

int check_join_route_fixed_offset_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      true,
      false,
      false);
}

int check_join_route_edge_store_slot_fixed_offset_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      true,
      false,
      false);
}

int check_join_route_fixed_offset_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      true,
      false,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_fixed_offset_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      true,
      false,
      false,
      true,
      false);
}

int check_join_route_fixed_offset_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      true,
      false,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_fixed_offset_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      true,
      false,
      false,
      false,
      true);
}

int check_join_route_fixed_offset_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      true,
      false,
      false);
}

int check_join_route_edge_store_slot_fixed_offset_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      true,
      false,
      false);
}

int check_join_route_fixed_offset_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      true,
      false,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_fixed_offset_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      true,
      false,
      false,
      true,
      false);
}

int check_join_route_fixed_offset_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      true,
      false,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_fixed_offset_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      true,
      false,
      false,
      false,
      true);
}

int check_join_route_pointer_backed_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_pointer_backed_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      false,
      true,
      false);
}

int check_join_route_pointer_backed_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      false,
      true,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_pointer_backed_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      false,
      true,
      false,
      true,
      false);
}

int check_join_route_pointer_backed_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      false,
      true,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_pointer_backed_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      false,
      true,
      false,
      false,
      true);
}

int check_join_route_pointer_backed_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_pointer_backed_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      false,
      true,
      false);
}

int check_join_route_pointer_backed_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      false,
      true,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_pointer_backed_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      false,
      true,
      false,
      true,
      false);
}

int check_join_route_pointer_backed_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      false,
      true,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_pointer_backed_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      false,
      true,
      false,
      false,
      true);
}

int check_join_route_offset_pointer_backed_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      true,
      true,
      false);
}

int check_join_route_edge_store_slot_offset_pointer_backed_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      true,
      true,
      false);
}

int check_join_route_offset_pointer_backed_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      true,
      true,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_offset_pointer_backed_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      true,
      true,
      false,
      true,
      false);
}

int check_join_route_offset_pointer_backed_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      true,
      true,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_offset_pointer_backed_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      true,
      true,
      false,
      false,
      true);
}

int check_join_route_offset_pointer_backed_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      true,
      true,
      false);
}

int check_join_route_edge_store_slot_offset_pointer_backed_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      true,
      true,
      false);
}

int check_join_route_offset_pointer_backed_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      true,
      true,
      false,
      true,
      false);
}

int check_join_route_edge_store_slot_offset_pointer_backed_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      true,
      true,
      false,
      true,
      false);
}

int check_join_route_offset_pointer_backed_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      true,
      true,
      true,
      false,
      false,
      true);
}

int check_join_route_edge_store_slot_offset_pointer_backed_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module,
      expected_asm,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      true,
      true,
      true,
      false,
      false,
      true);
}

int check_materialized_compare_join_branches_publish_prepared_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer has the expected entry block and param")
                    .c_str());
  }

  const auto compare_join_context = prepare::find_prepared_param_zero_materialized_compare_join_context(
      prepared.names, *control_flow, function, *entry_block, function.params.front(), true);
  if (!compare_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the materialized compare-join context")
                    .c_str());
  }

  const auto prepared_join_branches =
      prepare::find_prepared_materialized_compare_join_branches(
          prepared.names, compare_join_context->compare_join_context);
  if (!prepared_join_branches.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the compare-join branch contract")
                    .c_str());
  }

  const auto require_prepared_param_base =
      [&](const prepare::PreparedMaterializedCompareJoinReturnContext& return_context,
          bir::BinaryOpcode expected_opcode,
          std::int64_t expected_immediate) -> int {
    if (return_context.selected_value.base.kind != prepare::PreparedComputedBaseKind::ParamValue ||
        return_context.selected_value.base.param_name_id !=
            prepared.names.value_names.find("p.x")) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped classifying the selected-value base as the function param")
                      .c_str());
    }
    if (return_context.selected_value.operations.size() != 1 ||
        return_context.selected_value.operations.front().opcode != expected_opcode ||
        return_context.selected_value.operations.front().immediate != expected_immediate) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped publishing the selected-value immediate-op chain")
                      .c_str());
    }
    if (!return_context.trailing_binary.has_value() ||
        return_context.trailing_binary->opcode != bir::BinaryOpcode::Xor ||
        return_context.trailing_binary->immediate != 3) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped publishing the trailing immediate-op contract")
                      .c_str());
    }
    return 0;
  };

  if (const auto status = require_prepared_param_base(
          prepared_join_branches->true_return_context, bir::BinaryOpcode::Add, 5);
      status != 0) {
    return status;
  }
  return require_prepared_param_base(
      prepared_join_branches->false_return_context, bir::BinaryOpcode::Sub, 1);
}

int check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier,
    bool replace_entry_compare_with_non_compare = false) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "join");
  if (entry_block == nullptr || join_block == nullptr || function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer has the expected entry/join blocks and param")
                    .c_str());
  }

  const auto* entry_branch_condition =
      prepare::find_prepared_branch_condition(
          *control_flow, find_block_label_id(prepared, entry_block->label));
  if (entry_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes entry branch metadata")
                    .c_str());
  }

  const std::string expected_true_label =
      std::string(block_label(prepared, entry_branch_condition->true_label));
  const std::string expected_false_label =
      std::string(block_label(prepared, entry_branch_condition->false_label));
  entry_block->terminator.true_label = "carrier.compare.true";
  entry_block->terminator.false_label = "carrier.compare.false";
  if (replace_entry_compare_with_non_compare) {
    auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.front());
    if (entry_compare == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes the expected entry compare carrier")
                      .c_str());
    }
    *entry_compare = bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "contract.entry.compare.carrier"),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::immediate_i32(4),
        .rhs = bir::Value::immediate_i32(8),
    };
  }

  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture lost its mutable join-transfer contract")
                    .c_str());
  }

  auto& join_transfer = mutable_control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() < 2 || !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes authoritative true/false join ownership")
                    .c_str());
  }

  std::size_t join_select_index = join_block->insts.size() - 1;
  auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
  if (join_select == nullptr && join_block->insts.size() >= 2) {
    join_select_index = join_block->insts.size() - 2;
    join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
  }
  if (join_select == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes the expected select carrier")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture published invalid true/false join ownership indices")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name =
        intern_slot_name(prepared, "%contract.branch.plan.join.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });

    const std::string original_carrier_result_name = join_select->result.name;
    const auto renamed_carrier_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.join.branch.plan.edge.slot");
    join_block->insts[join_select_index] = bir::LoadLocalInst{
        .result = renamed_carrier_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    for (auto& inst : join_block->insts) {
      if (auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
        if (binary->lhs.kind == bir::Value::Kind::Named &&
            binary->lhs.name == original_carrier_result_name) {
          binary->lhs = renamed_carrier_result;
        }
        if (binary->rhs.kind == bir::Value::Kind::Named &&
            binary->rhs.name == original_carrier_result_name) {
          binary->rhs = renamed_carrier_result;
        }
      }
    }
  }

  const auto prepared_compare_join_branches =
      prepare::find_prepared_param_zero_materialized_compare_join_branches(
          prepared.names, *control_flow, function, *entry_block, function.params.front(), false);
  if (!prepared_compare_join_branches.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the materialized compare-join branch packet")
                    .c_str());
  }
  const auto& compare_join_context =
      prepared_compare_join_branches->prepared_join_branches.compare_join_context;
  if (compare_join_context.true_predecessor == nullptr ||
      compare_join_context.false_predecessor == nullptr ||
      compare_join_context.true_predecessor == compare_join_context.false_predecessor) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer exposes both compare-join predecessor blocks")
                    .c_str());
  }
  const_cast<bir::Block*>(compare_join_context.true_predecessor)->label = "carrier.topology.true";
  const_cast<bir::Block*>(compare_join_context.false_predecessor)->label =
      "carrier.topology.false";

  const auto prepared_branch_plan =
      prepare::find_prepared_materialized_compare_join_branch_plan(
          prepared.names, *prepared_compare_join_branches);
  if (!prepared_branch_plan.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the compare-join branch plan")
                    .c_str());
  }

  if (block_label(prepared, prepared_branch_plan->target_labels.true_label) != expected_true_label ||
      block_label(prepared, prepared_branch_plan->target_labels.false_label) != expected_false_label ||
      std::string(prepared_branch_plan->false_branch_opcode) != "jne" ||
      prepared_branch_plan->compare_shape !=
          prepare::PreparedParamZeroBranchCondition::CompareShape::SelfTest) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing the authoritative compare-join entry branch shape")
                    .c_str());
  }

  return 0;
}

int check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels_impl(
      module, function_name, failure_context, false);
}

int check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels_impl(
      module, function_name, failure_context, true);
}

int check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels_impl(
      module, function_name, failure_context, false, true);
}

int check_materialized_compare_join_route_ignores_non_compare_entry_carrier_impl(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "join");
  if (entry_block == nullptr || join_block == nullptr || entry_block->insts.empty() ||
      function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer has the expected entry/join blocks and param")
                    .c_str());
  }

  const auto* entry_branch_condition =
      prepare::find_prepared_branch_condition(
          *control_flow, find_block_label_id(prepared, entry_block->label));
  if (entry_branch_condition == nullptr || !entry_branch_condition->predicate.has_value() ||
      !entry_branch_condition->lhs.has_value() || !entry_branch_condition->rhs.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the authoritative compare-join entry branch metadata")
                    .c_str());
  }

  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.front());
  if (entry_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes the expected entry compare carrier")
                    .c_str());
  }
  *entry_compare = bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "contract.entry.compare.carrier"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(4),
      .rhs = bir::Value::immediate_i32(8),
  };

  if (use_edge_store_slot_carrier) {
    auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
    if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.size() != 1) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture lost its mutable join-transfer contract")
                      .c_str());
    }

    auto& join_transfer = mutable_control_flow->join_transfers.front();
    if (join_transfer.edge_transfers.size() < 2 || !join_transfer.source_true_transfer_index.has_value() ||
        !join_transfer.source_false_transfer_index.has_value()) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes authoritative true/false join ownership")
                      .c_str());
    }

    std::size_t join_select_index = join_block->insts.size() - 1;
    auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
    if (join_select == nullptr && join_block->insts.size() >= 2) {
      join_select_index = join_block->insts.size() - 2;
      join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
    }
    if (join_select == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes the expected select carrier")
                      .c_str());
    }
    const std::string original_carrier_result_name = join_select->result.name;

    const auto true_transfer_index = *join_transfer.source_true_transfer_index;
    const auto false_transfer_index = *join_transfer.source_false_transfer_index;
    if (true_transfer_index >= join_transfer.edge_transfers.size() ||
        false_transfer_index >= join_transfer.edge_transfers.size() ||
        true_transfer_index == false_transfer_index) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture published invalid true/false join ownership indices")
                      .c_str());
    }

    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = prepared.names.slot_names.intern("%contract.route.join.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });

    join_block->insts[join_select_index] = bir::LoadLocalInst{
        .result = bir::Value::named(bir::TypeKind::I32, "carrier.join.route.edge.slot"),
        .slot_name = std::string(prepare::prepared_slot_name(prepared.names,
                                                             *join_transfer.storage_name)),
    };
    const auto rewritten_carrier_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.join.route.edge.slot");
    for (auto& inst : join_block->insts) {
      if (auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
        if (binary->lhs.kind == bir::Value::Kind::Named &&
            binary->lhs.name == original_carrier_result_name) {
          binary->lhs = rewritten_carrier_result;
        }
        if (binary->rhs.kind == bir::Value::Kind::Named &&
            binary->rhs.name == original_carrier_result_name) {
          binary->rhs = rewritten_carrier_result;
        }
      }
    }
  }

  const auto resolved_render_contract =
      prepare::find_prepared_param_zero_resolved_materialized_compare_join_render_contract(
          prepared.names,
          prepared.module,
          *control_flow,
          function,
          *entry_block,
          function.params.front(),
          false);
  if (!resolved_render_contract.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped resolving the prepared compare-join render contract when the entry compare carrier drifts")
                    .c_str());
  }

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped preferring the prepared compare-join entry branch contract")
                    .c_str());
  }
  return 0;
}

int check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_route_ignores_non_compare_entry_carrier_impl(
      module, expected_asm, function_name, failure_context, false);
}

int check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_route_ignores_non_compare_entry_carrier_impl(
      module, expected_asm, function_name, failure_context, true);
}

int check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "join");
  if (entry_block == nullptr || join_block == nullptr || function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer has the expected entry/join blocks and param")
                    .c_str());
  }

  auto& branch_condition = control_flow->branch_conditions.front();
  auto& join_transfer = control_flow->join_transfers.front();
  if (!join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value() ||
      join_transfer.edge_transfers.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes authoritative true/false join ownership")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    std::size_t join_select_index = join_block->insts.size() - 1;
    auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
    if (join_select == nullptr && join_block->insts.size() >= 2) {
      join_select_index = join_block->insts.size() - 2;
      join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
    }
    if (join_select == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes the expected select carrier")
                      .c_str());
    }

    const std::size_t true_transfer_index = *join_transfer.source_true_transfer_index;
    const std::size_t false_transfer_index = *join_transfer.source_false_transfer_index;
    if (true_transfer_index >= join_transfer.edge_transfers.size() ||
        false_transfer_index >= join_transfer.edge_transfers.size() ||
        true_transfer_index == false_transfer_index) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture published invalid true/false join ownership indices")
                      .c_str());
    }

    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.route.join.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });

    const std::string original_carrier_result_name = join_select->result.name;
    const auto rewritten_carrier_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.join.route.edge.slot");
    join_block->insts[join_select_index] = bir::LoadLocalInst{
        .result = rewritten_carrier_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    for (auto& inst : join_block->insts) {
      if (auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
        if (binary->lhs.kind == bir::Value::Kind::Named &&
            binary->lhs.name == original_carrier_result_name) {
          binary->lhs = rewritten_carrier_result;
        }
        if (binary->rhs.kind == bir::Value::Kind::Named &&
            binary->rhs.name == original_carrier_result_name) {
          binary->rhs = rewritten_carrier_result;
        }
      }
    }
  }

  branch_condition.predicate.reset();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a drifted compare-join prepared branch contract")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the drifted compare-join prepared branch contract with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition_impl(
      module, function_name, failure_context, false);
}

int check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition_impl(
      module, function_name, failure_context, true);
}

int check_materialized_compare_join_route_requires_authoritative_prepared_branch_record_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* join_block = find_block(function, "join");
  if (join_block == nullptr || function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer has the expected join block and param")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  if (!join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value() ||
      join_transfer.edge_transfers.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes authoritative true/false join ownership")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    std::size_t join_select_index = join_block->insts.size() - 1;
    auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
    if (join_select == nullptr && join_block->insts.size() >= 2) {
      join_select_index = join_block->insts.size() - 2;
      join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
    }
    if (join_select == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes the expected select carrier")
                      .c_str());
    }

    const std::size_t true_transfer_index = *join_transfer.source_true_transfer_index;
    const std::size_t false_transfer_index = *join_transfer.source_false_transfer_index;
    if (true_transfer_index >= join_transfer.edge_transfers.size() ||
        false_transfer_index >= join_transfer.edge_transfers.size() ||
        true_transfer_index == false_transfer_index) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture published invalid true/false join ownership indices")
                      .c_str());
    }

    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.route.join.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });

    const std::string original_carrier_result_name = join_select->result.name;
    const auto rewritten_carrier_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.join.route.edge.slot");
    join_block->insts[join_select_index] = bir::LoadLocalInst{
        .result = rewritten_carrier_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    for (auto& inst : join_block->insts) {
      if (auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
        if (binary->lhs.kind == bir::Value::Kind::Named &&
            binary->lhs.name == original_carrier_result_name) {
          binary->lhs = rewritten_carrier_result;
        }
        if (binary->rhs.kind == bir::Value::Kind::Named &&
            binary->rhs.name == original_carrier_result_name) {
          binary->rhs = rewritten_carrier_result;
        }
      }
    }
  }

  control_flow->branch_conditions.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a missing compare-join prepared branch record")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing compare-join prepared branch record with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_route_requires_authoritative_prepared_branch_record_impl(
      module, function_name, failure_context, false);
}

int check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_route_requires_authoritative_prepared_branch_record_impl(
      module, function_name, failure_context, true);
}

int check_materialized_compare_join_route_requires_authoritative_prepared_return_bundle(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto function_name_id = prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_name_id.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer resolves the function name for value-location ownership")
                    .c_str());
  }
  auto function_locations_it =
      std::find_if(prepared.value_locations.functions.begin(),
                   prepared.value_locations.functions.end(),
                   [&](const prepare::PreparedValueLocationFunction& function_locations) {
                     return function_locations.function_name == *function_name_id;
                   });
  if (function_locations_it == prepared.value_locations.functions.end()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join value-location contract")
                    .c_str());
  }
  const auto erased_bundles =
      static_cast<std::size_t>(std::count_if(
          function_locations_it->move_bundles.begin(),
          function_locations_it->move_bundles.end(),
          [](const prepare::PreparedMoveBundle& bundle) {
            return bundle.phase == prepare::PreparedMovePhase::BeforeReturn;
          }));
  if (erased_bundles == 0) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the authoritative compare-join return bundle")
                    .c_str());
  }
  function_locations_it->move_bundles.erase(
      std::remove_if(
          function_locations_it->move_bundles.begin(),
          function_locations_it->move_bundles.end(),
          [](const prepare::PreparedMoveBundle& bundle) {
            return bundle.phase == prepare::PreparedMovePhase::BeforeReturn;
          }),
      function_locations_it->move_bundles.end());

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a compare-join return after the authoritative prepared return bundle was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing compare-join return bundle with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_materialized_compare_join_route_rejects_drifted_immediate_move_source(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto function_name_id = prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_name_id.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer resolves the function name for value-location ownership")
                    .c_str());
  }
  const auto* control_flow =
      prepare::find_prepared_control_flow_function(prepared.control_flow, *function_name_id);
  auto function_locations_it =
      std::find_if(prepared.value_locations.functions.begin(),
                   prepared.value_locations.functions.end(),
                   [&](const prepare::PreparedValueLocationFunction& function_locations) {
                     return function_locations.function_name == *function_name_id;
                   });
  const auto bir_function_it =
      std::find_if(prepared.module.functions.begin(),
                   prepared.module.functions.end(),
                   [&](const bir::Function& function) {
                     return function.name == function_name;
                   });
  const auto* bir_function =
      bir_function_it == prepared.module.functions.end() ? nullptr : &*bir_function_it;
  if (control_flow == nullptr || function_locations_it == prepared.value_locations.functions.end() ||
      bir_function == nullptr || control_flow->parallel_copy_bundles.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join immediate move authority contract")
                    .c_str());
  }

  prepare::PreparedMoveResolution* immediate_move = nullptr;
  for (const auto& parallel_copy : control_flow->parallel_copy_bundles) {
    const auto* step_move =
        prepare::find_prepared_parallel_copy_move_for_step(parallel_copy, 0);
    if (step_move == nullptr ||
        step_move->source_value.kind != bir::Value::Kind::Immediate ||
        step_move->source_value.type != bir::TypeKind::I32) {
      continue;
    }
    immediate_move = prepare::find_prepared_out_of_ssa_parallel_copy_move_for_step(
        prepared.names, *bir_function, *function_locations_it, parallel_copy, 0);
    if (immediate_move != nullptr) {
      break;
    }
  }
  if (immediate_move == nullptr || !immediate_move->source_immediate_i32.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes immediate-source out-of-SSA move authority for compare-join edges")
                    .c_str());
  }
  immediate_move->source_immediate_i32.reset();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a drifted immediate-source compare-join move contract")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the drifted immediate-source compare-join move contract with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_materialized_compare_join_route_consumes_authoritative_prepared_param_home(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* function_locations = find_mutable_prepared_value_location_function(prepared, function_name);
  if (function_locations == nullptr) {
    return fail((std::string(failure_context) +
                 ": missing prepared value-location function")
                    .c_str());
  }
  auto* param_home = find_mutable_prepared_value_home(prepared, *function_locations, "p.x");
  if (param_home == nullptr) {
    return fail((std::string(failure_context) +
                 ": missing prepared parameter home")
                    .c_str());
  }

  param_home->kind = prepare::PreparedValueHomeKind::Register;
  param_home->register_name = "r10";
  param_home->slot_id.reset();
  param_home->offset_bytes.reset();
  param_home->immediate_i32.reset();

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared parameter home in compare-join returns")
                    .c_str());
  }

  return 0;
}

int check_minimal_compare_branch_route_rejects_authoritative_join_contract_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the joined-branch control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* true_block = find_block(function, "is_zero");
  auto* false_block = find_block(function, "is_nonzero");
  auto* join_block = find_block(function, "join");
  if (entry_block == nullptr || true_block == nullptr || false_block == nullptr ||
      join_block == nullptr || entry_block->insts.size() != 1 || function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared joined-branch fixture no longer has the expected entry, leaves, join, and param")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  if (!join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value() ||
      join_transfer.edge_transfers.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared joined-branch fixture no longer exposes authoritative true/false join ownership")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    std::size_t join_select_index = join_block->insts.size() - 1;
    auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
    if (join_select == nullptr && join_block->insts.size() >= 2) {
      join_select_index = join_block->insts.size() - 2;
      join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
    }
    if (join_select == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared joined-branch fixture no longer exposes the expected select carrier")
                      .c_str());
    }

    const std::size_t true_transfer_index = *join_transfer.source_true_transfer_index;
    const std::size_t false_transfer_index = *join_transfer.source_false_transfer_index;
    if (true_transfer_index >= join_transfer.edge_transfers.size() ||
        false_transfer_index >= join_transfer.edge_transfers.size() ||
        true_transfer_index == false_transfer_index) {
      return fail((std::string(failure_context) +
                   ": prepared joined-branch fixture published invalid true/false join ownership indices")
                      .c_str());
    }

    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.route.join.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
  }

  true_block->insts.clear();
  true_block->terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(7)};
  false_block->insts.clear();
  false_block->terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(11)};

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a plain param-zero branch fallback while authoritative join ownership remained active")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the drifted joined-branch contract with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_minimal_compare_branch_route_rejects_authoritative_join_contract(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_minimal_compare_branch_route_rejects_authoritative_join_contract_impl(
      module, function_name, failure_context, false);
}

int check_minimal_compare_branch_edge_store_slot_route_rejects_authoritative_join_contract(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_minimal_compare_branch_route_rejects_authoritative_join_contract_impl(
      module, function_name, failure_context, true);
}

int check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer has the expected entry block and param")
                    .c_str());
  }

  const auto* entry_branch_condition =
      prepare::find_prepared_branch_condition(
          *control_flow, find_block_label_id(prepared, entry_block->label));
  if (entry_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes entry branch metadata")
                    .c_str());
  }

  const std::string expected_true_label =
      std::string(block_label(prepared, entry_branch_condition->true_label));
  const std::string expected_false_label =
      std::string(block_label(prepared, entry_branch_condition->false_label));
  entry_block->terminator.true_label = "carrier.compare.true";
  entry_block->terminator.false_label = "carrier.compare.false";

  const auto prepared_compare_join_branches =
      prepare::find_prepared_param_zero_materialized_compare_join_branches(
          prepared.names, *control_flow, function, *entry_block, function.params.front(), false);
  if (!prepared_compare_join_branches.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the materialized compare-join branch packet")
                    .c_str());
  }

  const auto render_contract =
      prepare::find_prepared_materialized_compare_join_render_contract(
          prepared.names, *prepared_compare_join_branches);
  if (!render_contract.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the compare-join render contract")
                    .c_str());
  }

  const auto expected_branch_plan =
      prepare::find_prepared_materialized_compare_join_branch_plan(
          prepared.names, *prepared_compare_join_branches);
  if (!expected_branch_plan.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the compare-join branch plan")
                    .c_str());
  }
  if (block_label(prepared, expected_branch_plan->target_labels.true_label) != expected_true_label ||
      block_label(prepared, expected_branch_plan->target_labels.false_label) != expected_false_label ||
      std::string(expected_branch_plan->false_branch_opcode) != "jne" ||
      expected_branch_plan->compare_shape !=
          prepare::PreparedParamZeroBranchCondition::CompareShape::SelfTest) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the authoritative compare-join entry branch shape")
                    .c_str());
  }
  if (render_contract->branch_plan.target_labels.true_label !=
          expected_branch_plan->target_labels.true_label ||
      render_contract->branch_plan.target_labels.false_label !=
          expected_branch_plan->target_labels.false_label ||
      std::string(render_contract->branch_plan.false_branch_opcode) !=
          expected_branch_plan->false_branch_opcode ||
      render_contract->branch_plan.compare_shape != expected_branch_plan->compare_shape) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped packaging the compare-join entry branch shape")
                    .c_str());
  }

  const auto expected_same_module_global_names =
      prepare::collect_prepared_materialized_compare_join_same_module_globals(
          prepared.names,
          prepared_compare_join_branches->prepared_join_branches);
  if (render_contract->same_module_global_names != expected_same_module_global_names) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped packaging compare-join same-module globals")
                    .c_str());
  }
  const auto resolved_same_module_globals =
      prepare::resolve_prepared_materialized_compare_join_same_module_globals(
          prepared.names, prepared.module, *render_contract);
  if (!resolved_same_module_globals.has_value() ||
      resolved_same_module_globals->size() != expected_same_module_global_names.size()) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped resolving compare-join same-module globals")
                    .c_str());
  }
  for (std::size_t index = 0; index < resolved_same_module_globals->size(); ++index) {
    if ((*resolved_same_module_globals)[index] == nullptr ||
        (*resolved_same_module_globals)[index]->name != expected_same_module_global_names[index]) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped resolving compare-join same-module globals in authoritative order")
                      .c_str());
    }
  }
  const auto resolved_render_contract =
      prepare::resolve_prepared_materialized_compare_join_render_contract(
          prepared.names, prepared.module, *render_contract);
  if (!resolved_render_contract.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped resolving the compare-join render contract")
                    .c_str());
  }
  const auto directly_resolved_render_contract =
      prepare::find_prepared_resolved_materialized_compare_join_render_contract(
          prepared.names, prepared.module, *prepared_compare_join_branches);
  if (!directly_resolved_render_contract.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing the direct resolved compare-join contract")
                    .c_str());
  }
  const auto direct_param_zero_resolved_render_contract =
      prepare::find_prepared_param_zero_resolved_materialized_compare_join_render_contract(
          prepared.names,
          prepared.module,
          *control_flow,
          function,
          *entry_block,
          function.params.front(),
          false);
  if (!direct_param_zero_resolved_render_contract.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing the direct param-zero resolved compare-join contract")
                    .c_str());
  }
  if (resolved_render_contract->branch_plan.target_labels.true_label !=
          render_contract->branch_plan.target_labels.true_label ||
      resolved_render_contract->branch_plan.target_labels.false_label !=
          render_contract->branch_plan.target_labels.false_label ||
      std::string(resolved_render_contract->branch_plan.false_branch_opcode) !=
          render_contract->branch_plan.false_branch_opcode ||
      resolved_render_contract->branch_plan.compare_shape !=
          render_contract->branch_plan.compare_shape ||
      resolved_render_contract->same_module_globals.size() !=
          resolved_same_module_globals->size()) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped preserving the compare-join render contract contents")
                    .c_str());
  }
  for (std::size_t index = 0; index < resolved_render_contract->same_module_globals.size();
       ++index) {
    if (resolved_render_contract->same_module_globals[index] !=
        (*resolved_same_module_globals)[index]) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped preserving resolved compare-join globals")
                      .c_str());
    }
  }
  if (directly_resolved_render_contract->branch_plan.target_labels.true_label !=
          resolved_render_contract->branch_plan.target_labels.true_label ||
      directly_resolved_render_contract->branch_plan.target_labels.false_label !=
          resolved_render_contract->branch_plan.target_labels.false_label ||
      std::string(directly_resolved_render_contract->branch_plan.false_branch_opcode) !=
          resolved_render_contract->branch_plan.false_branch_opcode ||
      directly_resolved_render_contract->branch_plan.compare_shape !=
          resolved_render_contract->branch_plan.compare_shape ||
      directly_resolved_render_contract->same_module_globals.size() !=
          resolved_render_contract->same_module_globals.size()) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped preserving the direct resolved compare-join contract")
                    .c_str());
  }
  for (std::size_t index = 0;
       index < directly_resolved_render_contract->same_module_globals.size();
       ++index) {
    if (directly_resolved_render_contract->same_module_globals[index] !=
        resolved_render_contract->same_module_globals[index]) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped preserving direct resolved compare-join globals")
                      .c_str());
    }
  }
  if (direct_param_zero_resolved_render_contract->branch_plan.target_labels.true_label !=
          directly_resolved_render_contract->branch_plan.target_labels.true_label ||
      direct_param_zero_resolved_render_contract->branch_plan.target_labels.false_label !=
          directly_resolved_render_contract->branch_plan.target_labels.false_label ||
      std::string(direct_param_zero_resolved_render_contract->branch_plan.false_branch_opcode) !=
          directly_resolved_render_contract->branch_plan.false_branch_opcode ||
      direct_param_zero_resolved_render_contract->branch_plan.compare_shape !=
          directly_resolved_render_contract->branch_plan.compare_shape ||
      direct_param_zero_resolved_render_contract->same_module_globals.size() !=
          directly_resolved_render_contract->same_module_globals.size()) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped preserving the direct param-zero resolved compare-join contract")
                    .c_str());
  }
  for (std::size_t index = 0;
       index < direct_param_zero_resolved_render_contract->same_module_globals.size();
       ++index) {
    if (direct_param_zero_resolved_render_contract->same_module_globals[index] !=
        directly_resolved_render_contract->same_module_globals[index]) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped preserving direct param-zero resolved compare-join globals")
                      .c_str());
    }
  }
  if (render_contract->true_return.shape !=
          prepare::classify_prepared_materialized_compare_join_return_shape(
              prepared_compare_join_branches->prepared_join_branches.true_return_context) ||
      render_contract->false_return.shape !=
          prepare::classify_prepared_materialized_compare_join_return_shape(
              prepared_compare_join_branches->prepared_join_branches.false_return_context)) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped packaging compare-join return-lane shapes")
                    .c_str());
  }
  const auto require_binary_plan =
      [&](const prepare::PreparedMaterializedCompareJoinReturnArm& return_arm,
          const prepare::PreparedMaterializedCompareJoinReturnContext& expected_return_context) -> int {
        const auto binary_plan =
            prepare::find_prepared_materialized_compare_join_return_binary_plan(return_arm);
        if (!binary_plan.has_value()) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped packaging the compare-join return-arm binary plan")
                          .c_str());
        }
        const bool trailing_binary_matches =
            binary_plan->trailing_binary.has_value() ==
                expected_return_context.trailing_binary.has_value() &&
            (!binary_plan->trailing_binary.has_value() ||
             (binary_plan->trailing_binary->opcode ==
                  expected_return_context.trailing_binary->opcode &&
              binary_plan->trailing_binary->immediate ==
                  expected_return_context.trailing_binary->immediate));
        if (!trailing_binary_matches) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped publishing authoritative compare-join trailing immediate ownership")
                          .c_str());
        }
        return 0;
      };
  if (const auto status =
          require_binary_plan(render_contract->true_return,
                              prepared_compare_join_branches->prepared_join_branches
                                  .true_return_context);
      status != 0) {
    return status;
  }
  if (const auto status =
          require_binary_plan(render_contract->false_return,
                              prepared_compare_join_branches->prepared_join_branches
                                  .false_return_context);
      status != 0) {
    return status;
  }
  if (const auto status =
          require_binary_plan(direct_param_zero_resolved_render_contract->true_return.arm,
                              prepared_compare_join_branches->prepared_join_branches
                                  .true_return_context);
      status != 0) {
    return status;
  }
  if (const auto status =
          require_binary_plan(direct_param_zero_resolved_render_contract->false_return.arm,
                              prepared_compare_join_branches->prepared_join_branches
                                  .false_return_context);
      status != 0) {
    return status;
  }

  const auto require_return_context =
      [&](const prepare::PreparedMaterializedCompareJoinReturnContext& return_context,
          const prepare::PreparedMaterializedCompareJoinReturnContext& expected_return_context) -> int {
        const auto operations_match =
            [&]() -> bool {
              if (return_context.selected_value.operations.size() !=
                  expected_return_context.selected_value.operations.size()) {
                return false;
              }
              for (std::size_t index = 0; index < return_context.selected_value.operations.size();
                   ++index) {
                const auto& actual = return_context.selected_value.operations[index];
                const auto& expected = expected_return_context.selected_value.operations[index];
                if (actual.opcode != expected.opcode || actual.immediate != expected.immediate) {
                  return false;
                }
              }
              return true;
            }();
        if (return_context.selected_value.base.kind != expected_return_context.selected_value.base.kind ||
            return_context.selected_value.base.global_name_id !=
                expected_return_context.selected_value.base.global_name_id ||
            return_context.selected_value.base.global_byte_offset !=
                expected_return_context.selected_value.base.global_byte_offset ||
            return_context.selected_value.base.pointer_root_global_name_id !=
                expected_return_context.selected_value.base.pointer_root_global_name_id ||
            !operations_match) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped packaging compare-join selected-value return contexts")
                          .c_str());
        }
        const bool trailing_binary_matches =
            return_context.trailing_binary.has_value() ==
                expected_return_context.trailing_binary.has_value() &&
            (!return_context.trailing_binary.has_value() ||
             (return_context.trailing_binary->opcode ==
                  expected_return_context.trailing_binary->opcode &&
              return_context.trailing_binary->immediate ==
                  expected_return_context.trailing_binary->immediate));
        if (!trailing_binary_matches) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped packaging compare-join trailing immediate ops")
                          .c_str());
        }
        return 0;
      };

  if (const auto status =
          require_return_context(render_contract->true_return.context,
                                 prepared_compare_join_branches->prepared_join_branches
                                     .true_return_context);
      status != 0) {
    return status;
  }
  if (const auto status =
          require_return_context(
              direct_param_zero_resolved_render_contract->true_return.arm.context,
              prepared_compare_join_branches->prepared_join_branches.true_return_context);
      status != 0) {
    return status;
  }
  if (direct_param_zero_resolved_render_contract->true_return.global !=
          directly_resolved_render_contract->true_return.global ||
      direct_param_zero_resolved_render_contract->true_return.pointer_root_global !=
          directly_resolved_render_contract->true_return.pointer_root_global ||
      direct_param_zero_resolved_render_contract->false_return.global !=
          directly_resolved_render_contract->false_return.global ||
      direct_param_zero_resolved_render_contract->false_return.pointer_root_global !=
          directly_resolved_render_contract->false_return.pointer_root_global) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped preserving direct param-zero resolved compare-join arm ownership")
                    .c_str());
  }
  if (const auto status =
          require_return_context(
              direct_param_zero_resolved_render_contract->false_return.arm.context,
              prepared_compare_join_branches->prepared_join_branches.false_return_context);
      status != 0) {
    return status;
  }
  return require_return_context(
      render_contract->false_return.context,
      prepared_compare_join_branches->prepared_join_branches.false_return_context);
}

int check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier,
    bool use_selected_value_chain,
    bool add_true_lane_passthrough_block = false,
    bool add_false_lane_passthrough_block = false,
    bool rewrite_entry_compare_to_non_compare = false,
    const std::string* expected_asm = nullptr) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "join");
  if (entry_block == nullptr || join_block == nullptr || function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer has the expected entry/join blocks and param")
                    .c_str());
  }
  if (rewrite_entry_compare_to_non_compare) {
    auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.front());
    if (entry_compare == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes the expected entry compare carrier")
                      .c_str());
    }
    *entry_compare = bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "contract.entry.compare.carrier"),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::immediate_i32(4),
        .rhs = bir::Value::immediate_i32(8),
    };
  }

  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture lost its mutable join-transfer contract")
                    .c_str());
  }

  auto& join_transfer = mutable_control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() < 2 || !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes authoritative true/false join ownership")
                    .c_str());
  }

  std::size_t join_select_index = join_block->insts.size() - 1;
  auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
  if (join_select == nullptr && join_block->insts.size() >= 2) {
    join_select_index = join_block->insts.size() - 2;
    join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
  }
  if (join_select == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes the expected select carrier")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture published invalid true/false join ownership indices")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.immediate.join.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });

    const std::string original_carrier_result_name = join_select->result.name;
    const auto renamed_carrier_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.join.immediate.edge.slot");
    join_block->insts[join_select_index] = bir::LoadLocalInst{
        .result = renamed_carrier_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    for (auto& inst : join_block->insts) {
      if (auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
        if (binary->lhs.kind == bir::Value::Kind::Named &&
            binary->lhs.name == original_carrier_result_name) {
          binary->lhs = renamed_carrier_result;
        }
        if (binary->rhs.kind == bir::Value::Kind::Named &&
            binary->rhs.name == original_carrier_result_name) {
          binary->rhs = renamed_carrier_result;
        }
      }
    }
  }

  if (use_selected_value_chain) {
    const auto true_chain_root =
        bir::Value::named(bir::TypeKind::I32, "contract.true.immediate.root");
    const auto true_chain_selected =
        bir::Value::named(bir::TypeKind::I32, "contract.true.immediate.chain");
    const auto false_chain_root =
        bir::Value::named(bir::TypeKind::I32, "contract.false.immediate.root");
    const auto false_chain_selected =
        bir::Value::named(bir::TypeKind::I32, "contract.false.immediate.chain");
    join_block->insts.insert(
        join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Add,
            .result = true_chain_root,
            .operand_type = bir::TypeKind::I32,
            .lhs = bir::Value::immediate_i32(5),
            .rhs = bir::Value::immediate_i32(4),
        });
    ++join_select_index;
    join_block->insts.insert(
        join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Xor,
            .result = true_chain_selected,
            .operand_type = bir::TypeKind::I32,
            .lhs = true_chain_root,
            .rhs = bir::Value::immediate_i32(0),
        });
    ++join_select_index;
    join_block->insts.insert(
        join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Sub,
            .result = false_chain_root,
            .operand_type = bir::TypeKind::I32,
            .lhs = bir::Value::immediate_i32(7),
            .rhs = bir::Value::immediate_i32(6),
        });
    ++join_select_index;
    join_block->insts.insert(
        join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Xor,
            .result = false_chain_selected,
            .operand_type = bir::TypeKind::I32,
            .lhs = false_chain_root,
            .rhs = bir::Value::immediate_i32(0),
        });
    join_transfer.edge_transfers[true_transfer_index].incoming_value = true_chain_selected;
    join_transfer.edge_transfers[false_transfer_index].incoming_value = false_chain_selected;
    for (auto& incoming : join_transfer.incomings) {
      if (incoming.label ==
          block_label(prepared, join_transfer.edge_transfers[true_transfer_index].predecessor_label)) {
        incoming.value = true_chain_selected;
      } else if (incoming.label ==
                 block_label(prepared,
                             join_transfer.edge_transfers[false_transfer_index].predecessor_label)) {
        incoming.value = false_chain_selected;
      }
    }
  }

  if (add_true_lane_passthrough_block) {
    const std::string true_predecessor_label = std::string(
        block_label(prepared, join_transfer.edge_transfers[true_transfer_index].predecessor_label));
    auto* true_predecessor =
        find_block(function, true_predecessor_label.c_str());
    if (true_predecessor == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes the authoritative true-lane predecessor")
                      .c_str());
    }
    true_predecessor->terminator = bir::BranchTerminator{.target_label = "contract.true.bridge"};
    function.blocks.push_back(bir::Block{
        .label = "contract.true.bridge",
        .terminator = bir::BranchTerminator{.target_label = join_block->label},
    });
  }
  if (add_false_lane_passthrough_block) {
    const std::string false_predecessor_label = std::string(
        block_label(prepared, join_transfer.edge_transfers[false_transfer_index].predecessor_label));
    auto* false_predecessor =
        find_block(function, false_predecessor_label.c_str());
    if (false_predecessor == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes the authoritative false-lane predecessor")
                      .c_str());
    }
    false_predecessor->terminator = bir::BranchTerminator{.target_label = "contract.false.bridge"};
    function.blocks.push_back(bir::Block{
        .label = "contract.false.bridge",
        .terminator = bir::BranchTerminator{.target_label = join_block->label},
    });
  }

  entry_block = find_block(function, "entry");
  join_block = find_block(function, "join");
  if (entry_block == nullptr || join_block == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer preserves entry/join blocks after passthrough rewrites")
                    .c_str());
  }

  const auto compare_join_context = prepare::find_prepared_param_zero_materialized_compare_join_context(
      prepared.names, *control_flow, function, *entry_block, function.params.front(), true);
  if (!compare_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the materialized compare-join context")
                    .c_str());
  }

  const auto prepared_join_branches =
      prepare::find_prepared_materialized_compare_join_branches(
          prepared.names, compare_join_context->compare_join_context);
  if (!prepared_join_branches.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the compare-join branch contract")
                    .c_str());
  }

  const auto require_prepared_immediate_base =
      [&](const prepare::PreparedMaterializedCompareJoinReturnContext& return_context,
          std::int64_t expected_immediate,
          bir::BinaryOpcode expected_opcode,
          std::int64_t expected_operation_immediate) -> int {
    if (return_context.selected_value.base.kind !=
            prepare::PreparedComputedBaseKind::ImmediateI32 ||
        return_context.selected_value.base.immediate != expected_immediate) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped classifying the selected-value base as an immediate")
                      .c_str());
    }
    if (use_selected_value_chain) {
      if (return_context.selected_value.operations.size() != 2 ||
          return_context.selected_value.operations.front().opcode != expected_opcode ||
          return_context.selected_value.operations.front().immediate !=
              expected_operation_immediate ||
          return_context.selected_value.operations.back().opcode != bir::BinaryOpcode::Xor ||
          return_context.selected_value.operations.back().immediate != 0) {
        return fail((std::string(failure_context) +
                     ": shared helper stopped publishing the immediate-base selected-value chain")
                        .c_str());
      }
    } else if (!return_context.selected_value.operations.empty()) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped keeping direct immediate selected values operation-free")
                      .c_str());
    }
    if (!return_context.trailing_binary.has_value() ||
        return_context.trailing_binary->opcode != bir::BinaryOpcode::Xor ||
        return_context.trailing_binary->immediate != 3) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped publishing the trailing immediate-op contract")
                      .c_str());
    }
    return 0;
  };

  if (const auto status = require_prepared_immediate_base(
          prepared_join_branches->true_return_context,
          5,
          bir::BinaryOpcode::Add,
          4);
      status != 0) {
    return status;
  }
  if (const auto status = require_prepared_immediate_base(
      prepared_join_branches->false_return_context,
      use_selected_value_chain ? 7 : 1,
      bir::BinaryOpcode::Sub,
      6);
      status != 0) {
    return status;
  }
  if (expected_asm != nullptr) {
    refresh_prepared_i32_return_bundle(
        prepared, target_profile, function, *join_block, function_name);
    const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
    if (prepared_asm != *expected_asm) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer stopped preferring the prepared compare-join entry branch contract")
                      .c_str());
    }
  }
  return 0;
}

int check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, false, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, false, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, true, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, false, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, true, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_immediate_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, true, true);
}

int check_materialized_compare_join_branches_publish_prepared_immediate_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, false, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, true, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_immediate_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, false, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, true, true, false, true);
}

int check_materialized_compare_join_immediate_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, false, false, false, false, true, &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_immediate_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, true, false, false, false, true, &expected_asm);
}

int check_materialized_compare_join_immediate_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, false, true, false, false, true, &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_immediate_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
      module, function_name, failure_context, true, true, false, false, true, &expected_asm);
}

int check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier,
    bool use_selected_value_chain,
    bool use_fixed_offset_global_roots,
    bool use_pointer_backed_global_roots,
    bool add_true_lane_passthrough_block = false,
    bool add_false_lane_passthrough_block = false,
    bool rewrite_entry_compare_to_non_compare = false,
    bool reject_drifted_branch_condition = false,
    bool reject_missing_branch_record = false,
    const std::string* expected_asm = nullptr) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "join");
  if (entry_block == nullptr || join_block == nullptr || function.params.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer has the expected entry/join blocks and param")
                    .c_str());
  }
  if (rewrite_entry_compare_to_non_compare) {
    auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.front());
    if (entry_compare == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer exposes the expected entry compare carrier")
                      .c_str());
    }
    *entry_compare = bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "contract.entry.compare.carrier"),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::immediate_i32(4),
        .rhs = bir::Value::immediate_i32(8),
    };
  }

  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture lost its mutable join-transfer contract")
                    .c_str());
  }

  auto& join_transfer = mutable_control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() < 2 || !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes authoritative true/false join ownership")
                    .c_str());
  }

  std::size_t join_select_index = join_block->insts.size() - 1;
  auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
  if (join_select == nullptr && join_block->insts.size() >= 2) {
    join_select_index = join_block->insts.size() - 2;
    join_select = std::get_if<bir::SelectInst>(&join_block->insts[join_select_index]);
  }
  if (join_select == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes the expected select carrier")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                    ": prepared compare-join fixture published invalid true/false join ownership indices")
                    .c_str());
  }

  auto* true_predecessor =
      find_block(function,
                 std::string(block_label(
                                 prepared,
                                 join_transfer.edge_transfers[true_transfer_index].predecessor_label))
                     .c_str());
  auto* false_predecessor =
      find_block(function,
                 std::string(block_label(
                                 prepared,
                                 join_transfer.edge_transfers[false_transfer_index].predecessor_label))
                     .c_str());
  if (true_predecessor == nullptr || false_predecessor == nullptr ||
      true_predecessor == join_block || false_predecessor == join_block) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes both authoritative join predecessors")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.global.join.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });

    const std::string original_carrier_result_name = join_select->result.name;
    const auto renamed_carrier_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.join.global.edge.slot");
    join_block->insts[join_select_index] = bir::LoadLocalInst{
        .result = renamed_carrier_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    for (auto& inst : join_block->insts) {
      if (auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
        if (binary->lhs.kind == bir::Value::Kind::Named &&
            binary->lhs.name == original_carrier_result_name) {
          binary->lhs = renamed_carrier_result;
        }
        if (binary->rhs.kind == bir::Value::Kind::Named &&
            binary->rhs.name == original_carrier_result_name) {
          binary->rhs = renamed_carrier_result;
        }
      }
    }
  }

  const char* true_global_name =
      use_fixed_offset_global_roots ? "selected_zero_pair"
      : use_pointer_backed_global_roots ? "selected_zero_backing"
                                        : "selected_zero";
  const char* false_global_name =
      use_fixed_offset_global_roots ? "selected_nonzero_pair"
      : use_pointer_backed_global_roots ? "selected_nonzero_backing"
                                        : "selected_nonzero";
  const std::size_t global_byte_offset = use_fixed_offset_global_roots ? 4 : 0;

  const auto true_global_root =
      bir::Value::named(bir::TypeKind::I32, "contract.true.global.root");
  const auto false_global_root =
      bir::Value::named(bir::TypeKind::I32, "contract.false.global.root");
  join_block->insts.insert(
      join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
      bir::LoadGlobalInst{
          .result = true_global_root,
          .global_name = true_global_name,
          .byte_offset = global_byte_offset,
          .align_bytes = 4,
          .address = use_pointer_backed_global_roots
                         ? std::optional<bir::MemoryAddress>(bir::MemoryAddress{
                               .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                               .base_name = "selected_zero_root",
                               .size_bytes = 4,
                               .align_bytes = 4,
                           })
                         : std::nullopt,
      });
  ++join_select_index;
  join_block->insts.insert(
      join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
      bir::LoadGlobalInst{
          .result = false_global_root,
          .global_name = false_global_name,
          .byte_offset = global_byte_offset,
          .align_bytes = 4,
          .address = use_pointer_backed_global_roots
                         ? std::optional<bir::MemoryAddress>(bir::MemoryAddress{
                               .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                               .base_name = "selected_nonzero_root",
                               .size_bytes = 4,
                               .align_bytes = 4,
                           })
                         : std::nullopt,
      });
  if (use_selected_value_chain) {
    const auto true_global_selected =
        bir::Value::named(bir::TypeKind::I32, "contract.true.global.chain");
    const auto false_global_selected =
        bir::Value::named(bir::TypeKind::I32, "contract.false.global.chain");
    ++join_select_index;
    join_block->insts.insert(
        join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Add,
            .result = true_global_selected,
            .operand_type = bir::TypeKind::I32,
            .lhs = true_global_root,
            .rhs = bir::Value::immediate_i32(4),
        });
    ++join_select_index;
    join_block->insts.insert(
        join_block->insts.begin() + static_cast<std::ptrdiff_t>(join_select_index),
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Sub,
            .result = false_global_selected,
            .operand_type = bir::TypeKind::I32,
            .lhs = false_global_root,
            .rhs = bir::Value::immediate_i32(1),
        });
    join_transfer.edge_transfers[true_transfer_index].incoming_value = true_global_selected;
    join_transfer.edge_transfers[false_transfer_index].incoming_value = false_global_selected;
  } else {
    join_transfer.edge_transfers[true_transfer_index].incoming_value = true_global_root;
    join_transfer.edge_transfers[false_transfer_index].incoming_value = false_global_root;
  }
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label ==
        block_label(prepared, join_transfer.edge_transfers[true_transfer_index].predecessor_label)) {
      incoming.value = join_transfer.edge_transfers[true_transfer_index].incoming_value;
    } else if (incoming.label ==
               block_label(prepared,
                           join_transfer.edge_transfers[false_transfer_index].predecessor_label)) {
      incoming.value = join_transfer.edge_transfers[false_transfer_index].incoming_value;
    }
  }

  if (add_true_lane_passthrough_block) {
    true_predecessor->terminator = bir::BranchTerminator{.target_label = "contract.true.bridge"};
    function.blocks.push_back(bir::Block{
        .label = "contract.true.bridge",
        .terminator = bir::BranchTerminator{.target_label = join_block->label},
    });
  }
  if (add_false_lane_passthrough_block) {
    false_predecessor->terminator =
        bir::BranchTerminator{.target_label = "contract.false.bridge"};
    function.blocks.push_back(bir::Block{
        .label = "contract.false.bridge",
        .terminator = bir::BranchTerminator{.target_label = join_block->label},
    });
  }

  entry_block = find_block(function, "entry");
  join_block = find_block(function, "join");
  if (entry_block == nullptr || join_block == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer preserves entry/join blocks after passthrough rewrites")
                    .c_str());
  }

  if (reject_drifted_branch_condition || reject_missing_branch_record) {
    auto* rejection_control_flow = find_control_flow_function(prepared, function_name);
    if (rejection_control_flow == nullptr || rejection_control_flow->branch_conditions.size() != 1) {
      return fail((std::string(failure_context) +
                   ": prepared compare-join fixture no longer publishes the authoritative branch contract")
                      .c_str());
    }

    if (reject_drifted_branch_condition) {
      rejection_control_flow->branch_conditions.front().predicate.reset();
    } else {
      rejection_control_flow->branch_conditions.clear();
    }

    try {
      (void)c4c::backend::x86::api::emit_prepared_module(prepared);
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer unexpectedly accepted a broken compare-join prepared branch contract")
                      .c_str());
    } catch (const std::invalid_argument& error) {
      if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
          std::string_view::npos) {
        return fail((std::string(failure_context) +
                     ": x86 prepared-module consumer rejected the broken compare-join prepared branch contract with the wrong contract message")
                        .c_str());
      }
    }

    return 0;
  }

  const auto compare_join_context = prepare::find_prepared_param_zero_materialized_compare_join_context(
      prepared.names, *control_flow, function, *entry_block, function.params.front(), true);
  if (!compare_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the materialized compare-join context")
                    .c_str());
  }

  const auto prepared_join_branches =
      prepare::find_prepared_materialized_compare_join_branches(
          prepared.names, compare_join_context->compare_join_context);
  if (!prepared_join_branches.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the compare-join branch contract")
                    .c_str());
  }

  const auto published_global_names =
      prepare::collect_prepared_materialized_compare_join_same_module_globals(
          prepared.names, *prepared_join_branches);
  const auto require_published_global_names =
      [&](std::initializer_list<std::string_view> expected_names) -> int {
        if (published_global_names.size() != expected_names.size()) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped publishing the compare-join same-module global ownership set")
                          .c_str());
        }
        std::size_t index = 0;
        for (const auto expected_name : expected_names) {
          if (published_global_names[index] != expected_name) {
            return fail((std::string(failure_context) +
                         ": shared helper stopped publishing compare-join same-module globals in authoritative order")
                            .c_str());
          }
          ++index;
        }
        return 0;
      };
  if (use_pointer_backed_global_roots) {
    if (const auto status = require_published_global_names(
            {"selected_zero_root",
             true_global_name,
             "selected_nonzero_root",
             false_global_name});
        status != 0) {
      return status;
    }
  } else {
    if (const auto status =
            require_published_global_names({true_global_name, false_global_name});
        status != 0) {
      return status;
    }
  }

  const auto require_prepared_global_base =
      [&](const prepare::PreparedMaterializedCompareJoinReturnContext& return_context,
          const char* expected_global_name,
          std::size_t expected_global_byte_offset) -> int {
    const auto expected_kind =
        use_pointer_backed_global_roots
            ? prepare::PreparedComputedBaseKind::PointerBackedGlobalI32Load
            : prepare::PreparedComputedBaseKind::GlobalI32Load;
    if (return_context.selected_value.base.kind != expected_kind ||
        return_context.selected_value.base.global_name_id !=
            prepared.names.link_names.find(expected_global_name) ||
        return_context.selected_value.base.global_byte_offset != expected_global_byte_offset) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped classifying the selected-value base as a same-module global load")
                      .c_str());
    }
    if (use_pointer_backed_global_roots) {
      const char* expected_root_name =
          expected_global_name == true_global_name ? "selected_zero_root" : "selected_nonzero_root";
      if (return_context.selected_value.base.pointer_root_global_name_id !=
          prepared.names.link_names.find(expected_root_name)) {
        return fail((std::string(failure_context) +
                     ": shared helper stopped publishing the pointer-backed same-module global root")
                        .c_str());
      }
    }
    if (use_selected_value_chain) {
      if (return_context.selected_value.base.global_name_id ==
          prepared.names.link_names.find(true_global_name)) {
        if (return_context.selected_value.operations.size() != 1 ||
            return_context.selected_value.operations.front().opcode != bir::BinaryOpcode::Add ||
            return_context.selected_value.operations.front().immediate != 4) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped publishing the true global-root selected-value chain")
                          .c_str());
        }
      } else if (return_context.selected_value.base.global_name_id ==
                 prepared.names.link_names.find(false_global_name)) {
        if (return_context.selected_value.operations.size() != 1 ||
            return_context.selected_value.operations.front().opcode != bir::BinaryOpcode::Sub ||
            return_context.selected_value.operations.front().immediate != 1) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped publishing the false global-root selected-value chain")
                          .c_str());
        }
      } else {
        return fail((std::string(failure_context) +
                     ": shared helper stopped publishing the global-root selected-value chain")
                        .c_str());
      }
    } else if (!return_context.selected_value.operations.empty()) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped keeping direct global selected values operation-free")
                      .c_str());
    }
    if (!return_context.trailing_binary.has_value() ||
        return_context.trailing_binary->opcode != bir::BinaryOpcode::Xor ||
        return_context.trailing_binary->immediate != 3) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped publishing the trailing immediate-op contract")
                      .c_str());
    }
    return 0;
  };

  if (const auto status = require_prepared_global_base(
          prepared_join_branches->true_return_context, true_global_name, global_byte_offset);
      status != 0) {
    return status;
  }
  if (const auto status = require_prepared_global_base(
          prepared_join_branches->false_return_context, false_global_name, global_byte_offset);
      status != 0) {
    return status;
  }

  const auto prepared_compare_join_packet =
      prepare::find_prepared_param_zero_materialized_compare_join_branches(
          prepared.names, *mutable_control_flow, function, *entry_block, function.params.front(), true);
  if (!prepared_compare_join_packet.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the compare-join packet for resolved arm ownership")
                    .c_str());
  }
  const auto resolved_render_contract =
      prepare::find_prepared_resolved_materialized_compare_join_render_contract(
          prepared.names, prepared.module, *prepared_compare_join_packet);
  if (!resolved_render_contract.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer resolves compare-join arm globals through the render contract")
                    .c_str());
  }

  const auto require_resolved_return_arm =
      [&](const prepare::PreparedResolvedMaterializedCompareJoinReturnArm& return_arm,
          const char* expected_global_name,
          std::size_t expected_global_byte_offset) -> int {
        if (return_arm.arm.context.selected_value.base.global_name_id !=
                prepared.names.link_names.find(expected_global_name) ||
            return_arm.arm.context.selected_value.base.global_byte_offset !=
                expected_global_byte_offset ||
            return_arm.global == nullptr ||
            return_arm.global->name != expected_global_name) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped packaging resolved compare-join arm globals")
                          .c_str());
        }
        if (use_pointer_backed_global_roots) {
          const char* expected_root_name =
              expected_global_name == true_global_name ? "selected_zero_root"
                                                       : "selected_nonzero_root";
          if (return_arm.arm.context.selected_value.base.pointer_root_global_name_id !=
                  prepared.names.link_names.find(expected_root_name) ||
              return_arm.pointer_root_global == nullptr ||
              return_arm.pointer_root_global->name != expected_root_name ||
              return_arm.pointer_root_global->type != bir::TypeKind::Ptr) {
            return fail((std::string(failure_context) +
                         ": shared helper stopped packaging resolved pointer-backed compare-join arm globals")
                            .c_str());
          }
        } else if (return_arm.pointer_root_global != nullptr) {
          return fail((std::string(failure_context) +
                       ": shared helper published an unexpected pointer-root global for a direct compare-join arm")
                          .c_str());
        }
        return 0;
      };

  if (const auto status = require_resolved_return_arm(
          resolved_render_contract->true_return, true_global_name, global_byte_offset);
      status != 0) {
    return status;
  }
  if (const auto status = require_resolved_return_arm(
          resolved_render_contract->false_return, false_global_name, global_byte_offset);
      status != 0) {
    return status;
  }
  if (expected_asm != nullptr) {
    const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
    if (prepared_asm != *expected_asm) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer stopped preferring the prepared compare-join entry branch contract")
                      .c_str());
    }
  }
  return 0;
}

int check_materialized_compare_join_branches_publish_prepared_global_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, false, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, false, false);
}

int check_materialized_compare_join_branches_publish_prepared_global_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, false, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, false, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_global_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, false, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, false, false, false, true);
}

int check_materialized_compare_join_global_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      false,
      false,
      false,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_global_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      false,
      false,
      false,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_global_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      false,
      false,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_global_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      false,
      false,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_global_chain_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      false,
      false,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_global_chain_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      false,
      false,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_global_chain_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      false,
      false,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_global_chain_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      false,
      false,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_global_chain_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, false, false, false, false, true);
}

int check_materialized_compare_join_edge_store_slot_global_chain_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, false, false, false, false, true);
}

int check_materialized_compare_join_global_chain_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, false, false, false, false, false, true);
}

int check_materialized_compare_join_edge_store_slot_global_chain_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, false, false, false, false, false, true);
}

int check_materialized_compare_join_fixed_offset_global_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      false,
      true,
      false,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_fixed_offset_global_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      false,
      true,
      false,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_fixed_offset_global_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      false,
      true,
      false,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_fixed_offset_global_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      false,
      true,
      false,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_fixed_offset_global_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      false,
      true,
      false,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_fixed_offset_global_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      false,
      true,
      false,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_fixed_offset_global_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      true,
      false,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      true,
      false,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_fixed_offset_global_chain_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      true,
      false,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      true,
      false,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_fixed_offset_global_chain_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      true,
      false,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      true,
      false,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_pointer_backed_global_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      false,
      false,
      true,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_pointer_backed_global_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      false,
      false,
      true,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_pointer_backed_global_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      false,
      true,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      false,
      true,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_pointer_backed_global_chain_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      false,
      true,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      false,
      true,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_pointer_backed_global_chain_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      false,
      true,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      false,
      true,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, true, false, false, false, true);
}

int check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, true, false, false, false, true);
}

int check_materialized_compare_join_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, true, false, false, false, false, true);
}

int check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, true, false, false, false, false, true);
}

int check_materialized_compare_join_offset_pointer_backed_global_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      true,
      true,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      true,
      true,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_offset_pointer_backed_global_chain_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      true,
      true,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      true,
      true,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_offset_pointer_backed_global_chain_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      true,
      true,
      true,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      true,
      true,
      true,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_offset_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, true, false, false, false, true);
}

int check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, true, false, false, false, true);
}

int check_materialized_compare_join_offset_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, true, false, false, false, false, true);
}

int check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, true, false, false, false, false, true);
}

int check_materialized_compare_join_fixed_offset_global_chain_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, false, false, false, false, true);
}

int check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_requires_authoritative_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, false, false, false, false, true);
}

int check_materialized_compare_join_fixed_offset_global_chain_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, false, false, false, false, false, true);
}

int check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, false, false, false, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_global_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, false);
}

int check_materialized_compare_join_branches_publish_prepared_global_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_global_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, true, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, true, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, true, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, true, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, false, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, false, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, false, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, false, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, false, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, false, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, false, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, true, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, true, true);
}

int check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, true, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, true, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, false, true, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, false, true, true, false, true);
}

int check_materialized_compare_join_offset_pointer_backed_global_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      false,
      true,
      true,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_route_ignores_non_compare_entry_carrier(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      false,
      true,
      true,
      false,
      false,
      true,
      &expected_asm);
}

int check_materialized_compare_join_offset_pointer_backed_global_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      false,
      true,
      true,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_route_with_true_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      false,
      true,
      true,
      true,
      false,
      false,
      &expected_asm);
}

int check_materialized_compare_join_offset_pointer_backed_global_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      false,
      false,
      true,
      true,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_route_with_false_lane_passthrough(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module,
      function_name,
      failure_context,
      true,
      false,
      true,
      true,
      false,
      true,
      false,
      &expected_asm);
}

int check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_chain_return_contexts(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, true);
}

int check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_chain_return_contexts_with_true_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, true, true, false);
}

int check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, false, true, true, true, false, true);
}

int check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_chain_return_contexts_with_false_lane_passthrough(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
      module, function_name, failure_context, true, true, true, true, false, true);
}


}  // namespace

int run_backend_x86_handoff_boundary_joined_branch_tests() {
  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_asm(
                  "branch_join_adjust", "is_nonzero", 5, 1),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  bir.ret i32 merge",
              "scalar-control-flow compare-against-zero joined branch lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_asm(
                  "branch_join_adjust", "is_nonzero", 5, 1),
              "branch_join_adjust",
              "scalar-control-flow compare-against-zero joined branch lane prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_asm(
                  "branch_join_adjust", "is_nonzero", 5, 1),
              "branch_join_adjust",
              "scalar-control-flow compare-against-zero joined branch lane EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_route_rejects_authoritative_join_contract(
              make_x86_param_eq_zero_branch_joined_add_or_sub_module(),
              "branch_join_adjust",
              "scalar-control-flow compare-against-zero joined branch lane rejects fallback to the plain param-zero branch consumer when authoritative join ownership remains active");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_edge_store_slot_route_rejects_authoritative_join_contract(
              make_x86_param_eq_zero_branch_joined_add_or_sub_module(),
              "branch_join_adjust",
              "scalar-control-flow compare-against-zero joined branch EdgeStoreSlot lane rejects fallback to the plain param-zero branch consumer when authoritative join ownership remains active");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_outputs(
              make_x86_param_ne_zero_branch_joined_sub_or_add_module(),
              expected_minimal_param_ne_zero_branch_joined_add_or_sub_asm(
                  "branch_join_adjust_nonzero", "is_zero", 1, 5),
              "join:\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  zero.adjusted = bir.add i32 p.x, 5\n  merge = bir.select ne i32 p.x, 0, i32 nonzero.adjusted, zero.adjusted\n  bir.ret i32 merge",
              "scalar-control-flow compare-against-zero nonzero joined branch lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_ne_zero_branch_joined_sub_or_add_module(),
              expected_minimal_param_ne_zero_branch_joined_add_or_sub_asm(
                  "branch_join_adjust_nonzero", "is_zero", 1, 5),
              "branch_join_adjust_nonzero",
              "scalar-control-flow compare-against-zero nonzero joined branch lane prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_ne_zero_branch_joined_sub_or_add_module(),
              expected_minimal_param_ne_zero_branch_joined_add_or_sub_asm(
                  "branch_join_adjust_nonzero", "is_zero", 1, 5),
              "branch_join_adjust_nonzero",
              "scalar-control-flow compare-against-zero nonzero joined branch lane EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  joined = bir.add i32 merge, 2\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join arithmetic");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join arithmetic prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join arithmetic ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join arithmetic ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join arithmetic EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join arithmetic EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join arithmetic EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-arithmetic render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-arithmetic branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-arithmetic branch-plan ownership ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero trailing-arithmetic compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero trailing-arithmetic compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero trailing-arithmetic compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_return_bundle(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero trailing-arithmetic compare-join route rejects reopening a direct ABI return when the authoritative prepared return bundle is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-arithmetic EdgeStoreSlot branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_add_asm(
                  "branch_join_adjust_then_add", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero trailing-arithmetic compare-join EdgeStoreSlot route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero trailing-arithmetic compare-join EdgeStoreSlot route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_add_module(),
              "branch_join_adjust_then_add",
              "scalar-control-flow compare-against-zero trailing-arithmetic compare-join EdgeStoreSlot route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  joined = bir.xor i32 merge, 3\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join xor");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join xor prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_unreachable_block_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane ignores unrelated block count when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_return_contexts(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join branch return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-xor render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join branch-plan ownership ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_consumes_authoritative_prepared_param_home(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_with_source_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", "r10d", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join route sources parameter-selected returns from the authoritative prepared value home");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join EdgeStoreSlot route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join EdgeStoreSlot route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join EdgeStoreSlot route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join xor prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join xor ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join xor ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join xor EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join xor EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join xor EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_chained_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "carrier.nonzero", 5, 1, 0, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with selected-value chain prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "join:\n  merge = bir.select eq i32 p.x, 0, i32 5, 1\n  joined = bir.xor i32 merge, 3\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected values");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected values prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_rejects_drifted_immediate_move_source(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected values rejects a drifted authoritative prepared immediate move source");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected values EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected values ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected values EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected values ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected values EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join immediate return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_return_contexts(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot immediate return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join immediate return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot immediate return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join immediate return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot immediate return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_immediate_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero immediate selected values compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_immediate_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediates_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 1, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero immediate selected values EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_immediate_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "carrier.nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected-value chain prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_immediate_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "carrier.nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected-value chain ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_immediate_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "carrier.nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected-value chain EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_immediate_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "carrier.nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected-value chain EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_immediate_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "carrier.nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected-value chain ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_immediate_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "carrier.nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected-value chain EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_immediate_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join immediate selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot immediate selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_immediate_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join immediate selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot immediate selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_immediate_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join immediate selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_immediate_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot immediate selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_immediate_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero immediate selected-value chain compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_immediate_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "is_nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero immediate selected-value chain EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_global_selected_values_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
                  "branch_join_global_then_xor", "carrier.nonzero", "selected_zero", "selected_nonzero", 3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected values prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_global_selected_values_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
                  "branch_join_global_then_xor", "carrier.nonzero", "selected_zero", "selected_nonzero", 3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected values EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
                  "branch_join_global_then_xor", "carrier.nonzero", "selected_zero", "selected_nonzero", 3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected values ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
                  "branch_join_global_then_xor", "carrier.nonzero", "selected_zero", "selected_nonzero", 3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected values EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
                  "branch_join_global_then_xor", "carrier.nonzero", "selected_zero", "selected_nonzero", 3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected values ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
                  "branch_join_global_then_xor", "carrier.nonzero", "selected_zero", "selected_nonzero", 3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected values EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join same-module global return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_global_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join same-module global return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_global_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join same-module global return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_global_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
                  "branch_join_global_then_xor",
                  "is_nonzero",
                  "selected_zero",
                  "selected_nonzero",
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot same-module global return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot same-module global return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot same-module global return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_global_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_globals_then_xor_asm(
                  "branch_join_global_then_xor",
                  "is_nonzero",
                  "selected_zero",
                  "selected_nonzero",
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_pointer_backed_global_selected_values_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected values prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_pointer_backed_global_selected_values_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected values EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_pointer_backed_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected values ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_pointer_backed_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected values EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_pointer_backed_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected values ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_pointer_backed_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected values EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot pointer-backed same-module global return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot pointer-backed same-module global return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot pointer-backed same-module global return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_pointer_backed_global_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_pointer_backed_global_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_pointer_backed_global_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected-value chain prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_pointer_backed_global_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected-value chain EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_pointer_backed_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected-value chain ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_pointer_backed_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected-value chain EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_pointer_backed_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected-value chain ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_pointer_backed_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with pointer-backed same-module global selected-value chain EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot pointer-backed same-module global selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_pointer_backed_global_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot pointer-backed same-module global selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_pointer_backed_global_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_pointer_backed_global_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot pointer-backed same-module global selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_pointer_backed_global_chain_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_pointer_backed_global_chain_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_backing",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_backing",
                  1,
                  3),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_pointer_backed_globals_then_xor_module(),
              "branch_join_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_global_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected-value chain prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected-value chain ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_global_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected-value chain EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected-value chain EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected-value chain ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with same-module global selected-value chain EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_global_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join same-module global selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot same-module global selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_global_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join same-module global selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot same-module global selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_global_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join same-module global selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_global_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot same-module global selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_global_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "is_nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_global_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "is_nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_global_chain_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "is_nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_global_chain_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "is_nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain EdgeStoreSlot compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_global_chain_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "is_nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_global_chain_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_global_chains_then_xor_asm(
                  "branch_join_global_then_xor",
                  "is_nonzero",
                  "selected_zero",
                  4,
                  "selected_nonzero",
                  1,
                  3),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain EdgeStoreSlot compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_global_chain_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_global_chain_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_global_chain_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain EdgeStoreSlot compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_global_chain_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero same-module global selected-value chain EdgeStoreSlot compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_fixed_offset_global_selected_values_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected values prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_fixed_offset_global_selected_values_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected values EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_fixed_offset_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected values ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_fixed_offset_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected values EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_fixed_offset_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected values ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_fixed_offset_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected values EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset same-module global return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset same-module global return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset same-module global return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset same-module global return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset same-module global return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset same-module global return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_fixed_offset_global_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_fixed_offset_global_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_fixed_offset_global_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_fixed_offset_global_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global EdgeStoreSlot compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_fixed_offset_global_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_fixed_offset_global_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_globals_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global EdgeStoreSlot compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_fixed_offset_global_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected-value chain prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_fixed_offset_global_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected-value chain EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_fixed_offset_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected-value chain ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_fixed_offset_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected-value chain EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_fixed_offset_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected-value chain ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_fixed_offset_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset same-module global selected-value chain EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset same-module global selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset same-module global selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset same-module global selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset same-module global selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_fixed_offset_global_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset same-module global selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_fixed_offset_global_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset same-module global selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_fixed_offset_global_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_fixed_offset_global_chain_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain EdgeStoreSlot compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_fixed_offset_global_chain_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_global_chains_then_xor_asm(
                  "branch_join_offset_global_then_xor",
                  "is_nonzero",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain EdgeStoreSlot compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_fixed_offset_global_chain_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_fixed_offset_global_chain_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain EdgeStoreSlot compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_fixed_offset_global_chain_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_offset_globals_then_xor_module(),
              "branch_join_offset_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset same-module global selected-value chain EdgeStoreSlot compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_offset_pointer_backed_global_selected_values_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected values prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_offset_pointer_backed_global_selected_values_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected values EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_offset_pointer_backed_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected values ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_offset_pointer_backed_global_selected_values_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected values EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_offset_pointer_backed_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected values ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_offset_pointer_backed_global_selected_values_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected values EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset pointer-backed same-module global return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset pointer-backed same-module global return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset pointer-backed same-module global return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset pointer-backed same-module global return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset pointer-backed same-module global return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset pointer-backed same-module global return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_offset_pointer_backed_global_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_offset_pointer_backed_global_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global EdgeStoreSlot compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_offset_pointer_backed_global_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global EdgeStoreSlot compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_offset_pointer_backed_global_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected-value chain prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_offset_pointer_backed_global_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected-value chain EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset pointer-backed same-module global selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_chain_return_contexts(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset pointer-backed same-module global selected-value chain return context ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_offset_pointer_backed_global_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_offset_pointer_backed_global_chain_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_offset_pointer_backed_global_chain_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "is_nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset pointer-backed same-module global selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_chain_return_contexts_with_true_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset pointer-backed same-module global selected-value chain return context ownership ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_offset_pointer_backed_global_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join fixed-offset pointer-backed same-module global selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branches_publish_prepared_edge_store_slot_offset_pointer_backed_global_chain_return_contexts_with_false_lane_passthrough(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join EdgeStoreSlot fixed-offset pointer-backed same-module global selected-value chain return context ownership ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_offset_pointer_backed_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected-value chain ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_offset_pointer_backed_global_selected_value_chain_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected-value chain EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_offset_pointer_backed_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected-value chain ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_offset_pointer_backed_global_selected_value_chain_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_offset_pointer_backed_global_chains_then_xor_asm(
                  "branch_join_offset_pointer_backed_global_then_xor",
                  "carrier.nonzero",
                  "selected_zero_root",
                  "selected_zero_pair",
                  4,
                  4,
                  "selected_nonzero_root",
                  "selected_nonzero_pair",
                  4,
                  1,
                  3),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with fixed-offset pointer-backed same-module global selected-value chain EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_offset_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_offset_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_offset_pointer_backed_global_chain_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_offset_pointer_backed_globals_then_xor_module(),
              "branch_join_offset_pointer_backed_global_then_xor",
              "scalar-control-flow compare-against-zero fixed-offset pointer-backed same-module global selected-value chain EdgeStoreSlot compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "is_nonzero", 5, 1, 15),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  joined = bir.and i32 merge, 15\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join and");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "is_nonzero", 5, 1, 15),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join and prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "is_nonzero", 5, 1, 15),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join and ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "is_nonzero", 5, 1, 15),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join and ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "carrier.nonzero", 5, 1, 15),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join and EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "carrier.nonzero", 5, 1, 15),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join and EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "carrier.nonzero", 5, 1, 15),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join and EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-and render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-and branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-and branch-plan ownership ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "is_nonzero", 5, 1, 15),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero trailing-and compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero trailing-and compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero trailing-and compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-and EdgeStoreSlot branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_and_asm(
                  "branch_join_adjust_then_and", "is_nonzero", 5, 1, 15),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero trailing-and compare-join EdgeStoreSlot route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero trailing-and compare-join EdgeStoreSlot route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_and_module(),
              "branch_join_adjust_then_and",
              "scalar-control-flow compare-against-zero trailing-and compare-join EdgeStoreSlot route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "is_nonzero", 5, 1, 12),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  joined = bir.or i32 merge, 12\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join or");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "is_nonzero", 5, 1, 12),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join or prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "is_nonzero", 5, 1, 12),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join or ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "is_nonzero", 5, 1, 12),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join or ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "carrier.nonzero", 5, 1, 12),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join or EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "carrier.nonzero", 5, 1, 12),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join or EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "carrier.nonzero", 5, 1, 12),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join or EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-or render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-or branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-or branch-plan ownership ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "is_nonzero", 5, 1, 12),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero trailing-or compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero trailing-or compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero trailing-or compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-or EdgeStoreSlot branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_or_asm(
                  "branch_join_adjust_then_or", "is_nonzero", 5, 1, 12),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero trailing-or compare-join EdgeStoreSlot route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero trailing-or compare-join EdgeStoreSlot route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_or_module(),
              "branch_join_adjust_then_or",
              "scalar-control-flow compare-against-zero trailing-or compare-join EdgeStoreSlot route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "is_nonzero", 5, 1, 3),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  joined = bir.mul i32 merge, 3\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join mul");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join mul prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join mul ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join mul ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "carrier.nonzero", 5, 1, 3),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join mul EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "carrier.nonzero", 5, 1, 3),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join mul EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "carrier.nonzero", 5, 1, 3),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join mul EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-mul render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-mul branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-mul branch-plan ownership ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero trailing-mul compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero trailing-mul compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero trailing-mul compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-mul EdgeStoreSlot branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_mul_asm(
                  "branch_join_adjust_then_mul", "is_nonzero", 5, 1, 3),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero trailing-mul compare-join EdgeStoreSlot route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero trailing-mul compare-join EdgeStoreSlot route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_mul_module(),
              "branch_join_adjust_then_mul",
              "scalar-control-flow compare-against-zero trailing-mul compare-join EdgeStoreSlot route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "is_nonzero", 5, 1, 2),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  joined = bir.shl i32 merge, 2\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join shl");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join shl prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join shl ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join shl ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join shl EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join shl EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join shl EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-shl render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-shl branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-shl branch-plan ownership ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero trailing-shl compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero trailing-shl compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero trailing-shl compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-shl EdgeStoreSlot branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_shl_asm(
                  "branch_join_adjust_then_shl", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero trailing-shl compare-join EdgeStoreSlot route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero trailing-shl compare-join EdgeStoreSlot route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_shl_module(),
              "branch_join_adjust_then_shl",
              "scalar-control-flow compare-against-zero trailing-shl compare-join EdgeStoreSlot route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "is_nonzero", 5, 1, 2),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  joined = bir.lshr i32 merge, 2\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join lshr");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join lshr prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join lshr ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join lshr ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join lshr EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join lshr EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join lshr EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-lshr render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-lshr branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-lshr branch-plan ownership ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero trailing-lshr compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero trailing-lshr compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero trailing-lshr compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-lshr EdgeStoreSlot branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_lshr_asm(
                  "branch_join_adjust_then_lshr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero trailing-lshr compare-join EdgeStoreSlot route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero trailing-lshr compare-join EdgeStoreSlot route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_lshr_module(),
              "branch_join_adjust_then_lshr",
              "scalar-control-flow compare-against-zero trailing-lshr compare-join EdgeStoreSlot route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "is_nonzero", 5, 1, 2),
              "join:\n  zero.adjusted = bir.add i32 p.x, 5\n  nonzero.adjusted = bir.sub i32 p.x, 1\n  merge = bir.select eq i32 p.x, 0, i32 zero.adjusted, nonzero.adjusted\n  joined = bir.ashr i32 merge, 2\n  bir.ret i32 joined",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join ashr");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join ashr prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join ashr ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join ashr ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join ashr EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_true_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join ashr EdgeStoreSlot ignores true-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_join_route_edge_store_slot_with_false_lane_passthrough_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "carrier.nonzero", 5, 1, 2),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero joined branch lane with trailing join ashr EdgeStoreSlot ignores false-lane passthrough topology when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_render_contract_publishes_prepared_globals_and_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-ashr render-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-ashr branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_branch_plan_helper_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-ashr branch-plan ownership ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero trailing-ashr compare-join route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero trailing-ashr compare-join route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero trailing-ashr compare-join route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero prepared compare-join trailing-ashr EdgeStoreSlot branch-plan ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_ignores_non_compare_entry_carrier(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              expected_minimal_param_eq_zero_branch_joined_add_or_sub_then_ashr_asm(
                  "branch_join_adjust_then_ashr", "is_nonzero", 5, 1, 2),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero trailing-ashr compare-join EdgeStoreSlot route ignores non-compare entry carrier state when prepared-control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_condition(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero trailing-ashr compare-join EdgeStoreSlot route rejects a drifted authoritative prepared branch contract instead of falling back past the compare-join handoff");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_materialized_compare_join_edge_store_slot_route_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_ashr_module(),
              "branch_join_adjust_then_ashr",
              "scalar-control-flow compare-against-zero trailing-ashr compare-join EdgeStoreSlot route rejects reopening raw recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }

  return 0;
}
