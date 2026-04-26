#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <cstdlib>
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

c4c::FunctionNameId find_function_name_id(const prepare::PreparedBirModule& prepared,
                                          std::string_view function_name) {
  return prepared.names.function_names.find(function_name);
}

c4c::BlockLabelId find_block_label_id(const prepare::PreparedBirModule& prepared,
                                      std::string_view block_label) {
  return prepared.names.block_labels.find(block_label);
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
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

int check_local_i16_guard_route_requires_authoritative_prepared_branch_labels(
    const bir::Module& module,
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
                 ": prepare no longer publishes the local i16 guard prepared branch contract")
                    .c_str());
  }

  auto mutated = prepared;
  auto* mutable_control_flow = &mutated.control_flow.functions.front();
  mutable_control_flow->branch_conditions.front().false_label =
      mutated.names.block_labels.intern("drifted.i16.guard.false");

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(mutated);
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

int check_local_i16_guard_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->blocks.empty() ||
      control_flow->branch_conditions.size() != 1 || !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local i16 guard prepared branch contract")
                    .c_str());
  }

  control_flow->branch_conditions.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly reopened the raw i16 guard matcher after the prepared branch record was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing prepared i16 branch record with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_local_i16_i64_sub_return_route_consumes_prepared_frame_access_contract(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* function_addressing =
      prepare::find_prepared_addressing(prepared, find_function_name_id(prepared, function_name));
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the i16/i64 subtract return prepared frame-slot accesses")
                    .c_str());
  }
  const auto* short_store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 0);
  const auto* long_store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 1);
  const auto* long_load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 2);
  const auto* short_load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 3);
  const auto* short_store_result_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 7);
  const auto* short_load_result_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 8);
  if (short_store_access == nullptr || long_store_access == nullptr || long_load_access == nullptr ||
      short_load_access == nullptr || short_store_result_access == nullptr ||
      short_load_result_access == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared i16/i64 subtract return fixture lost one of the expected direct frame-slot accesses")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  if (function.blocks.size() != 1 || function.blocks.front().insts.size() < 9) {
    return fail((std::string(failure_context) +
                 ": prepared i16/i64 subtract return fixture no longer exposes the expected single-block carriers")
                    .c_str());
  }
  auto& entry = function.blocks.front();
  auto* store_short = std::get_if<bir::StoreLocalInst>(&entry.insts[0]);
  auto* store_long = std::get_if<bir::StoreLocalInst>(&entry.insts[1]);
  auto* load_long = std::get_if<bir::LoadLocalInst>(&entry.insts[2]);
  auto* load_short = std::get_if<bir::LoadLocalInst>(&entry.insts[3]);
  auto* store_result = std::get_if<bir::StoreLocalInst>(&entry.insts[7]);
  auto* load_result = std::get_if<bir::LoadLocalInst>(&entry.insts[8]);
  if (store_short == nullptr || store_long == nullptr || load_long == nullptr ||
      load_short == nullptr || store_result == nullptr || load_result == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared i16/i64 subtract return fixture no longer exposes the expected raw local-slot carriers")
                    .c_str());
  }

  store_short->slot_name = "%drifted.short.store";
  store_long->slot_name = "%drifted.long.store";
  load_long->slot_name = "%drifted.long.load";
  load_short->slot_name = "%drifted.short.load";
  store_result->slot_name = "%drifted.short.result.store";
  load_result->slot_name = "%drifted.short.result.load";

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative prepared frame-slot accesses over drifted local slot carriers")
                    .c_str());
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_local_i16_guard_tests() {
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
          check_local_i16_guard_route_requires_authoritative_prepared_branch_record(
              make_x86_local_i16_increment_guard_module(),
              "main",
              "minimal local-slot i16 increment guard prepared branch-contract ownership rejects reopening the raw guard matcher when the prepared branch record is missing");
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
          check_local_i16_i64_sub_return_route_consumes_prepared_frame_access_contract(
              make_x86_local_i16_i64_sub_return_module(),
              expected_minimal_local_i16_i64_sub_return_asm("main"),
              "main",
              "minimal local-slot i16/i64 subtract return prepared frame-slot access ownership");
      status != 0) {
    return status;
  }

  return 0;
}
