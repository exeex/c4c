#include "src/backend/mir/x86/codegen/x86_codegen.hpp"
#include "src/backend/prealloc/prealloc.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

class ScopedEnvVar {
 public:
  ScopedEnvVar(const char* name, const char* value)
      : name_(name),
        had_previous_(std::getenv(name) != nullptr),
        previous_value_(had_previous_ ? std::string(std::getenv(name)) : std::string()) {
    if (value != nullptr) {
      setenv(name_, value, 1);
    } else {
      unsetenv(name_);
    }
  }

  ~ScopedEnvVar() {
    if (had_previous_) {
      setenv(name_, previous_value_.c_str(), 1);
    } else {
      unsetenv(name_);
    }
  }

 private:
  const char* name_;
  bool had_previous_ = false;
  std::string previous_value_;
};

c4c::TargetProfile x86_target_profile() {
  return c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
}

prepare::PreparedBirModule prepare_module(bir::Module module) {
  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = x86_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedControlFlowFunction* find_control_flow_function(prepare::PreparedBirModule& prepared,
                                                                 const char* function_name) {
  for (auto& function : prepared.control_flow.functions) {
    if (prepare::prepared_function_name(prepared.names, function.function_name) == function_name) {
      return &function;
    }
  }
  return nullptr;
}

prepare::PreparedBirModule legalize_short_circuit_or_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

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

  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_plain_route_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "plain_route_miss";
  function.return_type = bir::TypeKind::F32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_f32_bits(0)};

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_missing_short_circuit_contract_module() {
  auto prepared = legalize_short_circuit_or_guard_module();
  auto* control_flow = find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  if (control_flow != nullptr) {
    control_flow->branch_conditions.clear();
  }
  return prepared;
}

prepare::PreparedBirModule legalize_single_block_void_call_sequence_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "extern\n",
  });
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str1",
      .bytes = "helper\n",
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.format",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Function helper;
  helper.name = "same_module_variadic_helper";
  helper.return_type = bir::TypeKind::Void;
  helper.is_variadic = true;
  helper.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.format",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  helper.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{},
  });

  bir::Function function;
  function.name = "single_block_void_call_sequence_miss";
  function.return_type = bir::TypeKind::Void;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "%f0"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::immediate_f32_bits(0),
      .rhs = bir::Value::immediate_f32_bits(0),
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "same_module_variadic_helper",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str1")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{};

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(helper));
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_single_block_i64_ashr_return_helper_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "single_block_i64_ashr_return_helper_miss";
  function.return_type = bir::TypeKind::I64;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "p.x",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::AShr,
      .result = bir::Value::named(bir::TypeKind::I64, "shifted"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "p.x"),
      .rhs = bir::Value::immediate_i64(63),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I64, "shifted"),
  };

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_single_block_i64_immediate_return_helper_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "single_block_i64_immediate_return_helper_miss";
  function.return_type = bir::TypeKind::I64;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "p.x",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::LShr,
      .result = bir::Value::named(bir::TypeKind::I64, "shifted"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "p.x"),
      .rhs = bir::Value::immediate_i64(1),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I64, "shifted"),
  };

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_single_block_i64_extended_immediate_return_helper_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "single_block_i64_extended_immediate_return_helper_miss";
  function.return_type = bir::TypeKind::I64;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "p.x",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "negated"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(0),
      .rhs = bir::Value::immediate_i32(123),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I64, "widened"),
      .operand = bir::Value::named(bir::TypeKind::I32, "negated"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I64, "sum"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "p.x"),
      .rhs = bir::Value::named(bir::TypeKind::I64, "widened"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I64, "sum"),
  };

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_single_block_floating_aggregate_call_helper_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%f\n",
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.format",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Function function;
  function.name = "single_block_floating_aggregate_call_helper_miss";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.agg",
      .size_bytes = 16,
      .align_bytes = 4,
      .is_byval = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "agg.lane0"),
      .slot_name = "%lv.param.agg.0",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%p.agg",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "agg.lane0.f64"),
      .operand = bir::Value::named(bir::TypeKind::F32, "agg.lane0"),
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::F64, "agg.lane0.f64")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::F64},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{};

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_single_block_floating_aggregate_pointer_wrapper_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%f %f %f %f\n",
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.format",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Function function;
  function.name = "single_block_floating_aggregate_pointer_wrapper_miss";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.a",
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F64, "%t2"),
      .slot_name = "%t2.addr",
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F64, "%t4"),
      .slot_name = "%t4.addr",
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .byte_offset = 8,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F64, "%t6"),
      .slot_name = "%t6.addr",
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .byte_offset = 16,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F64, "%t8"),
      .slot_name = "%t8.addr",
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .byte_offset = 24,
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::F64, "%t2"),
               bir::Value::named(bir::TypeKind::F64, "%t4"),
               bir::Value::named(bir::TypeKind::F64, "%t6"),
               bir::Value::named(bir::TypeKind::F64, "%t8")},
      .arg_types = {bir::TypeKind::Ptr,
                    bir::TypeKind::F64,
                    bir::TypeKind::F64,
                    bir::TypeKind::F64,
                    bir::TypeKind::F64},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{};

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_single_block_floating_aggregate_sret_copyout_helper_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "agg0",
      .type = bir::TypeKind::F64,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_elements = {bir::Value::immediate_f64_bits(0x4002666666666666ULL)},
  });

  bir::Function function;
  function.name = "single_block_floating_aggregate_sret_copyout_helper_miss";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%ret.sret",
      .size_bytes = 8,
      .align_bytes = 8,
      .is_sret = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::F64,
      .size_bytes = 8,
      .align_bytes = 8,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F64, "%t0.global.aggregate.load.0"),
      .slot_name = "%t0.0",
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "agg0",
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::F64, "%t0.global.aggregate.load.0"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F64, "ret.sret.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::F64, "ret.sret.copy.0"),
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%ret.sret",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%ret.sret"),
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.terminator = bir::ReturnTerminator{};

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_single_block_aggregate_forwarding_wrapper_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "agg\n",
  });
  module.globals.push_back(bir::Global{
      .name = "agg0",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 4,
  });
  module.globals.push_back(bir::Global{
      .name = "agg1",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 4,
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.format",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Function helper0;
  helper0.name = "consume_agg0";
  helper0.return_type = bir::TypeKind::Void;
  helper0.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.agg0",
      .size_bytes = 12,
      .align_bytes = 4,
      .is_byval = true,
  });
  helper0.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{},
  });

  bir::Function helper1;
  helper1.name = "consume_agg1";
  helper1.return_type = bir::TypeKind::Void;
  helper1.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.agg1",
      .size_bytes = 16,
      .align_bytes = 8,
      .is_byval = true,
  });
  helper1.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{},
  });

  bir::Function function;
  function.name = "single_block_aggregate_forwarding_wrapper_miss";
  function.return_type = bir::TypeKind::Void;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%agg0.copy",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 4,
      .is_address_taken = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%agg1.copy",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%agg0.value"),
      .global_name = "agg0",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%agg0.copy.addr",
      .value = bir::Value::named(bir::TypeKind::I64, "%agg0.value"),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%agg0.copy",
          .size_bytes = 8,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "consume_agg0",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%agg0.copy")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%agg1.value"),
      .global_name = "agg1",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%agg1.copy.addr",
      .value = bir::Value::named(bir::TypeKind::I64, "%agg1.value"),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%agg1.copy",
          .size_bytes = 8,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "consume_agg1",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%agg1.copy")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{};

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(helper0));
  module.functions.push_back(std::move(helper1));
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_single_block_same_module_scalar_call_wrapper_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%lld\n",
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.format",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Function print_i64;
  print_i64.name = "print_i64";
  print_i64.return_type = bir::TypeKind::Void;
  print_i64.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "%p.value",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  bir::Block print_entry;
  print_entry.label = "entry";
  print_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I64, "%p.value")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I64},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  print_entry.terminator = bir::ReturnTerminator{};
  print_i64.blocks = {std::move(print_entry)};

  bir::Function addip0;
  addip0.name = "addip0";
  addip0.return_type = bir::TypeKind::I32;
  addip0.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "%p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  bir::Block addip0_entry;
  addip0_entry.label = "entry";
  addip0_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%p.x"),
      .rhs = bir::Value::immediate_i32(3),
  });
  addip0_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "%t0"),
  };
  addip0.blocks = {std::move(addip0_entry)};

  bir::Function sublp0;
  sublp0.name = "sublp0";
  sublp0.return_type = bir::TypeKind::I64;
  sublp0.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "%p.x",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  bir::Block sublp0_entry;
  sublp0_entry.label = "entry";
  sublp0_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I64, "%t0"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "%p.x"),
      .rhs = bir::Value::immediate_i64(9),
  });
  sublp0_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I64, "%t0"),
  };
  sublp0.blocks = {std::move(sublp0_entry)};

  bir::Function function;
  function.name = "single_block_same_module_scalar_call_wrapper_miss";
  function.return_type = bir::TypeKind::Void;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.x",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.x.addr",
      .value = bir::Value::immediate_i32(1000),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.x",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .slot_name = "%lv.x.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.x",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t2"),
      .callee = "addip0",
      .args = {bir::Value::named(bir::TypeKind::I32, "%t1")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::ZExt,
      .result = bir::Value::named(bir::TypeKind::I64, "%t3"),
      .operand = bir::Value::named(bir::TypeKind::I32, "%t2"),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "print_i64",
      .args = {bir::Value::named(bir::TypeKind::I64, "%t3")},
      .arg_types = {bir::TypeKind::I64},
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .slot_name = "%lv.x.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.x",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I64, "%t5"),
      .operand = bir::Value::named(bir::TypeKind::I32, "%t4"),
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%t6"),
      .callee = "sublp0",
      .args = {bir::Value::named(bir::TypeKind::I64, "%t5")},
      .arg_types = {bir::TypeKind::I64},
      .return_type_name = "i64",
      .return_type = bir::TypeKind::I64,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "print_i64",
      .args = {bir::Value::named(bir::TypeKind::I64, "%t6")},
      .arg_types = {bir::TypeKind::I64},
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{};

  function.blocks = {std::move(entry)};
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(print_i64));
  module.functions.push_back(std::move(addip0));
  module.functions.push_back(std::move(sublp0));
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_multi_param_compare_driven_miss_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "multi_param_compare_driven_miss";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.extra",
      .size_bytes = 8,
      .align_bytes = 8,
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

  function.blocks = {
      std::move(entry),
      std::move(is_zero),
      std::move(is_nonzero),
      std::move(join),
  };
  module.functions.push_back(std::move(function));
  return prepare_module(std::move(module));
}

prepare::PreparedBirModule legalize_multi_defined_global_function_pointer_rejection_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%d\n",
  });
  module.globals.push_back(bir::Global{
      .name = "f",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("fred"),
  });
  module.globals.push_back(bir::Global{
      .name = "printfptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("printf"),
  });

  bir::Function fred;
  fred.name = "fred";
  fred.return_type = bir::TypeKind::I32;
  fred.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "%p0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  fred.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(42)},
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%fred.ptr"),
      .global_name = "f",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%fred.ptr"),
      .args = {bir::Value::immediate_i32(24)},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%printf.ptr"),
      .global_name = "printfptr",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t2"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%printf.ptr"),
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t1")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  main_function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(fred));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return prepare_module(std::move(module));
}

bool expect_contains(const std::string& text,
                     const std::string& needle,
                     const char* description) {
  if (text.find(needle) != std::string::npos) {
    return true;
  }
  std::cerr << "[FAIL] missing " << description << ": " << needle << "\n";
  std::cerr << "--- text ---\n" << text << "\n";
  return false;
}

}  // namespace

int main() {
  const auto prepared = legalize_short_circuit_or_guard_module();
  const auto plain_miss = legalize_plain_route_miss_module();
  const auto missing_short_circuit_contract = legalize_missing_short_circuit_contract_module();
  const auto multi_param_compare_driven_miss = legalize_multi_param_compare_driven_miss_module();
  const std::string summary = c4c::backend::x86::summarize_prepared_module_routes(prepared);
  const std::string trace = c4c::backend::x86::trace_prepared_module_routes(prepared);
  const std::string plain_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(plain_miss);
  const std::string plain_miss_trace = c4c::backend::x86::trace_prepared_module_routes(plain_miss);
  const std::string missing_short_circuit_contract_summary =
      c4c::backend::x86::summarize_prepared_module_routes(missing_short_circuit_contract);
  const std::string missing_short_circuit_contract_trace =
      c4c::backend::x86::trace_prepared_module_routes(missing_short_circuit_contract);
  const std::string focused_summary = c4c::backend::x86::summarize_prepared_module_routes(
      prepared, "short_circuit_or_prepare_contract", "entry");
  const std::string focused_trace = c4c::backend::x86::trace_prepared_module_routes(
      prepared, "short_circuit_or_prepare_contract", "entry");
  std::string focused_value_summary;
  std::string focused_value_trace;
  {
    const ScopedEnvVar focused_value_env("C4C_MIR_FOCUS_VALUE", "%t3");
    focused_value_summary = c4c::backend::x86::summarize_prepared_module_routes(
        prepared, "short_circuit_or_prepare_contract");
    focused_value_trace = c4c::backend::x86::trace_prepared_module_routes(
        prepared, "short_circuit_or_prepare_contract");
  }
  std::string missing_value_summary;
  {
    const ScopedEnvVar missing_value_env("C4C_MIR_FOCUS_VALUE", "%missing");
    missing_value_summary = c4c::backend::x86::summarize_prepared_module_routes(
        prepared, "short_circuit_or_prepare_contract");
  }
  const auto single_block_void_call_sequence_miss =
      legalize_single_block_void_call_sequence_miss_module();
  const std::string single_block_void_call_sequence_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_void_call_sequence_miss);
  const std::string single_block_void_call_sequence_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_void_call_sequence_miss);
  const auto single_block_i64_ashr_return_helper_miss =
      legalize_single_block_i64_ashr_return_helper_miss_module();
  const std::string single_block_i64_ashr_return_helper_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_i64_ashr_return_helper_miss);
  const std::string single_block_i64_ashr_return_helper_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_i64_ashr_return_helper_miss);
  const auto single_block_i64_immediate_return_helper_miss =
      legalize_single_block_i64_immediate_return_helper_miss_module();
  const std::string single_block_i64_immediate_return_helper_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_i64_immediate_return_helper_miss);
  const std::string single_block_i64_immediate_return_helper_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_i64_immediate_return_helper_miss);
  const auto single_block_i64_extended_immediate_return_helper_miss =
      legalize_single_block_i64_extended_immediate_return_helper_miss_module();
  const std::string single_block_i64_extended_immediate_return_helper_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_i64_extended_immediate_return_helper_miss);
  const std::string single_block_i64_extended_immediate_return_helper_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_i64_extended_immediate_return_helper_miss);
  const auto single_block_floating_aggregate_call_helper_miss =
      legalize_single_block_floating_aggregate_call_helper_miss_module();
  const std::string single_block_floating_aggregate_call_helper_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_floating_aggregate_call_helper_miss);
  const std::string single_block_floating_aggregate_call_helper_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_floating_aggregate_call_helper_miss);
  const auto single_block_floating_aggregate_pointer_wrapper_miss =
      legalize_single_block_floating_aggregate_pointer_wrapper_miss_module();
  const std::string single_block_floating_aggregate_pointer_wrapper_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_floating_aggregate_pointer_wrapper_miss);
  const std::string single_block_floating_aggregate_pointer_wrapper_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_floating_aggregate_pointer_wrapper_miss);
  const auto single_block_floating_aggregate_sret_copyout_helper_miss =
      legalize_single_block_floating_aggregate_sret_copyout_helper_miss_module();
  const std::string single_block_floating_aggregate_sret_copyout_helper_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_floating_aggregate_sret_copyout_helper_miss);
  const std::string single_block_floating_aggregate_sret_copyout_helper_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_floating_aggregate_sret_copyout_helper_miss);
  const auto single_block_aggregate_forwarding_wrapper_miss =
      legalize_single_block_aggregate_forwarding_wrapper_miss_module();
  const std::string single_block_aggregate_forwarding_wrapper_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_aggregate_forwarding_wrapper_miss);
  const std::string single_block_aggregate_forwarding_wrapper_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_aggregate_forwarding_wrapper_miss);
  const auto single_block_same_module_scalar_call_wrapper_miss =
      legalize_single_block_same_module_scalar_call_wrapper_miss_module();
  const std::string single_block_same_module_scalar_call_wrapper_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          single_block_same_module_scalar_call_wrapper_miss);
  const std::string single_block_same_module_scalar_call_wrapper_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          single_block_same_module_scalar_call_wrapper_miss);
  const std::string multi_param_compare_driven_miss_summary =
      c4c::backend::x86::summarize_prepared_module_routes(multi_param_compare_driven_miss);
  const std::string multi_param_compare_driven_miss_trace =
      c4c::backend::x86::trace_prepared_module_routes(multi_param_compare_driven_miss);
  const auto multi_defined_global_function_pointer_rejection =
      legalize_multi_defined_global_function_pointer_rejection_module();
  const std::string multi_defined_global_function_pointer_rejection_summary =
      c4c::backend::x86::summarize_prepared_module_routes(
          multi_defined_global_function_pointer_rejection);
  const std::string multi_defined_global_function_pointer_rejection_trace =
      c4c::backend::x86::trace_prepared_module_routes(
          multi_defined_global_function_pointer_rejection);

  if (!expect_contains(summary, "x86 handoff summary", "summary header") ||
      !expect_contains(summary, "function short_circuit_or_prepare_contract", "function name") ||
      !expect_contains(summary, "- top-level lane:", "summary lane line") ||
      !expect_contains(trace, "x86 handoff trace", "trace header") ||
      !expect_contains(trace, "try lane local-slot-guard-chain", "trace lane") ||
      !expect_contains(trace, "result: ", "trace result") ||
      !expect_contains(plain_miss_summary,
                       "- final rejection: current x86 lanes did not recognize this prepared function shape",
                       "plain miss summary final rejection") ||
      !expect_contains(plain_miss_summary,
                       "- next inspect: inspect src/backend/mir/x86/codegen/prepared_module_emit.cpp for the next top-level lane",
                       "plain miss summary next inspect") ||
      !expect_contains(plain_miss_trace,
                       "final: rejected: current x86 lanes did not recognize this prepared function shape",
                       "plain miss trace final rejection") ||
      !expect_contains(plain_miss_trace,
                       "next inspect: inspect src/backend/mir/x86/codegen/prepared_module_emit.cpp for the next top-level lane",
                       "plain miss trace next inspect") ||
      !expect_contains(missing_short_circuit_contract_summary,
                       "- final rejection: local-slot-guard-chain is missing prepared handoff data required by the current x86 route",
                       "missing contract summary final rejection") ||
      !expect_contains(missing_short_circuit_contract_trace,
                       "final detail: x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff",
                       "missing contract trace detail") ||
      !expect_contains(missing_short_circuit_contract_trace,
                       "next inspect: inspect the prepared control-flow handoff consumed in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "missing contract trace next inspect") ||
      !expect_contains(focused_summary,
                       "focus function: short_circuit_or_prepare_contract",
                       "focused summary function header") ||
      !expect_contains(focused_summary,
                       "focus block: entry",
                       "focused summary block header") ||
      !expect_contains(focused_summary,
                       "- focused bir blocks: 1\n  - entry\n- focused prepared blocks: 1\n  - entry",
                       "focused summary block labels") ||
      !expect_contains(focused_trace,
                       "focused bir blocks: 1\n    - entry\n  focused prepared blocks: 1\n    - entry",
                       "focused trace block labels") ||
      !expect_contains(focused_value_summary,
                       "focus value: %t3",
                       "focused value summary header") ||
      !expect_contains(focused_value_summary,
                       "- focused prepared values: 1\n- focused prepared move bundles: 1",
                       "focused value summary counts") ||
      !expect_contains(focused_value_trace,
                       "focus value: %t3",
                       "focused value trace header") ||
      !expect_contains(focused_value_trace,
                       "focused prepared values: 1\n  focused prepared move bundles: 1",
                       "focused value trace counts") ||
      !expect_contains(missing_value_summary,
                       "focus value: %missing",
                       "missing value summary header") ||
      !expect_contains(missing_value_summary,
                       "focused prepared values matched: 0\nfocused prepared move bundles matched: 0\nno prepared value in the focused function matched the requested MIR value focus",
                       "missing value summary result") ||
      !expect_contains(single_block_void_call_sequence_miss_summary,
                       "- final rejection: single-block void call-sequence helper recognized the function, but the prepared call-wrapper shape is outside the current x86 support",
                       "single-block void call-sequence summary final rejection") ||
      !expect_contains(single_block_void_call_sequence_miss_summary,
                       "- final facts: prepared call-wrapper facts: same-module calls=1, direct variadic extern calls=1, direct fixed-arity extern calls=0, indirect calls=0, other call side effects=0",
                       "single-block void call-sequence summary final facts") ||
      !expect_contains(single_block_void_call_sequence_miss_summary,
                       "- next inspect: inspect the current x86 single-block call-sequence support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block void call-sequence summary next inspect") ||
      !expect_contains(single_block_void_call_sequence_miss_trace,
                       "try lane single-block-void-call-sequence",
                       "single-block void call-sequence trace lane") ||
      !expect_contains(single_block_void_call_sequence_miss_trace,
                       "final detail: x86 backend emitter only supports trivial single-block void helpers through the canonical prepared-module handoff; this prepared call-wrapper still carries 1 same-module call and 1 direct variadic extern call",
                       "single-block void call-sequence trace detail") ||
      !expect_contains(single_block_void_call_sequence_miss_trace,
                       "final facts: prepared call-wrapper facts: same-module calls=1, direct variadic extern calls=1, direct fixed-arity extern calls=0, indirect calls=0, other call side effects=0",
                       "single-block void call-sequence trace final facts") ||
      !expect_contains(single_block_void_call_sequence_miss_trace,
                       "next inspect: inspect the current x86 single-block call-sequence support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block void call-sequence trace next inspect") ||
      !expect_contains(single_block_i64_ashr_return_helper_miss_summary,
                       "- final rejection: single-block i64 arithmetic-right-shift return helper recognized the function, but the prepared return-helper shape is outside the current x86 support",
                       "single-block i64 ashr summary final rejection") ||
      !expect_contains(single_block_i64_ashr_return_helper_miss_summary,
                       "- next inspect: inspect the current x86 single-block i64 return-helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block i64 ashr summary next inspect") ||
      !expect_contains(single_block_i64_ashr_return_helper_miss_trace,
                       "try lane single-block-i64-ashr-return-helper",
                       "single-block i64 ashr trace lane") ||
      !expect_contains(single_block_i64_ashr_return_helper_miss_trace,
                       "final detail: x86 backend emitter only supports single-block i64 return helpers when they already reduce to the current direct passthrough or local i16/i64-sub helper surfaces; this helper still carries an i64 arithmetic-right-shift immediate return",
                       "single-block i64 ashr trace detail") ||
      !expect_contains(single_block_i64_ashr_return_helper_miss_trace,
                       "next inspect: inspect the current x86 single-block i64 return-helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block i64 ashr trace next inspect") ||
      !expect_contains(single_block_i64_immediate_return_helper_miss_summary,
                       "- final rejection: single-block i64 immediate return helper recognized the function, but the prepared return-helper shape is outside the current x86 support",
                       "single-block i64 immediate summary final rejection") ||
      !expect_contains(single_block_i64_immediate_return_helper_miss_summary,
                       "- next inspect: inspect the current x86 single-block i64 return-helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block i64 immediate summary next inspect") ||
      !expect_contains(single_block_i64_immediate_return_helper_miss_trace,
                       "try lane single-block-i64-immediate-return-helper",
                       "single-block i64 immediate trace lane") ||
      !expect_contains(single_block_i64_immediate_return_helper_miss_trace,
                       "final detail: x86 backend emitter only supports single-block i64 return helpers when they already reduce to the current direct passthrough or established scalar helper surfaces; this helper still carries an i64 logical-right-shift immediate return",
                       "single-block i64 immediate trace detail") ||
      !expect_contains(single_block_i64_immediate_return_helper_miss_trace,
                       "next inspect: inspect the current x86 single-block i64 return-helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block i64 immediate trace next inspect") ||
      !expect_contains(single_block_i64_extended_immediate_return_helper_miss_summary,
                       "- final rejection: single-block i64 immediate return helper recognized the function, but the prepared return-helper shape is outside the current x86 support",
                       "single-block i64 extended immediate summary final rejection") ||
      !expect_contains(single_block_i64_extended_immediate_return_helper_miss_summary,
                       "- next inspect: inspect the current x86 single-block i64 return-helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block i64 extended immediate summary next inspect") ||
      !expect_contains(single_block_i64_extended_immediate_return_helper_miss_trace,
                       "try lane single-block-i64-immediate-return-helper",
                       "single-block i64 extended immediate trace lane") ||
      !expect_contains(single_block_i64_extended_immediate_return_helper_miss_trace,
                       "final detail: x86 backend emitter only supports single-block i64 return helpers when they already reduce to the current direct passthrough or established scalar helper surfaces; this helper still carries an i64 add immediate return",
                       "single-block i64 extended immediate trace detail") ||
      !expect_contains(single_block_i64_extended_immediate_return_helper_miss_trace,
                       "next inspect: inspect the current x86 single-block i64 return-helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block i64 extended immediate trace next inspect") ||
      !expect_contains(single_block_floating_aggregate_call_helper_miss_summary,
                       "- final rejection: single-block floating aggregate call helper recognized the function, but the prepared aggregate-helper shape is outside the current x86 support",
                       "single-block floating aggregate helper summary final rejection") ||
      !expect_contains(single_block_floating_aggregate_call_helper_miss_summary,
                       "- next inspect: inspect the current x86 floating aggregate helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block floating aggregate helper summary next inspect") ||
      !expect_contains(single_block_floating_aggregate_call_helper_miss_trace,
                       "try lane single-block-floating-aggregate-call-helper",
                       "single-block floating aggregate helper trace lane") ||
      !expect_contains(single_block_floating_aggregate_call_helper_miss_trace,
                       "final detail: x86 backend emitter only supports single-block floating aggregate helpers when those aggregate arguments already reduce to the current local-slot or scalar helper surfaces; this helper still forwards floating aggregate lanes through byval/pointer wrappers into a direct variadic extern call",
                       "single-block floating aggregate helper trace detail") ||
      !expect_contains(single_block_floating_aggregate_call_helper_miss_trace,
                       "next inspect: inspect the current x86 floating aggregate helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block floating aggregate helper trace next inspect") ||
      !expect_contains(single_block_floating_aggregate_pointer_wrapper_miss_summary,
                       "- final rejection: single-block floating aggregate call helper recognized the function, but the prepared aggregate-helper shape is outside the current x86 support",
                       "single-block floating aggregate pointer wrapper summary final rejection") ||
      !expect_contains(single_block_floating_aggregate_pointer_wrapper_miss_summary,
                       "- next inspect: inspect the current x86 floating aggregate helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block floating aggregate pointer wrapper summary next inspect") ||
      !expect_contains(single_block_floating_aggregate_pointer_wrapper_miss_trace,
                       "try lane single-block-floating-aggregate-call-helper",
                       "single-block floating aggregate pointer wrapper trace lane") ||
      !expect_contains(single_block_floating_aggregate_pointer_wrapper_miss_trace,
                       "final detail: x86 backend emitter only supports single-block floating aggregate helpers when those aggregate arguments already reduce to the current local-slot or scalar helper surfaces; this helper still forwards floating aggregate lanes through byval/pointer wrappers into a direct variadic extern call",
                       "single-block floating aggregate pointer wrapper trace detail") ||
      !expect_contains(single_block_floating_aggregate_pointer_wrapper_miss_trace,
                       "next inspect: inspect the current x86 floating aggregate helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block floating aggregate pointer wrapper trace next inspect") ||
      !expect_contains(single_block_floating_aggregate_sret_copyout_helper_miss_summary,
                       "- final rejection: single-block floating aggregate sret copyout helper recognized the function, but the prepared return-helper shape is outside the current x86 support",
                       "single-block floating aggregate sret copyout helper summary final rejection") ||
      !expect_contains(single_block_floating_aggregate_sret_copyout_helper_miss_summary,
                       "- next inspect: inspect the current x86 floating aggregate return-helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block floating aggregate sret copyout helper summary next inspect") ||
      !expect_contains(single_block_floating_aggregate_sret_copyout_helper_miss_trace,
                       "try lane single-block-floating-aggregate-sret-copyout-helper",
                       "single-block floating aggregate sret copyout helper trace lane") ||
      !expect_contains(single_block_floating_aggregate_sret_copyout_helper_miss_trace,
                       "final detail: x86 backend emitter only supports single-block floating aggregate sret copyout helpers when those same-module aggregate returns already reduce to the current return-helper surfaces; this helper still copies floating aggregate lanes from same-module globals through scratch slots into an sret destination",
                       "single-block floating aggregate sret copyout helper trace detail") ||
      !expect_contains(single_block_floating_aggregate_sret_copyout_helper_miss_trace,
                       "next inspect: inspect the current x86 floating aggregate return-helper support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block floating aggregate sret copyout helper trace next inspect") ||
      !expect_contains(single_block_aggregate_forwarding_wrapper_miss_summary,
                       "- final rejection: single-block aggregate-forwarding wrapper recognized the function, but the prepared same-module aggregate-call shape is outside the current x86 support",
                       "single-block aggregate forwarding wrapper summary final rejection") ||
      !expect_contains(single_block_aggregate_forwarding_wrapper_miss_summary,
                       "- next inspect: inspect the current x86 same-module aggregate-call support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block aggregate forwarding wrapper summary next inspect") ||
      !expect_contains(single_block_aggregate_forwarding_wrapper_miss_trace,
                       "try lane single-block-aggregate-forwarding-wrapper",
                       "single-block aggregate forwarding wrapper trace lane") ||
      !expect_contains(single_block_aggregate_forwarding_wrapper_miss_trace,
                       "final detail: x86 backend emitter only supports single-block aggregate-forwarding wrappers when their direct extern preamble and same-module aggregate calls already reduce to the current helper surfaces; this wrapper still mixes a direct extern preamble with same-module aggregate call wrappers",
                       "single-block aggregate forwarding wrapper trace detail") ||
      !expect_contains(single_block_aggregate_forwarding_wrapper_miss_trace,
                       "next inspect: inspect the current x86 same-module aggregate-call support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block aggregate forwarding wrapper trace next inspect") ||
      !expect_contains(single_block_same_module_scalar_call_wrapper_miss_summary,
                       "- final rejection: single-block same-module scalar call-wrapper family recognized the function, but the prepared helper-family shape is outside the current x86 support",
                       "single-block same-module scalar call-wrapper summary final rejection") ||
      !expect_contains(single_block_same_module_scalar_call_wrapper_miss_summary,
                       "- final facts: prepared helper-family facts: local-slot reloads=2, scalar same-module helper calls=2, width-adjusting casts=2, same-module sink wrappers=2",
                       "single-block same-module scalar call-wrapper summary final facts") ||
      !expect_contains(single_block_same_module_scalar_call_wrapper_miss_summary,
                       "- next inspect: inspect the current x86 same-module scalar helper-family support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block same-module scalar call-wrapper summary next inspect") ||
      !expect_contains(single_block_same_module_scalar_call_wrapper_miss_trace,
                       "try lane single-block-same-module-scalar-call-wrapper",
                       "single-block same-module scalar call-wrapper trace lane") ||
      !expect_contains(single_block_same_module_scalar_call_wrapper_miss_trace,
                       "final detail: x86 backend emitter only supports single-block same-module scalar call-wrapper families when they already reduce to the current local-slot or established scalar helper surfaces; this wrapper family still carries 2 local-slot reloads, 2 scalar same-module helper calls, 2 width-adjusting casts, and 2 same-module sink wrappers",
                       "single-block same-module scalar call-wrapper trace detail") ||
      !expect_contains(single_block_same_module_scalar_call_wrapper_miss_trace,
                       "final facts: prepared helper-family facts: local-slot reloads=2, scalar same-module helper calls=2, width-adjusting casts=2, same-module sink wrappers=2",
                       "single-block same-module scalar call-wrapper trace final facts") ||
      !expect_contains(single_block_same_module_scalar_call_wrapper_miss_trace,
                       "next inspect: inspect the current x86 same-module scalar helper-family support in src/backend/mir/x86/codegen/prepared_local_slot_render.cpp",
                       "single-block same-module scalar call-wrapper trace next inspect") ||
      !expect_contains(multi_param_compare_driven_miss_summary,
                       "- final rejection: compare-driven-entry recognized the function, but the prepared shape is outside the current x86 support",
                       "multi-param compare-driven summary final rejection") ||
      !expect_contains(multi_param_compare_driven_miss_trace,
                       "final detail: x86 backend emitter only supports multi-block compare-driven entry routes through the canonical prepared-module handoff when the function exposes exactly one non-variadic i32 parameter",
                       "multi-param compare-driven trace detail") ||
      !expect_contains(multi_param_compare_driven_miss_trace,
                       "next inspect: inspect the current x86 shape support in src/backend/mir/x86/codegen/prepared_module_emit.cpp",
                       "multi-param compare-driven trace next inspect") ||
      !expect_contains(multi_defined_global_function_pointer_rejection_summary,
                       "- module-level final rejection: bounded multi-function handoff recognized the module, but the prepared shape is outside the current x86 support",
                       "module-level rejection summary final rejection") ||
      !expect_contains(multi_defined_global_function_pointer_rejection_summary,
                       "- module-level final detail: x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff",
                       "module-level rejection summary final detail") ||
      !expect_contains(multi_defined_global_function_pointer_rejection_summary,
                       "- module-level next inspect: inspect the current x86 bounded multi-function shape support in src/backend/mir/x86/codegen/prepared_module_emit.cpp",
                       "module-level rejection summary next inspect") ||
      !expect_contains(multi_defined_global_function_pointer_rejection_trace,
                       "final: rejected: bounded multi-function handoff recognized the module, but the prepared shape is outside the current x86 support",
                       "module-level rejection trace final rejection") ||
      !expect_contains(multi_defined_global_function_pointer_rejection_trace,
                       "final detail: x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff",
                       "module-level rejection trace final detail") ||
      !expect_contains(multi_defined_global_function_pointer_rejection_trace,
                       "next inspect: inspect the current x86 bounded multi-function shape support in src/backend/mir/x86/codegen/prepared_module_emit.cpp",
                       "module-level rejection trace next inspect")) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
