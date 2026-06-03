#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/regalloc/value_homes.hpp"
#include "src/target_profile.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
namespace prepare = c4c::backend::prepare;

struct PointerCarrierContractFixture {
  prepare::PreparedNameTables names;
  bir::Module module;
  bir::Function function;
  prepare::PreparedAddressingFunction addressing;
  prepare::PreparedRegallocFunction regalloc;
};

c4c::TargetProfile riscv_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::Riscv64);
}

c4c::TargetProfile aarch64_pic_target_profile() {
  auto profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  profile.relocation_model = c4c::TargetRelocationModel::Pic;
  return profile;
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const prepare::PreparedStackObject* find_stack_object(const prepare::PreparedBirModule& prepared,
                                                      std::string_view source_name) {
  for (const auto& object : prepared.stack_layout.objects) {
    if (prepare::prepared_stack_object_name(prepared.names, object) == source_name) {
      return &object;
    }
  }
  return nullptr;
}

const prepare::PreparedStackObject* find_stack_object(
    const prepare::PreparedNameTables& names,
    const std::vector<prepare::PreparedStackObject>& objects,
    std::string_view source_name) {
  for (const auto& object : objects) {
    if (prepare::prepared_stack_object_name(names, object) == source_name) {
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

c4c::FunctionNameId find_function_name_id(const prepare::PreparedBirModule& prepared,
                                          std::string_view function_name) {
  return prepared.names.function_names.find(function_name);
}

c4c::BlockLabelId find_block_label_id(const prepare::PreparedBirModule& prepared,
                                      std::string_view block_label) {
  return prepared.names.block_labels.find(block_label);
}

const prepare::PreparedValueHome* find_value_home_by_name(
    const prepare::PreparedNameTables& names,
    const std::vector<prepare::PreparedValueHome>& homes,
    std::string_view value_name) {
  const auto value_name_id = names.value_names.find(value_name);
  if (value_name_id == c4c::kInvalidValueName) {
    return nullptr;
  }
  for (const auto& home : homes) {
    if (home.value_name == value_name_id) {
      return &home;
    }
  }
  return nullptr;
}

c4c::BlockLabelId block_label_id(bir::Module& module, std::string_view label) {
  return module.names.block_labels.intern(label);
}

bir::Block make_block(bir::Module& module, std::string_view label) {
  return bir::Block{
      .label = std::string(label),
      .label_id = block_label_id(module, label),
  };
}

bir::BranchTerminator make_branch(bir::Module& module, std::string_view target_label) {
  return bir::BranchTerminator{
      .target_label = std::string(target_label),
      .target_label_id = block_label_id(module, target_label),
  };
}

bir::PhiIncoming make_phi_incoming(bir::Module& module,
                                   std::string_view label,
                                   bir::Value value) {
  return bir::PhiIncoming{
      .label = std::string(label),
      .value = std::move(value),
      .label_id = block_label_id(module, label),
  };
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

bir::NameTables intern_local_slot_ids(bir::Function& function) {
  bir::NameTables names;
  for (auto& slot : function.local_slots) {
    slot.slot_id = names.slot_names.intern(slot.name);
  }
  return names;
}

std::pair<prepare::PreparedNameTables, std::vector<prepare::PreparedStackObject>>
collect_stack_layout_analysis_objects_with_names() {
  bir::Function function = make_stack_layout_analysis_object_collection_function();
  bir::NameTables bir_names = intern_local_slot_ids(function);
  prepare::PreparedNameTables names;
  prepare::PreparedObjectId next_object_id = 0;
  auto objects =
      prepare::stack_layout::collect_function_stack_objects(names, bir_names, function, next_object_id);
  return {std::move(names), std::move(objects)};
}

std::pair<prepare::PreparedNameTables, std::vector<prepare::PreparedStackObject>>
collect_stack_layout_regalloc_hint_objects() {
  bir::Function function = make_stack_layout_analysis_object_collection_function();
  bir::NameTables bir_names = intern_local_slot_ids(function);
  prepare::PreparedNameTables names;
  prepare::PreparedObjectId next_object_id = 0;
  auto objects =
      prepare::stack_layout::collect_function_stack_objects(names, bir_names, function, next_object_id);
  prepare::stack_layout::apply_regalloc_hints(names,
                                              function,
                                              prepare::stack_layout::FunctionInlineAsmSummary{},
                                              objects);
  return {std::move(names), std::move(objects)};
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

std::pair<prepare::PreparedNameTables, std::vector<prepare::PreparedStackObject>>
collect_stack_layout_param_objects_with_names() {
  bir::Function function = make_stack_layout_param_object_collection_function();
  bir::NameTables bir_names = intern_local_slot_ids(function);
  prepare::PreparedNameTables names;
  prepare::PreparedObjectId next_object_id = 0;
  auto objects =
      prepare::stack_layout::collect_function_stack_objects(names, bir_names, function, next_object_id);
  return {std::move(names), std::move(objects)};
}

std::pair<prepare::PreparedNameTables, std::vector<prepare::PreparedStackObject>>
collect_stack_layout_param_regalloc_hint_objects() {
  bir::Function function = make_stack_layout_param_object_collection_function();
  bir::NameTables bir_names = intern_local_slot_ids(function);
  prepare::PreparedNameTables names;
  prepare::PreparedObjectId next_object_id = 0;
  auto objects =
      prepare::stack_layout::collect_function_stack_objects(names, bir_names, function, next_object_id);
  prepare::stack_layout::apply_regalloc_hints(names,
                                              function,
                                              prepare::stack_layout::FunctionInlineAsmSummary{},
                                              objects);
  return {std::move(names), std::move(objects)};
}

prepare::PreparedBirModule prepare_stack_layout_module() {
  bir::Module module;
  module.globals.push_back(bir::Global{
      .name = "g.counter",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.string_constants.push_back(bir::StringConstant{
      .name = ".L.str0",
      .name_id = module.names.texts.intern(".L.str0"),
      .bytes = "stack-layout",
      .align_bytes = 1,
  });

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
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "lv.live",
          .size_bytes = 4,
          .align_bytes = 4,
          .address_space = bir::AddressSpace::Fs,
          .is_volatile = true,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .slot_name = "lv.live",
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "lv.live",
          .size_bytes = 4,
          .align_bytes = 4,
          .address_space = bir::AddressSpace::Gs,
      },
  });
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "g.counter",
      .value = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "g.counter",
          .size_bytes = 4,
          .align_bytes = 4,
          .address_space = bir::AddressSpace::Tls,
          .is_volatile = true,
      },
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "message.ptr"),
      .global_name = "g.counter",
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::StringConstant,
          .base_name = ".L.str0",
          .size_bytes = 8,
          .align_bytes = 8,
          .address_space = bir::AddressSpace::Fs,
          .is_volatile = true,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.live",
      .value = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "g.counter",
          .size_bytes = 4,
          .align_bytes = 4,
          .address_space = bir::AddressSpace::Gs,
          .is_volatile = true,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "local.message.ptr"),
      .slot_name = "lv.live",
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::StringConstant,
          .base_name = ".L.str0",
          .size_bytes = 8,
          .align_bytes = 8,
          .address_space = bir::AddressSpace::Tls,
      },
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
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_direct_call_string_argument_module() {
  bir::Module module;
  module.string_constants.push_back(bir::StringConstant{
      .name = ".L.str.call",
      .name_id = module.names.texts.intern(".L.str.call"),
      .bytes = "call-arg",
      .align_bytes = 1,
  });

  bir::Function function;
  function.name = "stack_layout_direct_call_string_argument_activation";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id(module, "entry");
  entry.insts.push_back(bir::CallInst{
      .callee = "puts",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.L.str.call")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_indirect_call_string_argument_module() {
  bir::Module module;
  module.string_constants.push_back(bir::StringConstant{
      .name = ".L.str.indirect.call",
      .name_id = module.names.texts.intern(".L.str.indirect.call"),
      .bytes = "indirect-call-arg",
      .align_bytes = 1,
  });

  bir::Function function;
  function.name = "stack_layout_indirect_call_string_argument_activation";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id(module, "entry");
  entry.insts.push_back(bir::CallInst{
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "callee.ptr"),
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.L.str.indirect.call")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

PointerCarrierContractFixture make_pointer_carrier_contract_fixture() {
  PointerCarrierContractFixture fixture;
  fixture.function.name = "pointer_carrier_contract_activation";
  fixture.function.return_type = bir::TypeKind::I32;

  const auto function_name = fixture.names.function_names.intern(fixture.function.name);
  const auto entry_label = fixture.names.block_labels.intern("entry");
  fixture.regalloc.function_name = function_name;
  fixture.addressing.function_name = function_name;

  const auto intern_value = [&](std::string_view name) {
    return fixture.names.value_names.intern(name);
  };
  const auto symbol_ptr = intern_value("contract.symbol.ptr");
  const auto symbol_copy = intern_value("contract.symbol.copy.ptr");
  const auto missing_ptr = intern_value("contract.missing.ptr");
  const auto missing_copy = intern_value("contract.missing.copy.ptr");
  const auto base_ptr = intern_value("contract.base.ptr");
  const auto prepared_copy = intern_value("contract.prepared.copy.ptr");
  const auto adjacent_advanced = intern_value("contract.adjacent.advanced.ptr");
  const auto adjacent_after = intern_value("contract.adjacent.after.ptr");
  const auto local_recent = intern_value("contract.local.recent.ptr");
  const auto local_previous = intern_value("contract.local.previous.ptr");
  const auto global_recent = intern_value("contract.global.recent.ptr");
  const auto global_previous = intern_value("contract.global.previous.ptr");
  const auto symbol_name = fixture.module.names.link_names.intern("contract.symbol");

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id(fixture.module, "entry");
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named_symbol_pointer("contract.symbol.ptr", symbol_name),
      .global_name = "contract.symbol",
      .global_name_id = symbol_name,
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.contract.symbol",
      .value = bir::Value::named(bir::TypeKind::Ptr, "contract.symbol.ptr"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "contract.symbol.copy.ptr"),
      .slot_name = "lv.contract.symbol",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "contract.missing.ptr"),
      .global_name = "contract.missing",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.contract.missing",
      .value = bir::Value::named(bir::TypeKind::Ptr, "contract.missing.ptr"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "contract.missing.copy.ptr"),
      .slot_name = "lv.contract.missing",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.contract.prepared",
      .value = bir::Value::named(bir::TypeKind::Ptr, "contract.base.ptr"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "contract.prepared.copy.ptr"),
      .slot_name = "lv.contract.prepared",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.contract.prepared",
      .value = bir::Value::named(bir::TypeKind::Ptr, "contract.adjacent.advanced.ptr"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "contract.adjacent.after.ptr"),
      .slot_name = "lv.contract.prepared",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named_symbol_pointer("contract.local.recent.ptr", symbol_name),
      .global_name = "contract.symbol",
      .global_name_id = symbol_name,
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.contract.local.previous",
      .value = bir::Value::named(bir::TypeKind::Ptr, "contract.local.previous.ptr"),
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "contract.base.ptr"),
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named_symbol_pointer("contract.global.recent.ptr", symbol_name),
      .global_name = "contract.symbol",
      .global_name_id = symbol_name,
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "contract.global.sink",
      .value = bir::Value::named(bir::TypeKind::Ptr, "contract.global.previous.ptr"),
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "contract.base.ptr"),
          .size_bytes = 8,
          .align_bytes = 8,
      },
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  fixture.function.blocks.push_back(std::move(entry));

  fixture.addressing.accesses.push_back(prepare::PreparedMemoryAccess{
      .function_name = function_name,
      .block_label = entry_label,
      .inst_index = 11,
      .address = prepare::PreparedAddress{
          .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
          .pointer_value_name = base_ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .can_use_base_plus_offset = true,
      },
  });
  fixture.addressing.accesses.push_back(prepare::PreparedMemoryAccess{
      .function_name = function_name,
      .block_label = entry_label,
      .inst_index = 13,
      .address = prepare::PreparedAddress{
          .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
          .pointer_value_name = base_ptr,
          .size_bytes = 8,
          .align_bytes = 8,
          .can_use_base_plus_offset = true,
      },
  });

  const auto add_ptr_value = [&](prepare::PreparedValueId value_id,
                                 c4c::ValueNameId value_name) {
    fixture.regalloc.values.push_back(prepare::PreparedRegallocValue{
        .value_id = value_id,
        .function_name = function_name,
        .value_name = value_name,
        .type = bir::TypeKind::Ptr,
    });
  };
  add_ptr_value(1, symbol_copy);
  add_ptr_value(2, missing_copy);
  add_ptr_value(3, prepared_copy);
  add_ptr_value(4, adjacent_advanced);
  add_ptr_value(5, adjacent_after);
  add_ptr_value(6, local_previous);
  add_ptr_value(7, global_previous);

  (void)symbol_ptr;
  (void)missing_ptr;
  (void)local_recent;
  (void)global_recent;
  return fixture;
}

lir::LirModule make_lir_indirect_call_string_argument_module() {
  lir::LirModule module;
  module.target_profile = riscv_target_profile();
  module.string_pool.push_back(lir::LirStringConst{
      .pool_name = "@.L.str.lir.indirect.call",
      .raw_bytes = "lir-indirect-call-arg\\00",
      .byte_length = 22,
  });

  lir::LirFunction function;
  function.name = "stack_layout_lir_indirect_call_string_argument_activation";
  function.signature_text =
      "define void @stack_layout_lir_indirect_call_string_argument_activation()";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%string.ptr"),
      .element_type = lir::LirTypeRef("[22 x i8]"),
      .ptr = lir::LirOperand("@.L.str.lir.indirect.call"),
      .inbounds = true,
      .indices = {"i64 0", "i64 0"},
  });
  entry.insts.push_back(lir::LirCallOp{
      .return_type = lir::LirTypeRef("void"),
      .callee = lir::LirOperand("%callee.ptr"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %string.ptr",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

std::optional<prepare::PreparedBirModule> prepare_lir_indirect_call_string_argument_module() {
  auto lowered = c4c::backend::try_lower_to_bir(make_lir_indirect_call_string_argument_module());
  if (!lowered.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

std::optional<prepare::PreparedBirModule> prepare_lir_i16_local_writeback_module() {
  lir::LirModule module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);

  lir::LirFunction function;
  function.name = "stack_layout_lir_i16_local_writeback_activation";
  function.signature_text = "define void @stack_layout_lir_i16_local_writeback_activation()";
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.short"),
      .type_str = lir::LirTypeRef("i16"),
      .count = lir::LirOperand(""),
      .align = 2,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = lir::LirTypeRef("i16"),
      .val = lir::LirOperand("%incoming.short"),
      .ptr = lir::LirOperand("%lv.short"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%loaded.short"),
      .type_str = lir::LirTypeRef("i16"),
      .ptr = lir::LirOperand("%lv.short"),
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = lir::LirTypeRef("i16"),
      .val = lir::LirOperand("%loaded.short"),
      .ptr = lir::LirOperand("%lv.short"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%reloaded.short"),
      .type_str = lir::LirTypeRef("i16"),
      .ptr = lir::LirOperand("%lv.short"),
  });
  entry.terminator = lir::LirRet{std::nullopt, "void"};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = c4c::backend::try_lower_to_bir(module);
  if (!lowered.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered);
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_sliced_i16_local_address_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_sliced_i16_local_address_activation";
  function.return_type = bir::TypeKind::Void;
  const auto add_i16_slot = [&](std::string name) {
    function.local_slots.push_back(bir::LocalSlot{
        .name = std::move(name),
        .type = bir::TypeKind::I16,
        .size_bytes = 2,
        .align_bytes = 2,
        .is_address_taken = true,
    });
  };
  add_i16_slot("%slice.src.0");
  add_i16_slot("%slice.src.1");
  add_i16_slot("%slice.src.2");
  add_i16_slot("%slice.dst.0");
  add_i16_slot("%slice.dst.1");
  add_i16_slot("%slice.dst.2");

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id(module, "entry");
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I16, "%copy.plus2"),
      .slot_name = "%slice.src.0",
      .align_bytes = 2,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "%slice.src.0",
              .byte_offset = 2,
              .size_bytes = 2,
              .align_bytes = 2,
          },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%slice.dst.0",
      .value = bir::Value::named(bir::TypeKind::I16, "%copy.plus2"),
      .align_bytes = 2,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "%slice.dst.0",
              .byte_offset = 2,
              .size_bytes = 2,
              .align_bytes = 2,
          },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I16, "%copy.plus4"),
      .slot_name = "%slice.src.0",
      .align_bytes = 2,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "%slice.src.0",
              .byte_offset = 4,
              .size_bytes = 2,
              .align_bytes = 2,
          },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%slice.dst.0",
      .value = bir::Value::named(bir::TypeKind::I16, "%copy.plus4"),
      .align_bytes = 2,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "%slice.dst.0",
              .byte_offset = 4,
              .size_bytes = 2,
              .align_bytes = 2,
          },
  });
  entry.terminator = bir::ReturnTerminator{};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

int check_prepared_addressing_frame_fact_bootstrap(const prepare::PreparedBirModule& prepared) {
  const c4c::FunctionNameId function_name_id =
      find_function_name_id(prepared, "stack_layout_copy_coalescing_activation");
  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_name_id);
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing == nullptr) {
    return fail("expected stack layout to publish per-function prepared addressing frame facts");
  }
  if (function_addressing->frame_size_bytes != 4 ||
      function_addressing->frame_alignment_bytes != 4) {
    return fail("expected prepared addressing frame facts to mirror stack-layout metrics");
  }
  if (function_addressing->accesses.size() != 6) {
    return fail("expected prepared addressing to publish both direct frame-slot and symbol-backed accesses");
  }

  const auto* live_object = find_stack_object(prepared, "lv.live");
  if (live_object == nullptr) {
    return fail("expected the live local slot to produce a stack-layout object");
  }
  const auto* live_slot = find_frame_slot(prepared, live_object->object_id);
  if (live_slot == nullptr) {
    return fail("expected the live local slot to keep a canonical frame slot");
  }
  if (prepare::prepared_function_name(prepared.names, live_object->function_name) !=
          "stack_layout_copy_coalescing_activation" ||
      prepare::prepared_function_name(prepared.names, live_slot->function_name) !=
          "stack_layout_copy_coalescing_activation") {
    return fail("expected stack-layout objects and frame slots to carry semantic function ids");
  }

  const auto* store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 2);
  if (store_access == nullptr) {
    return fail("expected prepared addressing to record the direct frame-slot store");
  }
  if (store_access->result_value_name.has_value() ||
      !store_access->stored_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *store_access->stored_value_name) != "coalesced" ||
      store_access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !store_access->address.frame_slot_id.has_value() ||
      *store_access->address.frame_slot_id != live_slot->slot_id ||
      store_access->address.byte_offset != 0 ||
      store_access->address.size_bytes != 4 ||
      store_access->address.align_bytes != 4 ||
      !store_access->address.can_use_base_plus_offset ||
      store_access->address_space != bir::AddressSpace::Fs ||
      !store_access->is_volatile) {
    return fail("expected prepared addressing to preserve the direct live-slot store facts");
  }

  const auto* load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 3);
  if (load_access == nullptr) {
    return fail("expected prepared addressing to record the direct frame-slot load");
  }
  if (!load_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *load_access->result_value_name) != "loaded" ||
      load_access->stored_value_name.has_value() ||
      load_access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !load_access->address.frame_slot_id.has_value() ||
      *load_access->address.frame_slot_id != live_slot->slot_id ||
      load_access->address.byte_offset != 0 ||
      load_access->address.size_bytes != 4 ||
      load_access->address.align_bytes != 4 ||
      !load_access->address.can_use_base_plus_offset ||
      load_access->address_space != bir::AddressSpace::Gs ||
      load_access->is_volatile) {
    return fail("expected prepared addressing to preserve the direct live-slot load facts");
  }

  const auto* global_store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 4);
  if (global_store_access == nullptr) {
    return fail("expected prepared addressing to record the direct global-symbol store");
  }
  if (global_store_access->result_value_name.has_value() ||
      !global_store_access->stored_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *global_store_access->stored_value_name) != "loaded" ||
      global_store_access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !global_store_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *global_store_access->address.symbol_name) != "g.counter" ||
      global_store_access->address.byte_offset != 0 ||
      global_store_access->address.size_bytes != 4 ||
      global_store_access->address.align_bytes != 4 ||
      !global_store_access->address.can_use_base_plus_offset ||
      global_store_access->address_space != bir::AddressSpace::Tls ||
      !global_store_access->is_volatile) {
    return fail("expected prepared addressing to preserve the direct global-symbol store facts");
  }

  const auto* string_load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 5);
  if (string_load_access == nullptr) {
    return fail("expected prepared addressing to record the direct string-constant load");
  }
  if (!string_load_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *string_load_access->result_value_name) != "message.ptr" ||
      string_load_access->stored_value_name.has_value() ||
      string_load_access->address.base_kind != prepare::PreparedAddressBaseKind::StringConstant ||
      !string_load_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *string_load_access->address.symbol_name) != ".L.str0" ||
      string_load_access->address.byte_offset != 0 ||
      string_load_access->address.size_bytes != 8 ||
      string_load_access->address.align_bytes != 8 ||
      !string_load_access->address.can_use_base_plus_offset ||
      string_load_access->address_space != bir::AddressSpace::Fs ||
      !string_load_access->is_volatile) {
    return fail("expected prepared addressing to preserve the direct string-constant load facts");
  }

  const auto* local_symbol_store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 6);
  if (local_symbol_store_access == nullptr) {
    return fail("expected prepared addressing to record the local-inst global-symbol store");
  }
  if (local_symbol_store_access->result_value_name.has_value() ||
      !local_symbol_store_access->stored_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *local_symbol_store_access->stored_value_name) !=
          "loaded" ||
      local_symbol_store_access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !local_symbol_store_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *local_symbol_store_access->address.symbol_name) !=
          "g.counter" ||
      local_symbol_store_access->address.byte_offset != 0 ||
      local_symbol_store_access->address.size_bytes != 4 ||
      local_symbol_store_access->address.align_bytes != 4 ||
      !local_symbol_store_access->address.can_use_base_plus_offset ||
      local_symbol_store_access->address_space != bir::AddressSpace::Gs ||
      !local_symbol_store_access->is_volatile) {
    return fail("expected prepared addressing to preserve local-inst global-symbol store facts");
  }

  const auto* local_string_load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 7);
  if (local_string_load_access == nullptr) {
    return fail("expected prepared addressing to record the local-inst string-constant load");
  }
  if (!local_string_load_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *local_string_load_access->result_value_name) !=
          "local.message.ptr" ||
      local_string_load_access->stored_value_name.has_value() ||
      local_string_load_access->address.base_kind != prepare::PreparedAddressBaseKind::StringConstant ||
      !local_string_load_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *local_string_load_access->address.symbol_name) !=
          ".L.str0" ||
      local_string_load_access->address.byte_offset != 0 ||
      local_string_load_access->address.size_bytes != 8 ||
      local_string_load_access->address.align_bytes != 8 ||
      !local_string_load_access->address.can_use_base_plus_offset ||
      local_string_load_access->address_space != bir::AddressSpace::Tls ||
      local_string_load_access->is_volatile) {
    return fail("expected prepared addressing to preserve local-inst string-constant load facts");
  }

  if (prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 0) != nullptr ||
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 1) != nullptr) {
    return fail("expected coalesced scratch accesses to stay out of prepared frame-slot records");
  }
  if (function_addressing->address_materializations.size() != 2) {
    return fail("expected prepared addressing to publish string address materializations separately from accesses");
  }
  const auto* string_materialization =
      prepare::find_prepared_address_materialization(*function_addressing, entry_block_label_id, 5);
  if (string_materialization == nullptr) {
    return fail("expected prepared addressing to record the global-inst string address materialization");
  }
  if (string_materialization->kind != prepare::PreparedAddressMaterializationKind::StringConstant ||
      !string_materialization->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *string_materialization->result_value_name) !=
          "message.ptr" ||
      string_materialization->symbol_name.has_value() ||
      !string_materialization->text_name.has_value() ||
      prepared.names.texts.lookup(*string_materialization->text_name) != ".L.str0" ||
      string_materialization->byte_offset != 0 ||
      string_materialization->address_space != bir::AddressSpace::Fs ||
      string_materialization->is_thread_local ||
      string_materialization->has_tls_address_space) {
    return fail("expected string address materialization to preserve result, text, offset, and address-space facts");
  }
  const auto* local_string_materialization =
      prepare::find_prepared_address_materialization(*function_addressing, entry_block_label_id, 7);
  if (local_string_materialization == nullptr ||
      local_string_materialization->kind != prepare::PreparedAddressMaterializationKind::StringConstant ||
      !local_string_materialization->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *local_string_materialization->result_value_name) !=
          "local.message.ptr" ||
      local_string_materialization->address_space != bir::AddressSpace::Tls ||
      !local_string_materialization->has_tls_address_space) {
    return fail("expected local-inst string materialization to preserve TLS address-space facts");
  }
  if (prepare::find_prepared_addressing(
          prepared, find_function_name_id(prepared, "missing_function")) != nullptr) {
    return fail("expected prepared addressing frame-fact bootstrap to reject missing functions");
  }
  return 0;
}

int check_lir_i16_local_writeback_sizing(const prepare::PreparedBirModule& prepared) {
  const auto function_name_id =
      find_function_name_id(prepared, "stack_layout_lir_i16_local_writeback_activation");
  const auto* object = find_stack_object(prepared, "%lv.short");
  if (object == nullptr) {
    return fail("expected LIR i16 local to publish a prepared stack object");
  }
  if (object->type != bir::TypeKind::I16 || object->size_bytes != 2 ||
      object->align_bytes != 2) {
    return fail("expected LIR i16 local stack object to carry size=2 align=2");
  }

  const auto* slot = find_frame_slot(prepared, object->object_id);
  if (slot == nullptr) {
    return fail("expected LIR i16 local to receive a frame slot");
  }
  if (slot->size_bytes != 2 || slot->align_bytes != 2) {
    return fail("expected LIR i16 local frame slot to carry size=2 align=2");
  }
  if (prepared.stack_layout.frame_size_bytes < 2 ||
      prepared.stack_layout.frame_alignment_bytes < 2) {
    return fail("expected LIR i16 local to contribute nonzero frame metrics");
  }

  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_name_id);
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing == nullptr) {
    return fail("expected LIR i16 local to publish prepared addressing facts");
  }
  if (function_addressing->frame_size_bytes < 2 ||
      function_addressing->frame_alignment_bytes < 2) {
    return fail("expected LIR i16 prepared addressing to mirror nonzero frame metrics");
  }
  if (function_addressing->accesses.size() != 4) {
    return fail("expected LIR i16 local init/load/writeback/reload accesses");
  }

  const auto check_i16_access = [&](std::size_t inst_index, bool is_store) -> int {
    const auto* access =
        prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, inst_index);
    if (access == nullptr) {
      return fail("expected LIR i16 local access to be published");
    }
    if ((is_store && !access->stored_value_name.has_value()) ||
        (!is_store && !access->result_value_name.has_value())) {
      return fail("expected LIR i16 access direction metadata to be preserved");
    }
    if (access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
        !access->address.frame_slot_id.has_value() ||
        *access->address.frame_slot_id != slot->slot_id ||
        access->address.size_bytes != 2 ||
        access->address.align_bytes != 2 ||
        !access->address.can_use_base_plus_offset) {
      return fail("expected LIR i16 local access to use a direct size=2 align=2 frame slot");
    }
    return 0;
  };

  if (const int rc = check_i16_access(0, true); rc != 0) {
    return rc;
  }
  if (const int rc = check_i16_access(1, false); rc != 0) {
    return rc;
  }
  if (const int rc = check_i16_access(2, true); rc != 0) {
    return rc;
  }
  if (const int rc = check_i16_access(3, false); rc != 0) {
    return rc;
  }

  return 0;
}

int check_sliced_i16_local_address_coverage(const prepare::PreparedBirModule& prepared) {
  const auto function_name_id =
      find_function_name_id(prepared, "stack_layout_sliced_i16_local_address_activation");
  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_name_id);
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing == nullptr) {
    return fail("expected sliced i16 local fixture to publish prepared addressing");
  }

  const auto* src1_object = find_stack_object(prepared, "%slice.src.1");
  const auto* src2_object = find_stack_object(prepared, "%slice.src.2");
  const auto* dst1_object = find_stack_object(prepared, "%slice.dst.1");
  const auto* dst2_object = find_stack_object(prepared, "%slice.dst.2");
  if (src1_object == nullptr || src2_object == nullptr || dst1_object == nullptr ||
      dst2_object == nullptr) {
    return fail("expected sliced i16 local fixture to preserve consecutive slice objects");
  }
  const auto* src1_slot = find_frame_slot(prepared, src1_object->object_id);
  const auto* src2_slot = find_frame_slot(prepared, src2_object->object_id);
  const auto* dst1_slot = find_frame_slot(prepared, dst1_object->object_id);
  const auto* dst2_slot = find_frame_slot(prepared, dst2_object->object_id);
  if (src1_slot == nullptr || src2_slot == nullptr || dst1_slot == nullptr ||
      dst2_slot == nullptr) {
    return fail("expected sliced i16 local fixture to assign consecutive frame slots");
  }

  const auto check_access = [&](std::size_t inst_index,
                                prepare::PreparedFrameSlotId expected_slot_id,
                                bool is_store) -> int {
    const auto* access =
        prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, inst_index);
    if (access == nullptr) {
      return fail("expected sliced i16 local access to be published");
    }
    if ((is_store && !access->stored_value_name.has_value()) ||
        (!is_store && !access->result_value_name.has_value())) {
      return fail("expected sliced i16 local access direction metadata");
    }
    if (access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
        !access->address.frame_slot_id.has_value() ||
        *access->address.frame_slot_id != expected_slot_id ||
        access->address.byte_offset != 0 ||
        access->address.size_bytes != 2 ||
        access->address.align_bytes != 2 ||
        !access->address.can_use_base_plus_offset) {
      return fail(
          "expected base-slice + byte offset to resolve to the covering sliced frame slot");
    }
    return 0;
  };

  if (const int rc = check_access(0, src1_slot->slot_id, false); rc != 0) {
    return rc;
  }
  if (const int rc = check_access(1, dst1_slot->slot_id, true); rc != 0) {
    return rc;
  }
  if (const int rc = check_access(2, src2_slot->slot_id, false); rc != 0) {
    return rc;
  }
  if (const int rc = check_access(3, dst2_slot->slot_id, true); rc != 0) {
    return rc;
  }

  return 0;
}

int check_direct_call_string_argument_materialization(
    const prepare::PreparedBirModule& prepared) {
  const c4c::FunctionNameId function_name_id =
      find_function_name_id(prepared, "stack_layout_direct_call_string_argument_activation");
  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_name_id);
  if (function_addressing == nullptr) {
    return fail("expected direct-call string argument fixture to publish prepared addressing");
  }
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  const auto* materialization =
      prepare::find_prepared_address_materialization(*function_addressing,
                                                     entry_block_label_id,
                                                     0);
  if (materialization == nullptr) {
    return fail("expected direct call string argument to publish address materialization");
  }
  if (materialization->kind != prepare::PreparedAddressMaterializationKind::StringConstant ||
      !materialization->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *materialization->result_value_name) !=
          "@.L.str.call" ||
      !materialization->text_name.has_value() ||
      prepared.names.texts.lookup(*materialization->text_name) != ".L.str.call" ||
      materialization->byte_offset != 0 ||
      materialization->address_space != bir::AddressSpace::Default ||
      materialization->has_tls_address_space ||
      materialization->symbol_name.has_value()) {
    return fail("expected direct call string argument materialization to preserve string identity");
  }
  return 0;
}

int check_indirect_call_string_argument_materialization(
    const prepare::PreparedBirModule& prepared) {
  const c4c::FunctionNameId function_name_id =
      find_function_name_id(prepared, "stack_layout_indirect_call_string_argument_activation");
  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_name_id);
  if (function_addressing == nullptr) {
    return fail("expected indirect-call string argument fixture to publish prepared addressing");
  }
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  const auto* materialization =
      prepare::find_prepared_address_materialization(*function_addressing,
                                                     entry_block_label_id,
                                                     0);
  if (materialization == nullptr) {
    return fail("expected indirect call string argument to publish address materialization");
  }
  if (materialization->kind != prepare::PreparedAddressMaterializationKind::StringConstant ||
      !materialization->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *materialization->result_value_name) !=
          "@.L.str.indirect.call" ||
      !materialization->text_name.has_value() ||
      prepared.names.texts.lookup(*materialization->text_name) !=
          ".L.str.indirect.call" ||
      materialization->byte_offset != 0 ||
      materialization->address_space != bir::AddressSpace::Default ||
      materialization->has_tls_address_space ||
      materialization->symbol_name.has_value()) {
    return fail("expected indirect call string argument materialization to preserve string identity");
  }
  return 0;
}

int check_lir_indirect_call_string_argument_materialization(
    const prepare::PreparedBirModule& prepared) {
  const c4c::FunctionNameId function_name_id = find_function_name_id(
      prepared, "stack_layout_lir_indirect_call_string_argument_activation");
  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_name_id);
  if (function_addressing == nullptr) {
    return fail("expected LIR-origin indirect-call string argument fixture to publish prepared addressing");
  }

  bool saw_lir_string_argument_materialization = false;
  for (const auto& materialization : function_addressing->address_materializations) {
    if (materialization.kind != prepare::PreparedAddressMaterializationKind::StringConstant ||
        !materialization.result_value_name.has_value() ||
        prepare::prepared_value_name(prepared.names, *materialization.result_value_name) !=
            "@.L.str.lir.indirect.call" ||
        !materialization.text_name.has_value() ||
        prepared.names.texts.lookup(*materialization.text_name) !=
            ".L.str.lir.indirect.call" ||
        materialization.byte_offset != 0 ||
        materialization.address_space != bir::AddressSpace::Default ||
        materialization.has_tls_address_space ||
        materialization.symbol_name.has_value()) {
      continue;
    }
    saw_lir_string_argument_materialization = true;
    break;
  }
  if (!saw_lir_string_argument_materialization) {
    return fail(
        "expected LIR-to-BIR indirect call string argument rewrite to publish prepared string materialization");
  }
  return 0;
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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_fixed_location_gap_fill_frame_slot_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_fixed_location_gap_fill_frame_slot_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed.gap.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed.gap.wide",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed.gap.fill",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed.gap.wide",
      .value = bir::Value::immediate_i64(11),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed.gap.fill",
      .value = bir::Value::immediate_i32(5),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed.gap.wide",
      .value = bir::Value::immediate_i64(13),
      .align_bytes = 8,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "lv.fixed.gap.root",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.fixed.gap.fill.loaded"),
      .slot_name = "lv.fixed.gap.fill",
      .align_bytes = 4,
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "lv.fixed.gap.fill.loaded"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_call_escaped_scalarized_local_family_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_call_escaped_scalarized_local_family_activation";
  function.return_type = bir::TypeKind::I32;
  for (std::size_t index = 0; index < 6; ++index) {
    function.local_slots.push_back(bir::LocalSlot{
        .name = "lv.scalar.call." + std::to_string(index),
        .type = bir::TypeKind::I8,
        .size_bytes = 1,
        .align_bytes = 1,
    });
  }

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "sink_ptr",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "lv.scalar.call.0")},
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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
          .address_space = bir::AddressSpace::Gs,
          .is_volatile = true,
      },
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

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
          .address_space = bir::AddressSpace::Fs,
          .is_volatile = true,
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
          .address_space = bir::AddressSpace::Tls,
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "lv.global.ptr.addr.loaded"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_id_authoritative_stack_access_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_id_authoritative_block_access_activation";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.id.access",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.label = "raw.entry.block";
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "lv.id.loaded"),
      .slot_name = "lv.id.access",
      .align_bytes = 4,
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "lv.id.loaded"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_link_name_authoritative_global_access_module() {
  bir::Module module;

  const c4c::LinkNameId canonical_global_id =
      module.names.link_names.intern("g.authoritative");
  module.globals.push_back(bir::Global{
      .name = "g.authoritative",
      .link_name_id = canonical_global_id,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.globals.push_back(bir::Global{
      .name = "g.raw.drift",
      .link_name_id = module.names.link_names.intern("g.raw.drift"),
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.globals.push_back(bir::Global{
      .name = "g.compat",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  const c4c::LinkNameId got_global_id = module.names.link_names.intern("g.got");
  module.globals.push_back(bir::Global{
      .name = "g.got",
      .link_name_id = got_global_id,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::GotRequired,
  });
  const c4c::LinkNameId tls_global_id = module.names.link_names.intern("g.tls");
  module.globals.push_back(bir::Global{
      .name = "g.tls",
      .link_name_id = tls_global_id,
      .type = bir::TypeKind::I32,
      .is_thread_local = true,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  const c4c::LinkNameId indirect_target_id = module.names.link_names.intern("callee.fn");

  bir::Function function;
  function.name = "stack_layout_link_name_authoritative_global_access_activation";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id(module, "entry");
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "id.loaded"),
      .global_name_id = canonical_global_id,
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "g.raw.drift",
      .global_name_id = canonical_global_id,
      .value = bir::Value::named(bir::TypeKind::I32, "id.loaded"),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "g.compat",
      .value = bir::Value::named(bir::TypeKind::I32, "id.loaded"),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = bir::Value::named_symbol_pointer("@g.authoritative", canonical_global_id),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "unused.global.source"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = bir::Value::named_symbol_pointer("@g.tls", tls_global_id),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "unused.tls.source"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = bir::Value::named_symbol_pointer("@g.got", got_global_id),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "unused.got.source"),
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "label.ptr"),
      .byte_offset = 12,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::Label,
              .base_name = "target",
              .byte_offset = 4,
              .address_space = bir::AddressSpace::Fs,
              .base_label_id = block_label_id(module, "target"),
          },
  });
  entry.insts.push_back(bir::CallInst{
      .callee_value = bir::Value::named_symbol_pointer("@callee.fn", indirect_target_id),
      .return_type = bir::TypeKind::Void,
      .calling_convention = bir::CallingConv::C,
      .is_indirect = true,
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "id.loaded"),
  };

  function.blocks.push_back(std::move(entry));
  bir::Block target;
  target.label = "target";
  target.label_id = block_label_id(module, "target");
  target.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(target));
  module.functions.push_back(std::move(function));

  bir::Function indirect_target;
  indirect_target.name = "callee.fn";
  indirect_target.link_name_id = indirect_target_id;
  indirect_target.return_type = bir::TypeKind::Void;
  indirect_target.blocks.push_back(bir::Block{
      .label = "callee.entry",
      .terminator = bir::ReturnTerminator{},
      .label_id = block_label_id(module, "callee.entry"),
  });
  module.functions.push_back(std::move(indirect_target));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_pic_unspecified_global_policy_module() {
  bir::Module module;

  const c4c::LinkNameId external_global_id = module.names.link_names.intern("g.external");
  module.globals.push_back(bir::Global{
      .name = "g.external",
      .link_name_id = external_global_id,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Function function;
  function.name = "stack_layout_pic_unspecified_global_policy_activation";
  function.return_type = bir::TypeKind::Ptr;

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id(module, "entry");
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = bir::Value::named_symbol_pointer("@g.external", external_global_id),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "unused.external.source"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::Ptr, "@g.external"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = aarch64_pic_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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

int check_fixed_location_gap_fill_frame_slot_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.fixed.gap.root");
  const auto* wide_object = find_stack_object(prepared, "lv.fixed.gap.wide");
  const auto* fill_object = find_stack_object(prepared, "lv.fixed.gap.fill");
  if (root_object == nullptr || wide_object == nullptr || fill_object == nullptr) {
    return fail("expected fixed-location gap-fill activation to produce fixed, wide, and fill stack-layout objects");
  }

  const auto* root_slot = find_frame_slot(prepared, root_object->object_id);
  const auto* wide_slot = find_frame_slot(prepared, wide_object->object_id);
  const auto* fill_slot = find_frame_slot(prepared, fill_object->object_id);
  if (root_slot == nullptr || wide_slot == nullptr || fill_slot == nullptr) {
    return fail("expected fixed-location gap-fill activation to assign frame-slot storage to all objects");
  }

  if (!root_slot->fixed_location) {
    return fail("expected the addressed root local slot to remain in the fixed-location tier");
  }
  if (wide_slot->fixed_location || fill_slot->fixed_location) {
    return fail("expected the non-address-exposed comparison slots to remain reorderable");
  }
  if (root_slot->offset_bytes != 0 || fill_slot->offset_bytes != 4 ||
      wide_slot->offset_bytes != 8) {
    return fail("expected the reorderable fill slot to consume the fixed-tier alignment gap before the wider slot");
  }
  if (prepared.stack_layout.frame_size_bytes != 16 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected fixed-location gap-fill activation to shrink the frame to the hole-filled layout");
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

int check_unused_param_frame_slot_divergence_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* byval_object = find_stack_object(prepared, "p.byval.root");
  const auto* sret_object = find_stack_object(prepared, "p.sret.root");
  if (byval_object == nullptr || sret_object == nullptr) {
    return fail(
        "expected unused byval and sret params to stay in the active prepared "
        "stack-layout path until C++ exposes the Rust dead-param-alloca "
        "contract");
  }

  const auto* byval_slot = find_frame_slot(prepared, byval_object->object_id);
  const auto* sret_slot = find_frame_slot(prepared, sret_object->object_id);
  if (byval_slot == nullptr || sret_slot == nullptr) {
    return fail(
        "expected unused byval and sret params to keep fixed-tier frame slots "
        "until C++ BIR carries the Rust dead-param-alloca inputs");
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
    const prepare::PreparedNameTables& names,
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* scratch_object = find_stack_object(names, objects, "lv.analysis.scratch");
  const auto* copy_object = find_stack_object(names, objects, "lv.analysis.byval_copy");
  const auto* phi_object = find_stack_object(names, objects, "lv.analysis.phi");
  const auto* addressed_object = find_stack_object(names, objects, "lv.analysis.addr");
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
    const prepare::PreparedNameTables& names,
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* byval_object = find_stack_object(names, objects, "p.analysis.byval");
  const auto* sret_object = find_stack_object(names, objects, "p.analysis.sret");
  const auto* plain_object = find_stack_object(names, objects, "p.analysis.plain");
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

int check_stack_layout_unused_param_divergence_activation(
    const prepare::PreparedNameTables& names,
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* byval_object = find_stack_object(names, objects, "p.analysis.byval");
  const auto* sret_object = find_stack_object(names, objects, "p.analysis.sret");
  if (byval_object == nullptr || sret_object == nullptr) {
    return fail(
        "expected unused byval and sret params to stay materialized until the "
        "active C++ stack-layout contract grows the Rust dead-param-alloca "
        "inputs");
  }

  return 0;
}

int check_stack_layout_regalloc_hint_activation(
    const prepare::PreparedNameTables& names,
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* scratch_object = find_stack_object(names, objects, "lv.analysis.scratch");
  const auto* copy_object = find_stack_object(names, objects, "lv.analysis.byval_copy");
  const auto* phi_object = find_stack_object(names, objects, "lv.analysis.phi");
  const auto* addressed_object = find_stack_object(names, objects, "lv.analysis.addr");
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
    const prepare::PreparedNameTables& names,
    const std::vector<prepare::PreparedStackObject>& objects) {
  const auto* byval_object = find_stack_object(names, objects, "p.analysis.byval");
  const auto* sret_object = find_stack_object(names, objects, "p.analysis.sret");
  const auto* plain_object = find_stack_object(names, objects, "p.analysis.plain");
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

int check_call_escaped_scalarized_local_family_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* root_object = find_stack_object(prepared, "lv.scalar.call.0");
  if (root_object == nullptr) {
    return fail("expected the scalarized call-escaped root slot to produce a stack-layout object");
  }
  if (!root_object->address_exposed || !root_object->requires_home_slot ||
      !root_object->permanent_home_slot) {
    return fail("expected the scalarized root call arg to anchor a fixed home-slot family");
  }

  for (std::size_t index = 0; index < 6; ++index) {
    const std::string name = "lv.scalar.call." + std::to_string(index);
    const auto* object = find_stack_object(prepared, name);
    if (object == nullptr) {
      return fail("expected every scalarized call-escaped family member to remain visible");
    }
    const auto* slot = find_frame_slot(prepared, object->object_id);
    if (slot == nullptr) {
      return fail("expected every scalarized call-escaped family member to receive a frame slot");
    }
    if (!slot->fixed_location || slot->offset_bytes != index || slot->size_bytes != 1 ||
        slot->align_bytes != 1) {
      return fail("expected scalarized call-escaped family slots to reserve a contiguous frame extent");
    }
  }

  if (prepared.stack_layout.frame_slots.size() != 6 ||
      prepared.stack_layout.frame_size_bytes != 6 ||
      prepared.stack_layout.frame_alignment_bytes != 1) {
    return fail("expected scalarized call-escaped family metrics to cover the full byte extent");
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

  const auto* function_addressing =
      prepare::find_prepared_addressing(
          prepared,
          find_function_name_id(prepared, "stack_layout_pointer_addressed_local_slot_activation"));
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing == nullptr) {
    return fail("expected pointer-addressed locals to publish prepared addressing records");
  }
  if (function_addressing->accesses.size() != 1) {
    return fail("expected exactly one pointer-indirect access record for the local pointer fixture");
  }

  const auto* pointer_store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 1);
  if (pointer_store_access == nullptr) {
    return fail("expected prepared addressing to record the pointer-indirect local store");
  }
  if (pointer_store_access->result_value_name.has_value() ||
      pointer_store_access->stored_value_name.has_value() ||
      pointer_store_access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !pointer_store_access->address.pointer_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *pointer_store_access->address.pointer_value_name) !=
          "lv.ptr.addr.alias" ||
      pointer_store_access->address.symbol_name.has_value() ||
      pointer_store_access->address.frame_slot_id.has_value() ||
      pointer_store_access->address.byte_offset != 0 ||
      pointer_store_access->address.size_bytes != 4 ||
      pointer_store_access->address.align_bytes != 4 ||
      !pointer_store_access->address.can_use_base_plus_offset ||
      pointer_store_access->address_space != bir::AddressSpace::Gs ||
      !pointer_store_access->is_volatile) {
    return fail("expected prepared addressing to preserve the pointer-indirect local store facts");
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

  const auto* function_addressing = prepare::find_prepared_addressing(
      prepared, find_function_name_id(prepared, "stack_layout_global_pointer_addressed_local_slot_activation"));
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing == nullptr) {
    return fail("expected global pointer-addressed accesses to publish prepared addressing records");
  }
  if (function_addressing->accesses.size() != 2) {
    return fail("expected both pointer-indirect global load/store records for the global pointer fixture");
  }

  const auto* pointer_store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 1);
  if (pointer_store_access == nullptr) {
    return fail("expected prepared addressing to record the pointer-indirect global store");
  }
  if (pointer_store_access->result_value_name.has_value() ||
      pointer_store_access->stored_value_name.has_value() ||
      pointer_store_access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !pointer_store_access->address.pointer_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *pointer_store_access->address.pointer_value_name) !=
          "lv.global.ptr.addr.alias" ||
      pointer_store_access->address.symbol_name.has_value() ||
      pointer_store_access->address.frame_slot_id.has_value() ||
      pointer_store_access->address.byte_offset != 0 ||
      pointer_store_access->address.size_bytes != 4 ||
      pointer_store_access->address.align_bytes != 4 ||
      !pointer_store_access->address.can_use_base_plus_offset ||
      pointer_store_access->address_space != bir::AddressSpace::Fs ||
      !pointer_store_access->is_volatile) {
    return fail("expected prepared addressing to preserve the pointer-indirect global store facts");
  }

  const auto* pointer_load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 2);
  if (pointer_load_access == nullptr) {
    return fail("expected prepared addressing to record the pointer-indirect global load");
  }
  if (!pointer_load_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *pointer_load_access->result_value_name) !=
          "lv.global.ptr.addr.loaded" ||
      pointer_load_access->stored_value_name.has_value() ||
      pointer_load_access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !pointer_load_access->address.pointer_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *pointer_load_access->address.pointer_value_name) !=
          "lv.global.ptr.addr.alias" ||
      pointer_load_access->address.symbol_name.has_value() ||
      pointer_load_access->address.frame_slot_id.has_value() ||
      pointer_load_access->address.byte_offset != 0 ||
      pointer_load_access->address.size_bytes != 4 ||
      pointer_load_access->address.align_bytes != 4 ||
      !pointer_load_access->address.can_use_base_plus_offset ||
      pointer_load_access->address_space != bir::AddressSpace::Tls ||
      pointer_load_access->is_volatile) {
    return fail("expected prepared addressing to preserve the pointer-indirect global load facts");
  }

  return 0;
}

int check_pointer_carrier_contract_value_homes() {
  auto fixture = make_pointer_carrier_contract_fixture();
  const auto homes = prepare::regalloc_detail::build_prepared_value_homes(
      fixture.names,
      riscv_target_profile(),
      fixture.module,
      &fixture.function,
      nullptr,
      &fixture.addressing,
      fixture.regalloc);

  const auto* symbol_copy =
      find_value_home_by_name(fixture.names, homes, "contract.symbol.copy.ptr");
  if (symbol_copy == nullptr) {
    return fail("expected a value home for the symbol-carrier local-slot copy");
  }
  if (symbol_copy->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !symbol_copy->pointer_base_value_name.has_value() ||
      prepare::prepared_value_name(fixture.names, *symbol_copy->pointer_base_value_name) !=
          "contract.symbol.ptr" ||
      !symbol_copy->pointer_base_symbol_name.has_value() ||
      prepare::prepared_link_name(fixture.names, *symbol_copy->pointer_base_symbol_name) !=
          "contract.symbol" ||
      symbol_copy->pointer_byte_delta != std::optional<std::int64_t>{0}) {
    return fail("expected valid BIR link-name authority to publish a symbol-backed carrier");
  }

  const auto* missing_copy =
      find_value_home_by_name(fixture.names, homes, "contract.missing.copy.ptr");
  if (missing_copy == nullptr) {
    return fail("expected a value home for the missing-symbol local-slot copy");
  }
  if (missing_copy->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      missing_copy->pointer_base_symbol_name.has_value() ||
      missing_copy->pointer_byte_delta.has_value()) {
    return fail("expected missing pointer-symbol metadata to fail closed");
  }

  const auto* prepared_copy =
      find_value_home_by_name(fixture.names, homes, "contract.prepared.copy.ptr");
  if (prepared_copy == nullptr) {
    return fail("expected a value home for the prepared-pointer local-slot copy");
  }
  if (prepared_copy->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !prepared_copy->pointer_base_value_name.has_value() ||
      prepare::prepared_value_name(fixture.names, *prepared_copy->pointer_base_value_name) !=
          "contract.base.ptr" ||
      prepared_copy->pointer_base_symbol_name.has_value() ||
      prepared_copy->pointer_byte_delta != std::optional<std::int64_t>{0}) {
    return fail("expected prepared pointer-value authority to survive local-slot storage unchanged");
  }

  const auto* adjacent_advanced =
      find_value_home_by_name(fixture.names, homes, "contract.adjacent.advanced.ptr");
  if (adjacent_advanced == nullptr) {
    return fail("expected a value home for the raw same-slot store candidate");
  }
  if (adjacent_advanced->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      adjacent_advanced->pointer_base_value_name.has_value() ||
      adjacent_advanced->pointer_byte_delta.has_value()) {
    return fail("expected raw same-slot load/store adjacency not to mint a pointer carrier");
  }

  const auto* adjacent_after =
      find_value_home_by_name(fixture.names, homes, "contract.adjacent.after.ptr");
  if (adjacent_after == nullptr) {
    return fail("expected a value home for the post-adjacency local-slot copy");
  }
  if (adjacent_after->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !adjacent_after->pointer_base_value_name.has_value() ||
      prepare::prepared_value_name(fixture.names, *adjacent_after->pointer_base_value_name) !=
          "contract.base.ptr" ||
      adjacent_after->pointer_byte_delta != std::optional<std::int64_t>{0}) {
    return fail("expected unauthorized raw store not to replace the authorized slot carrier");
  }

  const auto* local_previous =
      find_value_home_by_name(fixture.names, homes, "contract.local.previous.ptr");
  const auto* global_previous =
      find_value_home_by_name(fixture.names, homes, "contract.global.previous.ptr");
  if (local_previous == nullptr || global_previous == nullptr) {
    return fail("expected value homes for addressed predecessor candidates");
  }
  if (local_previous->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      local_previous->pointer_base_value_name.has_value() ||
      local_previous->pointer_byte_delta.has_value()) {
    return fail("expected addressed local store predecessor inference to fail closed");
  }
  if (global_previous->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      global_previous->pointer_base_value_name.has_value() ||
      global_previous->pointer_byte_delta.has_value()) {
    return fail("expected addressed global store predecessor inference to fail closed");
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

int check_id_authoritative_stack_access_activation(const prepare::PreparedBirModule& prepared) {
  const auto* object = find_stack_object(prepared, "lv.id.access");
  if (object == nullptr) {
    return fail("expected id-authoritative stack access slot to produce an object");
  }

  const auto* function_addressing = prepare::find_prepared_addressing(
      prepared,
      find_function_name_id(prepared, "stack_layout_id_authoritative_block_access_activation"));
  if (function_addressing == nullptr) {
    return fail("expected id-authoritative stack access fixture to publish addressing");
  }
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (entry_block_label_id == c4c::kInvalidBlockLabel) {
    return fail("expected id-authoritative stack access block label to use structured spelling");
  }
  const auto* access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 0);
  if (access == nullptr) {
    return fail("expected prepared addressing to use BlockLabelId for stale raw block spelling");
  }
  if (!access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *access->result_value_name) != "lv.id.loaded") {
    return fail("expected id-authoritative stack access to preserve load result metadata");
  }

  return 0;
}

int check_link_name_authoritative_global_access_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* function_addressing = prepare::find_prepared_addressing(
      prepared,
      find_function_name_id(
          prepared, "stack_layout_link_name_authoritative_global_access_activation"));
  if (function_addressing == nullptr) {
    return fail("expected link-name authoritative global fixture to publish addressing");
  }
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing->accesses.size() != 2) {
    return fail("expected mismatched LinkNameId/raw global spelling to fail closed");
  }

  const auto* id_load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 0);
  if (id_load_access == nullptr) {
    return fail("expected prepared addressing to record ID-only global load");
  }
  if (!id_load_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *id_load_access->result_value_name) !=
          "id.loaded" ||
      id_load_access->stored_value_name.has_value() ||
      id_load_access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !id_load_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *id_load_access->address.symbol_name) !=
          "g.authoritative" ||
      id_load_access->address.byte_offset != 0 || id_load_access->address.size_bytes != 4 ||
      id_load_access->address.align_bytes != 4 ||
      !id_load_access->address.can_use_base_plus_offset) {
    return fail("expected ID-only global load to resolve through LinkNameId");
  }

  if (prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 1) !=
      nullptr) {
    return fail("expected raw global spelling drift to stay out of prepared addressing");
  }

  const auto* compat_store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 2);
  if (compat_store_access == nullptr) {
    return fail("expected raw-only compatibility global store to remain supported");
  }
  if (compat_store_access->result_value_name.has_value() ||
      !compat_store_access->stored_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *compat_store_access->stored_value_name) !=
          "id.loaded" ||
      compat_store_access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !compat_store_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *compat_store_access->address.symbol_name) !=
          "g.compat") {
    return fail("expected raw-only compatibility global store to resolve by spelling");
  }

  if (function_addressing->address_materializations.size() != 5) {
    return fail(
        "expected link-name fixture to publish global, GOT, label, and indirect-callee address materializations");
  }
  const auto* direct_global =
      prepare::find_prepared_address_materialization(*function_addressing, entry_block_label_id, 3);
  if (direct_global == nullptr) {
    return fail("expected prepared addressing to record direct global address materialization");
  }
  if (direct_global->kind != prepare::PreparedAddressMaterializationKind::DirectGlobal ||
      !direct_global->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *direct_global->result_value_name) !=
          "@g.authoritative" ||
      !direct_global->symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *direct_global->symbol_name) !=
          "g.authoritative" ||
      direct_global->text_name.has_value() ||
      direct_global->byte_offset != 0 ||
      direct_global->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      direct_global->address_space != bir::AddressSpace::Default ||
      direct_global->is_thread_local ||
      direct_global->has_tls_address_space) {
    return fail("expected direct global materialization to preserve result and symbol identity");
  }
  const auto* tls_global =
      prepare::find_prepared_address_materialization(*function_addressing, entry_block_label_id, 4);
  if (tls_global == nullptr) {
    return fail("expected prepared addressing to record TLS global address materialization");
  }
  if (tls_global->kind != prepare::PreparedAddressMaterializationKind::TlsGlobal ||
      !tls_global->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *tls_global->result_value_name) != "@g.tls" ||
      !tls_global->symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *tls_global->symbol_name) != "g.tls" ||
      tls_global->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      tls_global->address_space != bir::AddressSpace::Tls ||
      !tls_global->is_thread_local ||
      !tls_global->has_tls_address_space ||
      tls_global->tls_model !=
          prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
      tls_global->tls_thread_pointer_register !=
          prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0 ||
      tls_global->tls_high_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
      tls_global->tls_low_relocation !=
          prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc) {
    return fail("expected TLS global materialization to preserve structured TLS facts");
  }
  const auto* got_global =
      prepare::find_prepared_address_materialization(*function_addressing, entry_block_label_id, 5);
  if (got_global == nullptr) {
    return fail("expected prepared addressing to record explicit GOT global address materialization");
  }
  if (got_global->kind != prepare::PreparedAddressMaterializationKind::GotGlobal ||
      !got_global->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *got_global->result_value_name) != "@g.got" ||
      !got_global->symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *got_global->symbol_name) != "g.got" ||
      got_global->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired ||
      got_global->address_space != bir::AddressSpace::Default ||
      got_global->is_thread_local ||
      got_global->has_tls_address_space) {
    return fail("expected GOT materialization to preserve explicit policy and symbol facts");
  }
  const auto* label_address =
      prepare::find_prepared_address_materialization(*function_addressing, entry_block_label_id, 6);
  if (label_address == nullptr) {
    return fail("expected prepared addressing to record label address materialization");
  }
  if (label_address->kind != prepare::PreparedAddressMaterializationKind::Label ||
      !label_address->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *label_address->result_value_name) !=
          "label.ptr" ||
      label_address->symbol_name.has_value() ||
      label_address->text_name.has_value() ||
      !label_address->target_label.has_value() ||
      prepare::prepared_block_label(prepared.names, *label_address->target_label) != "target" ||
      label_address->byte_offset != 16 ||
      label_address->address_space != bir::AddressSpace::Fs ||
      label_address->is_thread_local ||
      label_address->has_tls_address_space) {
    return fail("expected label materialization to preserve target label and address facts");
  }
  const auto* indirect_callee =
      prepare::find_prepared_address_materialization(*function_addressing, entry_block_label_id, 7);
  if (indirect_callee == nullptr) {
    return fail("expected prepared addressing to record indirect callee address materialization");
  }
  if (indirect_callee->kind != prepare::PreparedAddressMaterializationKind::DirectGlobal ||
      !indirect_callee->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *indirect_callee->result_value_name) !=
          "@callee.fn" ||
      !indirect_callee->symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *indirect_callee->symbol_name) !=
          "callee.fn" ||
      indirect_callee->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      indirect_callee->address_space != bir::AddressSpace::Default ||
      indirect_callee->is_thread_local ||
      indirect_callee->has_tls_address_space) {
    return fail("expected indirect callee materialization to preserve function symbol identity");
  }

  return 0;
}

int check_pic_unspecified_global_policy_activation(const prepare::PreparedBirModule& prepared) {
  if (prepared.target_profile.relocation_model != c4c::TargetRelocationModel::Pic) {
    return fail("expected PIC fixture to preserve target relocation mode");
  }
  const auto* function_addressing = prepare::find_prepared_addressing(
      prepared,
      find_function_name_id(
          prepared, "stack_layout_pic_unspecified_global_policy_activation"));
  if (function_addressing == nullptr) {
    return fail("expected PIC unspecified-policy fixture to publish addressing function");
  }
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (prepare::find_prepared_address_materialization(
          *function_addressing, entry_block_label_id, 0) != nullptr) {
    return fail("expected PIC unresolved global policy to stay out of prepared materializations");
  }
  for (const auto& note : prepared.notes) {
    if (note.message.find("needs explicit GOT/direct policy for relocation model pic") !=
        std::string::npos) {
      return 0;
    }
  }
  return fail("expected PIC unresolved global policy to diagnose instead of defaulting");
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

int check_prepared_addressing_contract_activation() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("main");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto t0_name = prepared.names.value_names.intern("%t0");
  const auto t1_name = prepared.names.value_names.intern("%t1");
  const auto str_name = prepared.names.link_names.intern(".L.str0");
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 1,
                  .result_value_name = t0_name,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                          .frame_slot_id = 4,
                          .byte_offset = 8,
                          .size_bytes = 4,
                          .align_bytes = 4,
                          .can_use_base_plus_offset = true,
                      },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 2,
                  .stored_value_name = t1_name,
                  .address_space = bir::AddressSpace::Gs,
                  .is_volatile = true,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::StringConstant,
                          .symbol_name = str_name,
                          .size_bytes = 8,
                          .align_bytes = 8,
                      },
              },
          },
  });

  const auto* function_addressing =
      prepare::find_prepared_addressing(prepared, find_function_name_id(prepared, "main"));
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing == nullptr) {
    return fail("expected prepared addressing lookup to find the function contract");
  }
  if (function_addressing->frame_size_bytes != 32 ||
      function_addressing->frame_alignment_bytes != 16) {
    return fail("expected prepared addressing to preserve frame size and alignment facts");
  }

  const auto* frame_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 1);
  if (frame_access == nullptr) {
    return fail("expected prepared addressing to find the frame-slot access by block and instruction");
  }
  if (!frame_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *frame_access->result_value_name) != "%t0" ||
      frame_access->stored_value_name.has_value() ||
      frame_access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !frame_access->address.frame_slot_id.has_value() || *frame_access->address.frame_slot_id != 4 ||
      frame_access->address.byte_offset != 8 || frame_access->address.size_bytes != 4 ||
      frame_access->address.align_bytes != 4 ||
      !frame_access->address.can_use_base_plus_offset ||
      frame_access->address_space != bir::AddressSpace::Default ||
      frame_access->is_volatile) {
    return fail("expected prepared addressing to preserve direct frame-slot access facts");
  }

  const auto* symbol_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 2);
  if (symbol_access == nullptr) {
    return fail("expected prepared addressing to find the symbol-backed access by block and instruction");
  }
  if (symbol_access->result_value_name.has_value() || !symbol_access->stored_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *symbol_access->stored_value_name) != "%t1" ||
      symbol_access->address.base_kind != prepare::PreparedAddressBaseKind::StringConstant ||
      !symbol_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *symbol_access->address.symbol_name) != ".L.str0" ||
      symbol_access->address.frame_slot_id.has_value() ||
      symbol_access->address.pointer_value_name.has_value() ||
      symbol_access->address_space != bir::AddressSpace::Gs ||
      !symbol_access->is_volatile) {
    return fail("expected prepared addressing to preserve symbol-backed access facts");
  }

  if (prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 99) != nullptr) {
    return fail("expected prepared addressing lookup to reject missing instruction records");
  }
  if (prepare::find_prepared_addressing(prepared, find_function_name_id(prepared, "helper")) != nullptr) {
    return fail("expected prepared addressing lookup to reject missing function records");
  }
  if (prepare::prepared_address_base_kind_name(prepare::PreparedAddressBaseKind::PointerValue) !=
      "pointer_value") {
    return fail("expected prepared addressing base-kind names to stay stable");
  }

  return 0;
}

}  // namespace

int main() {
  const auto [analysis_names, analysis_objects] = collect_stack_layout_analysis_objects_with_names();
  if (const int rc =
          check_stack_layout_analysis_object_collection_activation(analysis_names, analysis_objects);
      rc != 0) {
    return rc;
  }

  const auto [param_analysis_names, param_analysis_objects] =
      collect_stack_layout_param_objects_with_names();
  if (const int rc =
          check_stack_layout_param_object_collection_activation(param_analysis_names,
                                                               param_analysis_objects);
      rc != 0) {
    return rc;
  }
  if (const int rc =
          check_stack_layout_unused_param_divergence_activation(param_analysis_names,
                                                               param_analysis_objects);
      rc != 0) {
    return rc;
  }

  const auto [regalloc_hint_names, regalloc_hint_objects] = collect_stack_layout_regalloc_hint_objects();
  if (const int rc =
          check_stack_layout_regalloc_hint_activation(regalloc_hint_names, regalloc_hint_objects);
      rc != 0) {
    return rc;
  }

  const auto [param_regalloc_hint_names, param_regalloc_hint_objects] =
      collect_stack_layout_param_regalloc_hint_objects();
  if (const int rc =
          check_stack_layout_param_regalloc_hint_activation(param_regalloc_hint_names,
                                                           param_regalloc_hint_objects);
      rc != 0) {
    return rc;
  }

  const auto copy_prepared = prepare_stack_layout_module();
  if (const int rc = check_stack_layout_activation(copy_prepared); rc != 0) {
    return rc;
  }
  if (const int rc = check_prepared_addressing_frame_fact_bootstrap(copy_prepared); rc != 0) {
    return rc;
  }
  const auto call_string_arg_prepared = prepare_direct_call_string_argument_module();
  if (const int rc = check_direct_call_string_argument_materialization(call_string_arg_prepared);
      rc != 0) {
    return rc;
  }
  const auto indirect_call_string_arg_prepared = prepare_indirect_call_string_argument_module();
  if (const int rc =
          check_indirect_call_string_argument_materialization(indirect_call_string_arg_prepared);
      rc != 0) {
    return rc;
  }
  auto lir_indirect_call_string_arg_prepared =
      prepare_lir_indirect_call_string_argument_module();
  if (!lir_indirect_call_string_arg_prepared.has_value()) {
    return fail("expected LIR-origin indirect-call string argument fixture to lower to BIR");
  }
  if (const int rc = check_lir_indirect_call_string_argument_materialization(
          *lir_indirect_call_string_arg_prepared);
      rc != 0) {
    return rc;
  }
  auto lir_i16_local_prepared = prepare_lir_i16_local_writeback_module();
  if (!lir_i16_local_prepared.has_value()) {
    return fail("expected LIR-origin i16 local fixture to lower to BIR");
  }
  if (const int rc = check_lir_i16_local_writeback_sizing(*lir_i16_local_prepared);
      rc != 0) {
    return rc;
  }
  const auto sliced_i16_local_prepared = prepare_sliced_i16_local_address_module();
  if (const int rc = check_sliced_i16_local_address_coverage(sliced_i16_local_prepared);
      rc != 0) {
    return rc;
  }
  if (const int rc = check_prepared_addressing_contract_activation(); rc != 0) {
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

  const auto fixed_location_gap_fill_prepared = prepare_fixed_location_gap_fill_frame_slot_module();
  if (const int rc =
          check_fixed_location_gap_fill_frame_slot_activation(
              fixed_location_gap_fill_prepared);
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
  if (const int rc =
          check_unused_param_frame_slot_divergence_activation(
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

  const auto call_escaped_scalarized_prepared =
      prepare_call_escaped_scalarized_local_family_module();
  if (const int rc = check_call_escaped_scalarized_local_family_activation(
          call_escaped_scalarized_prepared);
      rc != 0) {
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

  if (const int rc = check_pointer_carrier_contract_value_homes(); rc != 0) {
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
  const auto id_authoritative_stack_access_prepared =
      prepare_id_authoritative_stack_access_module();
  if (const int rc = check_id_authoritative_stack_access_activation(
          id_authoritative_stack_access_prepared);
      rc != 0) {
    return rc;
  }

  const auto link_name_authoritative_global_access_prepared =
      prepare_link_name_authoritative_global_access_module();
  if (const int rc = check_link_name_authoritative_global_access_activation(
          link_name_authoritative_global_access_prepared);
      rc != 0) {
    return rc;
  }

  const auto pic_unspecified_policy_prepared = prepare_pic_unspecified_global_policy_module();
  if (const int rc =
          check_pic_unspecified_global_policy_activation(pic_unspecified_policy_prepared);
      rc != 0) {
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
