#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/x86/api/api.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
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

c4c::FunctionNameId intern_function_name(prepare::PreparedBirModule& prepared,
                                         std::string_view name) {
  return prepared.names.function_names.intern(name);
}

c4c::SlotNameId intern_slot_name(prepare::PreparedBirModule& prepared,
                                 std::string_view name) {
  return prepared.names.slot_names.intern(name);
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
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
          .function_name = intern_function_name(prepared, function_name),
          .block_label = intern_block_label(prepared, "carrier.loop"),
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = bir::Value::named(bir::TypeKind::I32, "carrier.cond"),
          .predicate = bir::BinaryOpcode::Eq,
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::named(bir::TypeKind::I32, "carrier.counter"),
          .rhs = bir::Value::immediate_i32(0),
          .can_fuse_with_branch = true,
          .true_label = intern_block_label(prepared, "carrier.body"),
          .false_label = intern_block_label(prepared, "carrier.exit"),
      });
  mutable_control_flow->join_transfers.insert(
      mutable_control_flow->join_transfers.begin(),
      prepare::PreparedJoinTransfer{
          .function_name = intern_function_name(prepared, function_name),
          .join_block_label = intern_block_label(prepared, "carrier.loop"),
          .result = bir::Value::named(bir::TypeKind::I32, "carrier.counter"),
          .kind = prepare::PreparedJoinTransferKind::LoopCarry,
          .storage_name = intern_slot_name(prepared, "%carrier.counter"),
          .edge_transfers =
              {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = intern_block_label(prepared, "carrier.entry"),
                      .successor_label = intern_block_label(prepared, "carrier.loop"),
                      .incoming_value = bir::Value::immediate_i32(9),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "carrier.counter"),
                      .storage_name = intern_slot_name(prepared, "%carrier.counter"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = intern_block_label(prepared, "carrier.body"),
                      .successor_label = intern_block_label(prepared, "carrier.loop"),
                      .incoming_value = bir::Value::named(bir::TypeKind::I32, "carrier.next"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "carrier.counter"),
                      .storage_name = intern_slot_name(prepared, "%carrier.counter"),
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

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative loop control-flow contract over unrelated prepared records")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_ignores_slot_shaped_join_storage_metadata(
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
                 ": prepared loop fixture lost its mutable loop-countdown handoff contract")
                    .c_str());
  }

  auto& join_transfer = mutable_control_flow->join_transfers.front();
  if (join_transfer.edge_transfers.size() != 2) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the bounded loop-carry edge ownership")
                    .c_str());
  }

  join_transfer.storage_name.reset();
  for (auto& edge_transfer : join_transfer.edge_transfers) {
    edge_transfer.storage_name.reset();
  }

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer still depends on slot-shaped loop-carry storage metadata")
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
      block_label(prepared, join_transfer.edge_transfers.front().predecessor_label) != "entry" ||
      block_label(prepared, join_transfer.edge_transfers.back().predecessor_label) != "body") {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the expected entry/body loop-carry edge ownership")
                    .c_str());
  }

  std::swap(join_transfer.edge_transfers.front(), join_transfer.edge_transfers.back());

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
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
      block_label(prepared, join_transfer.edge_transfers.front().predecessor_label) !=
          "preheader" ||
      block_label(prepared, join_transfer.edge_transfers.back().predecessor_label) != "body") {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the preheader/body loop-carry ownership")
                    .c_str());
  }

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
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
      block_label(prepared, join_transfer.edge_transfers.front().predecessor_label) !=
          "preheader" ||
      block_label(prepared, join_transfer.edge_transfers.back().predecessor_label) != "body") {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the preheader/body loop-carry ownership across the transparent entry carrier")
                    .c_str());
  }

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
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
      block_label(prepared, join_transfer.edge_transfers.front().predecessor_label) !=
          "preheader" ||
      block_label(prepared, join_transfer.edge_transfers.back().predecessor_label) != "body") {
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

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
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
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
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
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
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
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
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
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
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

int check_loop_countdown_route_requires_authoritative_parallel_copy_bundles(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      control_flow->join_transfers.size() != 1 || control_flow->parallel_copy_bundles.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the loop countdown out-of-SSA handoff contract")
                    .c_str());
  }
  if (control_flow->join_transfers.front().kind != prepare::PreparedJoinTransferKind::LoopCarry) {
    return fail((std::string(failure_context) +
                 ": prepare no longer classifies the loop countdown join as explicit loop-carry traffic")
                    .c_str());
  }

  auto* mutable_control_flow = find_control_flow_function(prepared, function_name);
  if (mutable_control_flow == nullptr || mutable_control_flow->branch_conditions.size() != 1 ||
      mutable_control_flow->join_transfers.size() != 1 ||
      mutable_control_flow->parallel_copy_bundles.empty()) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture lost its mutable loop-countdown out-of-SSA handoff contract")
                    .c_str());
  }

  mutable_control_flow->parallel_copy_bundles.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted loop-countdown phi edge obligations after authoritative parallel-copy publication was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected missing loop-countdown parallel-copy publication with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_loop_countdown_regalloc_consumes_predecessor_parallel_copy_execution_site(
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
                 ": prepare no longer publishes the loop regalloc/value-location contract")
                    .c_str());
  }

  const auto* bundle =
      prepare::find_prepared_parallel_copy_bundle(prepared.names, *control_flow, "entry", "loop");
  if (bundle == nullptr ||
      bundle->execution_site != prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
    return fail((std::string(failure_context) +
                 ": prepare no longer classifies the entry-to-loop handoff as predecessor-terminator executable")
                    .c_str());
  }

  const auto predecessor_block_index = find_block_index(prepared.module.functions.front(), "entry");
  const auto successor_block_index = find_block_index(prepared.module.functions.front(), "loop");
  if (!predecessor_block_index.has_value() || !successor_block_index.has_value()) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the expected blocks for bundle placement")
                    .c_str());
  }

  const auto* move_bundle = prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
      prepared.names, prepared.module.functions.front(), *function_locations, *bundle);
  if (move_bundle == nullptr) {
    return fail((std::string(failure_context) +
                 ": regalloc stopped placing predecessor-owned loop bundles at the published predecessor block")
                    .c_str());
  }
  if (move_bundle->block_index != *predecessor_block_index) {
    return fail((std::string(failure_context) +
                 ": regalloc unexpectedly relocated predecessor-owned loop bundles into the header block")
                    .c_str());
  }

  return 0;
}

int check_local_countdown_guard_route_consumes_authoritative_guard_branch_condition(
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
        prepare::PreparedControlFlowFunction{
            .function_name = intern_function_name(prepared, function_name)});
    control_flow = &prepared.control_flow.functions.back();
  }
  control_flow->branch_conditions.clear();
  control_flow->join_transfers.clear();

  std::string baseline_asm;
  try {
    baseline_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer renders before authoritative guard ownership is injected: " +
                 ex.what())
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* guard_block = find_block(function, "guard");
  if (guard_block == nullptr || guard_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer exposes the expected mutable guard branch")
                    .c_str());
  }

  control_flow->branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = intern_function_name(prepared, function_name),
      .block_label = intern_block_label(prepared, "guard"),
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = bir::Value::named(bir::TypeKind::I32, "guard.cmp"),
      .predicate = bir::BinaryOpcode::Ne,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "guard.counter"),
      .rhs = bir::Value::immediate_i32(0),
      .can_fuse_with_branch = true,
      .true_label = intern_block_label(prepared, "guard_true"),
      .false_label = intern_block_label(prepared, "cont"),
  });

  guard_block->terminator.condition =
      bir::Value::named(bir::TypeKind::I32, "drifted.guard.condition");
  guard_block->terminator.true_label = "drifted.guard.true";
  guard_block->terminator.false_label = "drifted.guard.false";

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the authoritative guard-owned countdown fallback with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != baseline_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative prepared guard metadata over raw guard-terminator drift")
                    .c_str());
  }

  return 0;
}

int check_local_countdown_guard_route_consumes_authoritative_join_transfer(
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
        prepare::PreparedControlFlowFunction{
            .function_name = intern_function_name(prepared, function_name)});
    control_flow = &prepared.control_flow.functions.back();
  }
  control_flow->branch_conditions.clear();
  control_flow->join_transfers.clear();

  std::string baseline_asm;
  try {
    baseline_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer renders before authoritative join ownership is injected: " +
                 ex.what())
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* continuation_init = find_block(function, "cont");
  if (continuation_init == nullptr || continuation_init->insts.empty()) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer exposes the continuation init carrier")
                    .c_str());
  }
  auto* continuation_store =
      std::get_if<bir::StoreLocalInst>(&continuation_init->insts.front());
  if (continuation_store == nullptr) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer exposes the mutable continuation init store")
                    .c_str());
  }

  control_flow->join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = intern_function_name(prepared, function_name),
      .join_block_label = intern_block_label(prepared, "loop1"),
      .result = bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
      .kind = prepare::PreparedJoinTransferKind::PhiEdge,
      .edge_transfers =
          {
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = intern_block_label(prepared, "cont"),
                  .successor_label = intern_block_label(prepared, "loop1"),
                  .incoming_value = bir::Value::immediate_i32(2),
                  .destination_value =
                      bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
              },
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = intern_block_label(prepared, "body1"),
                  .successor_label = intern_block_label(prepared, "loop1"),
                  .incoming_value = bir::Value::named(bir::TypeKind::I32, "body1.next"),
                  .destination_value =
                      bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
              },
          },
      .source_branch_block_label = intern_block_label(prepared, "loop1"),
      .source_true_transfer_index = 1,
      .source_false_transfer_index = 0,
      .source_true_incoming_label = intern_block_label(prepared, "body1"),
      .source_false_incoming_label = intern_block_label(prepared, "cont"),
  });

  continuation_store->value = bir::Value::immediate_i32(99);

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the authoritative join-owned countdown fallback with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != baseline_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative continuation join ownership over a drifted local init store")
                    .c_str());
  }

  return 0;
}

int check_local_countdown_guard_route_prefers_authoritative_continuation_init_target(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));

  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local countdown control-flow contract")
                    .c_str());
  }
  control_flow->branch_conditions.clear();
  control_flow->join_transfers.clear();

  std::string baseline_asm;
  try {
    baseline_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer renders before continuation-carrier target drift is injected: " +
                 ex.what())
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* continuation_init = find_block(function, "cont");
  if (continuation_init == nullptr ||
      continuation_init->terminator.kind != bir::TerminatorKind::Branch) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer exposes the continuation init branch carrier")
                    .c_str());
  }

  continuation_init->terminator.target_label = "drifted.cont.loop";

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the authoritative continuation-carrier branch ownership with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != baseline_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative prepared continuation-carrier targets over raw branch drift")
                    .c_str());
  }

  return 0;
}

int check_local_countdown_guard_route_prefers_authoritative_continuation_body_target(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));

  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local countdown control-flow contract")
                    .c_str());
  }
  control_flow->branch_conditions.clear();
  control_flow->join_transfers.clear();

  std::string baseline_asm;
  try {
    baseline_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer renders before continuation-body target drift is injected: " +
                 ex.what())
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* continuation_body = find_block(function, "body1");
  if (continuation_body == nullptr ||
      continuation_body->terminator.kind != bir::TerminatorKind::Branch) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer exposes the continuation body branch carrier")
                    .c_str());
  }

  continuation_body->terminator.target_label = "drifted.body1.loop";

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the authoritative continuation-body branch ownership with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != baseline_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative prepared continuation-body targets over raw branch drift")
                    .c_str());
  }

  return 0;
}

int check_local_countdown_guard_route_prefers_authoritative_loop_targets_after_join_transfer(
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
        prepare::PreparedControlFlowFunction{
            .function_name = intern_function_name(prepared, function_name)});
    control_flow = &prepared.control_flow.functions.back();
  }
  control_flow->branch_conditions.clear();
  control_flow->join_transfers.clear();

  std::string baseline_asm;
  try {
    baseline_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer renders before authoritative continuation-loop ownership is injected: " +
                 ex.what())
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* continuation_loop = find_block(function, "loop1");
  if (continuation_loop == nullptr ||
      continuation_loop->terminator.kind != bir::TerminatorKind::CondBranch) {
    return fail((std::string(failure_context) +
                 ": local countdown fallback fixture no longer exposes the continuation loop branch carrier")
                    .c_str());
  }

  control_flow->join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = intern_function_name(prepared, function_name),
      .join_block_label = intern_block_label(prepared, "loop1"),
      .result = bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
      .kind = prepare::PreparedJoinTransferKind::PhiEdge,
      .edge_transfers =
          {
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = intern_block_label(prepared, "cont"),
                  .successor_label = intern_block_label(prepared, "loop1"),
                  .incoming_value = bir::Value::immediate_i32(2),
                  .destination_value =
                      bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
              },
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = intern_block_label(prepared, "body1"),
                  .successor_label = intern_block_label(prepared, "loop1"),
                  .incoming_value = bir::Value::named(bir::TypeKind::I32, "body1.next"),
                  .destination_value =
                      bir::Value::named(bir::TypeKind::I32, "loop1.counter"),
              },
          },
      .source_branch_block_label = intern_block_label(prepared, "loop1"),
      .source_true_transfer_index = 1,
      .source_false_transfer_index = 0,
      .source_true_incoming_label = intern_block_label(prepared, "body1"),
      .source_false_incoming_label = intern_block_label(prepared, "cont"),
  });

  continuation_loop->terminator.condition =
      bir::Value::named(bir::TypeKind::I32, "drifted.loop1.condition");
  continuation_loop->terminator.true_label = "drifted.loop1.body";
  continuation_loop->terminator.false_label = "drifted.loop1.exit";

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the authoritative continuation-loop ownership with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != baseline_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative prepared loop targets over raw continuation-loop terminator drift")
                    .c_str());
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
  branch_condition.true_label = intern_block_label(prepared, "exit");
  branch_condition.false_label = intern_block_label(prepared, "body");

  loop_block->terminator.true_label = std::string(block_label(prepared, branch_condition.true_label));
  loop_block->terminator.false_label =
      std::string(block_label(prepared, branch_condition.false_label));

  loop_compare->opcode = bir::BinaryOpcode::Eq;
  loop_compare->lhs = bir::Value::immediate_i32(7);
  loop_compare->rhs = bir::Value::immediate_i32(7);

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
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
  branch_condition.true_label = intern_block_label(prepared, "exit");
  branch_condition.false_label = intern_block_label(prepared, "body");

  loop_block->terminator.true_label = std::string(block_label(prepared, branch_condition.true_label));
  loop_block->terminator.false_label =
      std::string(block_label(prepared, branch_condition.false_label));

  loop_compare->opcode = bir::BinaryOpcode::Eq;
  loop_compare->lhs = bir::Value::immediate_i32(7);
  loop_compare->rhs = bir::Value::immediate_i32(7);

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
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

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped preferring authoritative prepared loop branch metadata over raw loop terminator drift")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_prefers_authoritative_single_successor_targets(
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
      control_flow->join_transfers.size() != 1 || control_flow->blocks.size() < 3) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the loop countdown single-successor control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  auto* body_block = find_block(function, "body");
  if (entry_block == nullptr || body_block == nullptr ||
      entry_block->terminator.kind != bir::TerminatorKind::Branch ||
      body_block->terminator.kind != bir::TerminatorKind::Branch) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the expected entry/body single-successor carriers")
                    .c_str());
  }

  entry_block->terminator.target_label = "drifted.entry.loop";
  body_block->terminator.target_label = "drifted.body.loop";

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped preferring authoritative prepared single-successor targets over raw entry/body branch drift")
                    .c_str());
  }

  return 0;
}

int check_loop_countdown_route_prefers_authoritative_transparent_prefix_targets(
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
      control_flow->join_transfers.size() != 1 || control_flow->blocks.size() < 5) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the transparent-prefix single-successor control-flow contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* carrier_block = find_block(function, "carrier");
  auto* preheader_block = find_block(function, "preheader");
  if (carrier_block == nullptr || preheader_block == nullptr ||
      carrier_block->terminator.kind != bir::TerminatorKind::Branch ||
      preheader_block->terminator.kind != bir::TerminatorKind::Branch) {
    return fail((std::string(failure_context) +
                 ": prepared loop fixture no longer exposes the expected transparent-prefix branch carriers")
                    .c_str());
  }

  carrier_block->terminator.target_label = "drifted.prefix.preheader";
  preheader_block->terminator.target_label = "drifted.prefix.loop";

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative transparent-prefix targets over raw preheader-chain drift")
                    .c_str());
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_loop_countdown_tests() {
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
          check_loop_countdown_route_ignores_slot_shaped_join_storage_metadata(
              make_x86_loop_countdown_join_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership ignores slot-shaped loop-carry storage metadata");
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
          check_loop_countdown_route_requires_authoritative_parallel_copy_bundles(
              make_x86_loop_countdown_join_module(),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership rejects loop phi-edge obligations when authoritative parallel-copy publication is removed but join metadata remains");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_regalloc_consumes_predecessor_parallel_copy_execution_site(
              make_x86_loop_countdown_join_module(),
              "main",
              "minimal loop-carried join countdown keeps predecessor-owned bundles on the published predecessor execution site");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_countdown_guard_route_consumes_authoritative_guard_branch_condition(
              make_x86_local_i32_countdown_guard_continuation_module(),
              "main",
              "two-segment local countdown fallback consumes authoritative prepared guard ownership over raw guard-terminator drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_countdown_guard_route_consumes_authoritative_join_transfer(
              make_x86_local_i32_countdown_guard_continuation_module(),
              "main",
              "two-segment local countdown fallback consumes authoritative continuation-loop join ownership over a drifted local init carrier");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_countdown_guard_route_prefers_authoritative_continuation_init_target(
              make_x86_local_i32_countdown_guard_continuation_module(),
              "main",
              "two-segment local countdown fallback consumes authoritative continuation-carrier branch targets over raw continuation-init drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_countdown_guard_route_prefers_authoritative_continuation_body_target(
              make_x86_local_i32_countdown_guard_continuation_module(),
              "main",
              "two-segment local countdown fallback consumes authoritative continuation-body branch targets over raw continuation-body drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_countdown_guard_route_prefers_authoritative_loop_targets_after_join_transfer(
              make_x86_local_i32_countdown_guard_continuation_module(),
              "main",
              "two-segment local countdown fallback consumes authoritative continuation-loop targets over raw continuation-loop terminator drift");
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
          check_loop_countdown_route_prefers_authoritative_single_successor_targets(
              make_x86_loop_countdown_join_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership prefers authoritative single-successor targets over raw entry/body branch drift");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_loop_countdown_route_prefers_authoritative_transparent_prefix_targets(
              make_x86_loop_countdown_join_with_preheader_chain_module(),
              expected_minimal_loop_countdown_join_asm("main"),
              "main",
              "minimal loop-carried join countdown prepared-control-flow ownership prefers authoritative transparent-prefix targets over raw preheader-chain drift");
      status != 0) {
    return status;
  }

  return 0;
}
