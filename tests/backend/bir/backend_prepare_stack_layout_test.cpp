#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
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

const bir::Function* find_bir_function(const prepare::PreparedBirModule& prepared,
                                       std::string_view function_name) {
  for (const auto& function : prepared.module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
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

bir::InlineAsmOperandMetadata inline_asm_operand(
    bir::InlineAsmOperandKind kind,
    std::size_t constraint_index,
    std::string constraint,
    std::optional<std::size_t> arg_index = std::nullopt,
    std::optional<bir::MemoryAddress> memory_address = std::nullopt,
    std::optional<bir::MemoryAddress> address = std::nullopt) {
  return bir::InlineAsmOperandMetadata{
      .kind = kind,
      .constraint_index = constraint_index,
      .constraint = std::move(constraint),
      .arg_index = arg_index,
      .memory_address = std::move(memory_address),
      .address = std::move(address),
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

prepare::PreparedBirModule prepare_inline_asm_stack_layout_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_inline_asm_metadata_activation";
  function.return_type = bir::TypeKind::Void;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.inline.memory.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.inline.address.root",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.inline.missing.root",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label_id(module, "entry");
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "inline.memory.unrooted.ptr")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "ldr w0, $0",
          .constraints = "m",
          .operands = {inline_asm_operand(
              bir::InlineAsmOperandKind::MemoryInput,
              0,
              "m",
              std::size_t{0},
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                  .base_value =
                      bir::Value::named(bir::TypeKind::Ptr, "lv.inline.memory.root"),
                  .size_bytes = 4,
                  .align_bytes = 4,
              })},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "inline.address.unrooted.ptr")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "adr x0, $0",
          .constraints = "p",
          .operands = {inline_asm_operand(
              bir::InlineAsmOperandKind::AddressInput,
              0,
              "p",
              std::size_t{0},
              std::nullopt,
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                  .base_value =
                      bir::Value::named(bir::TypeKind::Ptr, "lv.inline.address.root"),
                  .size_bytes = 8,
                  .align_bytes = 8,
              })},
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.inline_asm",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "inline.missing.unrooted.ptr")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type = bir::TypeKind::Void,
      .inline_asm = bir::InlineAsmMetadata{
          .asm_text = "ldr w0, $0",
          .constraints = "m,~{memory}",
          .side_effects = true,
          .operands = {
              inline_asm_operand(
                  bir::InlineAsmOperandKind::MemoryInput, 0, "m", std::size_t{0}),
              inline_asm_operand(bir::InlineAsmOperandKind::Clobber, 1, "~{memory}"),
          },
          .clobbers = {"memory"},
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
  const auto symbol_copy_plus = intern_value("contract.symbol.copy.plus.ptr");
  const auto missing_ptr = intern_value("contract.missing.ptr");
  const auto missing_copy = intern_value("contract.missing.copy.ptr");
  const auto base_ptr = intern_value("contract.base.ptr");
  const auto prepared_copy = intern_value("contract.prepared.copy.ptr");
  const auto prepared_copy_plus = intern_value("contract.prepared.copy.plus.ptr");
  const auto adjacent_advanced = intern_value("contract.adjacent.advanced.ptr");
  const auto adjacent_after = intern_value("contract.adjacent.after.ptr");
  const auto local_recent = intern_value("contract.local.recent.ptr");
  const auto local_previous = intern_value("contract.local.previous.ptr");
  const auto global_recent = intern_value("contract.global.recent.ptr");
  const auto global_previous = intern_value("contract.global.previous.ptr");
  const auto frame_addr = intern_value("contract.frame.addr.ptr");
  const auto frame_addr_plus = intern_value("contract.frame.addr.plus.ptr");
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
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "contract.frame.addr.plus.ptr"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "contract.frame.addr.ptr"),
      .rhs = bir::Value::immediate_i32(12),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "contract.symbol.copy.plus.ptr"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "contract.symbol.copy.ptr"),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "contract.prepared.copy.plus.ptr"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "contract.prepared.copy.ptr"),
      .rhs = bir::Value::immediate_i32(8),
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
  fixture.addressing.address_materializations.push_back(prepare::PreparedAddressMaterialization{
      .function_name = function_name,
      .block_label = entry_label,
      .inst_index = 14,
      .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
      .result_value_name = frame_addr,
      .frame_slot_id = prepare::PreparedFrameSlotId{0},
      .byte_offset = 24,
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
  add_ptr_value(8, frame_addr_plus);
  add_ptr_value(9, symbol_copy_plus);
  add_ptr_value(10, prepared_copy_plus);

  (void)symbol_ptr;
  (void)missing_ptr;
  (void)local_recent;
  (void)global_recent;
  (void)frame_addr;
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

std::optional<prepare::PreparedBirModule> prepare_lir_byte_storage_overlay_module() {
  lir::LirModule module;
  module.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);
  module.type_decls.push_back("%struct.uf = type [4 x i8]");

  lir::LirFunction function;
  function.name = "stack_layout_byte_storage_overlay_activation";
  function.signature_text = "define void @stack_layout_byte_storage_overlay_activation(i32 %p.v)";
  function.params.emplace_back("%p.v", c4c::TypeSpec{.base = c4c::TB_INT});
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.u"),
      .type_str = lir::LirTypeRef("%struct.uf"),
      .count = lir::LirOperand(""),
      .align = 4,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%lv.u.0"),
      .element_type = lir::LirTypeRef("%struct.uf"),
      .ptr = lir::LirOperand("%lv.u"),
      .inbounds = true,
      .indices = {"i64 0", "i64 0"},
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = lir::LirTypeRef("i32"),
      .val = lir::LirOperand("%p.v"),
      .ptr = lir::LirOperand("%lv.u.0"),
  });
  entry.insts.push_back(lir::LirLoadOp{
      .result = lir::LirOperand("%loaded.f"),
      .type_str = lir::LirTypeRef("float"),
      .ptr = lir::LirOperand("%lv.u.0"),
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
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Riscv64);

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

std::optional<prepare::PreparedBirModule>
prepare_same_module_computed_address_formal_provenance_module(
    bool callee_is_internal) {
  lir::LirModule module;
  module.target_profile = riscv_target_profile();
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  const auto global_link_name = module.link_names.intern("same_module_mem");
  const auto callee_link_name =
      module.link_names.intern("same_module_computed_address_callee");
  const auto caller_link_name =
      module.link_names.intern("same_module_computed_address_caller");

  lir::LirGlobal global;
  global.name = "same_module_mem";
  global.link_name_id = global_link_name;
  global.qualifier = "global ";
  global.llvm_type = "[4 x i64]";
  global.init_text = "zeroinitializer";
  global.align_bytes = 8;
  module.globals.push_back(std::move(global));

  lir::LirFunction callee;
  callee.name = "same_module_computed_address_callee";
  callee.link_name_id = callee_link_name;
  callee.is_internal = callee_is_internal;
  callee.signature_text =
      "define void @same_module_computed_address_callee(ptr %p.data)";
  callee.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.param.data"),
      .type_str = lir::LirTypeRef("ptr"),
      .count = lir::LirOperand(""),
      .align = 8,
  });

  lir::LirBlock callee_entry;
  callee_entry.label = "entry";
  callee_entry.insts.push_back(lir::LirStoreOp{
      .type_str = lir::LirTypeRef("ptr"),
      .val = lir::LirOperand("%p.data"),
      .ptr = lir::LirOperand("%lv.param.data"),
  });
  for (const auto& [loaded_ptr, loaded_value] :
       {std::pair<std::string_view, std::string_view>{"%loaded.ptr.0", "%loaded.value.0"},
        std::pair<std::string_view, std::string_view>{"%loaded.ptr.1", "%loaded.value.1"},
        std::pair<std::string_view, std::string_view>{"%loaded.ptr.2", "%loaded.value.2"}}) {
    callee_entry.insts.push_back(lir::LirLoadOp{
        .result = lir::LirOperand(std::string(loaded_ptr)),
        .type_str = lir::LirTypeRef("ptr"),
        .ptr = lir::LirOperand("%lv.param.data"),
    });
    callee_entry.insts.push_back(lir::LirLoadOp{
        .result = lir::LirOperand(std::string(loaded_value)),
        .type_str = lir::LirTypeRef("i64"),
        .ptr = lir::LirOperand(std::string(loaded_ptr)),
    });
  }
  callee_entry.terminator = lir::LirRet{std::nullopt, "void"};
  callee.blocks.push_back(std::move(callee_entry));
  module.functions.push_back(std::move(callee));

  lir::LirFunction caller;
  caller.name = "same_module_computed_address_caller";
  caller.link_name_id = caller_link_name;
  caller.signature_text = "define void @same_module_computed_address_caller()";
  caller.signature_has_void_param_list = true;

  lir::LirBlock caller_entry;
  caller_entry.label = "entry";
  caller_entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%arg.ptr"),
      .element_type = lir::LirTypeRef("[4 x i64]"),
      .ptr = lir::LirOperand("@same_module_mem"),
      .inbounds = true,
      .indices = {"i64 0", "i64 1"},
  });
  caller_entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand(""),
      .return_type = lir::LirTypeRef("void"),
      .callee = lir::LirOperand("@same_module_computed_address_callee"),
      .direct_callee_link_name_id = callee_link_name,
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %arg.ptr",
      .callee_signature =
          lir::LirCallSignature{
              .return_type_ref = lir::LirTypeRef("void"),
              .fixed_param_types = {"ptr"},
              .fixed_param_type_refs = {lir::LirTypeRef("ptr")},
          },
      .structured_args =
          {lir::LirCallArg{
              .type = "ptr",
              .operand = lir::LirOperand("%arg.ptr"),
              .type_ref = lir::LirTypeRef("ptr"),
          }},
  });
  caller_entry.terminator = lir::LirRet{std::nullopt, "void"};
  caller.blocks.push_back(std::move(caller_entry));
  module.functions.push_back(std::move(caller));

  const auto lowered_result =
      c4c::backend::try_lower_to_bir_with_options(module, c4c::backend::BirLoweringOptions{});
  if (!lowered_result.module.has_value()) {
    for (const auto& note : lowered_result.notes) {
      std::cerr << "same-module formal fixture lowering note [" << note.phase
                << "]: " << note.message << "\n";
    }
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = *lowered_result.module;
  prepared.target_profile = riscv_target_profile();

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

int check_byte_storage_overlay_local_slot_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* object = find_stack_object(prepared, "%lv.u.0");
  if (object == nullptr || object->type != bir::TypeKind::I8 ||
      object->size_bytes != 4 || object->align_bytes != 4 ||
      !object->address_exposed || !object->requires_home_slot) {
    return fail("byte-storage overlay base stack object should publish a covering i8 extent");
  }

  const auto* base_slot = find_frame_slot(prepared, object->object_id);
  if (base_slot == nullptr || base_slot->size_bytes != 4 ||
      base_slot->align_bytes != 4 || !base_slot->fixed_location) {
    return fail("byte-storage overlay base frame slot should cover the aggregate storage");
  }

  const auto* byte_object = find_stack_object(prepared, "%lv.u.1");
  if (byte_object == nullptr || byte_object->size_bytes != 1) {
    return fail("byte-storage overlay sibling byte leaves should remain byte-sized");
  }

  const auto function_name_id =
      find_function_name_id(prepared, "stack_layout_byte_storage_overlay_activation");
  const auto entry_block_label_id = find_block_label_id(prepared, "entry");
  const auto* function_addressing =
      prepare::find_prepared_addressing(prepared, function_name_id);
  if (function_addressing == nullptr) {
    return fail("byte-storage overlay function should publish prepared addressing facts");
  }

  const auto* store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 0);
  const auto* load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 1);
  if (store_access == nullptr || load_access == nullptr) {
    return fail("byte-storage overlay store/load should publish prepared memory accesses");
  }
  for (const auto* access : {store_access, load_access}) {
    if (access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
        !access->address.frame_slot_id.has_value() ||
        *access->address.frame_slot_id != base_slot->slot_id ||
        access->address.byte_offset != 0 ||
        access->address.size_bytes != 4 ||
        access->address.provenance.range_verdict !=
            bir::MemoryRangeVerdict::ProvenInBounds) {
      return fail("byte-storage overlay prepared access should select the covering frame slot");
    }
  }
  return 0;
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
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I16, "%copy.malformed"),
      .slot_name = "%slice.src.0",
      .align_bytes = 2,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "%slice.src.bad",
              .byte_offset = 2,
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

  const auto* src0_object = find_stack_object(prepared, "%slice.src.0");
  const auto* src1_object = find_stack_object(prepared, "%slice.src.1");
  const auto* src2_object = find_stack_object(prepared, "%slice.src.2");
  const auto* dst0_object = find_stack_object(prepared, "%slice.dst.0");
  const auto* dst1_object = find_stack_object(prepared, "%slice.dst.1");
  const auto* dst2_object = find_stack_object(prepared, "%slice.dst.2");
  if (src0_object == nullptr || src1_object == nullptr || src2_object == nullptr ||
      dst0_object == nullptr || dst1_object == nullptr || dst2_object == nullptr) {
    return fail("expected sliced i16 local fixture to preserve consecutive slice objects");
  }
  const auto check_slice_family = [&](const prepare::PreparedStackObject& object,
                                      std::string_view family,
                                      std::size_t offset) -> int {
    if (!object.slice_family.has_value()) {
      return fail("expected sliced local object to publish structured slice-family metadata");
    }
    if (!object.slice_family->legacy_slot_name_compatibility ||
        prepare::prepared_slot_name(prepared.names, object.slice_family->family_name) != family ||
        object.slice_family->slice_offset != offset) {
      return fail("expected sliced local object metadata to carry family and slice offset");
    }
    if (!object.requires_home_slot || !object.permanent_home_slot ||
        !object.address_exposed) {
      return fail("expected addressed sliced local object to keep frame placement requirements");
    }
    return 0;
  };
  if (const int rc = check_slice_family(*src0_object, "%slice.src", 0); rc != 0) {
    return rc;
  }
  if (const int rc = check_slice_family(*src1_object, "%slice.src", 1); rc != 0) {
    return rc;
  }
  if (const int rc = check_slice_family(*src2_object, "%slice.src", 2); rc != 0) {
    return rc;
  }
  if (const int rc = check_slice_family(*dst0_object, "%slice.dst", 0); rc != 0) {
    return rc;
  }
  if (const int rc = check_slice_family(*dst1_object, "%slice.dst", 1); rc != 0) {
    return rc;
  }
  if (const int rc = check_slice_family(*dst2_object, "%slice.dst", 2); rc != 0) {
    return rc;
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
  if (prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 4) !=
      nullptr) {
    return fail("expected malformed slice-family address to fail closed");
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

prepare::PreparedBirModule prepare_sret_param_home_module(
    c4c::TargetProfile target_profile,
    std::size_t sret_size_bytes,
    std::size_t sret_align_bytes) {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_sret_param_home_activation";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.sret.home",
      .size_bytes = sret_size_bytes,
      .align_bytes = sret_align_bytes,
      .is_sret = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(0),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = std::move(target_profile);

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
      .has_scalar_layout_authority = true,
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
  const c4c::LinkNameId aggregate_global_id =
      module.names.link_names.intern("g.aggregate.contract");
  module.globals.push_back(bir::Global{
      .name = "g.aggregate.contract",
      .link_name_id = aggregate_global_id,
      .type = bir::TypeKind::Void,
      .size_bytes = 96,
      .align_bytes = 16,
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  const c4c::LinkNameId tls_global_id = module.names.link_names.intern("g.tls");
  module.globals.push_back(bir::Global{
      .name = "g.tls",
      .link_name_id = tls_global_id,
      .type = bir::TypeKind::I32,
      .is_thread_local = true,
      .has_scalar_layout_authority = true,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  const c4c::LinkNameId no_extent_global_id = module.names.link_names.intern("g.no.extent");
  module.globals.push_back(bir::Global{
      .name = "g.no.extent",
      .link_name_id = no_extent_global_id,
      .type = bir::TypeKind::I32,
      .has_scalar_layout_authority = true,
      .align_bytes = 4,
  });
  const c4c::LinkNameId int_array_global_id = module.names.link_names.intern("g.i64.array");
  module.globals.push_back(bir::Global{
      .name = "g.i64.array",
      .link_name_id = int_array_global_id,
      .type = bir::TypeKind::I64,
      .has_integer_array_layout_authority = true,
      .integer_array_element_size_bytes = 8,
      .integer_array_element_count = 10,
      .size_bytes = 80,
      .align_bytes = 8,
  });
  const c4c::LinkNameId extern_int_array_global_id =
      module.names.link_names.intern("g.extern.i64.array");
  module.globals.push_back(bir::Global{
      .name = "g.extern.i64.array",
      .link_name_id = extern_int_array_global_id,
      .type = bir::TypeKind::I64,
      .is_extern = true,
      .has_integer_array_layout_authority = true,
      .integer_array_element_size_bytes = 8,
      .integer_array_element_count = 10,
      .size_bytes = 80,
      .align_bytes = 8,
  });
  const c4c::LinkNameId tls_int_array_global_id =
      module.names.link_names.intern("g.tls.i64.array");
  module.globals.push_back(bir::Global{
      .name = "g.tls.i64.array",
      .link_name_id = tls_int_array_global_id,
      .type = bir::TypeKind::I64,
      .is_thread_local = true,
      .has_integer_array_layout_authority = true,
      .integer_array_element_size_bytes = 8,
      .integer_array_element_count = 10,
      .size_bytes = 80,
      .align_bytes = 8,
  });
  const c4c::LinkNameId missing_extent_int_array_global_id =
      module.names.link_names.intern("g.missing.extent.i64.array");
  module.globals.push_back(bir::Global{
      .name = "g.missing.extent.i64.array",
      .link_name_id = missing_extent_int_array_global_id,
      .type = bir::TypeKind::I64,
      .has_integer_array_layout_authority = true,
      .integer_array_element_size_bytes = 8,
      .integer_array_element_count = 10,
      .align_bytes = 8,
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
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "raw.structured.loaded"),
      .global_name = "g.authoritative",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "gep.id.loaded"),
      .global_name_id = canonical_global_id,
      .byte_offset = 4,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .byte_offset = 8,
              .size_bytes = 4,
              .align_bytes = 4,
              .base_link_name_id = canonical_global_id,
          },
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "gep.raw.structured.loaded"),
      .global_name_id = canonical_global_id,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .base_name = "g.authoritative",
              .byte_offset = 12,
              .size_bytes = 4,
              .align_bytes = 4,
          },
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "gep.compat.loaded"),
      .global_name = "g.compat",
      .byte_offset = 4,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .base_name = "g.compat",
              .byte_offset = 16,
              .size_bytes = 4,
              .align_bytes = 4,
          },
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = bir::Value::named(bir::TypeKind::Ptr, "@g.raw.materialization"),
      .operand = bir::Value::named(bir::TypeKind::Ptr, "unused.raw.materialization.source"),
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "local.global.lane.loaded"),
      .global_name = "g.aggregate.contract",
      .global_name_id = aggregate_global_id,
      .byte_offset = 24,
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "local.global.raw.loaded"),
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .base_name = "g.aggregate.contract",
              .byte_offset = 28,
              .size_bytes = 4,
              .align_bytes = 4,
          },
  });
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name_id = canonical_global_id,
      .value = bir::Value::named(bir::TypeKind::I32, "id.loaded"),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "scalar.out.of.range.loaded"),
      .global_name_id = canonical_global_id,
      .byte_offset = 8,
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "scalar.missing.extent.loaded"),
      .global_name_id = no_extent_global_id,
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "array.element.loaded"),
      .global_name_id = int_array_global_id,
      .byte_offset = 72,
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "array.out.of.range.loaded"),
      .global_name_id = int_array_global_id,
      .byte_offset = 80,
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "array.misaligned.offset.loaded"),
      .global_name_id = int_array_global_id,
      .byte_offset = 4,
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "array.partial.width.loaded"),
      .global_name_id = int_array_global_id,
      .byte_offset = 8,
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "array.extern.loaded"),
      .global_name_id = extern_int_array_global_id,
      .byte_offset = 8,
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "array.tls.loaded"),
      .global_name_id = tls_int_array_global_id,
      .byte_offset = 8,
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "array.missing.extent.loaded"),
      .global_name_id = missing_extent_int_array_global_id,
      .byte_offset = 8,
      .align_bytes = 8,
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
  if (copy_object->slice_family.has_value() || copy_object->aggregate_address_published ||
      copy_object->frame_address_value_name.has_value() ||
      copy_object->legacy_frame_address_name_compatibility) {
    return fail("expected copy coalescing to remain a placement-only hint");
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

int check_sret_param_home_rv64_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* sret_object = find_stack_object(prepared, "p.sret.home");
  if (sret_object == nullptr) {
    return fail("expected RV64 sret param to produce a stack-layout object");
  }
  if (sret_object->source_kind != "sret_param" ||
      !sret_object->address_exposed ||
      !sret_object->requires_home_slot ||
      !sret_object->permanent_home_slot) {
    return fail("expected RV64 sret param to keep its permanent-home contract");
  }
  if (sret_object->type != bir::TypeKind::Ptr ||
      sret_object->size_bytes != 8 ||
      sret_object->align_bytes != 8) {
    return fail("expected RV64 sret param storage to normalize to pointer-width");
  }

  const auto* sret_slot = find_frame_slot(prepared, sret_object->object_id);
  if (sret_slot == nullptr) {
    return fail("expected RV64 sret param to receive frame-slot storage");
  }
  if (!sret_slot->fixed_location ||
      sret_slot->offset_bytes != 0 ||
      sret_slot->size_bytes != 8 ||
      sret_slot->align_bytes != 8) {
    return fail("expected RV64 sret param frame slot to use pointer-width storage");
  }
  if (prepared.stack_layout.frame_size_bytes != 8 ||
      prepared.stack_layout.frame_alignment_bytes != 8) {
    return fail("expected RV64 sret param frame metrics to reflect pointer-width storage");
  }

  return 0;
}

int check_sret_param_home_aarch64_activation(
    const prepare::PreparedBirModule& prepared,
    std::size_t expected_size_bytes,
    std::size_t expected_align_bytes) {
  const auto* sret_object = find_stack_object(prepared, "p.sret.home");
  if (sret_object == nullptr) {
    return fail("expected AArch64 sret param to produce a stack-layout object");
  }
  if (sret_object->source_kind != "sret_param" ||
      sret_object->size_bytes != expected_size_bytes ||
      sret_object->align_bytes != expected_align_bytes) {
    return fail("expected AArch64 sret param storage to preserve shared BIR layout");
  }

  const auto* sret_slot = find_frame_slot(prepared, sret_object->object_id);
  if (sret_slot == nullptr) {
    return fail("expected AArch64 sret param to receive frame-slot storage");
  }
  if (!sret_slot->fixed_location ||
      sret_slot->offset_bytes != 0 ||
      sret_slot->size_bytes != expected_size_bytes ||
      sret_slot->align_bytes != expected_align_bytes) {
    return fail("expected AArch64 sret param frame slot to preserve shared layout");
  }
  if (prepared.stack_layout.frame_size_bytes != expected_size_bytes ||
      prepared.stack_layout.frame_alignment_bytes != expected_align_bytes) {
    return fail("expected AArch64 sret param frame metrics to preserve shared layout");
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
  if (prepare::prepared_pointer_value_memory_has_proven_authority(
          pointer_store_access->address)) {
    return fail("expected unknown pointer-indirect local store authority to stay non-target-consumable");
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
  if (prepare::prepared_pointer_value_memory_has_proven_authority(
          pointer_store_access->address)) {
    return fail("expected unknown pointer-indirect global store authority to stay non-target-consumable");
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
  if (prepare::prepared_pointer_value_memory_has_proven_authority(
          pointer_load_access->address)) {
    return fail("expected unknown pointer-indirect global load authority to stay non-target-consumable");
  }

  return 0;
}

const prepare::PreparedMemoryAccess* find_pointer_value_access_by_result(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name,
    std::string_view result_name) {
  const auto* function_addressing =
      prepare::find_prepared_addressing(prepared, find_function_name_id(prepared, function_name));
  if (function_addressing == nullptr) {
    return nullptr;
  }
  for (const auto& access : function_addressing->accesses) {
    if (access.address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
        !access.result_value_name.has_value()) {
      continue;
    }
    if (prepare::prepared_value_name(prepared.names, *access.result_value_name) ==
        result_name) {
      return &access;
    }
  }
  return nullptr;
}

int check_same_module_computed_address_formal_provenance_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* callee = find_bir_function(prepared, "same_module_computed_address_callee");
  if (callee == nullptr) {
    return fail("expected same-module formal provenance fixture to keep callee function");
  }
  if (callee->formal_pointer_authority != bir::FormalPointerAuthorityKind::InternalOnly) {
    return fail("expected internal same-module callee to publish internal formal pointer authority");
  }

  for (const std::string_view result_name :
       {"%loaded.value.0", "%loaded.value.1", "%loaded.value.2"}) {
    const auto* access = find_pointer_value_access_by_result(
        prepared,
        "same_module_computed_address_callee",
        result_name);
    if (access == nullptr) {
      return fail("expected same-module formal pointer loads to publish prepared addressing");
    }
    if (!access->address.pointer_value_name.has_value() ||
        access->address.byte_offset != 0 ||
        access->address.size_bytes != 8 ||
        access->address.align_bytes != 8 ||
        !access->address.can_use_base_plus_offset) {
      return fail("expected same-module formal pointer loads to keep pointer-value address facts");
    }
    if (!prepare::prepared_pointer_value_memory_has_proven_authority(access->address)) {
      return fail("expected same-module computed-address formal provenance to be target-consumable");
    }
    const auto& provenance = access->address.provenance;
    if (provenance.base_identity.kind !=
            bir::MemoryProvenanceBaseIdentityKind::GlobalSymbol ||
        provenance.base_identity.spelling != "@same_module_mem+8" ||
        provenance.object_extent.completeness !=
            bir::MemoryObjectExtentCompleteness::Complete ||
        provenance.object_extent.size_bytes != 24 ||
        provenance.requested_range.begin != 0 ||
        provenance.requested_range.size_bytes != 8 ||
        provenance.requested_range.end != 8 ||
        provenance.layout_authority != bir::MemoryLayoutAuthorityKind::ScalarLayout ||
        provenance.range_verdict != bir::MemoryRangeVerdict::ProvenInBounds) {
      return fail("expected same-module formal pointer loads to retain global range authority");
    }
  }

  return 0;
}

int check_external_same_module_computed_address_formal_provenance_stays_fail_closed(
    const prepare::PreparedBirModule& prepared) {
  const auto* callee = find_bir_function(prepared, "same_module_computed_address_callee");
  if (callee == nullptr) {
    return fail("expected external-linkage same-module formal fixture to keep callee function");
  }
  if (callee->formal_pointer_authority != bir::FormalPointerAuthorityKind::Unknown) {
    return fail("expected external-linkage same-module callee to keep unknown formal pointer authority");
  }

  const auto* access = find_pointer_value_access_by_result(
      prepared,
      "same_module_computed_address_callee",
      "%loaded.value.0");
  if (access == nullptr) {
    return fail("expected external-linkage same-module formal fixture to publish addressing");
  }
  if (prepare::prepared_pointer_value_memory_has_proven_authority(access->address)) {
    return fail(
        "expected external-linkage same-module formal provenance to stay fail-closed");
  }

  return 0;
}

prepare::PreparedAddress make_proven_pointer_value_memory_address(
    c4c::ValueNameId pointer_name) {
  auto requested_range = bir::make_memory_byte_range(8, 4);
  return prepare::PreparedAddress{
      .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
      .pointer_value_name = pointer_name,
      .byte_offset = 8,
      .size_bytes = 4,
      .align_bytes = 4,
      .can_use_base_plus_offset = true,
      .provenance =
          bir::MemoryAccessProvenance{
              .base_identity =
                  bir::MemoryProvenanceBaseIdentity{
                      .kind = bir::MemoryProvenanceBaseIdentityKind::LocalSlot,
                      .spelling = "%object",
                  },
              .object_extent =
                  bir::MemoryObjectExtent{
                      .completeness = bir::MemoryObjectExtentCompleteness::Complete,
                      .size_bytes = 16,
                      .size_known = true,
                  },
              .requested_range = requested_range,
              .layout_authority = bir::MemoryLayoutAuthorityKind::ScalarLayout,
              .range_verdict = bir::MemoryRangeVerdict::ProvenInBounds,
          },
  };
}

int check_pointer_value_memory_authority_contract() {
  prepare::PreparedNameTables names;
  const auto pointer_name = names.value_names.intern("%ptr");
  const auto proven_address =
      make_proven_pointer_value_memory_address(pointer_name);
  if (!prepare::prepared_pointer_value_memory_has_proven_authority(proven_address)) {
    return fail("expected explicit pointer-value memory authority to classify target-consumable");
  }

  auto rejected_address = proven_address;
  rejected_address.provenance.layout_authority =
      bir::MemoryLayoutAuthorityKind::Unknown;
  if (prepare::prepared_pointer_value_memory_has_proven_authority(rejected_address)) {
    return fail("expected unknown pointer-value layout authority to stay fail-closed");
  }

  rejected_address = proven_address;
  rejected_address.provenance.range_verdict =
      bir::MemoryRangeVerdict::UnknownCompatible;
  if (prepare::prepared_pointer_value_memory_has_proven_authority(rejected_address)) {
    return fail("expected unknown-compatible pointer-value range to stay fail-closed");
  }

  rejected_address = proven_address;
  rejected_address.provenance.base_identity.kind =
      bir::MemoryProvenanceBaseIdentityKind::PointerValue;
  if (prepare::prepared_pointer_value_memory_has_proven_authority(rejected_address)) {
    return fail("expected bare pointer-value identity to stay fail-closed");
  }

  rejected_address = proven_address;
  rejected_address.pointer_value_name = std::nullopt;
  if (prepare::prepared_pointer_value_memory_has_proven_authority(rejected_address)) {
    return fail("expected missing pointer value name to stay fail-closed");
  }

  rejected_address = proven_address;
  rejected_address.can_use_base_plus_offset = false;
  if (prepare::prepared_pointer_value_memory_has_proven_authority(rejected_address)) {
    return fail("expected non-base-plus-offset pointer value access to stay fail-closed");
  }

  rejected_address = proven_address;
  rejected_address.provenance.requested_range =
      bir::make_memory_byte_range(12, 8);
  rejected_address.provenance.range_verdict =
      bir::MemoryRangeVerdict::ProvenOutOfBounds;
  if (prepare::prepared_pointer_value_memory_has_proven_authority(rejected_address)) {
    return fail("expected out-of-bounds pointer-value range to stay fail-closed");
  }

  return 0;
}

prepare::PreparedAddress make_proven_global_symbol_memory_address(
    c4c::LinkNameId symbol_name) {
  auto requested_range = bir::make_memory_byte_range(4, 4);
  return prepare::PreparedAddress{
      .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
      .symbol_name = symbol_name,
      .byte_offset = 4,
      .size_bytes = 4,
      .align_bytes = 4,
      .can_use_base_plus_offset = true,
      .provenance =
          bir::MemoryAccessProvenance{
              .base_identity =
                  bir::MemoryProvenanceBaseIdentity{
                      .kind = bir::MemoryProvenanceBaseIdentityKind::GlobalSymbol,
                      .spelling = "@g.counter",
                  },
              .object_extent =
                  bir::MemoryObjectExtent{
                      .completeness = bir::MemoryObjectExtentCompleteness::Complete,
                      .size_bytes = 16,
                      .size_known = true,
                  },
              .requested_range = requested_range,
              .layout_authority = bir::MemoryLayoutAuthorityKind::ScalarLayout,
              .range_verdict = bir::MemoryRangeVerdict::ProvenInBounds,
          },
  };
}

int check_global_memory_publication_authority_contract() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("authority.global");
  const auto block_label = names.block_labels.intern("entry");
  const auto symbol_name = names.link_names.intern("g.counter");
  const auto source_value_name = names.value_names.intern("%sum");

  const auto proven_address =
      make_proven_global_symbol_memory_address(symbol_name);
  if (!prepare::prepared_global_symbol_memory_has_publication_authority(
          proven_address)) {
    return fail("expected explicit global memory authority to classify target-consumable");
  }

  auto rejected_address = proven_address;
  rejected_address.provenance.layout_authority =
      bir::MemoryLayoutAuthorityKind::Unknown;
  if (prepare::prepared_global_symbol_memory_has_publication_authority(
          rejected_address)) {
    return fail("expected unknown global layout authority to stay fail-closed");
  }

  rejected_address = proven_address;
  rejected_address.provenance.range_verdict =
      bir::MemoryRangeVerdict::UnknownCompatible;
  if (prepare::prepared_global_symbol_memory_has_publication_authority(
          rejected_address)) {
    return fail("expected unknown-compatible global range to stay fail-closed");
  }

  rejected_address = proven_address;
  rejected_address.provenance.base_identity.kind =
      bir::MemoryProvenanceBaseIdentityKind::PointerValue;
  if (prepare::prepared_global_symbol_memory_has_publication_authority(
          rejected_address)) {
    return fail("expected non-global provenance identity to stay fail-closed");
  }

  rejected_address = proven_address;
  rejected_address.base_kind = prepare::PreparedAddressBaseKind::PointerValue;
  rejected_address.pointer_value_name = source_value_name;
  if (prepare::prepared_global_symbol_memory_has_publication_authority(
          rejected_address)) {
    return fail("expected pointer-value memory to stay out of global authority");
  }

  rejected_address = proven_address;
  rejected_address.symbol_name = std::nullopt;
  if (prepare::prepared_global_symbol_memory_has_publication_authority(
          rejected_address)) {
    return fail("expected missing global symbol identity to stay fail-closed");
  }

  const bir::Value source_value = bir::Value::named(bir::TypeKind::I32, "%sum");
  const bir::BinaryInst binary{
      .opcode = bir::BinaryOpcode::Add,
      .result = source_value,
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%lhs"),
      .rhs = bir::Value::immediate_i32(1),
  };
  const prepare::PreparedMemoryAccess global_store_access{
      .function_name = function_name,
      .block_label = block_label,
      .inst_index = 2,
      .stored_value_name = source_value_name,
      .address = proven_address,
  };
  const prepare::PreparedValueHome source_home{
      .value_id = 7,
      .function_name = function_name,
      .value_name = source_value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"s1"},
      .size_bytes = 4,
      .align_bytes = 4,
  };
  const prepare::PreparedEdgePublicationSourceProducer binary_producer{
      .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
      .block_label = block_label,
      .instruction_index = 1,
      .binary = &binary,
  };
  const auto coherent_plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source_value,
      .destination_access = &global_store_access,
      .source_home = &source_home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
      .source_producer = &binary_producer,
      .publication_instruction_index = std::size_t{2},
  });
  if (!prepare::prepared_store_global_publication_has_authority(coherent_plan)) {
    return fail("expected coherent global store source publication authority");
  }

  auto unknown_layout_access = global_store_access;
  unknown_layout_access.address.provenance.layout_authority =
      bir::MemoryLayoutAuthorityKind::Unknown;
  const auto unknown_layout_plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source_value,
      .destination_access = &unknown_layout_access,
      .source_home = &source_home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
      .source_producer = &binary_producer,
      .publication_instruction_index = std::size_t{2},
  });
  if (prepare::prepared_store_global_publication_has_authority(
          unknown_layout_plan)) {
    return fail("expected global store with unknown layout authority to stay fail-closed");
  }

  const auto unknown_source_plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source_value,
      .destination_access = &global_store_access,
      .source_home = &source_home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
      .publication_instruction_index = std::size_t{2},
  });
  if (prepare::prepared_store_global_publication_has_authority(
          unknown_source_plan)) {
    return fail("expected missing store-source producer to stay fail-closed");
  }

  const auto missing_home_plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source_value,
      .destination_access = &global_store_access,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
      .source_producer = &binary_producer,
      .publication_instruction_index = std::size_t{2},
  });
  if (prepare::prepared_store_global_publication_has_authority(
          missing_home_plan)) {
    return fail("expected missing store-source value home to stay fail-closed");
  }

  const auto local_plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source_value,
      .destination_access = &global_store_access,
      .source_home = &source_home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
      .source_producer = &binary_producer,
      .publication_instruction_index = std::size_t{2},
  });
  if (prepare::prepared_store_global_publication_has_authority(local_plan)) {
    return fail("expected local store-source publication to stay out of global authority");
  }

  const bir::Value immediate_source = bir::Value::immediate_i32(1);
  const auto implicit_immediate_plan = prepare::plan_prepared_store_source_publication({
      .source_value = &immediate_source,
      .destination_access = &global_store_access,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
      .publication_instruction_index = std::size_t{2},
  });
  if (prepare::prepared_store_global_publication_has_authority(
          implicit_immediate_plan)) {
    return fail("expected implicit immediate store source to stay fail-closed");
  }

  auto explicit_immediate_plan = implicit_immediate_plan;
  explicit_immediate_plan.source_producer_kind =
      prepare::PreparedEdgePublicationSourceProducerKind::Immediate;
  if (!prepare::prepared_store_global_publication_has_authority(
          explicit_immediate_plan)) {
    return fail("expected explicit immediate store source encoding to be accepted");
  }

  return 0;
}

int check_direct_global_return_authority_contract() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("direct_global_return");
  const auto value_name = names.value_names.intern("@global");
  const auto symbol_name = names.link_names.intern("global");
  const bir::Value direct_global_return =
      bir::Value::named_symbol_pointer("@global", symbol_name);
  const prepare::PreparedValueHome direct_global_home{
      .value_id = 4,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  };
  const prepare::PreparedMoveResolution return_move{
      .from_value_id = direct_global_home.value_id,
      .to_value_id = direct_global_home.value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = std::string{"a0"},
      .block_index = 1,
      .instruction_index = 3,
      .destination_register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = prepare::PreparedRegisterSlotPool::CallResult,
          },
  };

  const auto accepted =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &direct_global_return,
          .value_home = &direct_global_home,
          .before_return_move = &return_move,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (!prepare::prepared_direct_global_return_authority_available(accepted) ||
      accepted.global_symbol_name != symbol_name ||
      accepted.value_id != direct_global_home.value_id ||
      accepted.value_name != value_name ||
      accepted.return_bank != prepare::PreparedRegisterBank::Gpr) {
    return fail("expected coherent direct-global return authority");
  }

  auto raw_named_pointer = direct_global_return;
  raw_named_pointer.pointer_symbol_link_name_id = c4c::kInvalidLinkName;
  const auto raw_only =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &raw_named_pointer,
          .value_home = &direct_global_home,
          .before_return_move = &return_move,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (raw_only.status !=
      prepare::PreparedDirectGlobalReturnAuthorityStatus::MissingGlobalIdentity) {
    return fail("expected raw named pointer return to miss direct-global identity");
  }

  const bir::Value non_pointer = bir::Value::named(bir::TypeKind::I32, "@global");
  const auto wrong_type =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &non_pointer,
          .value_home = &direct_global_home,
          .before_return_move = &return_move,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (wrong_type.status !=
      prepare::PreparedDirectGlobalReturnAuthorityStatus::UnsupportedReturnValue) {
    return fail("expected non-pointer direct-global return to stay fail-closed");
  }

  const auto missing_home =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &direct_global_return,
          .before_return_move = &return_move,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (missing_home.status !=
      prepare::PreparedDirectGlobalReturnAuthorityStatus::MissingValueHome) {
    return fail("expected missing direct-global return home to stay fail-closed");
  }

  auto mismatched_home = direct_global_home;
  mismatched_home.value_name = names.value_names.intern("@other");
  const auto home_mismatch =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &direct_global_return,
          .value_home = &mismatched_home,
          .before_return_move = &return_move,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (home_mismatch.status !=
      prepare::PreparedDirectGlobalReturnAuthorityStatus::HomeValueMismatch) {
    return fail("expected mismatched direct-global return home to stay fail-closed");
  }

  auto stack_home = direct_global_home;
  stack_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  stack_home.register_name = std::nullopt;
  const auto unsupported_home =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &direct_global_return,
          .value_home = &stack_home,
          .before_return_move = &return_move,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (unsupported_home.status !=
      prepare::PreparedDirectGlobalReturnAuthorityStatus::UnsupportedHomeKind) {
    return fail("expected stack-home direct-global return to stay fail-closed");
  }

  const auto missing_move =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &direct_global_return,
          .value_home = &direct_global_home,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (missing_move.status !=
      prepare::PreparedDirectGlobalReturnAuthorityStatus::MissingReturnMove) {
    return fail("expected missing return ABI move to stay fail-closed");
  }

  auto mismatched_move = return_move;
  mismatched_move.from_value_id = 99;
  const auto move_mismatch =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &direct_global_return,
          .value_home = &direct_global_home,
          .before_return_move = &mismatched_move,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (move_mismatch.status !=
      prepare::PreparedDirectGlobalReturnAuthorityStatus::ReturnMoveMismatch) {
    return fail("expected mismatched return ABI move to stay fail-closed");
  }

  auto unsupported_move = return_move;
  unsupported_move.destination_kind =
      prepare::PreparedMoveDestinationKind::Value;
  const auto unsupported_destination =
      prepare::plan_prepared_direct_global_return_authority({
          .names = &names,
          .return_value = &direct_global_return,
          .value_home = &direct_global_home,
          .before_return_move = &unsupported_move,
          .block_index = 1,
          .instruction_index = 3,
      });
  if (unsupported_destination.status !=
      prepare::PreparedDirectGlobalReturnAuthorityStatus::UnsupportedReturnDestination) {
    return fail("expected non-return destination move to stay fail-closed");
  }

  return 0;
}

int check_fused_pointer_branch_publication_contract() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("pointer_branch");
  const auto entry_label = names.block_labels.intern("entry");
  const auto true_label = names.block_labels.intern("is_true");
  const auto false_label = names.block_labels.intern("is_false");
  const auto condition_name = names.value_names.intern("%cmp");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto rhs_name = names.value_names.intern("%rhs");

  bir::Terminator terminator;
  terminator.kind = bir::TerminatorKind::CondBranch;
  terminator.condition = bir::Value::named(bir::TypeKind::I32, "%cmp");
  terminator.true_label = "is_true";
  terminator.false_label = "is_false";

  const prepare::PreparedBranchCondition branch_condition{
      .function_name = function_name,
      .block_label = entry_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = bir::Value::named(bir::TypeKind::I32, "%cmp"),
      .predicate = bir::BinaryOpcode::Ne,
      .compare_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "%lhs"),
      .rhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs"),
      .can_fuse_with_branch = true,
      .true_label = true_label,
      .false_label = false_label,
  };
  const prepare::PreparedValueHome condition_home{
      .value_id = 1,
      .function_name = function_name,
      .value_name = condition_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"s2"},
  };
  const prepare::PreparedValueHome lhs_home{
      .value_id = 2,
      .function_name = function_name,
      .value_name = lhs_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  };
  const prepare::PreparedValueHome rhs_home{
      .value_id = 3,
      .function_name = function_name,
      .value_name = rhs_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"s1"},
  };

  const auto accepted =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .condition_home = &condition_home,
          .lhs_home = &lhs_home,
          .rhs_home = &rhs_home,
      });
  if (!prepare::prepared_fused_pointer_branch_publication_available(accepted) ||
      accepted.predicate != bir::BinaryOpcode::Ne ||
      accepted.condition_home != &condition_home ||
      accepted.lhs_home != &lhs_home ||
      accepted.rhs_home != &rhs_home) {
    return fail("expected coherent fused pointer branch publication");
  }

  auto unsigned_relational = branch_condition;
  unsigned_relational.predicate = bir::BinaryOpcode::Ult;
  const auto accepted_relational =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &unsigned_relational,
          .terminator = &terminator,
          .condition_home = &condition_home,
          .lhs_home = &lhs_home,
          .rhs_home = &rhs_home,
      });
  if (!prepare::prepared_fused_pointer_branch_publication_available(
          accepted_relational) ||
      accepted_relational.predicate != bir::BinaryOpcode::Ult) {
    return fail("expected unsigned pointer relational branch publication");
  }

  auto signed_relational = branch_condition;
  signed_relational.predicate = bir::BinaryOpcode::Slt;
  const auto unsupported_predicate =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &signed_relational,
          .terminator = &terminator,
          .condition_home = &condition_home,
          .lhs_home = &lhs_home,
          .rhs_home = &rhs_home,
      });
  if (unsupported_predicate.status !=
      prepare::PreparedFusedPointerBranchPublicationStatus::UnsupportedPredicate) {
    return fail("expected signed pointer relational branch predicate to stay fail-closed");
  }

  auto mismatched_condition = terminator;
  mismatched_condition.condition = bir::Value::named(bir::TypeKind::I32, "%other");
  const auto condition_mismatch =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &mismatched_condition,
          .condition_home = &condition_home,
          .lhs_home = &lhs_home,
          .rhs_home = &rhs_home,
      });
  if (condition_mismatch.status !=
      prepare::PreparedFusedPointerBranchPublicationStatus::ConditionMismatch) {
    return fail("expected condition-mismatched pointer branch to stay fail-closed");
  }

  auto target_mismatch = terminator;
  target_mismatch.false_label = "other";
  const auto mismatched_target =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &target_mismatch,
          .condition_home = &condition_home,
          .lhs_home = &lhs_home,
          .rhs_home = &rhs_home,
      });
  if (mismatched_target.status !=
      prepare::PreparedFusedPointerBranchPublicationStatus::TargetMismatch) {
    return fail("expected target-mismatched pointer branch to stay fail-closed");
  }

  const auto missing_lhs =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .condition_home = &condition_home,
          .rhs_home = &rhs_home,
      });
  if (missing_lhs.status !=
      prepare::PreparedFusedPointerBranchPublicationStatus::MissingLhsHome) {
    return fail("expected missing pointer operand home to stay fail-closed");
  }

  auto stack_lhs = lhs_home;
  stack_lhs.kind = prepare::PreparedValueHomeKind::StackSlot;
  stack_lhs.register_name = std::nullopt;
  const auto unsupported_home =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .condition_home = &condition_home,
          .lhs_home = &stack_lhs,
          .rhs_home = &rhs_home,
      });
  if (unsupported_home.status !=
      prepare::PreparedFusedPointerBranchPublicationStatus::UnsupportedOperandHome) {
    return fail("expected stack-home pointer branch operand to stay fail-closed");
  }

  auto nonnull_immediate_rhs = branch_condition;
  nonnull_immediate_rhs.rhs = bir::Value{
      .kind = bir::Value::Kind::Immediate,
      .type = bir::TypeKind::Ptr,
      .immediate = 8,
      .immediate_bits = 8,
  };
  const auto unsupported_operand =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &nonnull_immediate_rhs,
          .terminator = &terminator,
          .condition_home = &condition_home,
          .lhs_home = &lhs_home,
      });
  if (unsupported_operand.status !=
      prepare::PreparedFusedPointerBranchPublicationStatus::UnsupportedOperand) {
    return fail("expected non-null pointer immediate branch operand to stay fail-closed");
  }

  auto integer_compare = branch_condition;
  integer_compare.compare_type = bir::TypeKind::I32;
  integer_compare.lhs = bir::Value::named(bir::TypeKind::I32, "%lhs");
  integer_compare.rhs = bir::Value::named(bir::TypeKind::I32, "%rhs");
  const auto unsupported_compare_type =
      prepare::plan_prepared_fused_pointer_branch_publication({
          .names = &names,
          .branch_condition = &integer_compare,
          .terminator = &terminator,
          .condition_home = &condition_home,
          .lhs_home = &lhs_home,
          .rhs_home = &rhs_home,
      });
  if (unsupported_compare_type.status !=
      prepare::PreparedFusedPointerBranchPublicationStatus::UnsupportedCompareType) {
    return fail("expected non-pointer fused compare to stay outside pointer branch publication");
  }

  return 0;
}

int check_branch_stack_load_authority_contract() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("branch_stack_load");
  const auto entry_label = names.block_labels.intern("entry");
  const auto true_label = names.block_labels.intern("is_true");
  const auto false_label = names.block_labels.intern("is_false");
  const auto condition_name = names.value_names.intern("%cmp");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto rhs_name = names.value_names.intern("%rhs");
  const auto other_name = names.value_names.intern("%other");

  bir::Terminator terminator;
  terminator.kind = bir::TerminatorKind::CondBranch;
  terminator.condition = bir::Value::named(bir::TypeKind::I32, "%cmp");
  terminator.true_label = "is_true";
  terminator.false_label = "is_false";

  const prepare::PreparedBranchCondition branch_condition{
      .function_name = function_name,
      .block_label = entry_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = bir::Value::named(bir::TypeKind::I32, "%cmp"),
      .predicate = bir::BinaryOpcode::Ult,
      .compare_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "%lhs"),
      .rhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs"),
      .can_fuse_with_branch = true,
      .true_label = true_label,
      .false_label = false_label,
  };
  const prepare::PreparedValueHome condition_home{
      .value_id = 1,
      .function_name = function_name,
      .value_name = condition_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{11},
      .offset_bytes = std::size_t{88},
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
  };
  const prepare::PreparedStackObject condition_object{
      .object_id = 11,
      .function_name = function_name,
      .value_name = condition_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  };
  const prepare::PreparedFrameSlot condition_frame_slot{
      .slot_id = prepare::PreparedFrameSlotId{11},
      .object_id = 11,
      .function_name = function_name,
      .offset_bytes = 88,
      .size_bytes = 4,
      .align_bytes = 4,
  };
  const prepare::PreparedValueHome lhs_home{
      .value_id = 2,
      .function_name = function_name,
      .value_name = lhs_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{10},
      .offset_bytes = std::size_t{80},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
  };
  const prepare::PreparedStackObject lhs_object{
      .object_id = 10,
      .function_name = function_name,
      .value_name = lhs_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  };
  const prepare::PreparedFrameSlot lhs_frame_slot{
      .slot_id = prepare::PreparedFrameSlotId{10},
      .object_id = 10,
      .function_name = function_name,
      .offset_bytes = 80,
      .size_bytes = 8,
      .align_bytes = 8,
  };

  const auto accepted_condition =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &condition_home,
          .frame_slot = &condition_frame_slot,
          .stack_object = &condition_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (!prepare::prepared_branch_stack_load_authority_available(
          accepted_condition) ||
      accepted_condition.role !=
          prepare::PreparedBranchStackLoadRole::Condition ||
      accepted_condition.policy !=
          prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot ||
      accepted_condition.value_id != condition_home.value_id ||
      accepted_condition.value_name != condition_name ||
      accepted_condition.stack_object_id !=
          std::optional<prepare::PreparedObjectId>{condition_object.object_id} ||
      accepted_condition.value_type != bir::TypeKind::I32 ||
      accepted_condition.slot_id != condition_home.slot_id ||
      accepted_condition.stack_offset_bytes != condition_home.offset_bytes ||
      accepted_condition.stack_size_bytes != condition_home.size_bytes ||
      accepted_condition.stack_align_bytes != condition_home.align_bytes) {
    return fail("expected explicit branch stack-load authority for scalar condition");
  }

  const auto missing_policy =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &condition_home,
          .frame_slot = &condition_frame_slot,
          .stack_object = &condition_object,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (missing_policy.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::MissingPolicy) {
    return fail("expected missing branch stack-load policy to stay fail-closed");
  }

  const auto missing_freshness =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &condition_home,
          .frame_slot = &condition_frame_slot,
          .stack_object = &condition_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (missing_freshness.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::MissingStackFreshness) {
    return fail("expected branch stack load without freshness to stay fail-closed");
  }

  const auto missing_clobber =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &condition_home,
          .frame_slot = &condition_frame_slot,
          .stack_object = &condition_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_fresh_at_branch = true,
      });
  if (missing_clobber.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::
          MissingStackClobberSafety) {
    return fail("expected branch stack load without clobber safety to stay fail-closed");
  }

  auto mismatched_home = condition_home;
  mismatched_home.value_name = other_name;
  const auto home_mismatch =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &mismatched_home,
          .frame_slot = &condition_frame_slot,
          .stack_object = &condition_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (home_mismatch.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::HomeValueMismatch) {
    return fail("expected wrong branch stack-load home to stay fail-closed");
  }

  const auto missing_frame_slot =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &condition_home,
          .stack_object = &condition_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (missing_frame_slot.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::MissingFrameSlot) {
    return fail("expected branch stack load without frame slot to stay fail-closed");
  }

  auto wrong_frame_slot = condition_frame_slot;
  wrong_frame_slot.object_id = 99;
  const auto frame_slot_mismatch =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &condition_home,
          .frame_slot = &wrong_frame_slot,
          .stack_object = &condition_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (frame_slot_mismatch.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::FrameSlotMismatch) {
    return fail("expected mismatched branch stack-load frame slot to stay fail-closed");
  }

  auto wrong_offset_frame_slot = condition_frame_slot;
  wrong_offset_frame_slot.offset_bytes = 96;
  const auto frame_slot_offset_mismatch =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &condition_home,
          .frame_slot = &wrong_offset_frame_slot,
          .stack_object = &condition_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (frame_slot_offset_mismatch.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::FrameSlotMismatch) {
    return fail("expected mismatched branch stack-load frame offset to stay fail-closed");
  }

  auto wrong_object = condition_object;
  wrong_object.value_name = other_name;
  const auto object_mismatch =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Condition,
          .value_home = &condition_home,
          .frame_slot = &condition_frame_slot,
          .stack_object = &wrong_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status =
              prepare::PreparedBranchStackLoadPointerStatus::NotPointer,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (object_mismatch.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::StackObjectMismatch) {
    return fail("expected wrong branch stack-load object to stay fail-closed");
  }

  const auto pointer_unknown =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Lhs,
          .value_home = &lhs_home,
          .frame_slot = &lhs_frame_slot,
          .stack_object = &lhs_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (pointer_unknown.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::PointerStatusUnknown) {
    return fail("expected pointer branch stack load without pointer status to stay fail-closed");
  }

  const auto accepted_pointer =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Lhs,
          .value_home = &lhs_home,
          .frame_slot = &lhs_frame_slot,
          .stack_object = &lhs_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status = prepare::PreparedBranchStackLoadPointerStatus::Proven,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (!prepare::prepared_branch_stack_load_authority_available(
          accepted_pointer) ||
      accepted_pointer.role != prepare::PreparedBranchStackLoadRole::Lhs ||
      accepted_pointer.value_type != bir::TypeKind::Ptr) {
    return fail("expected proven pointer branch stack-load authority");
  }

  const prepare::PreparedValueHome rhs_register_home{
      .value_id = 3,
      .function_name = function_name,
      .value_name = rhs_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"a4"},
  };
  const auto register_home =
      prepare::plan_prepared_branch_stack_load_authority({
          .names = &names,
          .branch_condition = &branch_condition,
          .terminator = &terminator,
          .role = prepare::PreparedBranchStackLoadRole::Rhs,
          .value_home = &rhs_register_home,
          .frame_slot = &lhs_frame_slot,
          .stack_object = &lhs_object,
          .policy = prepare::PreparedBranchStackLoadPolicy::LoadFromStackSlot,
          .pointer_status = prepare::PreparedBranchStackLoadPointerStatus::Proven,
          .stack_slot_fresh_at_branch = true,
          .stack_slot_clobber_safe_at_branch = true,
      });
  if (register_home.status !=
      prepare::PreparedBranchStackLoadAuthorityStatus::UnsupportedHome) {
    return fail("expected register-home branch value to stay outside stack-load authority");
  }

  prepare::PreparedBirModule prepared;
  prepared.target_profile = riscv_target_profile();
  const auto prepared_function_name =
      prepared.names.function_names.intern("branch_stack_load_collector");
  const auto prepared_entry_label = prepared.names.block_labels.intern("entry");
  const auto prepared_true_label = prepared.names.block_labels.intern("is_true");
  const auto prepared_false_label = prepared.names.block_labels.intern("is_false");
  const auto prepared_condition_name = prepared.names.value_names.intern("%cmp");
  const auto prepared_lhs_name = prepared.names.value_names.intern("%lhs");
  const auto prepared_rhs_name = prepared.names.value_names.intern("%rhs");

  const bir::Value prepared_condition =
      bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value prepared_lhs =
      bir::Value::named(bir::TypeKind::Ptr, "%lhs");
  const bir::Value prepared_rhs =
      bir::Value::named(bir::TypeKind::Ptr, "%rhs");

  bir::Function prepared_function;
  prepared_function.name = "branch_stack_load_collector";
  bir::Block prepared_entry;
  prepared_entry.label = "entry";
  prepared_entry.label_id = c4c::kInvalidBlockLabel;
  prepared_entry.terminator = bir::CondBranchTerminator{
      .condition = prepared_condition,
      .true_label = "is_true",
      .false_label = "is_false",
      .true_label_id = prepared_true_label,
      .false_label_id = prepared_false_label,
  };
  bir::Block prepared_true;
  prepared_true.label = "is_true";
  prepared_true.label_id = prepared_true_label;
  prepared_true.terminator = bir::ReturnTerminator{.value = prepared_condition};
  bir::Block prepared_false;
  prepared_false.label = "is_false";
  prepared_false.label_id = prepared_false_label;
  prepared_false.terminator = bir::ReturnTerminator{.value = prepared_condition};
  prepared_function.blocks.push_back(std::move(prepared_entry));
  prepared_function.blocks.push_back(std::move(prepared_true));
  prepared_function.blocks.push_back(std::move(prepared_false));
  prepared.module.functions.push_back(std::move(prepared_function));

  prepare::PreparedControlFlowFunction prepared_cf;
  prepared_cf.function_name = prepared_function_name;
  prepared_cf.blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = prepared_entry_label,
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = prepared_true_label,
      .false_label = prepared_false_label,
  });
  prepared_cf.branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = prepared_function_name,
      .block_label = prepared_entry_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = prepared_condition,
      .predicate = bir::BinaryOpcode::Ult,
      .compare_type = bir::TypeKind::Ptr,
      .lhs = prepared_lhs,
      .rhs = prepared_rhs,
      .can_fuse_with_branch = true,
      .true_label = prepared_true_label,
      .false_label = prepared_false_label,
  });
  prepared.control_flow.functions.push_back(std::move(prepared_cf));

  prepare::PreparedValueLocationFunction prepared_locations;
  prepared_locations.function_name = prepared_function_name;
  prepared_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 7,
      .function_name = prepared_function_name,
      .value_name = prepared_condition_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{11},
      .offset_bytes = std::size_t{88},
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
  });
  prepared_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 6,
      .function_name = prepared_function_name,
      .value_name = prepared_lhs_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{10},
      .offset_bytes = std::size_t{80},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
  });
  prepared_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 4,
      .function_name = prepared_function_name,
      .value_name = prepared_rhs_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"a4"},
  });
  prepared.value_locations.functions.push_back(std::move(prepared_locations));
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 11,
      .function_name = prepared_function_name,
      .value_name = prepared_condition_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 10,
      .function_name = prepared_function_name,
      .value_name = prepared_lhs_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{11},
      .object_id = 11,
      .function_name = prepared_function_name,
      .offset_bytes = 88,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{10},
      .object_id = 10,
      .function_name = prepared_function_name,
      .offset_bytes = 80,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  const auto records =
      prepare::collect_prepared_branch_stack_load_authorities(prepared);
  if (records.records.size() != 2) {
    return fail("expected stack-home branch collector to emit condition and lhs rows");
  }
  const auto* condition_record = &records.records[0];
  const auto* lhs_record = &records.records[1];
  if (condition_record->role !=
          prepare::PreparedBranchStackLoadRole::Condition ||
      condition_record->authority.status !=
          prepare::PreparedBranchStackLoadAuthorityStatus::MissingPolicy ||
      condition_record->authority.pointer_status !=
          prepare::PreparedBranchStackLoadPointerStatus::NotPointer ||
      condition_record->authority.value_id != 7 ||
      condition_record->authority.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{11}} ||
      condition_record->authority.stack_object_id !=
          std::optional<prepare::PreparedObjectId>{11}) {
    return fail("expected collected branch condition stack-load row to expose missing policy");
  }
  if (lhs_record->role != prepare::PreparedBranchStackLoadRole::Lhs ||
      lhs_record->authority.status !=
          prepare::PreparedBranchStackLoadAuthorityStatus::MissingPolicy ||
      lhs_record->authority.pointer_status !=
          prepare::PreparedBranchStackLoadPointerStatus::Unknown ||
      lhs_record->authority.value_id != 6 ||
      lhs_record->authority.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{10}} ||
      lhs_record->authority.stack_object_id !=
          std::optional<prepare::PreparedObjectId>{10}) {
    return fail("expected collected branch lhs stack-load row to preserve pointer boundary");
  }

  const std::string dump = prepare::print(prepared);
  if (dump.find("--- prepared-branch-stack-load-authorities ---") ==
      std::string::npos) {
    return fail("expected prepared dump to expose branch stack-load authority section");
  }
  if (dump.find("branch_stack_load_authority "
                "function=branch_stack_load_collector block=entry "
                "role=condition value=%cmp value_id=7 policy=none "
                "pointer_status=not_pointer status=missing_policy slot=#11 "
                "object=#11 stack_offset=88 size=4 align=4") ==
      std::string::npos) {
    return fail("expected prepared dump to expose condition stack-load row");
  }
  if (dump.find("branch_stack_load_authority "
                "function=branch_stack_load_collector block=entry "
                "role=lhs value=%lhs value_id=6 policy=none "
                "pointer_status=unknown status=missing_policy slot=#10 "
                "object=#10 stack_offset=80 size=8 align=8") ==
      std::string::npos) {
    return fail("expected prepared dump to expose lhs stack-load row");
  }

  return 0;
}

int check_frame_slot_source_fact_carrier_contract() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("frame_slot_source");
  const auto entry_label = names.block_labels.intern("entry");
  const auto target_name = names.value_names.intern("%cmp");
  const auto other_name = names.value_names.intern("%other");
  const bir::Value target_value = bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value other_value = bir::Value::named(bir::TypeKind::I32, "%other");

  const prepare::PreparedValueHome target_home{
      .value_id = 17,
      .function_name = function_name,
      .value_name = target_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{21},
      .offset_bytes = std::size_t{156},
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
  };
  const prepare::PreparedFrameSlot target_frame_slot{
      .slot_id = prepare::PreparedFrameSlotId{21},
      .object_id = 21,
      .function_name = function_name,
      .offset_bytes = 156,
      .size_bytes = 4,
      .align_bytes = 4,
  };
  const prepare::PreparedStackObject target_object{
      .object_id = 21,
      .function_name = function_name,
      .value_name = target_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  };

  prepare::PreparedFrameSlotSourceFactInputs accepted_inputs{
      .names = &names,
      .target_value = &target_value,
      .target_home = &target_home,
      .target_frame_slot = &target_frame_slot,
      .target_stack_object = &target_object,
      .source_value = &target_value,
      .source_producer_kind =
          prepare::PreparedEdgePublicationSourceProducerKind::Binary,
      .source_producer_block_label = entry_label,
      .source_producer_instruction_index = std::size_t{0},
      .materialization_kind =
          prepare::PreparedFrameSlotSourceFactMaterializationKind::ExplicitWrite,
      .materialization_source_value = &target_value,
      .materialization_frame_slot = &target_frame_slot,
      .materialization_stack_object = &target_object,
      .materialization_block_label = entry_label,
      .materialization_instruction_index = std::size_t{1},
      .path_validity_known = true,
      .path_covers_consumer = true,
      .same_slot_writes_classified = true,
      .same_slot_write_found = false,
      .call_or_helper_effects_classified_safe = true,
      .call_or_helper_clobbers_slot = false,
      .publication_effects_classified_non_clobber = true,
      .publication_clobbers_slot = false,
      .move_bundle_effects_classified_non_clobber = true,
      .move_bundle_clobbers_slot = false,
      .parallel_copy_effects_classified_non_clobber = true,
      .parallel_copy_clobbers_slot = false,
  };
  const auto accepted =
      prepare::plan_prepared_frame_slot_source_fact(accepted_inputs);
  if (!prepare::prepared_frame_slot_source_fact_available(accepted) ||
      accepted.status != prepare::PreparedFrameSlotSourceFactStatus::Available ||
      accepted.target_value_id != target_home.value_id ||
      accepted.target_value_name != target_name ||
      accepted.source_value_id != std::optional<prepare::PreparedValueId>{17} ||
      accepted.source_value_name != target_name ||
      accepted.slot_id != target_home.slot_id ||
      accepted.stack_object_id !=
          std::optional<prepare::PreparedObjectId>{target_object.object_id} ||
      accepted.stack_offset_bytes != target_home.offset_bytes ||
      accepted.materialization_kind !=
          prepare::PreparedFrameSlotSourceFactMaterializationKind::ExplicitWrite) {
    return fail("expected explicit frame-slot source fact to be available");
  }

  auto missing_materialization_inputs = accepted_inputs;
  missing_materialization_inputs.materialization_kind =
      prepare::PreparedFrameSlotSourceFactMaterializationKind::None;
  missing_materialization_inputs.materialization_source_value = nullptr;
  missing_materialization_inputs.materialization_frame_slot = nullptr;
  missing_materialization_inputs.materialization_stack_object = nullptr;
  const auto missing_materialization =
      prepare::plan_prepared_frame_slot_source_fact(missing_materialization_inputs);
  if (missing_materialization.status !=
      prepare::PreparedFrameSlotSourceFactStatus::MissingMaterializationEvent) {
    return fail("expected missing materialization event to stay fail-closed");
  }

  auto wrong_slot = target_frame_slot;
  wrong_slot.slot_id = prepare::PreparedFrameSlotId{22};
  auto slot_mismatch_inputs = accepted_inputs;
  slot_mismatch_inputs.materialization_frame_slot = &wrong_slot;
  const auto slot_mismatch =
      prepare::plan_prepared_frame_slot_source_fact(slot_mismatch_inputs);
  if (slot_mismatch.status !=
      prepare::PreparedFrameSlotSourceFactStatus::MaterializationSlotMismatch) {
    return fail("expected materialization slot mismatch to stay fail-closed");
  }

  auto value_mismatch_inputs = accepted_inputs;
  value_mismatch_inputs.materialization_source_value = &other_value;
  const auto value_mismatch =
      prepare::plan_prepared_frame_slot_source_fact(value_mismatch_inputs);
  if (value_mismatch.status !=
      prepare::PreparedFrameSlotSourceFactStatus::MaterializationValueMismatch) {
    return fail("expected materialization value mismatch to stay fail-closed");
  }

  auto missing_path_inputs = accepted_inputs;
  missing_path_inputs.path_validity_known = false;
  const auto missing_path =
      prepare::plan_prepared_frame_slot_source_fact(missing_path_inputs);
  if (missing_path.status !=
      prepare::PreparedFrameSlotSourceFactStatus::MissingPathValidity) {
    return fail("expected missing path validity to stay fail-closed");
  }

  auto same_slot_write_inputs = accepted_inputs;
  same_slot_write_inputs.same_slot_write_found = true;
  const auto same_slot_write =
      prepare::plan_prepared_frame_slot_source_fact(same_slot_write_inputs);
  if (same_slot_write.status !=
      prepare::PreparedFrameSlotSourceFactStatus::SameSlotWriteFound) {
    return fail("expected same-slot write to stay fail-closed");
  }

  auto call_unknown_inputs = accepted_inputs;
  call_unknown_inputs.call_or_helper_effects_classified_safe = false;
  const auto call_unknown =
      prepare::plan_prepared_frame_slot_source_fact(call_unknown_inputs);
  if (call_unknown.status !=
      prepare::PreparedFrameSlotSourceFactStatus::CallOrHelperEffectUnknown) {
    return fail("expected unknown call/helper effect to stay fail-closed");
  }

  auto unsupported_inputs = accepted_inputs;
  unsupported_inputs.unsupported_boundary = true;
  const auto unsupported =
      prepare::plan_prepared_frame_slot_source_fact(unsupported_inputs);
  if (unsupported.status !=
      prepare::PreparedFrameSlotSourceFactStatus::UnsupportedBoundary) {
    return fail("expected unsupported source-fact boundary to stay separate");
  }

  prepare::PreparedBirModule prepared;
  prepared.target_profile = riscv_target_profile();
  const auto prepared_function_name =
      prepared.names.function_names.intern("frame_slot_source_collector");
  const auto prepared_entry_label = prepared.names.block_labels.intern("entry");
  const auto prepared_true_label = prepared.names.block_labels.intern("is_true");
  const auto prepared_false_label = prepared.names.block_labels.intern("is_false");
  const auto prepared_target_name = prepared.names.value_names.intern("%cmp");
  const bir::Value prepared_condition =
      bir::Value::named(bir::TypeKind::I32, "%cmp");

  prepare::PreparedControlFlowFunction prepared_cf;
  prepared_cf.function_name = prepared_function_name;
  prepared_cf.branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = prepared_function_name,
      .block_label = prepared_entry_label,
      .kind = prepare::PreparedBranchConditionKind::MaterializedBool,
      .condition_value = prepared_condition,
      .true_label = prepared_true_label,
      .false_label = prepared_false_label,
  });
  prepared.control_flow.functions.push_back(std::move(prepared_cf));

  bir::Function prepared_function;
  prepared_function.name = "frame_slot_source_collector";
  bir::Block prepared_entry;
  prepared_entry.label = "entry";
  prepared_entry.label_id = prepared_entry_label;
  prepared_entry.terminator = bir::CondBranchTerminator{
      .condition = prepared_condition,
      .true_label = "is_true",
      .false_label = "is_false",
      .true_label_id = prepared_true_label,
      .false_label_id = prepared_false_label,
  };
  prepared_function.blocks.push_back(std::move(prepared_entry));
  prepared.module.functions.push_back(std::move(prepared_function));

  prepare::PreparedValueLocationFunction prepared_locations;
  prepared_locations.function_name = prepared_function_name;
  prepared_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 17,
      .function_name = prepared_function_name,
      .value_name = prepared_target_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{21},
      .offset_bytes = std::size_t{156},
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
  });
  prepared.value_locations.functions.push_back(std::move(prepared_locations));
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 21,
      .function_name = prepared_function_name,
      .value_name = prepared_target_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{21},
      .object_id = 21,
      .function_name = prepared_function_name,
      .offset_bytes = 156,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  const auto records = prepare::collect_prepared_frame_slot_source_facts(prepared);
  if (records.records.size() != 1) {
    return fail("expected frame-slot source-fact collector to emit the stack-home row");
  }
  const auto& record = records.records.front();
  if (record.function_name != prepared_function_name ||
      record.consumer_block_label != prepared_entry_label ||
      record.role != prepare::PreparedBranchStackLoadRole::Condition ||
      record.fact.status !=
          prepare::PreparedFrameSlotSourceFactStatus::MissingMaterializationEvent ||
      record.fact.target_value_id != 17 ||
      record.fact.target_value_name != prepared_target_name ||
      record.fact.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{
              prepare::PreparedFrameSlotId{21}} ||
      record.fact.stack_object_id !=
          std::optional<prepare::PreparedObjectId>{21}) {
    return fail("expected collected source-fact row to expose missing materialization");
  }

  prepare::PreparedBirModule select_prepared;
  select_prepared.target_profile = riscv_target_profile();
  const auto select_function_name =
      select_prepared.names.function_names.intern("frame_slot_select_boundary");
  const auto select_entry_label = select_prepared.names.block_labels.intern("entry");
  const auto select_true_label = select_prepared.names.block_labels.intern("is_true");
  const auto select_false_label = select_prepared.names.block_labels.intern("is_false");
  const auto select_condition_name = select_prepared.names.value_names.intern("%cmp");
  const auto select_result_name = select_prepared.names.value_names.intern("%sel");
  const bir::Value select_condition =
      bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value select_result =
      bir::Value::named(bir::TypeKind::I32, "%sel");
  prepare::PreparedControlFlowFunction select_cf;
  select_cf.function_name = select_function_name;
  select_cf.branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = select_function_name,
      .block_label = select_entry_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = select_condition,
      .predicate = bir::BinaryOpcode::Ne,
      .compare_type = bir::TypeKind::I32,
      .lhs = select_result,
      .rhs = bir::Value::immediate_i32(0),
      .can_fuse_with_branch = true,
      .true_label = select_true_label,
      .false_label = select_false_label,
  });
  select_cf.join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = select_function_name,
      .join_block_label = select_entry_label,
      .result = select_result,
      .kind = prepare::PreparedJoinTransferKind::PhiEdge,
      .carrier_kind =
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
  });
  select_prepared.control_flow.functions.push_back(std::move(select_cf));
  bir::Function select_function;
  select_function.name = "frame_slot_select_boundary";
  bir::Block select_entry;
  select_entry.label = "entry";
  select_entry.label_id = select_entry_label;
  select_entry.terminator = bir::CondBranchTerminator{
      .condition = select_condition,
      .true_label = "is_true",
      .false_label = "is_false",
      .true_label_id = select_true_label,
      .false_label_id = select_false_label,
  };
  select_function.blocks.push_back(std::move(select_entry));
  select_prepared.module.functions.push_back(std::move(select_function));
  prepare::PreparedValueLocationFunction select_locations;
  select_locations.function_name = select_function_name;
  select_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 30,
      .function_name = select_function_name,
      .value_name = select_condition_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });
  select_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 31,
      .function_name = select_function_name,
      .value_name = select_result_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{31},
      .offset_bytes = std::size_t{200},
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
  });
  select_prepared.value_locations.functions.push_back(std::move(select_locations));
  select_prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 31,
      .function_name = select_function_name,
      .value_name = select_result_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  select_prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{31},
      .object_id = 31,
      .function_name = select_function_name,
      .offset_bytes = 200,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  const auto select_records =
      prepare::collect_prepared_frame_slot_source_facts(select_prepared);
  if (select_records.records.size() != 1 ||
      select_records.records.front().role !=
          prepare::PreparedBranchStackLoadRole::Lhs ||
      select_records.records.front().fact.status !=
          prepare::PreparedFrameSlotSourceFactStatus::UnsupportedBoundary) {
    return fail("expected select-result stack destination to stay an unsupported source-fact boundary");
  }

  prepare::PreparedBirModule pointer_prepared;
  pointer_prepared.target_profile = riscv_target_profile();
  const auto pointer_function_name =
      pointer_prepared.names.function_names.intern("frame_slot_pointer_boundary");
  const auto pointer_entry_label = pointer_prepared.names.block_labels.intern("entry");
  const auto pointer_true_label = pointer_prepared.names.block_labels.intern("is_true");
  const auto pointer_false_label = pointer_prepared.names.block_labels.intern("is_false");
  const auto pointer_condition_name = pointer_prepared.names.value_names.intern("%cmp");
  const auto pointer_lhs_name = pointer_prepared.names.value_names.intern("%ptr");
  const bir::Value pointer_condition =
      bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value pointer_lhs = bir::Value::named(bir::TypeKind::Ptr, "%ptr");
  prepare::PreparedControlFlowFunction pointer_cf;
  pointer_cf.function_name = pointer_function_name;
  pointer_cf.branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = pointer_function_name,
      .block_label = pointer_entry_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = pointer_condition,
      .predicate = bir::BinaryOpcode::Ult,
      .compare_type = bir::TypeKind::Ptr,
      .lhs = pointer_lhs,
      .rhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs"),
      .can_fuse_with_branch = true,
      .true_label = pointer_true_label,
      .false_label = pointer_false_label,
  });
  pointer_prepared.control_flow.functions.push_back(std::move(pointer_cf));
  bir::Function pointer_function;
  pointer_function.name = "frame_slot_pointer_boundary";
  bir::Block pointer_entry;
  pointer_entry.label = "entry";
  pointer_entry.label_id = pointer_entry_label;
  pointer_entry.terminator = bir::CondBranchTerminator{
      .condition = pointer_condition,
      .true_label = "is_true",
      .false_label = "is_false",
      .true_label_id = pointer_true_label,
      .false_label_id = pointer_false_label,
  };
  pointer_function.blocks.push_back(std::move(pointer_entry));
  pointer_prepared.module.functions.push_back(std::move(pointer_function));
  prepare::PreparedValueLocationFunction pointer_locations;
  pointer_locations.function_name = pointer_function_name;
  pointer_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 40,
      .function_name = pointer_function_name,
      .value_name = pointer_condition_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });
  pointer_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 41,
      .function_name = pointer_function_name,
      .value_name = pointer_lhs_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{41},
      .offset_bytes = std::size_t{208},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
  });
  pointer_prepared.value_locations.functions.push_back(std::move(pointer_locations));
  pointer_prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 41,
      .function_name = pointer_function_name,
      .value_name = pointer_lhs_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  pointer_prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{41},
      .object_id = 41,
      .function_name = pointer_function_name,
      .offset_bytes = 208,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  const auto pointer_records =
      prepare::collect_prepared_frame_slot_source_facts(pointer_prepared);
  if (pointer_records.records.size() != 1 ||
      pointer_records.records.front().role !=
          prepare::PreparedBranchStackLoadRole::Lhs ||
      pointer_records.records.front().fact.status !=
          prepare::PreparedFrameSlotSourceFactStatus::UnsupportedBoundary) {
    return fail("expected pointer/provenance stack row to stay an unsupported source-fact boundary");
  }

  prepare::PreparedBirModule terminator_prepared;
  terminator_prepared.target_profile = riscv_target_profile();
  const auto terminator_function_name =
      terminator_prepared.names.function_names.intern("frame_slot_terminator_boundary");
  const auto terminator_entry_label =
      terminator_prepared.names.block_labels.intern("entry");
  const auto terminator_true_label =
      terminator_prepared.names.block_labels.intern("is_true");
  const auto terminator_false_label =
      terminator_prepared.names.block_labels.intern("is_false");
  const auto terminator_condition_name =
      terminator_prepared.names.value_names.intern("%cmp");
  const bir::Value terminator_condition =
      bir::Value::named(bir::TypeKind::I32, "%cmp");
  prepare::PreparedControlFlowFunction terminator_cf;
  terminator_cf.function_name = terminator_function_name;
  terminator_cf.branch_conditions.push_back(prepare::PreparedBranchCondition{
      .function_name = terminator_function_name,
      .block_label = terminator_entry_label,
      .kind = prepare::PreparedBranchConditionKind::MaterializedBool,
      .condition_value = terminator_condition,
      .true_label = terminator_true_label,
      .false_label = terminator_false_label,
  });
  terminator_prepared.control_flow.functions.push_back(std::move(terminator_cf));
  bir::Function terminator_function;
  terminator_function.name = "frame_slot_terminator_boundary";
  bir::Block terminator_entry;
  terminator_entry.label = "entry";
  terminator_entry.label_id = terminator_entry_label;
  terminator_entry.terminator = bir::ReturnTerminator{.value = terminator_condition};
  terminator_function.blocks.push_back(std::move(terminator_entry));
  terminator_prepared.module.functions.push_back(std::move(terminator_function));
  prepare::PreparedValueLocationFunction terminator_locations;
  terminator_locations.function_name = terminator_function_name;
  terminator_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 50,
      .function_name = terminator_function_name,
      .value_name = terminator_condition_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{50},
      .offset_bytes = std::size_t{216},
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
  });
  terminator_prepared.value_locations.functions.push_back(
      std::move(terminator_locations));
  terminator_prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 50,
      .function_name = terminator_function_name,
      .value_name = terminator_condition_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  terminator_prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = prepare::PreparedFrameSlotId{50},
      .object_id = 50,
      .function_name = terminator_function_name,
      .offset_bytes = 216,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  const auto terminator_records =
      prepare::collect_prepared_frame_slot_source_facts(terminator_prepared);
  if (terminator_records.records.size() != 1 ||
      terminator_records.records.front().fact.status !=
          prepare::PreparedFrameSlotSourceFactStatus::UnsupportedBoundary) {
    return fail("expected unsupported terminator stack row to stay an unsupported source-fact boundary");
  }

  return 0;
}

int check_dependency_operand_authority_contract() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("dependency_operand");
  const auto pred_label = names.block_labels.intern("pred");
  const auto succ_label = names.block_labels.intern("join");
  const auto cmp_name = names.value_names.intern("%cmp");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto dep_name = names.value_names.intern("%dep");
  const auto src_name = names.value_names.intern("%src");
  const auto selected_name = names.value_names.intern("%selected");

  const bir::Value lhs = bir::Value::named(bir::TypeKind::Ptr, "%lhs");
  const bir::Value dependency = bir::Value::named(bir::TypeKind::Ptr, "%dep");
  const bir::Value source = bir::Value::named(bir::TypeKind::I32, "%src");
  const bir::Value cmp = bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value selected = bir::Value::named(bir::TypeKind::I32, "%selected");

  const bir::BinaryInst compare{
      .opcode = bir::BinaryOpcode::Ule,
      .result = cmp,
      .operand_type = bir::TypeKind::Ptr,
      .lhs = lhs,
      .rhs = dependency,
  };
  const bir::CastInst cast{
      .opcode = bir::CastOpcode::IntToPtr,
      .result = dependency,
      .operand = source,
  };

  const prepare::PreparedEdgePublication publication{
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .predecessor_label = pred_label,
      .successor_label = succ_label,
      .destination_value = selected,
      .source_value = cmp,
      .destination_value_id = 11,
      .destination_value_name = selected_name,
      .source_value_id = 10,
      .source_value_name = cmp_name,
      .source_value_kind = bir::Value::Kind::Named,
      .source_producer_kind =
          prepare::PreparedEdgePublicationSourceProducerKind::Binary,
      .source_producer_block_label = succ_label,
      .source_producer_instruction_index = 3,
      .source_binary = &compare,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .phase = prepare::PreparedMovePhase::BlockEntry,
      .carrier_kind = prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
      .parallel_copy_execution_site =
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
      .parallel_copy_execution_block_label = pred_label,
  };
  const prepare::PreparedValueHome dependency_home{
      .value_id = 9,
      .function_name = function_name,
      .value_name = dep_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{2},
      .offset_bytes = std::size_t{16},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
  };
  const prepare::PreparedStackObject dependency_object{
      .object_id = 2,
      .function_name = function_name,
      .value_name = dep_name,
      .source_kind = "regalloc.spill_slot",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  };
  const prepare::PreparedEdgePublicationSourceProducer cast_producer{
      .kind = prepare::PreparedEdgePublicationSourceProducerKind::Cast,
      .block_label = succ_label,
      .instruction_index = 2,
      .cast = &cast,
  };
  const prepare::PreparedValueHome cast_source_home{
      .value_id = 8,
      .function_name = function_name,
      .value_name = src_name,
      .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
      .immediate_i32 = -2147483643,
  };

  const auto accepted_cast =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .cast_producer = &cast_producer,
          .cast_source_home = &cast_source_home,
          .policy = prepare::PreparedDependencyOperandMaterializationPolicy::
              RematerializeCastFromSource,
      });
  if (!prepare::prepared_dependency_operand_authority_available(accepted_cast) ||
      accepted_cast.policy !=
          prepare::PreparedDependencyOperandMaterializationPolicy::
              RematerializeCastFromSource ||
      accepted_cast.operand_role != prepare::PreparedDependencyOperandRole::Rhs ||
      accepted_cast.dependency_value_id != dependency_home.value_id ||
      accepted_cast.dependency_value_name != dep_name ||
      accepted_cast.dependency_slot_id != dependency_home.slot_id ||
      accepted_cast.cast_source_value_id != cast_source_home.value_id ||
      accepted_cast.cast_source_value_name != src_name ||
      accepted_cast.cast_source_type != bir::TypeKind::I32) {
    return fail("expected explicit cast dependency operand authority");
  }

  const auto missing_policy =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
      });
  if (missing_policy.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::MissingPolicy) {
    return fail("expected missing dependency operand policy to stay fail-closed");
  }

  const auto missing_cast_source =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .cast_producer = &cast_producer,
          .policy = prepare::PreparedDependencyOperandMaterializationPolicy::
              RematerializeCastFromSource,
      });
  if (missing_cast_source.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::MissingCastSourceHome) {
    return fail("expected missing cast source home to stay fail-closed");
  }

  auto stack_cast_source_home = cast_source_home;
  stack_cast_source_home.kind = prepare::PreparedValueHomeKind::StackSlot;
  stack_cast_source_home.immediate_i32 = std::nullopt;
  const auto unsupported_cast_source =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .cast_producer = &cast_producer,
          .cast_source_home = &stack_cast_source_home,
          .policy = prepare::PreparedDependencyOperandMaterializationPolicy::
              RematerializeCastFromSource,
      });
  if (unsupported_cast_source.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::
          UnsupportedCastSourceHome) {
    return fail("expected stack-home cast source to stay fail-closed");
  }

  const bir::CastInst unsupported_width_cast{
      .opcode = bir::CastOpcode::IntToPtr,
      .result = dependency,
      .operand = bir::Value::named(bir::TypeKind::I16, "%src"),
  };
  const prepare::PreparedEdgePublicationSourceProducer unsupported_width_producer{
      .kind = prepare::PreparedEdgePublicationSourceProducerKind::Cast,
      .block_label = succ_label,
      .instruction_index = 2,
      .cast = &unsupported_width_cast,
  };
  const auto unsupported_width =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .cast_producer = &unsupported_width_producer,
          .cast_source_home = &cast_source_home,
          .policy = prepare::PreparedDependencyOperandMaterializationPolicy::
              RematerializeCastFromSource,
      });
  if (unsupported_width.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::UnsupportedCastWidth) {
    return fail("expected unsupported cast source width to stay fail-closed");
  }

  const auto missing_freshness =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .policy =
              prepare::PreparedDependencyOperandMaterializationPolicy::
                  LoadFromStackSlot,
      });
  if (missing_freshness.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::MissingStackFreshness) {
    return fail("expected stack load without freshness to stay fail-closed");
  }

  const auto missing_clobber_safety =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .policy =
              prepare::PreparedDependencyOperandMaterializationPolicy::
                  LoadFromStackSlot,
          .stack_slot_fresh_at_edge = true,
      });
  if (missing_clobber_safety.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::
          MissingStackClobberSafety) {
    return fail("expected stack load without clobber safety to stay fail-closed");
  }

  const auto accepted_load =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .policy =
              prepare::PreparedDependencyOperandMaterializationPolicy::
                  LoadFromStackSlot,
          .stack_slot_fresh_at_edge = true,
          .stack_slot_clobber_safe_at_edge = true,
      });
  if (!prepare::prepared_dependency_operand_authority_available(accepted_load) ||
      accepted_load.policy !=
          prepare::PreparedDependencyOperandMaterializationPolicy::
              LoadFromStackSlot) {
    return fail("expected explicit stack-load dependency operand authority");
  }

  auto mismatched_home = dependency_home;
  mismatched_home.value_name = lhs_name;
  const auto home_mismatch =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &mismatched_home,
          .dependency_stack_object = &dependency_object,
          .policy = prepare::PreparedDependencyOperandMaterializationPolicy::
              RematerializeCastFromSource,
      });
  if (home_mismatch.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::HomeValueMismatch) {
    return fail("expected mismatched dependency home to stay fail-closed");
  }

  const auto operand_mismatch =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &lhs,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .policy = prepare::PreparedDependencyOperandMaterializationPolicy::
              RematerializeCastFromSource,
      });
  if (operand_mismatch.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::OperandMismatch) {
    return fail("expected wrong dependency operand role to stay fail-closed");
  }

  auto unavailable_publication = publication;
  unavailable_publication.status =
      prepare::PreparedEdgePublicationLookupStatus::MissingDestinationValue;
  const auto unsupported_publication =
      prepare::plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &unavailable_publication,
          .dependency_operand = &dependency,
          .operand_role = prepare::PreparedDependencyOperandRole::Rhs,
          .dependency_home = &dependency_home,
          .dependency_stack_object = &dependency_object,
          .policy = prepare::PreparedDependencyOperandMaterializationPolicy::
              RematerializeCastFromSource,
      });
  if (unsupported_publication.status !=
      prepare::PreparedDependencyOperandAuthorityStatus::UnsupportedPublication) {
    return fail("expected unavailable edge publication to stay fail-closed");
  }

  return 0;
}

int check_select_edge_source_producer_placement_contract() {
  prepare::PreparedNameTables names;
  const auto pred_label = names.block_labels.intern("pred");
  const auto join_label = names.block_labels.intern("join");
  const auto other_label = names.block_labels.intern("other");
  const auto cmp_name = names.value_names.intern("%cmp");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto rhs_name = names.value_names.intern("%rhs");
  const auto selected_name = names.value_names.intern("%selected");

  const bir::Value lhs = bir::Value::named(bir::TypeKind::Ptr, "%lhs");
  const bir::Value rhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs");
  const bir::Value cmp = bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value selected = bir::Value::named(bir::TypeKind::I32, "%selected");
  const bir::BinaryInst compare{
      .opcode = bir::BinaryOpcode::Ule,
      .result = cmp,
      .operand_type = bir::TypeKind::Ptr,
      .lhs = lhs,
      .rhs = rhs,
  };

  const prepare::PreparedEdgePublication publication{
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .predecessor_label = pred_label,
      .successor_label = join_label,
      .destination_value = selected,
      .source_value = cmp,
      .destination_value_id = 11,
      .destination_value_name = selected_name,
      .source_value_id = 10,
      .source_value_name = cmp_name,
      .source_value_kind = bir::Value::Kind::Named,
      .source_producer_kind =
          prepare::PreparedEdgePublicationSourceProducerKind::Binary,
      .source_producer_block_label = join_label,
      .source_producer_instruction_index = 2,
      .source_binary = &compare,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .phase = prepare::PreparedMovePhase::BlockEntry,
      .carrier_kind =
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
      .parallel_copy_execution_site =
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
      .parallel_copy_execution_block_label = pred_label,
  };
  prepare::PreparedMoveBundle bundle{
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .authority_kind = prepare::PreparedMoveAuthorityKind::None,
      .block_index = 4,
      .instruction_index = 2,
  };
  bundle.moves.push_back(prepare::PreparedMoveResolution{
      .from_value_id = 7,
      .to_value_id = 10,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = std::string{"t0"},
      .block_index = 4,
      .instruction_index = 2,
      .authority_kind = prepare::PreparedMoveAuthorityKind::None,
      .reason = "consumer_register_to_register",
  });
  bundle.moves.push_back(prepare::PreparedMoveResolution{
      .from_value_id = 9,
      .to_value_id = 10,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = std::string{"t0"},
      .block_index = 4,
      .instruction_index = 2,
      .authority_kind = prepare::PreparedMoveAuthorityKind::None,
      .reason = "consumer_stack_to_register",
  });

  const auto accepted =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &publication,
          .move_bundle = &bundle,
          .placement_kind =
              prepare::PreparedSelectEdgeSourceProducerPlacementKind::
                  PredecessorEdgeConsumedSuppression,
          .move_bundle_block_label = join_label,
      });
  if (!prepare::prepared_select_edge_source_producer_placement_available(
          accepted) ||
      accepted.placement_kind !=
          prepare::PreparedSelectEdgeSourceProducerPlacementKind::
              PredecessorEdgeConsumedSuppression ||
      accepted.predecessor_label != pred_label ||
      accepted.successor_label != join_label ||
      accepted.source_value_id != std::optional<prepare::PreparedValueId>{10} ||
      accepted.destination_value_id != 11 ||
      accepted.source_producer_block_label !=
          std::optional<c4c::BlockLabelId>{join_label} ||
      accepted.source_producer_instruction_index != std::optional<std::size_t>{2} ||
      accepted.move_bundle_block_label !=
          std::optional<c4c::BlockLabelId>{join_label} ||
      accepted.move_bundle_instruction_index != 2 ||
      accepted.move_count != 2) {
    return fail("expected explicit predecessor-edge consumed suppression authority");
  }

  const auto missing_kind =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &publication,
          .move_bundle = &bundle,
          .move_bundle_block_label = join_label,
      });
  if (missing_kind.status !=
      prepare::PreparedSelectEdgeSourceProducerPlacementStatus::
          UnsupportedPlacementKind) {
    return fail("expected missing placement kind to stay fail-closed");
  }

  auto non_binary_publication = publication;
  non_binary_publication.source_producer_kind =
      prepare::PreparedEdgePublicationSourceProducerKind::Cast;
  const auto unsupported_source =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &non_binary_publication,
          .move_bundle = &bundle,
          .placement_kind =
              prepare::PreparedSelectEdgeSourceProducerPlacementKind::
                  PredecessorEdgeConsumedSuppression,
          .move_bundle_block_label = join_label,
      });
  if (unsupported_source.status !=
      prepare::PreparedSelectEdgeSourceProducerPlacementStatus::
          UnsupportedSourceProducer) {
    return fail("expected non-binary source producer to stay fail-closed");
  }

  auto unsupported_publication = publication;
  unsupported_publication.carrier_kind =
      prepare::PreparedJoinTransferCarrierKind::None;
  const auto unsupported_carrier =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &unsupported_publication,
          .move_bundle = &bundle,
          .placement_kind =
              prepare::PreparedSelectEdgeSourceProducerPlacementKind::
                  PredecessorEdgeConsumedSuppression,
          .move_bundle_block_label = join_label,
      });
  if (unsupported_carrier.status !=
      prepare::PreparedSelectEdgeSourceProducerPlacementStatus::
          UnsupportedPublication) {
    return fail("expected non-select carrier publication to stay fail-closed");
  }

  const auto missing_site =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &publication,
          .move_bundle = &bundle,
          .placement_kind =
              prepare::PreparedSelectEdgeSourceProducerPlacementKind::
                  PredecessorEdgeConsumedSuppression,
      });
  if (missing_site.status !=
      prepare::PreparedSelectEdgeSourceProducerPlacementStatus::MissingProducerSite) {
    return fail("expected missing bundle block label to stay fail-closed");
  }

  const auto wrong_site =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &publication,
          .move_bundle = &bundle,
          .placement_kind =
              prepare::PreparedSelectEdgeSourceProducerPlacementKind::
                  PredecessorEdgeConsumedSuppression,
          .move_bundle_block_label = other_label,
      });
  if (wrong_site.status !=
      prepare::PreparedSelectEdgeSourceProducerPlacementStatus::
          MoveBundleProducerMismatch) {
    return fail("expected wrong producer site to stay fail-closed");
  }

  auto out_of_ssa_bundle = bundle;
  out_of_ssa_bundle.authority_kind =
      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy;
  const auto unsupported_bundle =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &publication,
          .move_bundle = &out_of_ssa_bundle,
          .placement_kind =
              prepare::PreparedSelectEdgeSourceProducerPlacementKind::
                  PredecessorEdgeConsumedSuppression,
          .move_bundle_block_label = join_label,
      });
  if (unsupported_bundle.status !=
      prepare::PreparedSelectEdgeSourceProducerPlacementStatus::
          UnsupportedMoveBundle) {
    return fail("expected out-of-SSA before-instruction bundle to stay fail-closed");
  }

  auto wrong_destination_bundle = bundle;
  wrong_destination_bundle.moves[0].to_value_id = 12;
  const auto wrong_destination =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &publication,
          .move_bundle = &wrong_destination_bundle,
          .placement_kind =
              prepare::PreparedSelectEdgeSourceProducerPlacementKind::
                  PredecessorEdgeConsumedSuppression,
          .move_bundle_block_label = join_label,
      });
  if (wrong_destination.status !=
      prepare::PreparedSelectEdgeSourceProducerPlacementStatus::
          MoveDestinationMismatch) {
    return fail("expected non-producer move destination to stay fail-closed");
  }

  auto stack_destination_bundle = bundle;
  stack_destination_bundle.moves[0].destination_storage_kind =
      prepare::PreparedMoveStorageKind::StackSlot;
  const auto stack_destination =
      prepare::plan_prepared_select_edge_source_producer_placement({
          .publication = &publication,
          .move_bundle = &stack_destination_bundle,
          .placement_kind =
              prepare::PreparedSelectEdgeSourceProducerPlacementKind::
                  PredecessorEdgeConsumedSuppression,
          .move_bundle_block_label = join_label,
      });
  if (stack_destination.status !=
      prepare::PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedMove) {
    return fail("expected non-register destination move to stay fail-closed");
  }

  if (prepare::prepared_select_edge_source_producer_placement_kind_name(
          prepare::PreparedSelectEdgeSourceProducerPlacementKind::
              PredecessorEdgeConsumedSuppression) !=
      "predecessor_edge_consumed_suppression") {
    return fail("expected placement kind name to stay stable");
  }
  if (prepare::prepared_select_edge_source_producer_placement_status_name(
          prepare::PreparedSelectEdgeSourceProducerPlacementStatus::
              MoveDestinationMismatch) != "move_destination_mismatch") {
    return fail("expected placement status name to stay stable");
  }

  prepare::PreparedBirModule prepared;
  prepared.target_profile = riscv_target_profile();
  const auto function_name =
      prepared.names.function_names.intern("select_edge_placement_contract");
  const auto prepared_pred_label = prepared.names.block_labels.intern("pred");
  const auto prepared_join_label = prepared.names.block_labels.intern("join");
  prepared.names.value_names.intern("%lhs");
  prepared.names.value_names.intern("%rhs");
  const auto prepared_cmp_name = prepared.names.value_names.intern("%cmp");
  const auto prepared_selected_name =
      prepared.names.value_names.intern("%selected");

  const bir::Value prepared_lhs = bir::Value::named(bir::TypeKind::Ptr, "%lhs");
  const bir::Value prepared_rhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs");
  const bir::Value prepared_cmp = bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value prepared_selected =
      bir::Value::named(bir::TypeKind::I32, "%selected");

  bir::Block pred_block;
  pred_block.label = "pred";
  pred_block.label_id = prepared_pred_label;
  pred_block.terminator = bir::Terminator{bir::BranchTerminator{
      .target_label = "join",
      .target_label_id = prepared_join_label,
  }};

  bir::Block join_block;
  join_block.label = "join";
  join_block.label_id = prepared_join_label;
  join_block.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::PtrToInt,
      .result = bir::Value::named(bir::TypeKind::I64, "%tmp0"),
      .operand = prepared_lhs,
  });
  join_block.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::PtrToInt,
      .result = bir::Value::named(bir::TypeKind::I64, "%tmp1"),
      .operand = prepared_rhs,
  });
  join_block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ule,
      .result = prepared_cmp,
      .operand_type = bir::TypeKind::Ptr,
      .lhs = prepared_lhs,
      .rhs = prepared_rhs,
  });

  bir::Function bir_function;
  bir_function.name = "select_edge_placement_contract";
  bir_function.blocks.push_back(std::move(pred_block));
  bir_function.blocks.push_back(std::move(join_block));
  prepared.module.functions.push_back(std::move(bir_function));

  prepare::PreparedControlFlowFunction control_flow;
  control_flow.function_name = function_name;
  control_flow.blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = prepared_pred_label,
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = prepared_join_label,
  });
  control_flow.blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = prepared_join_label,
      .terminator_kind = bir::TerminatorKind::Return,
  });
  control_flow.join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = function_name,
      .join_block_label = prepared_join_label,
      .result = prepared_selected,
      .carrier_kind =
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
      .edge_transfers =
          {
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = prepared_pred_label,
                  .successor_label = prepared_join_label,
                  .incoming_value = prepared_cmp,
                  .destination_value = prepared_selected,
              },
          },
  });
  control_flow.parallel_copy_bundles.push_back(prepare::PreparedParallelCopyBundle{
      .predecessor_label = prepared_pred_label,
      .successor_label = prepared_join_label,
      .execution_site =
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
      .execution_block_label = prepared_pred_label,
      .moves =
          {
              prepare::PreparedParallelCopyMove{
                  .join_transfer_index = 0,
                  .edge_transfer_index = 0,
                  .source_value = prepared_cmp,
                  .destination_value = prepared_selected,
                  .carrier_kind =
                      prepare::PreparedJoinTransferCarrierKind::
                          SelectMaterialization,
              },
          },
      .steps =
          {
              prepare::PreparedParallelCopyStep{
                  .kind = prepare::PreparedParallelCopyStepKind::Move,
                  .move_index = 0,
              },
          },
  });
  prepared.control_flow.functions.push_back(std::move(control_flow));

  prepare::PreparedValueLocationFunction locations;
  locations.function_name = function_name;
  locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 7,
      .function_name = function_name,
      .value_name = prepared.names.value_names.find("%lhs"),
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"s1"},
  });
  locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 9,
      .function_name = function_name,
      .value_name = prepared.names.value_names.find("%rhs"),
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{2},
      .offset_bytes = std::size_t{16},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
  });
  locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 10,
      .function_name = function_name,
      .value_name = prepared_cmp_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });
  locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 11,
      .function_name = function_name,
      .value_name = prepared_selected_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });
  locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = function_name,
      .phase = prepare::PreparedMovePhase::BlockEntry,
      .authority_kind =
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .block_index = 0,
      .instruction_index = 0,
      .source_parallel_copy_predecessor_label = prepared_pred_label,
      .source_parallel_copy_successor_label = prepared_join_label,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = 10,
                  .to_value_id = 11,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"t0"},
                  .block_index = 0,
                  .instruction_index = 0,
                  .source_parallel_copy_step_index = std::size_t{0},
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .source_parallel_copy_predecessor_label = prepared_pred_label,
                  .source_parallel_copy_successor_label = prepared_join_label,
              },
          },
  });
  locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = function_name,
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .authority_kind = prepare::PreparedMoveAuthorityKind::None,
      .block_index = 1,
      .instruction_index = 2,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = 7,
                  .to_value_id = 10,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"t0"},
                  .block_index = 1,
                  .instruction_index = 2,
                  .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              },
              prepare::PreparedMoveResolution{
                  .from_value_id = 9,
                  .to_value_id = 10,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"t0"},
                  .block_index = 1,
                  .instruction_index = 2,
                  .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              },
          },
  });
  prepared.value_locations.functions.push_back(std::move(locations));

  const auto collected =
      prepare::collect_prepared_select_edge_source_producer_placements(prepared);
  if (collected.records.size() != 1) {
    return fail("expected one collected select-edge placement authority record");
  }
  const auto& collected_record = collected.records.front();
  const auto& collected_placement = collected_record.placement;
  if (collected_record.function_name != function_name ||
      !prepare::prepared_select_edge_source_producer_placement_available(
          collected_placement) ||
      collected_placement.placement_kind !=
          prepare::PreparedSelectEdgeSourceProducerPlacementKind::
              PredecessorEdgeConsumedSuppression ||
      collected_placement.predecessor_label != prepared_pred_label ||
      collected_placement.successor_label != prepared_join_label ||
      collected_placement.source_value_id !=
          std::optional<prepare::PreparedValueId>{10} ||
      collected_placement.destination_value_id != 11 ||
      collected_placement.source_producer_block_label !=
          std::optional<c4c::BlockLabelId>{prepared_join_label} ||
      collected_placement.source_producer_instruction_index !=
          std::optional<std::size_t>{2} ||
      collected_placement.move_bundle_block_label !=
          std::optional<c4c::BlockLabelId>{prepared_join_label} ||
      collected_placement.move_bundle_instruction_index != 2 ||
      collected_placement.move_count != 2 ||
      collected_placement.publication != nullptr ||
      collected_placement.move_bundle != nullptr) {
    return fail("expected collected placement record to preserve semantic facts");
  }

  prepared.value_locations.functions.front().move_bundles.back().block_index = 0;
  const auto mismatched_collected =
      prepare::collect_prepared_select_edge_source_producer_placements(prepared);
  if (!mismatched_collected.records.empty()) {
    return fail("expected collector to reject mismatched bundle placement");
  }

  (void)lhs_name;
  (void)rhs_name;
  return 0;
}

int check_select_carrier_alias_authority_contract() {
  prepare::PreparedNameTables names;
  const auto pred_label = names.block_labels.intern("pred");
  const auto join_label = names.block_labels.intern("join");
  const auto function_name = names.function_names.intern("carrier_alias_contract");
  const auto source_name = names.value_names.intern("%cmp");
  const auto selected_name = names.value_names.intern("%selected");
  const auto alias0_name = names.value_names.intern("%alias0");
  const auto alias1_name = names.value_names.intern("%alias1");

  const bir::Value lhs = bir::Value::named(bir::TypeKind::Ptr, "%lhs");
  const bir::Value rhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs");
  const bir::Value cmp = bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value selected = bir::Value::named(bir::TypeKind::I32, "%selected");
  const bir::Value alias0 = bir::Value::named(bir::TypeKind::I32, "%alias0");
  const bir::Value alias1 = bir::Value::named(bir::TypeKind::I32, "%alias1");
  const bir::Value cond0 = bir::Value::named(bir::TypeKind::I32, "%cond0");
  const bir::Value cond1 = bir::Value::named(bir::TypeKind::I32, "%cond1");
  const bir::Value root_cond = bir::Value::named(bir::TypeKind::I32, "%root");

  bir::Function function;
  function.name = "carrier_alias_contract";
  bir::Block join_block;
  join_block.label = "join";
  join_block.label_id = join_label;
  join_block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ule,
      .result = cmp,
      .operand_type = bir::TypeKind::Ptr,
      .lhs = lhs,
      .rhs = rhs,
  });
  join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = alias0,
      .compare_type = bir::TypeKind::I32,
      .lhs = cond0,
      .rhs = bir::Value::immediate_i32(0),
      .true_value = cmp,
      .false_value = bir::Value::immediate_i32(0),
  });
  join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = alias1,
      .compare_type = bir::TypeKind::I32,
      .lhs = cond1,
      .rhs = bir::Value::immediate_i32(0),
      .true_value = cmp,
      .false_value = bir::Value::immediate_i32(0),
  });
  join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = selected,
      .compare_type = bir::TypeKind::I32,
      .lhs = root_cond,
      .rhs = bir::Value::immediate_i32(0),
      .true_value = alias0,
      .false_value = alias1,
  });
  function.blocks.push_back(std::move(join_block));

  const auto* compare =
      std::get_if<bir::BinaryInst>(&function.blocks.front().insts[0]);
  const auto* alias0_select =
      std::get_if<bir::SelectInst>(&function.blocks.front().insts[1]);
  const auto* alias1_select =
      std::get_if<bir::SelectInst>(&function.blocks.front().insts[2]);
  if (compare == nullptr || alias0_select == nullptr || alias1_select == nullptr) {
    return fail("carrier alias fixture was malformed");
  }

  prepare::PreparedJoinTransfer join_transfer{
      .function_name = function_name,
      .join_block_label = join_label,
      .result = selected,
      .carrier_kind =
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
      .edge_transfers =
          {
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = pred_label,
                  .successor_label = join_label,
                  .incoming_value = cmp,
                  .destination_value = selected,
              },
          },
  };
  const prepare::PreparedEdgePublication publication{
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .predecessor_label = pred_label,
      .successor_label = join_label,
      .destination_value = selected,
      .source_value = cmp,
      .destination_value_id = 11,
      .destination_value_name = selected_name,
      .source_value_id = 10,
      .source_value_name = source_name,
      .source_value_kind = bir::Value::Kind::Named,
      .source_producer_kind =
          prepare::PreparedEdgePublicationSourceProducerKind::Binary,
      .source_producer_block_label = join_label,
      .source_producer_instruction_index = 0,
      .source_binary = compare,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .phase = prepare::PreparedMovePhase::BlockEntry,
      .carrier_kind =
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
      .parallel_copy_execution_site =
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
      .parallel_copy_execution_block_label = pred_label,
      .join_transfer = &join_transfer,
  };

  prepare::PreparedValueHomeLookups value_home_lookups;
  value_home_lookups.value_ids.emplace(alias0_name, 20);
  value_home_lookups.value_ids.emplace(alias1_name, 21);
  prepare::PreparedControlFlowFunction control_flow;
  control_flow.function_name = function_name;

  const auto accepted = prepare::plan_prepared_select_carrier_alias_authority({
      .names = &names,
      .control_flow = &control_flow,
      .function = &function,
      .value_home_lookups = &value_home_lookups,
      .publication = &publication,
      .carrier_aliases =
          {
              prepare::PreparedSelectCarrierAliasCandidate{
                  .carrier_select = alias0_select,
                  .carrier_block_label = join_label,
                  .carrier_instruction_index = 1,
              },
              prepare::PreparedSelectCarrierAliasCandidate{
                  .carrier_select = alias1_select,
                  .carrier_block_label = join_label,
                  .carrier_instruction_index = 2,
              },
          },
  });
  if (!prepare::prepared_select_carrier_alias_authority_available(accepted) ||
      !accepted.source_use_closure_proven ||
      accepted.function_name != function_name ||
      accepted.predecessor_label != pred_label ||
      accepted.successor_label != join_label ||
      accepted.destination_value_id != 11 ||
      accepted.destination_value_name != selected_name ||
      accepted.source_value_id != std::optional<prepare::PreparedValueId>{10} ||
      accepted.source_value_name != source_name ||
      accepted.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
      accepted.source_producer_block_label !=
          std::optional<c4c::BlockLabelId>{join_label} ||
      accepted.source_producer_instruction_index !=
          std::optional<std::size_t>{0} ||
      accepted.carrier_aliases.size() != 2 ||
      accepted.carrier_aliases[0].carrier_value_name != alias0_name ||
      accepted.carrier_aliases[0].carrier_value_id !=
          std::optional<prepare::PreparedValueId>{20} ||
      accepted.carrier_aliases[1].carrier_value_name != alias1_name ||
      accepted.carrier_aliases[1].carrier_value_id !=
          std::optional<prepare::PreparedValueId>{21}) {
    return fail("expected duplicate carrier aliases to publish authority");
  }

  auto missing_source_publication = publication;
  missing_source_publication.source_binary = nullptr;
  const auto missing_source =
      prepare::plan_prepared_select_carrier_alias_authority({
          .names = &names,
          .control_flow = &control_flow,
          .function = &function,
          .publication = &missing_source_publication,
          .carrier_aliases =
              {
                  prepare::PreparedSelectCarrierAliasCandidate{
                      .carrier_select = alias0_select,
                      .carrier_block_label = join_label,
                      .carrier_instruction_index = 1,
                  },
              },
      });
  if (missing_source.status !=
      prepare::PreparedSelectCarrierAliasAuthorityStatus::MissingSourceProducer) {
    return fail("expected missing source producer to stay fail-closed");
  }

  auto wrong_result_function = function;
  names.value_names.intern("%raw.phi.sel");
  auto* wrong_alias = std::get_if<bir::SelectInst>(
      &wrong_result_function.blocks.front().insts[1]);
  wrong_alias->result = bir::Value::named(bir::TypeKind::I32, "%raw.phi.sel");
  const auto wrong_result =
      prepare::plan_prepared_select_carrier_alias_authority({
          .names = &names,
          .control_flow = &control_flow,
          .function = &wrong_result_function,
          .publication = &publication,
          .carrier_aliases =
              {
                  prepare::PreparedSelectCarrierAliasCandidate{
                      .carrier_select = wrong_alias,
                      .carrier_block_label = join_label,
                      .carrier_instruction_index = 1,
                  },
              },
      });
  if (wrong_result.status !=
      prepare::PreparedSelectCarrierAliasAuthorityStatus::MismatchedCarrierAlias) {
    return fail("expected raw-name-only carrier to stay fail-closed");
  }

  auto extra_use_function = function;
  extra_use_function.blocks.front().insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "%extra"),
      .operand_type = bir::TypeKind::I32,
      .lhs = cmp,
      .rhs = bir::Value::immediate_i32(0),
  });
  const auto* extra_alias0 = std::get_if<bir::SelectInst>(
      &extra_use_function.blocks.front().insts[1]);
  const auto* extra_alias1 = std::get_if<bir::SelectInst>(
      &extra_use_function.blocks.front().insts[2]);
  const auto extra_use =
      prepare::plan_prepared_select_carrier_alias_authority({
          .names = &names,
          .control_flow = &control_flow,
          .function = &extra_use_function,
          .publication = &publication,
          .carrier_aliases =
              {
                  prepare::PreparedSelectCarrierAliasCandidate{
                      .carrier_select = extra_alias0,
                      .carrier_block_label = join_label,
                      .carrier_instruction_index = 1,
                  },
                  prepare::PreparedSelectCarrierAliasCandidate{
                      .carrier_select = extra_alias1,
                      .carrier_block_label = join_label,
                      .carrier_instruction_index = 2,
                  },
              },
      });
  if (extra_use.status !=
      prepare::PreparedSelectCarrierAliasAuthorityStatus::NonCarrierSourceUse) {
    return fail("expected extra source use to stay fail-closed");
  }

  if (prepare::prepared_select_carrier_alias_authority_status_name(
          prepare::PreparedSelectCarrierAliasAuthorityStatus::
              NonCarrierSourceUse) != "non_carrier_source_use") {
    return fail("expected carrier alias status name to stay stable");
  }

  prepare::PreparedBirModule identity_prepared;
  identity_prepared.target_profile = riscv_target_profile();
  const auto identity_function_name =
      identity_prepared.names.function_names.intern("carrier_alias_identity");
  const auto identity_pred_label =
      identity_prepared.names.block_labels.intern("pred");
  const auto identity_join_label =
      identity_prepared.names.block_labels.intern("join");
  const auto identity_cmp_name =
      identity_prepared.names.value_names.intern("%cmp");
  const auto identity_selected_name =
      identity_prepared.names.value_names.intern("%selected");
  identity_prepared.names.value_names.intern("%lhs");
  identity_prepared.names.value_names.intern("%rhs");
  identity_prepared.names.value_names.intern("%cond0");
  identity_prepared.names.value_names.intern("%cond1");
  identity_prepared.names.value_names.intern("%root");

  const bir::Value identity_lhs =
      bir::Value::named(bir::TypeKind::Ptr, "%lhs");
  const bir::Value identity_rhs =
      bir::Value::named(bir::TypeKind::Ptr, "%rhs");
  const bir::Value identity_cmp =
      bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value identity_selected =
      bir::Value::named(bir::TypeKind::I32, "%selected");
  const bir::Value identity_alias0 =
      bir::Value::named(bir::TypeKind::I32, "%alias0");
  const bir::Value identity_alias1 =
      bir::Value::named(bir::TypeKind::I32, "%alias1");

  bir::Block identity_pred_block;
  identity_pred_block.label = "pred";
  identity_pred_block.label_id = identity_pred_label;
  identity_pred_block.terminator = bir::Terminator{bir::BranchTerminator{
      .target_label = "join",
      .target_label_id = identity_join_label,
  }};
  bir::Block identity_join_block;
  identity_join_block.label = "join";
  identity_join_block.label_id = identity_join_label;
  identity_join_block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ule,
      .result = identity_cmp,
      .operand_type = bir::TypeKind::Ptr,
      .lhs = identity_lhs,
      .rhs = identity_rhs,
  });
  identity_join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = identity_alias0,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%cond0"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = identity_cmp,
      .false_value = bir::Value::immediate_i32(0),
  });
  identity_join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = identity_alias1,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%cond1"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = identity_cmp,
      .false_value = bir::Value::immediate_i32(0),
  });
  identity_join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = identity_selected,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%root"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = identity_alias0,
      .false_value = identity_alias1,
  });
  bir::Function identity_function;
  identity_function.name = "carrier_alias_identity";
  identity_function.blocks.push_back(std::move(identity_pred_block));
  identity_function.blocks.push_back(std::move(identity_join_block));
  identity_prepared.module.functions.push_back(std::move(identity_function));

  prepare::PreparedControlFlowFunction identity_cf;
  identity_cf.function_name = identity_function_name;
  identity_cf.blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = identity_pred_label,
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = identity_join_label,
  });
  identity_cf.blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = identity_join_label,
      .terminator_kind = bir::TerminatorKind::Return,
  });
  identity_cf.join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = identity_function_name,
      .join_block_label = identity_join_label,
      .result = identity_selected,
      .carrier_kind =
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
      .edge_transfers =
          {
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = identity_pred_label,
                  .successor_label = identity_join_label,
                  .incoming_value = identity_cmp,
                  .destination_value = identity_selected,
              },
          },
  });
  identity_cf.parallel_copy_bundles.push_back(prepare::PreparedParallelCopyBundle{
      .predecessor_label = identity_pred_label,
      .successor_label = identity_join_label,
      .execution_site =
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
      .execution_block_label = identity_pred_label,
      .moves =
          {
              prepare::PreparedParallelCopyMove{
                  .join_transfer_index = 0,
                  .edge_transfer_index = 0,
                  .source_value = identity_cmp,
                  .destination_value = identity_selected,
                  .carrier_kind =
                      prepare::PreparedJoinTransferCarrierKind::
                          SelectMaterialization,
              },
          },
      .steps =
          {
              prepare::PreparedParallelCopyStep{
                  .kind = prepare::PreparedParallelCopyStepKind::Move,
                  .move_index = 0,
              },
          },
  });
  identity_prepared.control_flow.functions.push_back(std::move(identity_cf));

  prepare::PreparedValueLocationFunction identity_locations;
  identity_locations.function_name = identity_function_name;
  identity_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 10,
      .function_name = identity_function_name,
      .value_name = identity_cmp_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });
  identity_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 11,
      .function_name = identity_function_name,
      .value_name = identity_selected_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });
  identity_locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = identity_function_name,
      .phase = prepare::PreparedMovePhase::BlockEntry,
      .authority_kind =
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .block_index = 0,
      .instruction_index = 0,
      .source_parallel_copy_predecessor_label = identity_pred_label,
      .source_parallel_copy_successor_label = identity_join_label,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = 10,
                  .to_value_id = 11,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"t0"},
                  .block_index = 0,
                  .instruction_index = 0,
                  .source_parallel_copy_step_index = std::size_t{0},
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .source_parallel_copy_predecessor_label = identity_pred_label,
                  .source_parallel_copy_successor_label = identity_join_label,
              },
          },
  });
  identity_prepared.value_locations.functions.push_back(
      std::move(identity_locations));

  const auto identity_before =
      prepare::collect_prepared_select_carrier_alias_authority_evidence(
          identity_prepared);
  if (identity_before.records.size() != 1 ||
      identity_before.records.front().authority.status !=
          prepare::PreparedSelectCarrierAliasAuthorityStatus::
              UnsupportedCarrierAlias ||
      identity_prepared.names.value_names.find("%alias0") !=
          c4c::kInvalidValueName ||
      identity_prepared.names.value_names.find("%alias1") !=
          c4c::kInvalidValueName) {
    return fail("expected const carrier alias collection not to publish identity");
  }

  prepare::populate_select_carrier_alias_identity(identity_prepared);
  const auto identity_alias0_name =
      identity_prepared.names.value_names.find("%alias0");
  const auto identity_alias1_name =
      identity_prepared.names.value_names.find("%alias1");
  const auto identity_after =
      prepare::collect_prepared_select_carrier_alias_authorities(
          identity_prepared);
  if (identity_alias0_name == c4c::kInvalidValueName ||
      identity_alias1_name == c4c::kInvalidValueName ||
      identity_after.records.size() != 1 ||
      identity_after.records.front().authority.carrier_aliases.size() != 2 ||
      identity_after.records.front()
              .authority.carrier_aliases[0]
              .carrier_value_name != identity_alias0_name ||
      identity_after.records.front()
              .authority.carrier_aliases[0]
              .carrier_value_id.has_value() ||
      identity_after.records.front()
              .authority.carrier_aliases[1]
              .carrier_value_name != identity_alias1_name ||
      identity_after.records.front()
              .authority.carrier_aliases[1]
              .carrier_value_id.has_value()) {
    return fail("expected shared carrier alias identity to publish without homes");
  }

  auto identity_extra_use = identity_prepared;
  identity_extra_use.module.functions.front().blocks.back().insts.push_back(
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Eq,
          .result = bir::Value::named(bir::TypeKind::I32, "%extra_identity"),
          .operand_type = bir::TypeKind::I32,
          .lhs = identity_cmp,
          .rhs = bir::Value::immediate_i32(0),
      });
  identity_extra_use.names.value_names.intern("%extra_identity");
  const auto identity_extra_collected =
      prepare::collect_prepared_select_carrier_alias_authorities(
          identity_extra_use);
  const auto identity_extra_evidence =
      prepare::collect_prepared_select_carrier_alias_authority_evidence(
          identity_extra_use);
  if (!identity_extra_collected.records.empty() ||
      identity_extra_evidence.records.size() != 1 ||
      identity_extra_evidence.records.front().authority.status !=
          prepare::PreparedSelectCarrierAliasAuthorityStatus::NonCarrierSourceUse ||
      identity_extra_evidence.records.front().authority.carrier_aliases.size() !=
          2) {
    return fail("expected shared identity not to bypass source-use closure");
  }

  prepare::PreparedBirModule prepared;
  prepared.target_profile = riscv_target_profile();
  const auto prepared_function_name =
      prepared.names.function_names.intern("carrier_alias_collector");
  const auto prepared_pred_label = prepared.names.block_labels.intern("pred");
  const auto prepared_join_label = prepared.names.block_labels.intern("join");
  const auto prepared_cmp_name = prepared.names.value_names.intern("%cmp");
  const auto prepared_selected_name =
      prepared.names.value_names.intern("%selected");
  const auto prepared_alias0_name = prepared.names.value_names.intern("%alias0");
  const auto prepared_alias1_name = prepared.names.value_names.intern("%alias1");
  prepared.names.value_names.intern("%lhs");
  prepared.names.value_names.intern("%rhs");
  prepared.names.value_names.intern("%cond0");
  prepared.names.value_names.intern("%cond1");
  prepared.names.value_names.intern("%root");

  const bir::Value prepared_lhs = bir::Value::named(bir::TypeKind::Ptr, "%lhs");
  const bir::Value prepared_rhs = bir::Value::named(bir::TypeKind::Ptr, "%rhs");
  const bir::Value prepared_cmp = bir::Value::named(bir::TypeKind::I32, "%cmp");
  const bir::Value prepared_selected =
      bir::Value::named(bir::TypeKind::I32, "%selected");
  const bir::Value prepared_alias0 =
      bir::Value::named(bir::TypeKind::I32, "%alias0");
  const bir::Value prepared_alias1 =
      bir::Value::named(bir::TypeKind::I32, "%alias1");

  bir::Block prepared_pred_block;
  prepared_pred_block.label = "pred";
  prepared_pred_block.label_id = prepared_pred_label;
  prepared_pred_block.terminator = bir::Terminator{bir::BranchTerminator{
      .target_label = "join",
      .target_label_id = prepared_join_label,
  }};
  bir::Block prepared_join_block;
  prepared_join_block.label = "join";
  prepared_join_block.label_id = prepared_join_label;
  prepared_join_block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ule,
      .result = prepared_cmp,
      .operand_type = bir::TypeKind::Ptr,
      .lhs = prepared_lhs,
      .rhs = prepared_rhs,
  });
  prepared_join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = prepared_alias0,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%cond0"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = prepared_cmp,
      .false_value = bir::Value::immediate_i32(0),
  });
  prepared_join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = prepared_alias1,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%cond1"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = prepared_cmp,
      .false_value = bir::Value::immediate_i32(0),
  });
  prepared_join_block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = prepared_selected,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%root"),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = prepared_alias0,
      .false_value = prepared_alias1,
  });
  bir::Function prepared_function;
  prepared_function.name = "carrier_alias_collector";
  prepared_function.blocks.push_back(std::move(prepared_pred_block));
  prepared_function.blocks.push_back(std::move(prepared_join_block));
  prepared.module.functions.push_back(std::move(prepared_function));

  prepare::PreparedControlFlowFunction prepared_cf;
  prepared_cf.function_name = prepared_function_name;
  prepared_cf.blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = prepared_pred_label,
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = prepared_join_label,
  });
  prepared_cf.blocks.push_back(prepare::PreparedControlFlowBlock{
      .block_label = prepared_join_label,
      .terminator_kind = bir::TerminatorKind::Return,
  });
  prepared_cf.join_transfers.push_back(prepare::PreparedJoinTransfer{
      .function_name = prepared_function_name,
      .join_block_label = prepared_join_label,
      .result = prepared_selected,
      .carrier_kind =
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
      .edge_transfers =
          {
              prepare::PreparedEdgeValueTransfer{
                  .predecessor_label = prepared_pred_label,
                  .successor_label = prepared_join_label,
                  .incoming_value = prepared_cmp,
                  .destination_value = prepared_selected,
              },
          },
  });
  prepared_cf.parallel_copy_bundles.push_back(prepare::PreparedParallelCopyBundle{
      .predecessor_label = prepared_pred_label,
      .successor_label = prepared_join_label,
      .execution_site =
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
      .execution_block_label = prepared_pred_label,
      .moves =
          {
              prepare::PreparedParallelCopyMove{
                  .join_transfer_index = 0,
                  .edge_transfer_index = 0,
                  .source_value = prepared_cmp,
                  .destination_value = prepared_selected,
                  .carrier_kind =
                      prepare::PreparedJoinTransferCarrierKind::
                          SelectMaterialization,
              },
          },
      .steps =
          {
              prepare::PreparedParallelCopyStep{
                  .kind = prepare::PreparedParallelCopyStepKind::Move,
                  .move_index = 0,
              },
          },
  });
  prepared.control_flow.functions.push_back(std::move(prepared_cf));

  prepare::PreparedValueLocationFunction prepared_locations;
  prepared_locations.function_name = prepared_function_name;
  prepared_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 10,
      .function_name = prepared_function_name,
      .value_name = prepared_cmp_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });
  prepared_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 11,
      .function_name = prepared_function_name,
      .value_name = prepared_selected_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t0"},
  });
  prepared_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 20,
      .function_name = prepared_function_name,
      .value_name = prepared_alias0_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t1"},
  });
  prepared_locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = 21,
      .function_name = prepared_function_name,
      .value_name = prepared_alias1_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"t2"},
  });
  prepared_locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = prepared_function_name,
      .phase = prepare::PreparedMovePhase::BlockEntry,
      .authority_kind =
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .block_index = 0,
      .instruction_index = 0,
      .source_parallel_copy_predecessor_label = prepared_pred_label,
      .source_parallel_copy_successor_label = prepared_join_label,
      .moves =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = 10,
                  .to_value_id = 11,
                  .destination_kind =
                      prepare::PreparedMoveDestinationKind::Value,
                  .destination_storage_kind =
                      prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"t0"},
                  .block_index = 0,
                  .instruction_index = 0,
                  .source_parallel_copy_step_index = std::size_t{0},
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .source_parallel_copy_predecessor_label = prepared_pred_label,
                  .source_parallel_copy_successor_label = prepared_join_label,
              },
          },
  });
  prepared.value_locations.functions.push_back(std::move(prepared_locations));

  const auto collected =
      prepare::collect_prepared_select_carrier_alias_authorities(prepared);
  if (collected.records.size() != 1) {
    return fail("expected one collected select carrier alias authority record");
  }
  const auto evidence =
      prepare::collect_prepared_select_carrier_alias_authority_evidence(prepared);
  if (evidence.records.size() != 1 ||
      evidence.records.front().authority.status !=
          prepare::PreparedSelectCarrierAliasAuthorityStatus::Available ||
      evidence.records.front().authority.carrier_alias_candidate_count != 2) {
    return fail("expected carrier alias authority evidence to expose available row");
  }
  const auto& collected_record = collected.records.front();
  const auto& collected_authority = collected_record.authority;
  if (collected_record.function_name != prepared_function_name ||
      !prepare::prepared_select_carrier_alias_authority_available(
          collected_authority) ||
      !collected_authority.source_use_closure_proven ||
      collected_authority.publication != nullptr ||
      collected_authority.join_transfer != nullptr ||
      collected_authority.predecessor_label != prepared_pred_label ||
      collected_authority.successor_label != prepared_join_label ||
      collected_authority.destination_value_id != 11 ||
      collected_authority.source_value_id !=
          std::optional<prepare::PreparedValueId>{10} ||
      collected_authority.carrier_aliases.size() != 2 ||
      collected_authority.carrier_aliases[0].carrier_value_name !=
          prepared_alias0_name ||
      collected_authority.carrier_aliases[0].carrier_value_id !=
          std::optional<prepare::PreparedValueId>{20} ||
      collected_authority.carrier_aliases[1].carrier_value_name !=
          prepared_alias1_name ||
      collected_authority.carrier_aliases[1].carrier_value_id !=
          std::optional<prepare::PreparedValueId>{21}) {
    return fail("expected collected carrier alias record to preserve facts");
  }
  const std::string dump = prepare::print(prepared);
  if (dump.find("select_carrier_alias_authority "
                "function=carrier_alias_collector status=available "
                "predecessor=pred successor=join destination=%selected "
                "destination_value_id=11 source=%cmp source_value_id=10 "
                "source_producer=binary source_producer_block=join "
                "source_producer_inst=0 carrier_alias_candidates=2 "
                "carrier_aliases=2 source_use_closure=yes") ==
      std::string::npos) {
    return fail("expected prepared dump to expose available carrier alias authority");
  }
  if (dump.find("alias[0]=%alias0 alias[0]_value_id=20 "
                "alias[0]_block=join alias[0]_inst=1") ==
          std::string::npos ||
      dump.find("alias[1]=%alias1 alias[1]_value_id=21 "
                "alias[1]_block=join alias[1]_inst=2") ==
          std::string::npos) {
    return fail("expected prepared dump to expose carrier alias fields");
  }

  auto rejected_prepared = prepared;
  rejected_prepared.module.functions.front().blocks.back().insts.push_back(
      bir::BinaryInst{
          .opcode = bir::BinaryOpcode::Eq,
          .result = bir::Value::named(bir::TypeKind::I32, "%extra"),
          .operand_type = bir::TypeKind::I32,
          .lhs = prepared_cmp,
          .rhs = bir::Value::immediate_i32(0),
      });
  rejected_prepared.names.value_names.intern("%extra");
  const auto rejected_collected =
      prepare::collect_prepared_select_carrier_alias_authorities(
          rejected_prepared);
  if (!rejected_collected.records.empty()) {
    return fail("expected rejected carrier alias evidence to remain non-authority");
  }
  const auto rejected_evidence =
      prepare::collect_prepared_select_carrier_alias_authority_evidence(
          rejected_prepared);
  if (rejected_evidence.records.size() != 1 ||
      rejected_evidence.records.front().authority.status !=
          prepare::PreparedSelectCarrierAliasAuthorityStatus::NonCarrierSourceUse) {
    return fail("expected rejected carrier alias evidence to expose status");
  }
  const std::string rejected_dump = prepare::print(rejected_prepared);
  if (rejected_dump.find("select_carrier_alias_authority "
                         "function=carrier_alias_collector "
                         "status=non_carrier_source_use") ==
          std::string::npos ||
      rejected_dump.find("carrier_alias_candidates=2 carrier_aliases=2 "
                         "source_use_closure=no") == std::string::npos) {
    return fail("expected prepared dump to expose rejected carrier alias status");
  }

  return 0;
}

int check_populated_immediate_global_store_source_publication() {
  const auto run_case = [](bool coherent_destination) {
    prepare::PreparedBirModule prepared;
    const auto function_name =
        prepared.names.function_names.intern(
            coherent_destination ? "immediate_global_store"
                                 : "immediate_global_store_unknown_layout");
    const auto block_label = prepared.names.block_labels.intern("entry");
    const auto symbol_name = prepared.names.link_names.intern("g.counter");

    bir::Block block;
    block.label = "entry";
    block.label_id = block_label;
    block.insts.push_back(bir::StoreGlobalInst{
        .global_name = "g.counter",
        .global_name_id = symbol_name,
        .value = bir::Value::immediate_i32(1),
        .address =
            bir::MemoryAddress{
                .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                .base_name = "g.counter",
                .size_bytes = 4,
                .align_bytes = 4,
                .base_link_name_id = symbol_name,
            },
    });

    bir::Function function;
    function.name = coherent_destination ? "immediate_global_store"
                                         : "immediate_global_store_unknown_layout";
    function.blocks = {block};
    prepared.module.functions.push_back(function);
    prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
        .function_name = function_name,
        .blocks =
            {
                prepare::PreparedControlFlowBlock{
                    .block_label = block_label,
                    .terminator_kind = bir::TerminatorKind::Return,
                },
            },
    });

    auto address = make_proven_global_symbol_memory_address(symbol_name);
    if (!coherent_destination) {
      address.provenance.layout_authority =
          bir::MemoryLayoutAuthorityKind::Unknown;
    }
    prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
        .function_name = function_name,
        .accesses =
            {
                prepare::PreparedMemoryAccess{
                    .function_name = function_name,
                    .block_label = block_label,
                    .inst_index = std::size_t{0},
                    .address = address,
                },
            },
    });

    prepare::populate_store_source_publication_plans(prepared);
    const auto& records = prepared.store_source_publications.records;
    if (records.size() != 1) {
      return fail("expected one populated immediate global store-source record");
    }
    const auto& plan = records.front().plan;
    if (!prepare::prepared_store_source_publication_available(plan) ||
        plan.intent !=
            prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication ||
        plan.source_value.kind != bir::Value::Kind::Immediate ||
        plan.source_value.type != bir::TypeKind::I32 ||
        plan.source_value.immediate != 1 ||
        plan.source_value_name != c4c::kInvalidValueName ||
        plan.source_home != nullptr ||
        plan.source_producer_block_label.has_value() ||
        plan.source_producer_instruction_index.has_value()) {
      return fail("expected populated immediate global store record to preserve literal source shape");
    }

    const bool explicit_immediate =
        plan.source_producer_kind ==
        prepare::PreparedEdgePublicationSourceProducerKind::Immediate;
    const bool has_authority =
        prepare::prepared_store_global_publication_has_authority(plan);
    if (coherent_destination) {
      if (!explicit_immediate || !has_authority) {
        return fail("expected coherent immediate global store to publish explicit immediate authority");
      }
    } else if (explicit_immediate || has_authority) {
      return fail("expected unknown-layout immediate global store to stay fail-closed");
    }
    return 0;
  };

  if (const int result = run_case(true); result != 0) {
    return result;
  }
  return run_case(false);
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

  const auto* symbol_copy_plus =
      find_value_home_by_name(fixture.names, homes, "contract.symbol.copy.plus.ptr");
  if (symbol_copy_plus == nullptr) {
    return fail("expected a value home for the symbol-carrier local-slot copy offset");
  }
  if (symbol_copy_plus->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !symbol_copy_plus->pointer_base_value_name.has_value() ||
      prepare::prepared_value_name(fixture.names, *symbol_copy_plus->pointer_base_value_name) !=
          "contract.symbol.ptr" ||
      !symbol_copy_plus->pointer_base_symbol_name.has_value() ||
      prepare::prepared_link_name(fixture.names, *symbol_copy_plus->pointer_base_symbol_name) !=
          "contract.symbol" ||
      symbol_copy_plus->pointer_byte_delta != std::optional<std::int64_t>{4}) {
    return fail("expected acyclic local-slot symbol copy arithmetic to seed derived carriers");
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

  const auto* prepared_copy_plus =
      find_value_home_by_name(fixture.names, homes, "contract.prepared.copy.plus.ptr");
  if (prepared_copy_plus == nullptr) {
    return fail("expected a value home for the prepared-pointer local-slot copy offset");
  }
  if (prepared_copy_plus->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !prepared_copy_plus->pointer_base_value_name.has_value() ||
      prepare::prepared_value_name(fixture.names, *prepared_copy_plus->pointer_base_value_name) !=
          "contract.base.ptr" ||
      prepared_copy_plus->pointer_base_symbol_name.has_value() ||
      prepared_copy_plus->pointer_byte_delta != std::optional<std::int64_t>{8}) {
    return fail("expected acyclic prepared local-slot copy arithmetic to seed derived carriers");
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

  const auto* frame_addr_plus =
      find_value_home_by_name(fixture.names, homes, "contract.frame.addr.plus.ptr");
  if (frame_addr_plus == nullptr) {
    return fail("expected a value home for the frame-address offset carrier");
  }
  if (frame_addr_plus->kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !frame_addr_plus->pointer_base_value_name.has_value() ||
      prepare::prepared_value_name(fixture.names, *frame_addr_plus->pointer_base_value_name) !=
          "contract.frame.addr.ptr" ||
      frame_addr_plus->pointer_base_symbol_name.has_value() ||
      frame_addr_plus->pointer_byte_delta != std::optional<std::int64_t>{12}) {
    return fail("expected prepared frame-address materialization authority to seed pointer offsets");
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
  if (function_addressing->accesses.size() != 15) {
    return fail("expected raw structured-global fallbacks to fail closed while compatibility remains");
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
  if (id_load_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::ScalarLayout ||
      id_load_access->address.provenance.object_extent.completeness !=
          bir::MemoryObjectExtentCompleteness::Complete ||
      id_load_access->address.provenance.object_extent.size_bytes != 4 ||
      !id_load_access->address.provenance.object_extent.size_known ||
      id_load_access->address.provenance.range_verdict !=
          bir::MemoryRangeVerdict::ProvenInBounds ||
      !prepare::prepared_global_symbol_memory_has_publication_authority(
          id_load_access->address)) {
    return fail("expected in-bounds scalar global load to publish layout authority");
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

  if (prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 8) !=
      nullptr) {
    return fail("expected raw/no-id ordinary load to fail closed for a structured global");
  }

  const auto* id_gep_load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 9);
  if (id_gep_load_access == nullptr) {
    return fail("expected explicit GlobalSymbol address with LinkNameId to publish prepared access");
  }
  if (!id_gep_load_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *id_gep_load_access->result_value_name) !=
          "gep.id.loaded" ||
      id_gep_load_access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !id_gep_load_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *id_gep_load_access->address.symbol_name) !=
          "g.authoritative" ||
      id_gep_load_access->address.byte_offset != 12 ||
      id_gep_load_access->address.size_bytes != 4 ||
      id_gep_load_access->address.align_bytes != 4) {
    return fail("expected explicit GlobalSymbol LinkNameId address to preserve structured facts");
  }

  if (prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 10) !=
      nullptr) {
    return fail("expected raw GlobalSymbol base_name alone to fail closed for a structured global");
  }

  const auto* compat_gep_load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 11);
  if (compat_gep_load_access == nullptr) {
    return fail("expected raw/no-id GlobalSymbol compatibility access to remain supported");
  }
  if (!compat_gep_load_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *compat_gep_load_access->result_value_name) !=
          "gep.compat.loaded" ||
      compat_gep_load_access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !compat_gep_load_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *compat_gep_load_access->address.symbol_name) !=
          "g.compat" ||
      compat_gep_load_access->address.byte_offset != 20 ||
      compat_gep_load_access->address.size_bytes != 4 ||
      compat_gep_load_access->address.align_bytes != 4) {
    return fail("expected raw/no-id GlobalSymbol compatibility to preserve address facts");
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
  if (prepare::find_prepared_address_materialization(*function_addressing,
                                                     entry_block_label_id,
                                                     12) != nullptr) {
    return fail("expected raw @name pointer value without LinkNameId to stay out of global materialization");
  }

  const auto* local_global_lane_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 13);
  if (local_global_lane_access == nullptr) {
    return fail("expected LoadGlobalInst global lane with LinkNameId to publish prepared access");
  }
  const auto& local_global_lane_address = local_global_lane_access->address;
  const auto& local_global_lane_provenance = local_global_lane_address.provenance;
  if (!local_global_lane_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names,
                                   *local_global_lane_access->result_value_name) !=
          "local.global.lane.loaded" ||
      local_global_lane_access->stored_value_name.has_value() ||
      local_global_lane_access->address_space != bir::AddressSpace::Default ||
      local_global_lane_access->is_volatile ||
      local_global_lane_address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !local_global_lane_address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *local_global_lane_address.symbol_name) !=
          "g.aggregate.contract" ||
      local_global_lane_address.global_address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      local_global_lane_address.byte_offset != 24 ||
      local_global_lane_address.size_bytes != 4 ||
      local_global_lane_address.align_bytes != 4 ||
      !local_global_lane_address.can_use_base_plus_offset ||
      local_global_lane_provenance.base_identity.kind !=
          bir::MemoryProvenanceBaseIdentityKind::GlobalSymbol ||
      local_global_lane_provenance.base_identity.spelling != "g.aggregate.contract" ||
      local_global_lane_provenance.base_identity.link_name_id == c4c::kInvalidLinkName ||
      prepared.module.names.link_names.spelling(
          local_global_lane_provenance.base_identity.link_name_id) !=
          "g.aggregate.contract" ||
      local_global_lane_provenance.object_extent.completeness !=
          bir::MemoryObjectExtentCompleteness::Complete ||
      local_global_lane_provenance.object_extent.size_bytes != 96 ||
      !local_global_lane_provenance.object_extent.size_known ||
      local_global_lane_provenance.requested_range.begin != 24 ||
      local_global_lane_provenance.requested_range.size_bytes != 4 ||
      local_global_lane_provenance.requested_range.end != 28 ||
      !local_global_lane_provenance.requested_range.available ||
      !local_global_lane_provenance.requested_range.end_available ||
      local_global_lane_provenance.requested_range.overflowed ||
      local_global_lane_provenance.range_verdict != bir::MemoryRangeVerdict::ProvenInBounds) {
    return fail("expected LoadGlobalInst global lane to preserve semantic prepared contract facts");
  }
  if (local_global_lane_provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          local_global_lane_address)) {
    return fail("expected aggregate global lane to stay out of scalar layout authority");
  }

  if (prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 14) !=
      nullptr) {
    return fail("expected LoadLocalInst structured global spelling without LinkNameId to fail closed");
  }

  const auto* scalar_store_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 15);
  if (scalar_store_access == nullptr) {
    return fail("expected in-bounds scalar global store to publish prepared access");
  }
  if (scalar_store_access->result_value_name.has_value() ||
      !scalar_store_access->stored_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *scalar_store_access->stored_value_name) !=
          "id.loaded" ||
      scalar_store_access->address.base_kind !=
          prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !scalar_store_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *scalar_store_access->address.symbol_name) !=
          "g.authoritative" ||
      scalar_store_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::ScalarLayout ||
      !prepare::prepared_global_symbol_memory_has_publication_authority(
          scalar_store_access->address)) {
    return fail("expected in-bounds scalar global store to publish layout authority");
  }

  const auto* out_of_range_scalar_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 16);
  if (out_of_range_scalar_access == nullptr) {
    return fail("expected out-of-range scalar global load to publish prepared access");
  }
  if (out_of_range_scalar_access->address.provenance.range_verdict !=
          bir::MemoryRangeVerdict::ProvenOutOfBounds ||
      out_of_range_scalar_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          out_of_range_scalar_access->address)) {
    return fail("expected out-of-range scalar global load to stay fail-closed");
  }

  const auto* missing_extent_scalar_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 17);
  if (missing_extent_scalar_access == nullptr) {
    return fail("expected missing-extent scalar global load to publish prepared access");
  }
  if (missing_extent_scalar_access->address.provenance.object_extent.size_known ||
      missing_extent_scalar_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          missing_extent_scalar_access->address)) {
    return fail("expected missing-extent scalar global load to stay fail-closed");
  }

  const auto* integer_array_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 18);
  if (integer_array_access == nullptr) {
    return fail("expected in-bounds integer-array global load to publish prepared access");
  }
  if (!integer_array_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *integer_array_access->result_value_name) !=
          "array.element.loaded" ||
      integer_array_access->stored_value_name.has_value() ||
      integer_array_access->address.base_kind !=
          prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !integer_array_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *integer_array_access->address.symbol_name) !=
          "g.i64.array" ||
      integer_array_access->address.byte_offset != 72 ||
      integer_array_access->address.size_bytes != 8 ||
      integer_array_access->address.align_bytes != 8 ||
      integer_array_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::ByteStorageAggregate ||
      integer_array_access->address.provenance.object_extent.completeness !=
          bir::MemoryObjectExtentCompleteness::Complete ||
      integer_array_access->address.provenance.object_extent.size_bytes != 80 ||
      !integer_array_access->address.provenance.object_extent.size_known ||
      integer_array_access->address.provenance.requested_range.begin != 72 ||
      integer_array_access->address.provenance.requested_range.size_bytes != 8 ||
      integer_array_access->address.provenance.requested_range.end != 80 ||
      integer_array_access->address.provenance.range_verdict !=
          bir::MemoryRangeVerdict::ProvenInBounds ||
      !prepare::prepared_global_symbol_memory_has_publication_authority(
          integer_array_access->address)) {
    return fail("expected in-bounds integer-array element to publish layout authority");
  }

  const auto* out_of_range_array_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 19);
  if (out_of_range_array_access == nullptr) {
    return fail("expected out-of-range integer-array load to publish prepared access");
  }
  if (out_of_range_array_access->address.provenance.range_verdict !=
          bir::MemoryRangeVerdict::ProvenOutOfBounds ||
      out_of_range_array_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          out_of_range_array_access->address)) {
    return fail("expected out-of-range integer-array load to stay fail-closed");
  }

  const auto* misaligned_array_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 20);
  if (misaligned_array_access == nullptr) {
    return fail("expected misaligned integer-array load to publish prepared access");
  }
  if (misaligned_array_access->address.provenance.range_verdict !=
          bir::MemoryRangeVerdict::ProvenInBounds ||
      misaligned_array_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          misaligned_array_access->address)) {
    return fail("expected misaligned integer-array offset to stay fail-closed");
  }

  const auto* partial_width_array_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 21);
  if (partial_width_array_access == nullptr) {
    return fail("expected partial-width integer-array load to publish prepared access");
  }
  if (partial_width_array_access->address.provenance.range_verdict !=
          bir::MemoryRangeVerdict::ProvenInBounds ||
      partial_width_array_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          partial_width_array_access->address)) {
    return fail("expected partial-width integer-array access to stay fail-closed");
  }

  const auto* extern_array_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 22);
  if (extern_array_access == nullptr) {
    return fail("expected extern integer-array load to publish prepared access");
  }
  if (extern_array_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          extern_array_access->address)) {
    return fail("expected extern integer-array global to stay fail-closed");
  }

  const auto* tls_array_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 23);
  if (tls_array_access == nullptr) {
    return fail("expected TLS integer-array load to publish prepared access");
  }
  if (tls_array_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          tls_array_access->address)) {
    return fail("expected TLS integer-array global to stay fail-closed");
  }

  const auto* missing_extent_array_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 24);
  if (missing_extent_array_access == nullptr) {
    return fail("expected missing-extent integer-array load to publish prepared access");
  }
  if (missing_extent_array_access->address.provenance.object_extent.size_known ||
      missing_extent_array_access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::Unknown ||
      prepare::prepared_global_symbol_memory_has_publication_authority(
          missing_extent_array_access->address)) {
    return fail("expected missing-extent integer-array global to stay fail-closed");
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
  if (!root_object->frame_address_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *root_object->frame_address_value_name) !=
          "lv.binary.rooted.root" ||
      !root_object->legacy_frame_address_name_compatibility) {
    return fail("expected rooted pointer slot to publish bounded frame-address compatibility metadata");
  }

  const auto function_name_id =
      find_function_name_id(prepared, "stack_layout_rooted_pointer_binary_local_slot_activation");
  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_name_id);
  const c4c::BlockLabelId entry_block_label_id = find_block_label_id(prepared, "entry");
  if (function_addressing == nullptr) {
    return fail("expected rooted pointer binary fixture to publish prepared addressing");
  }
  bool saw_frame_address_materialization = false;
  for (const auto& materialization : function_addressing->address_materializations) {
    if (materialization.block_label != entry_block_label_id || materialization.inst_index != 0 ||
        materialization.kind != prepare::PreparedAddressMaterializationKind::FrameSlot ||
        !materialization.result_value_name.has_value() ||
        prepare::prepared_value_name(prepared.names, *materialization.result_value_name) !=
            "lv.binary.rooted.root") {
      continue;
    }
    if (!materialization.frame_slot_id.has_value() ||
        *materialization.frame_slot_id != root_slot->slot_id ||
        materialization.byte_offset != static_cast<std::int64_t>(root_slot->offset_bytes)) {
      return fail("expected frame-address materialization to consume the prepared frame-address fact");
    }
    saw_frame_address_materialization = true;
  }
  if (!saw_frame_address_materialization) {
    return fail("expected rooted pointer binary fixture to publish frame-address materialization");
  }

  return 0;
}

int check_inline_asm_metadata_stack_layout_activation(
    const prepare::PreparedBirModule& prepared) {
  const auto* memory_object = find_stack_object(prepared, "lv.inline.memory.root");
  const auto* address_object = find_stack_object(prepared, "lv.inline.address.root");
  const auto* missing_object = find_stack_object(prepared, "lv.inline.missing.root");
  if (memory_object == nullptr || address_object == nullptr || missing_object == nullptr) {
    return fail("expected inline-asm stack-layout fixture to produce all local-slot objects");
  }

  if (!memory_object->address_exposed || !memory_object->requires_home_slot) {
    return fail(
        "expected structured inline-asm memory_address metadata to expose the local root");
  }
  if (!address_object->address_exposed || !address_object->requires_home_slot) {
    return fail(
        "expected structured inline-asm address metadata to expose the local root");
  }
  if (missing_object->address_exposed || missing_object->requires_home_slot ||
      missing_object->permanent_home_slot) {
    return fail(
        "expected missing inline-asm memory/address metadata not to create object-specific placement");
  }

  const auto* memory_slot = find_frame_slot(prepared, memory_object->object_id);
  const auto* address_slot = find_frame_slot(prepared, address_object->object_id);
  const auto* missing_slot = find_frame_slot(prepared, missing_object->object_id);
  if (memory_slot == nullptr || address_slot == nullptr) {
    return fail("expected structured inline-asm metadata roots to keep frame-slot storage");
  }
  if (memory_slot->size_bytes != 4 || memory_slot->align_bytes != 4 ||
      address_slot->size_bytes != 8 || address_slot->align_bytes != 8) {
    return fail("expected inline-asm metadata roots to preserve stack object layout");
  }
  if (missing_slot != nullptr) {
    return fail(
        "expected missing inline-asm metadata root to stay out of frame-slot storage");
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
  auto byte_storage_overlay_prepared = prepare_lir_byte_storage_overlay_module();
  if (!byte_storage_overlay_prepared.has_value()) {
    return fail("expected LIR-origin byte-storage overlay fixture to lower to BIR");
  }
  if (const int rc =
          check_byte_storage_overlay_local_slot_activation(*byte_storage_overlay_prepared);
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

  const auto rv64_narrow_sret_home_prepared =
      prepare_sret_param_home_module(riscv_target_profile(), 4, 4);
  if (const int rc =
          check_sret_param_home_rv64_activation(rv64_narrow_sret_home_prepared);
      rc != 0) {
    return rc;
  }
  const auto rv64_wide_sret_home_prepared =
      prepare_sret_param_home_module(riscv_target_profile(), 16, 16);
  if (const int rc =
          check_sret_param_home_rv64_activation(rv64_wide_sret_home_prepared);
      rc != 0) {
    return rc;
  }
  const auto aarch64_narrow_sret_home_prepared =
      prepare_sret_param_home_module(aarch64_pic_target_profile(), 4, 4);
  if (const int rc =
          check_sret_param_home_aarch64_activation(aarch64_narrow_sret_home_prepared, 4, 4);
      rc != 0) {
    return rc;
  }
  const auto aarch64_wide_sret_home_prepared =
      prepare_sret_param_home_module(aarch64_pic_target_profile(), 16, 16);
  if (const int rc =
          check_sret_param_home_aarch64_activation(aarch64_wide_sret_home_prepared, 16, 16);
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

  const auto same_module_formal_prepared =
      prepare_same_module_computed_address_formal_provenance_module(true);
  if (!same_module_formal_prepared.has_value()) {
    return fail("same-module computed-address formal provenance fixture did not lower");
  }
  if (const int rc =
          check_same_module_computed_address_formal_provenance_activation(
              *same_module_formal_prepared);
      rc != 0) {
    return rc;
  }
  const auto external_same_module_formal_prepared =
      prepare_same_module_computed_address_formal_provenance_module(false);
  if (!external_same_module_formal_prepared.has_value()) {
    return fail(
        "external-linkage same-module computed-address formal fixture did not lower");
  }
  if (const int rc =
          check_external_same_module_computed_address_formal_provenance_stays_fail_closed(
              *external_same_module_formal_prepared);
      rc != 0) {
    return rc;
  }

  if (const int rc = check_pointer_value_memory_authority_contract(); rc != 0) {
    return rc;
  }

  if (const int rc = check_global_memory_publication_authority_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_direct_global_return_authority_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_fused_pointer_branch_publication_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_branch_stack_load_authority_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_frame_slot_source_fact_carrier_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_dependency_operand_authority_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_select_edge_source_producer_placement_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_select_carrier_alias_authority_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_populated_immediate_global_store_source_publication();
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

  const auto inline_asm_stack_layout_prepared = prepare_inline_asm_stack_layout_module();
  if (const int rc =
          check_inline_asm_metadata_stack_layout_activation(inline_asm_stack_layout_prepared);
      rc != 0) {
    return rc;
  }

  return 0;
}
