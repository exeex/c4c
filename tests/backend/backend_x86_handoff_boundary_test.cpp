#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"
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

}  // namespace

int run_backend_x86_handoff_boundary_compare_branch_tests();
int run_backend_x86_handoff_boundary_joined_branch_tests();
int run_backend_x86_handoff_boundary_local_i16_guard_tests();
int run_backend_x86_handoff_boundary_local_i32_guard_tests();
int run_backend_x86_handoff_boundary_short_circuit_tests();
int run_backend_x86_handoff_boundary_loop_countdown_tests();
int run_backend_x86_handoff_boundary_i32_guard_chain_tests();
int run_backend_x86_handoff_boundary_lir_tests();
int run_backend_x86_handoff_boundary_multi_defined_call_tests();
int run_backend_x86_handoff_boundary_multi_defined_rejection_tests();
int run_backend_x86_handoff_boundary_scalar_smoke_tests();
int run_backend_x86_handoff_boundary_local_slot_guard_lane_tests();

int main() {
  if (const auto status = run_backend_x86_handoff_boundary_scalar_smoke_tests(); status != 0) {
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
  if (const auto status = run_backend_x86_handoff_boundary_local_i32_guard_tests();
      status != 0) {
    return status;
  }
  if (const auto status = run_backend_x86_handoff_boundary_local_i16_guard_tests();
      status != 0) {
    return status;
  }

  if (const auto status = run_backend_x86_handoff_boundary_local_slot_guard_lane_tests();
      status != 0) {
    return status;
  }

  if (const auto status = run_backend_x86_handoff_boundary_loop_countdown_tests(); status != 0) {
    return status;
  }

  if (const auto status = run_backend_x86_handoff_boundary_i32_guard_chain_tests();
      status != 0) {
    return status;
  }

  if (const auto status = run_backend_x86_handoff_boundary_multi_defined_call_tests();
      status != 0) {
    return status;
  }

  if (const auto status = run_backend_x86_handoff_boundary_multi_defined_rejection_tests();
      status != 0) {
    return status;
  }

  if (const auto status = run_backend_x86_handoff_boundary_lir_tests(); status != 0) {
    return status;
  }

  return 0;
}
