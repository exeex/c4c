#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/target.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

using c4c::backend::Target;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const prepare::PreparedStackObject* find_stack_object(const prepare::PreparedBirModule& prepared,
                                                      std::string_view source_name) {
  for (const auto& object : prepared.stack_layout.objects) {
    if (object.source_name == source_name) {
      return &object;
    }
  }
  return nullptr;
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

bir::Function make_stack_layout_analysis_object_collection_function() {
  bir::Function function;
  function.name = "stack_layout_analysis_object_collection_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.analysis.scratch",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.analysis.byval_copy",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.analysis.phi",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .phi_observation = bir::PhiObservation{
          .result = bir::Value::named(bir::TypeKind::I32, "lv.analysis.phi.result"),
          .incomings = {bir::PhiIncoming{
                            .label = "entry",
                            .value = bir::Value::immediate_i32(1),
                        }},
      },
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.analysis.addr",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .is_address_taken = true,
  });

  return function;
}

std::vector<prepare::PreparedStackObject> collect_stack_layout_analysis_objects() {
  bir::Function function = make_stack_layout_analysis_object_collection_function();

  prepare::PreparedObjectId next_object_id = 0;
  return prepare::stack_layout::collect_function_stack_objects(function, next_object_id);
}

std::vector<prepare::PreparedStackObject> collect_stack_layout_regalloc_hint_objects() {
  bir::Function function = make_stack_layout_analysis_object_collection_function();
  prepare::PreparedObjectId next_object_id = 0;
  auto objects = prepare::stack_layout::collect_function_stack_objects(function, next_object_id);
  prepare::stack_layout::apply_regalloc_hints(function, prepare::FunctionInlineAsmSummary{}, objects);
  return objects;
}

bir::Function make_stack_layout_param_object_collection_function() {
  bir::Function function;
  function.name = "stack_layout_param_object_collection_activation";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.analysis.byval",
      .size_bytes = 8,
      .align_bytes = 8,
      .is_byval = true,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.analysis.sret",
      .size_bytes = 8,
      .align_bytes = 8,
      .is_sret = true,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.analysis.plain",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  return function;
}

std::vector<prepare::PreparedStackObject> collect_stack_layout_param_objects() {
  bir::Function function = make_stack_layout_param_object_collection_function();

  prepare::PreparedObjectId next_object_id = 0;
  return prepare::stack_layout::collect_function_stack_objects(function, next_object_id);
}

std::vector<prepare::PreparedStackObject> collect_stack_layout_param_regalloc_hint_objects() {
  bir::Function function = make_stack_layout_param_object_collection_function();
  prepare::PreparedObjectId next_object_id = 0;
  auto objects = prepare::stack_layout::collect_function_stack_objects(function, next_object_id);
  prepare::stack_layout::apply_regalloc_hints(function, prepare::FunctionInlineAsmSummary{}, objects);
  return objects;
}

const prepare::PreparedStackObject* find_stack_object(
    const std::vector<prepare::PreparedStackObject>& objects,
    std::string_view source_name) {
  for (const auto& object : objects) {
    if (object.source_name == source_name) {
      return &object;
    }
  }
  return nullptr;
}

prepare::PreparedBirModule prepare_stack_layout_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_copy_coalescing_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.live",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.dead",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.copy",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.copy",
      .value = bir::Value::immediate_i32(40),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "coalesced"),
      .slot_name = "lv.copy",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.live",
      .value = bir::Value::named(bir::TypeKind::I32, "coalesced"),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .slot_name = "lv.live",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "sum"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_param_permanent_home_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_param_permanent_home_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.byval.root",
      .size_bytes = 8,
      .align_bytes = 8,
      .is_byval = true,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.sret.root",
      .size_bytes = 8,
      .align_bytes = 8,
      .is_sret = true,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.plain",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.param.wide",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.param.wide",
      .value = bir::Value::immediate_i64(9),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "loaded"),
      .slot_name = "lv.param.wide",
      .align_bytes = 8,
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(0),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_param_fixed_location_ordering_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_param_fixed_location_ordering_activation";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.byval.fixed",
      .size_bytes = 8,
      .align_bytes = 8,
      .is_byval = true,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.sret.fixed",
      .size_bytes = 8,
      .align_bytes = 8,
      .is_sret = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed.local.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed.local.wide",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed.local.wide",
      .value = bir::Value::immediate_i64(21),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed.local.wide",
      .value = bir::Value::immediate_i64(34),
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "lv.fixed.local.root",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(0),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_addressed_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_addressed_local_slot_activation";
  function.return_type = bir::TypeKind::I64;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.addr.root",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.addr.store",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.addr.load",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.addr.store",
      .value = bir::Value::immediate_i64(7),
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "lv.addr.root",
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.terminator = bir::BranchTerminator{.target_label = "use"};

  bir::Block use;
  use.label = "use";
  use.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "loaded.addr"),
      .slot_name = "lv.addr.load",
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "lv.addr.root",
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  use.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I64, "loaded.addr"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(use));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_packed_frame_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_packed_frame_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.pack.small0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.pack.big",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.pack.small1",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.pack.small0",
      .value = bir::Value::immediate_i32(1),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.pack.big",
      .value = bir::Value::immediate_i64(2),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.pack.small1",
      .value = bir::Value::immediate_i32(3),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "lv.pack.big.loaded"),
      .slot_name = "lv.pack.big",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.pack.small0.loaded"),
      .slot_name = "lv.pack.small0",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.pack.small1.loaded"),
      .slot_name = "lv.pack.small1",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "lv.pack.sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "lv.pack.small0.loaded"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "lv.pack.small1.loaded"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "lv.pack.sum"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_fixed_location_frame_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_fixed_location_frame_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed.wide",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed.wide",
      .value = bir::Value::immediate_i64(11),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed.wide",
      .value = bir::Value::immediate_i64(13),
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "lv.fixed.root",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_permanent_home_slot_frame_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_permanent_home_slot_frame_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.perm.copy",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.perm.wide",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.perm.copy",
      .value = bir::Value::immediate_i32(7),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.perm.wide",
      .value = bir::Value::immediate_i64(11),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.perm.copy.loaded"),
      .slot_name = "lv.perm.copy",
      .align_bytes = 4,
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "lv.perm.copy.loaded"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_lowering_scratch_frame_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_lowering_scratch_frame_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.scratch.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.scratch.wide",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.scratch.root",
      .value = bir::Value::immediate_i32(9),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.scratch.wide",
      .value = bir::Value::immediate_i64(17),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.scratch.loaded"),
      .slot_name = "lv.scratch.root",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.scratch.reloaded"),
      .slot_name = "lv.scratch.root",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "lv.scratch.sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "lv.scratch.loaded"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "lv.scratch.reloaded"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "lv.scratch.sum"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_dead_lowering_scratch_frame_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_dead_lowering_scratch_frame_slot_activation";
  function.return_type = bir::TypeKind::I64;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.scratch.dead",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.scratch.live",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.scratch.live",
      .value = bir::Value::immediate_i64(23),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "lv.scratch.live.loaded"),
      .slot_name = "lv.scratch.live",
      .align_bytes = 8,
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I64, "lv.scratch.live.loaded"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_phi_permanent_home_slot_frame_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_phi_permanent_home_slot_frame_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.phi.perm.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .phi_observation = bir::PhiObservation{
          .result = bir::Value::named(bir::TypeKind::I32, "lv.phi.perm.result"),
          .incomings = {
              bir::PhiIncoming{
                  .label = "entry",
                  .value = bir::Value::immediate_i32(1),
              },
          },
      },
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.phi.perm.wide",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.phi.perm.root",
      .value = bir::Value::immediate_i32(5),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.phi.perm.wide",
      .value = bir::Value::immediate_i64(13),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.phi.perm.loaded"),
      .slot_name = "lv.phi.perm.root",
      .align_bytes = 4,
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "lv.phi.perm.loaded"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_call_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_call_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.call.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "sink_ptr",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "lv.call.root")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_sret_storage_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_sret_storage_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.call.sret.storage",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "make_sret_value",
      .return_type_name = "agg",
      .return_type = bir::TypeKind::Void,
      .sret_storage_name = "lv.call.sret.storage",
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_indirect_call_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_indirect_call_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.call.indirect.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.call.indirect.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.call.indirect.root"),
  });
  entry.insts.push_back(bir::CallInst{
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "lv.call.indirect.alias"),
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
      .is_indirect = true,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_select_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_select_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.select.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.select.alias"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
      .true_value = bir::Value::named(bir::TypeKind::Ptr, "lv.select.root"),
      .false_value = bir::Value::named(bir::TypeKind::Ptr, "lv.select.root"),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "sink_ptr",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "lv.select.alias")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_mixed_select_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_mixed_select_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.select.mixed.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.select.mixed.alias"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
      .true_value = bir::Value::named(bir::TypeKind::Ptr, "lv.select.mixed.root"),
      .false_value = bir::Value::named(bir::TypeKind::Ptr, "external.ptr"),
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_rooted_select_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_rooted_select_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.select.rooted.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.select.rooted.alias"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
      .true_value = bir::Value::named(bir::TypeKind::Ptr, "lv.select.rooted.root"),
      .false_value = bir::Value::named(bir::TypeKind::Ptr, "lv.select.rooted.root"),
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_select_compare_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_select_compare_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.select.compare.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.select.compare.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.select.compare.root"),
  });
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "select.compare.result"),
      .compare_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "lv.select.compare.alias"),
      .rhs = bir::Value::named(bir::TypeKind::Ptr, "external.compare.ptr"),
      .true_value = bir::Value::immediate_i32(1),
      .false_value = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "select.compare.result"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_rooted_select_compare_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_rooted_select_compare_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.select.compare.rooted.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.select.compare.rooted.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.select.compare.rooted.root"),
  });
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "select.compare.rooted.result"),
      .compare_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "lv.select.compare.rooted.alias"),
      .rhs = bir::Value::named(bir::TypeKind::Ptr, "lv.select.compare.rooted.root"),
      .true_value = bir::Value::immediate_i32(1),
      .false_value = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "select.compare.rooted.result"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_cast_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_cast_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.cast.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.cast.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.cast.root"),
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_return_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_return_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::Ptr;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.ret.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::Ptr, "lv.ret.root"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_cond_branch_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_cond_branch_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.cond.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::Ptr, "lv.cond.root"),
      .true_label = "then",
      .false_label = "else",
  };

  bir::Block then_block;
  then_block.label = "then";
  then_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block else_block;
  else_block.label = "else";
  else_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(then_block));
  function.blocks.push_back(std::move(else_block));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_pointer_addressed_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_pointer_addressed_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.ptr.addr.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.ptr.addr.store",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.ptr.addr.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.ptr.addr.root"),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.ptr.addr.store",
      .value = bir::Value::immediate_i32(9),
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "lv.ptr.addr.alias"),
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_global_pointer_addressed_local_slot_module() {
  bir::Module module;
  module.globals.push_back(bir::Global{
      .name = "g.ptr.addr",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Function function;
  function.name = "stack_layout_global_pointer_addressed_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.global.ptr.addr.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.global.ptr.addr.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.global.ptr.addr.root"),
  });
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "g.ptr.addr",
      .value = bir::Value::immediate_i32(11),
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "lv.global.ptr.addr.alias"),
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.global.ptr.addr.loaded"),
      .global_name = "g.ptr.addr",
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "lv.global.ptr.addr.alias"),
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "lv.global.ptr.addr.loaded"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_store_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_store_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.store.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.store.dest",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.store.dest",
      .value = bir::Value::named(bir::TypeKind::Ptr, "lv.store.root"),
      .align_bytes = 8,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_phi_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_phi_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.phi.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::immediate_i32(1),
      .true_label = "left",
      .false_label = "right",
  };

  bir::Block left;
  left.label = "left";
  left.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.left.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.root"),
  });
  left.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block right;
  right.label = "right";
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.alias"),
      .incomings = {
          bir::PhiIncoming{
              .label = "left",
              .value = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.left.alias"),
          },
          bir::PhiIncoming{
              .label = "right",
              .value = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.root"),
          },
      },
  });
  join.insts.push_back(bir::CallInst{
      .callee = "sink_ptr",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "lv.phi.alias")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  join.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(right));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_phi_single_block_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_phi_single_block_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.phi.single.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "left"};

  bir::Block left;
  left.label = "left";
  left.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.single.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.single.root"),
  });
  left.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.single.forwarded"),
      .incomings = {
          bir::PhiIncoming{
              .label = "left",
              .value = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.single.alias"),
          },
      },
  });
  join.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_phi_multi_block_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_phi_multi_block_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.phi.multi.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::immediate_i1(true),
      .true_label = "left",
      .false_label = "right",
  };

  bir::Block left;
  left.label = "left";
  left.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.multi.left.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.multi.root"),
  });
  left.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block right;
  right.label = "right";
  right.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.multi.right.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.multi.root"),
  });
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.multi.forwarded"),
      .incomings = {
          bir::PhiIncoming{
              .label = "left",
              .value = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.multi.left.alias"),
          },
          bir::PhiIncoming{
              .label = "right",
              .value = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.multi.right.alias"),
          },
      },
  });
  join.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(right));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_mixed_phi_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_mixed_phi_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.phi.mixed.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::immediate_i1(true),
      .true_label = "left",
      .false_label = "right",
  };

  bir::Block left;
  left.label = "left";
  left.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.mixed.left.alias"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.mixed.root"),
  });
  left.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block right;
  right.label = "right";
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.mixed.forwarded"),
      .incomings = {
          bir::PhiIncoming{
              .label = "left",
              .value = bir::Value::named(bir::TypeKind::Ptr, "lv.phi.mixed.left.alias"),
          },
          bir::PhiIncoming{
              .label = "right",
              .value = bir::Value::named(bir::TypeKind::Ptr, "external.ptr"),
          },
      },
  });
  join.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(right));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_pointer_binary_escaped_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_pointer_binary_escaped_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.binary.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.binary.alias"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "lv.binary.root"),
      .rhs = bir::Value::immediate_i64(0),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "sink_ptr",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "lv.binary.alias")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_rooted_pointer_binary_local_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_rooted_pointer_binary_local_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.binary.rooted.root",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "lv.binary.rooted.alias"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "lv.binary.rooted.root"),
      .rhs = bir::Value::named(bir::TypeKind::Ptr, "lv.binary.rooted.root"),
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

int check_stack_layout_activation(const prepare::PreparedBirModule& prepared) {
  const auto* live_object = find_stack_object(prepared, "lv.live");
  const auto* dead_object = find_stack_object(prepared, "lv.dead");
  const auto* copy_object = find_stack_object(prepared, "lv.copy");
  if (live_object == nullptr || dead_object == nullptr || copy_object == nullptr) {
    return fail("expected stack-layout objects for live, dead, and copy local slots");
  }

  if (!live_object->requires_home_slot) {
    return fail("expected the live local slot to keep a home-slot requirement");
  }
  if (dead_object->requires_home_slot) {
    return fail("expected the dead local slot to drop its home-slot requirement");
  }
  if (copy_object->source_kind != "copy_coalescing_candidate") {
    return fail("expected the lowering scratch slot to become a copy-coalescing candidate");
  }
  if (copy_object->requires_home_slot) {
    return fail("expected the copy-coalescing candidate to stop advertising a dedicated home-slot requirement");
  }

  const auto* live_slot = find_frame_slot(prepared, live_object->object_id);
  const auto* dead_slot = find_frame_slot(prepared, dead_object->object_id);
  const auto* copy_slot = find_frame_slot(prepared, copy_object->object_id);
  if (live_slot == nullptr) {
    return fail("expected the live local slot to receive a frame slot");
  }
  if (dead_slot != nullptr) {
    return fail("expected the dead local slot to be elided from frame-slot assignment");
  }
  if (copy_slot != nullptr) {
    return fail("expected the copy-coalescing candidate to skip dedicated frame-slot assignment");
  }

  if (prepared.stack_layout.frame_slots.size() != 1) {
    return fail("expected only one frame slot after dead-slot and copy-coalescing elision");
  }
  if (prepared.stack_layout.frame_size_bytes != 4 ||
      prepared.stack_layout.frame_alignment_bytes != 4) {
    return fail("expected frame metrics to reflect only the live local slot");
  }

  return 0;
}

int check_addressed_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.addr.root");
  if (root_object == nullptr) {
    return fail("expected the addressed root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected addressed local-slot uses to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected addressed local-slot uses to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the addressed root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the addressed root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_packed_frame_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* small0_object = find_stack_object(prepared, "lv.pack.small0");
  const auto* big_object = find_stack_object(prepared, "lv.pack.big");
  const auto* small1_object = find_stack_object(prepared, "lv.pack.small1");
  if (small0_object == nullptr || big_object == nullptr || small1_object == nullptr) {
    return fail("expected all packed-frame local slots to produce stack-layout objects");
  }

  const auto* small0_slot = find_frame_slot(prepared, small0_object->object_id);
  const auto* big_slot = find_frame_slot(prepared, big_object->object_id);
  const auto* small1_slot = find_frame_slot(prepared, small1_object->object_id);
  if (small0_slot == nullptr || big_slot == nullptr || small1_slot == nullptr) {
    return fail("expected all packed-frame local slots to receive frame-slot storage");
  }

  if (big_slot->offset_bytes != 0) {
    return fail("expected the widest aligned home slot to be packed at frame offset 0");
  }
  if (small0_slot->offset_bytes != 8 || small1_slot->offset_bytes != 12) {
    return fail("expected smaller home slots to pack after the widest aligned slot");
  }

  if (prepared.stack_layout.frame_slots.size() != 3) {
    return fail("expected packed-frame activation to keep exactly three frame slots");
  }
  if (prepared.stack_layout.frame_size_bytes != 16 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected packed-frame activation to reduce frame metrics to the packed layout");
  }

  return 0;
}

int check_fixed_location_frame_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.fixed.root");
  const auto* wide_object = find_stack_object(prepared, "lv.fixed.wide");
  if (root_object == nullptr || wide_object == nullptr) {
    return fail("expected fixed-location activation to produce both stack-layout objects");
  }
  if (!root_object->address_exposed || !root_object->requires_home_slot) {
    return fail("expected the fixed root local slot to stay address-exposed with a dedicated home slot");
  }
  if (!root_object->permanent_home_slot) {
    return fail("expected the fixed root local slot to advertise a permanent home-slot contract");
  }
  if (wide_object->address_exposed) {
    return fail("expected the reorderable wide local slot to remain non-address-exposed");
  }
  if (wide_object->permanent_home_slot) {
    return fail("expected the non-address-exposed wide local slot to remain reorderable");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  const auto* wide_slot = find_frame_slot(prepared, wide_object->object_id);
  if (root_slot == nullptr || wide_slot == nullptr) {
    return fail("expected fixed-location activation to assign frame-slot storage to both objects");
  }

  if (!root_slot->fixed_location) {
    return fail("expected the address-exposed root local slot to use the fixed-location tier");
  }
  if (root_slot->offset_bytes != 0 || root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the fixed root local slot to anchor frame offset 0 with its original layout");
  }
  if (wide_slot->fixed_location) {
    return fail("expected the non-address-exposed wide local slot to stay reorderable");
  }
  if (wide_slot->offset_bytes != 8 || wide_slot->size_bytes != 8 || wide_slot->align_bytes != 8) {
    return fail("expected the reorderable wide local slot to pack after the fixed-location root");
  }
  if (prepared.stack_layout.frame_size_bytes != 16 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected fixed-location activation to preserve the packed frame metrics");
  }

  return 0;
}

int check_permanent_home_slot_frame_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* copy_object = find_stack_object(prepared, "lv.perm.copy");
  const auto* wide_object = find_stack_object(prepared, "lv.perm.wide");
  if (copy_object == nullptr || wide_object == nullptr) {
    return fail("expected permanent-home-slot activation to produce both stack-layout objects");
  }
  if (copy_object->source_kind != "byval_copy") {
    return fail("expected the byval-copy local slot to publish its explicit prepared source kind");
  }
  if (copy_object->address_exposed) {
    return fail("expected the byval-copy local slot to stay non-address-exposed");
  }
  if (!copy_object->requires_home_slot || !copy_object->permanent_home_slot) {
    return fail("expected the byval-copy local slot to keep a permanent dedicated home-slot contract");
  }
  if (wide_object->permanent_home_slot) {
    return fail("expected the comparison wide local slot to remain reorderable");
  }

  const auto* copy_slot = find_frame_slot(prepared, copy_object->object_id);
  const auto* wide_slot = find_frame_slot(prepared, wide_object->object_id);
  if (copy_slot == nullptr || wide_slot == nullptr) {
    return fail("expected permanent-home-slot activation to assign frame-slot storage to both objects");
  }
  if (!copy_slot->fixed_location) {
    return fail("expected the byval-copy local slot to use the fixed-location tier");
  }
  if (copy_slot->offset_bytes != 0 || copy_slot->size_bytes != 4 || copy_slot->align_bytes != 4) {
    return fail("expected the byval-copy local slot to anchor frame offset 0 with its original layout");
  }
  if (wide_slot->fixed_location) {
    return fail("expected the comparison wide local slot to remain reorderable");
  }
  if (wide_slot->offset_bytes != 8 || wide_slot->size_bytes != 8 || wide_slot->align_bytes != 8) {
    return fail("expected the reorderable wide local slot to pack after the permanent home-slot tier");
  }
  if (prepared.stack_layout.frame_size_bytes != 16 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected permanent-home-slot activation to preserve the packed frame metrics");
  }

  return 0;
}

int check_param_permanent_home_slot_frame_slot_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* byval_object = find_stack_object(prepared, "p.byval.root");
  const auto* sret_object = find_stack_object(prepared, "p.sret.root");
  const auto* wide_object = find_stack_object(prepared, "lv.param.wide");
  const auto* plain_object = find_stack_object(prepared, "p.plain");
  if (byval_object == nullptr || sret_object == nullptr || wide_object == nullptr) {
    return fail("expected parameter permanent-home activation to produce byval, sret, and comparison objects");
  }
  if (plain_object != nullptr) {
    return fail("expected plain params to stay out of the prepared stack-layout objects");
  }

  if (byval_object->source_kind != "byval_param" || !byval_object->address_exposed ||
      !byval_object->requires_home_slot || !byval_object->permanent_home_slot) {
    return fail("expected the byval param to keep its explicit permanent-home contract in the active stack-layout path");
  }
  if (sret_object->source_kind != "sret_param" || !sret_object->address_exposed ||
      !sret_object->requires_home_slot || !sret_object->permanent_home_slot) {
    return fail("expected the sret param to keep its explicit permanent-home contract in the active stack-layout path");
  }
  if (wide_object->permanent_home_slot) {
    return fail("expected the comparison local slot to remain reorderable");
  }

  const auto* byval_slot = find_frame_slot(prepared, byval_object->object_id);
  const auto* sret_slot = find_frame_slot(prepared, sret_object->object_id);
  const auto* wide_slot = find_frame_slot(prepared, wide_object->object_id);
  if (byval_slot == nullptr || sret_slot == nullptr || wide_slot == nullptr) {
    return fail("expected parameter permanent-home activation to assign frame-slot storage");
  }
  if (!byval_slot->fixed_location || !sret_slot->fixed_location) {
    return fail("expected byval and sret params to use the fixed-location tier");
  }
  if (wide_slot->fixed_location) {
    return fail("expected the comparison local slot to stay reorderable");
  }
  if (byval_slot->offset_bytes != 0 || sret_slot->offset_bytes != 8 ||
      wide_slot->offset_bytes != 16) {
    return fail("expected byval and sret param slots to anchor the fixed tier before the reorderable local slot");
  }
  if (prepared.stack_layout.frame_size_bytes != 24 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected parameter permanent-home activation to preserve the fixed-tier frame metrics");
  }

  return 0;
}

int check_param_fixed_location_ordering_activation(const prepare::PreparedBirModule& prepared) {
  const auto* byval_object = find_stack_object(prepared, "p.byval.fixed");
  const auto* sret_object = find_stack_object(prepared, "p.sret.fixed");
  const auto* local_object = find_stack_object(prepared, "lv.fixed.local.root");
  const auto* wide_object = find_stack_object(prepared, "lv.fixed.local.wide");
  if (byval_object == nullptr || sret_object == nullptr || local_object == nullptr ||
      wide_object == nullptr) {
    return fail("expected parameter-vs-local fixed-tier activation to produce params, fixed local, and comparison objects");
  }
  if (!byval_object->permanent_home_slot || !sret_object->permanent_home_slot ||
      !local_object->permanent_home_slot) {
    return fail("expected the byval param, sret param, and addressed local to remain fixed-tier objects");
  }
  if (wide_object->permanent_home_slot) {
    return fail("expected the comparison wide local slot to remain reorderable");
  }

  const auto* byval_slot = find_frame_slot(prepared, byval_object->object_id);
  const auto* sret_slot = find_frame_slot(prepared, sret_object->object_id);
  const auto* local_slot = find_frame_slot(prepared, local_object->object_id);
  const auto* wide_slot = find_frame_slot(prepared, wide_object->object_id);
  if (byval_slot == nullptr || sret_slot == nullptr || local_slot == nullptr ||
      wide_slot == nullptr) {
    return fail("expected parameter-vs-local fixed-tier activation to assign frame-slot storage");
  }
  if (!byval_slot->fixed_location || !sret_slot->fixed_location || !local_slot->fixed_location) {
    return fail("expected byval, sret, and addressed local objects to use the fixed-location tier");
  }
  if (wide_slot->fixed_location) {
    return fail("expected the comparison wide local slot to stay reorderable");
  }
  if (byval_slot->offset_bytes != 0 || sret_slot->offset_bytes != 8 ||
      local_slot->offset_bytes != 16 || wide_slot->offset_bytes != 24) {
    return fail("expected parameter-owned fixed slots to anchor ahead of the later fixed-location local");
  }
  if (prepared.stack_layout.frame_size_bytes != 32 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected parameter-vs-local fixed-tier activation to preserve the aligned frame metrics");
  }

  return 0;
}

int check_lowering_scratch_frame_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* scratch_object = find_stack_object(prepared, "lv.scratch.root");
  const auto* wide_object = find_stack_object(prepared, "lv.scratch.wide");
  if (scratch_object == nullptr || wide_object == nullptr) {
    return fail("expected lowering-scratch activation to produce both stack-layout objects");
  }
  if (scratch_object->source_kind != "lowering_scratch") {
    return fail("expected the lowering-scratch local slot to publish its explicit prepared source kind");
  }
  if (scratch_object->address_exposed) {
    return fail("expected the generic lowering-scratch local slot to stay non-address-exposed");
  }
  if (!scratch_object->requires_home_slot) {
    return fail("expected the generic lowering-scratch local slot to keep its home-slot requirement");
  }
  if (scratch_object->permanent_home_slot) {
    return fail("expected the generic lowering-scratch local slot to stay reorderable without an explicit permanent-home contract");
  }

  const auto* scratch_slot = find_frame_slot(prepared, scratch_object->object_id);
  const auto* wide_slot = find_frame_slot(prepared, wide_object->object_id);
  if (scratch_slot == nullptr || wide_slot == nullptr) {
    return fail("expected lowering-scratch activation to assign frame-slot storage to both objects");
  }
  if (scratch_slot->fixed_location) {
    return fail("expected the generic lowering-scratch local slot to stay out of the fixed-location tier");
  }
  if (wide_slot->fixed_location) {
    return fail("expected the comparison wide local slot to remain reorderable");
  }
  if (wide_slot->offset_bytes != 0 || wide_slot->size_bytes != 8 || wide_slot->align_bytes != 8) {
    return fail("expected the reorderable wide local slot to anchor frame offset 0");
  }
  if (scratch_slot->offset_bytes != 8 || scratch_slot->size_bytes != 4 ||
      scratch_slot->align_bytes != 4) {
    return fail("expected the generic lowering-scratch local slot to pack after the wider reorderable slot");
  }
  if (prepared.stack_layout.frame_size_bytes != 16 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected lowering-scratch activation to preserve the packed frame metrics");
  }

  return 0;
}

int check_dead_lowering_scratch_frame_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* dead_scratch_object = find_stack_object(prepared, "lv.scratch.dead");
  const auto* live_object = find_stack_object(prepared, "lv.scratch.live");
  if (dead_scratch_object == nullptr || live_object == nullptr) {
    return fail("expected dead lowering-scratch activation to produce both stack-layout objects");
  }
  if (dead_scratch_object->source_kind != "lowering_scratch") {
    return fail("expected the dead generic lowering-scratch slot to keep its explicit prepared source kind");
  }
  if (dead_scratch_object->address_exposed) {
    return fail("expected the dead generic lowering-scratch slot to stay non-address-exposed");
  }
  if (dead_scratch_object->requires_home_slot) {
    return fail("expected the dead generic lowering-scratch slot to drop its dedicated home-slot requirement");
  }
  if (dead_scratch_object->permanent_home_slot) {
    return fail("expected the dead generic lowering-scratch slot to stay out of the permanent-home tier");
  }

  const auto* dead_slot = find_frame_slot(prepared, dead_scratch_object->object_id);
  const auto* live_slot = find_frame_slot(prepared, live_object->object_id);
  if (dead_slot != nullptr) {
    return fail("expected the dead generic lowering-scratch slot to skip dedicated frame-slot assignment");
  }
  if (live_slot == nullptr) {
    return fail("expected the comparison live local slot to keep frame-slot storage");
  }
  if (live_slot->offset_bytes != 0 || live_slot->size_bytes != 8 || live_slot->align_bytes != 8) {
    return fail("expected the live comparison local slot to anchor the remaining frame layout");
  }
  if (prepared.stack_layout.frame_slots.size() != 1) {
    return fail("expected dead lowering-scratch activation to leave only one frame slot");
  }
  if (prepared.stack_layout.frame_size_bytes != 8 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected dead lowering-scratch activation to shrink the frame metrics to the live slot");
  }

  return 0;
}

int check_stack_layout_analysis_object_collection_activation(
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* scratch_object = find_stack_object(objects, "lv.analysis.scratch");
  const auto* copy_object = find_stack_object(objects, "lv.analysis.byval_copy");
  const auto* phi_object = find_stack_object(objects, "lv.analysis.phi");
  const auto* addressed_object = find_stack_object(objects, "lv.analysis.addr");
  if (scratch_object == nullptr || copy_object == nullptr || phi_object == nullptr ||
      addressed_object == nullptr) {
    return fail("expected analysis object collection to publish all local-slot objects");
  }

  if (scratch_object->source_kind != "lowering_scratch") {
    return fail("expected generic lowering-scratch analysis objects to retain their explicit source kind");
  }
  if (scratch_object->requires_home_slot || scratch_object->permanent_home_slot ||
      scratch_object->address_exposed) {
    return fail("expected generic lowering-scratch analysis objects to skip home-slot requirements without an explicit contract");
  }

  if (!copy_object->requires_home_slot || !copy_object->permanent_home_slot) {
    return fail("expected byval-copy analysis objects to keep their explicit home-slot contract");
  }
  if (!phi_object->requires_home_slot || !phi_object->permanent_home_slot) {
    return fail("expected phi analysis objects to keep their explicit home-slot contract");
  }
  if (!addressed_object->requires_home_slot || !addressed_object->permanent_home_slot ||
      !addressed_object->address_exposed) {
    return fail("expected address-taken analysis objects to keep their explicit home-slot contract");
  }

  return 0;
}

int check_stack_layout_param_object_collection_activation(
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* byval_object = find_stack_object(objects, "p.analysis.byval");
  const auto* sret_object = find_stack_object(objects, "p.analysis.sret");
  const auto* plain_object = find_stack_object(objects, "p.analysis.plain");
  if (byval_object == nullptr || sret_object == nullptr) {
    return fail("expected byval and sret params to publish prepared stack objects");
  }
  if (plain_object != nullptr) {
    return fail("expected plain params to stay out of stack-layout object collection");
  }

  if (byval_object->source_kind != "byval_param") {
    return fail("expected byval params to publish their explicit prepared source kind");
  }
  if (!byval_object->address_exposed || !byval_object->requires_home_slot ||
      !byval_object->permanent_home_slot) {
    return fail("expected byval params to keep an explicit permanent home-slot contract");
  }

  if (sret_object->source_kind != "sret_param") {
    return fail("expected sret params to publish their explicit prepared source kind");
  }
  if (!sret_object->address_exposed || !sret_object->requires_home_slot ||
      !sret_object->permanent_home_slot) {
    return fail("expected sret params to keep an explicit permanent home-slot contract");
  }

  return 0;
}

int check_stack_layout_regalloc_hint_activation(
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* scratch_object = find_stack_object(objects, "lv.analysis.scratch");
  const auto* copy_object = find_stack_object(objects, "lv.analysis.byval_copy");
  const auto* phi_object = find_stack_object(objects, "lv.analysis.phi");
  const auto* addressed_object = find_stack_object(objects, "lv.analysis.addr");
  if (scratch_object == nullptr || copy_object == nullptr || phi_object == nullptr ||
      addressed_object == nullptr) {
    return fail("expected regalloc hints to preserve all analysis objects");
  }

  if (scratch_object->source_kind != "lowering_scratch") {
    return fail("expected generic lowering-scratch regalloc-hint objects to retain their explicit source kind");
  }
  if (scratch_object->requires_home_slot || scratch_object->permanent_home_slot ||
      scratch_object->address_exposed) {
    return fail("expected generic lowering-scratch regalloc-hint objects to skip home-slot widening without explicit contracts or real use data");
  }

  if (!copy_object->requires_home_slot || !copy_object->permanent_home_slot) {
    return fail("expected byval-copy regalloc-hint objects to keep their explicit home-slot contract");
  }
  if (!phi_object->requires_home_slot || !phi_object->permanent_home_slot) {
    return fail("expected phi regalloc-hint objects to keep their explicit home-slot contract");
  }
  if (!addressed_object->requires_home_slot || !addressed_object->permanent_home_slot ||
      !addressed_object->address_exposed) {
    return fail("expected address-taken regalloc-hint objects to keep their explicit home-slot contract");
  }

  return 0;
}

int check_stack_layout_param_regalloc_hint_activation(
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* byval_object = find_stack_object(objects, "p.analysis.byval");
  const auto* sret_object = find_stack_object(objects, "p.analysis.sret");
  const auto* plain_object = find_stack_object(objects, "p.analysis.plain");
  if (byval_object == nullptr || sret_object == nullptr) {
    return fail("expected regalloc hints to preserve byval and sret parameter objects");
  }
  if (plain_object != nullptr) {
    return fail("expected regalloc hints to keep plain params out of stack-layout object collection");
  }

  if (!byval_object->address_exposed || !byval_object->requires_home_slot ||
      !byval_object->permanent_home_slot) {
    return fail("expected byval params to keep their explicit permanent-home contract through regalloc hints");
  }
  if (!sret_object->address_exposed || !sret_object->requires_home_slot ||
      !sret_object->permanent_home_slot) {
    return fail("expected sret params to keep their explicit permanent-home contract through regalloc hints");
  }

  return 0;
}

int check_phi_permanent_home_slot_frame_slot_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.phi.perm.root");
  const auto* wide_object = find_stack_object(prepared, "lv.phi.perm.wide");
  if (root_object == nullptr || wide_object == nullptr) {
    return fail("expected phi permanent-home-slot activation to produce both stack-layout objects");
  }
  if (root_object->source_kind != "phi") {
    return fail("expected the phi-observed local slot to publish its explicit prepared source kind");
  }
  if (root_object->address_exposed) {
    return fail("expected the phi-observed local slot to stay non-address-exposed");
  }
  if (!root_object->requires_home_slot || !root_object->permanent_home_slot) {
    return fail("expected the phi-observed local slot to keep a permanent dedicated home-slot contract");
  }
  if (wide_object->permanent_home_slot) {
    return fail("expected the comparison wide local slot to remain reorderable");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  const auto* wide_slot = find_frame_slot(prepared, wide_object->object_id);
  if (root_slot == nullptr || wide_slot == nullptr) {
    return fail("expected phi permanent-home-slot activation to assign frame-slot storage to both objects");
  }
  if (!root_slot->fixed_location) {
    return fail("expected the phi-observed local slot to use the fixed-location tier");
  }
  if (root_slot->offset_bytes != 0 || root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the phi-observed local slot to anchor frame offset 0 with its original layout");
  }
  if (wide_slot->fixed_location) {
    return fail("expected the comparison wide local slot to remain reorderable");
  }
  if (wide_slot->offset_bytes != 8 || wide_slot->size_bytes != 8 || wide_slot->align_bytes != 8) {
    return fail("expected the reorderable wide local slot to pack after the phi permanent-home tier");
  }
  if (prepared.stack_layout.frame_size_bytes != 16 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected phi permanent-home-slot activation to preserve the packed frame metrics");
  }

  return 0;
}

int check_call_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.call.root");
  if (root_object == nullptr) {
    return fail("expected the call-escaped root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected pointer call args to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected pointer call args to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the call-escaped root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the call-escaped root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_sret_storage_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.call.sret.storage");
  if (root_object == nullptr) {
    return fail("expected the sret-storage local slot to produce a stack-layout object");
  }
  if (root_object->address_exposed) {
    return fail("expected sret-storage tracking to keep the local slot live without marking it address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected sret-storage tracking to keep a dedicated home-slot requirement");
  }
  if (!root_object->permanent_home_slot) {
    return fail("expected sret-storage tracking to publish a permanent dedicated home-slot contract");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the sret-storage local slot to receive frame-slot storage");
  }
  if (!root_slot->fixed_location) {
    return fail("expected the sret-storage local slot to use the fixed-location tier");
  }
  if (root_slot->offset_bytes != 0) {
    return fail("expected the sret-storage local slot to anchor frame offset 0");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the sret-storage local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_indirect_call_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.call.indirect.root");
  if (root_object == nullptr) {
    return fail("expected the indirect-call root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected an indirect callee pointer alias to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected an indirect callee pointer alias to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the indirect-call root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the indirect-call root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_select_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.select.root");
  if (root_object == nullptr) {
    return fail("expected the select-escaped root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected pointer selects that later escape to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected pointer selects that later escape to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the select-escaped root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the select-escaped root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_mixed_select_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.select.mixed.root");
  if (root_object == nullptr) {
    return fail("expected the mixed-select root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected a rooted-plus-unrooted pointer select to conservatively expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected a rooted-plus-unrooted pointer select to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the mixed-select root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the mixed-select root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_rooted_select_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.select.rooted.root");
  if (root_object == nullptr) {
    return fail("expected the rooted-select root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected rooted-only pointer select value merges to expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected rooted-only pointer select value merges to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the rooted-select local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the rooted-select local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_select_compare_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.select.compare.root");
  if (root_object == nullptr) {
    return fail("expected the select-compare root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail(
        "expected rooted pointer aliases used only in select comparisons to expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected rooted pointer aliases used only in select comparisons to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the select-compare root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the select-compare root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_rooted_select_compare_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.select.compare.rooted.root");
  if (root_object == nullptr) {
    return fail("expected the rooted select-compare root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail(
        "expected rooted pointer operands used only in select comparisons to expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail(
        "expected rooted pointer operands used only in select comparisons to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the rooted select-compare root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the rooted select-compare root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_cast_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.cast.root");
  if (root_object == nullptr) {
    return fail("expected the cast-escaped root local slot to produce a stack-layout object");
  }
  if (root_object->address_exposed) {
    return fail("expected rooted pointer casts without later escape to keep the root slot non-exposed");
  }
  if (root_object->requires_home_slot) {
    return fail("expected rooted pointer casts without later escape to drop the dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot != nullptr) {
    return fail("expected the cast-only rooted local slot to skip dedicated frame-slot assignment");
  }

  return 0;
}

int check_return_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.ret.root");
  if (root_object == nullptr) {
    return fail("expected the return-escaped root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected a returned pointer local slot to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected a returned pointer local slot to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the return-escaped root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the return-escaped root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_cond_branch_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.cond.root");
  if (root_object == nullptr) {
    return fail("expected the cond-branch root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected a pointer cond_br condition to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected a pointer cond_br condition to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the cond-branch root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the cond-branch root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_pointer_addressed_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.ptr.addr.root");
  if (root_object == nullptr) {
    return fail("expected the pointer-addressed root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected pointer-based memory addresses to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected pointer-based memory addresses to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the pointer-addressed root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail("expected the pointer-addressed root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_global_pointer_addressed_local_slot_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.global.ptr.addr.root");
  if (root_object == nullptr) {
    return fail(
        "expected the global-pointer-addressed root local slot to produce a "
        "stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail(
        "expected global pointer-based memory addresses to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail(
        "expected global pointer-based memory addresses to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail(
        "expected the global-pointer-addressed root local slot to receive "
        "frame-slot storage");
  }
  if (root_slot->size_bytes != 4 || root_slot->align_bytes != 4) {
    return fail(
        "expected the global-pointer-addressed root local slot to preserve "
        "its frame-slot layout");
  }

  return 0;
}

int check_store_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.store.root");
  if (root_object == nullptr) {
    return fail("expected the stored-pointer root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected storing a pointer local slot value to mark the root slot address-exposed");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected storing a pointer local slot value to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the stored-pointer root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the stored-pointer root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_phi_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.phi.root");
  if (root_object == nullptr) {
    return fail("expected the phi-escaped root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected a pointer phi alias that later escapes to expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail(
        "expected a pointer phi alias that later escapes to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the phi-escaped root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the phi-escaped root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_phi_single_block_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.phi.single.root");
  if (root_object == nullptr) {
    return fail("expected the single-block phi root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected rooted-only phi merges to expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected rooted-only phi merges to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the single-block rooted phi path to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the single-block rooted phi path to preserve its frame-slot layout");
  }

  return 0;
}

int check_phi_multi_block_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.phi.multi.root");
  if (root_object == nullptr) {
    return fail("expected the multi-block phi root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected rooted-only multi-block phi merges to expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected rooted-only multi-block phi merges to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the multi-block rooted phi path to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the multi-block rooted phi path to preserve its frame-slot layout");
  }

  return 0;
}

int check_mixed_phi_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.phi.mixed.root");
  if (root_object == nullptr) {
    return fail("expected the mixed-phi root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected a rooted-plus-unrooted pointer phi to conservatively expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected a rooted-plus-unrooted pointer phi to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the mixed-phi root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the mixed-phi root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_pointer_binary_escaped_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.binary.root");
  if (root_object == nullptr) {
    return fail("expected the pointer-binary root local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected a pointer-valued binary alias that later escapes to expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail(
        "expected a pointer-valued binary alias that later escapes to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the pointer-binary root local slot to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the pointer-binary root local slot to preserve its frame-slot layout");
  }

  return 0;
}

int check_rooted_pointer_binary_local_slot_activation(const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.binary.rooted.root");
  if (root_object == nullptr) {
    return fail("expected the rooted-only pointer-binary local slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed) {
    return fail("expected rooted-only pointer binary operands to expose the root slot");
  }
  if (!root_object->requires_home_slot) {
    return fail("expected rooted-only pointer binary operands to keep a dedicated home-slot requirement");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  if (root_slot == nullptr) {
    return fail("expected the rooted-only pointer-binary path to receive frame-slot storage");
  }
  if (root_slot->size_bytes != 8 || root_slot->align_bytes != 8) {
    return fail("expected the rooted-only pointer-binary path to preserve its frame-slot layout");
  }

  return 0;
}

}  // namespace

int main() {
  const auto analysis_objects = collect_stack_layout_analysis_objects();
  if (const int rc = check_stack_layout_analysis_object_collection_activation(analysis_objects);
      rc != 0) {
    return rc;
  }

  const auto param_analysis_objects = collect_stack_layout_param_objects();
  if (const int rc =
          check_stack_layout_param_object_collection_activation(param_analysis_objects);
      rc != 0) {
    return rc;
  }

  const auto regalloc_hint_objects = collect_stack_layout_regalloc_hint_objects();
  if (const int rc = check_stack_layout_regalloc_hint_activation(regalloc_hint_objects);
      rc != 0) {
    return rc;
  }

  const auto param_regalloc_hint_objects = collect_stack_layout_param_regalloc_hint_objects();
  if (const int rc =
          check_stack_layout_param_regalloc_hint_activation(param_regalloc_hint_objects);
      rc != 0) {
    return rc;
  }

  const auto copy_prepared = prepare_stack_layout_module();
  if (const int rc = check_stack_layout_activation(copy_prepared); rc != 0) {
    return rc;
  }

  const auto addressed_prepared = prepare_addressed_local_slot_module();
  if (const int rc = check_addressed_local_slot_activation(addressed_prepared); rc != 0) {
    return rc;
  }

  const auto packed_frame_prepared = prepare_packed_frame_slot_module();
  if (const int rc = check_packed_frame_slot_activation(packed_frame_prepared); rc != 0) {
    return rc;
  }

  const auto fixed_location_prepared = prepare_fixed_location_frame_slot_module();
  if (const int rc = check_fixed_location_frame_slot_activation(fixed_location_prepared);
      rc != 0) {
    return rc;
  }

  const auto permanent_home_slot_prepared = prepare_permanent_home_slot_frame_slot_module();
  if (const int rc =
          check_permanent_home_slot_frame_slot_activation(permanent_home_slot_prepared);
      rc != 0) {
    return rc;
  }

  const auto param_permanent_home_slot_prepared = prepare_param_permanent_home_slot_module();
  if (const int rc =
          check_param_permanent_home_slot_frame_slot_activation(
              param_permanent_home_slot_prepared);
      rc != 0) {
    return rc;
  }

  const auto param_fixed_location_ordering_prepared =
      prepare_param_fixed_location_ordering_module();
  if (const int rc =
          check_param_fixed_location_ordering_activation(param_fixed_location_ordering_prepared);
      rc != 0) {
    return rc;
  }

  const auto lowering_scratch_prepared = prepare_lowering_scratch_frame_slot_module();
  if (const int rc = check_lowering_scratch_frame_slot_activation(lowering_scratch_prepared);
      rc != 0) {
    return rc;
  }

  const auto dead_lowering_scratch_prepared = prepare_dead_lowering_scratch_frame_slot_module();
  if (const int rc =
          check_dead_lowering_scratch_frame_slot_activation(dead_lowering_scratch_prepared);
      rc != 0) {
    return rc;
  }

  const auto phi_permanent_home_slot_prepared =
      prepare_phi_permanent_home_slot_frame_slot_module();
  if (const int rc =
          check_phi_permanent_home_slot_frame_slot_activation(phi_permanent_home_slot_prepared);
      rc != 0) {
    return rc;
  }

  const auto call_escaped_prepared = prepare_call_escaped_local_slot_module();
  if (const int rc = check_call_escaped_local_slot_activation(call_escaped_prepared); rc != 0) {
    return rc;
  }

  const auto sret_storage_prepared = prepare_sret_storage_local_slot_module();
  if (const int rc = check_sret_storage_local_slot_activation(sret_storage_prepared); rc != 0) {
    return rc;
  }

  const auto indirect_call_escaped_prepared = prepare_indirect_call_escaped_local_slot_module();
  if (const int rc =
          check_indirect_call_escaped_local_slot_activation(indirect_call_escaped_prepared);
      rc != 0) {
    return rc;
  }

  const auto select_escaped_prepared = prepare_select_escaped_local_slot_module();
  if (const int rc = check_select_escaped_local_slot_activation(select_escaped_prepared); rc != 0) {
    return rc;
  }

  const auto mixed_select_prepared = prepare_mixed_select_local_slot_module();
  if (const int rc = check_mixed_select_local_slot_activation(mixed_select_prepared); rc != 0) {
    return rc;
  }

  const auto rooted_select_prepared = prepare_rooted_select_local_slot_module();
  if (const int rc = check_rooted_select_local_slot_activation(rooted_select_prepared); rc != 0) {
    return rc;
  }

  const auto select_compare_prepared = prepare_select_compare_escaped_local_slot_module();
  if (const int rc =
          check_select_compare_escaped_local_slot_activation(select_compare_prepared);
      rc != 0) {
    return rc;
  }

  const auto rooted_select_compare_prepared = prepare_rooted_select_compare_local_slot_module();
  if (const int rc =
          check_rooted_select_compare_local_slot_activation(rooted_select_compare_prepared);
      rc != 0) {
    return rc;
  }

  const auto cast_escaped_prepared = prepare_cast_escaped_local_slot_module();
  if (const int rc = check_cast_escaped_local_slot_activation(cast_escaped_prepared); rc != 0) {
    return rc;
  }

  const auto return_escaped_prepared = prepare_return_escaped_local_slot_module();
  if (const int rc = check_return_escaped_local_slot_activation(return_escaped_prepared); rc != 0) {
    return rc;
  }

  const auto cond_branch_escaped_prepared = prepare_cond_branch_escaped_local_slot_module();
  if (const int rc =
          check_cond_branch_escaped_local_slot_activation(cond_branch_escaped_prepared);
      rc != 0) {
    return rc;
  }

  const auto pointer_addressed_prepared = prepare_pointer_addressed_local_slot_module();
  if (const int rc = check_pointer_addressed_local_slot_activation(pointer_addressed_prepared);
      rc != 0) {
    return rc;
  }

  const auto global_pointer_addressed_prepared =
      prepare_global_pointer_addressed_local_slot_module();
  if (const int rc =
          check_global_pointer_addressed_local_slot_activation(global_pointer_addressed_prepared);
      rc != 0) {
    return rc;
  }

  const auto store_escaped_prepared = prepare_store_escaped_local_slot_module();
  if (const int rc = check_store_escaped_local_slot_activation(store_escaped_prepared); rc != 0) {
    return rc;
  }

  const auto phi_escaped_prepared = prepare_phi_escaped_local_slot_module();
  if (const int rc = check_phi_escaped_local_slot_activation(phi_escaped_prepared); rc != 0) {
    return rc;
  }

  const auto phi_single_block_prepared = prepare_phi_single_block_local_slot_module();
  if (const int rc = check_phi_single_block_local_slot_activation(phi_single_block_prepared);
      rc != 0) {
    return rc;
  }

  const auto phi_multi_block_prepared = prepare_phi_multi_block_local_slot_module();
  if (const int rc = check_phi_multi_block_local_slot_activation(phi_multi_block_prepared);
      rc != 0) {
    return rc;
  }

  const auto mixed_phi_prepared = prepare_mixed_phi_local_slot_module();
  if (const int rc = check_mixed_phi_local_slot_activation(mixed_phi_prepared); rc != 0) {
    return rc;
  }

  const auto pointer_binary_escaped_prepared = prepare_pointer_binary_escaped_local_slot_module();
  if (const int rc =
          check_pointer_binary_escaped_local_slot_activation(pointer_binary_escaped_prepared);
      rc != 0) {
    return rc;
  }

  const auto rooted_pointer_binary_prepared = prepare_rooted_pointer_binary_local_slot_module();
  if (const int rc =
          check_rooted_pointer_binary_local_slot_activation(rooted_pointer_binary_prepared);
      rc != 0) {
    return rc;
  }

  return 0;
}
