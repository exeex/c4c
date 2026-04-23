#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

c4c::TargetProfile riscv_target_profile() {
  return c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");
}

prepare::PreparedBirModule legalize_short_circuit_or_guard_module() {
  bir::Module module;

  bir::Function function;
  function.name = "short_circuit_or_prepare_contract";
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

  function.blocks = {
      std::move(entry),
      std::move(logic_rhs),
      std::move(logic_rhs_end),
      std::move(logic_skip),
      std::move(logic_end),
      std::move(block_1),
      std::move(block_2),
  };
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule legalize_loop_countdown_module() {
  bir::Module module;

  bir::Function function;
  function.name = "loop_countdown_prepare_contract";
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
      .result = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cmp0"),
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

  function.blocks = {
      std::move(entry),
      std::move(loop),
      std::move(body),
      std::move(exit),
  };
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule legalize_parallel_copy_cycle_module() {
  bir::Module module;

  bir::Function function;
  function.name = "parallel_copy_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "a"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(1),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "b"),
          },
      },
  });
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "b"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(2),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "a"),
          },
      },
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "a"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .true_label = "body",
      .false_label = "exit",
  };

  bir::Block body;
  body.label = "body";
  body.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "a"),
  };

  function.blocks = {
      std::move(entry),
      std::move(loop),
      std::move(body),
      std::move(exit),
  };
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_call_wrapper_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function same_module_callee;
  same_module_callee.name = "same_module_i32";
  same_module_callee.return_type = bir::TypeKind::I32;
  same_module_callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "value",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  bir::Block same_module_entry;
  same_module_entry.label = "entry";
  same_module_entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "value")};
  same_module_callee.blocks.push_back(std::move(same_module_entry));
  module.functions.push_back(std::move(same_module_callee));

  bir::Function fixed_extern;
  fixed_extern.name = "extern_fixed_i32";
  fixed_extern.is_declaration = true;
  fixed_extern.return_type = bir::TypeKind::I32;
  fixed_extern.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "value",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(fixed_extern));

  bir::Function variadic_extern;
  variadic_extern.name = "extern_variadic_i32";
  variadic_extern.is_declaration = true;
  variadic_extern.is_variadic = true;
  variadic_extern.return_type = bir::TypeKind::I32;
  variadic_extern.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "head",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(variadic_extern));

  bir::Function caller;
  caller.name = "call_wrapper_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  caller.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "callee.ptr",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.same_module"),
      .callee = "same_module_i32",
      .args = {bir::Value::immediate_i32(1)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.extern_fixed"),
      .callee = "extern_fixed_i32",
      .args = {bir::Value::immediate_i32(2)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.extern_variadic"),
      .callee = "extern_variadic_i32",
      .args = {bir::Value::immediate_i32(3), bir::Value::immediate_f32_bits(0x40800000)},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::F32},
      .arg_abi = {
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::I32,
              .size_bytes = 4,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::F32,
              .size_bytes = 4,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Sse,
              .passed_in_register = true,
          },
      },
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
      .is_variadic = true,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.indirect_wrapper"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "callee.ptr"),
      .args = {bir::Value::immediate_i32(5)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
      .is_indirect = true,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "tmp.indirect_wrapper")};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_memory_return_call_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "extern_make_pair";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::Void;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "ret.sret",
      .size_bytes = 8,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      },
      .is_sret = true,
  });
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "seed",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "memory_return_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "lv.call.sret.storage",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "extern_make_pair",
      .args = {
          bir::Value::named(bir::TypeKind::Ptr, "lv.call.sret.storage"),
          bir::Value::immediate_i32(13),
      },
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .arg_abi = {
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = 8,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Memory,
              .sret_pointer = true,
          },
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::I32,
              .size_bytes = 4,
              .align_bytes = 4,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
      },
      .return_type_name = "pair",
      .return_type = bir::TypeKind::Void,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Void,
          .primary_class = bir::AbiValueClass::Memory,
          .returned_in_memory = true,
      },
      .sret_storage_name = "lv.call.sret.storage",
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_call_argument_source_shape_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "extern_data",
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Function callee;
  callee.name = "extern_consume_ptr_pair";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::I32;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "lhs",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "rhs",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "call_argument_source_shape_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  caller.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "base.ptr",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "lv.ptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "lv.scratch",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.ptr",
      .value = bir::Value::named(bir::TypeKind::Ptr, "base.ptr"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "loaded.ptr"),
      .slot_name = "lv.ptr",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "touch"),
      .slot_name = "lv.scratch",
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "loaded.ptr"),
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.ptr",
      .value = bir::Value::named(bir::TypeKind::Ptr, "derived.seed"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "arg.ptr"),
      .slot_name = "lv.ptr",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.call"),
      .callee = "extern_consume_ptr_pair",
      .args = {
          bir::Value::named(bir::TypeKind::Ptr, "@extern_data"),
          bir::Value::named(bir::TypeKind::Ptr, "arg.ptr"),
      },
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .arg_abi = {
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = 8,
              .align_bytes = 8,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
          bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = 8,
              .align_bytes = 8,
              .primary_class = bir::AbiValueClass::Integer,
              .passed_in_register = true,
          },
      },
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "tmp.call")};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_cross_call_preservation_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "boundary_helper";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::I32;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "cross_call_preservation_dump_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "pre.only"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(3),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.out"),
      .callee = "boundary_helper",
      .args = {bir::Value::named(bir::TypeKind::I32, "pre.only")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "after"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "call.out"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "after")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(module, riscv_target_profile(), options);
}

prepare::PreparedBirModule prepare_stack_cross_call_preservation_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "stack_boundary_helper";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::I32;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "stack_cross_call_preservation_dump_contract";
  function.return_type = bir::TypeKind::I32;
  constexpr int carry_count = 16;
  for (int index = 0; index < carry_count; ++index) {
    function.params.push_back(bir::Param{
        .type = bir::TypeKind::I32,
        .name = "p" + std::to_string(index),
        .size_bytes = 4,
        .align_bytes = 4,
        .abi = bir::CallArgAbiInfo{
            .type = bir::TypeKind::I32,
            .size_bytes = 4,
            .align_bytes = 4,
            .primary_class = bir::AbiValueClass::Integer,
            .passed_in_register = true,
        },
    });
  }

  bir::Block entry;
  entry.label = "entry";
  for (int index = 0; index < carry_count; ++index) {
    entry.insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "carry.stack." + std::to_string(index)),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32, "p" + std::to_string(index)),
        .rhs = bir::Value::immediate_i32(index + 2),
    });
  }
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.stack.out"),
      .callee = "stack_boundary_helper",
      .args = {bir::Value::immediate_i32(17)},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "stack.sum.0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "call.stack.out"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "carry.stack.0"),
  });
  for (int index = 1; index < carry_count; ++index) {
    entry.insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "stack.sum." + std::to_string(index)),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32, "stack.sum." + std::to_string(index - 1)),
        .rhs = bir::Value::named(bir::TypeKind::I32, "carry.stack." + std::to_string(index)),
    });
  }
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "stack.sum." + std::to_string(carry_count - 1))};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_stack_argument_slot_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "extern_take_byval";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::Void;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "pair",
      .size_bytes = 8,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      },
      .is_byval = true,
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "stack_argument_slot_dump_contract";
  caller.return_type = bir::TypeKind::Void;
  caller.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.byval",
      .size_bytes = 8,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      },
      .is_byval = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "extern_take_byval",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "p.byval")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

prepare::PreparedBirModule prepare_stack_result_slot_dump_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "extern_spill_i32";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::I32;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "value",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "stack_result_slot_dump_contract";
  caller.return_type = bir::TypeKind::I32;
  for (int index = 0; index < 8; ++index) {
    caller.params.push_back(bir::Param{
        .type = bir::TypeKind::I32,
        .name = "p" + std::to_string(index),
        .size_bytes = 4,
        .align_bytes = 4,
        .abi = bir::CallArgAbiInfo{
            .type = bir::TypeKind::I32,
            .size_bytes = 4,
            .align_bytes = 4,
            .primary_class = bir::AbiValueClass::Integer,
            .passed_in_register = true,
        },
    });
  }

  bir::Block entry;
  entry.label = "entry";
  for (int index = 0; index < 8; ++index) {
    entry.insts.push_back(bir::CallInst{
        .result = bir::Value::named(bir::TypeKind::I32, "tmp.call." + std::to_string(index)),
        .callee = "extern_spill_i32",
        .args = {bir::Value::named(bir::TypeKind::I32, "p" + std::to_string(index))},
        .arg_types = {bir::TypeKind::I32},
        .arg_abi = {bir::CallArgAbiInfo{
            .type = bir::TypeKind::I32,
            .size_bytes = 4,
            .align_bytes = 4,
            .primary_class = bir::AbiValueClass::Integer,
            .passed_in_register = true,
        }},
        .return_type_name = "i32",
        .return_type = bir::TypeKind::I32,
        .result_abi = bir::CallResultAbiInfo{
            .type = bir::TypeKind::I32,
            .primary_class = bir::AbiValueClass::Integer,
        },
    });
  }
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum.0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "tmp.call.0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "tmp.call.1"),
  });
  for (int index = 2; index < 8; ++index) {
    entry.insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "sum." + std::to_string(index - 1)),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32, "sum." + std::to_string(index - 2)),
        .rhs = bir::Value::named(bir::TypeKind::I32, "tmp.call." + std::to_string(index)),
    });
  }
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "sum.6")};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      c4c::default_target_profile(c4c::TargetArch::X86_64),
      options);
}

const prepare::PreparedStackObject* find_stack_object(const prepare::PreparedBirModule& prepared,
                                                      const char* object_name) {
  for (const auto& object : prepared.stack_layout.objects) {
    if (prepare::prepared_stack_object_name(prepared.names, object) == object_name) {
      return &object;
    }
  }
  return nullptr;
}

const prepare::PreparedStoragePlanFunction* find_storage_plan_function(
    const prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_storage_plan(prepared, function_id);
}

const prepare::PreparedCallPlansFunction* find_call_plans_function(
    const prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_call_plans(prepared, function_id);
}

const prepare::PreparedStoragePlanValue* find_storage_value(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedStoragePlanFunction& function,
    const char* value_name) {
  const auto value_id = prepared.names.value_names.find(value_name);
  if (value_id == c4c::kInvalidValueName) {
    return nullptr;
  }
  for (const auto& value : function.values) {
    if (value.value_name == value_id) {
      return &value;
    }
  }
  return nullptr;
}

std::string clobber_summary(const prepare::PreparedClobberedRegister& clobber) {
  std::string summary = std::string(prepare::prepared_register_bank_name(clobber.bank)) + ":" +
                        clobber.register_name + "/w" + std::to_string(clobber.contiguous_width);
  if (!clobber.occupied_register_names.empty()) {
    summary += "[";
    for (std::size_t index = 0; index < clobber.occupied_register_names.size(); ++index) {
      if (index != 0) {
        summary += ",";
      }
      summary += clobber.occupied_register_names[index];
    }
    summary += "]";
  }
  return summary;
}

std::string clobber_detail(const prepare::PreparedClobberedRegister& clobber) {
  std::string detail = "clobber bank=" +
                       std::string(prepare::prepared_register_bank_name(clobber.bank)) +
                       " reg=" + clobber.register_name + " width=" +
                       std::to_string(clobber.contiguous_width);
  if (!clobber.occupied_register_names.empty()) {
    detail += " units=";
    for (std::size_t index = 0; index < clobber.occupied_register_names.size(); ++index) {
      if (index != 0) {
        detail += ",";
      }
      detail += clobber.occupied_register_names[index];
    }
  }
  return detail;
}

const prepare::PreparedFrameSlot* find_frame_slot(const prepare::PreparedBirModule& prepared,
                                                  prepare::PreparedObjectId object_id) {
  for (const auto& slot : prepared.stack_layout.frame_slots) {
    if (slot.object_id == object_id) {
      return &slot;
    }
  }
  return nullptr;
}

bool expect_contains(const std::string& text,
                     const std::string& needle,
                     const char* description) {
  if (text.find(needle) != std::string::npos) {
    return true;
  }
  std::cerr << "[FAIL] missing " << description << ": " << needle << "\n";
  std::cerr << "--- dump ---\n" << text << "\n";
  return false;
}

bool expect_not_contains(const std::string& text,
                         const std::string& needle,
                         const char* description) {
  if (text.find(needle) == std::string::npos) {
    return true;
  }
  std::cerr << "[FAIL] unexpected " << description << ": " << needle << "\n";
  std::cerr << "--- dump ---\n" << text << "\n";
  return false;
}

prepare::PreparedControlFlowFunction* find_control_flow_function(
    prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const c4c::FunctionNameId function_name_id =
      prepared.names.function_names.find(function_name);
  if (function_name_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  for (auto& function : prepared.control_flow.functions) {
    if (function.function_name == function_name_id) {
      return &function;
    }
  }
  return nullptr;
}

bir::Function* find_function(prepare::PreparedBirModule& prepared, const char* function_name) {
  for (auto& function : prepared.module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

bir::Block* find_block(bir::Function& function, const char* block_label) {
  for (auto& block : function.blocks) {
    if (block.label == block_label) {
      return &block;
    }
  }
  return nullptr;
}

}  // namespace

int main() {
  auto prepared = legalize_short_circuit_or_guard_module();
  const std::string dump = prepare::print(prepared);

  if (!expect_contains(dump, "prepared.module target=riscv64-unknown-linux-gnu",
                       "module header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "--- prepared-bir ---", "prepared bir section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "prepared.func @short_circuit_or_prepare_contract",
                       "prepared function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "branch_condition entry", "branch condition entry")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "join_transfer logic.end.10", "join transfer")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "carrier=select_materialization",
                       "select materialization carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "ownership=authoritative_branch_pair incomings=2 edge_transfers=2",
                       "join transfer ownership summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "source_branch=entry", "source branch ownership")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "source_transfer_indexes=(0, 1)",
                       "join transfer source transfer indexes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "continuation_targets=(block_1, block_2)",
                       "join transfer continuation targets")) {
    return EXIT_FAILURE;
  }

  auto* control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  auto* function = find_function(prepared, "short_circuit_or_prepare_contract");
  if (control_flow == nullptr || function == nullptr) {
    std::cerr << "[FAIL] missing prepared short-circuit control-flow fixture\n";
    return EXIT_FAILURE;
  }
  auto* join_block = find_block(*function, "logic.end.10");
  if (join_block == nullptr) {
    std::cerr << "[FAIL] missing prepared short-circuit join block fixture\n";
    return EXIT_FAILURE;
  }
  join_block->insts.clear();
  control_flow->branch_conditions.erase(
      std::remove_if(control_flow->branch_conditions.begin(),
                     control_flow->branch_conditions.end(),
                     [&](const prepare::PreparedBranchCondition& branch_condition) {
                       return prepare::prepared_block_label(prepared.names,
                                                            branch_condition.block_label) !=
                              "entry";
                     }),
      control_flow->branch_conditions.end());

  const std::string published_dump = prepare::print(prepared);
  if (!expect_contains(published_dump, "continuation_targets=(block_1, block_2)",
                       "published join transfer continuation targets")) {
    return EXIT_FAILURE;
  }

  auto* mutable_control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  if (mutable_control_flow == nullptr || mutable_control_flow->join_transfers.empty()) {
    std::cerr << "[FAIL] missing mutable authoritative join transfer for printer contract\n";
    return EXIT_FAILURE;
  }
  mutable_control_flow->join_transfers.front().continuation_true_label.reset();
  mutable_control_flow->join_transfers.front().continuation_false_label.reset();

  const std::string unpublished_dump = prepare::print(prepared);
  if (!expect_not_contains(unpublished_dump, "continuation_targets=(",
                           "recomputed continuation targets after removing publication")) {
    return EXIT_FAILURE;
  }

  const auto loop_prepared = legalize_loop_countdown_module();
  const std::string loop_dump = prepare::print(loop_prepared);
  if (!expect_contains(loop_dump, "prepared.func @loop_countdown_prepare_contract",
                       "loop printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "join_transfer loop result=counter kind=loop_carry carrier=edge_store_slot",
                       "loop-carry join header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "ownership=per_edge incomings=2 edge_transfers=2 storage=counter.phi",
                       "slot-backed loop-carry authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump, "incoming [entry] -> 3", "loop entry incoming")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump, "incoming [body] -> next", "loop backedge incoming")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "edge_transfer body -> loop incoming=next destination=counter storage=counter.phi",
                       "loop backedge edge transfer")) {
    return EXIT_FAILURE;
  }

  const auto parallel_copy_prepared = legalize_parallel_copy_cycle_module();
  const std::string parallel_copy_dump = prepare::print(parallel_copy_prepared);
  if (!expect_contains(parallel_copy_dump, "prepared.func @parallel_copy_prepare_contract",
                       "parallel-copy printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy entry -> loop has_cycle=no resolution=acyclic moves=2 steps=2",
                       "acyclic parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy body -> loop has_cycle=yes resolution=cycle_break moves=2 steps=3",
                       "cycle-break parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[0] save_destination_to_temp move_index=0 uses_cycle_temp_source=no",
                       "cycle temp save step")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[1] move move_index=0 uses_cycle_temp_source=no",
                       "non-temp move step")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[2] move move_index=1 uses_cycle_temp_source=yes",
                       "temp-fed move step")) {
    return EXIT_FAILURE;
  }

  const auto call_wrapper_prepared = prepare_call_wrapper_dump_module();
  const std::string call_wrapper_dump = prepare::print(call_wrapper_prepared);
  if (!expect_contains(call_wrapper_dump,
                       "callsite block=0 inst=0 wrapper=same_module callee=same_module_i32 variadic_fpr_args=0",
                       "same-module wrapper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "callsite block=0 inst=1 wrapper=direct_extern_fixed_arity callee=extern_fixed_i32 variadic_fpr_args=0",
                       "fixed extern wrapper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "callsite block=0 inst=2 wrapper=direct_extern_variadic callee=extern_variadic_i32 variadic_fpr_args=1",
                       "variadic extern wrapper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "callsite block=0 inst=3 wrapper=indirect variadic_fpr_args=0 args=1 indirect_callee=callee.ptr indirect_home=register indirect_bank=gpr indirect_value_id=",
                       "indirect wrapper summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "call block_index=0 inst_index=2 wrapper_kind=direct_extern_variadic variadic_fpr_arg_register_count=1 indirect=no callee=extern_variadic_i32",
                       "variadic wrapper call-plan detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "call block_index=0 inst_index=3 wrapper_kind=indirect variadic_fpr_arg_register_count=0 indirect=yes indirect_callee=callee.ptr indirect_encoding=register indirect_bank=gpr indirect_value_id=",
                       "indirect wrapper call-plan detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "arg index=0 value_bank=gpr source_encoding=immediate source_literal=5",
                       "indirect wrapper immediate argument detail")) {
    return EXIT_FAILURE;
  }

  const auto memory_return_prepared = prepare_memory_return_call_dump_module();
  const std::string memory_return_dump = prepare::print(memory_return_prepared);
  if (!expect_contains(memory_return_dump,
                       "callsite block=0 inst=0 wrapper=direct_extern_fixed_arity callee=extern_make_pair variadic_fpr_args=0 args=2 memory_return=lv.call.sret.storage memory_home=frame_slot sret_arg=0",
                       "memory-return summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(memory_return_dump,
                       "call block_index=0 inst_index=0 wrapper_kind=direct_extern_fixed_arity variadic_fpr_arg_register_count=0 indirect=no callee=extern_make_pair memory_return=lv.call.sret.storage memory_encoding=frame_slot sret_arg_index=0",
                       "memory-return call-plan detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(memory_return_dump,
                       "arg index=1 value_bank=gpr source_encoding=immediate source_literal=13",
                       "memory-return immediate argument detail")) {
    return EXIT_FAILURE;
  }

  const auto source_shape_prepared = prepare_call_argument_source_shape_dump_module();
  const std::string source_shape_dump = prepare::print(source_shape_prepared);
  const auto* source_shape_storage =
      find_storage_plan_function(source_shape_prepared, "call_argument_source_shape_dump_contract");
  const auto* loaded_ptr =
      source_shape_storage == nullptr
          ? nullptr
          : find_storage_value(source_shape_prepared, *source_shape_storage, "loaded.ptr");
  if (loaded_ptr == nullptr) {
    std::cerr << "missing loaded.ptr storage publication\n";
    return EXIT_FAILURE;
  }
  const std::string computed_summary =
      "arg1 bank=gpr from=computed_address:loaded.ptr#" + std::to_string(loaded_ptr->value_id) + "+4";
  const std::string computed_detail_id =
      "source_base_value_id=" + std::to_string(loaded_ptr->value_id);
  const std::string computed_detail_shape = "source_base=loaded.ptr source_delta=4";
  if (!expect_contains(source_shape_dump,
                       "arg0 bank=gpr from=symbol_address:@extern_data",
                       "symbol-address argument summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       computed_summary,
                       "computed-address argument summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "arg index=0 value_bank=gpr source_encoding=symbol_address",
                       "symbol-address argument detail encoding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "source_symbol=@extern_data",
                       "symbol-address argument detail payload")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       "arg index=1 value_bank=gpr source_encoding=computed_address",
                       "computed-address argument detail encoding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       computed_detail_id,
                       "computed-address argument detail id")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(source_shape_dump,
                       computed_detail_shape,
                       "computed-address argument detail payload")) {
    return EXIT_FAILURE;
  }

  const auto cross_call_prepared = prepare_cross_call_preservation_dump_module();
  const auto* cross_call_storage =
      find_storage_plan_function(cross_call_prepared, "cross_call_preservation_dump_contract");
  const auto* cross_call_carry =
      cross_call_storage == nullptr ? nullptr : find_storage_value(cross_call_prepared, *cross_call_storage, "carry");
  const auto cross_call_function_id =
      cross_call_prepared.names.function_names.find("cross_call_preservation_dump_contract");
  const auto* cross_call_frame_plan =
      cross_call_function_id == c4c::kInvalidFunctionName
          ? nullptr
          : prepare::find_prepared_frame_plan(cross_call_prepared, cross_call_function_id);
  if (cross_call_storage == nullptr || cross_call_carry == nullptr || cross_call_frame_plan == nullptr) {
    std::cerr << "[FAIL] missing prepared carry storage fixture for cross-call preservation dump\n";
    return EXIT_FAILURE;
  }
  const auto cross_call_saved_it = std::find_if(cross_call_frame_plan->saved_callee_registers.begin(),
                                                cross_call_frame_plan->saved_callee_registers.end(),
                                                [](const auto& saved) {
                                                  return saved.register_name == "s1";
                                                });
  if (cross_call_saved_it == cross_call_frame_plan->saved_callee_registers.end()) {
    std::cerr << "[FAIL] missing published save authority for s1 in cross-call dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::size_t cross_call_save_index = cross_call_saved_it->save_index;
  const std::string cross_call_dump = prepare::print(cross_call_prepared);
  if (!expect_contains(cross_call_dump,
                       "preserves=carry#" + std::to_string(cross_call_carry->value_id) +
                           ":callee_saved_register:s1:save" + std::to_string(cross_call_save_index),
                       "cross-call preservation summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "preserve value=carry value_id=" + std::to_string(cross_call_carry->value_id) +
                           " route=callee_saved_register save_index=" +
                           std::to_string(cross_call_save_index) + " reg=s1 bank=gpr",
                       "cross-call preservation detail")) {
    return EXIT_FAILURE;
  }

  const auto stack_cross_call_prepared = prepare_stack_cross_call_preservation_dump_module();
  const auto* stack_cross_call_plans =
      find_call_plans_function(stack_cross_call_prepared, "stack_cross_call_preservation_dump_contract");
  if (stack_cross_call_plans == nullptr || stack_cross_call_plans->calls.size() != 1) {
    std::cerr << "[FAIL] missing prepared stack-preservation call fixture for dump\n";
    return EXIT_FAILURE;
  }
  const auto& stack_cross_call = stack_cross_call_plans->calls.front();
  const auto stack_preserved_it = std::find_if(
      stack_cross_call.preserved_values.begin(),
      stack_cross_call.preserved_values.end(),
      [](const auto& preserved) {
        return preserved.route == prepare::PreparedCallPreservationRoute::StackSlot;
      });
  if (stack_preserved_it == stack_cross_call.preserved_values.end() ||
      !stack_preserved_it->slot_id.has_value() ||
      !stack_preserved_it->stack_offset_bytes.has_value()) {
    std::cerr << "[FAIL] missing stack-slot preservation authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string stack_cross_call_dump = prepare::print(stack_cross_call_prepared);
  const std::string stack_preserved_summary =
      std::string(prepare::prepared_value_name(stack_cross_call_prepared.names,
                                               stack_preserved_it->value_name)) +
      "#" + std::to_string(stack_preserved_it->value_id) + ":stack_slot:slot#" +
      std::to_string(*stack_preserved_it->slot_id) + ":stack+" +
      std::to_string(*stack_preserved_it->stack_offset_bytes);
  if (!expect_contains(stack_cross_call_dump,
                       stack_preserved_summary,
                       "stack-preserved call-slot summary")) {
    return EXIT_FAILURE;
  }

  const auto call_wrapper_call_plans =
      find_call_plans_function(call_wrapper_prepared, "call_wrapper_dump_contract");
  if (call_wrapper_call_plans == nullptr || call_wrapper_call_plans->calls.size() < 2 ||
      call_wrapper_call_plans->calls[1].clobbered_registers.empty()) {
    std::cerr << "[FAIL] missing prepared clobber fixture for call wrapper dump\n";
    return EXIT_FAILURE;
  }
  const std::string fixed_call_clobber_summary =
      "clobbers=" + clobber_summary(call_wrapper_call_plans->calls[1].clobbered_registers.front());
  if (!expect_contains(call_wrapper_dump,
                       fixed_call_clobber_summary,
                       "call summary clobber authority")) {
    return EXIT_FAILURE;
  }
  const std::string fixed_call_clobber_detail =
      clobber_detail(call_wrapper_call_plans->calls[1].clobbered_registers.front());
  if (!expect_contains(call_wrapper_dump,
                       fixed_call_clobber_detail,
                       "call detail clobber authority")) {
    return EXIT_FAILURE;
  }

  const auto stack_arg_prepared = prepare_stack_argument_slot_dump_module();
  const auto* stack_arg_object = find_stack_object(stack_arg_prepared, "p.byval");
  const auto* stack_arg_slot =
      stack_arg_object == nullptr ? nullptr : find_frame_slot(stack_arg_prepared, stack_arg_object->object_id);
  if (stack_arg_object == nullptr || stack_arg_slot == nullptr) {
    std::cerr << "[FAIL] missing prepared byval home slot fixture for stack argument dump\n";
    return EXIT_FAILURE;
  }
  const std::string stack_arg_dump = prepare::print(stack_arg_prepared);
  if (!expect_contains(stack_arg_dump,
                       "arg0 bank=aggregate_address from=frame_slot:stack+0 to=rdi",
                       "stack-backed byval argument summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_arg_dump,
                       "arg index=0 value_bank=aggregate_address source_encoding=frame_slot",
                       "stack-backed byval argument detail encoding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_arg_dump,
                       "source_slot=#" + std::to_string(stack_arg_slot->slot_id),
                       "stack-backed byval argument frame-slot identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_arg_dump,
                       "source_stack_offset=" + std::to_string(stack_arg_slot->offset_bytes),
                       "stack-backed byval argument stack offset")) {
    return EXIT_FAILURE;
  }

  const auto stack_result_prepared = prepare_stack_result_slot_dump_module();
  const auto* stack_result_storage =
      find_storage_plan_function(stack_result_prepared, "stack_result_slot_dump_contract");
  const auto* stack_result_value =
      stack_result_storage == nullptr
          ? nullptr
          : find_storage_value(stack_result_prepared, *stack_result_storage, "tmp.call.1");
  if (stack_result_storage == nullptr || stack_result_value == nullptr ||
      !stack_result_value->slot_id.has_value() || !stack_result_value->stack_offset_bytes.has_value()) {
    std::cerr << "[FAIL] missing prepared spilled-result fixture for call result dump\n";
    return EXIT_FAILURE;
  }
  const std::string stack_result_dump = prepare::print(stack_result_prepared);
  if (!expect_contains(stack_result_dump,
                       "result bank=gpr from=register:",
                       "stack-backed scalar result summary source shape")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_result_dump,
                       "result value_bank=gpr source_storage=register destination_storage=stack_slot",
                       "stack-backed scalar result detail encoding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_result_dump,
                       "destination_value_id=" + std::to_string(stack_result_value->value_id),
                       "stack-backed scalar result direct owner identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_result_dump,
                       "destination_slot=#" + std::to_string(*stack_result_value->slot_id),
                       "stack-backed scalar result frame-slot identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_result_dump,
                       "dest_stack_offset=" + std::to_string(*stack_result_value->stack_offset_bytes),
                       "stack-backed scalar result stack offset")) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
