#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <algorithm>
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

bir::Block* find_block(bir::Function& function, std::string_view label) {
  for (auto& block : function.blocks) {
    if (block.label == label) {
      return &block;
    }
  }
  return nullptr;
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
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

std::string expected_minimal_local_i8_copy_guard_asm(const char* function_name,
                                                     int immediate) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov BYTE PTR [rsp + 7], " + std::to_string(immediate) + "\n"
         "    movsx eax, BYTE PTR [rsp + 7]\n"
         "    mov BYTE PTR [rsp + 6], al\n"
         "    movsx eax, BYTE PTR [rsp + 6]\n"
         "    cmp eax, " + std::to_string(immediate) + "\n"
         "    je .L" + std::string(function_name) + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_local_i32_branch_passthrough_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov DWORD PTR [rsp], 5\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    cmp eax, 5\n"
         "    je .L" + std::string(function_name) + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + std::string(function_name) + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
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

bir::Module make_x86_local_i32_branch_passthrough_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.branch.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.branch.0",
      .value = bir::Value::immediate_i32(5),
  });
  entry.terminator = bir::BranchTerminator{.target_label = "guard"};

  bir::Block guard;
  guard.label = "guard";
  guard.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .slot_name = "%lv.branch.0",
  });
  guard.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .rhs = bir::Value::immediate_i32(5),
  });
  guard.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t1"),
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
  function.blocks.push_back(std::move(guard));
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

bir::Module make_x86_local_i8_copy_guard_module() {
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
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.arr.6",
      .value = bir::Value::named(bir::TypeKind::I8, "loaded.byte"),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "copied.byte"),
      .slot_name = "%lv.arr.6",
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "extended.byte"),
      .operand = bir::Value::named(bir::TypeKind::I8, "copied.byte"),
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
  block_2.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(block_1));
  function.blocks.push_back(std::move(block_2));
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

int check_guard_lane_requires_authoritative_prepared_branch_record(const bir::Module& module,
                                                                   const char* function_name,
                                                                   const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->blocks.empty() ||
      control_flow->branch_conditions.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local-slot guard prepared control-flow contract")
                    .c_str());
  }

  control_flow->branch_conditions.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly reopened the raw compare-driven local-slot guard carrier after the prepared branch record was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing prepared branch record with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_guard_lane_consumes_prepared_branch_contract(const bir::Module& module,
                                                       const std::string& expected_asm,
                                                       const char* function_name,
                                                       const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.empty() ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local-slot guard prepared branch contract")
                    .c_str());
  }

  auto* entry_block = find_block(prepared.module.functions.front(), "entry");
  if (entry_block == nullptr || entry_block->terminator.kind != bir::TerminatorKind::CondBranch ||
      entry_block->insts.empty()) {
    return fail((std::string(failure_context) +
                 ": prepared local-slot guard fixture no longer has the expected entry shape")
                    .c_str());
  }
  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.back());
  if (entry_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared local-slot guard fixture no longer exposes the bounded compare carrier")
                    .c_str());
  }

  const std::string original_true_label = entry_block->terminator.true_label;
  const std::string original_false_label = entry_block->terminator.false_label;
  entry_block->terminator.condition.name = "contract.local_slot.guard.condition";
  entry_block->terminator.true_label = original_false_label;
  entry_block->terminator.false_label = original_true_label;
  entry_compare->opcode = bir::BinaryOpcode::Eq;
  entry_compare->lhs = bir::Value::immediate_i32(7);
  entry_compare->rhs = bir::Value::immediate_i32(9);

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the drifted local-slot guard carriers with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative local-slot prepared branch contract over raw entry-label carrier drift")
                    .c_str());
  }

  return 0;
}

int check_branch_lane_requires_authoritative_prepared_block_record(const bir::Module& module,
                                                                   const char* function_name,
                                                                   const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->blocks.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local-slot passthrough control-flow contract")
                    .c_str());
  }

  const auto entry_label_id = prepare::resolve_prepared_block_label_id(prepared.names, "entry");
  if (!entry_label_id.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared local-slot passthrough fixture no longer interns the entry block label")
                    .c_str());
  }

  const auto original_size = control_flow->blocks.size();
  control_flow->blocks.erase(
      std::remove_if(control_flow->blocks.begin(),
                     control_flow->blocks.end(),
                     [&](const prepare::PreparedControlFlowBlock& block) {
                       return block.block_label == *entry_label_id;
                     }),
      control_flow->blocks.end());
  if (control_flow->blocks.size() != original_size - 1) {
    return fail((std::string(failure_context) +
                 ": prepared local-slot passthrough fixture no longer exposes the entry branch carrier control-flow block")
                    .c_str());
  }

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly reopened the raw local-slot branch carrier after the prepared control-flow block was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing local-slot branch control-flow block with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_branch_lane_consumes_prepared_block_contract(const bir::Module& module,
                                                       const std::string& expected_asm,
                                                       const char* function_name,
                                                       const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->blocks.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local-slot passthrough control-flow contract")
                    .c_str());
  }

  auto* entry_block = find_block(prepared.module.functions.front(), "entry");
  if (entry_block == nullptr || entry_block->terminator.kind != bir::TerminatorKind::Branch) {
    return fail((std::string(failure_context) +
                 ": prepared local-slot passthrough fixture no longer has the expected branch entry")
                    .c_str());
  }

  const auto entry_label_id = prepare::resolve_prepared_block_label_id(prepared.names, "entry");
  const auto guard_label_id = prepare::resolve_prepared_block_label_id(prepared.names, "guard");
  if (!entry_label_id.has_value() || !guard_label_id.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared local-slot passthrough fixture no longer interns the expected block labels")
                    .c_str());
  }

  const auto* prepared_entry_block =
      prepare::find_prepared_control_flow_block(*control_flow, *entry_label_id);
  if (prepared_entry_block == nullptr ||
      prepared_entry_block->branch_target_label != *guard_label_id) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the authoritative passthrough branch target contract")
                    .c_str());
  }

  entry_block->terminator.target_label = "block_1";

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the drifted local-slot passthrough carrier with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared passthrough target over raw branch drift")
                    .c_str());
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_local_slot_guard_lane_tests() {
  // Keep the residual local add/sub/address guard lane isolated so Step 5.2
  // can finish shrinking the monolithic handoff file without widening into a
  // generic helper rewrite.
  if (const auto status =
          check_route_outputs(make_x86_local_i32_add_chain_guard_module(),
                              expected_minimal_local_i32_add_chain_guard_asm("main"),
                              "%t10 = bir.add i32 %t6, %t9",
                              "minimal local-slot add-chain guard route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_guard_lane_requires_authoritative_prepared_branch_record(
              make_x86_local_i32_add_chain_guard_module(),
              "main",
              "minimal local-slot add-chain guard route rejects reopening the raw compare-driven fallback when prepared control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_guard_lane_consumes_prepared_branch_contract(
              make_x86_local_i32_add_chain_guard_module(),
              expected_minimal_local_i32_add_chain_guard_asm("main"),
              "main",
              "minimal local-slot add-chain guard route follows authoritative prepared branch labels over drifted raw entry carriers");
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
          check_guard_lane_consumes_prepared_branch_contract(
              make_x86_local_i32_sub_guard_module(),
              expected_minimal_local_i32_sub_guard_asm("main"),
              "main",
              "minimal local-slot sub guard route follows authoritative prepared branch labels over drifted raw entry carriers");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_outputs(make_x86_local_i32_branch_passthrough_module(),
                              expected_minimal_local_i32_branch_passthrough_asm("main"),
                              "bir.store_local %lv.branch.0, i32 5",
                              "minimal local-slot single-successor passthrough route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_branch_lane_requires_authoritative_prepared_block_record(
              make_x86_local_i32_branch_passthrough_module(),
              "main",
              "minimal local-slot single-successor passthrough route rejects reopening the raw branch carrier when prepared control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_branch_lane_consumes_prepared_block_contract(
              make_x86_local_i32_branch_passthrough_module(),
              expected_minimal_local_i32_branch_passthrough_asm("main"),
              "main",
              "minimal local-slot single-successor passthrough route follows authoritative prepared branch targets over drifted raw entry labels");
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
          check_guard_lane_requires_authoritative_prepared_branch_record(
              make_x86_local_i8_address_guard_module(),
              "main",
              "minimal local-slot byte addressed-guard route rejects reopening the raw compare-driven fallback when prepared control-flow ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_guard_lane_consumes_prepared_branch_contract(
              make_x86_local_i8_address_guard_module(),
              expected_minimal_local_i8_address_guard_asm("main", 2),
              "main",
              "minimal local-slot byte addressed-guard route follows authoritative prepared branch labels over drifted raw entry carriers");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_outputs(make_x86_local_i8_copy_guard_module(),
                              expected_minimal_local_i8_copy_guard_asm("main", 2),
                              "bir.store_local %lv.arr.6, i8 ",
                              "minimal local-slot byte copy guard route");
      status != 0) {
    return status;
  }

  return 0;
}
