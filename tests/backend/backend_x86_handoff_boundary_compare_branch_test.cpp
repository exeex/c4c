#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir/lowering.hpp"
#include "src/backend/mir/x86/abi/abi.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"  // Compatibility holdout for render_prepared_stack_memory_operand().
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
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

std::optional<std::size_t> find_block_index(const bir::Function& function, const char* label) {
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    if (function.blocks[index].label == label) {
      return index;
    }
  }
  return std::nullopt;
}

const prepare::PreparedValueLocationFunction* find_value_location_function(
    const prepare::PreparedBirModule& prepared,
    const char* function_name) {
  return prepare::find_prepared_value_location_function(prepared, function_name);
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

std::string expected_stack_home_zero_branch_prefix(const char* function_name,
                                                   const char* false_label,
                                                   const char* false_branch_opcode,
                                                   std::size_t frame_size,
                                                   std::size_t byte_offset) {
  return asm_header(function_name) + "    sub rsp, " + std::to_string(frame_size) + "\n    mov eax, " +
         c4c::backend::x86::render_prepared_stack_memory_operand(byte_offset, "DWORD") +
         "\n    test eax, eax\n    add rsp, " + std::to_string(frame_size) + "\n    " +
         false_branch_opcode + " .L" + function_name + "_" + false_label + "\n";
}

std::string expected_rematerialized_zero_branch_prefix(const char* function_name,
                                                       const char* false_label,
                                                       const char* false_branch_opcode,
                                                       int immediate) {
  return asm_header(function_name) + "    mov eax, " + std::to_string(immediate) +
         "\n    test eax, eax\n    " + false_branch_opcode + " .L" + function_name + "_" +
         false_label + "\n";
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

std::string expected_stack_home_zero_branch_joined_add_or_sub_then_xor_asm(
    const char* function_name,
    const char* false_label,
    std::size_t frame_size,
    std::size_t byte_offset,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  const auto stack_operand =
      c4c::backend::x86::render_prepared_stack_memory_operand(byte_offset, "DWORD");
  return expected_stack_home_zero_branch_prefix(
             function_name, false_label, "jne", frame_size, byte_offset) +
         "    sub rsp, " + std::to_string(frame_size) + "\n    mov " +
         minimal_i32_return_register() + ", " + stack_operand + "\n    add " +
         minimal_i32_return_register() + ", " + std::to_string(true_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    add rsp, " + std::to_string(frame_size) + "\n    ret\n.L" + function_name +
         "_" + false_label + ":\n    sub rsp, " + std::to_string(frame_size) + "\n    mov " +
         minimal_i32_return_register() + ", " + stack_operand + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    xor " +
         minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    add rsp, " + std::to_string(frame_size) + "\n    ret\n";
}

std::string expected_rematerialized_zero_branch_joined_add_or_sub_then_xor_asm(
    const char* function_name,
    const char* false_label,
    int immediate,
    int true_immediate,
    int false_immediate,
    int joined_immediate) {
  return expected_rematerialized_zero_branch_prefix(function_name, false_label, "jne", immediate) +
         "    mov " + minimal_i32_return_register() + ", " + std::to_string(immediate) +
         "\n    add " + minimal_i32_return_register() + ", " + std::to_string(true_immediate) +
         "\n    xor " + minimal_i32_return_register() + ", " + std::to_string(joined_immediate) +
         "\n    ret\n.L" + function_name + "_" + false_label + ":\n    mov " +
         minimal_i32_return_register() + ", " + std::to_string(immediate) + "\n    sub " +
         minimal_i32_return_register() + ", " + std::to_string(false_immediate) + "\n    xor " +
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

void attach_structured_block_label_ids(bir::Module& module) {
  for (auto& function : module.functions) {
    for (auto& block : function.blocks) {
      block.label_id = module.names.block_labels.intern(block.label);
      if (block.terminator.kind == bir::TerminatorKind::Branch) {
        block.terminator.target_label_id =
            module.names.block_labels.intern(block.terminator.target_label);
      } else if (block.terminator.kind == bir::TerminatorKind::CondBranch) {
        block.terminator.true_label_id =
            module.names.block_labels.intern(block.terminator.true_label);
        block.terminator.false_label_id =
            module.names.block_labels.intern(block.terminator.false_label);
      }
      for (auto& inst : block.insts) {
        auto* phi = std::get_if<bir::PhiInst>(&inst);
        if (phi == nullptr) {
          continue;
        }
        for (auto& incoming : phi->incomings) {
          incoming.label_id = module.names.block_labels.intern(incoming.label);
        }
      }
    }
  }
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
  attach_structured_block_label_ids(module);
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
  attach_structured_block_label_ids(module);
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
  attach_structured_block_label_ids(module);
  return module;
}

bir::Module make_x86_multi_param_compare_driven_boundary_module() {
  auto module = make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module();
  if (module.functions.empty()) {
    return module;
  }
  module.functions.front().params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.extra",
      .size_bytes = 8,
      .align_bytes = 8,
  });
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
  attach_structured_block_label_ids(module);
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
    const auto dead_label = prepared.module.names.block_labels.intern("contract.dead.unreachable");
    auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
    if (mutable_control_flow == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared minimal compare branch fixture lost its mutable control-flow contract")
                      .c_str());
    }
    mutable_control_flow->blocks.push_back(prepare::PreparedControlFlowBlock{
        .block_label = prepared.names.block_labels.intern("contract.dead.unreachable"),
        .terminator_kind = bir::TerminatorKind::Return,
    });
    function.blocks.push_back(bir::Block{
        .label = "contract.dead.unreachable",
        .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(99)},
        .label_id = dead_label,
    });
  }

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
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
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
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
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
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

int check_compare_join_requires_authoritative_join_transfer_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }

  control_flow->join_transfers.clear();
  control_flow->parallel_copy_bundles.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly reopened raw compare-join recovery after authoritative out-of-SSA metadata was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing compare-join metadata with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_compare_join_requires_authoritative_parallel_copy_bundles(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.empty() || control_flow->parallel_copy_bundles.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join out-of-SSA handoff contract")
                    .c_str());
  }

  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->branch_conditions.size() != 1 ||
      mutable_control_flow->join_transfers.empty() ||
      mutable_control_flow->parallel_copy_bundles.empty()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture lost its mutable out-of-SSA handoff contract")
                    .c_str());
  }

  mutable_control_flow->parallel_copy_bundles.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted compare-join phi edge obligations after authoritative parallel-copy publication was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected missing compare-join parallel-copy publication with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_compare_join_regalloc_consumes_predecessor_parallel_copy_execution_site(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  const auto* function_locations = find_value_location_function(prepared, function_name);
  if (control_flow == nullptr || function_locations == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join regalloc/value-location contract")
                    .c_str());
  }

  const auto* bundle =
      prepare::find_prepared_parallel_copy_bundle(prepared.names, *control_flow, "is_zero", "join");
  if (bundle == nullptr ||
      bundle->execution_site != prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
    return fail((std::string(failure_context) +
                 ": prepare no longer classifies the compare-join handoff as predecessor-terminator executable")
                    .c_str());
  }

  const auto predecessor_block_index = find_block_index(prepared.module.functions.front(), "is_zero");
  const auto successor_block_index = find_block_index(prepared.module.functions.front(), "join");
  if (!predecessor_block_index.has_value() || !successor_block_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes the expected blocks for bundle placement")
                    .c_str());
  }

  const auto* move_bundle = prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
      prepared.names, prepared.module.functions.front(), *function_locations, *bundle);
  if (move_bundle == nullptr) {
    return fail((std::string(failure_context) +
                 ": regalloc stopped placing predecessor-owned compare-join bundles at the published predecessor block")
                    .c_str());
  }
  if (move_bundle->block_index != *predecessor_block_index) {
    return fail((std::string(failure_context) +
                 ": regalloc unexpectedly relocated predecessor-owned compare-join bundles into the join block")
                    .c_str());
  }

  return 0;
}

int check_compare_join_route_accepts_successor_entry_parallel_copy_handoff(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow =
      find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join control-flow contract")
                    .c_str());
  }
  auto* bundle = const_cast<prepare::PreparedParallelCopyBundle*>(
      prepare::find_prepared_parallel_copy_bundle(
          prepared.names, *control_flow, "is_zero", "join"));
  if (bundle == nullptr ||
      bundle->execution_site != prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
    return fail((std::string(failure_context) +
                 ": prepare no longer exposes the predecessor-owned compare-join bundle needed for successor-entry handoff mutation")
                    .c_str());
  }

  if (prepared.module.functions.empty()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes a function body for bundle relocation")
                    .c_str());
  }
  const auto predecessor_block_index = find_block_index(prepared.module.functions.front(), "is_zero");
  const auto successor_block_index = find_block_index(prepared.module.functions.front(), "join");
  if (!predecessor_block_index.has_value() || !successor_block_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared compare-join fixture no longer exposes the expected blocks for successor-entry relocation")
                    .c_str());
  }
  const auto published_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*bundle);
  if (!published_execution_block.has_value() ||
      prepare::prepared_block_label(prepared.names, *published_execution_block) != "is_zero") {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the predecessor-owned compare-join execution block directly")
                    .c_str());
  }

  const auto* function_locations = find_value_location_function(prepared, function_name);
  if (function_locations == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the compare-join value-location contract")
                    .c_str());
  }
  auto function_locations_it =
      std::find_if(prepared.value_locations.functions.begin(),
                   prepared.value_locations.functions.end(),
                   [&](const prepare::PreparedValueLocationFunction& candidate) {
                     return candidate.function_name == function_locations->function_name;
                   });
  if (function_locations_it == prepared.value_locations.functions.end()) {
    return fail((std::string(failure_context) +
                 ": compare-join value-location publication became unstable during successor-entry relocation")
                    .c_str());
  }
  auto* move_bundle = prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
      prepared.names, prepared.module.functions.front(), *function_locations_it, *bundle);
  if (move_bundle == nullptr ||
      !prepare::prepared_move_bundle_has_out_of_ssa_parallel_copy_authority(*move_bundle)) {
    return fail((std::string(failure_context) +
                 ": regalloc no longer publishes explicit predecessor-owned out-of-SSA move authority for successor-entry relocation")
                    .c_str());
  }

  bundle->execution_site = prepare::PreparedParallelCopyExecutionSite::SuccessorEntry;
  bundle->execution_block_label =
      prepare::resolve_prepared_block_label_id(prepared.names, "join");
  const auto successor_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*bundle);
  if (!successor_execution_block.has_value() ||
      prepare::prepared_block_label(prepared.names, *successor_execution_block) != "join") {
    return fail((std::string(failure_context) +
                 ": successor-entry relocation no longer flows through the published execution-block seam")
                    .c_str());
  }

  move_bundle->block_index = *successor_block_index;
  move_bundle->instruction_index = 0;
  for (auto& move : move_bundle->moves) {
    move.block_index = *successor_block_index;
    move.instruction_index = 0;
  }

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::invalid_argument& error) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected a successor-entry compare-join handoff after the authoritative block-entry move bundle was relocated into the successor block: " +
                 error.what())
                    .c_str());
  }

  return 0;
}

int check_minimal_compare_branch_param_return_requires_authoritative_prepared_return_bundle(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, function_name);
  if (function_locations == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the parameter-return value-location contract")
                    .c_str());
  }

  auto& mutable_locations = prepared.value_locations.functions.front();
  mutable_locations.move_bundles.erase(
      std::remove_if(
          mutable_locations.move_bundles.begin(),
          mutable_locations.move_bundles.end(),
          [](const prepare::PreparedMoveBundle& bundle) {
            return bundle.phase == prepare::PreparedMovePhase::BeforeReturn;
          }),
      mutable_locations.move_bundles.end());

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a parameter-leaf return after the authoritative prepared return bundle was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing prepared return bundle with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_minimal_compare_branch_param_return_requires_authoritative_prepared_return_home(
    const bir::Module& module,
    const char* function_name,
    const char* returned_value_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto function_name_id = prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_name_id.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer resolves the compare-branch function name for value-home ownership")
                    .c_str());
  }
  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, returned_value_name);
  if (!value_name_id.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer resolves the compare-branch returned value name for value-home ownership")
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
                 ": prepare no longer publishes the compare-branch value-location contract")
                    .c_str());
  }
  const auto erased_homes = static_cast<std::size_t>(std::count_if(
      function_locations_it->value_homes.begin(),
      function_locations_it->value_homes.end(),
      [&](const prepare::PreparedValueHome& value_home) {
        return value_home.value_name == *value_name_id;
      }));
  if (erased_homes == 0) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the authoritative prepared return home for the compare-branch leaf")
                    .c_str());
  }
  function_locations_it->value_homes.erase(
      std::remove_if(
          function_locations_it->value_homes.begin(),
          function_locations_it->value_homes.end(),
          [&](const prepare::PreparedValueHome& value_home) {
            return value_home.value_name == *value_name_id;
          }),
      function_locations_it->value_homes.end());

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a parameter-leaf return after the authoritative prepared return home was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing prepared return home with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

prepare::PreparedValueHome* find_mutable_value_home(prepare::PreparedBirModule& prepared,
                                                    const char* function_name,
                                                    const char* value_name) {
  const auto function_name_id = prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_name_id.has_value()) {
    return nullptr;
  }
  auto function_locations_it =
      std::find_if(prepared.value_locations.functions.begin(),
                   prepared.value_locations.functions.end(),
                   [&](const prepare::PreparedValueLocationFunction& function_locations) {
                     return function_locations.function_name == *function_name_id;
                   });
  if (function_locations_it == prepared.value_locations.functions.end()) {
    return nullptr;
  }
  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, value_name);
  if (!value_name_id.has_value()) {
    return nullptr;
  }
  for (auto& value_home : function_locations_it->value_homes) {
    if (value_home.value_name == *value_name_id) {
      return &value_home;
    }
  }
  return nullptr;
}

int check_minimal_compare_branch_stack_home_consumes_prepared_entry_home(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* param_home = find_mutable_value_home(prepared, function_name, "p.x");
  if (param_home == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the authoritative prepared parameter home")
                    .c_str());
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  param_home->immediate_i32.reset();
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared stack entry home")
                    .c_str());
  }

  return 0;
}

int check_minimal_compare_branch_rematerialized_home_consumes_prepared_entry_home(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* param_home = find_mutable_value_home(prepared, function_name, "p.x");
  if (param_home == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the authoritative prepared parameter home")
                    .c_str());
  }

  param_home->kind = prepare::PreparedValueHomeKind::RematerializableImmediate;
  param_home->register_name.reset();
  param_home->slot_id.reset();
  param_home->offset_bytes.reset();
  param_home->immediate_i32 = 13;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared rematerializable entry home")
                    .c_str());
  }

  return 0;
}

int check_compare_join_stack_home_consumes_prepared_entry_and_return_home(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* param_home = find_mutable_value_home(prepared, function_name, "p.x");
  if (param_home == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the authoritative prepared parameter home")
                    .c_str());
  }

  param_home->kind = prepare::PreparedValueHomeKind::StackSlot;
  param_home->register_name.reset();
  param_home->slot_id = 0;
  param_home->offset_bytes = 0;
  param_home->immediate_i32.reset();
  prepared.stack_layout.frame_size_bytes = 4;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared stack home through compare-join entry and return")
                    .c_str());
  }

  return 0;
}

int check_compare_join_rematerialized_home_consumes_prepared_entry_and_return_home(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* param_home = find_mutable_value_home(prepared, function_name, "p.x");
  if (param_home == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the authoritative prepared parameter home")
                    .c_str());
  }

  param_home->kind = prepare::PreparedValueHomeKind::RematerializableImmediate;
  param_home->register_name.reset();
  param_home->slot_id.reset();
  param_home->offset_bytes.reset();
  param_home->immediate_i32 = 13;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared rematerialized home through compare-join entry and return")
                    .c_str());
  }

  return 0;
}

int check_multi_param_compare_driven_shape_rejection(const bir::Module& module,
                                                     const char* failure_context) {
  constexpr std::string_view kExpectedMessage =
      "only supports a minimal single-block i32 return terminator, a bounded equality-against-immediate guard family with immediate return leaves including fixed-offset same-module global i32 loads and pointer-backed same-module global roots, or one bounded compare-against-zero branch family through the canonical prepared-module handoff";
  c4c::TargetProfile target_profile;
  const auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a multi-parameter compare-driven join shape")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(kExpectedMessage) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the multi-parameter compare-driven join shape with the wrong message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_x86_bir_module_entry(module, target_profile);
    return fail((std::string(failure_context) +
                 ": explicit x86 BIR entry unexpectedly accepted a multi-parameter compare-driven join shape")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(kExpectedMessage) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": explicit x86 BIR entry rejected the multi-parameter compare-driven join shape with the wrong message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_module(
        BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
    return fail((std::string(failure_context) +
                 ": generic x86 backend emit path unexpectedly accepted a multi-parameter compare-driven join shape")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(kExpectedMessage) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": generic x86 backend emit path rejected the multi-parameter compare-driven join shape with the wrong message")
                      .c_str());
    }
  }

  return 0;
}

int check_minimal_compare_branch_requires_authoritative_prepared_entry_home(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto function_name_id = prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_name_id.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer resolves the compare-branch function name for value-home ownership")
                    .c_str());
  }
  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, "p.x");
  if (!value_name_id.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer resolves the compare-branch parameter name for value-home ownership")
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
                 ": prepare no longer publishes the compare-branch value-location contract")
                    .c_str());
  }

  function_locations_it->value_homes.erase(
      std::remove_if(function_locations_it->value_homes.begin(),
                     function_locations_it->value_homes.end(),
                     [&](const prepare::PreparedValueHome& value_home) {
                       return value_home.value_name == *value_name_id;
                     }),
      function_locations_it->value_homes.end());

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly reopened ABI entry fallback after the authoritative prepared home was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing prepared entry home with the wrong contract message")
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
          check_minimal_compare_branch_stack_home_consumes_prepared_entry_home(
              make_x86_param_eq_zero_branch_module(),
              expected_stack_home_zero_branch_prefix("branch_on_zero", "is_nonzero", "jne", 4, 0) +
                  "    mov " + minimal_i32_return_register() + ", 7\n    ret\n.Lbranch_on_zero_is_nonzero:\n    mov " +
                  minimal_i32_return_register() + ", 11\n    ret\n",
              "branch_on_zero",
              "scalar-control-flow compare-against-zero branch lane with stack entry home");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_rematerialized_home_consumes_prepared_entry_home(
              make_x86_param_eq_zero_branch_module(),
              expected_rematerialized_zero_branch_prefix("branch_on_zero", "is_nonzero", "jne", 13) +
                  "    mov " + minimal_i32_return_register() + ", 7\n    ret\n.Lbranch_on_zero_is_nonzero:\n    mov " +
                  minimal_i32_return_register() + ", 11\n    ret\n",
              "branch_on_zero",
              "scalar-control-flow compare-against-zero branch lane with rematerializable entry home");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_requires_authoritative_prepared_entry_home(
              make_x86_param_eq_zero_branch_module(),
              "branch_on_zero",
              "scalar-control-flow compare-against-zero branch lane rejects reopening ABI entry fallback when the authoritative prepared home is missing");
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
          check_minimal_compare_branch_requires_authoritative_prepared_branch_labels(
              make_x86_param_ne_zero_branch_module(),
              "branch_on_nonzero",
              "scalar-control-flow compare-against-zero nonzero branch lane rejects drifted authoritative prepared branch labels instead of falling back to raw branch recovery");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_requires_authoritative_prepared_branch_record(
              make_x86_param_ne_zero_branch_module(),
              "branch_on_nonzero",
              "scalar-control-flow compare-against-zero nonzero branch lane rejects reopening raw branch recovery when the authoritative prepared branch record is missing");
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
          check_minimal_compare_branch_consumes_prepared_control_flow(
              make_x86_param_eq_zero_branch_param_or_immediate_module(),
              expected_minimal_param_eq_zero_branch_param_or_immediate_asm(
                  "branch_zero_or_passthrough", "is_nonzero", 5),
              "branch_zero_or_passthrough",
              "scalar-control-flow compare-against-zero branch lane with parameter leaf return prepared-control-flow ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_requires_authoritative_prepared_branch_labels(
              make_x86_param_eq_zero_branch_param_or_immediate_module(),
              "branch_zero_or_passthrough",
              "scalar-control-flow compare-against-zero branch lane with parameter leaf return rejects drifted authoritative prepared branch labels instead of falling back to raw branch recovery");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_requires_authoritative_prepared_branch_record(
              make_x86_param_eq_zero_branch_param_or_immediate_module(),
              "branch_zero_or_passthrough",
              "scalar-control-flow compare-against-zero branch lane with parameter leaf return rejects reopening raw branch recovery when the authoritative prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_param_return_requires_authoritative_prepared_return_bundle(
              make_x86_param_eq_zero_branch_param_or_immediate_module(),
              "branch_zero_or_passthrough",
              "scalar-control-flow compare-against-zero branch lane with parameter leaf return rejects reopening local return fallback when the authoritative prepared return bundle is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_param_return_requires_authoritative_prepared_return_home(
              make_x86_param_eq_zero_branch_param_or_immediate_module(),
              "branch_zero_or_passthrough",
              "p.x",
              "scalar-control-flow compare-against-zero branch lane with parameter leaf return rejects reopening local return fallback when the authoritative prepared home is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_compare_join_stack_home_consumes_prepared_entry_and_return_home(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_stack_home_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 4, 0, 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join lane with stack-backed parameter home");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_compare_join_rematerialized_home_consumes_prepared_entry_and_return_home(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              expected_rematerialized_zero_branch_joined_add_or_sub_then_xor_asm(
                  "branch_join_adjust_then_xor", "is_nonzero", 13, 5, 1, 3),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join lane with rematerializable parameter home");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_requires_authoritative_prepared_entry_home(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join lane rejects reopening ABI entry fallback when the authoritative prepared home is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_param_return_requires_authoritative_prepared_return_bundle(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join lane rejects reopening local return fallback when the authoritative prepared return bundle is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_minimal_compare_branch_param_return_requires_authoritative_prepared_return_home(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "p.x",
              "scalar-control-flow compare-against-zero compare-join lane rejects reopening local return fallback when the authoritative prepared home is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_compare_join_requires_authoritative_join_transfer_record(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join lane rejects reopening raw compare-join recovery when authoritative out-of-SSA join metadata is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_compare_join_requires_authoritative_parallel_copy_bundles(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join lane rejects reopening phi-edge fallback when authoritative parallel-copy publication is missing but join transfers remain");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_compare_join_regalloc_consumes_predecessor_parallel_copy_execution_site(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join lane keeps predecessor-owned bundles on the published predecessor execution site");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_compare_join_route_accepts_successor_entry_parallel_copy_handoff(
              make_x86_param_eq_zero_branch_joined_add_or_sub_then_xor_module(),
              "branch_join_adjust_then_xor",
              "scalar-control-flow compare-against-zero compare-join lane still accepts a successor-entry prepared handoff once regalloc has relocated the authoritative phi move bundle into the successor block");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_multi_param_compare_driven_shape_rejection(
              make_x86_multi_param_compare_driven_boundary_module(),
              "scalar-control-flow compare-driven join lane routes multi-parameter shapes into the downstream generic scalar-family rejection instead of the old single-i32 compare-entry gate");
      status != 0) {
    return status;
  }
  return 0;
}
