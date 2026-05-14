#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
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

int check_route_rejection(const bir::Module& module,
                          std::string_view expected_message_fragment,
                          const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
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
    (void)c4c::backend::emit_x86_bir_module_entry(module, target_profile);
    return fail((std::string(failure_context) +
                 ": explicit x86 BIR entry unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": explicit x86 BIR entry rejected the out-of-scope route with the wrong contract message")
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

std::string expected_x86_constant_pointer_eq_null_return_asm() {
  return ".intel_syntax noprefix\n"
         ".text\n"
         ".globl main\n"
         ".type main, @function\n"
         "main:\n"
         "    mov eax, 0\n"
         "    ret\n";
}

std::string expected_x86_constant_pointer_ne_null_return_asm() {
  return ".intel_syntax noprefix\n"
         ".text\n"
         ".globl main\n"
         ".type main, @function\n"
         "main:\n"
         "    mov eax, 1\n"
         "    ret\n";
}

bir::Module make_x86_constant_pointer_compare_null_return_module(bir::BinaryOpcode opcode,
                                                                 std::string result_name) {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "abc",
  });

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = opcode,
      .result = bir::Value::named(bir::TypeKind::I32, result_name),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "%t0"),
      .rhs = bir::Value{
          .kind = bir::Value::Kind::Immediate,
          .type = bir::TypeKind::Ptr,
          .immediate = 0,
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, result_name),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_constant_pointer_eq_null_return_module() {
  return make_x86_constant_pointer_compare_null_return_module(bir::BinaryOpcode::Eq, "%t2");
}

bir::Module make_x86_constant_pointer_ne_null_return_module() {
  return make_x86_constant_pointer_compare_null_return_module(bir::BinaryOpcode::Ne, "%t3");
}

int check_constant_pointer_eq_null_single_block_return_route() {
  auto module = make_x86_constant_pointer_eq_null_return_module();
  return check_route_outputs(module,
                             expected_x86_constant_pointer_eq_null_return_asm(),
                             "%t2 = bir.eq ptr %t0, 0",
                             "single-block constant pointer-eq-null return route");
}

int check_constant_pointer_ne_null_single_block_return_route() {
  auto module = make_x86_constant_pointer_ne_null_return_module();
  return check_route_outputs(module,
                             expected_x86_constant_pointer_ne_null_return_asm(),
                             "%t3 = bir.ne ptr %t0, 0",
                             "single-block constant pointer-ne-null return route");
}

}  // namespace

int run_backend_x86_handoff_boundary_compare_branch_tests();
int run_backend_x86_handoff_boundary_direct_extern_call_tests();
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
  if (const auto status = check_constant_pointer_eq_null_single_block_return_route();
      status != 0) {
    return status;
  }
  if (const auto status = check_constant_pointer_ne_null_single_block_return_route();
      status != 0) {
    return status;
  }

  if (const auto status = run_backend_x86_handoff_boundary_scalar_smoke_tests(); status != 0) {
    return status;
  }

  // Keep the bounded compare and joined compare-branch families isolated so
  // later Step 5 splits can keep shrinking the remaining loop and guard lanes
  // without re-threading earlier packets.
  if (const auto status = run_backend_x86_handoff_boundary_compare_branch_tests(); status != 0) {
    return status;
  }
  if (const auto status = run_backend_x86_handoff_boundary_direct_extern_call_tests();
      status != 0) {
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
