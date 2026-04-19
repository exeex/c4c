#include "src/backend/backend.hpp"
#include "src/backend/bir/bir_printer.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"
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

  return 0;
}
