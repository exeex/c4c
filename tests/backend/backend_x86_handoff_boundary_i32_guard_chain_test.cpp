#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

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

bir::Block* find_block(bir::Function& function, const char* label) {
  for (auto& block : function.blocks) {
    if (block.label == label) {
      return &block;
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

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
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
           "    je .L" + std::string(function_name) + "_block_4\n"
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

int check_same_module_global_guard_chain_route_consumes_prepared_address_contract(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_name);
  if (function_addressing == nullptr || function_addressing->accesses.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the same-module global prepared accesses")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* block_2 = find_block(function, "block_2");
  if (entry_block == nullptr || block_2 == nullptr || entry_block->insts.empty() ||
      block_2->insts.empty()) {
    return fail((std::string(failure_context) +
                 ": prepared same-module global guard fixture no longer exposes the expected load carriers")
                    .c_str());
  }
  auto* entry_load = std::get_if<bir::LoadGlobalInst>(&entry_block->insts.front());
  auto* second_load = std::get_if<bir::LoadGlobalInst>(&block_2->insts.front());
  if (entry_load == nullptr || second_load == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared same-module global guard fixture no longer exposes the expected direct global load carriers")
                    .c_str());
  }

  entry_load->global_name = "drifted_same_module_global";
  second_load->global_name = "drifted_same_module_global";

  const auto prepared_asm = c4c::backend::x86::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative prepared same-module global accesses over drifted raw global carriers")
                    .c_str());
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_i32_guard_chain_tests() {
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
          check_same_module_global_guard_chain_route_consumes_prepared_address_contract(
              make_x86_same_module_global_i32_guard_chain_module(),
              expected_minimal_same_module_global_i32_guard_chain_asm("main"),
              "main",
              "same-module defined-global equality-against-immediate guard route consumes authoritative prepared global accesses");
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

  return 0;
}
