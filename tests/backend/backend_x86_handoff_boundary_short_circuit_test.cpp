#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/x86/api/api.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
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

std::string_view block_label(const prepare::PreparedBirModule& prepared,
                             c4c::BlockLabelId label) {
  return prepare::prepared_block_label(prepared.names, label);
}

c4c::BlockLabelId intern_block_label(prepare::PreparedBirModule& prepared,
                                     std::string_view label) {
  return prepared.names.block_labels.intern(label);
}

c4c::BlockLabelId find_block_label_id(const prepare::PreparedBirModule& prepared,
                                      std::string_view label) {
  return prepared.names.block_labels.find(label);
}

c4c::SlotNameId intern_slot_name(prepare::PreparedBirModule& prepared,
                                 std::string_view name) {
  return prepared.names.slot_names.intern(name);
}

int convert_short_circuit_join_to_edge_store_slot(
    prepare::PreparedBirModule& prepared,
    prepare::PreparedControlFlowFunction& control_flow,
    bir::Function& function,
    bir::Block& join_block,
    prepare::PreparedBranchCondition& join_branch_condition,
    const char* failure_context) {
  if (control_flow.join_transfers.size() != 1 || join_block.insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the mutable EdgeStoreSlot join contract")
                    .c_str());
  }

  auto* join_compare = std::get_if<bir::BinaryInst>(&join_block.insts.back());
  if (join_compare == nullptr || !join_branch_condition.lhs.has_value() ||
      !join_branch_condition.rhs.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the expected EdgeStoreSlot join carriers")
                    .c_str());
  }

  auto& join_transfer = control_flow.join_transfers.front();
  if (join_transfer.edge_transfers.size() != 2 ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer no longer exposes both EdgeStoreSlot lanes")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer published invalid EdgeStoreSlot lane indices")
                    .c_str());
  }

  join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
  join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
  join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
  join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;

  function.local_slots.push_back(bir::LocalSlot{
      .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = false,
  });

  const auto load_result =
      bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.edge_store_slot.join");
  join_block.insts.front() = bir::LoadLocalInst{
      .result = load_result,
      .slot_name =
          std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
  };
  if (join_compare->lhs.kind == bir::Value::Kind::Named &&
      join_compare->lhs.name == join_transfer.result.name) {
    join_compare->lhs = load_result;
  }
  if (join_compare->rhs.kind == bir::Value::Kind::Named &&
      join_compare->rhs.name == join_transfer.result.name) {
    join_compare->rhs = load_result;
  }
  if (join_branch_condition.lhs->kind == bir::Value::Kind::Named &&
      join_branch_condition.lhs->name == join_transfer.result.name) {
    *join_branch_condition.lhs = load_result;
  }
  if (join_branch_condition.rhs->kind == bir::Value::Kind::Named &&
      join_branch_condition.rhs->name == join_transfer.result.name) {
    *join_branch_condition.rhs = load_result;
  }

  return 0;
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
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

std::string expected_minimal_local_i8_short_circuit_or_guard_asm(const char* function_name) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov BYTE PTR [rsp], 1\n"
         "    mov BYTE PTR [rsp], 3\n"
         "    movsx eax, BYTE PTR [rsp]\n"
         "    cmp al, 3\n"
         "    je .L" + function_name + "_logic.rhs.7\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_logic.rhs.7:\n"
         "    movsx eax, BYTE PTR [rsp]\n"
         "    cmp al, 3\n"
         "    je .L" + function_name + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
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

bir::Module make_x86_local_i8_short_circuit_or_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.u.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.addr",
      .value = bir::Value::immediate_i8(1),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t1.addr",
      .value = bir::Value::immediate_i8(3),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%t3"),
      .slot_name = "%t3.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .operand_type = bir::TypeKind::I8,
      .lhs = bir::Value::named(bir::TypeKind::I8, "%t3"),
      .rhs = bir::Value::immediate_i8(3),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .true_label = "logic.skip.8",
      .false_label = "logic.rhs.7",
  };

  bir::Block logic_rhs;
  logic_rhs.label = "logic.rhs.7";
  logic_rhs.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%t12"),
      .slot_name = "%t12.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  logic_rhs.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t13"),
      .operand_type = bir::TypeKind::I8,
      .lhs = bir::Value::named(bir::TypeKind::I8, "%t12"),
      .rhs = bir::Value::immediate_i8(3),
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
      .compare_type = bir::TypeKind::I8,
      .lhs = bir::Value::named(bir::TypeKind::I8, "%t3"),
      .rhs = bir::Value::immediate_i8(3),
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

int check_local_i32_guard_route_rejects_authoritative_join_contract(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.empty() ||
      control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit join ownership needed for the fallback rejection")
                    .c_str());
  }
  if (!prepare::find_authoritative_branch_owned_join_transfer(
           prepared.names, *control_flow, "entry")
           .has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes authoritative entry-owned join ownership")
                    .c_str());
  }

  auto* entry_branch_condition = static_cast<prepare::PreparedBranchCondition*>(nullptr);
  for (auto& branch_condition : control_flow->branch_conditions) {
    if (block_label(prepared, branch_condition.block_label) == "entry") {
      entry_branch_condition = &branch_condition;
      break;
    }
  }
  if (entry_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative entry branch condition")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  if (function.local_slots.empty()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the local-slot carrier needed for the guard fallback probe")
                    .c_str());
  }

  const std::string slot_name = function.local_slots.front().name;
  function.blocks.clear();

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = slot_name,
      .value = bir::Value::immediate_i32(123),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .slot_name = slot_name,
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

  entry_branch_condition->kind = prepare::PreparedBranchConditionKind::FusedCompare;
  entry_branch_condition->condition_value = bir::Value::named(bir::TypeKind::I32, "cmp");
  entry_branch_condition->predicate = bir::BinaryOpcode::Ne;
  entry_branch_condition->compare_type = bir::TypeKind::I32;
  entry_branch_condition->lhs = bir::Value::named(bir::TypeKind::I32, "loaded");
  entry_branch_condition->rhs = bir::Value::immediate_i32(123);
  entry_branch_condition->can_fuse_with_branch = true;
  entry_branch_condition->true_label = intern_block_label(prepared, "block_1");
  entry_branch_condition->false_label = intern_block_label(prepared, "block_2");

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a local guard fallback while authoritative join ownership remained active")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the authoritative join-owned guard fallback with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_requires_authoritative_parallel_copy_bundles(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2 ||
      control_flow->join_transfers.size() != 1 || control_flow->parallel_copy_bundles.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit out-of-SSA handoff contract")
                    .c_str());
  }
  if (!prepare::find_authoritative_branch_owned_join_transfer(
           prepared.names, *control_flow, "entry")
           .has_value()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes authoritative short-circuit join ownership before bundle stripping")
                    .c_str());
  }

  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->branch_conditions.size() < 2 ||
      mutable_control_flow->join_transfers.size() != 1 ||
      mutable_control_flow->parallel_copy_bundles.empty()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture lost its mutable out-of-SSA handoff contract")
                    .c_str());
  }

  mutable_control_flow->parallel_copy_bundles.clear();
  if (mutable_control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture stopped publishing join transfers while stripping parallel-copy bundles")
                    .c_str());
  }

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted short-circuit phi edge obligations after authoritative parallel-copy publication was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected missing short-circuit parallel-copy publication with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_regalloc_consumes_published_parallel_copy_execution_sites(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  const auto* function_locations = find_value_location_function(prepared, function_name);
  if (control_flow == nullptr || function_locations == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit regalloc/value-location contract")
                    .c_str());
  }

  const auto authoritative_bundles =
      prepare::find_authoritative_branch_owned_parallel_copy_bundles(
          prepared.names,
          *control_flow,
          "entry",
          prepare::PreparedJoinTransferKind::PhiEdge);
  if (!authoritative_bundles.has_value() ||
      authoritative_bundles->true_bundle == nullptr ||
      authoritative_bundles->false_bundle == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes authoritative branch-owned short-circuit bundles")
                    .c_str());
  }

  for (const auto* bundle : {authoritative_bundles->true_bundle,
                             authoritative_bundles->false_bundle}) {
    if (bundle->execution_site !=
        prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
      return fail((std::string(failure_context) +
                   ": prepare no longer classifies branch-owned short-circuit bundles as published edge-block executable")
                      .c_str());
    }

    const auto execution_block_index =
        prepare::published_prepared_parallel_copy_execution_block_index(
            prepared.names, prepared.module.functions.front(), *bundle);
    if (!execution_block_index.has_value()) {
      return fail((std::string(failure_context) +
                   ": prepare no longer publishes short-circuit parallel-copy execution blocks")
                      .c_str());
    }

    const auto* move_bundle = prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
        prepared.names, prepared.module.functions.front(), *function_locations, *bundle);
    if (move_bundle == nullptr) {
      return fail((std::string(failure_context) +
                   ": regalloc stopped placing short-circuit bundles at the published execution site")
                      .c_str());
    }
    if (move_bundle->block_index != *execution_block_index) {
      return fail((std::string(failure_context) +
                   ": regalloc drifted from the published short-circuit parallel-copy execution block")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_rejects_drifted_branch_plan_continuation_targets(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit continuation branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* rhs_block = find_block(function, "logic.rhs.7");
  if (rhs_block == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected rhs compare block")
                    .c_str());
  }

  auto* rhs_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    const auto rhs_block_label = find_block_label_id(prepared, rhs_block->label);
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (branch_condition.block_label == rhs_block_label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (rhs_branch_condition == nullptr ||
      !rhs_branch_condition->predicate.has_value() ||
      !rhs_branch_condition->compare_type.has_value() ||
      !rhs_branch_condition->lhs.has_value() ||
      !rhs_branch_condition->rhs.has_value() ||
      rhs_branch_condition->true_label == rhs_branch_condition->false_label) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes rhs continuation compare metadata")
                    .c_str());
  }

  std::swap(rhs_branch_condition->true_label, rhs_branch_condition->false_label);

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted drifted prepared branch-plan continuation targets")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    const std::string_view message(error.what());
    if (message.find("canonical prepared-module handoff") == std::string_view::npos ||
        message.find("prepared branch condition targets drifted from prepared block targets") ==
            std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected drifted prepared branch-plan continuation targets with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_rejects_missing_prepared_local_frame_slot(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  if (find_control_flow_function(prepared, function_name) == nullptr ||
      find_value_location_function(prepared, function_name) == nullptr ||
      prepared.stack_layout.frame_slots.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local-slot short-circuit frame/value-location contract")
                    .c_str());
  }

  bool saw_prepared_frame_slot_access = false;
  for (const auto& function_addressing : prepared.addressing.functions) {
    for (const auto& access : function_addressing.accesses) {
      if (access.address.base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
          access.address.frame_slot_id.has_value()) {
        saw_prepared_frame_slot_access = true;
        break;
      }
    }
  }
  if (!saw_prepared_frame_slot_access) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes local-slot short-circuit frame-slot accesses")
                    .c_str());
  }

  prepared.stack_layout.frame_slots.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted local-slot short-circuit rendering without prepared frame-slot authority")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    const std::string_view message(error.what());
    if (message.find("canonical prepared-module handoff") == std::string_view::npos ||
        message.find("prepared frame slot") == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected missing local frame-slot authority with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_consumes_prepared_control_flow_impl(const bir::Module& module,
                                                                  const char* function_name,
                                                                  const char* failure_context,
                                                                  bool add_rhs_passthrough_block);

int check_short_circuit_route_consumes_prepared_control_flow(const bir::Module& module,
                                                             const char* function_name,
                                                             const char* failure_context) {
  return check_short_circuit_route_consumes_prepared_control_flow_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_route_with_rhs_passthrough_consumes_prepared_control_flow(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_consumes_prepared_control_flow_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_prefers_prepared_passthrough_branch_target_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit continuation/control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* rhs_end_block = find_block(function, "logic.rhs.end.9");
  if (rhs_end_block == nullptr ||
      rhs_end_block->terminator.kind != bir::TerminatorKind::Branch) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected rhs passthrough branch")
                    .c_str());
  }
  if (use_edge_store_slot_carrier) {
    auto* join_block = find_block(function, "logic.end.10");
    if (join_block == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit fixture no longer has the expected EdgeStoreSlot join block")
                      .c_str());
    }
    auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
      for (auto& branch_condition : control_flow->branch_conditions) {
        if (block_label(prepared, branch_condition.block_label) == join_block->label) {
          return &branch_condition;
        }
      }
      return nullptr;
    }();
    if (join_branch_condition == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit fixture no longer exposes the EdgeStoreSlot join metadata")
                      .c_str());
    }
    if (const auto status = convert_short_circuit_join_to_edge_store_slot(
            prepared, *control_flow, function, *join_block, *join_branch_condition, failure_context);
        status != 0) {
      return status;
    }
  }

  const auto rhs_end_label_id = find_block_label_id(prepared, rhs_end_block->label);
  const auto* rhs_end_control_flow =
      rhs_end_label_id == c4c::kInvalidBlockLabel
          ? nullptr
          : prepare::find_prepared_control_flow_block(*control_flow, rhs_end_label_id);
  if (rhs_end_control_flow == nullptr ||
      rhs_end_control_flow->terminator_kind != bir::TerminatorKind::Branch ||
      block_label(prepared, rhs_end_control_flow->branch_target_label) != "logic.end.10") {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative rhs passthrough branch target")
                    .c_str());
  }

  rhs_end_block->terminator.target_label = "contract.raw.rhs.end.drift";
  function.blocks.push_back(bir::Block{
      .label = "contract.raw.rhs.end.drift",
      .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(77)},
  });

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_minimal_local_i32_short_circuit_or_guard_asm(function_name)) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped preferring the authoritative rhs passthrough branch target over raw branch drift")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_route_prefers_prepared_passthrough_branch_target(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_prefers_prepared_passthrough_branch_target_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_prefers_prepared_passthrough_branch_target(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_prefers_prepared_passthrough_branch_target_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_ignores_rhs_compare_carrier_state_when_prepared_control_flow_is_authoritative(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit continuation branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* rhs_block = find_block(function, "logic.rhs.7");
  if (rhs_block == nullptr || rhs_block->insts.empty()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected rhs compare block")
                    .c_str());
  }

  const auto* rhs_branch_condition =
      prepare::find_prepared_branch_condition(
          *control_flow, find_block_label_id(prepared, rhs_block->label));
  if (rhs_branch_condition == nullptr || !rhs_branch_condition->predicate.has_value() ||
      !rhs_branch_condition->lhs.has_value() || !rhs_branch_condition->rhs.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes authoritative rhs continuation compare metadata")
                    .c_str());
  }
  if (*rhs_branch_condition->predicate != bir::BinaryOpcode::Ne ||
      rhs_branch_condition->lhs->kind != bir::Value::Kind::Named ||
      rhs_branch_condition->lhs->name != "%t12" ||
      rhs_branch_condition->rhs->kind != bir::Value::Kind::Immediate ||
      rhs_branch_condition->rhs->immediate != 3 ||
      block_label(prepared, rhs_branch_condition->true_label) != "block_1" ||
      block_label(prepared, rhs_branch_condition->false_label) != "block_2") {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing the authoritative rhs continuation compare semantics")
                    .c_str());
  }

  auto* rhs_compare = std::get_if<bir::BinaryInst>(&rhs_block->insts.back());
  if (rhs_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the expected rhs compare carrier")
                    .c_str());
  }
  *rhs_compare = bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "contract.rhs.compare.carrier"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(7),
      .rhs = bir::Value::immediate_i32(7),
  };

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_minimal_local_i32_short_circuit_or_guard_asm(function_name)) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped preferring the prepared rhs continuation compare contract")
                    .c_str());
  }
  return 0;
}

int check_short_circuit_route_requires_authoritative_rhs_continuation_compare_carrier_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit continuation branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* rhs_block = find_block(function, "logic.rhs.7");
  auto* join_block = find_block(function, "logic.end.10");
  if (rhs_block == nullptr || rhs_block->insts.size() < 2 || join_block == nullptr ||
      join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected rhs/join shape")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
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

  if (use_edge_store_slot_carrier) {
    if (!join_transfer.source_true_transfer_index.has_value() ||
        !join_transfer.source_false_transfer_index.has_value()) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit join transfer no longer exposes authoritative lane ownership")
                      .c_str());
    }
    const auto true_transfer_index = *join_transfer.source_true_transfer_index;
    const auto false_transfer_index = *join_transfer.source_false_transfer_index;
    if (true_transfer_index >= join_transfer.edge_transfers.size() ||
        false_transfer_index >= join_transfer.edge_transfers.size()) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit join transfer published invalid authoritative lane indices")
                      .c_str());
    }

    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    const auto load_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_block->insts.front() = bir::LoadLocalInst{
        .result = load_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    if (join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == join_transfer.result.name) {
      *join_branch_condition->lhs = load_result;
    }
    if (join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == join_transfer.result.name) {
      *join_branch_condition->rhs = load_result;
    }
  }

  const auto* rhs_branch_condition =
      prepare::find_prepared_branch_condition(
          *control_flow, find_block_label_id(prepared, rhs_block->label));
  if (rhs_branch_condition == nullptr || !rhs_branch_condition->predicate.has_value() ||
      !rhs_branch_condition->lhs.has_value() || !rhs_branch_condition->rhs.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes authoritative rhs continuation compare metadata")
                    .c_str());
  }

  rhs_block->insts.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a rhs branch carrier with authoritative continuation ownership but no compare carrier")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing rhs continuation compare carrier with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_requires_authoritative_rhs_continuation_compare_carrier(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_requires_authoritative_rhs_continuation_compare_carrier_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_requires_authoritative_rhs_continuation_compare_carrier(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_requires_authoritative_rhs_continuation_compare_carrier_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_consumes_prepared_control_flow_impl(const bir::Module& module,
                                                                  const char* function_name,
                                                                  const char* failure_context,
                                                                  bool add_rhs_passthrough_block) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2 ||
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
      mutable_control_flow->branch_conditions.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture lost its mutable join-transfer contract")
                    .c_str());
  }
  auto& join_transfer = mutable_control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : mutable_control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
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

  const std::string original_true_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
  const std::string original_false_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
  join_transfer.source_true_incoming_label = intern_block_label(prepared, "contract.short_circuit");
  join_transfer.source_false_incoming_label = intern_block_label(prepared, "contract.rhs");
  join_transfer.edge_transfers[false_transfer_index].incoming_value = bir::Value::immediate_i32(9);
  bool rewrote_short_circuit_label = false;
  bool rewrote_rhs_label = false;
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == original_true_incoming_label) {
      incoming.label = std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
      rewrote_short_circuit_label = true;
      continue;
    }
    if (incoming.label == original_false_incoming_label) {
      incoming.label =
          std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
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
  entry_block->terminator.condition = bir::Value::named(bir::TypeKind::I32, "contract.entry.condition");
  entry_block->insts.back() = bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "contract.entry.carrier"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(4),
      .rhs = bir::Value::immediate_i32(8),
  };
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
  if (add_rhs_passthrough_block) {
    auto* rhs_block = find_block(function, "logic.rhs.7");
    if (rhs_block == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit fixture no longer has the expected rhs compare block")
                      .c_str());
    }
    rhs_block->terminator.target_label = "contract.rhs.bridge";
    function.blocks.push_back(bir::Block{
        .label = "contract.rhs.bridge",
        .terminator = bir::BranchTerminator{.target_label = "contract.rhs"},
    });
  }

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_minimal_local_i32_short_circuit_or_guard_asm(function_name)) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative short-circuit metadata over carrier labels")
                    .c_str());
  }
  return 0;
}

int check_short_circuit_context_publishes_prepared_continuation_labels_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* join_block = find_block(function, "logic.end.10");
  if (join_block == nullptr || join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected join block")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (join_branch_condition == nullptr || !join_branch_condition->lhs.has_value() ||
      !join_branch_condition->rhs.has_value() ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value() ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative join metadata")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer published invalid transfer ownership indices")
                    .c_str());
  }

  const std::string original_true_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
  const std::string original_false_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
  join_transfer.source_true_incoming_label = intern_block_label(prepared, "contract.short_circuit");
  join_transfer.source_false_incoming_label = intern_block_label(prepared, "contract.rhs");
  join_transfer.edge_transfers[false_transfer_index].incoming_value = bir::Value::immediate_i32(9);
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == original_true_incoming_label) {
      incoming.label = std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
    } else if (incoming.label == original_false_incoming_label) {
      incoming.label =
          std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
      incoming.value = bir::Value::immediate_i32(9);
    }
  }

  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    const auto load_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_block->insts.front() = bir::LoadLocalInst{
        .result = load_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    if (join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == join_transfer.result.name) {
      *join_branch_condition->lhs = load_result;
    }
    if (join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == join_transfer.result.name) {
      *join_branch_condition->rhs = load_result;
    }
  } else {
    auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts.front());
    if (join_select == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit fixture no longer exposes the expected select carrier")
                      .c_str());
    }
    const std::string original_result_name = join_select->result.name;
    const auto rewritten_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_select->result = rewritten_result;
    if (join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == original_result_name) {
      *join_branch_condition->lhs = rewritten_result;
    }
    if (join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == original_result_name) {
      *join_branch_condition->rhs = rewritten_result;
    }
  }

  join_block->terminator.true_label = "carrier.join.true";
  join_block->terminator.false_label = "carrier.join.false";
  join_branch_condition->true_label = intern_block_label(prepared, join_block->terminator.true_label);
  join_branch_condition->false_label =
      intern_block_label(prepared, join_block->terminator.false_label);
  join_branch_condition->predicate = bir::BinaryOpcode::Eq;

  const auto prepared_context =
      prepare::find_prepared_short_circuit_join_context(
          prepared.names, *control_flow, function, "entry");
  if (!prepared_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the authoritative short-circuit join context")
                    .c_str());
  }
  if (prepared_context->join_block != join_block ||
      prepared_context->true_transfer != &join_transfer.edge_transfers[true_transfer_index] ||
      prepared_context->false_transfer != &join_transfer.edge_transfers[false_transfer_index]) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped returning the authoritative short-circuit join ownership")
                    .c_str());
  }
  if (!prepared_context->classified_incoming.short_circuit_on_compare_true ||
      !prepared_context->classified_incoming.short_circuit_value ||
      block_label(prepared, prepared_context->continuation_true_label) != "block_1" ||
      block_label(prepared, prepared_context->continuation_false_label) != "block_2") {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing the authoritative short-circuit continuation labels")
                    .c_str());
  }

  const auto prepared_compare_join_targets =
      prepare::find_prepared_compare_join_continuation_targets(
          prepared.names, *control_flow, function, "entry");
  if (!prepared_compare_join_targets.has_value() ||
      prepared_compare_join_targets->true_label != prepared_context->continuation_true_label ||
      prepared_compare_join_targets->false_label != prepared_context->continuation_false_label) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing authoritative compare-join continuation targets")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_context_publishes_prepared_continuation_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_context_publishes_prepared_continuation_labels_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_context_publishes_prepared_continuation_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_context_publishes_prepared_continuation_labels_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_context_prefers_prepared_continuation_branch_condition_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_join) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit continuation branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* join_block = find_block(function, "logic.end.10");
  auto* rhs_block = find_block(function, "logic.rhs.7");
  if (join_block == nullptr || rhs_block == nullptr || join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected rhs/join shape")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* rhs_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == rhs_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (join_branch_condition == nullptr || rhs_branch_condition == nullptr ||
      !rhs_branch_condition->lhs.has_value() || !rhs_branch_condition->rhs.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative rhs/join compare metadata")
                    .c_str());
  }
  if (!join_transfer.continuation_true_label.has_value() ||
      !join_transfer.continuation_false_label.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer no longer publishes authoritative continuation labels")
                    .c_str());
  }

  const std::string expected_true_label =
      std::string(block_label(prepared, *join_transfer.continuation_true_label));
  const std::string expected_false_label =
      std::string(block_label(prepared, *join_transfer.continuation_false_label));
  if (block_label(prepared, rhs_branch_condition->true_label) != expected_true_label ||
      block_label(prepared, rhs_branch_condition->false_label) != expected_false_label) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer keeps rhs continuation metadata aligned with published join-transfer labels")
                    .c_str());
  }

  if (use_edge_store_slot_join) {
    if (join_transfer.edge_transfers.size() != 2 ||
        !join_transfer.source_true_transfer_index.has_value() ||
        !join_transfer.source_false_transfer_index.has_value()) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit join transfer no longer exposes both authoritative lanes")
                      .c_str());
    }
    const auto true_transfer_index = *join_transfer.source_true_transfer_index;
    const auto false_transfer_index = *join_transfer.source_false_transfer_index;
    if (true_transfer_index >= join_transfer.edge_transfers.size() ||
        false_transfer_index >= join_transfer.edge_transfers.size()) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit join transfer published invalid authoritative lane indices")
                      .c_str());
    }

    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    const auto load_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_block->insts.front() = bir::LoadLocalInst{
        .result = load_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    if (join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == join_transfer.result.name) {
      *join_branch_condition->lhs = load_result;
    }
    if (join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == join_transfer.result.name) {
      *join_branch_condition->rhs = load_result;
    }
  }

  join_block->terminator.true_label = "carrier.join.misleading.true";
  join_block->terminator.false_label = "carrier.join.misleading.false";
  join_branch_condition->true_label = intern_block_label(prepared, join_block->terminator.true_label);
  join_branch_condition->false_label =
      intern_block_label(prepared, join_block->terminator.false_label);
  control_flow->branch_conditions.erase(
      std::remove_if(control_flow->branch_conditions.begin(),
                     control_flow->branch_conditions.end(),
                     [&](const prepare::PreparedBranchCondition& branch_condition) {
                       return block_label(prepared, branch_condition.block_label) != "entry";
                     }),
      control_flow->branch_conditions.end());

  const auto prepared_context =
      prepare::find_prepared_short_circuit_join_context(
          prepared.names, *control_flow, function, "entry");
  if (!prepared_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the short-circuit continuation contract once non-entry branch metadata is removed")
                    .c_str());
  }
  if (block_label(prepared, prepared_context->continuation_true_label) != expected_true_label ||
      block_label(prepared, prepared_context->continuation_false_label) != expected_false_label) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped preferring published continuation labels over join/rhs recomputation")
                    .c_str());
  }

  const auto prepared_compare_join_targets =
      prepare::find_prepared_compare_join_continuation_targets(
          prepared.names, *control_flow, function, "entry");
  if (!prepared_compare_join_targets.has_value() ||
      block_label(prepared, prepared_compare_join_targets->true_label) != expected_true_label ||
      block_label(prepared, prepared_compare_join_targets->false_label) != expected_false_label) {
    return fail((std::string(failure_context) +
                 ": shared compare-join helper stopped preferring published continuation labels over join/rhs recomputation")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_context_prefers_prepared_continuation_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_context_prefers_prepared_continuation_branch_condition_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_context_prefers_prepared_continuation_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_context_prefers_prepared_continuation_branch_condition_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_prefers_prepared_continuation_labels_when_join_branch_labels_drift_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit continuation branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* join_block = find_block(function, "logic.end.10");
  if (join_block == nullptr || join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected join block")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (join_branch_condition == nullptr || !join_branch_condition->lhs.has_value() ||
      !join_branch_condition->rhs.has_value() ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value() ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative join metadata")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer published invalid transfer ownership indices")
                    .c_str());
  }

  const std::string original_true_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
  const std::string original_false_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
  join_transfer.source_true_incoming_label = intern_block_label(prepared, "contract.short_circuit");
  join_transfer.source_false_incoming_label = intern_block_label(prepared, "contract.rhs");
  join_transfer.edge_transfers[false_transfer_index].incoming_value = bir::Value::immediate_i32(9);
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == original_true_incoming_label) {
      incoming.label = std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
    } else if (incoming.label == original_false_incoming_label) {
      incoming.label =
          std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
      incoming.value = bir::Value::immediate_i32(9);
    }
  }

  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    const auto load_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_block->insts.front() = bir::LoadLocalInst{
        .result = load_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    if (join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == join_transfer.result.name) {
      *join_branch_condition->lhs = load_result;
    }
    if (join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == join_transfer.result.name) {
      *join_branch_condition->rhs = load_result;
    }
  } else {
    auto* join_select = std::get_if<bir::SelectInst>(&join_block->insts.front());
    if (join_select == nullptr) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit fixture no longer exposes the expected select carrier")
                      .c_str());
    }
    const std::string original_result_name = join_select->result.name;
    const auto rewritten_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_select->result = rewritten_result;
    if (join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == original_result_name) {
      *join_branch_condition->lhs = rewritten_result;
    }
    if (join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == original_result_name) {
      *join_branch_condition->rhs = rewritten_result;
    }
  }

  join_block->terminator.true_label = "carrier.join.misleading.true";
  join_block->terminator.false_label = "carrier.join.misleading.false";
  join_branch_condition->true_label = intern_block_label(prepared, join_block->terminator.true_label);
  join_branch_condition->false_label =
      intern_block_label(prepared, join_block->terminator.false_label);
  join_branch_condition->predicate = bir::BinaryOpcode::Eq;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_minimal_local_i32_short_circuit_or_guard_asm(function_name)) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative rhs continuation labels when join-branch labels drift")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_route_prefers_prepared_continuation_labels_when_join_branch_labels_drift(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_prefers_prepared_continuation_labels_when_join_branch_labels_drift_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_prefers_prepared_continuation_labels_when_join_branch_labels_drift(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_prefers_prepared_continuation_labels_when_join_branch_labels_drift_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_requires_authoritative_rhs_prepared_branch_condition_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit continuation branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* join_block = find_block(function, "logic.end.10");
  auto* rhs_block = find_block(function, "logic.rhs.7");
  if (join_block == nullptr || rhs_block == nullptr || join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected rhs/join shape")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* rhs_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == rhs_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (join_branch_condition == nullptr || rhs_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the expected rhs/join branch metadata")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    if (join_transfer.edge_transfers.size() != 2 ||
        !join_transfer.source_true_transfer_index.has_value() ||
        !join_transfer.source_false_transfer_index.has_value()) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit join transfer no longer exposes both authoritative lanes")
                      .c_str());
    }
    const auto true_transfer_index = *join_transfer.source_true_transfer_index;
    const auto false_transfer_index = *join_transfer.source_false_transfer_index;
    if (true_transfer_index >= join_transfer.edge_transfers.size() ||
        false_transfer_index >= join_transfer.edge_transfers.size()) {
      return fail((std::string(failure_context) +
                   ": prepared short-circuit join transfer published invalid authoritative lane indices")
                      .c_str());
    }

    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    const auto load_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_block->insts.front() = bir::LoadLocalInst{
        .result = load_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    if (join_branch_condition->lhs.has_value() &&
        join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == join_transfer.result.name) {
      *join_branch_condition->lhs = load_result;
    }
    if (join_branch_condition->rhs.has_value() &&
        join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == join_transfer.result.name) {
      *join_branch_condition->rhs = load_result;
    }
  }

  rhs_branch_condition->predicate.reset();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a drifted rhs prepared branch contract")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the drifted rhs prepared branch contract with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_requires_authoritative_rhs_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_requires_authoritative_rhs_prepared_branch_condition_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_requires_authoritative_rhs_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_requires_authoritative_rhs_prepared_branch_condition_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_rejects_missing_authoritative_rhs_prepared_branch_condition_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit continuation branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* join_block = find_block(function, "logic.end.10");
  auto* rhs_block = find_block(function, "logic.rhs.7");
  if (join_block == nullptr || rhs_block == nullptr || join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected rhs/join shape")
                    .c_str());
  }

  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* rhs_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == rhs_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (join_branch_condition == nullptr || rhs_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the expected rhs/join branch metadata")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    if (const auto status = convert_short_circuit_join_to_edge_store_slot(
            prepared, *control_flow, function, *join_block, *join_branch_condition, failure_context);
        status != 0) {
      return status;
    }
  }

  const auto original_branch_condition_count = control_flow->branch_conditions.size();
  control_flow->branch_conditions.erase(
      std::remove_if(control_flow->branch_conditions.begin(),
                     control_flow->branch_conditions.end(),
                     [&](const prepare::PreparedBranchCondition& branch_condition) {
                       return block_label(prepared, branch_condition.block_label) ==
                              rhs_block->label;
                     }),
      control_flow->branch_conditions.end());
  const auto removed_count =
      original_branch_condition_count - control_flow->branch_conditions.size();
  if (removed_count != 1) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes exactly one authoritative rhs branch record")
                    .c_str());
  }

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a missing rhs prepared branch record")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing rhs prepared branch record with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_rejects_missing_authoritative_rhs_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_rejects_missing_authoritative_rhs_prepared_branch_condition_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_rejects_missing_authoritative_rhs_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_rejects_missing_authoritative_rhs_prepared_branch_condition_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_requires_authoritative_entry_prepared_branch_condition_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit entry-branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "logic.end.10");
  if (entry_block == nullptr || join_block == nullptr || join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry/join shape")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  auto* entry_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == entry_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (entry_branch_condition == nullptr || join_branch_condition == nullptr ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value() ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative entry/join metadata")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer published invalid authoritative indices")
                    .c_str());
  }

  const std::string original_true_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
  const std::string original_false_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
  join_transfer.source_true_incoming_label = intern_block_label(prepared, "contract.short_circuit");
  join_transfer.source_false_incoming_label = intern_block_label(prepared, "contract.rhs");
  join_transfer.edge_transfers[false_transfer_index].incoming_value = bir::Value::immediate_i32(9);
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == original_true_incoming_label) {
      incoming.label = std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
    } else if (incoming.label == original_false_incoming_label) {
      incoming.label =
          std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
      incoming.value = bir::Value::immediate_i32(9);
    }
  }

  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    const auto load_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_block->insts.front() = bir::LoadLocalInst{
        .result = load_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    if (join_branch_condition->lhs.has_value() &&
        join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == join_transfer.result.name) {
      *join_branch_condition->lhs = load_result;
    }
    if (join_branch_condition->rhs.has_value() &&
        join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == join_transfer.result.name) {
      *join_branch_condition->rhs = load_result;
    }
  }

  entry_branch_condition->predicate.reset();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a drifted short-circuit entry prepared branch contract")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the drifted short-circuit entry prepared branch contract with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_requires_authoritative_entry_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_requires_authoritative_entry_prepared_branch_condition_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_requires_authoritative_entry_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_requires_authoritative_entry_prepared_branch_condition_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_rejects_missing_authoritative_entry_prepared_branch_condition_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit entry-branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "logic.end.10");
  if (entry_block == nullptr || join_block == nullptr || join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry/join shape")
                    .c_str());
  }

  auto* entry_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == entry_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (entry_branch_condition == nullptr || join_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the expected entry/join branch metadata")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    if (const auto status = convert_short_circuit_join_to_edge_store_slot(
            prepared, *control_flow, function, *join_block, *join_branch_condition, failure_context);
        status != 0) {
      return status;
    }
  }

  const auto original_branch_condition_count = control_flow->branch_conditions.size();
  control_flow->branch_conditions.erase(
      std::remove_if(control_flow->branch_conditions.begin(),
                     control_flow->branch_conditions.end(),
                     [&](const prepare::PreparedBranchCondition& branch_condition) {
                       return block_label(prepared, branch_condition.block_label) ==
                              entry_block->label;
                     }),
      control_flow->branch_conditions.end());
  const auto removed_count =
      original_branch_condition_count - control_flow->branch_conditions.size();
  if (removed_count != 1) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes exactly one authoritative entry branch record")
                    .c_str());
  }

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted a missing entry prepared branch record")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing entry prepared branch record with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_rejects_missing_authoritative_entry_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_rejects_missing_authoritative_entry_prepared_branch_condition_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_rejects_missing_authoritative_entry_prepared_branch_condition(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_rejects_missing_authoritative_entry_prepared_branch_condition_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_requires_authoritative_entry_prepared_target_labels_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 3 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit entry target-label contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "logic.end.10");
  if (entry_block == nullptr || join_block == nullptr || join_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry/join shape")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  auto* entry_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == entry_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (entry_branch_condition == nullptr || join_branch_condition == nullptr ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value() ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the authoritative entry/join target metadata")
                    .c_str());
  }

  const auto true_transfer_index = *join_transfer.source_true_transfer_index;
  const auto false_transfer_index = *join_transfer.source_false_transfer_index;
  if (true_transfer_index >= join_transfer.edge_transfers.size() ||
      false_transfer_index >= join_transfer.edge_transfers.size() ||
      true_transfer_index == false_transfer_index) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer published invalid authoritative indices")
                    .c_str());
  }

  const std::string original_true_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
  const std::string original_false_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
  join_transfer.source_true_incoming_label = intern_block_label(prepared, "contract.short_circuit");
  join_transfer.source_false_incoming_label = intern_block_label(prepared, "contract.rhs");
  join_transfer.edge_transfers[false_transfer_index].incoming_value = bir::Value::immediate_i32(9);
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == original_true_incoming_label) {
      incoming.label = std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
    } else if (incoming.label == original_false_incoming_label) {
      incoming.label =
          std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
      incoming.value = bir::Value::immediate_i32(9);
    }
  }

  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    const auto load_result =
        bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.contract.join");
    join_block->insts.front() = bir::LoadLocalInst{
        .result = load_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
    };
    if (join_branch_condition->lhs.has_value() &&
        join_branch_condition->lhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->lhs->name == join_transfer.result.name) {
      *join_branch_condition->lhs = load_result;
    }
    if (join_branch_condition->rhs.has_value() &&
        join_branch_condition->rhs->kind == bir::Value::Kind::Named &&
        join_branch_condition->rhs->name == join_transfer.result.name) {
      *join_branch_condition->rhs = load_result;
    }
  }

  entry_block->terminator.true_label = "carrier.short_circuit";
  entry_block->terminator.false_label = "carrier.rhs";
  entry_branch_condition->true_label = intern_block_label(prepared, "contract.entry.misleading.true");
  entry_branch_condition->false_label =
      intern_block_label(prepared, "contract.entry.misleading.false");

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted drifted entry prepared target labels")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected drifted entry prepared target labels with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_short_circuit_route_requires_authoritative_entry_prepared_target_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_requires_authoritative_entry_prepared_target_labels_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_requires_authoritative_entry_prepared_target_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_requires_authoritative_entry_prepared_target_labels_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_entry_branch_helper_publishes_prepared_target_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit entry branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry block")
                    .c_str());
  }

  auto* entry_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == entry_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (entry_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes entry branch metadata")
                    .c_str());
  }

  entry_block->terminator.true_label = "carrier.short_circuit";
  entry_block->terminator.false_label = "carrier.rhs";
  entry_block->terminator.condition = bir::Value::named(bir::TypeKind::I32, "carrier.entry.condition");
  entry_branch_condition->true_label = intern_block_label(prepared, "logic.rhs.9");
  entry_branch_condition->false_label = intern_block_label(prepared, "logic.end.10");

  const auto prepared_targets = prepare::find_prepared_compare_branch_target_labels(
      prepared.names, *entry_branch_condition, *entry_block);
  if (!prepared_targets.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the prepared short-circuit entry branch contract")
                    .c_str());
  }
  if (block_label(prepared, prepared_targets->true_label) != "logic.rhs.9" ||
      block_label(prepared, prepared_targets->false_label) != "logic.end.10") {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing authoritative short-circuit entry branch labels")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_branch_plan_helper_publishes_prepared_labels_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit branch-plan contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "logic.end.10");
  if (entry_block == nullptr || join_block == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry/join blocks")
                    .c_str());
  }

  auto* entry_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == entry_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (entry_branch_condition == nullptr || join_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the expected branch metadata")
                    .c_str());
  }

  auto& join_transfer = control_flow->join_transfers.front();
  entry_block->terminator.true_label = "carrier.short_circuit";
  entry_block->terminator.false_label = "carrier.rhs";
  entry_block->terminator.condition = bir::Value::named(bir::TypeKind::I32, "carrier.entry.condition");
  entry_branch_condition->true_label = intern_block_label(prepared, "logic.rhs.9");
  entry_branch_condition->false_label = intern_block_label(prepared, "logic.end.10");
  join_block->terminator.true_label = "carrier.join.true";
  join_block->terminator.false_label = "carrier.join.false";
  join_branch_condition->true_label = intern_block_label(prepared, join_block->terminator.true_label);
  join_branch_condition->false_label =
      intern_block_label(prepared, join_block->terminator.false_label);
  join_branch_condition->predicate = bir::BinaryOpcode::Eq;
  if (use_edge_store_slot_carrier) {
    if (const auto status = convert_short_circuit_join_to_edge_store_slot(
            prepared, *control_flow, function, *join_block, *join_branch_condition, failure_context);
        status != 0) {
      return status;
    }
  }

  const auto prepared_join_context =
      prepare::find_prepared_short_circuit_join_context(
          prepared.names, *control_flow, function, "entry");
  if (!prepared_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the short-circuit join context")
                    .c_str());
  }

  const auto prepared_entry_targets = prepare::find_prepared_compare_branch_target_labels(
      prepared.names, *entry_branch_condition, *entry_block);
  if (!prepared_entry_targets.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the short-circuit entry branch contract")
                    .c_str());
  }

  const auto prepared_branch_plan = prepare::find_prepared_short_circuit_branch_plan(
      prepared.names, *prepared_join_context, *prepared_entry_targets);
  if (!prepared_branch_plan.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer publishes the short-circuit branch plan")
                    .c_str());
  }
  if (!join_transfer.source_false_transfer_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer no longer exposes false-lane ownership")
                    .c_str());
  }

  if (prepared_branch_plan->on_compare_true.block_label !=
          prepared_join_context->continuation_true_label ||
      !prepared_branch_plan->on_compare_false.continuation.has_value() ||
      block_label(prepared, prepared_branch_plan->on_compare_false.block_label) != "logic.end.10" ||
      block_label(prepared, prepared_branch_plan->on_compare_false.continuation->incoming_label) !=
          block_label(prepared,
                      join_transfer.edge_transfers[*join_transfer.source_false_transfer_index]
                          .predecessor_label) ||
      prepared_branch_plan->on_compare_false.continuation->true_label !=
          prepared_join_context->continuation_true_label ||
      prepared_branch_plan->on_compare_false.continuation->false_label !=
          prepared_join_context->continuation_false_label) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing the authoritative short-circuit branch-plan labels")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_branch_plan_helper_requires_authoritative_entry_targets_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit branch-plan contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "logic.end.10");
  if (entry_block == nullptr || join_block == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry/join blocks")
                    .c_str());
  }

  auto* entry_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == entry_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (entry_branch_condition == nullptr || join_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the expected branch metadata")
                    .c_str());
  }

  entry_block->terminator.true_label = "carrier.short_circuit";
  entry_block->terminator.false_label = "carrier.rhs";
  entry_block->terminator.condition = bir::Value::named(bir::TypeKind::I32, "carrier.entry.condition");
  entry_branch_condition->true_label = intern_block_label(prepared, "logic.rhs.9");
  entry_branch_condition->false_label = intern_block_label(prepared, "logic.end.10");
  join_block->terminator.true_label = "carrier.join.true";
  join_block->terminator.false_label = "carrier.join.false";
  join_branch_condition->true_label = intern_block_label(prepared, join_block->terminator.true_label);
  join_branch_condition->false_label =
      intern_block_label(prepared, join_block->terminator.false_label);
  join_branch_condition->predicate = bir::BinaryOpcode::Eq;
  if (use_edge_store_slot_carrier) {
    if (const auto status = convert_short_circuit_join_to_edge_store_slot(
            prepared, *control_flow, function, *join_block, *join_branch_condition, failure_context);
        status != 0) {
      return status;
    }
  }

  const auto prepared_join_context =
      prepare::find_prepared_short_circuit_join_context(
          prepared.names, *control_flow, function, "entry");
  if (!prepared_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the short-circuit join context")
                    .c_str());
  }

  auto prepared_entry_targets = prepare::find_prepared_compare_branch_target_labels(
      prepared.names, *entry_branch_condition, *entry_block);
  if (!prepared_entry_targets.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the short-circuit entry branch contract")
                    .c_str());
  }
  prepared_entry_targets->false_label = c4c::kInvalidBlockLabel;

  if (prepare::find_prepared_short_circuit_branch_plan(
          prepared.names, *prepared_join_context, *prepared_entry_targets)
          .has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper unexpectedly kept a short-circuit branch plan after authoritative entry-target loss")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_branch_plan_helper_requires_authoritative_continuation_labels_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2 ||
      control_flow->join_transfers.size() != 1) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the short-circuit branch-plan contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* join_block = find_block(function, "logic.end.10");
  if (entry_block == nullptr || join_block == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer has the expected entry/join blocks")
                    .c_str());
  }

  auto* entry_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == entry_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
        return &branch_condition;
      }
    }
    return nullptr;
  }();
  if (entry_branch_condition == nullptr || join_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture no longer exposes the expected branch metadata")
                    .c_str());
  }

  entry_block->terminator.true_label = "carrier.short_circuit";
  entry_block->terminator.false_label = "carrier.rhs";
  entry_block->terminator.condition = bir::Value::named(bir::TypeKind::I32, "carrier.entry.condition");
  entry_branch_condition->true_label = intern_block_label(prepared, "logic.rhs.9");
  entry_branch_condition->false_label = intern_block_label(prepared, "logic.end.10");
  join_block->terminator.true_label = "carrier.join.true";
  join_block->terminator.false_label = "carrier.join.false";
  join_branch_condition->true_label = intern_block_label(prepared, join_block->terminator.true_label);
  join_branch_condition->false_label =
      intern_block_label(prepared, join_block->terminator.false_label);
  join_branch_condition->predicate = bir::BinaryOpcode::Eq;
  if (use_edge_store_slot_carrier) {
    if (const auto status = convert_short_circuit_join_to_edge_store_slot(
            prepared, *control_flow, function, *join_block, *join_branch_condition, failure_context);
        status != 0) {
      return status;
    }
  }

  auto prepared_join_context =
      prepare::find_prepared_short_circuit_join_context(
          prepared.names, *control_flow, function, "entry");
  if (!prepared_join_context.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the short-circuit join context")
                    .c_str());
  }

  const auto prepared_entry_targets = prepare::find_prepared_compare_branch_target_labels(
      prepared.names, *entry_branch_condition, *entry_block);
  if (!prepared_entry_targets.has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the short-circuit entry branch contract")
                    .c_str());
  }
  prepared_join_context->continuation_false_label = c4c::kInvalidBlockLabel;

  if (prepare::find_prepared_short_circuit_branch_plan(
          prepared.names, *prepared_join_context, *prepared_entry_targets)
          .has_value()) {
    return fail((std::string(failure_context) +
                 ": shared helper unexpectedly kept a short-circuit branch plan after authoritative continuation-label loss")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_branch_plan_helper_publishes_prepared_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_branch_plan_helper_publishes_prepared_labels_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_branch_plan_helper_requires_authoritative_entry_targets(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_branch_plan_helper_requires_authoritative_entry_targets_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_branch_plan_helper_requires_authoritative_continuation_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_branch_plan_helper_requires_authoritative_continuation_labels_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_branch_plan_helper_publishes_prepared_labels_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_edge_store_slot_branch_plan_helper_requires_authoritative_entry_targets(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_branch_plan_helper_requires_authoritative_entry_targets_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_edge_store_slot_branch_plan_helper_requires_authoritative_continuation_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_branch_plan_helper_requires_authoritative_continuation_labels_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_route_validates_authoritative_join_carrier_impl(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context,
    bool use_edge_store_slot_carrier) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() < 2 ||
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
      mutable_control_flow->branch_conditions.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit fixture lost its mutable join-transfer contract")
                    .c_str());
  }

  auto& join_transfer = mutable_control_flow->join_transfers.front();
  auto* join_branch_condition = [&]() -> prepare::PreparedBranchCondition* {
    for (auto& branch_condition : mutable_control_flow->branch_conditions) {
      if (block_label(prepared, branch_condition.block_label) == join_block->label) {
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

  const std::string original_true_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
  const std::string original_false_incoming_label =
      std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
  join_transfer.source_true_incoming_label = intern_block_label(prepared, "contract.short_circuit");
  join_transfer.source_false_incoming_label = intern_block_label(prepared, "contract.rhs");
  join_transfer.edge_transfers[false_transfer_index].incoming_value = bir::Value::immediate_i32(9);
  bool rewrote_short_circuit_label = false;
  bool rewrote_rhs_label = false;
  for (auto& incoming : join_transfer.incomings) {
    if (incoming.label == original_true_incoming_label) {
      incoming.label = std::string(block_label(prepared, *join_transfer.source_true_incoming_label));
      rewrote_short_circuit_label = true;
      continue;
    }
    if (incoming.label == original_false_incoming_label) {
      incoming.label =
          std::string(block_label(prepared, *join_transfer.source_false_incoming_label));
      incoming.value = bir::Value::immediate_i32(9);
      rewrote_rhs_label = true;
    }
  }
  if (!rewrote_short_circuit_label || !rewrote_rhs_label) {
    return fail((std::string(failure_context) +
                 ": prepared short-circuit join transfer no longer exposes both contract-owned join incoming lanes")
                    .c_str());
  }

  if (use_edge_store_slot_carrier) {
    join_transfer.kind = prepare::PreparedJoinTransferKind::EdgeStoreSlot;
    join_transfer.storage_name = intern_slot_name(prepared, "%contract.short_circuit.slot");
    join_transfer.edge_transfers[true_transfer_index].storage_name = join_transfer.storage_name;
    join_transfer.edge_transfers[false_transfer_index].storage_name = join_transfer.storage_name;

    function.local_slots.push_back(bir::LocalSlot{
        .name = std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
        .type = bir::TypeKind::I32,
        .size_bytes = 4,
        .align_bytes = 4,
        .is_address_taken = false,
    });
    const auto load_result = bir::Value::named(bir::TypeKind::I32, "carrier.short_circuit.join");
    join_block->insts.front() = bir::LoadLocalInst{
        .result = load_result,
        .slot_name =
            std::string(prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name)),
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

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_minimal_local_i32_short_circuit_or_guard_asm(function_name)) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped validating the authoritative short-circuit join carrier")
                    .c_str());
  }

  return 0;
}

int check_short_circuit_route_validates_authoritative_join_carrier(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_validates_authoritative_join_carrier_impl(
      module, function_name, failure_context, false);
}

int check_short_circuit_edge_store_slot_route_consumes_prepared_control_flow(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_validates_authoritative_join_carrier_impl(
      module, function_name, failure_context, true);
}

int check_short_circuit_edge_store_slot_route_validates_authoritative_join_carrier(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  return check_short_circuit_route_validates_authoritative_join_carrier_impl(
      module, function_name, failure_context, true);
}

}  // namespace


int run_backend_x86_handoff_boundary_short_circuit_tests() {
  if (const auto status =
          check_local_i32_guard_route_rejects_authoritative_join_contract(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot compare-against-immediate guard route rejects fallback when authoritative short-circuit join ownership remains active");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_short_circuit_route_rejects_drifted_branch_plan_continuation_targets(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route rejects drifted prepared branch-plan continuation targets");
      status != 0) {
    return status;
  }

  if (const auto status =
          check_short_circuit_route_rejects_missing_prepared_local_frame_slot(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route rejects missing prepared frame-slot authority");
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
          check_route_outputs(make_x86_local_i8_short_circuit_or_guard_module(),
                              expected_minimal_local_i8_short_circuit_or_guard_asm("main"),
                              "select ne i8 %t3, 3, i32 1, %t13",
                              "minimal local-slot i8 short-circuit or-guard route");
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
          check_short_circuit_route_requires_authoritative_parallel_copy_bundles(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit or-guard prepared-control-flow ownership rejects phi-edge obligations when authoritative parallel-copy publication is removed but join metadata remains");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_regalloc_consumes_published_parallel_copy_execution_sites(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit or-guard keeps branch-owned bundles on the published execution sites");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_with_rhs_passthrough_consumes_prepared_control_flow(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit or-guard ignores rhs passthrough topology when prepared continuation ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_prefers_prepared_passthrough_branch_target(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route prefers authoritative rhs passthrough branch targets over raw branch drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_prefers_prepared_passthrough_branch_target(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot route prefers authoritative rhs passthrough branch targets over raw branch drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_ignores_rhs_compare_carrier_state_when_prepared_control_flow_is_authoritative(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit or-guard ignores rhs compare carrier state when prepared continuation ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_requires_authoritative_rhs_continuation_compare_carrier(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route rejects a missing rhs compare carrier when prepared continuation ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_context_publishes_prepared_continuation_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit prepared continuation-label ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_context_prefers_prepared_continuation_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit prefers prepared rhs continuation metadata over join-branch label drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_prefers_prepared_continuation_labels_when_join_branch_labels_drift(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route prefers prepared rhs continuation labels over join-branch label drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_requires_authoritative_rhs_prepared_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route rejects a drifted rhs prepared branch contract instead of falling back to the raw rhs compare");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_rejects_missing_authoritative_rhs_prepared_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route rejects a missing rhs prepared branch record instead of falling back to the raw rhs compare");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_requires_authoritative_entry_prepared_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route rejects a drifted authoritative entry prepared branch contract instead of falling back to the plain conditional lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_rejects_missing_authoritative_entry_prepared_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route rejects a missing authoritative entry prepared branch record instead of falling back to the plain conditional lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_requires_authoritative_entry_prepared_target_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route rejects drifted authoritative entry prepared target labels instead of following local entry labels");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_entry_branch_helper_publishes_prepared_target_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit entry-branch prepared target-label ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_branch_plan_helper_publishes_prepared_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit branch-plan prepared label ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_branch_plan_helper_requires_authoritative_entry_targets(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit branch-plan helper rejects entry-target loss instead of keeping a local fallback alive");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_branch_plan_helper_requires_authoritative_continuation_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit branch-plan helper rejects continuation-label loss instead of keeping a local fallback alive");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_branch_plan_helper_publishes_prepared_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot branch-plan prepared label ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_branch_plan_helper_requires_authoritative_entry_targets(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot branch-plan helper rejects entry-target loss instead of keeping a local fallback alive");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_branch_plan_helper_requires_authoritative_continuation_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot branch-plan helper rejects continuation-label loss instead of keeping a local fallback alive");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_route_validates_authoritative_join_carrier(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route validates the authoritative PhiEdge join carrier");
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
          check_short_circuit_edge_store_slot_route_validates_authoritative_join_carrier(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit route validates the authoritative EdgeStoreSlot join carrier");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_context_publishes_prepared_continuation_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot prepared continuation-label ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_context_prefers_prepared_continuation_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot prefers prepared rhs continuation metadata over join-branch label drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_requires_authoritative_rhs_continuation_compare_carrier(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot route rejects a missing rhs compare carrier when prepared continuation ownership is authoritative");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_prefers_prepared_continuation_labels_when_join_branch_labels_drift(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot route prefers prepared rhs continuation labels over join-branch label drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_requires_authoritative_rhs_prepared_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot route rejects a drifted rhs prepared branch contract instead of falling back to the raw rhs compare");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_rejects_missing_authoritative_rhs_prepared_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot route rejects a missing rhs prepared branch record instead of falling back to the raw rhs compare");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_requires_authoritative_entry_prepared_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot route rejects a drifted authoritative entry prepared branch contract instead of falling back to the plain conditional lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_rejects_missing_authoritative_entry_prepared_branch_condition(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot route rejects a missing authoritative entry prepared branch record instead of falling back to the plain conditional lane");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_short_circuit_edge_store_slot_route_requires_authoritative_entry_prepared_target_labels(
              make_x86_local_i32_short_circuit_or_guard_module(),
              "main",
              "minimal local-slot short-circuit EdgeStoreSlot route rejects drifted authoritative entry prepared target labels instead of following local entry labels");
      status != 0) {
    return status;
  }

  return 0;
}
