#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
#include "src/backend/mir/x86/x86.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

c4c::TargetProfile riscv_target_profile() {
  return c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");
}

c4c::TargetProfile aarch64_target_profile() {
  return c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
}

void set_register_group_override(prepare::PreparedBirModule& prepared,
                                 std::string_view function_name,
                                 std::string_view value_name,
                                 prepare::PreparedRegisterClass register_class,
                                 std::size_t contiguous_width) {
  const auto function_id = prepared.names.function_names.find(function_name);
  const auto value_id = prepared.names.value_names.find(value_name);
  if (function_id == c4c::kInvalidFunctionName || value_id == c4c::kInvalidValueName) {
    return;
  }
  prepared.register_group_overrides.values.push_back(prepare::PreparedRegisterGroupOverride{
      .function_name = function_id,
      .value_name = value_id,
      .register_class = register_class,
      .contiguous_width = contiguous_width,
  });
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

prepare::PreparedBirModule legalize_critical_edge_parallel_copy_module() {
  bir::Module module;

  bir::Function function;
  function.name = "critical_edge_parallel_copy_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(0),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .true_label = "left",
      .false_label = "right",
  };

  bir::Block left;
  left.label = "left";
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
  });
  left.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond1"),
      .true_label = "join",
      .false_label = "exit",
  };

  bir::Block right;
  right.label = "right";
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(-1)};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{.label = "left", .value = bir::Value::immediate_i32(11)},
          bir::PhiIncoming{.label = "right", .value = bir::Value::immediate_i32(22)},
      },
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
  };

  function.blocks = {
      std::move(entry),
      std::move(left),
      std::move(right),
      std::move(exit),
      std::move(join),
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

  const c4c::LinkNameId same_module_link_name_id =
      module.names.link_names.intern("same_module_i32");

  bir::Function same_module_callee;
  same_module_callee.name = "same_module_i32";
  same_module_callee.link_name_id = same_module_link_name_id;
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

  bir::Function variadic_entry;
  variadic_entry.name = "variadic_entry_dump_contract";
  variadic_entry.is_variadic = true;
  variadic_entry.return_type = bir::TypeKind::I32;
  variadic_entry.params.push_back(bir::Param{
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
  variadic_entry.params.push_back(bir::Param{
      .type = bir::TypeKind::F64,
      .name = "scale",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      },
  });
  bir::Block variadic_entry_block;
  variadic_entry_block.label = "entry";
  variadic_entry_block.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "head")};
  variadic_entry.blocks.push_back(std::move(variadic_entry_block));
  module.functions.push_back(std::move(variadic_entry));

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
      .callee_link_name_id = same_module_link_name_id,
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

prepare::PreparedBirModule prepare_aapcs64_variadic_entry_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "aapcs64_variadic_entry_dump_contract";
  function.is_variadic = true;
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
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
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F64,
      .name = "scale",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_start.p0",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "head")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      options);
}

prepare::PreparedBirModule prepare_aapcs64_variadic_entry_helper_family_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "aapcs64_variadic_entry_helper_family_dump_contract";
  function.is_variadic = true;
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
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
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F64,
      .name = "scale",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      },
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "aggregate.byval",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Memory,
          .passed_in_register = false,
          .byval_copy = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_start.p0",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "next.i32"),
      .callee = "llvm.va_arg.i32",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::F64, "next.f64"),
      .callee = "llvm.va_arg.f64",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "double",
      .return_type = bir::TypeKind::F64,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_arg.aggregate",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "next.aggregate"),
               bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .arg_abi =
          {bir::CallArgAbiInfo{
               .type = bir::TypeKind::Ptr,
               .size_bytes = 8,
               .align_bytes = 4,
               .primary_class = bir::AbiValueClass::Memory,
               .sret_pointer = true,
           },
           bir::CallArgAbiInfo{
               .type = bir::TypeKind::Ptr,
               .size_bytes = 8,
               .align_bytes = 8,
               .primary_class = bir::AbiValueClass::Integer,
               .passed_in_register = true,
           }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "missing.aggregate"),
      .callee = "llvm.va_arg.aggregate",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_copy.p0.p0",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "dst.ap"),
               bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "head")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
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
  const c4c::LinkNameId extern_data_id = module.names.link_names.intern("extern_data");
  module.globals.push_back(bir::Global{
      .name = "extern_data",
      .link_name_id = extern_data_id,
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
          bir::Value::named_symbol_pointer("", extern_data_id),
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

prepare::PreparedBirModule prepare_grouped_cross_call_preservation_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "vec_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "grouped_cross_call_preservation_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.vcarry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.local",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry.pre"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.vcarry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.arg"),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "vec_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "post.local"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.local"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.local"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "out"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry.pre"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "post.local"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "out")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule seeded;
  seeded.module = std::move(module);
  seeded.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_legalize();
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  set_register_group_override(prepared,
                              "grouped_cross_call_preservation_dump_contract",
                              "carry.pre",
                              prepare::PreparedRegisterClass::Vector,
                              2);
  set_register_group_override(prepared,
                              "grouped_cross_call_preservation_dump_contract",
                              "post.local",
                              prepare::PreparedRegisterClass::Vector,
                              1);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_grouped_spill_reload_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "vec_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "grouped_spill_reload_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.carry"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "seed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(99),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "vec_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "local0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(2),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(6),
      .rhs = bir::Value::immediate_i32(7),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot"),
      .rhs = bir::Value::immediate_i32(7),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "local0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "merge0"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "merge")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule seeded;
  seeded.module = std::move(module);
  seeded.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  set_register_group_override(prepared,
                              "grouped_spill_reload_dump_contract",
                              "carry",
                              prepare::PreparedRegisterClass::Vector,
                              16);
  set_register_group_override(prepared,
                              "grouped_spill_reload_dump_contract",
                              "local0",
                              prepare::PreparedRegisterClass::Vector,
                              16);
  set_register_group_override(prepared,
                              "grouped_spill_reload_dump_contract",
                              "hot",
                              prepare::PreparedRegisterClass::Vector,
                              16);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_general_grouped_spill_reload_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "spill_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "general_grouped_spill_reload_dump_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.seed",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.carry"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.seed"),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "spill_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot"),
      .rhs = bir::Value::immediate_i32(7),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix5"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix6"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix5"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix6"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "merge")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule seeded;
  seeded.module = std::move(module);
  seeded.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  set_register_group_override(prepared,
                              "general_grouped_spill_reload_dump_contract",
                              "carry",
                              prepare::PreparedRegisterClass::General,
                              2);
  set_register_group_override(prepared,
                              "general_grouped_spill_reload_dump_contract",
                              "hot",
                              prepare::PreparedRegisterClass::General,
                              2);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_float_grouped_spill_reload_dump_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "spill_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "float_grouped_spill_reload_dump_contract";
  function.return_type = bir::TypeKind::F32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.seed",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "carry"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "p.carry"),
      .rhs = bir::Value::immediate_f32_bits(0x3f800000U),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "p.seed"),
      .rhs = bir::Value::immediate_f32_bits(0x40000000U),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "spill_sink",
      .args = {bir::Value::named(bir::TypeKind::F32, "p.arg")},
      .arg_types = {bir::TypeKind::F32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix0"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot"),
      .rhs = bir::Value::immediate_f32_bits(0x40e00000U),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix1"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix2"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix3"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix2"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix4"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix5"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix4"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix6"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix5"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "merge"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot.mix6"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::F32, "merge")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule seeded;
  seeded.module = std::move(module);
  seeded.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  set_register_group_override(prepared,
                              "float_grouped_spill_reload_dump_contract",
                              "carry",
                              prepare::PreparedRegisterClass::Float,
                              2);
  set_register_group_override(prepared,
                              "float_grouped_spill_reload_dump_contract",
                              "hot",
                              prepare::PreparedRegisterClass::Float,
                              2);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

const prepare::PreparedRegallocValue* find_regalloc_value(
    const prepare::PreparedRegallocFunction& function,
    prepare::PreparedValueId value_id) {
  for (const auto& value : function.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
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

const prepare::PreparedF128Carrier* find_f128_carrier(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name,
    std::string_view value_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  const auto value_id = prepared.names.value_names.find(value_name);
  if (function_id == c4c::kInvalidFunctionName || value_id == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* function_carriers = prepare::find_prepared_f128_carriers(prepared, function_id);
  if (function_carriers == nullptr) {
    return nullptr;
  }
  return prepare::find_prepared_f128_carrier(*function_carriers, value_id);
}

const prepare::PreparedF128RuntimeHelper* find_first_f128_runtime_helper(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  const auto* function_helpers =
      prepare::find_prepared_f128_runtime_helpers(prepared, function_id);
  if (function_helpers == nullptr || function_helpers->helpers.empty()) {
    return nullptr;
  }
  return &function_helpers->helpers.front();
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
                       std::string(prepare::prepared_register_bank_name(clobber.bank));
  if (clobber.placement.has_value()) {
    detail += " placement=" +
              std::string(prepare::prepared_register_bank_name(clobber.placement->bank)) +
              ":" + std::string(prepare::prepared_register_slot_pool_name(clobber.placement->pool)) +
              "#" + std::to_string(clobber.placement->slot_index) + "/w" +
              std::to_string(clobber.placement->contiguous_width);
  }
  detail += " reg=" + clobber.register_name;
  detail += " width=" + std::to_string(clobber.contiguous_width);
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

std::string register_placement_text(
    const std::optional<prepare::PreparedRegisterPlacement>& placement,
    std::string_view label = "placement") {
  if (!placement.has_value()) {
    return std::string(label) + "=<missing>";
  }
  return std::string(label) + "=" +
         std::string(prepare::prepared_register_bank_name(placement->bank)) + ":" +
         std::string(prepare::prepared_register_slot_pool_name(placement->pool)) + "#" +
         std::to_string(placement->slot_index) + "/w" +
         std::to_string(placement->contiguous_width);
}

std::string spill_slot_placement_text(
    const std::optional<prepare::PreparedSpillSlotPlacement>& placement,
    std::string_view label = "spill_slot") {
  if (!placement.has_value()) {
    return std::string(label) + "=<missing>";
  }
  return std::string(label) + "=slot#" + std::to_string(placement->slot_id) +
         "+stack" + std::to_string(placement->offset_bytes);
}

std::string saved_register_slot_placement_text(
    const std::optional<prepare::PreparedSavedRegisterSlotPlacement>& placement) {
  if (!placement.has_value()) {
    return "slot_placement=<missing>";
  }
  std::string text = "slot_placement=";
  text += placement->slot_id.has_value() ? "slot#" + std::to_string(*placement->slot_id)
                                         : "<none>";
  text += "+stack";
  text += placement->stack_offset_bytes.has_value()
              ? std::to_string(*placement->stack_offset_bytes)
              : "<unknown>";
  text += " slot_size=";
  text += placement->size_bytes.has_value() ? std::to_string(*placement->size_bytes)
                                            : "<unknown>";
  text += " slot_align=";
  text += placement->align_bytes.has_value() ? std::to_string(*placement->align_bytes)
                                             : "<unknown>";
  text += " fixed_location=";
  text += placement->fixed_location ? "yes" : "no";
  text += " slot_reg=";
  text += std::string(prepare::prepared_register_bank_name(placement->bank)) + ":" +
          placement->register_name;
  text += " slot_save_index=" + std::to_string(placement->save_index);
  text += " slot_width=" + std::to_string(placement->contiguous_width);
  if (!placement->occupied_register_names.empty()) {
    text += " slot_units=";
    for (std::size_t index = 0; index < placement->occupied_register_names.size(); ++index) {
      if (index != 0) {
        text += ",";
      }
      text += placement->occupied_register_names[index];
    }
  }
  text += " " + register_placement_text(placement->register_placement,
                                         "slot_register_placement");
  return text;
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

prepare::PreparedParallelCopyBundle* find_parallel_copy_bundle(
    prepare::PreparedBirModule& prepared,
    const char* function_name,
    const char* predecessor_label,
    const char* successor_label) {
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return nullptr;
  }
  const auto predecessor_id = prepared.names.block_labels.find(predecessor_label);
  const auto successor_id = prepared.names.block_labels.find(successor_label);
  if (predecessor_id == c4c::kInvalidBlockLabel ||
      successor_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  for (auto& bundle : control_flow->parallel_copy_bundles) {
    if (bundle.predecessor_label == predecessor_id &&
        bundle.successor_label == successor_id) {
      return &bundle;
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

prepare::PreparedBirModule prepare_i128_runtime_helper_mapping_dump_module() {
  prepare::PreparedBirModule seeded;
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "i128_runtime_helper_mapping_dump_contract";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I128,
      .name = "lhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I128,
      .name = "rhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F64,
      .name = "fp",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::SDiv,
      .result = bir::Value::named(bir::TypeKind::I128, "wide.div"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "lhs"),
      .rhs = bir::Value::named(bir::TypeKind::I128, "rhs"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::URem,
      .result = bir::Value::named(bir::TypeKind::I128, "wide.rem"),
      .operand_type = bir::TypeKind::I128,
      .lhs = bir::Value::named(bir::TypeKind::I128, "wide.div"),
      .rhs = bir::Value::named(bir::TypeKind::I128, "rhs"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPToSI,
      .result = bir::Value::named(bir::TypeKind::I128, "from.fp"),
      .operand = bir::Value::named(bir::TypeKind::F64, "fp"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::UIToFP,
      .result = bir::Value::named(bir::TypeKind::F64, "to.fp"),
      .operand = bir::Value::named(bir::TypeKind::I128, "rhs"),
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  seeded.module = std::move(module);
  seeded.target_profile = c4c::default_target_profile(c4c::TargetArch::X86_64);

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(seeded), options);
  planner.run_legalize();
  planner.run_stack_layout();
  planner.run_liveness();

  auto prepared = std::move(planner.prepared());
  for (const std::string_view value_name : {"lhs", "rhs", "wide.div", "wide.rem", "from.fp"}) {
    set_register_group_override(prepared,
                                "i128_runtime_helper_mapping_dump_contract",
                                value_name,
                                prepare::PreparedRegisterClass::General,
                                2);
  }

  options.run_legalize = false;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(prepared), options);
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_f128_memory_carrier_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "f128_memory_carrier_dump_contract";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "p.value",
      .size_bytes = 16,
      .align_bytes = 16,
      .is_byval = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

prepare::PreparedBirModule prepare_f128_register_carrier_dump_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "f128_register_carrier_dump_contract";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "p.value",
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
}

prepare::PreparedBirModule prepare_f128_soft_float_helper_dump_module(
    bir::BinaryOpcode opcode = bir::BinaryOpcode::Add,
    std::string function_name_spelling = "f128_soft_float_helper_dump_contract",
    std::string result_name_spelling = "sum") {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = std::move(function_name_spelling);
  function.return_type = bir::TypeKind::F128;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "lhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "rhs",
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = opcode,
      .result = bir::Value::named(bir::TypeKind::F128, result_name_spelling),
      .operand_type = bir::TypeKind::F128,
      .lhs = bir::Value::named(bir::TypeKind::F128, "lhs"),
      .rhs = bir::Value::named(bir::TypeKind::F128, "rhs"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::F128, result_name_spelling),
  };
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  return prepare::prepare_semantic_bir_module_with_options(
      module,
      aarch64_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
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

  auto parallel_copy_prepared = legalize_parallel_copy_cycle_module();
  const std::string parallel_copy_dump = prepare::print(parallel_copy_prepared);
  if (!expect_contains(parallel_copy_dump, "prepared.func @parallel_copy_prepare_contract",
                       "parallel-copy printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy entry -> loop execution_site=predecessor_terminator execution_block=entry has_cycle=no resolution=acyclic moves=2 steps=2",
                       "acyclic parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy body -> loop execution_site=predecessor_terminator execution_block=body has_cycle=yes resolution=cycle_break moves=2 steps=3",
                       "cycle-break parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "authority execution_site=predecessor_terminator execution_block=entry",
                       "acyclic parallel-copy authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "authority execution_site=predecessor_terminator execution_block=body",
                       "cycle-break parallel-copy authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[0] save_destination_to_temp move_index=0 save_destination=a blocked_source=b temp_source=cycle_temp(a) carrier=edge_store_slot storage=a.phi uses_cycle_temp_source=no",
                       "cycle temp save step")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[1] move move_index=0 source=b destination=a carrier=edge_store_slot storage=a.phi uses_cycle_temp_source=no",
                       "non-temp move step")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[2] move move_index=1 source=cycle_temp(a) destination=b carrier=edge_store_slot storage=b.phi uses_cycle_temp_source=yes",
                       "temp-fed move step")) {
    return EXIT_FAILURE;
  }
  auto* mutable_body_bundle = find_parallel_copy_bundle(
      parallel_copy_prepared, "parallel_copy_prepare_contract", "body", "loop");
  if (mutable_body_bundle == nullptr) {
    std::cerr << "[FAIL] missing mutable backedge bundle for printer authority contract\n";
    return EXIT_FAILURE;
  }
  mutable_body_bundle->execution_block_label.reset();
  const std::string unpublished_parallel_copy_dump = prepare::print(parallel_copy_prepared);
  if (!expect_contains(unpublished_parallel_copy_dump,
                       "authority execution_site=predecessor_terminator execution_block=<none>",
                       "parallel-copy authority after removing publication")) {
    return EXIT_FAILURE;
  }
  if (!expect_not_contains(unpublished_parallel_copy_dump,
                           "authority execution_site=predecessor_terminator execution_block=body",
                           "recomputed parallel-copy execution block after removing publication")) {
    return EXIT_FAILURE;
  }

  const auto critical_edge_prepared = legalize_critical_edge_parallel_copy_module();
  const std::string critical_edge_dump = prepare::print(critical_edge_prepared);
  if (!expect_contains(critical_edge_dump,
                       "prepared.func @critical_edge_parallel_copy_prepare_contract",
                       "critical-edge printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "parallel_copy left -> join execution_site=critical_edge execution_block=<none> has_cycle=no resolution=acyclic moves=1 steps=1",
          "critical-edge parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "parallel_copy right -> join execution_site=predecessor_terminator execution_block=right has_cycle=no resolution=acyclic moves=1 steps=1",
          "linear-edge parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "authority execution_site=critical_edge execution_block=<none>",
          "critical-edge parallel-copy authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "authority execution_site=predecessor_terminator execution_block=right",
          "linear-edge parallel-copy authority")) {
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
  const auto entry_function_id =
      call_wrapper_prepared.names.function_names.find("variadic_entry_dump_contract");
  const auto extern_function_id =
      call_wrapper_prepared.names.function_names.find("extern_variadic_i32");
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(call_wrapper_prepared, entry_function_id);
  const auto* extern_plan =
      prepare::find_prepared_variadic_entry_plan(call_wrapper_prepared, extern_function_id);
  if (entry_plan == nullptr || entry_plan->named_parameter_count != 2 ||
      entry_plan->named_register_counts.gp != std::optional<std::size_t>{1} ||
      entry_plan->named_register_counts.fp != std::optional<std::size_t>{1} ||
      entry_plan->register_save_area.required ||
      entry_plan->overflow_area.required ||
      entry_plan->va_list_layout.required ||
      extern_plan != nullptr) {
    std::cerr << "[FAIL] variadic entry prepared carrier did not stay distinct from call wrappers or extern declarations\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "prepared.summary @variadic_entry_dump_contract stable_base=rsp frame_size=0 frame_alignment=1 has_dynamic_stack=no saved_regs=0 calls=0 dynamic_stack_ops=0 variadic_entry=yes",
                       "variadic entry summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "prepared.func @variadic_entry_dump_contract named_params=2 named_gp=1 named_fp=1 helpers=0",
                       "variadic entry plan header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "register_save_area required=no size=<unknown> align=<unknown> slot=<none>",
                       "variadic entry register-save-area placeholder")) {
    return EXIT_FAILURE;
  }
  const auto aapcs64_variadic_prepared = prepare_aapcs64_variadic_entry_dump_module();
  const std::string aapcs64_variadic_dump = prepare::print(aapcs64_variadic_prepared);
  const auto aapcs64_function_id =
      aapcs64_variadic_prepared.names.function_names.find("aapcs64_variadic_entry_dump_contract");
  const auto* aapcs64_entry_plan =
      prepare::find_prepared_variadic_entry_plan(aapcs64_variadic_prepared, aapcs64_function_id);
  if (aapcs64_entry_plan == nullptr ||
      aapcs64_entry_plan->named_parameter_count != 2 ||
      aapcs64_entry_plan->named_register_counts.gp != std::optional<std::size_t>{1} ||
      aapcs64_entry_plan->named_register_counts.fp != std::optional<std::size_t>{1} ||
      !aapcs64_entry_plan->register_save_area.required ||
      aapcs64_entry_plan->register_save_area.size_bytes != std::optional<std::size_t>{192} ||
      aapcs64_entry_plan->register_save_area.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{0} ||
      aapcs64_entry_plan->register_save_area.stack_offset_bytes !=
          std::optional<std::size_t>{0} ||
      aapcs64_entry_plan->register_save_area.initial_gp_offset_bytes !=
          std::optional<std::ptrdiff_t>{-56} ||
      aapcs64_entry_plan->register_save_area.initial_fp_offset_bytes !=
          std::optional<std::ptrdiff_t>{-112} ||
      !aapcs64_entry_plan->overflow_area.required ||
      aapcs64_entry_plan->overflow_area.base_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{1} ||
      aapcs64_entry_plan->overflow_area.base_stack_offset_bytes !=
          std::optional<std::size_t>{192} ||
      !aapcs64_entry_plan->va_list_layout.required ||
      aapcs64_entry_plan->va_list_layout.fields.size() != 5 ||
      aapcs64_entry_plan->helper_resources.required_helpers.size() != 1 ||
      aapcs64_entry_plan->helper_resources.required_helpers.front() !=
          prepare::PreparedVariadicEntryHelperKind::VaStart ||
      aapcs64_entry_plan->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{1} ||
      aapcs64_entry_plan->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      aapcs64_entry_plan->helper_operand_homes.size() != 1 ||
      !aapcs64_entry_plan->helper_operand_homes.front()
           .destination_va_list.has_value() ||
      !aapcs64_entry_plan->missing_required_facts.empty()) {
    std::cerr << "[FAIL] AAPCS64 variadic entry carrier did not publish structured ABI facts "
                 "and prepared storage/scratch authority\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_variadic_dump,
          "register_save_area required=yes size=192 align=16 slot=#0 stack_offset=0 gp_offset=0 fp_offset=64 gp_slot=8 fp_slot=16 saved_gp=7 saved_fp=7 initial_gp_offset=-56 initial_fp_offset=-112",
          "AAPCS64 variadic entry register-save facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "overflow_area required=yes base_slot=#1 base_stack_offset=192 align=8",
                       "AAPCS64 variadic overflow-area storage facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "va_list_layout required=yes size=32 align=8 fields=5",
                       "AAPCS64 variadic va_list layout header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "field kind=gp_register_save_area offset=8 size=8",
                       "AAPCS64 variadic GP save-area field")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "field kind=fp_register_save_area offset=16 size=8",
                       "AAPCS64 variadic FP save-area field")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "helper_resources scratch_registers=1 scratch_stack=0 helpers=[va_start]",
                       "AAPCS64 variadic va_start helper scratch facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "helper kind=va_start",
                       "AAPCS64 variadic va_start helper need")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_variadic_dump,
                       "helper_operand kind=va_start block=0 inst=0 dst_va_list=ap:",
                       "AAPCS64 variadic va_start helper operand home")) {
    return EXIT_FAILURE;
  }
  const auto aapcs64_helper_family_prepared =
      prepare_aapcs64_variadic_entry_helper_family_dump_module();
  const std::string aapcs64_helper_family_dump = prepare::print(aapcs64_helper_family_prepared);
  const auto aapcs64_helper_family_function_id =
      aapcs64_helper_family_prepared.names.function_names.find(
          "aapcs64_variadic_entry_helper_family_dump_contract");
  const auto* aapcs64_helper_family_entry_plan =
      prepare::find_prepared_variadic_entry_plan(aapcs64_helper_family_prepared,
                                                 aapcs64_helper_family_function_id);
  if (aapcs64_helper_family_entry_plan == nullptr ||
      aapcs64_helper_family_entry_plan->named_parameter_count != 3 ||
      aapcs64_helper_family_entry_plan->named_register_counts.gp !=
          std::optional<std::size_t>{1} ||
      aapcs64_helper_family_entry_plan->named_register_counts.fp !=
          std::optional<std::size_t>{1} ||
      aapcs64_helper_family_entry_plan->helper_resources.required_helpers.size() != 4 ||
      aapcs64_helper_family_entry_plan->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{2} ||
      aapcs64_helper_family_entry_plan->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      aapcs64_helper_family_entry_plan->helper_operand_homes.size() != 6) {
    std::cerr << "[FAIL] AAPCS64 variadic helper-family carrier lost named counts, helpers, or scratch facts\n";
    return EXIT_FAILURE;
  }
  const auto& helper_family = aapcs64_helper_family_entry_plan->helper_resources.required_helpers;
  for (const auto helper : {
           prepare::PreparedVariadicEntryHelperKind::VaStart,
           prepare::PreparedVariadicEntryHelperKind::VaArg,
           prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
           prepare::PreparedVariadicEntryHelperKind::VaCopy,
       }) {
    if (std::find(helper_family.begin(), helper_family.end(), helper) == helper_family.end()) {
      std::cerr << "[FAIL] AAPCS64 variadic helper-family carrier missed a helper kind\n";
      return EXIT_FAILURE;
    }
  }
  const auto* scalar_i32_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 1);
  const auto* scalar_f64_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 2);
  const auto* aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 3);
  const auto* missing_aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(
          *aapcs64_helper_family_entry_plan, 0, 4);
  if (scalar_i32_homes == nullptr || scalar_f64_homes == nullptr ||
      !scalar_i32_homes->scalar_access_plan.has_value() ||
      scalar_i32_homes->scalar_access_plan->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea ||
      scalar_i32_homes->scalar_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::GpRegisterSaveArea} ||
      scalar_i32_homes->scalar_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{8} ||
      scalar_i32_homes->scalar_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::GpOffset} ||
      scalar_i32_homes->scalar_access_plan->progression_field_offset_bytes !=
          std::optional<std::size_t>{24} ||
      scalar_i32_homes->scalar_access_plan->overflow_source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      scalar_i32_homes->scalar_access_plan->overflow_source_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      !scalar_f64_homes->scalar_access_plan.has_value() ||
      scalar_f64_homes->scalar_access_plan->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea ||
      scalar_f64_homes->scalar_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea} ||
      scalar_f64_homes->scalar_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{16} ||
      scalar_f64_homes->scalar_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::FpOffset} ||
      scalar_f64_homes->scalar_access_plan->progression_field_offset_bytes !=
          std::optional<std::size_t>{28} ||
      scalar_f64_homes->scalar_access_plan->overflow_source_field_offset_bytes !=
          std::optional<std::size_t>{0}) {
    std::cerr << "[FAIL] AAPCS64 variadic helper-family carrier missed scalar va_arg access-plan facts\n";
    return EXIT_FAILURE;
  }
  prepare::PreparedValueHome aggregate_payload_home{
      .value_name = c4c::kInvalidValueName,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = std::size_t{42},
      .offset_bytes = std::size_t{16},
  };
  prepare::PreparedValueHome aggregate_source_va_list_home{
      .value_name = c4c::kInvalidValueName,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string("x0"),
  };
  prepare::PreparedVariadicAggregateVaArgAccessPlan incomplete_aggregate_plan;
  if (prepare::is_complete_prepared_variadic_aggregate_va_arg_access_plan(
          incomplete_aggregate_plan)) {
    std::cerr << "[FAIL] AAPCS64 variadic aggregate va_arg carrier treated an empty access plan as complete\n";
    return EXIT_FAILURE;
  }
  prepare::PreparedVariadicAggregateVaArgAccessPlan complete_aggregate_plan{
      .source_class = prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea,
      .payload_size_bytes = 8,
      .payload_align_bytes = 4,
      .destination_payload_home = aggregate_payload_home,
      .source_field = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
      .source_field_offset_bytes = std::size_t{0},
      .source_payload_offset_bytes = std::size_t{0},
      .source_slot_size_bytes = std::size_t{8},
      .copy_size_bytes = std::size_t{8},
      .copy_align_bytes = std::size_t{4},
      .progression_field = prepare::PreparedVariadicVaListFieldKind::OverflowArgArea,
      .progression_field_offset_bytes = std::size_t{0},
      .progression_stride_bytes = std::size_t{8},
  };
  prepare::PreparedVariadicEntryHelperOperandHomes complete_aggregate_homes{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArgAggregate,
      .source_va_list = aggregate_source_va_list_home,
      .aggregate_destination_payload = aggregate_payload_home,
      .aggregate_access_plan = complete_aggregate_plan,
  };
  if (!prepare::is_complete_prepared_variadic_aggregate_va_arg_access_plan(
          complete_aggregate_plan) ||
      !prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(
          complete_aggregate_homes)) {
    std::cerr << "[FAIL] AAPCS64 variadic aggregate va_arg carrier cannot represent a complete access plan\n";
    return EXIT_FAILURE;
  }
  if (aggregate_homes == nullptr ||
      !aggregate_homes->aggregate_access_plan.has_value() ||
      aggregate_homes->aggregate_access_plan->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      aggregate_homes->aggregate_access_plan->payload_size_bytes != 8 ||
      aggregate_homes->aggregate_access_plan->payload_align_bytes != 4 ||
      !aggregate_homes->aggregate_access_plan->destination_payload_home.has_value() ||
      aggregate_homes->aggregate_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      aggregate_homes->aggregate_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      aggregate_homes->aggregate_access_plan->source_payload_offset_bytes !=
          std::optional<std::size_t>{0} ||
      aggregate_homes->aggregate_access_plan->source_slot_size_bytes !=
          std::optional<std::size_t>{8} ||
      aggregate_homes->aggregate_access_plan->copy_size_bytes !=
          std::optional<std::size_t>{8} ||
      aggregate_homes->aggregate_access_plan->copy_align_bytes !=
          std::optional<std::size_t>{4} ||
      aggregate_homes->aggregate_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      aggregate_homes->aggregate_access_plan->progression_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      aggregate_homes->aggregate_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{8}) {
    std::cerr << "[FAIL] AAPCS64 variadic aggregate va_arg carrier missed prepared access-plan facts\n";
    return EXIT_FAILURE;
  }
  if (missing_aggregate_homes == nullptr ||
      missing_aggregate_homes->aggregate_access_plan.has_value() ||
      std::find(aapcs64_helper_family_entry_plan->missing_required_facts.begin(),
                aapcs64_helper_family_entry_plan->missing_required_facts.end(),
                "helper_operand_homes.va_arg_aggregate.aggregate_access_plan") ==
          aapcs64_helper_family_entry_plan->missing_required_facts.end()) {
    std::cerr << "[FAIL] AAPCS64 variadic aggregate va_arg carrier did not expose the missing aggregate access-plan fact\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "prepared.func @aapcs64_variadic_entry_helper_family_dump_contract named_params=3 named_gp=1 named_fp=1 helpers=4",
          "AAPCS64 variadic helper-family plan header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_resources scratch_registers=2 scratch_stack=0 helpers=[va_start,va_arg,va_arg_aggregate,va_copy]",
                       "AAPCS64 variadic helper-family summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper kind=va_copy",
                       "AAPCS64 variadic va_copy helper need")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_arg block=0 inst=1",
                       "AAPCS64 variadic scalar va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "scalar_access_plan=source_class=gp_register_save_area:type=i32:size=4:align=4",
          "AAPCS64 variadic scalar i32 va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "source_field=gp_register_save_area@8:source_slot=8:progression_field=gp_offset@24:progression_stride=8:overflow_source_field=overflow_arg_area@0:overflow_stride=4",
          "AAPCS64 variadic scalar i32 va_arg source coordinates")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "scalar_access_plan=source_class=fp_register_save_area:type=f64:size=8:align=8",
          "AAPCS64 variadic scalar f64 va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "source_field=fp_register_save_area@16:source_slot=16:progression_field=fp_offset@28:progression_stride=16:overflow_source_field=overflow_arg_area@0:overflow_stride=8",
          "AAPCS64 variadic scalar f64 va_arg source coordinates")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_arg_aggregate block=0 inst=3",
                       "AAPCS64 variadic aggregate va_arg operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "aggregate_access_plan=source_class=overflow_arg_area:payload_size=8:payload_align=4",
          "AAPCS64 variadic aggregate va_arg access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "source_field=overflow_arg_area@0:source_payload_offset=0:source_slot=8:copy_size=8:copy_align=4:progression_field=overflow_arg_area@0:progression_stride=8",
          "AAPCS64 variadic aggregate va_arg source and progression coordinates")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_arg_aggregate block=0 inst=4",
                       "AAPCS64 variadic aggregate va_arg missing operand homes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "aggregate_access_plan=<none>",
                       "AAPCS64 variadic aggregate va_arg explicit missing access plan")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          aapcs64_helper_family_dump,
          "missing fact=helper_operand_homes.va_arg_aggregate.aggregate_access_plan",
          "AAPCS64 variadic aggregate va_arg missing access-plan fact")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(aapcs64_helper_family_dump,
                       "helper_operand kind=va_copy block=0 inst=5",
                       "AAPCS64 variadic va_copy operand homes")) {
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

  const auto i128_helper_prepared = prepare_i128_runtime_helper_mapping_dump_module();
  const std::string i128_helper_dump = prepare::print(i128_helper_prepared);
  if (!expect_contains(i128_helper_dump,
                       "--- prepared-i128-runtime-helpers ---",
                       "i128 runtime helper section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=0 family=div_rem kind=signed_div opcode=sdiv callee=__divti3 source_type=i128 result_type=i128 result=wide.div#",
                       "i128 signed div helper mapping detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result_ownership=direct_low_high_lanes memory_return=<none>",
                       "i128 helper direct-result ownership policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "resources=[call_boundary,runtime_helper_callee,caller_saved_clobbers,"
                       "source_operation_identity]",
                       "i128 helper resource boundary policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "abi_transition=direct_register_pair_arguments_and_result arg_bank=gpr "
                       "result_bank=gpr arg_count=2 lanes_per_arg=2 result_lanes=2 lane_width=8",
                       "i128 helper ABI register-bank transition policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "clobbers=gpr:",
                       "i128 helper call-clobber policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=1 family=div_rem kind=unsigned_rem opcode=urem callee=__umodti3 source_type=i128 result_type=i128 result=wide.rem#",
                       "i128 unsigned rem helper mapping detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=2 family=float_integer_conversion kind=float_to_signed_int opcode=fptosi callee=__fixdfti source_type=f64 result_type=i128 result=from.fp#",
                       "i128 float-to-signed conversion helper mapping detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "operand=fp#",
                       "i128 conversion helper operand identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "source_width=8 result_width=16 source_signed=no result_signed=yes",
                       "i128 conversion helper signedness and width facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "scalar_ownership operand=fp#",
                       "i128 conversion helper scalar FP source ownership")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "[type=f64,width=8,bank=fpr,home=",
                       "i128 conversion helper scalar FP home facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=3 family=float_integer_conversion kind=unsigned_int_to_float opcode=uitofp callee=__floatuntidf source_type=i128 result_type=f64 result=to.fp#",
                       "i128 unsigned-to-float conversion helper mapping detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "source_width=16 result_width=8 source_signed=no result_signed=no",
                       "i128 conversion helper unsigned source facts")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result_ownership=scalar_value memory_return=<none>",
                       "i128 conversion helper scalar result ownership policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "scalar_ownership operand=<none> result=to.fp#",
                       "i128 conversion helper scalar result ownership")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "i128_helper block=0 inst=2 family=float_integer_conversion "
                       "kind=float_to_signed_int opcode=fptosi callee=__fixdfti",
                       "i128 conversion helper supported boundary detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "resources=[call_boundary,runtime_helper_callee,caller_saved_clobbers,"
                       "source_operation_identity]",
                       "i128 conversion helper resource boundary policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "scalar.operand=scalar_value_to_abi_argument",
                       "i128 conversion helper scalar argument marshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "scalar.result=abi_result_to_scalar_value",
                       "i128 conversion helper scalar result unmarshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "selected_call_ownership=[owns_terminal_call=yes,callee=yes,"
                       "resources=yes,clobbers=yes,abi_bindings=yes,marshaling=yes,"
                       "live_preservation=yes]",
                       "i128 conversion helper selected-call ownership policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "lhs.low=lhs#",
                       "i128 helper lhs low lane identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result.high=wide.rem#",
                       "i128 helper result high lane identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "abi_bindings lhs.low=lhs#",
                       "i128 helper ABI binding section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "arg=0,abi_index=0,bank=gpr,class=general,reg=rdi",
                       "i128 helper lhs low ABI argument register binding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "arg=1,abi_index=3,bank=gpr,class=general,reg=rcx",
                       "i128 helper rhs high ABI argument register binding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result,abi_index=1,bank=gpr,class=general,reg=rdx",
                       "i128 helper high result ABI register binding")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "marshaling lhs.low=carrier_lane_to_abi_argument",
                       "i128 helper marshaling section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "phase=before_call,op=move,value=lhs#",
                       "i128 helper source lane marshaling phase")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "arg=0,abi_index=0,abi_reg=rdi",
                       "i128 helper source lane ABI argument move target")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "carrier_slot=#",
                       "i128 helper source lane memory-backed marshal source")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result.high=abi_result_to_carrier_lane",
                       "i128 helper result unmarshal section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "phase=after_call,op=move,value=wide.div#",
                       "i128 helper result lane unmarshal phase")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result,abi_index=1,abi_reg=rdx",
                       "i128 helper result lane ABI source register")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "live_preservation=[evaluated=yes,caller_saved_clobbers=yes,"
                       "additional=none,preserved=0]",
                       "i128 helper live-preservation policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "selected_call_ownership=[owns_terminal_call=yes,callee=yes,"
                       "resources=yes,clobbers=yes,abi_bindings=yes,marshaling=yes,"
                       "live_preservation=yes]",
                       "i128 helper selected-call ownership policy")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "carrier=memory_backed,slot=",
                       "i128 helper structured memory-backed lane authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(i128_helper_dump,
                       "result_requires_register_pair_carrier",
                       "i128 helper non-register lane fail-closed diagnostic")) {
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
                       "source_symbol_id=",
                       "symbol-address argument detail LinkNameId payload")) {
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
  if (!cross_call_saved_it->placement.has_value() ||
      !cross_call_saved_it->slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(
          *cross_call_saved_it->slot_placement)) {
    std::cerr << "[FAIL] missing structured saved-register placement for s1 in cross-call dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::size_t cross_call_save_index = cross_call_saved_it->save_index;
  const std::string cross_call_dump = prepare::print(cross_call_prepared);
  if (!expect_contains(cross_call_dump,
                       "saved gpr:s1 order=" + std::to_string(cross_call_save_index) +
                           " " + register_placement_text(cross_call_saved_it->placement) +
                           " width=1 units=s1 " +
                           saved_register_slot_placement_text(cross_call_saved_it->slot_placement),
                       "cross-call saved-register slot placement summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "saved_register bank=gpr " +
                           register_placement_text(cross_call_saved_it->placement) +
                           " reg=s1 save_index=" + std::to_string(cross_call_save_index) +
                           " width=1 units=s1 " +
                           saved_register_slot_placement_text(cross_call_saved_it->slot_placement),
                       "cross-call frame-plan saved-register slot placement")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "preserves=carry#" + std::to_string(cross_call_carry->value_id) +
                           ":callee_saved_register:s1[s1]:save" +
                           std::to_string(cross_call_save_index),
                       "cross-call preservation summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(cross_call_dump,
                       "preserve value=carry value_id=" + std::to_string(cross_call_carry->value_id) +
                           " route=callee_saved_register save_index=" +
                           std::to_string(cross_call_save_index) + " " +
                           register_placement_text(cross_call_saved_it->placement) +
                           " reg=s1 bank=gpr units=s1",
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
      !stack_preserved_it->stack_offset_bytes.has_value() ||
      !stack_preserved_it->stack_size_bytes.has_value() ||
      !stack_preserved_it->stack_align_bytes.has_value() ||
      !stack_preserved_it->spill_slot_placement.has_value()) {
    std::cerr << "[FAIL] missing stack-slot preservation authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string stack_cross_call_dump = prepare::print(stack_cross_call_prepared);
  const std::string stack_preserved_summary =
      std::string(prepare::prepared_value_name(stack_cross_call_prepared.names,
                                               stack_preserved_it->value_name)) +
      "#" + std::to_string(stack_preserved_it->value_id) + ":stack_slot:slot#" +
      std::to_string(*stack_preserved_it->slot_id) + ":stack+" +
      std::to_string(*stack_preserved_it->stack_offset_bytes) + ":size=" +
      std::to_string(*stack_preserved_it->stack_size_bytes) + ":align=" +
      std::to_string(*stack_preserved_it->stack_align_bytes);
  if (!expect_contains(stack_cross_call_dump,
                       stack_preserved_summary,
                       "stack-preserved call-slot extent summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(stack_cross_call_dump,
                       "preserve value=" +
                           std::string(prepare::prepared_value_name(stack_cross_call_prepared.names,
                                                                    stack_preserved_it->value_name)) +
                           " value_id=" + std::to_string(stack_preserved_it->value_id) +
                           " route=stack_slot " +
                           spill_slot_placement_text(stack_preserved_it->spill_slot_placement) +
                           " bank=none slot=#" + std::to_string(*stack_preserved_it->slot_id) +
                           " stack_offset=" +
                           std::to_string(*stack_preserved_it->stack_offset_bytes) +
                           " stack_size=" +
                           std::to_string(*stack_preserved_it->stack_size_bytes) +
                           " stack_align=" +
                           std::to_string(*stack_preserved_it->stack_align_bytes),
                       "stack-preserved structured spill-slot extent detail")) {
    return EXIT_FAILURE;
  }

  const auto f128_memory_prepared = prepare_f128_memory_carrier_dump_module();
  const auto* f128_memory_carrier = find_f128_carrier(
      f128_memory_prepared, "f128_memory_carrier_dump_contract", "p.value");
  if (f128_memory_carrier == nullptr ||
      f128_memory_carrier->kind != prepare::PreparedF128CarrierKind::MemoryBacked ||
      f128_memory_carrier->source_type != bir::TypeKind::F128 ||
      f128_memory_carrier->total_size_bytes != 16 ||
      f128_memory_carrier->total_align_bytes != 16 ||
      !f128_memory_carrier->slot_id.has_value() ||
      !f128_memory_carrier->stack_offset_bytes.has_value() ||
      !f128_memory_carrier->missing_required_facts.empty()) {
    std::cerr << "[FAIL] prepared f128 memory carrier lost frame-slot storage authority\n";
    return EXIT_FAILURE;
  }

  const std::string f128_memory_dump = prepare::print(f128_memory_prepared);
  if (!expect_contains(f128_memory_dump,
                       "--- prepared-f128-carriers ---",
                       "f128 carrier printer section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_memory_dump,
                       "f128_carrier p.value value_id=0",
                       "f128 memory carrier value identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_memory_dump,
                       "kind=memory_backed size=16 align=16",
                       "f128 memory carrier storage shape")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_memory_dump,
                       "slot_id=#",
                       "f128 memory carrier frame-slot id")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_memory_dump,
                       "stack_offset=",
                       "f128 memory carrier frame-slot offset")) {
    return EXIT_FAILURE;
  }

  const auto f128_register_prepared = prepare_f128_register_carrier_dump_module();
  const auto* f128_register_carrier = find_f128_carrier(
      f128_register_prepared, "f128_register_carrier_dump_contract", "p.value");
  if (f128_register_carrier == nullptr ||
      f128_register_carrier->kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      f128_register_carrier->source_type != bir::TypeKind::F128 ||
      f128_register_carrier->total_size_bytes != 16 ||
      f128_register_carrier->total_align_bytes != 16 ||
      f128_register_carrier->register_bank != prepare::PreparedRegisterBank::Vreg ||
      f128_register_carrier->register_class != prepare::PreparedRegisterClass::Vector ||
      f128_register_carrier->contiguous_width != 1 ||
      f128_register_carrier->register_name != std::optional<std::string>{"q0"} ||
      f128_register_carrier->occupied_register_names != std::vector<std::string>{"q0"} ||
      !f128_register_carrier->missing_required_facts.empty()) {
    std::cerr << "[FAIL] prepared f128 register carrier lost q-register authority\n";
    return EXIT_FAILURE;
  }

  const std::string f128_register_dump = prepare::print(f128_register_prepared);
  if (!expect_contains(f128_register_dump,
                       "f128_carrier p.value value_id=0",
                       "f128 register carrier value identity")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_register_dump,
                       "kind=full_width_register size=16 align=16 bank=vreg class=vector",
                       "f128 register carrier storage shape")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_register_dump,
                       "width=1 units=q0 reg=q0",
                       "f128 register carrier q-register occupancy")) {
    return EXIT_FAILURE;
  }

  const auto f128_helper_prepared = prepare_f128_soft_float_helper_dump_module();
  const auto* f128_helper = find_first_f128_runtime_helper(
      f128_helper_prepared, "f128_soft_float_helper_dump_contract");
  if (f128_helper == nullptr ||
      f128_helper->helper_family != prepare::PreparedF128RuntimeHelperFamily::Arithmetic ||
      f128_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::Add ||
      f128_helper->callee_name != "__addtf3" ||
      f128_helper->source_type != bir::TypeKind::F128 ||
      f128_helper->result_type != bir::TypeKind::F128 ||
      f128_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      !f128_helper->lhs_carrier.has_value() ||
      !f128_helper->rhs_carrier.has_value() ||
      !f128_helper->result_carrier.has_value() ||
      !f128_helper->lhs_abi_argument.has_value() ||
      !f128_helper->rhs_abi_argument.has_value() ||
      !f128_helper->result_abi_result.has_value() ||
      !f128_helper->lhs_argument_move.has_value() ||
      !f128_helper->rhs_argument_move.has_value() ||
      !f128_helper->result_unmarshal_move.has_value() ||
      !f128_helper->resource_policy.caller_saved_clobbers ||
      f128_helper->clobbered_registers.empty() ||
      f128_helper->lhs_carrier->width_bytes != 16 ||
      f128_helper->rhs_carrier->width_bytes != 16 ||
      f128_helper->result_carrier->width_bytes != 16 ||
      f128_helper->lhs_abi_argument->width_bytes != 16 ||
      f128_helper->rhs_abi_argument->width_bytes != 16 ||
      f128_helper->result_abi_result->width_bytes != 16 ||
      f128_helper->lhs_abi_argument->register_name != "q0" ||
      f128_helper->rhs_abi_argument->register_name != "q1" ||
      f128_helper->result_abi_result->register_name != "q0" ||
      f128_helper->lhs_argument_move->direction !=
          prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument ||
      f128_helper->rhs_argument_move->direction !=
          prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument ||
      f128_helper->result_unmarshal_move->direction !=
          prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier ||
      !f128_helper->live_preservation_policy.evaluated ||
      !f128_helper->live_preservation_policy.caller_saved_clobbers_modeled ||
      !f128_helper->live_preservation_policy.no_additional_live_preservation_required ||
      !f128_helper->live_preservation_policy.preserved_values.empty() ||
      !f128_helper->selected_call_ownership.has_clobber_policy ||
      !f128_helper->selected_call_ownership.has_live_preservation ||
      !f128_helper->selected_call_ownership.owns_terminal_call) {
    std::cerr << "[FAIL] prepared f128 soft-float helper lost structured record authority\n";
    return EXIT_FAILURE;
  }
  const auto f128_vreg_clobber_it = std::find_if(
      f128_helper->clobbered_registers.begin(),
      f128_helper->clobbered_registers.end(),
      [](const prepare::PreparedClobberedRegister& clobber) {
        return clobber.bank == prepare::PreparedRegisterBank::Vreg &&
               clobber.register_name == "v0" &&
               clobber.contiguous_width == 1 &&
               clobber.occupied_register_names.size() == 1 &&
               clobber.occupied_register_names.front() == "v0" &&
               clobber.placement.has_value() &&
               clobber.placement->pool ==
                   prepare::PreparedRegisterSlotPool::ReservedScratch;
      });
  if (f128_vreg_clobber_it == f128_helper->clobbered_registers.end()) {
    std::cerr
        << "[FAIL] prepared f128 soft-float helper lost structured vreg clobber authority\n";
    return EXIT_FAILURE;
  }
  const auto has_f128_missing_fact = [&](std::string_view fact) {
    return std::any_of(f128_helper->missing_required_facts.begin(),
                       f128_helper->missing_required_facts.end(),
                       [&](const std::string& candidate) {
                         return candidate == fact;
                       });
  };
  if (has_f128_missing_fact("f128_helper_boundary_requires_live_preservation_policy") ||
      has_f128_missing_fact("selected_call_ownership_requires_live_preservation_policy") ||
      has_f128_missing_fact("live_preservation_requires_structured_live_across_helper_facts") ||
      has_f128_missing_fact("live_preservation_requires_complete_preserved_value_routes") ||
      has_f128_missing_fact("f128_helper_boundary_requires_caller_saved_clobber_policy") ||
      has_f128_missing_fact("f128_helper_boundary_requires_caller_saved_clobbers") ||
      has_f128_missing_fact("selected_call_ownership_requires_clobber_policy") ||
      has_f128_missing_fact("selected_call_ownership_requires_resource_policy") ||
      has_f128_missing_fact("selected_call_ownership_requires_abi_bindings") ||
      has_f128_missing_fact("selected_call_ownership_requires_marshaling")) {
    std::cerr
        << "[FAIL] prepared f128 soft-float helper did not fail closed on missing authority\n";
    return EXIT_FAILURE;
  }
  const std::string f128_helper_dump = prepare::print(f128_helper_prepared);
  if (!expect_contains(f128_helper_dump,
                       "--- prepared-f128-runtime-helpers ---",
                       "f128 runtime helper printer section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "f128_helper block=0 inst=0 family=arithmetic kind=add opcode=add "
                       "callee=__addtf3 source_type=f128 result_type=f128 result=sum#",
                       "f128 add helper identity and callee")) {
    return EXIT_FAILURE;
  }
  const auto f128_sub_helper_prepared =
      prepare_f128_soft_float_helper_dump_module(
          bir::BinaryOpcode::Sub,
          "f128_soft_float_sub_helper_dump_contract",
          "diff");
  const auto* f128_sub_helper = find_first_f128_runtime_helper(
      f128_sub_helper_prepared, "f128_soft_float_sub_helper_dump_contract");
  if (f128_sub_helper == nullptr ||
      f128_sub_helper->helper_family !=
          prepare::PreparedF128RuntimeHelperFamily::Arithmetic ||
      f128_sub_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::Sub ||
      f128_sub_helper->callee_name != "__subtf3" ||
      f128_sub_helper->source_binary_opcode != bir::BinaryOpcode::Sub ||
      f128_sub_helper->source_type != bir::TypeKind::F128 ||
      f128_sub_helper->result_type != bir::TypeKind::F128 ||
      f128_sub_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      !f128_sub_helper->selected_call_ownership.owns_terminal_call ||
      !f128_sub_helper->selected_call_ownership.has_marshaling ||
      !f128_sub_helper->selected_call_ownership.has_live_preservation) {
    std::cerr << "[FAIL] prepared f128 sub soft-float helper lost structured record authority\n";
    return EXIT_FAILURE;
  }
  const std::string f128_sub_helper_dump = prepare::print(f128_sub_helper_prepared);
  if (!expect_contains(f128_sub_helper_dump,
                       "f128_helper block=0 inst=0 family=arithmetic kind=sub opcode=sub "
                       "callee=__subtf3 source_type=f128 result_type=f128 result=diff#",
                       "f128 sub helper identity and callee")) {
    return EXIT_FAILURE;
  }
  const auto f128_mul_helper_prepared =
      prepare_f128_soft_float_helper_dump_module(
          bir::BinaryOpcode::Mul,
          "f128_soft_float_mul_helper_dump_contract",
          "product");
  const auto* f128_mul_helper = find_first_f128_runtime_helper(
      f128_mul_helper_prepared, "f128_soft_float_mul_helper_dump_contract");
  if (f128_mul_helper == nullptr ||
      f128_mul_helper->helper_family !=
          prepare::PreparedF128RuntimeHelperFamily::Arithmetic ||
      f128_mul_helper->helper_kind != prepare::PreparedF128RuntimeHelperKind::Mul ||
      f128_mul_helper->callee_name != "__multf3" ||
      f128_mul_helper->source_binary_opcode != bir::BinaryOpcode::Mul ||
      f128_mul_helper->source_type != bir::TypeKind::F128 ||
      f128_mul_helper->result_type != bir::TypeKind::F128 ||
      f128_mul_helper->result_ownership !=
          prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier ||
      !f128_mul_helper->selected_call_ownership.owns_terminal_call ||
      !f128_mul_helper->selected_call_ownership.has_marshaling ||
      !f128_mul_helper->selected_call_ownership.has_live_preservation) {
    std::cerr << "[FAIL] prepared f128 mul soft-float helper lost structured record authority\n";
    return EXIT_FAILURE;
  }
  const std::string f128_mul_helper_dump = prepare::print(f128_mul_helper_prepared);
  if (!expect_contains(f128_mul_helper_dump,
                       "f128_helper block=0 inst=0 family=arithmetic kind=mul opcode=mul "
                       "callee=__multf3 source_type=f128 result_type=f128 result=product#",
                       "f128 mul helper identity and callee")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result_ownership=full_width_carrier "
                       "resources=[call_boundary,runtime_helper_callee,caller_saved_clobbers,"
                       "source_operation_identity]",
                       "f128 helper resource record")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "clobbers=gpr:x13/w1[x13],fpr:d13/w1[d13],vreg:v0/w1[v0]",
                       "f128 helper caller-saved clobber summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "carriers lhs=lhs#",
                       "f128 helper lhs carrier record")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "rhs=rhs#",
                       "f128 helper rhs carrier record")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result=sum#",
                       "f128 helper result carrier record")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "abi_bindings lhs=lhs#",
                       "f128 helper lhs ABI binding section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "arg=0,abi_index=0,bank=vreg,class=vector,abi_reg=q0",
                       "f128 helper lhs ABI q-register")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "rhs=rhs#",
                       "f128 helper rhs ABI binding section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "arg=1,abi_index=1,bank=vreg,class=vector,abi_reg=q1",
                       "f128 helper rhs ABI q-register")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result=sum#",
                       "f128 helper result ABI binding section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result,abi_index=0,bank=vreg,class=vector,abi_reg=q0",
                       "f128 helper result ABI q-register")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "marshaling lhs=carrier_to_abi_argument",
                       "f128 helper lhs marshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "rhs=carrier_to_abi_argument",
                       "f128 helper rhs marshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "result=abi_result_to_carrier",
                       "f128 helper result unmarshaling")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "live_preservation=[evaluated=yes,caller_saved_clobbers=yes,"
                       "additional=none,preserved=0]",
                       "f128 helper live-preservation authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(f128_helper_dump,
                       "selected_call_ownership=[owns_terminal_call=yes,callee=yes,resources=yes,"
                       "clobbers=yes,abi_bindings=yes,marshaling=yes,live_preservation=yes]",
                       "f128 helper complete selected ownership")) {
    return EXIT_FAILURE;
  }

  const auto grouped_cross_call_prepared = prepare_grouped_cross_call_preservation_dump_module();
  const auto* grouped_cross_call_plans = find_call_plans_function(
      grouped_cross_call_prepared, "grouped_cross_call_preservation_dump_contract");
  const auto* grouped_cross_call_storage = find_storage_plan_function(
      grouped_cross_call_prepared, "grouped_cross_call_preservation_dump_contract");
  const auto* grouped_cross_call_carry =
      grouped_cross_call_storage == nullptr
          ? nullptr
          : find_storage_value(grouped_cross_call_prepared, *grouped_cross_call_storage, "carry.pre");
  const auto grouped_cross_call_function_id =
      grouped_cross_call_prepared.names.function_names.find(
          "grouped_cross_call_preservation_dump_contract");
  const auto* grouped_cross_call_frame_plan =
      grouped_cross_call_function_id == c4c::kInvalidFunctionName
          ? nullptr
          : prepare::find_prepared_frame_plan(grouped_cross_call_prepared,
                                              grouped_cross_call_function_id);
  if (grouped_cross_call_plans == nullptr || grouped_cross_call_plans->calls.size() != 1 ||
      grouped_cross_call_frame_plan == nullptr || grouped_cross_call_carry == nullptr) {
    std::cerr << "[FAIL] missing grouped cross-call preservation dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto& grouped_cross_call = grouped_cross_call_plans->calls.front();
  const auto grouped_preserved_it = std::find_if(
      grouped_cross_call.preserved_values.begin(),
      grouped_cross_call.preserved_values.end(),
      [](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.contiguous_width == 2 &&
               preserved.register_bank ==
                   std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Vreg};
      });
  if (grouped_preserved_it == grouped_cross_call.preserved_values.end() ||
      !grouped_preserved_it->register_name.has_value() ||
      !grouped_preserved_it->callee_saved_save_index.has_value() ||
      !grouped_preserved_it->register_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped preserved-value authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto grouped_saved_it = std::find_if(
      grouped_cross_call_frame_plan->saved_callee_registers.begin(),
      grouped_cross_call_frame_plan->saved_callee_registers.end(),
      [&](const prepare::PreparedSavedRegister& saved) {
        return saved.occupied_register_names == grouped_preserved_it->occupied_register_names;
      });
  if (grouped_saved_it == grouped_cross_call_frame_plan->saved_callee_registers.end()) {
    std::cerr << "[FAIL] missing grouped saved-register span authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!grouped_saved_it->placement.has_value() ||
      !grouped_saved_it->slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(
          *grouped_saved_it->slot_placement) ||
      !grouped_cross_call_carry->register_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped structured placement identity in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string grouped_cross_call_dump = prepare::print(grouped_cross_call_prepared);
  if (!expect_contains(grouped_cross_call_dump,
                       "saved vreg:" + grouped_saved_it->register_name +
                           " order=" + std::to_string(grouped_saved_it->save_index) +
                           " " + register_placement_text(grouped_saved_it->placement) +
                           " width=2 units=" +
                           grouped_saved_it->occupied_register_names.front() + "," +
                           grouped_saved_it->occupied_register_names.back() + " " +
                           saved_register_slot_placement_text(grouped_saved_it->slot_placement),
                       "grouped saved-register slot placement summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       "saved_register bank=vreg " +
                           register_placement_text(grouped_saved_it->placement) +
                           " reg=" + grouped_saved_it->register_name +
                           " save_index=" + std::to_string(grouped_saved_it->save_index) +
                           " width=2 units=" +
                           grouped_saved_it->occupied_register_names.front() + "," +
                           grouped_saved_it->occupied_register_names.back() + " " +
                           saved_register_slot_placement_text(grouped_saved_it->slot_placement),
                       "grouped frame-plan saved-register slot placement")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       "carry.pre#" + std::to_string(grouped_preserved_it->value_id) +
                           ":callee_saved_register:" + *grouped_preserved_it->register_name +
                           "/w2[" + grouped_preserved_it->occupied_register_names.front() + "," +
                           grouped_preserved_it->occupied_register_names.back() + "]" + ":save" +
                           std::to_string(*grouped_preserved_it->callee_saved_save_index),
                       "grouped cross-call preservation summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       "preserve value=carry.pre value_id=" +
                           std::to_string(grouped_preserved_it->value_id) +
                           " route=callee_saved_register save_index=" +
                           std::to_string(*grouped_preserved_it->callee_saved_save_index) +
                           " " + register_placement_text(grouped_preserved_it->register_placement) +
                           " reg=" + *grouped_preserved_it->register_name +
                           " width=2 bank=vreg units=" +
                           grouped_saved_it->occupied_register_names.front() + "," +
                           grouped_saved_it->occupied_register_names.back(),
                       "grouped cross-call preservation detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       "storage carry.pre value_id=" +
                           std::to_string(grouped_cross_call_carry->value_id) +
                           " encoding=register bank=vreg " +
                           register_placement_text(grouped_cross_call_carry->register_placement) +
                           " reg=" +
                           *grouped_cross_call_carry->register_name +
                           " width=2 units=" +
                           grouped_cross_call_carry->occupied_register_names.front() + "," +
                           grouped_cross_call_carry->occupied_register_names.back(),
                       "grouped storage plan detail")) {
    return EXIT_FAILURE;
  }
  const auto grouped_clobber_it = std::find_if(
      grouped_cross_call.clobbered_registers.begin(),
      grouped_cross_call.clobbered_registers.end(),
      [](const prepare::PreparedClobberedRegister& clobber) {
        return clobber.bank == prepare::PreparedRegisterBank::Vreg &&
               clobber.contiguous_width == 2 &&
               clobber.occupied_register_names.size() == 2;
      });
  if (grouped_clobber_it == grouped_cross_call.clobbered_registers.end()) {
    std::cerr << "[FAIL] missing grouped clobber span authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!grouped_clobber_it->placement.has_value()) {
    std::cerr << "[FAIL] missing grouped clobber structured placement in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       clobber_summary(*grouped_clobber_it),
                       "grouped call clobber summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_cross_call_dump,
                       clobber_detail(*grouped_clobber_it),
                       "grouped call clobber detail")) {
    return EXIT_FAILURE;
  }

  const auto grouped_spill_reload_prepared = prepare_grouped_spill_reload_dump_module();
  const auto consumed = c4c::backend::x86::consume_plans(grouped_spill_reload_prepared,
                                                         "grouped_spill_reload_dump_contract");
  if (consumed.regalloc == nullptr || consumed.storage == nullptr) {
    std::cerr << "[FAIL] missing grouped spill/reload dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto grouped_spill_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [&](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Vreg &&
               op.contiguous_width == 16 &&
               op.occupied_register_names.size() == 16 &&
               op.occupied_register_names.front() == "v0" &&
               op.occupied_register_names.back() == "v15";
      });
  if (grouped_spill_it == consumed.regalloc->spill_reload_ops.end()) {
    std::cerr << "[FAIL] missing grouped spill/reload authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto* grouped_spill_value = find_regalloc_value(*consumed.regalloc, grouped_spill_it->value_id);
  const std::string grouped_spill_value_name =
      grouped_spill_value == nullptr
          ? std::string{}
          : std::string(prepare::prepared_value_name(grouped_spill_reload_prepared.names,
                                                     grouped_spill_value->value_name));
  if (grouped_spill_value == nullptr ||
      !grouped_spill_it->slot_id.has_value() || !grouped_spill_it->stack_offset_bytes.has_value() ||
      !grouped_spill_it->register_placement.has_value() ||
      !grouped_spill_it->spill_slot_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped spill slot authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string grouped_spill_reload_dump = prepare::print(grouped_spill_reload_prepared);
  if (!expect_contains(grouped_spill_reload_dump,
                       "spill_reload kind=spill value_id=" +
                           std::to_string(grouped_spill_it->value_id) +
                           " value=" + grouped_spill_value_name + " block_index=" +
                           std::to_string(grouped_spill_it->block_index) +
                           " inst_index=" +
                           std::to_string(grouped_spill_it->instruction_index) +
                           " bank=vreg " +
                           register_placement_text(grouped_spill_it->register_placement) +
                           " " + spill_slot_placement_text(grouped_spill_it->spill_slot_placement) +
                           " reg=v0 width=16 units=v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15 slot_id=#" +
                           std::to_string(*grouped_spill_it->slot_id) +
                           " stack_offset=" +
                           std::to_string(*grouped_spill_it->stack_offset_bytes),
                       "grouped spill detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(grouped_spill_reload_dump,
                       "spill_reload kind=reload value_id=" +
                           std::to_string(grouped_spill_it->value_id) + " value=" +
                           grouped_spill_value_name,
                       "grouped reload detail")) {
    return EXIT_FAILURE;
  }

  const auto general_grouped_spill_reload_prepared =
      prepare_general_grouped_spill_reload_dump_module();
  const auto general_consumed = c4c::backend::x86::consume_plans(
      general_grouped_spill_reload_prepared, "general_grouped_spill_reload_dump_contract");
  if (general_consumed.regalloc == nullptr || general_consumed.storage == nullptr) {
    std::cerr << "[FAIL] missing grouped general spill/reload dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto general_spill_it = std::find_if(
      general_consumed.regalloc->spill_reload_ops.begin(),
      general_consumed.regalloc->spill_reload_ops.end(),
      [&](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Gpr &&
               op.register_name == std::optional<std::string>{"s1"} &&
               op.contiguous_width == 2 &&
               op.occupied_register_names == std::vector<std::string>{"s1", "s2"};
      });
  if (general_spill_it == general_consumed.regalloc->spill_reload_ops.end()) {
    std::cerr << "[FAIL] missing grouped general spill/reload authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto* general_spill_value =
      find_regalloc_value(*general_consumed.regalloc, general_spill_it->value_id);
  const auto* general_storage_carry = find_storage_value(
      general_grouped_spill_reload_prepared, *general_consumed.storage, "carry");
  const std::string general_spill_value_name =
      general_spill_value == nullptr
          ? std::string{}
          : std::string(prepare::prepared_value_name(general_grouped_spill_reload_prepared.names,
                                                     general_spill_value->value_name));
  if (general_spill_value == nullptr || general_storage_carry == nullptr ||
      !general_spill_it->slot_id.has_value() ||
      !general_spill_it->stack_offset_bytes.has_value() ||
      !general_spill_it->register_placement.has_value() ||
      !general_spill_it->spill_slot_placement.has_value() ||
      !general_storage_carry->spill_slot_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped general spill slot authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string general_grouped_spill_reload_dump =
      prepare::print(general_grouped_spill_reload_prepared);
  if (!expect_contains(general_grouped_spill_reload_dump,
                       "spill_reload kind=spill value_id=" +
                           std::to_string(general_spill_it->value_id) +
                           " value=" + general_spill_value_name + " block_index=" +
                           std::to_string(general_spill_it->block_index) +
                           " inst_index=" +
                           std::to_string(general_spill_it->instruction_index) +
                           " bank=gpr " +
                           register_placement_text(general_spill_it->register_placement) +
                           " " + spill_slot_placement_text(general_spill_it->spill_slot_placement) +
                           " reg=s1 width=2 units=s1,s2 slot_id=#" +
                           std::to_string(*general_spill_it->slot_id) +
                           " stack_offset=" +
                           std::to_string(*general_spill_it->stack_offset_bytes),
                       "grouped general spill detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(general_grouped_spill_reload_dump,
                       "spill_reload kind=reload value_id=" +
                           std::to_string(general_spill_it->value_id) + " value=" +
                           general_spill_value_name,
                       "grouped general reload detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(general_grouped_spill_reload_dump,
                       "storage carry value_id=" +
                           std::to_string(general_storage_carry->value_id) +
                           " encoding=frame_slot bank=gpr " +
                           spill_slot_placement_text(general_storage_carry->spill_slot_placement) +
                           " width=2 slot_id=#" +
                           std::to_string(*general_storage_carry->slot_id) +
                           " stack_offset=" +
                           std::to_string(*general_storage_carry->stack_offset_bytes),
                       "grouped general storage detail")) {
    return EXIT_FAILURE;
  }

  const auto float_grouped_spill_reload_prepared = prepare_float_grouped_spill_reload_dump_module();
  const auto float_consumed = c4c::backend::x86::consume_plans(
      float_grouped_spill_reload_prepared, "float_grouped_spill_reload_dump_contract");
  if (float_consumed.regalloc == nullptr || float_consumed.storage == nullptr) {
    std::cerr << "[FAIL] missing grouped float spill/reload dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto float_spill_it = std::find_if(
      float_consumed.regalloc->spill_reload_ops.begin(),
      float_consumed.regalloc->spill_reload_ops.end(),
      [&](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Fpr &&
               op.register_name == std::optional<std::string>{"fs1"} &&
               op.contiguous_width == 2 &&
               op.occupied_register_names == std::vector<std::string>{"fs1", "fs2"};
      });
  if (float_spill_it == float_consumed.regalloc->spill_reload_ops.end()) {
    std::cerr << "[FAIL] missing grouped float spill/reload authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto* float_spill_value =
      find_regalloc_value(*float_consumed.regalloc, float_spill_it->value_id);
  const auto* float_storage_carry =
      find_storage_value(float_grouped_spill_reload_prepared, *float_consumed.storage, "carry");
  const std::string float_spill_value_name =
      float_spill_value == nullptr
          ? std::string{}
          : std::string(prepare::prepared_value_name(float_grouped_spill_reload_prepared.names,
                                                     float_spill_value->value_name));
  if (float_spill_value == nullptr || float_storage_carry == nullptr ||
      !float_spill_it->slot_id.has_value() || !float_spill_it->stack_offset_bytes.has_value() ||
      !float_spill_it->register_placement.has_value() ||
      !float_spill_it->spill_slot_placement.has_value() ||
      !float_storage_carry->spill_slot_placement.has_value()) {
    std::cerr << "[FAIL] missing grouped float spill slot authority in dump fixture\n";
    return EXIT_FAILURE;
  }
  const std::string float_grouped_spill_reload_dump =
      prepare::print(float_grouped_spill_reload_prepared);
  if (!expect_contains(float_grouped_spill_reload_dump,
                       "spill_reload kind=spill value_id=" +
                           std::to_string(float_spill_it->value_id) +
                           " value=" + float_spill_value_name + " block_index=" +
                           std::to_string(float_spill_it->block_index) +
                           " inst_index=" +
                           std::to_string(float_spill_it->instruction_index) +
                           " bank=fpr " +
                           register_placement_text(float_spill_it->register_placement) +
                           " " + spill_slot_placement_text(float_spill_it->spill_slot_placement) +
                           " reg=fs1 width=2 units=fs1,fs2 slot_id=#" +
                           std::to_string(*float_spill_it->slot_id) +
                           " stack_offset=" +
                           std::to_string(*float_spill_it->stack_offset_bytes),
                       "grouped float spill detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(float_grouped_spill_reload_dump,
                       "spill_reload kind=reload value_id=" +
                           std::to_string(float_spill_it->value_id) + " value=" +
                           float_spill_value_name,
                       "grouped float reload detail")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(float_grouped_spill_reload_dump,
                       "storage carry value_id=" +
                           std::to_string(float_storage_carry->value_id) +
                           " encoding=frame_slot bank=fpr " +
                           spill_slot_placement_text(float_storage_carry->spill_slot_placement) +
                           " width=2 slot_id=#" +
                           std::to_string(*float_storage_carry->slot_id) +
                           " stack_offset=" +
                           std::to_string(*float_storage_carry->stack_offset_bytes),
                       "grouped float storage detail")) {
    return EXIT_FAILURE;
  }

  const auto call_wrapper_call_plans =
      find_call_plans_function(call_wrapper_prepared, "call_wrapper_dump_contract");
  if (call_wrapper_call_plans == nullptr || call_wrapper_call_plans->calls.size() < 2 ||
      call_wrapper_call_plans->calls[1].clobbered_registers.empty()) {
    std::cerr << "[FAIL] missing prepared clobber fixture for call wrapper dump\n";
    return EXIT_FAILURE;
  }
  const auto& fixed_call = call_wrapper_call_plans->calls[1];
  if (fixed_call.arguments.empty() ||
      !fixed_call.arguments.front().destination_register_name.has_value() ||
      !fixed_call.arguments.front().destination_register_placement.has_value() ||
      !fixed_call.result.has_value() ||
      !fixed_call.result->destination_value_id.has_value() ||
      !fixed_call.result->source_register_name.has_value() ||
      !fixed_call.result->source_register_placement.has_value()) {
    std::cerr << "[FAIL] missing structured call arg/result placement in call wrapper dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       register_placement_text(fixed_call.arguments.front().destination_register_placement,
                                               "dest_placement") +
                           " dest_reg=" + *fixed_call.arguments.front().destination_register_name,
                       "call argument structured destination placement")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "result value_bank=gpr source_storage=register destination_storage=register "
                           "destination_value_id=" +
                           std::to_string(*fixed_call.result->destination_value_id) + " " +
                           register_placement_text(fixed_call.result->source_register_placement,
                                                   "source_placement") +
                           " source_reg=" + *fixed_call.result->source_register_name,
                       "call result structured source placement")) {
    return EXIT_FAILURE;
  }
  const auto* call_wrapper_locations =
      prepare::find_prepared_value_location_function(call_wrapper_prepared,
                                                     "call_wrapper_dump_contract");
  const auto* fixed_before_call_bundle =
      call_wrapper_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*call_wrapper_locations,
                                               prepare::PreparedMovePhase::BeforeCall,
                                               0,
                                               1);
  const auto* fixed_after_call_bundle =
      call_wrapper_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*call_wrapper_locations,
                                               prepare::PreparedMovePhase::AfterCall,
                                               0,
                                               1);
  if (fixed_before_call_bundle == nullptr || fixed_after_call_bundle == nullptr) {
    std::cerr << "[FAIL] missing fixed-call move bundles in dump fixture\n";
    return EXIT_FAILURE;
  }
  const auto fixed_arg_binding_it = std::find_if(
      fixed_before_call_bundle->abi_bindings.begin(),
      fixed_before_call_bundle->abi_bindings.end(),
      [](const prepare::PreparedAbiBinding& binding) {
        return binding.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
               binding.destination_register_placement.has_value();
      });
  const auto fixed_result_binding_it = std::find_if(
      fixed_after_call_bundle->abi_bindings.begin(),
      fixed_after_call_bundle->abi_bindings.end(),
      [](const prepare::PreparedAbiBinding& binding) {
        return binding.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallResultAbi &&
               binding.destination_register_placement.has_value();
      });
  if (fixed_arg_binding_it == fixed_before_call_bundle->abi_bindings.end() ||
      fixed_result_binding_it == fixed_after_call_bundle->abi_bindings.end()) {
    std::cerr << "[FAIL] missing structured ABI-binding placement in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "abi_binding destination_kind=call_argument_abi destination_storage=register "
                           "abi_index=0 " +
                           register_placement_text(
                               fixed_arg_binding_it->destination_register_placement,
                               "placement") +
                           " reg=" + *fixed_arg_binding_it->destination_register_name,
                       "call argument ABI binding structured placement")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "abi_binding destination_kind=call_result_abi destination_storage=register " +
                           register_placement_text(
                               fixed_result_binding_it->destination_register_placement,
                               "placement") +
                           " reg=" + *fixed_result_binding_it->destination_register_name,
                       "call result ABI binding structured placement")) {
    return EXIT_FAILURE;
  }
  const auto fixed_result_move_it = std::find_if(
      fixed_after_call_bundle->moves.begin(),
      fixed_after_call_bundle->moves.end(),
      [&](const prepare::PreparedMoveResolution& move) {
        return move.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallResultAbi &&
               fixed_call.result->destination_value_id.has_value() &&
               move.to_value_id == *fixed_call.result->destination_value_id &&
               move.destination_register_placement.has_value();
      });
  if (fixed_result_move_it == fixed_after_call_bundle->moves.end()) {
    std::cerr << "[FAIL] missing structured move destination placement in dump fixture\n";
    return EXIT_FAILURE;
  }
  if (!expect_contains(call_wrapper_dump,
                       "move from_value_id=" +
                           std::to_string(fixed_result_move_it->from_value_id) +
                           " to_value_id=" +
                           std::to_string(fixed_result_move_it->to_value_id) +
                           " destination_kind=call_result_abi destination_storage=register "
                           "op_kind=move uses_cycle_temp_source=no " +
                           register_placement_text(
                               fixed_result_move_it->destination_register_placement,
                               "placement"),
                       "call result move structured destination placement")) {
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
