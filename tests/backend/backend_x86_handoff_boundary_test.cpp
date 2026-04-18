#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"
#include "src/codegen/lir/ir.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
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
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

prepare::PreparedControlFlowFunction* find_control_flow_function(prepare::PreparedBirModule& prepared,
                                                                 const char* function_name) {
  for (auto& function : prepared.control_flow.functions) {
    if (function.function_name == function_name) {
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

bir::Module make_x86_param_passthrough_module();

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
      c4c::backend::prepare::call_result_destination_register_name(x86_target_profile(), *abi);
  if (!wide_register.has_value()) {
    throw std::runtime_error("missing canonical i32 return register for x86 handoff test");
  }
  return narrow_abi_register(*wide_register);
}

std::string minimal_i32_param_register() {
  const auto prepared = c4c::backend::prepare::prepare_semantic_bir_module_with_options(
      make_x86_param_passthrough_module(), x86_target_profile());
  if (prepared.module.functions.empty() || prepared.module.functions.front().params.empty() ||
      !prepared.module.functions.front().params.front().abi.has_value()) {
    throw std::runtime_error("missing prepared i32 parameter ABI metadata for x86 handoff test");
  }
  const auto wide_register = c4c::backend::prepare::call_arg_destination_register_name(
      x86_target_profile(), *prepared.module.functions.front().params.front().abi, 0);
  if (!wide_register.has_value()) {
    throw std::runtime_error("missing canonical i32 parameter register for x86 handoff test");
  }
  return narrow_abi_register(*wide_register);
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
}

std::string expected_minimal_param_binary_asm(const char* function_name,
                                              const char* mnemonic,
                                              int immediate) {
  const auto return_register = minimal_i32_return_register();
  const auto param_register = minimal_i32_param_register();
  return asm_header(function_name) + "    mov " + return_register + ", " + param_register +
         "\n    " + mnemonic + " " + return_register + ", " + std::to_string(immediate) +
         "\n    ret\n";
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

std::string expected_minimal_constant_return_asm(const char* function_name, int returned_value) {
  return asm_header(function_name) + "    mov " + minimal_i32_return_register() + ", " +
         std::to_string(returned_value) + "\n    ret\n";
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

std::string expected_minimal_param_passthrough_asm(const char* function_name) {
  return asm_header(function_name) + "    mov " + minimal_i32_return_register() + ", " +
         minimal_i32_param_register() + "\n    ret\n";
}

std::string expected_minimal_local_pointer_chain_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 32\n"
         "    mov DWORD PTR [rsp], 0\n"
         "    lea rax, [rsp]\n"
         "    mov QWORD PTR [rsp + 8], rax\n"
         "    lea rax, [rsp + 8]\n"
         "    mov QWORD PTR [rsp + 16], rax\n"
         "    mov rax, QWORD PTR [rsp + 16]\n"
         "    mov rax, QWORD PTR [rsp + 8]\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    add rsp, 32\n"
         "    ret\n";
}

std::string expected_minimal_local_pointer_guard_chain_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 32\n"
         "    mov DWORD PTR [rsp], 0\n"
         "    lea rax, [rsp]\n"
         "    mov QWORD PTR [rsp + 8], rax\n"
         "    lea rax, [rsp + 8]\n"
         "    mov QWORD PTR [rsp + 16], rax\n"
         "    mov rax, QWORD PTR [rsp + 8]\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    test eax, eax\n"
         "    je .Lmain_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 32\n"
         "    ret\n"
         ".Lmain_block_2:\n"
         "    mov rax, QWORD PTR [rsp + 16]\n"
         "    mov rax, QWORD PTR [rsp + 8]\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    test eax, eax\n"
         "    je .Lmain_block_4\n"
         "    mov eax, 1\n"
         "    add rsp, 32\n"
         "    ret\n"
         ".Lmain_block_4:\n"
         "    mov rax, QWORD PTR [rsp + 16]\n"
         "    mov rax, QWORD PTR [rsp + 8]\n"
         "    mov DWORD PTR [rsp], 1\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    test eax, eax\n"
         "    je .Lmain_block_7\n"
         "    mov eax, 0\n"
         "    add rsp, 32\n"
         "    ret\n"
         ".Lmain_block_7:\n"
         "    mov eax, 1\n"
         "    add rsp, 32\n"
         "    ret\n";
}

std::string expected_minimal_local_i32_immediate_guard_asm(const char* function_name,
                                                           int immediate) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov DWORD PTR [rsp], " + std::to_string(immediate) + "\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    cmp eax, " + std::to_string(immediate) + "\n"
         "    je .L" + function_name + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_local_i32_add_chain_guard_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov DWORD PTR [rsp], 1\n"
         "    mov DWORD PTR [rsp + 4], 2\n"
         "    mov DWORD PTR [rsp + 8], 3\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    add eax, DWORD PTR [rsp + 4]\n"
         "    add eax, DWORD PTR [rsp + 8]\n"
         "    cmp eax, 6\n"
         "    je .L" + std::string(function_name) + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_local_i32_sub_guard_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov DWORD PTR [rsp], 1\n"
         "    mov DWORD PTR [rsp + 4], 1\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    sub eax, DWORD PTR [rsp + 4]\n"
         "    test eax, eax\n"
         "    je .L" + std::string(function_name) + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_local_i32_byte_storage_asm(const char* function_name,
                                                        int immediate) {
  return asm_header(function_name) + "    sub rsp, 32\n"
         "    mov DWORD PTR [rsp], " + std::to_string(immediate) + "\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    add rsp, 32\n"
         "    ret\n";
}

std::string expected_minimal_local_i8_address_guard_asm(const char* function_name,
                                                        int immediate) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov BYTE PTR [rsp + 7], " + std::to_string(immediate) + "\n"
         "    movsx eax, BYTE PTR [rsp + 7]\n"
         "    cmp eax, " + std::to_string(immediate) + "\n"
         "    je .L" + function_name + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    movsx eax, BYTE PTR [rsp + 7]\n"
         "    cmp eax, " + std::to_string(immediate) + "\n"
         "    je .L" + function_name + "_block_4\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_4:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_loaded_local_array_pointer_guard_asm(const char* function_name,
                                                                  int immediate) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov BYTE PTR [rsp + 7], " + std::to_string(immediate) + "\n"
         "    mov rax, QWORD PTR [rsp + 8]\n"
         "    movsx eax, BYTE PTR [rsp + 7]\n"
         "    cmp eax, " + std::to_string(immediate) + "\n"
         "    je .L" + function_name + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_local_i32_short_circuit_or_guard_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov DWORD PTR [rsp], 1\n"
         "    mov DWORD PTR [rsp], 3\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    cmp eax, 3\n"
         "    je .L" + function_name + "_logic.rhs.7\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_logic.rhs.7:\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    cmp eax, 3\n"
         "    je .L" + function_name + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_loop_countdown_join_asm(const char* function_name) {
  return asm_header(function_name) + "    mov eax, 3\n"
         ".L" + function_name + "_loop:\n"
         "    test eax, eax\n"
         "    je .L" + function_name + "_exit\n"
         ".L" + function_name + "_body:\n"
         "    sub eax, 1\n"
         "    jmp .L" + function_name + "_loop\n"
         ".L" + function_name + "_exit:\n"
         "    ret\n";
}

std::string expected_minimal_local_i16_increment_guard_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov WORD PTR [rsp], 0\n"
         "    movsx eax, WORD PTR [rsp]\n"
         "    add eax, 1\n"
         "    mov WORD PTR [rsp], ax\n"
         "    movsx eax, WORD PTR [rsp]\n"
         "    cmp eax, 1\n"
         "    je .L" + std::string(function_name) + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_local_i16_i64_sub_return_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov WORD PTR [rsp], 1\n"
         "    mov QWORD PTR [rsp + 8], 1\n"
         "    movsx rax, WORD PTR [rsp]\n"
         "    sub rax, QWORD PTR [rsp + 8]\n"
         "    mov WORD PTR [rsp], ax\n"
         "    movsx eax, WORD PTR [rsp]\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_i32_immediate_guard_chain_asm(const char* function_name) {
  return asm_header(function_name) + "    mov eax, 0\n"
         "    cmp eax, 0\n"
         "    je .L" + std::string(function_name) + "_block_2\n"
         "    mov eax, 1\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 1\n"
         "    cmp eax, 1\n"
         "    je .L" + std::string(function_name) + "_block_4\n"
         "    mov eax, 2\n"
         "    ret\n"
         ".L" + function_name + "_block_4:\n"
         "    mov eax, 0\n"
         "    ret\n";
}

std::string expected_minimal_same_module_global_i32_guard_chain_asm(const char* function_name) {
  return asm_header(function_name)
         + "    mov eax, DWORD PTR [rip + s]\n"
           "    cmp eax, 1\n"
           "    je .L" + std::string(function_name) + "_block_2\n"
           "    mov eax, 1\n"
           "    ret\n"
           ".L" + function_name + "_block_2:\n"
           "    mov eax, DWORD PTR [rip + s + 4]\n"
           "    cmp eax, 2\n"
           "    je .L" + std::string(function_name) + "_block_4\n"
           "    mov eax, 2\n"
           "    ret\n"
           ".L" + function_name + "_block_4:\n"
           "    mov eax, 0\n"
           "    ret\n"
           ".data\n.globl s\n.type s, @object\n.p2align 2\ns:\n"
           "    .long 1\n"
           "    .long 2\n";
}

std::string expected_minimal_pointer_backed_same_module_global_guard_chain_asm(
    const char* function_name) {
  return asm_header(function_name)
         + "    mov rax, QWORD PTR [rip + s]\n"
           "    mov eax, DWORD PTR [rip + s_backing]\n"
           "    cmp eax, 1\n"
           "    je .L" + std::string(function_name) + "_block_2\n"
           "    mov eax, 1\n"
           "    ret\n"
           ".L" + function_name + "_block_2:\n"
           "    mov eax, DWORD PTR [rip + s_backing + 4]\n"
           "    cmp eax, 2\n"
           "    je .L" + function_name + "_block_4\n"
           "    mov eax, 2\n"
           "    ret\n"
           ".L" + function_name + "_block_4:\n"
           "    mov rax, QWORD PTR [rip + s_backing + 8]\n"
           "    mov eax, DWORD PTR [rip + gs1]\n"
           "    cmp eax, 1\n"
           "    je .L" + function_name + "_block_6\n"
           "    mov eax, 3\n"
           "    ret\n"
           ".L" + function_name + "_block_6:\n"
           "    mov eax, DWORD PTR [rip + gs1 + 4]\n"
           "    cmp eax, 2\n"
           "    je .L" + function_name + "_block_8\n"
           "    mov eax, 4\n"
           "    ret\n"
           ".L" + function_name + "_block_8:\n"
           "    mov eax, DWORD PTR [rip + s_backing + 16]\n"
           "    cmp eax, 1\n"
           "    je .L" + function_name + "_block_10\n"
           "    mov eax, 5\n"
           "    ret\n"
           ".L" + function_name + "_block_10:\n"
           "    mov eax, DWORD PTR [rip + s_backing + 20]\n"
           "    cmp eax, 2\n"
           "    je .L" + function_name + "_block_12\n"
           "    mov eax, 6\n"
           "    ret\n"
           ".L" + function_name + "_block_12:\n"
           "    mov eax, 0\n"
           "    ret\n"
           ".data\n.globl s\n.type s, @object\n.p2align 2\ns:\n"
           "    .quad s_backing\n"
           ".data\n.globl s_backing\n.type s_backing, @object\n.p2align 2\ns_backing:\n"
           "    .long 1\n"
           "    .long 2\n"
           "    .quad gs1\n"
           "    .long 1\n"
           "    .long 2\n"
           ".data\n.globl gs1\n.type gs1, @object\n.p2align 2\ngs1:\n"
           "    .long 1\n"
           "    .long 2\n";
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

std::string expected_minimal_param_eq_zero_branch_asm(const char* function_name,
                                                      const char* false_label,
                                                      int true_returned_value,
                                                      int false_returned_value) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + std::to_string(true_returned_value) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " +
         std::to_string(false_returned_value) + "\n    ret\n";
}

std::string expected_minimal_param_eq_zero_branch_param_or_immediate_asm(
    const char* function_name,
    const char* false_label,
    int true_returned_value) {
  return expected_branch_prefix(function_name, false_label) + "    mov " +
         minimal_i32_return_register() + ", " + std::to_string(true_returned_value) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + minimal_i32_param_register() + "\n    ret\n";
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
         ", @object\n.p2align 2\n" + std::string(true_global_name) + ":\n    .long 5\n.data\n.globl " +
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
         std::string(true_global_name) + ":\n    .long 5\n.data\n.globl " +
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
         std::string(true_root_name) + ", @object\n.p2align 2\n" + std::string(true_root_name) +
         ":\n    .quad " + std::string(true_global_name) + "\n.data\n.globl " +
         std::string(true_global_name) + "\n.type " + std::string(true_global_name) +
         ", @object\n.p2align 2\n" + std::string(true_global_name) + ":\n    .long 5\n.data\n.globl " +
         std::string(false_root_name) + "\n.type " + std::string(false_root_name) +
         ", @object\n.p2align 2\n" + std::string(false_root_name) + ":\n    .quad " +
         std::string(false_global_name) + "\n.data\n.globl " + std::string(false_global_name) +
         "\n.type " + std::string(false_global_name) + ", @object\n.p2align 2\n" +
         std::string(false_global_name) + ":\n    .long 1\n";
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
         ":\n    .long 0\n    .long 5\n.data\n.globl " + std::string(false_global_name) +
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
         std::string(true_global_name) + ":\n    .long 0\n    .long 5\n.data\n.globl " +
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

std::string expected_minimal_param_ne_zero_branch_asm(const char* function_name,
                                                      const char* false_label,
                                                      int true_returned_value,
                                                      int false_returned_value) {
  return expected_zero_branch_prefix(function_name, false_label, "je") + "    mov " +
         minimal_i32_return_register() + ", " + std::to_string(true_returned_value) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " +
         std::to_string(false_returned_value) + "\n    ret\n";
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
      .result = bir::Value::named(bir::TypeKind::I32, "widened"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(12),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "widened"),
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

bir::Module make_x86_param_eq_zero_branch_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_on_zero";
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
  is_zero.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(7),
  };

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(11),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_eq_zero_branch_param_or_immediate_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_zero_or_passthrough";
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
  is_zero.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(5),
  };

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "p.x"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_zero));
  function.blocks.push_back(std::move(is_nonzero));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_param_ne_zero_branch_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_on_nonzero";
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
  is_nonzero.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(11),
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(7),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(is_nonzero));
  function.blocks.push_back(std::move(is_zero));
  module.functions.push_back(std::move(function));
  return module;
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

bir::Module make_x86_local_i32_immediate_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.x",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.x",
      .value = bir::Value::immediate_i32(123),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .slot_name = "%lv.x",
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .rhs = bir::Value::immediate_i32(123),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_local_i32_add_chain_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  for (int index = 0; index < 3; ++index) {
    function.local_slots.push_back(bir::LocalSlot{
        .name = "%lv.v." + std::to_string(index * 4),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = true,
    });
  }

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.v.0",
      .value = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.v.4",
      .value = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.v.8",
      .value = bir::Value::immediate_i32(3),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .slot_name = "%lv.v.0",
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t9"),
      .slot_name = "%lv.v.4",
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%t10"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "%t9"),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t13"),
      .slot_name = "%lv.v.8",
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%t14"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t10"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "%t13"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t15"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t14"),
      .rhs = bir::Value::immediate_i32(6),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t15"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_local_i32_sub_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.s1.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.s2.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.s1.0",
      .value = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.s2.0",
      .value = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .slot_name = "%lv.s1.0",
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t5"),
      .slot_name = "%lv.s2.0",
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "%t5"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t7"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t7"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_local_i8_address_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  for (int index = 0; index < 8; ++index) {
    function.local_slots.push_back(bir::LocalSlot{
        .name = "%lv.arr." + std::to_string(index),
        .type = bir::TypeKind::I8,
        .size_bytes = 1,
        .align_bytes = 1,
        .is_address_taken = true,
    });
  }

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.arr.7",
      .value = bir::Value::immediate_i8(2),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "loaded.byte"),
      .slot_name = "%lv.arr.7",
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "extended.byte"),
      .operand = bir::Value::named(bir::TypeKind::I8, "loaded.byte"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "entry.cmp"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "extended.byte"),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "entry.cmp"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "addressed.byte"),
      .slot_name = "%t.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.arr.0",
          .byte_offset = 7,
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  block_2.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "addressed.extended"),
      .operand = bir::Value::named(bir::TypeKind::I8, "addressed.byte"),
  });
  block_2.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "addressed.cmp"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "addressed.extended"),
      .rhs = bir::Value::immediate_i32(2),
  });
  block_2.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "addressed.cmp"),
      .true_label = "block_3",
      .false_label = "block_4",
  };

  bir::Block block_3;
  block_3.label = "block_3";
  block_3.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_4;
  block_4.label = "block_4";
  block_4.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_local_i32_short_circuit_or_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.u.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.addr",
      .value = bir::Value::immediate_i32(1),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t1.addr",
      .value = bir::Value::immediate_i32(3),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .slot_name = "%t3.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .true_label = "logic.skip.8",
      .false_label = "logic.rhs.7",
  };

  bir::Block logic_rhs;
  logic_rhs.label = "logic.rhs.7";
  logic_rhs.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t12"),
      .slot_name = "%t12.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  logic_rhs.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t13"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t12"),
      .rhs = bir::Value::immediate_i32(3),
  });
  logic_rhs.terminator = bir::BranchTerminator{.target_label = "logic.rhs.end.9"};

  bir::Block logic_rhs_end;
  logic_rhs_end.label = "logic.rhs.end.9";
  logic_rhs_end.terminator = bir::BranchTerminator{.target_label = "logic.end.10"};

  bir::Block logic_skip;
  logic_skip.label = "logic.skip.8";
  logic_skip.terminator = bir::BranchTerminator{.target_label = "logic.end.10"};

  bir::Block logic_end;
  logic_end.label = "logic.end.10";
  logic_end.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t17"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .rhs = bir::Value::immediate_i32(3),
      .true_value = bir::Value::immediate_i32(1),
      .false_value = bir::Value::named(bir::TypeKind::I32, "%t13"),
  });
  logic_end.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t18"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t17"),
      .rhs = bir::Value::immediate_i32(0),
  });
  logic_end.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t18"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(logic_rhs));
  function.blocks.push_back(std::move(logic_rhs_end));
  function.blocks.push_back(std::move(logic_skip));
  function.blocks.push_back(std::move(logic_end));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_loop_countdown_join_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "counter"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(3),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "next"),
          },
      },
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp0"),
      .true_label = "body",
      .false_label = "exit",
  };

  bir::Block body;
  body.label = "body";
  body.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "next"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(1),
  });
  body.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "counter"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(loop));
  function.blocks.push_back(std::move(body));
  function.blocks.push_back(std::move(exit));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_local_i16_increment_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.x",
      .type = bir::TypeKind::I16,
      .size_bytes = 2,
      .align_bytes = 2,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.x",
      .value = bir::Value::immediate_i16(0),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I16, "%t1"),
      .slot_name = "%lv.x",
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "%t2"),
      .operand = bir::Value::named(bir::TypeKind::I16, "%t1"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t2"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Trunc,
      .result = bir::Value::named(bir::TypeKind::I16, "%t4"),
      .operand = bir::Value::named(bir::TypeKind::I32, "%t3"),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.x",
      .value = bir::Value::named(bir::TypeKind::I16, "%t4"),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I16, "%t5"),
      .slot_name = "%lv.x",
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .operand = bir::Value::named(bir::TypeKind::I16, "%t5"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t7"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t7"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_local_i16_i64_sub_return_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.s",
      .type = bir::TypeKind::I16,
      .size_bytes = 2,
      .align_bytes = 2,
      .is_address_taken = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.l",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.s",
      .value = bir::Value::immediate_i16(1),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.l",
      .value = bir::Value::immediate_i64(1),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%t0"),
      .slot_name = "%lv.l",
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I16, "%t1"),
      .slot_name = "%lv.s",
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I64, "%t2"),
      .operand = bir::Value::named(bir::TypeKind::I16, "%t1"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I64, "%t3"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "%t2"),
      .rhs = bir::Value::named(bir::TypeKind::I64, "%t0"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Trunc,
      .result = bir::Value::named(bir::TypeKind::I16, "%t4"),
      .operand = bir::Value::named(bir::TypeKind::I64, "%t3"),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.s",
      .value = bir::Value::named(bir::TypeKind::I16, "%t4"),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I16, "%t5"),
      .slot_name = "%lv.s",
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .operand = bir::Value::named(bir::TypeKind::I16, "%t5"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "%t6"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_immediate_i32_guard_chain_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.entry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(0),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.entry"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.second"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
  });
  block_2.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.second"),
      .true_label = "block_3",
      .false_label = "block_4",
  };

  bir::Block block_3;
  block_3.label = "block_3";
  block_3.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(2)};

  bir::Block block_4;
  block_4.label = "block_4";
  block_4.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_same_module_global_i32_guard_chain_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "s",
      .type = bir::TypeKind::I32,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer_elements = {bir::Value::immediate_i32(1), bir::Value::immediate_i32(2)},
  });

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.first"),
      .global_name = "s",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.first"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded.first"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.first"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.second"),
      .global_name = "s",
      .byte_offset = 4,
      .align_bytes = 4,
  });
  block_2.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.second"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded.second"),
      .rhs = bir::Value::immediate_i32(2),
  });
  block_2.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.second"),
      .true_label = "block_3",
      .false_label = "block_4",
  };

  bir::Block block_3;
  block_3.label = "block_3";
  block_3.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(2)};

  bir::Block block_4;
  block_4.label = "block_4";
  block_4.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_pointer_backed_same_module_global_guard_chain_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "s",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("s_backing"),
  });
  module.globals.push_back(bir::Global{
      .name = "s_backing",
      .type = bir::TypeKind::I8,
      .size_bytes = 24,
      .align_bytes = 8,
      .initializer_elements = {bir::Value::immediate_i32(1),
                               bir::Value::immediate_i32(2),
                               bir::Value::named(bir::TypeKind::Ptr, "@gs1"),
                               bir::Value::immediate_i32(1),
                               bir::Value::immediate_i32(2)},
  });
  module.globals.push_back(bir::Global{
      .name = "gs1",
      .type = bir::TypeKind::I8,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer_elements = {bir::Value::immediate_i32(1), bir::Value::immediate_i32(2)},
  });

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "loaded.root"),
      .global_name = "s",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.first"),
      .global_name = "s_backing",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.first"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded.first"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.first"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.second"),
      .global_name = "s_backing",
      .byte_offset = 4,
      .align_bytes = 4,
  });
  block_2.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.second"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded.second"),
      .rhs = bir::Value::immediate_i32(2),
  });
  block_2.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.second"),
      .true_label = "block_3",
      .false_label = "block_4",
  };

  bir::Block block_3;
  block_3.label = "block_3";
  block_3.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(2)};

  bir::Block block_4;
  block_4.label = "block_4";
  block_4.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "loaded.ptr"),
      .global_name = "s_backing",
      .byte_offset = 8,
      .align_bytes = 8,
  });
  block_4.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.third"),
      .global_name = "gs1",
      .align_bytes = 4,
  });
  block_4.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.third"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded.third"),
      .rhs = bir::Value::immediate_i32(1),
  });
  block_4.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.third"),
      .true_label = "block_5",
      .false_label = "block_6",
  };

  bir::Block block_5;
  block_5.label = "block_5";
  block_5.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(3)};

  bir::Block block_6;
  block_6.label = "block_6";
  block_6.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.fourth"),
      .global_name = "gs1",
      .byte_offset = 4,
      .align_bytes = 4,
  });
  block_6.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.fourth"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded.fourth"),
      .rhs = bir::Value::immediate_i32(2),
  });
  block_6.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.fourth"),
      .true_label = "block_7",
      .false_label = "block_8",
  };

  bir::Block block_7;
  block_7.label = "block_7";
  block_7.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(4)};

  bir::Block block_8;
  block_8.label = "block_8";
  block_8.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.fifth"),
      .global_name = "s_backing",
      .byte_offset = 16,
      .align_bytes = 4,
  });
  block_8.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.fifth"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded.fifth"),
      .rhs = bir::Value::immediate_i32(1),
  });
  block_8.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.fifth"),
      .true_label = "block_9",
      .false_label = "block_10",
  };

  bir::Block block_9;
  block_9.label = "block_9";
  block_9.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(5)};

  bir::Block block_10;
  block_10.label = "block_10";
  block_10.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.sixth"),
      .global_name = "s_backing",
      .byte_offset = 20,
      .align_bytes = 4,
  });
  block_10.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp.sixth"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded.sixth"),
      .rhs = bir::Value::immediate_i32(2),
  });
  block_10.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp.sixth"),
      .true_label = "block_11",
      .false_label = "block_12",
  };

  bir::Block block_11;
  block_11.label = "block_11";
  block_11.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(6)};

  bir::Block block_12;
  block_12.label = "block_12";
  block_12.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  function.blocks.push_back(std::move(block_5));
  function.blocks.push_back(std::move(block_6));
  function.blocks.push_back(std::move(block_7));
  function.blocks.push_back(std::move(block_8));
  function.blocks.push_back(std::move(block_9));
  function.blocks.push_back(std::move(block_10));
  function.blocks.push_back(std::move(block_11));
  function.blocks.push_back(std::move(block_12));
  module.functions.push_back(std::move(function));
  return module;
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

bir::Module make_x86_multi_defined_global_function_pointer_boundary_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%d\n",
  });
  module.globals.push_back(bir::Global{
      .name = "f",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("fred"),
  });
  module.globals.push_back(bir::Global{
      .name = "printfptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("printf"),
  });

  bir::Function fred;
  fred.name = "fred";
  fred.return_type = bir::TypeKind::I32;
  fred.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "%p0",
  });
  fred.blocks.push_back(bir::Block{
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

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%fred.ptr"),
      .global_name = "f",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%fred.ptr"),
      .args = {bir::Value::immediate_i32(24)},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%printf.ptr"),
      .global_name = "printfptr",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t2"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%printf.ptr"),
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t1")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(fred));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

lir::LirModule make_x86_return_constant_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};

  lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = lir::LirRet{
      .value_str = std::string("7"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_param_add_immediate_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "add_one";
  function.signature_text = "define i32 @add_one(i32 %x)";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.params.emplace_back("%x", c4c::TypeSpec{.base = c4c::TB_INT});

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirBinOp{
      .result = lir::LirOperand("%sum"),
      .opcode = "add",
      .type_str = "i32",
      .lhs = lir::LirOperand("%x"),
      .rhs = lir::LirOperand("1"),
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%sum"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_local_pointer_chain_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.x"),
      .type_str = "i32",
      .count = lir::LirOperand(""),
      .align = 4,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.p"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.pp"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "i32",
      .val = lir::LirOperand("0"),
      .ptr = lir::LirOperand("%lv.x"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%lv.x"),
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%lv.p"),
      .ptr = lir::LirOperand("%lv.pp"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t0"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.pp"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t1"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%t0"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t2"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%t1"),
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%t2"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_local_i32_byte_storage_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.u"),
      .type_str = "{ [4 x i8] }",
      .count = lir::LirOperand(""),
      .align = 4,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t0"),
      .element_type = "{ [4 x i8] }",
      .ptr = lir::LirOperand("%lv.u"),
      .indices = {"i32 0", "i32 0"},
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "i32",
      .val = lir::LirOperand("3"),
      .ptr = lir::LirOperand("%t0"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t1"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%t0"),
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%t1"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_loaded_local_array_pointer_guard_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.a"),
      .type_str = "[2 x [4 x i8]]",
      .count = lir::LirOperand(""),
      .align = 1,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.p"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t0"),
      .element_type = "[2 x [4 x i8]]",
      .ptr = lir::LirOperand("%lv.a"),
      .indices = {lir::LirOperand("i64 0"), lir::LirOperand("i64 0")},
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%t0"),
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t1"),
      .element_type = "[2 x [4 x i8]]",
      .ptr = lir::LirOperand("%lv.a"),
      .indices = {lir::LirOperand("i64 0"), lir::LirOperand("i64 0")},
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t2"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i32",
      .operand = lir::LirOperand("1"),
      .to_type = "i64",
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t3"),
      .element_type = "[4 x i8]",
      .ptr = lir::LirOperand("%t1"),
      .indices = {lir::LirOperand("i64 %t2")},
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t4"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i32",
      .operand = lir::LirOperand("3"),
      .to_type = "i64",
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t5"),
      .element_type = "i8",
      .ptr = lir::LirOperand("%t3"),
      .indices = {lir::LirOperand("i64 %t4")},
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "i8",
      .val = lir::LirOperand("2"),
      .ptr = lir::LirOperand("%t5"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t7"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t8"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i32",
      .operand = lir::LirOperand("1"),
      .to_type = "i64",
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t9"),
      .element_type = "[4 x i8]",
      .ptr = lir::LirOperand("%t7"),
      .indices = {lir::LirOperand("i64 %t8")},
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t10"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i32",
      .operand = lir::LirOperand("3"),
      .to_type = "i64",
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t11"),
      .element_type = "i8",
      .ptr = lir::LirOperand("%t9"),
      .indices = {lir::LirOperand("i64 %t10")},
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t12"),
      .type_str = "i8",
      .ptr = lir::LirOperand("%t11"),
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t13"),
      .kind = lir::LirCastKind::SExt,
      .from_type = "i8",
      .operand = lir::LirOperand("%t12"),
      .to_type = "i32",
  });
  entry.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t14"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t13"),
      .rhs = lir::LirOperand("2"),
  });
  entry.insts.push_back(lir::LirCastOp{
      .result = lir::LirOperand("%t15"),
      .kind = lir::LirCastKind::ZExt,
      .from_type = "i1",
      .operand = lir::LirOperand("%t14"),
      .to_type = "i32",
  });
  entry.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t16"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t15"),
      .rhs = lir::LirOperand("0"),
  });
  entry.terminator = lir::LirCondBr{
      .cond_name = "%t16",
      .true_label = "block_1",
      .false_label = "block_2",
  };

  lir::LirBlock block_1;
  block_1.label = "block_1";
  block_1.terminator = lir::LirRet{
      .value_str = std::string("1"),
      .type_str = "i32",
  };

  lir::LirBlock block_2;
  block_2.label = "block_2";
  block_2.terminator = lir::LirRet{
      .value_str = std::string("0"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_x86_local_pointer_guard_chain_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";
  function.return_type = c4c::TypeSpec{.base = c4c::TB_INT};
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.x"),
      .type_str = "i32",
      .count = lir::LirOperand(""),
      .align = 4,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.p"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.pp"),
      .type_str = "ptr",
      .count = lir::LirOperand(""),
      .align = 8,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "i32",
      .val = lir::LirOperand("0"),
      .ptr = lir::LirOperand("%lv.x"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%lv.x"),
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "ptr",
      .val = lir::LirOperand("%lv.p"),
      .ptr = lir::LirOperand("%lv.pp"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t0"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.p"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t1"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%t0"),
  });
  entry.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t2"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t1"),
      .rhs = lir::LirOperand("0"),
  });
  entry.terminator = lir::LirCondBr{
      .cond_name = "%t2",
      .true_label = "block_1",
      .false_label = "block_2",
  };

  lir::LirBlock block_1;
  block_1.label = "block_1";
  block_1.terminator = lir::LirRet{
      .value_str = std::string("1"),
      .type_str = "i32",
  };

  lir::LirBlock block_2;
  block_2.label = "block_2";
  block_2.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t3"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.pp"),
  });
  block_2.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t4"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%t3"),
  });
  block_2.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t5"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%t4"),
  });
  block_2.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t6"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t5"),
      .rhs = lir::LirOperand("0"),
  });
  block_2.terminator = lir::LirCondBr{
      .cond_name = "%t6",
      .true_label = "block_3",
      .false_label = "block_4",
  };

  lir::LirBlock block_3;
  block_3.label = "block_3";
  block_3.terminator = lir::LirRet{
      .value_str = std::string("1"),
      .type_str = "i32",
  };

  lir::LirBlock block_4;
  block_4.label = "block_4";
  block_4.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t7"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%lv.pp"),
  });
  block_4.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t8"),
      .type_str = "ptr",
      .ptr = lir::LirOperand("%t7"),
  });
  block_4.insts.push_back(lir::LirStoreOp{
      .type_str = "i32",
      .val = lir::LirOperand("1"),
      .ptr = lir::LirOperand("%t8"),
  });
  block_4.terminator = lir::LirBr{
      .target_label = "block_5",
  };

  lir::LirBlock block_5;
  block_5.label = "block_5";
  block_5.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%t9"),
      .type_str = "i32",
      .ptr = lir::LirOperand("%lv.x"),
  });
  block_5.insts.push_back(lir::LirCmpOp{
      .result = lir::LirOperand("%t10"),
      .is_float = false,
      .predicate = "ne",
      .type_str = "i32",
      .lhs = lir::LirOperand("%t9"),
      .rhs = lir::LirOperand("0"),
  });
  block_5.terminator = lir::LirCondBr{
      .cond_name = "%t10",
      .true_label = "block_6",
      .false_label = "block_7",
  };

  lir::LirBlock block_6;
  block_6.label = "block_6";
  block_6.terminator = lir::LirRet{
      .value_str = std::string("0"),
      .type_str = "i32",
  };

  lir::LirBlock block_7;
  block_7.label = "block_7";
  block_7.terminator = lir::LirRet{
      .value_str = std::string("1"),
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
  function.blocks.push_back(std::move(block_3));
  function.blocks.push_back(std::move(block_4));
  function.blocks.push_back(std::move(block_5));
  function.blocks.push_back(std::move(block_6));
  function.blocks.push_back(std::move(block_7));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_unsupported_x86_inline_asm_lir_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "bad_inline_asm";
  function.signature_text = "define void @bad_inline_asm()";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirInlineAsmOp{
      .result = lir::LirOperand("%t0"),
      .ret_type = "{ i32, i32 }",
      .asm_text = "",
      .constraints = "=r,=r",
      .side_effects = true,
      .args_str = "",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
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

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
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
                                                         bool use_pointer_backed_global_selected_value_roots) {
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
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared joined branch contract published invalid true/false join ownership indices")
                    .c_str());
  }

  const std::string renamed_true_label = "carrier.zero";
  const std::string renamed_false_label = "carrier.nonzero";
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
  branch_condition.true_label = renamed_true_label;
  branch_condition.false_label = renamed_false_label;
  entry_block->terminator.true_label = "carrier.raw.zero";
  entry_block->terminator.false_label = "carrier.raw.nonzero";

  join_transfer.source_true_incoming_label = "contract.true_lane";
  join_transfer.source_false_incoming_label = "contract.false_lane";
  join_transfer.edge_transfers[true_transfer_index].predecessor_label =
      *join_transfer.source_true_incoming_label;
  join_transfer.edge_transfers[false_transfer_index].predecessor_label =
      *join_transfer.source_false_incoming_label;
  join_transfer.incomings.push_back(
      bir::PhiIncoming{.label = "contract.extra_lane", .value = bir::Value::immediate_i32(77)});
  join_transfer.edge_transfers.push_back(prepare::PreparedEdgeValueTransfer{
      .predecessor_label = "contract.extra_lane",
      .successor_label = join_transfer.join_block_label,
      .incoming_value = bir::Value::immediate_i32(77),
      .destination_value = join_transfer.result,
      .storage_name = std::nullopt,
  });
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == *join_transfer.source_true_incoming_label ||
        incoming.label == renamed_true_label) {
      incoming.label = *join_transfer.source_true_incoming_label;
    } else if (incoming.label == *join_transfer.source_false_incoming_label ||
               incoming.label == renamed_false_label) {
      incoming.label = *join_transfer.source_false_incoming_label;
    }
  }

  entry_compare->opcode = bir::BinaryOpcode::Ne;
  entry_compare->lhs = bir::Value::immediate_i32(9);
  entry_compare->rhs = bir::Value::immediate_i32(3);
  const std::string original_carrier_result_name = join_select->result.name;
  const auto renamed_carrier_result =
      bir::Value::named(bir::TypeKind::I32, "carrier.join.contract.result");
  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = "%contract.join.slot";
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = *join_transfer.storage_name,
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    join_block->insts[join_select_index] = bir::LoadLocalInst{
        .result = renamed_carrier_result,
        .slot_name = *join_transfer.storage_name,
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
      if (incoming.label == *join_transfer.source_true_incoming_label) {
        incoming.value = true_chain_selected;
      } else if (incoming.label == *join_transfer.source_false_incoming_label) {
        incoming.value = false_chain_selected;
      }
    }
  } else if (use_global_selected_value_roots) {
    const char* true_global_name =
        use_pointer_backed_global_selected_value_roots ? "selected_zero_backing"
        : use_fixed_offset_global_selected_value_roots ? "selected_zero_pair"
                                                       : "selected_zero";
    const char* false_global_name =
        use_pointer_backed_global_selected_value_roots ? "selected_nonzero_backing"
        : use_fixed_offset_global_selected_value_roots ? "selected_nonzero_pair"
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
      if (incoming.label == *join_transfer.source_true_incoming_label) {
        incoming.value = join_transfer.edge_transfers[true_transfer_index].incoming_value;
      } else if (incoming.label == *join_transfer.source_false_incoming_label) {
        incoming.value = join_transfer.edge_transfers[false_transfer_index].incoming_value;
      }
    }
  }

  mutable_control_flow->branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = function_name,
      .block_label = "dead.cond",
      .kind = prepare::PreparedBranchConditionKind::MaterializedBool,
      .condition_value = bir::Value::named(bir::TypeKind::I32, "dead.cond"),
      .true_label = "dead.true",
      .false_label = "dead.false",
  });
  mutable_control_flow->join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = function_name,
      .join_block_label = "dead.join",
      .result = bir::Value::named(bir::TypeKind::I32, "dead.result"),
      .kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot,
  });

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

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
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
      module, expected_asm, function_name, failure_context, false, false, false, false, false, false, false);
}

int check_join_route_edge_store_slot_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true, false, false, false, false, false, false);
}

int check_join_route_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true, true, false, false, false, false, false);
}

int check_join_route_immediate_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true, true, true, false, false, false, false);
}

int check_join_route_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, false, false, false, true, false, false, false);
}

int check_join_route_edge_store_slot_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true, false, false, true, false, false, false);
}

int check_join_route_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, false, false, false, true, true, false, false);
}

int check_join_route_edge_store_slot_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true, false, false, true, true, false, false);
}

int check_join_route_fixed_offset_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, false, false, false, true, false, true, false);
}

int check_join_route_edge_store_slot_fixed_offset_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true, false, false, true, false, true, false);
}

int check_join_route_fixed_offset_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, false, false, false, true, true, true, false);
}

int check_join_route_edge_store_slot_fixed_offset_global_selected_value_chain_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true, false, false, true, true, true, false);
}

int check_join_route_pointer_backed_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, false, false, false, true, false, false, true);
}

int check_join_route_edge_store_slot_pointer_backed_global_selected_values_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_join_route_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true, false, false, true, false, false, true);
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
      *control_flow, function, *entry_block, function.params.front(), true);
  if (!compare_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the materialized compare-join context")
                    .c_str());
  }

  const auto prepared_join_branches =
      prepare::find_prepared_materialized_compare_join_branches(
          compare_join_context->compare_join_context);
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
        return_context.selected_value.base.param_name != "p.x") {
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

int check_materialized_compare_join_branches_publish_prepared_immediate_return_contexts_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier,
    bool use_selected_value_chain) {
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
    join_transfer.storage_name = "%contract.immediate.join.slot";
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = *join_transfer.storage_name,
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
        .slot_name = *join_transfer.storage_name,
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
      if (incoming.label == join_transfer.edge_transfers[true_transfer_index].predecessor_label) {
        incoming.value = true_chain_selected;
      } else if (incoming.label ==
                 join_transfer.edge_transfers[false_transfer_index].predecessor_label) {
        incoming.value = false_chain_selected;
      }
    }
  }

  const auto compare_join_context = prepare::find_prepared_param_zero_materialized_compare_join_context(
      *control_flow, function, *entry_block, function.params.front(), true);
  if (!compare_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the materialized compare-join context")
                    .c_str());
  }

  const auto prepared_join_branches =
      prepare::find_prepared_materialized_compare_join_branches(
          compare_join_context->compare_join_context);
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
  return require_prepared_immediate_base(
      prepared_join_branches->false_return_context,
      use_selected_value_chain ? 7 : 1,
      bir::BinaryOpcode::Sub,
      6);
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

int check_materialized_compare_join_branches_publish_prepared_global_return_contexts_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier,
    bool use_selected_value_chain,
    bool use_fixed_offset_global_roots,
    bool use_pointer_backed_global_roots) {
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
    join_transfer.storage_name = "%contract.global.join.slot";
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = *join_transfer.storage_name,
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
        .slot_name = *join_transfer.storage_name,
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
      use_pointer_backed_global_roots ? "selected_zero_backing"
      : use_fixed_offset_global_roots ? "selected_zero_pair"
                                      : "selected_zero";
  const char* false_global_name =
      use_pointer_backed_global_roots ? "selected_nonzero_backing"
      : use_fixed_offset_global_roots ? "selected_nonzero_pair"
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
    if (incoming.label == join_transfer.edge_transfers[true_transfer_index].predecessor_label) {
      incoming.value = join_transfer.edge_transfers[true_transfer_index].incoming_value;
    } else if (incoming.label ==
               join_transfer.edge_transfers[false_transfer_index].predecessor_label) {
      incoming.value = join_transfer.edge_transfers[false_transfer_index].incoming_value;
    }
  }

  const auto compare_join_context = prepare::find_prepared_param_zero_materialized_compare_join_context(
      *control_flow, function, *entry_block, function.params.front(), true);
  if (!compare_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the materialized compare-join context")
                    .c_str());
  }

  const auto prepared_join_branches =
      prepare::find_prepared_materialized_compare_join_branches(
          compare_join_context->compare_join_context);
  if (!prepared_join_branches.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the compare-join branch contract")
                    .c_str());
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
        return_context.selected_value.base.global_name != expected_global_name ||
        return_context.selected_value.base.global_byte_offset != expected_global_byte_offset) {
      return fail((std::string(failure_context) +
                   ": shared helper stopped classifying the selected-value base as a same-module global load")
                      .c_str());
    }
    if (use_pointer_backed_global_roots) {
      const char* expected_root_name =
          expected_global_name == true_global_name ? "selected_zero_root" : "selected_nonzero_root";
      if (return_context.selected_value.base.pointer_root_global_name != expected_root_name) {
        return fail((std::string(failure_context) +
                     ": shared helper stopped publishing the pointer-backed same-module global root")
                        .c_str());
      }
    }
    if (use_selected_value_chain) {
      if (return_context.selected_value.base.global_name == true_global_name) {
        if (return_context.selected_value.operations.size() != 1 ||
            return_context.selected_value.operations.front().opcode != bir::BinaryOpcode::Add ||
            return_context.selected_value.operations.front().immediate != 4) {
          return fail((std::string(failure_context) +
                       ": shared helper stopped publishing the true global-root selected-value chain")
                          .c_str());
        }
      } else if (return_context.selected_value.base.global_name == false_global_name) {
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
  return require_prepared_global_base(
      prepared_join_branches->false_return_context, false_global_name, global_byte_offset);
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

int check_minimal_compare_branch_consumes_prepared_control_flow(const bir::Module& module,
                                                                const std::string& expected_asm,
                                                                const char* function_name,
                                                                const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the minimal compare branch control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || entry_block->insts.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared minimal compare branch fixture no longer has the expected entry shape")
                    .c_str());
  }

  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.front());
  if (entry_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared minimal compare branch fixture no longer exposes the bounded compare carrier")
                    .c_str());
  }

  entry_compare->opcode = bir::BinaryOpcode::Ne;
  entry_compare->lhs = bir::Value::immediate_i32(9);
  entry_compare->rhs = bir::Value::immediate_i32(3);

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared branch metadata")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_route_consumes_prepared_control_flow(const bir::Module& module,
                                                             const char* function_name,
                                                             const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 2 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "logic.end.10");
  if (entry_block == nullptr || join_block == nullptr || entry_block->insts.size() < 4 ||
      join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry/join shape")
                    .c_str());
  }

  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.back());
  auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts.front());
  auto* join_compare = std::get_if<bir::BinaryInst>(&join_block->insts.back());
  if (entry_compare == nullptr || join_select == nullptr || join_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the bounded compare/select carriers")
                    .c_str());
  }
  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.size() != 1 ||
      mutable_control_flow->branch_conditions.size() != 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture lost its mutable join-transfer contract")
                    .c_str());
  }
  auto& join_transfer = mutable_control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : mutable_control_flow->branch_conditions) {
      if (branch_condition.block_label == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (join_branch_condition == nullptr || !join_branch_condition->lhs.has_value() ||
      !join_branch_condition->rhs.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative join compare contract")
                    .c_str());
  }
  if (join_transfer.edge_transfers.size() != 2 ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value() ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer no longer maps compare truth to authoritative join ownership")
                    .c_str());
  }
  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer published invalid true/false ownership indices")
                    .c_str());
  }

  const std::string original_true_incoming_label = *join_transfer.source_true_incoming_label;
  const std::string original_false_incoming_label = *join_transfer.source_false_incoming_label;
  join_transfer.source_true_incoming_label = "contract.short_circuit";
  join_transfer.source_false_incoming_label = "contract.rhs";
  join_transfer.edge_transfers[false_transfer_index].incoming_value = bir::Value::immediate_i32(9);
  bool rewrote_short_circuit_label = false;
  bool rewrote_rhs_label = false;
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == original_true_incoming_label) {
      incoming.label = *join_transfer.source_true_incoming_label;
      rewrote_short_circuit_label = true;
      continue;
    }
    if (incoming.label == original_false_incoming_label) {
      incoming.label = *join_transfer.source_false_incoming_label;
      incoming.value = bir::Value::immediate_i32(9);
      rewrote_rhs_label = true;
    }
  }
  if (!rewrote_short_circuit_label || !rewrote_rhs_label) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer no longer exposes both contract-owned join incoming lanes")
                    .c_str());
  }

  entry_compare->opcode = bir::BinaryOpcode::Eq;
  entry_compare->lhs = bir::Value::immediate_i32(7);
  entry_compare->rhs = bir::Value::immediate_i32(7);
  const std::string original_join_carrier_name = join_select->result.name;
  join_select->result = bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.join");
  if (join_compare->lhs.kind == bir::Value::Kind::Named &&
      join_compare->lhs.name == original_join_carrier_name) {
    join_compare->lhs = join_select->result;
  }
  if (join_compare->rhs.kind == bir::Value::Kind::Named &&
      join_compare->rhs.name == original_join_carrier_name) {
    join_compare->rhs = join_select->result;
  }
  if (join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
      join_branch_condition->lhs->name == join_transfer.result.name) {
    *join_branch_condition->lhs = join_select->result;
  }
  if (join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
      join_branch_condition->rhs->name == join_transfer.result.name) {
    *join_branch_condition->rhs = join_select->result;
  }
  join_select->predicate = bir::BinaryOpcode::Eq;
  join_select->lhs = bir::Value::immediate_i32(2);
  join_select->rhs = bir::Value::immediate_i32(9);
  join_compare->opcode = bir::BinaryOpcode::Eq;
  join_compare->lhs = bir::Value::immediate_i32(5);
  join_compare->rhs = bir::Value::immediate_i32(5);
  entry_block->terminator.true_label = "carrier.short_circuit";
  entry_block->terminator.false_label = "carrier.rhs";
  join_block->terminator.true_label = "carrier.join.true";
  join_block->terminator.false_label = "carrier.join.false";

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_minimal_local_i32_short_circuit_or_guard_asm(function_name)) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative short-circuit metadata over carrier labels")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_edge_store_slot_route_consumes_prepared_control_flow(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 2 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "logic.end.10");
  if (entry_block == nullptr || join_block == nullptr || entry_block->insts.size() < 4 ||
      join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry/join shape")
                    .c_str());
  }

  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.back());
  auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts.front());
  auto* join_compare = std::get_if<bir::BinaryInst>(&join_block->insts.back());
  if (entry_compare == nullptr || join_select == nullptr || join_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the bounded compare/select carriers")
                    .c_str());
  }

  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.size() != 1 ||
      mutable_control_flow->branch_conditions.size() != 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture lost its mutable join-transfer contract")
                    .c_str());
  }

  auto& join_transfer = mutable_control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : mutable_control_flow->branch_conditions) {
      if (branch_condition.block_label == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (join_branch_condition == nullptr || !join_branch_condition->lhs.has_value() ||
      !join_branch_condition->rhs.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative join compare contract")
                    .c_str());
  }
  if (join_transfer.edge_transfers.size() != 2 ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value() ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer no longer maps compare truth to authoritative join ownership")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer published invalid true/false ownership indices")
                    .c_str());
  }

  const std::string original_true_incoming_label = *join_transfer.source_true_incoming_label;
  const std::string original_false_incoming_label = *join_transfer.source_false_incoming_label;
  join_transfer.source_true_incoming_label = "contract.short_circuit";
  join_transfer.source_false_incoming_label = "contract.rhs";
  join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
  join_transfer.storage_name = "%contract.short_circuit.slot";
  join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
  join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
  join_transfer.edge_transfers[false_transfer_index].incoming_value = bir::Value::immediate_i32(9);
  bool rewrote_short_circuit_label = false;
  bool rewrote_rhs_label = false;
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == original_true_incoming_label) {
      incoming.label = *join_transfer.source_true_incoming_label;
      rewrote_short_circuit_label = true;
      continue;
    }
    if (incoming.label == original_false_incoming_label) {
      incoming.label = *join_transfer.source_false_incoming_label;
      incoming.value = bir::Value::immediate_i32(9);
      rewrote_rhs_label = true;
    }
  }
  if (!rewrote_short_circuit_label || !rewrote_rhs_label) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer no longer exposes both contract-owned join incoming lanes")
                    .c_str());
  }

  function.local_slots.push_back(bir::LocalSlot{
      .name = *join_transfer.storage_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = false,
  });
  const auto load_result = bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.join");
  join_block->insts.front() = bir::LoadLocalInst{
      .result = load_result,
      .slot_name = *join_transfer.storage_name,
  };
  if (join_compare->lhs.kind == bir::Value::Kind::Named &&
      join_compare->lhs.name == join_transfer.result.name) {
    join_compare->lhs = load_result;
  }
  if (join_compare->rhs.kind == bir::Value::Kind::Named &&
      join_compare->rhs.name == join_transfer.result.name) {
    join_compare->rhs = load_result;
  }
  if (join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
      join_branch_condition->lhs->name == join_transfer.result.name) {
    *join_branch_condition->lhs = load_result;
  }
  if (join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
      join_branch_condition->rhs->name == join_transfer.result.name) {
    *join_branch_condition->rhs = load_result;
  }

  entry_compare->opcode = bir::BinaryOpcode::Eq;
  entry_compare->lhs = bir::Value::immediate_i32(7);
  entry_compare->rhs = bir::Value::immediate_i32(7);
  join_compare->opcode = bir::BinaryOpcode::Eq;
  join_compare->lhs = bir::Value::immediate_i32(5);
  join_compare->rhs = bir::Value::immediate_i32(5);
  entry_block->terminator.true_label = "carrier.short_circuit";
  entry_block->terminator.false_label = "carrier.rhs";
  join_block->terminator.true_label = "carrier.join.true";
  join_block->terminator.false_label = "carrier.join.false";

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_minimal_local_i32_short_circuit_or_guard_asm(function_name)) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped validating the authoritative short-circuit EdgeStoreSlot carrier")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_consumes_prepared_control_flow(const bir::Module& module,
                                                              const std::string& expected_asm,
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
                 ": prepare no longer publishes the loop countdown control-flow contract")
                    .c_str());
  }
  if (control_flow->join_transfers.front().kind != prepare::PreparedJoinTransferKind::LoopCarry) {
    return fail((std::string(failure_context) +
                 ": prepare no longer classifies the loop countdown join as explicit loop-carry traffic")
                    .c_str());
  }
  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->branch_conditions.size() != 1 ||
      mutable_control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture lost its mutable loop-countdown control-flow contract")
                    .c_str());
  }
  mutable_control_flow->branch_conditions.insert(
      mutable_control_flow->branch_conditions.begin(),
      prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = "carrier.loop",
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = bir::Value::named(bir::TypeKind::I32, "carrier.cond"),
          .predicate = bir::BinaryOpcode::Eq,
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "carrier.counter"),
          .rhs = bir::Value::immediate_i32(0),
          .can_fuse_with_branch = true,
          .true_label = "carrier.body",
          .false_label = "carrier.exit",
      });
  mutable_control_flow->join_transfers.insert(
      mutable_control_flow->join_transfers.begin(),
      prepare::PreparedJoinTransfer{
          .function_name = function_name,
          .join_block_label = "carrier.loop",
          .result = bir::Value::named(bir::TypeKind::I32, "carrier.counter"),
          .kind = prepare::PreparedJoinTransferKind::LoopCarry,
          .storage_name = "%carrier.counter",
          .edge_transfers =
              {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = "carrier.entry",
                      .successor_label = "carrier.loop",
                      .incoming_value = bir::Value::immediate_i32(9),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "carrier.counter"),
                      .storage_name = "%carrier.counter",
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = "carrier.body",
                      .successor_label = "carrier.loop",
                      .incoming_value = bir::Value::named(bir::TypeKind::I32, "carrier.next"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "carrier.counter"),
                      .storage_name = "%carrier.counter",
                  },
              },
      });

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* loop_block = find_block(function, "loop");
  auto* body_block = find_block(function, "body");
  if (entry_block == nullptr || loop_block == nullptr || body_block == nullptr ||
      entry_block->insts.empty() || loop_block->insts.size() < 2 || body_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer has the expected entry/header/body shape")
                    .c_str());
  }

  auto* entry_store = std::get_if<bir::StoreLocalInst>(&entry_block->insts.front());
  auto* loop_compare = std::get_if<bir::BinaryInst>(&loop_block->insts.back());
  auto* body_store = std::get_if<bir::StoreLocalInst>(&body_block->insts.back());
  if (entry_store == nullptr || loop_compare == nullptr || body_store == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the bounded slot/compare carriers")
                    .c_str());
  }

  entry_store->value = bir::Value::immediate_i32(99);
  loop_compare->opcode = bir::BinaryOpcode::Eq;
  loop_compare->lhs = bir::Value::immediate_i32(7);
  loop_compare->rhs = bir::Value::immediate_i32(7);
  body_store->value = bir::Value::immediate_i32(44);

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative loop control-flow contract over unrelated prepared records")
                    .c_str());
  }

  return 0;
}

int check_route_rejection(const bir::Module& module,
                          std::string_view expected_message_fragment,
                          const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted an out-of-scope route")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the out-of-scope route with the wrong contract message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_target_bir_module(module, target_profile);
    return fail((std::string(failure_context) +
                 ": public x86 BIR entry unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": public x86 BIR entry rejected the out-of-scope route with the wrong contract message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_module(
        BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
    return fail((std::string(failure_context) +
                 ": generic x86 backend emit path unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": generic x86 backend emit path rejected the out-of-scope route with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_lir_route_outputs(const lir::LirModule& module,
                            const std::string& /*expected_asm*/,
                            const char* failure_context) {
  c4c::backend::BirLoweringOptions lowering_options{};
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (!lowering.module.has_value()) {
    return fail((std::string(failure_context) +
                 ": semantic lir_to_bir no longer produces the canonical prepared-module handoff")
                    .c_str());
  }

  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(*lowering.module, module.target_profile);
  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);

  const auto public_asm =
      c4c::backend::emit_target_lir_module(module, module.target_profile);
  if (public_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": public x86 LIR entry did not route through the canonical x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
  if (generic_asm != public_asm) {
    return fail((std::string(failure_context) +
                 ": generic backend emit path no longer routes x86 LIR input through emit_target_lir_module")
                    .c_str());
  }

  return 0;
}

int check_lir_route_rejection(const lir::LirModule& module,
                              std::string_view expected_message_fragment,
                              const char* failure_context) {
  try {
    (void)c4c::backend::emit_target_lir_module(module, module.target_profile);
    return fail((std::string(failure_context) +
                 ": public x86 LIR entry unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": public x86 LIR entry rejected unsupported lowering with the wrong contract message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_module(
        BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
    return fail((std::string(failure_context) +
                 ": generic x86 backend emit path unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": generic x86 backend emit path rejected unsupported lowering with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

}  // namespace

int main() {
  if (const auto status =
          check_route_outputs(make_x86_return_constant_module(),
                              expected_minimal_constant_return_asm("main", 7),
                              "bir.func @main() -> i32 {",
                              "minimal immediate return route");
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

  if (const auto status =
          check_route_outputs(make_x86_param_add_immediate_module(),
                              expected_minimal_param_add_immediate_asm("add_one", 1),
                              "bir.func @add_one(i32 p.x) -> i32 {",
                              "minimal i32 parameter add-immediate route");
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
          check_route_outputs(make_x86_param_xor_immediate_module(),
                              expected_minimal_param_xor_immediate_asm("flip_masked_bits", 10),
                              "bir.func @flip_masked_bits(i32 p.x) -> i32 {",
                              "minimal i32 parameter xor-immediate route");
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

  // The first scalar-control-flow capability lane is still the bounded
  // compare-against-zero branch family. The current c-testsuite cluster
  // (00051, 00158, 00193, 00213, 00215) stays outside this handoff lane.
  if (const auto status =
          check_route_outputs(make_x86_param_eq_zero_branch_module(),
                              expected_minimal_param_eq_zero_branch_asm("branch_on_zero",
                                                                        "is_nonzero",
                                                                        7,
                                                                        11),
                              "bir.func @branch_on_zero(i32 p.x) -> i32 {",
                              "scalar-control-flow compare-against-zero branch lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_module(),
              expected_minimal_param_eq_zero_branch_asm("branch_on_zero", "is_nonzero", 7, 11),
              "branch_on_zero",
              "scalar-control-flow compare-against-zero branch lane prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_outputs(make_x86_param_ne_zero_branch_module(),
                              expected_minimal_param_ne_zero_branch_asm("branch_on_nonzero",
                                                                        "is_zero",
                                                                        11,
                                                                        7),
                              "bir.func @branch_on_nonzero(i32 p.x) -> i32 {",
                              "scalar-control-flow compare-against-zero nonzero branch lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_consumes_prepared_control_flow(
              make_x86_param_ne_zero_branch_module(),
              expected_minimal_param_ne_zero_branch_asm(
                  "branch_on_nonzero", "is_zero", 11, 7),
              "branch_on_nonzero",
              "scalar-control-flow compare-against-zero nonzero branch lane prepared-control-flow ownership");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_param_eq_zero_branch_param_or_immediate_module(),
              expected_minimal_param_eq_zero_branch_param_or_immediate_asm(
                  "branch_zero_or_passthrough", "is_nonzero", 5),
              "bir.func @branch_zero_or_passthrough(i32 p.x) -> i32 {",
              "scalar-control-flow compare-against-zero branch lane with parameter leaf return");
      status != 0) {
    return status;
  }

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
          check_materialized_compare_join_branches_publish_prepared_return_contexts(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join branch return context ownership");
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
          check_join_route_immediate_selected_value_chain_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_joined_immediates_then_xor_module(),
              expected_minimal_param_eq_zero_branch_joined_immediate_chains_then_xor_asm(
                  "branch_join_immediate_then_xor", "carrier.nonzero", 5, 4, 7, 6, 0, 3),
              "branch_join_immediate_then_xor",
              "scalar-control-flow compare-against-zero joined branch lane with immediate selected-value chain EdgeStoreSlot prepared-control-flow ownership");
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
          check_materialized_compare_join_branches_publish_prepared_global_return_contexts(
              make_x86_param_eq_zero_branch_joined_globals_then_xor_module(),
              "branch_join_global_then_xor",
              "scalar-control-flow compare-against-zero prepared compare-join same-module global return context ownership");
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
          check_route_outputs(make_x86_local_i32_immediate_guard_module(),
                              expected_minimal_local_i32_immediate_guard_asm("main", 123),
                              "bir.store_local %lv.x, i32 123",
                              "minimal local-slot compare-against-immediate guard route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_local_i32_add_chain_guard_module(),
                              expected_minimal_local_i32_add_chain_guard_asm("main"),
                              "%t10 = bir.add i32 %t6, %t9",
                              "minimal local-slot add-chain guard route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_local_i32_sub_guard_module(),
                              expected_minimal_local_i32_sub_guard_asm("main"),
                              "%t6 = bir.sub i32 %t3, %t5",
                              "minimal local-slot sub guard route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_local_i8_address_guard_module(),
                              expected_minimal_local_i8_address_guard_asm("main", 2),
                              "bir.load_local i8 %t.addr, addr %lv.arr.0+7",
                              "minimal local-slot byte addressed-guard route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_local_i32_short_circuit_or_guard_module(),
                              expected_minimal_local_i32_short_circuit_or_guard_asm("main"),
                              "select ne i32 %t3, 3, i32 1, %t13",
                              "minimal local-slot short-circuit or-guard route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_consumes_prepared_control_flow(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit or-guard prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_consumes_prepared_control_flow(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit or-guard EdgeStoreSlot prepared-control-flow ownership");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_loop_countdown_join_module(),
                              expected_minimal_loop_countdown_join_asm("main"),
                              "entry:\n  bir.store_local counter.phi, i32 3\n  bir.br loop\nloop:\n  counter = bir.load_local i32 counter.phi\n  cmp0 = bir.ne i32 counter, 0\n  bir.cond_br i32 cmp0, body, exit",
                              "minimal loop-carried join countdown route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_consumes_prepared_control_flow(
              make_x86_loop_countdown_join_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_local_i16_increment_guard_module(),
                              expected_minimal_local_i16_increment_guard_asm("main"),
                              "%t3 = bir.add i32 %t2, 1",
                              "minimal local-slot i16 increment guard route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_local_i16_i64_sub_return_module(),
                              expected_minimal_local_i16_i64_sub_return_asm("main"),
                              "%t3 = bir.sub i64 %t2, %t0",
                              "minimal local-slot i16/i64 subtract return route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_immediate_i32_guard_chain_module(),
                              expected_minimal_i32_immediate_guard_chain_asm("main"),
                              "cmp.second = bir.ne i32 1, 1",
                              "minimal non-global equality-against-immediate guard-chain route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_same_module_global_i32_guard_chain_module(),
                              expected_minimal_same_module_global_i32_guard_chain_asm("main"),
                              "loaded.second = bir.load_global i32 @s, offset 4",
                              "same-module defined-global equality-against-immediate guard route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(
              make_x86_pointer_backed_same_module_global_guard_chain_module(),
              expected_minimal_pointer_backed_same_module_global_guard_chain_asm("main"),
              "loaded.ptr = bir.load_global ptr @s_backing, offset 8",
              "pointer-backed same-module global equality-against-immediate guard route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_route_outputs(make_x86_multi_defined_direct_call_lane_module(),
                              expected_minimal_multi_defined_direct_call_lane_asm(),
                              "bir.func @main() -> i32 {",
                              "bounded multi-defined-function same-module symbol-call prepared-module route");
      status != 0) {
    return status;
  }

  if (const auto status = check_route_rejection(
          make_x86_multi_defined_global_function_pointer_boundary_module(),
          "one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls",
          "multi-defined global-function-pointer and indirect variadic-runtime boundary");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_outputs(make_x86_return_constant_lir_module(),
                                  expected_minimal_constant_return_asm("main", 7),
                                  "minimal immediate return LIR route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_outputs(make_x86_param_add_immediate_lir_module(),
                                  expected_minimal_param_add_immediate_asm("add_one", 1),
                                  "minimal i32 parameter add-immediate LIR route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_outputs(make_x86_local_pointer_chain_lir_module(),
                                  expected_minimal_local_pointer_chain_asm("main"),
                                  "minimal local-slot pointer-chain LIR route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_outputs(make_x86_local_i32_byte_storage_lir_module(),
                                  expected_minimal_local_i32_byte_storage_asm("main", 3),
                                  "minimal local aggregate byte-storage i32 LIR route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_outputs(make_x86_loaded_local_array_pointer_guard_lir_module(),
                                  expected_minimal_loaded_local_array_pointer_guard_asm("main", 2),
                                  "loaded local array-pointer guard LIR route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_outputs(make_x86_local_pointer_guard_chain_lir_module(),
                                  expected_minimal_local_pointer_guard_chain_asm("main"),
                                  "minimal local-slot compare-against-zero guard-chain LIR route");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_lir_route_rejection(
              make_unsupported_x86_inline_asm_lir_module(),
              "requires semantic lir_to_bir lowering before the canonical prepared-module handoff",
              "unsupported x86 inline-asm LIR route");
      status != 0) {
    return status;
  }

  return 0;
}
