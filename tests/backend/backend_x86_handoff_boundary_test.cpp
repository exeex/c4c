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

bir::Module make_x86_loop_countdown_join_with_preheader_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "preheader"};

  bir::Block preheader;
  preheader.label = "preheader";
  preheader.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "counter"),
      .incomings = {
          bir::PhiIncoming{
              .label = "preheader",
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
  function.blocks.push_back(std::move(preheader));
  function.blocks.push_back(std::move(loop));
  function.blocks.push_back(std::move(body));
  function.blocks.push_back(std::move(exit));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_loop_countdown_join_with_preheader_chain_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "carrier"};

  bir::Block carrier;
  carrier.label = "carrier";
  carrier.terminator = bir::BranchTerminator{.target_label = "preheader"};

  bir::Block preheader;
  preheader.label = "preheader";
  preheader.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "counter"),
      .incomings = {
          bir::PhiIncoming{
              .label = "preheader",
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
  function.blocks.push_back(std::move(carrier));
  function.blocks.push_back(std::move(preheader));
  function.blocks.push_back(std::move(loop));
  function.blocks.push_back(std::move(body));
  function.blocks.push_back(std::move(exit));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_local_i32_countdown_guard_continuation_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.counter",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.counter",
      .value = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::BranchTerminator{.target_label = "loop0"};

  bir::Block loop0;
  loop0.label = "loop0";
  loop0.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loop0.counter"),
      .slot_name = "%lv.counter",
  });
  loop0.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "loop0.cmp"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loop0.counter"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop0.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "loop0.cmp"),
      .true_label = "body0",
      .false_label = "guard",
  };

  bir::Block body0;
  body0.label = "body0";
  body0.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "body0.counter"),
      .slot_name = "%lv.counter",
  });
  body0.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "body0.next"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "body0.counter"),
      .rhs = bir::Value::immediate_i32(1),
  });
  body0.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.counter",
      .value = bir::Value::named(bir::TypeKind::I32, "body0.next"),
  });
  body0.terminator = bir::BranchTerminator{.target_label = "loop0"};

  bir::Block guard;
  guard.label = "guard";
  guard.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "guard.counter"),
      .slot_name = "%lv.counter",
  });
  guard.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "guard.cmp"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "guard.counter"),
      .rhs = bir::Value::immediate_i32(0),
  });
  guard.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "guard.cmp"),
      .true_label = "guard_true",
      .false_label = "cont",
  };

  bir::Block guard_true;
  guard_true.label = "guard_true";
  guard_true.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(1),
  };

  bir::Block cont;
  cont.label = "cont";
  cont.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.counter",
      .value = bir::Value::immediate_i32(2),
  });
  cont.terminator = bir::BranchTerminator{.target_label = "loop1"};

  bir::Block loop1;
  loop1.label = "loop1";
  loop1.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
      .slot_name = "%lv.counter",
  });
  loop1.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "loop1.cmp"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop1.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "loop1.cmp"),
      .true_label = "body1",
      .false_label = "exit",
  };

  bir::Block body1;
  body1.label = "body1";
  body1.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "body1.counter"),
      .slot_name = "%lv.counter",
  });
  body1.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "body1.next"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "body1.counter"),
      .rhs = bir::Value::immediate_i32(1),
  });
  body1.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.counter",
      .value = bir::Value::named(bir::TypeKind::I32, "body1.next"),
  });
  body1.terminator = bir::BranchTerminator{.target_label = "loop1"};

  bir::Block exit;
  exit.label = "exit";
  exit.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "exit.counter"),
      .slot_name = "%lv.counter",
  });
  exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "exit.counter"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(loop0));
  function.blocks.push_back(std::move(body0));
  function.blocks.push_back(std::move(guard));
  function.blocks.push_back(std::move(guard_true));
  function.blocks.push_back(std::move(cont));
  function.blocks.push_back(std::move(loop1));
  function.blocks.push_back(std::move(body1));
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

int check_local_i32_guard_route_consumes_prepared_branch_contract(
    const bir::Module& module,
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
                 ": prepare no longer publishes the local guard prepared branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || entry_block->insts.size() < 3) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer has the expected entry shape")
                    .c_str());
  }
  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.back());
  if (entry_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer exposes the bounded compare carrier")
                    .c_str());
  }

  const std::string original_true_label = entry_block->terminator.true_label;
  const std::string original_false_label = entry_block->terminator.false_label;
  entry_block->terminator.condition.name = "contract.guard.condition";
  entry_block->terminator.true_label = "contract.guard.true";
  entry_block->terminator.false_label = "contract.guard.false";
  entry_compare->opcode = bir::BinaryOpcode::Eq;
  entry_compare->lhs = bir::Value::immediate_i32(7);
  entry_compare->rhs = bir::Value::immediate_i32(9);
  function.blocks.push_back(bir::Block{
      .label = "contract.guard.true",
      .terminator = bir::BranchTerminator{.target_label = original_true_label},
  });
  function.blocks.push_back(bir::Block{
      .label = "contract.guard.false",
      .terminator = bir::BranchTerminator{.target_label = original_false_label},
  });

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared branch contract over local guard carrier state")
                    .c_str());
  }

  return 0;
}

int check_local_i32_guard_route_requires_authoritative_prepared_branch_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local guard prepared branch contract")
                    .c_str());
  }

  auto& branch_condition = control_flow->branch_conditions.front();
  branch_condition.false_label = "drifted.guard.false";

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted drifted prepared guard labels")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected drifted prepared guard labels with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_local_i16_guard_route_requires_authoritative_prepared_branch_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local i16 guard prepared branch contract")
                    .c_str());
  }

  auto& branch_condition = control_flow->branch_conditions.front();
  branch_condition.false_label = "drifted.i16.guard.false";

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted drifted prepared i16 guard labels")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected drifted prepared i16 guard labels with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_i32_guard_chain_route_requires_authoritative_prepared_branch_labels(
    const bir::Module& module,
    const char* function_name,
    const char* branch_block_label,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the guard-chain prepared branch contracts")
                    .c_str());
  }

  auto* branch_condition = static_cast<prepare::PreparedBranchCondition*>(nullptr);
  for (auto& candidate : control_flow->branch_conditions) {
    if (candidate.block_label == branch_block_label) {
      branch_condition = &candidate;
      break;
    }
  }
  if (branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared guard-chain fixture no longer exposes the expected authoritative branch contract")
                    .c_str());
  }

  branch_condition->false_label = "drifted.guard.chain.false";

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted drifted prepared guard-chain labels")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected drifted prepared guard-chain labels with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_local_i32_guard_helper_publishes_prepared_compare_contract(const bir::Module& module,
                                                                     const char* function_name,
                                                                     const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local guard prepared branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || entry_block->insts.size() < 3) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer has the expected entry shape")
                    .c_str());
  }
  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.back());
  if (entry_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer exposes the bounded compare carrier")
                    .c_str());
  }

  entry_compare->opcode = bir::BinaryOpcode::Eq;
  entry_compare->lhs = bir::Value::immediate_i32(7);
  entry_compare->rhs = bir::Value::immediate_i32(9);

  const auto* prepared_branch_condition =
      prepare::find_prepared_i32_immediate_branch_condition(*control_flow, entry_block->label, "loaded");
  if (prepared_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the authoritative local guard compare contract")
                    .c_str());
  }
  if (!prepared_branch_condition->predicate.has_value() ||
      *prepared_branch_condition->predicate != bir::BinaryOpcode::Ne ||
      !prepared_branch_condition->lhs.has_value() || prepared_branch_condition->lhs->name != "loaded" ||
      !prepared_branch_condition->rhs.has_value() ||
      prepared_branch_condition->rhs->kind != bir::Value::Kind::Immediate ||
      prepared_branch_condition->rhs->immediate != 123) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing the authoritative local guard compare semantics")
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

int check_loop_countdown_route_consumes_prepared_control_flow_with_reversed_join_edges(
    const bir::Module& module,
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
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture lost its mutable loop-countdown control-flow contract")
                    .c_str());
  }

  auto& join_transfer = mutable_control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() != 2 ||
      join_transfer.edge_transfers.front().predecessor_label != "entry" ||
      join_transfer.edge_transfers.back().predecessor_label != "body") {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the expected entry/body loop-carry edge ownership")
                    .c_str());
  }

  std::swap(join_transfer.edge_transfers.front(), join_transfer.edge_transfers.back());

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped identifying loop-carry ownership by predecessor labels")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_consumes_prepared_control_flow_with_preheader_block(
    const bir::Module& module,
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

  const auto& join_transfer = control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() != 2 ||
      join_transfer.edge_transfers.front().predecessor_label != "preheader" ||
      join_transfer.edge_transfers.back().predecessor_label != "body") {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the preheader/body loop-carry ownership")
                    .c_str());
  }

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the prepared loop handoff with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative loop join contract when entry reaches the header through a trivial preheader")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_consumes_prepared_control_flow_with_preheader_chain(
    const bir::Module& module,
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

  const auto& join_transfer = control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() != 2 ||
      join_transfer.edge_transfers.front().predecessor_label != "preheader" ||
      join_transfer.edge_transfers.back().predecessor_label != "body") {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the preheader/body loop-carry ownership across the transparent entry carrier")
                    .c_str());
  }

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the prepared loop handoff with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative loop join contract when entry reaches the loop predecessor through a transparent carrier chain")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_prefers_prepared_preheader_handoff_value(
    const bir::Module& module,
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

  const auto& join_transfer = control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() != 2 ||
      join_transfer.edge_transfers.front().predecessor_label != "preheader" ||
      join_transfer.edge_transfers.back().predecessor_label != "body") {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the authoritative preheader/body loop-carry ownership")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* preheader_block = find_block(function, "preheader");
  auto* loop_block = find_block(function, "loop");
  auto* body_block = find_block(function, "body");
  if (preheader_block == nullptr || loop_block == nullptr || body_block == nullptr ||
      preheader_block->insts.empty() || loop_block->insts.size() < 2 || body_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer has the expected preheader/header/body carrier shape")
                    .c_str());
  }

  auto* preheader_store = std::get_if<bir::StoreLocalInst>(&preheader_block->insts.front());
  auto* loop_compare = std::get_if<bir::BinaryInst>(&loop_block->insts.back());
  auto* body_store = std::get_if<bir::StoreLocalInst>(&body_block->insts.back());
  if (preheader_store == nullptr || loop_compare == nullptr || body_store == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the expected preheader/body store and compare carriers")
                    .c_str());
  }

  preheader_store->value = bir::Value::immediate_i32(99);
  loop_compare->opcode = bir::BinaryOpcode::Eq;
  loop_compare->lhs = bir::Value::immediate_i32(7);
  loop_compare->rhs = bir::Value::immediate_i32(7);
  body_store->value = bir::Value::immediate_i32(44);

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped taking the authoritative preheader loop-carry handoff value from prepared metadata")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_requires_supported_entry_handoff_carrier(
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
                 ": prepare no longer publishes the loop countdown control-flow contract")
                    .c_str());
  }
  if (control_flow->join_transfers.front().kind != prepare::PreparedJoinTransferKind::LoopCarry) {
    return fail((std::string(failure_context) +
                 ": prepare no longer classifies the loop countdown join as explicit loop-carry traffic")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* entry_store =
      entry_block == nullptr || entry_block->insts.empty()
          ? nullptr
          : std::get_if<bir::StoreLocalInst>(&entry_block->insts.front());
  if (entry_block == nullptr || entry_store == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the authoritative entry handoff carrier")
                    .c_str());
  }

  entry_store->slot_name = "%entry.counter.drift";

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a drifted entry handoff carrier")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the drifted entry handoff carrier with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_loop_countdown_route_requires_authoritative_transparent_entry_prefix(
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
                 ": prepare no longer publishes the loop countdown control-flow contract")
                    .c_str());
  }
  if (control_flow->join_transfers.front().kind != prepare::PreparedJoinTransferKind::LoopCarry) {
    return fail((std::string(failure_context) +
                 ": prepare no longer classifies the loop countdown join as explicit loop-carry traffic")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* carrier_block = find_block(function, "carrier");
  if (carrier_block == nullptr || find_block(function, "preheader") == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the transparent entry carrier prefix")
                    .c_str());
  }

  bir::Block intruder;
  intruder.label = "intruder";
  intruder.terminator = bir::BranchTerminator{.target_label = carrier_block->label};
  function.blocks.push_back(std::move(intruder));

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a non-authoritative transparent entry prefix")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the non-authoritative transparent entry prefix with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_loop_countdown_route_requires_authoritative_prepared_branch_condition(
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
                 ": prepare no longer publishes the loop countdown control-flow contract")
                    .c_str());
  }
  if (control_flow->join_transfers.front().kind != prepare::PreparedJoinTransferKind::LoopCarry) {
    return fail((std::string(failure_context) +
                 ": prepare no longer classifies the loop countdown join as explicit loop-carry traffic")
                    .c_str());
  }

  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->branch_conditions.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture lost its mutable loop-countdown branch contract")
                    .c_str());
  }

  auto& branch_condition = mutable_control_flow->branch_conditions.front();
  branch_condition.condition_value =
      bir::Value::named(bir::TypeKind::I32, "drifted.loop.condition");

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a drifted prepared loop branch condition")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the drifted prepared loop branch condition with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_loop_countdown_route_rejects_transfer_drift_when_authoritative_branch_contract_remains(
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
                 ": prepared loop fixture lost its mutable loop-countdown handoff contract")
                    .c_str());
  }

  mutable_control_flow->join_transfers.front().kind =
      prepare::PreparedJoinTransferKind::SelectMaterialization;

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly reopened the local countdown fallback after loop-transfer drift")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the drifted loop transfer with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_local_countdown_guard_route_requires_authoritative_guard_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));

  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    prepared.control_flow.functions.push_back(
        prepare::PreparedControlFlowFunction{.function_name = function_name});
    control_flow = &prepared.control_flow.functions.back();
  }
  control_flow->branch_conditions.clear();
  control_flow->join_transfers.clear();

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer renders before authoritative guard ownership is injected: " +
                 ex.what())
                    .c_str());
  }

  control_flow->branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = function_name,
      .block_label = "guard",
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = bir::Value::named(bir::TypeKind::I32, "drifted.guard.condition"),
      .predicate = bir::BinaryOpcode::Ne,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "guard.counter"),
      .rhs = bir::Value::immediate_i32(0),
      .can_fuse_with_branch = true,
      .true_label = "drifted.guard.true",
      .false_label = "cont",
  });

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a local countdown guard fallback while authoritative guard ownership remained active")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the authoritative guard-owned countdown fallback with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_local_countdown_guard_route_rejects_authoritative_join_transfer(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));

  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    prepared.control_flow.functions.push_back(
        prepare::PreparedControlFlowFunction{.function_name = function_name});
    control_flow = &prepared.control_flow.functions.back();
  }
  control_flow->branch_conditions.clear();
  control_flow->join_transfers.clear();

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer renders before authoritative join ownership is injected: " +
                 ex.what())
                    .c_str());
  }

  control_flow->join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = function_name,
      .join_block_label = "loop1",
      .result = bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
      .kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot,
      .storage_name = "%lv.counter",
      .edge_transfers =
          {
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = "cont",
                  .successor_label = "loop1",
                  .incoming_value = bir::Value::immediate_i32(2),
                  .destination_value =
                      bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
                  .storage_name = "%lv.counter",
              },
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = "body1",
                  .successor_label = "loop1",
                  .incoming_value = bir::Value::named(bir::TypeKind::I32, "body1.next"),
                  .destination_value =
                      bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
                  .storage_name = "%lv.counter",
              },
          },
      .source_branch_block_label = "loop1",
      .source_true_transfer_index = 1,
      .source_false_transfer_index = 0,
      .source_true_incoming_label = "body1",
      .source_false_incoming_label = "cont",
  });

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a local countdown fallback while authoritative join ownership remained active")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the authoritative join-owned countdown fallback with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_loop_countdown_route_consumes_prepared_eq_zero_branch_contract(
    const bir::Module& module,
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
  auto& function = prepared.module.functions.front();
  auto* loop_block = find_block(function, "loop");
  auto* loop_compare =
      loop_block == nullptr || loop_block->insts.empty()
          ? nullptr
          : std::get_if<bir::BinaryInst>(&loop_block->insts.back());
  if (mutable_control_flow == nullptr || mutable_control_flow->branch_conditions.size() != 1 ||
      mutable_control_flow->join_transfers.size() != 1 || loop_block == nullptr ||
      loop_compare == nullptr ||
      loop_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture lost its mutable loop-countdown control-flow contract")
                    .c_str());
  }

  auto& branch_condition = mutable_control_flow->branch_conditions.front();
  branch_condition.predicate = bir::BinaryOpcode::Eq;
  branch_condition.true_label = "exit";
  branch_condition.false_label = "body";

  loop_block->terminator.true_label = branch_condition.true_label;
  loop_block->terminator.false_label = branch_condition.false_label;

  loop_compare->opcode = bir::BinaryOpcode::Eq;
  loop_compare->lhs = bir::Value::immediate_i32(7);
  loop_compare->rhs = bir::Value::immediate_i32(7);

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative eq-zero loop branch contract")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_consumes_prepared_eq_zero_branch_contract_with_zero_lhs(
    const bir::Module& module,
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
  auto& function = prepared.module.functions.front();
  auto* loop_block = find_block(function, "loop");
  auto* loop_compare =
      loop_block == nullptr || loop_block->insts.empty()
          ? nullptr
          : std::get_if<bir::BinaryInst>(&loop_block->insts.back());
  if (mutable_control_flow == nullptr || mutable_control_flow->branch_conditions.size() != 1 ||
      mutable_control_flow->join_transfers.size() != 1 || loop_block == nullptr ||
      loop_compare == nullptr ||
      loop_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture lost its mutable loop-countdown control-flow contract")
                    .c_str());
  }

  auto& branch_condition = mutable_control_flow->branch_conditions.front();
  branch_condition.predicate = bir::BinaryOpcode::Eq;
  branch_condition.lhs = bir::Value::immediate_i32(0);
  branch_condition.rhs = bir::Value::named(bir::TypeKind::I32, "counter");
  branch_condition.true_label = "exit";
  branch_condition.false_label = "body";

  loop_block->terminator.true_label = branch_condition.true_label;
  loop_block->terminator.false_label = branch_condition.false_label;

  loop_compare->opcode = bir::BinaryOpcode::Eq;
  loop_compare->lhs = bir::Value::immediate_i32(7);
  loop_compare->rhs = bir::Value::immediate_i32(7);

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative eq-zero loop branch contract when zero is the prepared lhs")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_prefers_authoritative_prepared_branch_labels(
    const bir::Module& module,
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

  auto& function = prepared.module.functions.front();
  auto* loop_block = find_block(function, "loop");
  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (loop_block == nullptr || mutable_control_flow == nullptr ||
      mutable_control_flow->branch_conditions.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture lost its mutable loop-countdown branch contract")
                    .c_str());
  }
  if (loop_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the expected loop cond branch")
                    .c_str());
  }

  auto& branch_condition = mutable_control_flow->branch_conditions.front();
  loop_block->terminator.condition = bir::Value::named(bir::TypeKind::I32, "drifted.loop.cond");
  loop_block->terminator.true_label = "drifted.body";
  loop_block->terminator.false_label = "drifted.exit";
  if (branch_condition.condition_value.kind != bir::Value::Kind::Named ||
      branch_condition.condition_value.name != "cmp0") {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the expected authoritative prepared condition value")
                    .c_str());
  }

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped preferring authoritative prepared loop branch metadata over raw loop terminator drift")
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

int run_backend_x86_handoff_boundary_compare_branch_tests();
int run_backend_x86_handoff_boundary_joined_branch_tests();
int run_backend_x86_handoff_boundary_short_circuit_tests();

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

  // Keep the bounded compare and joined compare-branch families isolated so
  // later Step 5 splits can keep shrinking the remaining loop and guard lanes
  // without re-threading earlier packets.
  if (const auto status = run_backend_x86_handoff_boundary_compare_branch_tests(); status != 0) {
    return status;
  }
  if (const auto status = run_backend_x86_handoff_boundary_joined_branch_tests(); status != 0) {
    return status;
  }
  if (const auto status = run_backend_x86_handoff_boundary_short_circuit_tests(); status != 0) {
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
          check_local_i32_guard_route_consumes_prepared_branch_contract(
              make_x86_local_i32_immediate_guard_module(),
              expected_minimal_local_i32_immediate_guard_asm("main", 123),
              "main",
              "minimal local-slot compare-against-immediate guard prepared branch-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_guard_route_requires_authoritative_prepared_branch_labels(
              make_x86_local_i32_immediate_guard_module(),
              "main",
              "minimal local-slot compare-against-immediate guard prepared branch-contract ownership rejects drifted prepared labels instead of falling back to the local guard matcher");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_guard_helper_publishes_prepared_compare_contract(
              make_x86_local_i32_immediate_guard_module(),
              "main",
              "minimal local-slot compare-against-immediate guard helper publishes prepared compare contract");
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
          check_loop_countdown_route_requires_supported_entry_handoff_carrier(
              make_x86_loop_countdown_join_module(),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership rejects a drifted entry handoff carrier lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_consumes_prepared_control_flow_with_reversed_join_edges(
              make_x86_loop_countdown_join_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership ignores join-edge ordering when predecessor labels stay authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_consumes_prepared_control_flow_with_preheader_block(
              make_x86_loop_countdown_join_with_preheader_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership tolerates a trivial preheader when prepared predecessor labels stay authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_prefers_prepared_preheader_handoff_value(
              make_x86_loop_countdown_join_with_preheader_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership keeps the init handoff value authoritative when a trivial preheader store drifts after prepare");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_consumes_prepared_control_flow_with_preheader_chain(
              make_x86_loop_countdown_join_with_preheader_chain_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership tolerates a transparent entry-carrier chain when the authoritative predecessor labels stay unchanged");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_prefers_prepared_preheader_handoff_value(
              make_x86_loop_countdown_join_with_preheader_chain_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership keeps the init handoff value authoritative through a transparent entry-carrier chain");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_requires_authoritative_transparent_entry_prefix(
              make_x86_loop_countdown_join_with_preheader_chain_module(),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership rejects a transparent entry-carrier prefix once it stops being the unique authoritative init path");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_requires_authoritative_prepared_branch_condition(
              make_x86_loop_countdown_join_module(),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership rejects drifted prepared branch metadata instead of falling back to local countdown topology");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_rejects_transfer_drift_when_authoritative_branch_contract_remains(
              make_x86_loop_countdown_join_module(),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership rejects drifted loop-transfer metadata instead of reopening the local countdown fallback");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_countdown_guard_route_requires_authoritative_guard_branch_condition(
              make_x86_local_i32_countdown_guard_continuation_module(),
              "main",
              "two-segment local countdown fallback rejects mixed guard ownership once prepared branch metadata becomes authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_countdown_guard_route_rejects_authoritative_join_transfer(
              make_x86_local_i32_countdown_guard_continuation_module(),
              "main",
              "two-segment local countdown fallback rejects authoritative join ownership on the continuation loop");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_consumes_prepared_eq_zero_branch_contract(
              make_x86_loop_countdown_join_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership respects the alternate eq-zero branch contract");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_consumes_prepared_eq_zero_branch_contract_with_zero_lhs(
              make_x86_loop_countdown_join_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership respects the mirrored alternate eq-zero branch contract");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_prefers_authoritative_prepared_branch_labels(
              make_x86_loop_countdown_join_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership prefers authoritative prepared branch metadata over raw loop-terminator drift");
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
          check_local_i16_guard_route_requires_authoritative_prepared_branch_labels(
              make_x86_local_i16_increment_guard_module(),
              "main",
              "minimal local-slot i16 increment guard prepared branch-contract ownership rejects drifted prepared labels instead of falling back to the raw guard matcher");
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
          check_i32_guard_chain_route_requires_authoritative_prepared_branch_labels(
              make_x86_immediate_i32_guard_chain_module(),
              "main",
              "block_2",
              "minimal non-global equality-against-immediate guard-chain route rejects drifted prepared branch labels instead of falling back to the raw guard-chain matcher");
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
          check_i32_guard_chain_route_requires_authoritative_prepared_branch_labels(
              make_x86_same_module_global_i32_guard_chain_module(),
              "main",
              "block_2",
              "same-module defined-global equality-against-immediate guard route rejects drifted prepared branch labels instead of falling back to the raw guard-chain matcher");
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
          check_i32_guard_chain_route_requires_authoritative_prepared_branch_labels(
              make_x86_pointer_backed_same_module_global_guard_chain_module(),
              "main",
              "block_10",
              "pointer-backed same-module global equality-against-immediate guard route rejects drifted prepared branch labels instead of falling back to the raw guard-chain matcher");
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
