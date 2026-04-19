#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
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

int check_route_outputs(const bir::Module& module,
                        const std::string& expected_asm,
                        const std::string& expected_bir_fragment,
                        const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared = prepare::prepare_semantic_bir_module_with_options(
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

int check_minimal_compare_branch_consumes_prepared_control_flow_impl(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context,
    bool add_unreachable_block) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
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
  if (add_unreachable_block) {
    function.blocks.push_back(bir::Block{
        .label = "contract.dead.unreachable",
        .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(99)},
    });
  }

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared branch metadata")
                    .c_str());
  }

  return 0;
}

int check_minimal_compare_branch_consumes_prepared_control_flow(const bir::Module& module,
                                                                const std::string& expected_asm,
                                                                const char* function_name,
                                                                const char* failure_context) {
  return check_minimal_compare_branch_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, false);
}

int check_minimal_compare_branch_with_unreachable_block_consumes_prepared_control_flow(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  return check_minimal_compare_branch_consumes_prepared_control_flow_impl(
      module, expected_asm, function_name, failure_context, true);
}

int check_minimal_compare_branch_requires_authoritative_prepared_branch_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the minimal compare branch control-flow contract")
                    .c_str());
  }

  control_flow->branch_conditions.front().false_label =
      prepared.names.block_labels.intern("drifted.compare.false");

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted drifted prepared compare-branch labels")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected drifted prepared compare-branch labels with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_minimal_compare_branch_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the minimal compare branch control-flow contract")
                    .c_str());
  }

  control_flow->branch_conditions.clear();

  try {
    (void)c4c::backend::x86::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly reopened raw compare-branch recovery after the prepared branch record was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing prepared compare-branch record with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_compare_branch_tests() {
  // Keep the first prepared-control-flow branch lane isolated so later splits
  // can move adjacent join and loop families without re-threading this packet.
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
          check_minimal_compare_branch_requires_authoritative_prepared_branch_labels(
              make_x86_param_eq_zero_branch_module(),
              "branch_on_zero",
              "scalar-control-flow compare-against-zero branch lane rejects drifted authoritative prepared branch labels instead of falling back to raw branch recovery");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_module(),
              "branch_on_zero",
              "scalar-control-flow compare-against-zero branch lane rejects reopening raw branch recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_with_unreachable_block_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_module(),
              expected_minimal_param_eq_zero_branch_asm("branch_on_zero", "is_nonzero", 7, 11),
              "branch_on_zero",
              "scalar-control-flow compare-against-zero branch lane ignores unrelated block count when prepared-control-flow ownership is authoritative");
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
  return 0;
}
