#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/bir/lir_to_bir/lowering.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/codegen/calls.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/query.hpp"
#include "src/backend/mir/x86/x86.hpp"
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/call_plans.hpp"
#include "src/backend/prealloc/calls.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const bir::Function* find_function(const bir::Module& module, std::string_view function_name) {
  for (const auto& function : module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

const bir::CallInst* find_first_call(const bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* call = std::get_if<bir::CallInst>(&inst); call != nullptr) {
        return call;
      }
    }
  }
  return nullptr;
}

const bir::CallInst* find_call_with_destination(const bir::Function& function,
                                                std::string_view destination_name) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<bir::CallInst>(&inst);
      if (call == nullptr || call->callee != "llvm.va_arg.aggregate" ||
          call->args.empty() || call->args.front().name != destination_name) {
        continue;
      }
      return call;
    }
  }
  return nullptr;
}

int count_calls_to(const bir::Function& function, std::string_view callee) {
  int count = 0;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* call = std::get_if<bir::CallInst>(&inst);
          call != nullptr && call->callee == callee) {
        ++count;
      }
    }
  }
  return count;
}

int count_block_calls_to(const bir::Block& block, std::string_view callee) {
  int count = 0;
  for (const auto& inst : block.insts) {
    if (const auto* call = std::get_if<bir::CallInst>(&inst);
        call != nullptr && call->callee == callee) {
      ++count;
    }
  }
  return count;
}

const prepare::PreparedLivenessFunction* find_liveness_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  for (const auto& function : prepared.liveness.functions) {
    if (function.function_name == function_id) {
      return &function;
    }
  }
  return nullptr;
}

const prepare::PreparedStackObject* find_stack_object(const prepare::PreparedBirModule& prepared,
                                                      std::string_view object_name) {
  for (const auto& object : prepared.stack_layout.objects) {
    if (prepare::prepared_stack_object_name(prepared.names, object) == object_name) {
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

const prepare::PreparedControlFlowFunction* find_control_flow_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_control_flow_function(prepared.control_flow, function_id);
}

const prepare::PreparedControlFlowBlock* find_control_flow_block(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedControlFlowFunction& function,
    std::string_view block_label) {
  const auto label_id = prepared.names.block_labels.find(block_label);
  if (label_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  for (const auto& block : function.blocks) {
    if (block.block_label == label_id) {
      return &block;
    }
  }
  return nullptr;
}

const prepare::PreparedCallPlansFunction* find_call_plans_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_call_plans(prepared, function_id);
}

const prepare::PreparedFramePlanFunction* find_frame_plan_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_frame_plan(prepared, function_id);
}

const prepare::PreparedStoragePlanFunction* find_storage_plan_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_storage_plan(prepared, function_id);
}

const prepare::PreparedStoragePlanValue* find_storage_value(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedStoragePlanFunction& function,
    std::string_view value_name) {
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

const prepare::PreparedAddressMaterialization* find_address_materialization(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name,
    std::string_view block_label,
    std::size_t inst_index,
    std::string_view result_value_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  const auto block_label_id = prepared.names.block_labels.find(block_label);
  const auto result_value_id = prepared.names.value_names.find(result_value_name);
  if (function_id == c4c::kInvalidFunctionName ||
      block_label_id == c4c::kInvalidBlockLabel ||
      result_value_id == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* function_addressing = prepare::find_prepared_addressing(prepared, function_id);
  if (function_addressing == nullptr) {
    return nullptr;
  }
  for (const auto& materialization : function_addressing->address_materializations) {
    if (materialization.block_label == block_label_id &&
        materialization.inst_index == inst_index &&
        materialization.result_value_name ==
            std::optional<c4c::ValueNameId>{result_value_id}) {
      return &materialization;
    }
  }
  return nullptr;
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

std::string grouped_span_summary(prepare::PreparedRegisterBank bank,
                                 std::optional<std::string_view> register_name,
                                 std::size_t contiguous_width,
                                 const std::vector<std::string>& occupied_register_names) {
  std::string summary = std::string(prepare::prepared_register_bank_name(bank)) + ":";
  summary += register_name.has_value() ? std::string(*register_name) : std::string("<none>");
  summary += "/w" + std::to_string(contiguous_width);
  if (!occupied_register_names.empty()) {
    summary += "[";
    for (std::size_t index = 0; index < occupied_register_names.size(); ++index) {
      if (index != 0) {
        summary += ",";
      }
      summary += occupied_register_names[index];
    }
    summary += "]";
  }
  return summary;
}

std::optional<std::string_view> optional_string_view(
    const std::optional<std::string>& value) {
  if (!value.has_value()) {
    return std::nullopt;
  }
  return std::string_view(*value);
}

std::string grouped_call_argument_destination_span_summary(
    const prepare::PreparedCallArgumentPlan& argument) {
  return grouped_span_summary(argument.destination_register_bank.value_or(
                                  prepare::PreparedRegisterBank::None),
                              optional_string_view(argument.destination_register_name),
                              argument.destination_contiguous_width,
                              argument.destination_occupied_register_names);
}

std::string grouped_call_result_source_span_summary(
    const prepare::PreparedCallResultPlan& result) {
  return grouped_span_summary(result.source_register_bank.value_or(
                                  prepare::PreparedRegisterBank::None),
                              optional_string_view(result.source_register_name),
                              result.source_contiguous_width,
                              result.source_occupied_register_names);
}

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

c4c::TargetProfile riscv_target_profile() {
  return c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");
}

c4c::TargetProfile aarch64_target_profile() {
  return c4c::target_profile_from_triple("aarch64-linux-gnu");
}

prepare::PreparedBirModule prepare_module(const bir::Module& module) {
  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(module, x86_target_profile(), options);
}

prepare::PreparedBirModule prepare_riscv_module(const bir::Module& module) {
  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(module, riscv_target_profile(), options);
}

prepare::PreparedBirModule prepare_riscv_float_abi_module(
    const bir::Module& module,
    std::string_view target_triple = "riscv64gc-unknown-linux-gnu") {
  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module, c4c::target_profile_from_triple(target_triple), options);
}

prepare::PreparedBirModule prepare_aarch64_module(const bir::Module& module) {
  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(
      module, aarch64_target_profile(), options);
}

bir::Module make_f128_symbol_backed_load_local_addressing_contract_module() {
  bir::Module module;
  module.target_triple = "aarch64-linux-gnu";

  const auto source_slot_id = module.names.slot_names.intern("hfa31");
  const auto source_link_name_id = module.names.link_names.intern("hfa31");
  module.globals.push_back(bir::Global{
      .name = "hfa31",
      .link_name_id = source_link_name_id,
      .type = bir::TypeKind::F128,
      .is_extern = true,
      .size_bytes = 16,
      .align_bytes = 16,
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });

  bir::Function function;
  function.name = "f128_symbol_backed_load_local_addressing_contract";
  function.return_type = bir::TypeKind::Void;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F128, "loaded.hfa31"),
      .slot_name = "hfa31",
      .slot_id = source_slot_id,
      .byte_offset = 0,
      .align_bytes = 16,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .byte_offset = 0,
              .size_bytes = 16,
              .align_bytes = 16,
          },
  });
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
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

prepare::PreparedBirModule prepare_grouped_riscv_module_with_overrides(
    const bir::Module& module,
    std::initializer_list<std::pair<std::string_view, std::size_t>> overrides) {
  prepare::PreparedBirModule seeded;
  seeded.module = module;
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
  for (const auto& [value_name, contiguous_width] : overrides) {
    set_register_group_override(prepared,
                                prepared.module.functions.back().name,
                                value_name,
                                prepare::PreparedRegisterClass::Vector,
                                contiguous_width);
  }

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_grouped_spill_reload_contract_module() {
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
  function.name = "grouped_spill_reload_contract";
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
                              "grouped_spill_reload_contract",
                              "carry",
                              prepare::PreparedRegisterClass::Vector,
                              16);
  set_register_group_override(prepared,
                              "grouped_spill_reload_contract",
                              "local0",
                              prepare::PreparedRegisterClass::Vector,
                              16);
  set_register_group_override(prepared,
                              "grouped_spill_reload_contract",
                              "hot",
                              prepare::PreparedRegisterClass::Vector,
                              16);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_general_grouped_spill_reload_contract_module() {
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
  function.name = "general_grouped_spill_reload_contract";
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
                              "general_grouped_spill_reload_contract",
                              "carry",
                              prepare::PreparedRegisterClass::General,
                              2);
  set_register_group_override(prepared,
                              "general_grouped_spill_reload_contract",
                              "hot",
                              prepare::PreparedRegisterClass::General,
                              2);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_float_grouped_spill_reload_contract_module() {
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
  function.name = "float_grouped_spill_reload_contract";
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
                              "float_grouped_spill_reload_contract",
                              "carry",
                              prepare::PreparedRegisterClass::Float,
                              2);
  set_register_group_override(prepared,
                              "float_grouped_spill_reload_contract",
                              "hot",
                              prepare::PreparedRegisterClass::Float,
                              2);

  prepare::BirPreAlloc regalloc_planner(std::move(prepared), {});
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
}

bir::Module make_fixed_frame_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "fixed_frame_contract";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed",
      .value = bir::Value::immediate_i32(42),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.fixed"),
      .slot_name = "lv.fixed",
      .align_bytes = 4,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "tmp.fixed")};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_call_contract_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "extern_i32";
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
  caller.name = "call_contract";
  caller.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.call"),
      .callee = "extern_i32",
      .args = {bir::Value::immediate_i32(7)},
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
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "tmp.call")};
  caller.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(caller));
  return module;
}

bir::Module make_same_module_i16_call_argument_contract_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "same_module_i16";
  callee.return_type = bir::TypeKind::I16;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I16,
      .name = "value",
      .size_bytes = 2,
      .align_bytes = 2,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I16,
          .size_bytes = 2,
          .align_bytes = 2,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  bir::Block callee_entry;
  callee_entry.label = "entry";
  callee_entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I16, "value")};
  callee.blocks.push_back(std::move(callee_entry));
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "same_module_i16_call_argument_contract";
  caller.return_type = bir::TypeKind::I16;
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "lv.i16",
      .type = bir::TypeKind::I16,
      .size_bytes = 2,
      .align_bytes = 2,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.i16",
      .value = bir::Value::immediate_i16(7),
      .align_bytes = 2,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I16, "tmp.i16"),
      .slot_name = "lv.i16",
      .align_bytes = 2,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I16, "tmp.call"),
      .callee = "same_module_i16",
      .args = {bir::Value::named(bir::TypeKind::I16, "tmp.i16")},
      .return_type_name = "i16",
      .return_type = bir::TypeKind::I16,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I16,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I16, "tmp.call")};
  caller.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(caller));
  return module;
}

bir::Module make_stack_result_slot_contract_module() {
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
  caller.name = "stack_result_slot_contract";
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
  return module;
}

bir::Module make_float_call_contract_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function callee;
  callee.name = "extern_f32";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::F32;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "value",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "float_call_contract";
  caller.return_type = bir::TypeKind::F32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::F32, "tmp.float.call"),
      .callee = "extern_f32",
      .args = {bir::Value::named(bir::TypeKind::F32, "arg.float")},
      .arg_types = {bir::TypeKind::F32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::F32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      }},
      .return_type_name = "f32",
      .return_type = bir::TypeKind::F32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::F32,
          .primary_class = bir::AbiValueClass::Sse,
      },
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::F32, "tmp.float.call")};
  caller.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(caller));
  return module;
}

bir::Module make_cross_call_preservation_contract_module() {
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
  function.name = "cross_call_preservation_contract";
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
  return module;
}

bir::Module make_prior_preservation_source_selection_contract_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "selection_helper";
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
  function.name = "prior_preservation_source_selection_contract";
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
      .callee = "selection_helper",
      .args = {bir::Value::named(bir::TypeKind::I32, "pre.only")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.consume"),
      .callee = "selection_helper",
      .args = {bir::Value::named(bir::TypeKind::I32, "carry")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "call.consume")};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_aarch64_formal_preservation_source_endpoint_contract_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "aarch64_formal_preservation_source_endpoint_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "live.formal",
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

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Slt,
      .result = bir::Value::named(bir::TypeKind::I1, "is.base"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "live.formal"),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "is.base"),
      .true_label = "base",
      .false_label = "recurse",
  };
  function.blocks.push_back(std::move(entry));

  bir::Block base;
  base.label = "base";
  base.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "live.formal")};
  function.blocks.push_back(std::move(base));

  bir::Block recurse;
  recurse.label = "recurse";
  recurse.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "dec"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "live.formal"),
      .rhs = bir::Value::immediate_i32(1),
  });
  recurse.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.out"),
      .callee = "aarch64_formal_preservation_source_endpoint_contract",
      .args = {bir::Value::named(bir::TypeKind::I32, "dec")},
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
  recurse.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Mul,
      .result = bir::Value::named(bir::TypeKind::I32, "after"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "live.formal"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "call.out"),
  });
  recurse.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "after")};
  function.blocks.push_back(std::move(recurse));

  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_local_frame_address_source_selection_contract_module() {
  bir::Module module;
  module.target_triple = "aarch64-linux-gnu";

  bir::Function decl;
  decl.name = "consume_local_address_selection";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "arg0",
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
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "local_frame_address_source_selection_contract";
  function.return_type = bir::TypeKind::Void;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "local.aggregate.0",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "local.aggregate"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "local.aggregate.0"),
      .rhs = bir::Value::immediate_i64(0),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "consume_local_address_selection",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "local.aggregate")},
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
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_derived_local_frame_address_source_selection_contract_module() {
  bir::Module module;
  module.target_triple = "aarch64-linux-gnu";

  bir::Function decl;
  decl.name = "consume_derived_local_address_selection";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "arg0",
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
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "derived_local_frame_address_source_selection_contract";
  function.return_type = bir::TypeKind::Void;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "local.aggregate.0",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "local.aggregate"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "local.aggregate.0"),
      .rhs = bir::Value::immediate_i64(4),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "consume_derived_local_address_selection",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "local.aggregate")},
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
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_stack_cross_call_preservation_contract_module() {
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
  function.name = "stack_cross_call_preservation_contract";
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
  return module;
}

bir::Module make_grouped_cross_call_preservation_contract_module() {
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
  function.name = "grouped_cross_call_preservation_contract";
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
  return module;
}

bir::Module make_indirect_call_contract_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function caller;
  caller.name = "indirect_call_contract";
  caller.return_type = bir::TypeKind::I64;
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
      .result = bir::Value::named(bir::TypeKind::I64, "tmp.indirect.call"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "callee.ptr"),
      .args = {bir::Value::immediate_i64(11)},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i64",
      .return_type = bir::TypeKind::I64,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I64,
          .primary_class = bir::AbiValueClass::Integer,
      },
      .is_indirect = true,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I64, "tmp.indirect.call")};
  caller.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(caller));
  return module;
}

bir::Module make_memory_return_call_contract_module() {
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
  caller.name = "memory_return_call_contract";
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
  return module;
}

bir::Module make_stack_argument_slot_contract_module() {
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
  caller.name = "stack_argument_slot_contract";
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
  return module;
}

bir::Module make_call_argument_source_shape_module() {
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
  caller.name = "call_argument_source_shape_contract";
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
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::Ptr, "derived.seed"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "loaded.ptr"),
      .rhs = bir::Value::immediate_i32(4),
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
  return module;
}

bir::Module make_direct_global_select_chain_call_argument_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  const c4c::LinkNameId global_id = module.names.link_names.intern("extern_i32");
  module.globals.push_back(bir::Global{
      .name = "extern_i32",
      .link_name_id = global_id,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Function callee;
  callee.name = "consume_i32";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::Void;
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
  caller.name = "direct_global_select_chain_call_argument_contract";
  caller.return_type = bir::TypeKind::Void;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded.global"),
      .global_name_id = global_id,
  });
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "selected.global"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
      .true_value = bir::Value::named(bir::TypeKind::I32, "loaded.global"),
      .false_value = bir::Value::immediate_i32(0),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "consume_i32",
      .args = {bir::Value::named(bir::TypeKind::I32, "selected.global")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{};
  caller.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(caller));
  return module;
}

bir::Module make_call_argument_symbol_link_name_id_mismatch_module() {
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
  callee.name = "extern_consume_ptr";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::Void;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "value",
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
  caller.name = "call_argument_symbol_mismatch_contract";
  caller.return_type = bir::TypeKind::Void;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "extern_consume_ptr",
      .args = {bir::Value::named_symbol_pointer("@stale_extern_data", extern_data_id)},
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
  entry.terminator = bir::ReturnTerminator{};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));
  return module;
}

bir::Module make_call_wrapper_kind_contract_module() {
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
  caller.name = "call_wrapper_kind_contract";
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
  return module;
}

bir::Module make_call_wrapper_link_name_id_contract_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  const c4c::LinkNameId same_module_link_name_id =
      module.names.link_names.intern("same_module_id_target");

  bir::Function same_module_callee;
  same_module_callee.name = "same_module_id_target";
  same_module_callee.link_name_id = same_module_link_name_id;
  same_module_callee.return_type = bir::TypeKind::Void;
  bir::Block same_module_entry;
  same_module_entry.label = "entry";
  same_module_entry.terminator = bir::ReturnTerminator{};
  same_module_callee.blocks.push_back(std::move(same_module_entry));
  module.functions.push_back(std::move(same_module_callee));

  bir::Function raw_compat_extern;
  raw_compat_extern.name = "raw_compat_extern";
  raw_compat_extern.is_declaration = true;
  raw_compat_extern.return_type = bir::TypeKind::Void;
  module.functions.push_back(std::move(raw_compat_extern));

  bir::Function caller;
  caller.name = "call_wrapper_link_name_id_contract";
  caller.return_type = bir::TypeKind::Void;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee_link_name_id = same_module_link_name_id,
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "raw_compat_extern",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "raw_compat_extern",
      .callee_link_name_id = same_module_link_name_id,
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{};
  caller.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(caller));
  return module;
}

bir::Module make_dynamic_stack_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "dynamic_stack_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "n",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "saved.sp"),
      .callee = "llvm.stacksave",
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "vla.buf"),
      .callee = "llvm.dynamic_alloca.i32",
      .args = {bir::Value::named(bir::TypeKind::I64, "n")},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.stackrestore",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "saved.sp")},
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
  return module;
}

bir::Module make_dynamic_stack_stale_raw_block_label_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  const auto canonical_entry = module.names.block_labels.intern("entry.authoritative");

  bir::Function function;
  function.name = "dynamic_stack_stale_raw_block_label_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "stale.entry.raw";
  entry.label_id = canonical_entry;
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "saved.sp"),
      .callee = "llvm.stacksave",
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "vla.buf"),
      .callee = "llvm.dynamic_alloca.i32",
      .args = {bir::Value::immediate_i64(16)},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_cross_block_dynamic_stack_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "cross_block_dynamic_stack_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "n",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.total",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.xs",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "saved.sp"),
      .callee = "llvm.stacksave",
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.total",
      .value = bir::Value::immediate_i32(0),
      .align_bytes = 4,
  });
  entry.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "loop.buf"),
      .callee = "llvm.dynamic_alloca.i32",
      .args = {bir::Value::named(bir::TypeKind::I64, "n")},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  loop.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.xs",
      .value = bir::Value::named(bir::TypeKind::Ptr, "loop.buf"),
      .align_bytes = 8,
  });
  loop.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.total",
      .value = bir::Value::immediate_i32(9),
      .align_bytes = 4,
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond.keep_looping"),
      .true_label = "restore_then_loop",
      .false_label = "exit",
  };

  bir::Block restore_then_loop;
  restore_then_loop.label = "restore_then_loop";
  restore_then_loop.insts.push_back(bir::CallInst{
      .callee = "llvm.stackrestore",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "saved.sp")},
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
  restore_then_loop.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block exit;
  exit.label = "exit";
  exit.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "total.out"),
      .slot_name = "lv.total",
      .align_bytes = 4,
  });
  exit.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "total.out")};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(loop));
  function.blocks.push_back(std::move(restore_then_loop));
  function.blocks.push_back(std::move(exit));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_nested_dynamic_stack_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "nested_dynamic_stack_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "n",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.outer.ptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.inner.ptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "saved.outer"),
      .callee = "llvm.stacksave",
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "outer.buf"),
      .callee = "llvm.dynamic_alloca.i32",
      .args = {bir::Value::named(bir::TypeKind::I64, "n")},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.outer.ptr",
      .value = bir::Value::named(bir::TypeKind::Ptr, "outer.buf"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed",
      .value = bir::Value::immediate_i32(5),
      .align_bytes = 4,
  });
  entry.terminator = bir::BranchTerminator{.target_label = "inner"};

  bir::Block inner;
  inner.label = "inner";
  inner.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "saved.inner"),
      .callee = "llvm.stacksave",
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  inner.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "inner.buf"),
      .callee = "llvm.dynamic_alloca.i32",
      .args = {bir::Value::named(bir::TypeKind::I64, "n")},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  inner.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.inner.ptr",
      .value = bir::Value::named(bir::TypeKind::Ptr, "inner.buf"),
      .align_bytes = 8,
  });
  inner.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond.inner"),
      .true_label = "restore_inner",
      .false_label = "restore_outer",
  };

  bir::Block restore_inner;
  restore_inner.label = "restore_inner";
  restore_inner.insts.push_back(bir::CallInst{
      .callee = "llvm.stackrestore",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "saved.inner")},
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
  restore_inner.terminator = bir::BranchTerminator{.target_label = "restore_outer"};

  bir::Block restore_outer;
  restore_outer.label = "restore_outer";
  restore_outer.insts.push_back(bir::CallInst{
      .callee = "llvm.stackrestore",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "saved.outer")},
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
  restore_outer.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "fixed.out"),
      .slot_name = "lv.fixed",
      .align_bytes = 4,
  });
  restore_outer.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "fixed.out")};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(inner));
  function.blocks.push_back(std::move(restore_inner));
  function.blocks.push_back(std::move(restore_outer));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_branch_restore_dynamic_stack_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "branch_restore_dynamic_stack_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "n",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "saved.sp"),
      .callee = "llvm.stacksave",
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "buf"),
      .callee = "llvm.dynamic_alloca.i32",
      .args = {bir::Value::named(bir::TypeKind::I64, "n")},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond.branch"),
      .true_label = "restore_true",
      .false_label = "restore_false",
  };

  bir::Block restore_true;
  restore_true.label = "restore_true";
  restore_true.insts.push_back(bir::CallInst{
      .callee = "llvm.stackrestore",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "saved.sp")},
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
  restore_true.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block restore_false;
  restore_false.label = "restore_false";
  restore_false.insts.push_back(bir::CallInst{
      .callee = "llvm.stackrestore",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "saved.sp")},
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
  restore_false.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(restore_true));
  function.blocks.push_back(std::move(restore_false));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_variadic_nested_dynamic_stack_call_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

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

  bir::Function function;
  function.name = "variadic_nested_dynamic_stack_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I64,
      .name = "n",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.fixed",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.outer.ptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.inner.ptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "saved.outer"),
      .callee = "llvm.stacksave",
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "outer.buf"),
      .callee = "llvm.dynamic_alloca.i32",
      .args = {bir::Value::named(bir::TypeKind::I64, "n")},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.outer.ptr",
      .value = bir::Value::named(bir::TypeKind::Ptr, "outer.buf"),
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed",
      .value = bir::Value::immediate_i32(7),
      .align_bytes = 4,
  });
  entry.terminator = bir::BranchTerminator{.target_label = "inner"};

  bir::Block inner;
  inner.label = "inner";
  inner.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "saved.inner"),
      .callee = "llvm.stacksave",
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  inner.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "inner.buf"),
      .callee = "llvm.dynamic_alloca.i32",
      .args = {bir::Value::named(bir::TypeKind::I64, "n")},
      .arg_types = {bir::TypeKind::I64},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::Ptr,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  inner.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.inner.ptr",
      .value = bir::Value::named(bir::TypeKind::Ptr, "inner.buf"),
      .align_bytes = 8,
  });
  inner.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .slot_name = "lv.fixed",
      .align_bytes = 4,
  });
  inner.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "tmp.variadic"),
      .callee = "extern_variadic_i32",
      .args = {bir::Value::named(bir::TypeKind::I32, "carry"),
               bir::Value::immediate_f32_bits(0x40800000)},
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
  inner.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.fixed",
      .value = bir::Value::named(bir::TypeKind::I32, "carry"),
      .align_bytes = 4,
  });
  inner.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond.inner"),
      .true_label = "restore_inner",
      .false_label = "restore_outer",
  };

  bir::Block restore_inner;
  restore_inner.label = "restore_inner";
  restore_inner.insts.push_back(bir::CallInst{
      .callee = "llvm.stackrestore",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "saved.inner")},
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
  restore_inner.terminator = bir::BranchTerminator{.target_label = "restore_outer"};

  bir::Block restore_outer;
  restore_outer.label = "restore_outer";
  restore_outer.insts.push_back(bir::CallInst{
      .callee = "llvm.stackrestore",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "saved.outer")},
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
  restore_outer.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "fixed.out"),
      .slot_name = "lv.fixed",
      .align_bytes = 4,
  });
  restore_outer.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "fixed.out")};

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(inner));
  function.blocks.push_back(std::move(restore_inner));
  function.blocks.push_back(std::move(restore_outer));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_aarch64_scalar_parameter_subtract_module() {
  bir::Module module;
  module.target_triple = "aarch64-linux-gnu";

  bir::Function function;
  function.name = "scalar_param_sub";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "%p.c",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "%p.b",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%p.c"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "%p.b"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "%t0")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_riscv_fpr_formal_identity_contract_module(std::string_view target_triple) {
  bir::Module module;
  module.target_triple = std::string(target_triple);

  bir::Function function;
  function.name = "riscv_fpr_formal_identity_contract";
  function.return_type = bir::TypeKind::F64;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%p.a",
      .size_bytes = 4,
      .align_bytes = 4,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Sse,
          .passed_in_register = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%t0"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%p.a"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::F64, "%t0")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_riscv_i16_formal_identity_contract_module() {
  bir::Module module;
  module.target_triple = "riscv64-linux-gnu";

  bir::Function function;
  function.name = "riscv_i16_formal_identity_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I16,
      .name = "%p.x",
      .size_bytes = 2,
      .align_bytes = 2,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I16,
          .size_bytes = 2,
          .align_bytes = 2,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I16,
      .name = "%p.y",
      .size_bytes = 2,
      .align_bytes = 2,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I16,
          .size_bytes = 2,
          .align_bytes = 2,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .operand = bir::Value::named(bir::TypeKind::I16, "%p.x"),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .operand = bir::Value::named(bir::TypeKind::I16, "%p.y"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%t2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "%t1"),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "%t2")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int check_riscv_fpr_formal_home_publishes_target_identity(std::string_view target_triple) {
  const auto prepared =
      prepare_riscv_float_abi_module(
          make_riscv_fpr_formal_identity_contract_module(target_triple), target_triple);
  const auto* locations =
      prepare::find_prepared_value_location_function(
          prepared, "riscv_fpr_formal_identity_contract");
  const auto* home = locations == nullptr
                         ? nullptr
                         : prepare::find_prepared_value_home(
                               prepared.names, *locations, "%p.a");
  const auto* result_home = locations == nullptr
                                ? nullptr
                                : prepare::find_prepared_value_home(
                                      prepared.names, *locations, "%t0");
  if (home == nullptr) {
    return fail("rv64 FPR formal identity contract: missing prepared formal home");
  }
  if (home->kind != prepare::PreparedValueHomeKind::Register ||
      home->register_name != std::optional<std::string>{"fa0"} ||
      !home->target_register_identity.has_value()) {
    return fail("rv64 FPR formal identity contract: formal did not publish FPR register identity");
  }
  const auto& identity = *home->target_register_identity;
  if (identity.target_arch != c4c::TargetArch::Riscv64 ||
      identity.bank != prepare::PreparedRegisterBank::Fpr ||
      identity.register_class != prepare::PreparedRegisterClass::Float ||
      identity.physical_index != 10) {
    return fail("rv64 FPR formal identity contract: formal identity did not name physical fa0");
  }
  if (result_home == nullptr) {
    return fail("rv64 FPR formal identity contract: missing prepared FPExt result home");
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_home->register_name != std::optional<std::string>{"ft0"} ||
      !result_home->target_register_identity.has_value()) {
    return fail("rv64 FPR formal identity contract: FPExt result did not publish FPR register identity");
  }
  const auto& result_identity = *result_home->target_register_identity;
  if (result_identity.target_arch != c4c::TargetArch::Riscv64 ||
      result_identity.bank != prepare::PreparedRegisterBank::Fpr ||
      result_identity.register_class != prepare::PreparedRegisterClass::Float ||
      result_identity.physical_index != 0) {
    return fail("rv64 FPR formal identity contract: FPExt result identity did not name physical ft0");
  }
  return 0;
}

int check_riscv_i16_formal_home_publishes_gpr_identity() {
  const auto prepared =
      prepare_riscv_module(make_riscv_i16_formal_identity_contract_module());
  const auto* locations =
      prepare::find_prepared_value_location_function(
          prepared, "riscv_i16_formal_identity_contract");
  const auto* storage_plan =
      find_storage_plan_function(prepared, "riscv_i16_formal_identity_contract");
  const auto* x_home = locations == nullptr
                           ? nullptr
                           : prepare::find_prepared_value_home(
                                 prepared.names, *locations, "%p.x");
  const auto* y_home = locations == nullptr
                           ? nullptr
                           : prepare::find_prepared_value_home(
                                 prepared.names, *locations, "%p.y");
  const auto* x_storage =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "%p.x");
  const auto* y_storage =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "%p.y");

  if (x_home == nullptr || y_home == nullptr ||
      x_storage == nullptr || y_storage == nullptr) {
    return fail("rv64 i16 formal identity contract: missing prepared formal facts");
  }
  const auto check_home = [](const prepare::PreparedValueHome& home,
                             const char* register_name,
                             std::size_t physical_index) -> bool {
    return home.kind == prepare::PreparedValueHomeKind::Register &&
           home.register_name == std::optional<std::string>{register_name} &&
           home.target_register_identity.has_value() &&
           home.target_register_identity->target_arch == c4c::TargetArch::Riscv64 &&
           home.target_register_identity->bank == prepare::PreparedRegisterBank::Gpr &&
           home.target_register_identity->register_class ==
               prepare::PreparedRegisterClass::General &&
           home.target_register_identity->physical_index == physical_index;
  };
  if (!check_home(*x_home, "a0", 10) || !check_home(*y_home, "a1", 11)) {
    return fail("rv64 i16 formal identity contract: formal did not publish GPR identity");
  }

  const auto check_storage =
      [](const prepare::PreparedStoragePlanValue& storage,
         const char* register_name) -> bool {
    return storage.encoding == prepare::PreparedStorageEncodingKind::Register &&
           storage.bank == prepare::PreparedRegisterBank::Gpr &&
           storage.register_name == std::optional<std::string>{register_name} &&
           storage.occupied_register_names == std::vector<std::string>{register_name};
  };
  if (!check_storage(*x_storage, "a0") || !check_storage(*y_storage, "a1")) {
    return fail("rv64 i16 formal identity contract: storage did not publish GPR register facts");
  }
  return 0;
}

int check_aarch64_scalar_parameter_homes_and_storage_contract() {
  const auto prepared = prepare_aarch64_module(make_aarch64_scalar_parameter_subtract_module());
  const auto* locations =
      prepare::find_prepared_value_location_function(prepared, "scalar_param_sub");
  const auto* storage_plan = find_storage_plan_function(prepared, "scalar_param_sub");
  if (locations == nullptr || storage_plan == nullptr) {
    return fail("aarch64 scalar parameter contract: missing value homes or storage plan");
  }

  const auto* lhs_home = prepare::find_prepared_value_home(prepared.names, *locations, "%p.c");
  const auto* rhs_home = prepare::find_prepared_value_home(prepared.names, *locations, "%p.b");
  const auto* result_home = prepare::find_prepared_value_home(prepared.names, *locations, "%t0");
  const auto* lhs_storage = find_storage_value(prepared, *storage_plan, "%p.c");
  const auto* rhs_storage = find_storage_value(prepared, *storage_plan, "%p.b");
  const auto* result_storage = find_storage_value(prepared, *storage_plan, "%t0");

  if (lhs_home == nullptr || rhs_home == nullptr || result_home == nullptr ||
      lhs_storage == nullptr || rhs_storage == nullptr || result_storage == nullptr) {
    return fail("aarch64 scalar parameter contract: missing parameter/result facts");
  }
  if (lhs_home->kind != prepare::PreparedValueHomeKind::Register ||
      rhs_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_home->kind != prepare::PreparedValueHomeKind::Register ||
      lhs_home->register_name != std::optional<std::string>{"x0"} ||
      rhs_home->register_name != std::optional<std::string>{"x1"} ||
      result_home->register_name != std::optional<std::string>{"x13"}) {
    return fail("aarch64 scalar parameter contract: value homes lost ABI register authority");
  }
  if (lhs_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      rhs_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      lhs_storage->bank != prepare::PreparedRegisterBank::Gpr ||
      rhs_storage->bank != prepare::PreparedRegisterBank::Gpr ||
      result_storage->bank != prepare::PreparedRegisterBank::Gpr ||
      lhs_storage->register_name != std::optional<std::string>{"x0"} ||
      rhs_storage->register_name != std::optional<std::string>{"x1"} ||
      result_storage->register_name != std::optional<std::string>{"x13"} ||
      lhs_storage->occupied_register_names != std::vector<std::string>{"x0"} ||
      rhs_storage->occupied_register_names != std::vector<std::string>{"x1"} ||
      result_storage->occupied_register_names != std::vector<std::string>{"x13"}) {
    return fail("aarch64 scalar parameter contract: storage plan lost ABI register authority");
  }
  if (lhs_storage->register_placement.has_value() ||
      rhs_storage->register_placement.has_value() ||
      !result_storage->register_placement.has_value()) {
    return fail("aarch64 scalar parameter contract: parameter storage should be spelling-authoritative while result storage keeps placement authority");
  }
  return 0;
}

int check_fixed_frame_contract() {
  const auto prepared = prepare_module(make_fixed_frame_module());
  const auto* object = find_stack_object(prepared, "lv.fixed");
  if (object == nullptr) {
    return fail("fixed-frame contract: missing prepared stack object for lv.fixed");
  }
  const auto* slot = find_frame_slot(prepared, object->object_id);
  if (slot == nullptr) {
    return fail("fixed-frame contract: missing frame slot for lv.fixed");
  }
  if (prepared.stack_layout.frame_size_bytes != 4 ||
      prepared.stack_layout.frame_alignment_bytes != 4) {
    return fail("fixed-frame contract: unexpected frame size/alignment");
  }
  if (slot->size_bytes != 4 || slot->align_bytes != 4) {
    return fail("fixed-frame contract: lv.fixed lost its slot size/alignment");
  }
  const auto function_id = prepared.names.function_names.find("fixed_frame_contract");
  const auto* frame_plan = prepare::find_prepared_frame_plan(prepared, function_id);
  if (frame_plan == nullptr || frame_plan->frame_size_bytes != 4 ||
      frame_plan->frame_alignment_bytes != 4 || frame_plan->has_dynamic_stack ||
      frame_plan->uses_frame_pointer_for_fixed_slots ||
      frame_plan->frame_slot_order.size() != 1) {
    return fail("fixed-frame contract: frame_plan no longer publishes the fixed frame correctly");
  }
  return 0;
}

int check_call_contract() {
  const auto prepared = prepare_module(make_call_contract_module());
  const auto* function = find_function(prepared.module, "call_contract");
  if (function == nullptr || function->blocks.empty() || function->blocks.front().insts.empty()) {
    return fail("call contract: missing caller body");
  }
  const auto* call = std::get_if<bir::CallInst>(&function->blocks.front().insts.front());
  if (call == nullptr) {
    return fail("call contract: first instruction is no longer a call");
  }
  if (call->callee != "extern_i32" || call->arg_abi.size() != 1 || !call->result_abi.has_value()) {
    return fail("call contract: semantic call ABI metadata is incomplete");
  }
  if (!call->arg_abi.front().passed_in_register ||
      call->arg_abi.front().primary_class != bir::AbiValueClass::Integer) {
    return fail("call contract: argument ABI metadata lost integer-register classification");
  }
  if (call->result_abi->primary_class != bir::AbiValueClass::Integer) {
    return fail("call contract: result ABI metadata lost integer classification");
  }
  const auto* liveness = find_liveness_function(prepared, "call_contract");
  if (liveness == nullptr || liveness->call_points.size() != 1) {
    return fail("call contract: prepared liveness no longer records the call point");
  }
  const auto* call_plans = find_call_plans_function(prepared, "call_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1) {
    return fail("call contract: call_plans no longer publish the direct call");
  }
  const auto& call_plan = call_plans->calls.front();
  if (call_plan.is_indirect || !call_plan.direct_callee_name.has_value() ||
      *call_plan.direct_callee_name != "extern_i32" || call_plan.arguments.size() != 1 ||
      !call_plan.result.has_value()) {
    return fail("call contract: call_plans lost direct-call ownership details");
  }
  if (call_plan.arguments.front().value_bank != prepare::PreparedRegisterBank::Gpr ||
      call_plan.arguments.front().source_encoding !=
          prepare::PreparedStorageEncodingKind::Immediate ||
      !call_plan.arguments.front().source_literal.has_value() ||
      call_plan.arguments.front().source_literal->immediate != 7 ||
      !call_plan.arguments.front().destination_register_name.has_value() ||
      call_plan.arguments.front().destination_register_bank != prepare::PreparedRegisterBank::Gpr) {
    return fail("call contract: call_plans lost immediate integer argument authority");
  }
  if (!call_plan.arguments.front().destination_register_placement.has_value() ||
      call_plan.arguments.front().destination_register_placement->bank !=
          prepare::PreparedRegisterBank::Gpr ||
      call_plan.arguments.front().destination_register_placement->pool !=
          prepare::PreparedRegisterSlotPool::CallArgument ||
      call_plan.arguments.front().destination_register_placement->slot_index != 0 ||
      call_plan.arguments.front().destination_register_placement->contiguous_width != 1) {
    return fail("call contract: argument ABI destination lost structured placement identity");
  }
  if (call_plan.result->value_bank != prepare::PreparedRegisterBank::Gpr ||
      call_plan.result->source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !call_plan.result->source_register_name.has_value() ||
      call_plan.result->source_register_bank != prepare::PreparedRegisterBank::Gpr) {
    return fail("call contract: call_plans lost integer result ABI source");
  }
  const auto result_late_publication =
      prepare::find_prepared_call_result_late_publication(*call_plan.result);
  if (!result_late_publication.source_register_publication_available ||
      result_late_publication.current_block_publication_consumption_available) {
    return fail("call contract: call result late-publication query overclaimed current-block visibility");
  }
  const prepare::PreparedCallResultPlan aliased_result{
      .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_value_id = prepare::PreparedValueId{1},
      .source_register_name = std::string("rax"),
      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
      .destination_register_name = std::string("rcx"),
      .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
  };
  const auto aliased_late_publication =
      prepare::find_prepared_call_result_late_publication(aliased_result);
  if (!aliased_late_publication.source_in_destination_alias_available) {
    return fail("call contract: call result late-publication query lost alias visibility");
  }
  if (!call_plan.result->source_register_placement.has_value() ||
      call_plan.result->source_register_placement->bank != prepare::PreparedRegisterBank::Gpr ||
      call_plan.result->source_register_placement->pool !=
          prepare::PreparedRegisterSlotPool::CallResult ||
      call_plan.result->source_register_placement->slot_index != 0 ||
      call_plan.result->source_register_placement->contiguous_width != 1) {
    return fail("call contract: result ABI source lost structured placement identity");
  }
  const auto* value_locations =
      prepare::find_prepared_value_location_function(prepared, "call_contract");
  const auto* before_call_bundle =
      value_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*value_locations,
                                               prepare::PreparedMovePhase::BeforeCall,
                                               0,
                                               0);
  const auto* after_call_bundle =
      value_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*value_locations,
                                               prepare::PreparedMovePhase::AfterCall,
                                               0,
                                               0);
  if (before_call_bundle == nullptr || after_call_bundle == nullptr) {
    return fail("call contract: value locations lost call-site move bundles");
  }
  const auto arg_binding_it = std::find_if(
      before_call_bundle->abi_bindings.begin(),
      before_call_bundle->abi_bindings.end(),
      [](const prepare::PreparedAbiBinding& binding) {
        return binding.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
               binding.destination_abi_index == std::optional<std::size_t>{0};
      });
  if (arg_binding_it == before_call_bundle->abi_bindings.end() ||
      !arg_binding_it->destination_register_placement.has_value() ||
      arg_binding_it->destination_register_placement->pool !=
          prepare::PreparedRegisterSlotPool::CallArgument ||
      arg_binding_it->destination_register_placement->slot_index != 0) {
    return fail("call contract: before-call ABI binding lost structured argument placement");
  }
  const auto result_binding_it = std::find_if(
      after_call_bundle->abi_bindings.begin(),
      after_call_bundle->abi_bindings.end(),
      [](const prepare::PreparedAbiBinding& binding) {
        return binding.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallResultAbi &&
               !binding.destination_abi_index.has_value();
      });
  if (result_binding_it == after_call_bundle->abi_bindings.end() ||
      !result_binding_it->destination_register_placement.has_value() ||
      result_binding_it->destination_register_placement->pool !=
          prepare::PreparedRegisterSlotPool::CallResult ||
      result_binding_it->destination_register_placement->slot_index != 0) {
    return fail("call contract: after-call ABI binding lost structured result placement");
  }
  if (call_plan.clobbered_registers.empty()) {
    return fail("call contract: call_plans lost the clobber contract");
  }
  for (const auto& clobber : call_plan.clobbered_registers) {
    if (clobber.contiguous_width != 1 || clobber.occupied_register_names.size() != 1 ||
        clobber.occupied_register_names.front() != clobber.register_name) {
      return fail("call contract: clobber metadata no longer publishes singleton occupancy");
    }
    if (!clobber.placement.has_value() ||
        clobber.placement->pool != prepare::PreparedRegisterSlotPool::ReservedScratch ||
        clobber.placement->bank != clobber.bank ||
        clobber.placement->contiguous_width != clobber.contiguous_width) {
      return fail("call contract: clobber metadata lost reserved scratch placement identity");
    }
  }
  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("clobbers=" + clobber_summary(call_plan.clobbered_registers.front())) ==
      std::string::npos) {
    return fail("call contract: prepared summary no longer publishes clobber authority");
  }
  const auto* storage_plan = find_storage_plan_function(prepared, "call_contract");
  const auto* tmp_call = storage_plan == nullptr ? nullptr
                                                 : find_storage_value(prepared, *storage_plan, "tmp.call");
  if (storage_plan == nullptr || tmp_call == nullptr ||
      tmp_call->encoding != prepare::PreparedStorageEncodingKind::Register ||
      tmp_call->bank != prepare::PreparedRegisterBank::Gpr ||
      !tmp_call->register_name.has_value()) {
    return fail("call contract: storage_plans lost the tmp.call register home");
  }
  if (!tmp_call->register_placement.has_value() ||
      tmp_call->register_placement->bank != prepare::PreparedRegisterBank::Gpr ||
      tmp_call->register_placement->contiguous_width != 1) {
    return fail("call contract: storage_plans lost structured register-home placement identity");
  }
  if (call_plan.result->destination_value_id !=
      std::optional<prepare::PreparedValueId>{tmp_call->value_id}) {
    return fail("call contract: call_plans lost direct integer result source identity");
  }
  const auto result_move_it = std::find_if(after_call_bundle->moves.begin(),
                                           after_call_bundle->moves.end(),
                                           [&](const prepare::PreparedMoveResolution& move) {
                                             return move.to_value_id == tmp_call->value_id &&
                                                    move.destination_kind ==
                                                        prepare::PreparedMoveDestinationKind::CallResultAbi &&
                                                    move.destination_storage_kind ==
                                                        prepare::PreparedMoveStorageKind::Register;
                                           });
  if (result_move_it == after_call_bundle->moves.end() ||
      !result_move_it->destination_register_placement.has_value() ||
      *result_move_it->destination_register_placement !=
          *result_binding_it->destination_register_placement) {
    return fail("call contract: after-call move lost structured ABI source placement");
  }
  return 0;
}

int check_same_module_i16_call_argument_contract() {
  const auto prepared =
      prepare_riscv_module(make_same_module_i16_call_argument_contract_module());
  const auto* call_plans =
      find_call_plans_function(prepared, "same_module_i16_call_argument_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1) {
    return fail("same-module i16 call argument contract: missing prepared call plan");
  }

  const auto& call_plan = call_plans->calls.front();
  if (call_plan.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule ||
      !call_plan.direct_callee_name.has_value() ||
      *call_plan.direct_callee_name != "same_module_i16") {
    return fail("same-module i16 call argument contract: lost same-module direct-call identity");
  }

  const auto& argument = call_plan.arguments.front();
  if (argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      argument.value_bank != prepare::PreparedRegisterBank::Gpr ||
      !argument.source_selection.has_value() ||
      argument.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue ||
      argument.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
      !argument.destination_register_name.has_value() ||
      !argument.destination_register_placement.has_value() ||
      argument.destination_register_placement->bank != prepare::PreparedRegisterBank::Gpr ||
      argument.destination_register_placement->pool !=
          prepare::PreparedRegisterSlotPool::CallArgument) {
    return fail("same-module i16 call argument contract: lost GPR argument publication");
  }
  const auto missing_frame_slot_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(argument);
  if (!missing_frame_slot_need.available ||
      missing_frame_slot_need.destination_register_bank != prepare::PreparedRegisterBank::Gpr) {
    return fail("same-module i16 call argument contract: frame-slot-to-GPR publication query failed");
  }

  const auto* value_locations =
      prepare::find_prepared_value_location_function(prepared,
                                                     "same_module_i16_call_argument_contract");
  const auto* before_call_bundle =
      value_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*value_locations,
                                               prepare::PreparedMovePhase::BeforeCall,
                                               0,
                                               2);
  if (before_call_bundle == nullptr) {
    return fail("same-module i16 call argument contract: missing before-call move bundle");
  }
  const auto move_it = std::find_if(
      before_call_bundle->moves.begin(),
      before_call_bundle->moves.end(),
      [](const prepare::PreparedMoveResolution& move) {
        return move.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
               move.destination_abi_index == std::optional<std::size_t>{0};
      });
  if (move_it == before_call_bundle->moves.end() ||
      move_it->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !move_it->destination_register_name.has_value() ||
      !move_it->destination_register_placement.has_value() ||
      move_it->destination_register_placement->bank != prepare::PreparedRegisterBank::Gpr ||
      move_it->destination_register_placement->pool !=
          prepare::PreparedRegisterSlotPool::CallArgument) {
    return fail("same-module i16 call argument contract: before-call move lost GPR destination");
  }
  const auto binding_it = std::find_if(
      before_call_bundle->abi_bindings.begin(),
      before_call_bundle->abi_bindings.end(),
      [](const prepare::PreparedAbiBinding& binding) {
        return binding.destination_kind ==
                   prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
               binding.destination_abi_index == std::optional<std::size_t>{0};
      });
  if (binding_it == before_call_bundle->abi_bindings.end() ||
      binding_it->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !binding_it->destination_register_name.has_value() ||
      !binding_it->destination_register_placement.has_value() ||
      binding_it->destination_register_placement->bank != prepare::PreparedRegisterBank::Gpr ||
      binding_it->destination_register_placement->pool !=
          prepare::PreparedRegisterSlotPool::CallArgument) {
    return fail("same-module i16 call argument contract: ABI binding lost GPR destination");
  }

  return 0;
}

int check_float_call_contract() {
  const auto prepared = prepare_module(make_float_call_contract_module());
  const auto* function = find_function(prepared.module, "float_call_contract");
  if (function == nullptr) {
    return fail("float-call contract: missing caller body");
  }
  const auto* call = find_first_call(*function);
  if (call == nullptr || call->callee != "extern_f32" || call->arg_abi.size() != 1 ||
      !call->result_abi.has_value()) {
    return fail("float-call contract: missing float call ABI surface");
  }
  if (call->arg_abi.front().primary_class != bir::AbiValueClass::Sse ||
      !call->arg_abi.front().passed_in_register) {
    return fail("float-call contract: float arg lost SSE register ABI classification");
  }
  if (call->result_abi->primary_class != bir::AbiValueClass::Sse) {
    return fail("float-call contract: float result lost SSE ABI classification");
  }
  const auto* liveness = find_liveness_function(prepared, "float_call_contract");
  if (liveness == nullptr || liveness->call_points.size() != 1) {
    return fail("float-call contract: prepared liveness no longer records the float call point");
  }
  const auto* call_plans = find_call_plans_function(prepared, "float_call_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1 || !call_plans->calls.front().result.has_value()) {
    return fail("float-call contract: call_plans no longer publish the float call");
  }
  if (call_plans->calls.front().arguments.front().value_bank != prepare::PreparedRegisterBank::Fpr ||
      call_plans->calls.front().arguments.front().destination_register_bank !=
          prepare::PreparedRegisterBank::Fpr ||
      call_plans->calls.front().result->value_bank != prepare::PreparedRegisterBank::Fpr ||
      call_plans->calls.front().result->source_storage_kind !=
          prepare::PreparedMoveStorageKind::Register ||
      call_plans->calls.front().result->source_register_bank != prepare::PreparedRegisterBank::Fpr) {
    return fail("float-call contract: call_plans lost SSE/FPR bank ownership");
  }
  const auto* storage_plan = find_storage_plan_function(prepared, "float_call_contract");
  const auto* arg_float =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "arg.float");
  const auto* tmp_call = storage_plan == nullptr
                             ? nullptr
                             : find_storage_value(prepared, *storage_plan, "tmp.float.call");
  if (storage_plan == nullptr || arg_float == nullptr || tmp_call == nullptr ||
      arg_float->bank != prepare::PreparedRegisterBank::Fpr ||
      tmp_call->bank != prepare::PreparedRegisterBank::Fpr) {
    return fail("float-call contract: storage_plans lost the FPR homes");
  }
  if (call_plans->calls.front().result->destination_value_id !=
      std::optional<prepare::PreparedValueId>{tmp_call->value_id}) {
    return fail("float-call contract: call_plans lost direct float result source identity");
  }
  const auto result_late_publication =
      prepare::find_prepared_call_result_late_publication(*call_plans->calls.front().result);
  if (!result_late_publication.source_register_publication_available ||
      !result_late_publication.fpr_or_vreg_store_value_retarget_available) {
    return fail("float-call contract: call result late-publication query lost FPR retarget visibility");
  }
  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("late_fpr_vreg_store_value_retarget=yes") == std::string::npos) {
    return fail("float-call contract: prepared dump no longer exposes FPR late-publication retargeting");
  }
  return 0;
}

int check_stack_result_slot_contract() {
  const auto prepared = prepare_module(make_stack_result_slot_contract_module());
  const auto* call_plans = find_call_plans_function(prepared, "stack_result_slot_contract");
  const auto* storage_plan = find_storage_plan_function(prepared, "stack_result_slot_contract");
  const auto* spilled_result = storage_plan == nullptr
                                   ? nullptr
                                   : find_storage_value(prepared, *storage_plan, "tmp.call.1");
  if (call_plans == nullptr || call_plans->calls.size() != 8 || storage_plan == nullptr ||
      spilled_result == nullptr) {
    return fail("stack-result slot contract: missing prepared call or storage publication");
  }
  if (spilled_result->encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      !spilled_result->slot_id.has_value() || !spilled_result->stack_offset_bytes.has_value()) {
    return fail("stack-result slot contract: fixture did not produce a frame-slot-backed call result");
  }

  const auto& call_plan = call_plans->calls[1];
  if (!call_plan.result.has_value()) {
    return fail("stack-result slot contract: call_plans lost result publication");
  }

  const auto& result = *call_plan.result;
  if (result.source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      result.destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      result.destination_slot_id != spilled_result->slot_id ||
      result.destination_stack_offset_bytes != spilled_result->stack_offset_bytes) {
    return fail("stack-result slot contract: call_plans lost frame-slot identity for spilled result");
  }
  if (!result.source_register_name.has_value() ||
      result.source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr}) {
    return fail("stack-result slot contract: call_plans lost call-result ABI source while publishing slot home");
  }
  if (!result.source_register_placement.has_value() ||
      result.source_register_placement->pool != prepare::PreparedRegisterSlotPool::CallResult ||
      !result.destination_spill_slot_placement.has_value() ||
      result.destination_spill_slot_placement->slot_id != *spilled_result->slot_id ||
      result.destination_spill_slot_placement->offset_bytes != *spilled_result->stack_offset_bytes ||
      !spilled_result->spill_slot_placement.has_value() ||
      spilled_result->spill_slot_placement->slot_id != *spilled_result->slot_id ||
      spilled_result->spill_slot_placement->offset_bytes != *spilled_result->stack_offset_bytes) {
    return fail("stack-result slot contract: spilled result lost structured source or slot placement");
  }
  if (result.destination_value_id != std::optional<prepare::PreparedValueId>{spilled_result->value_id}) {
    return fail("stack-result slot contract: call_plans lost direct spilled-result source identity");
  }
  const auto result_late_publication =
      prepare::find_prepared_call_result_late_publication(result);
  if (!result_late_publication.source_register_publication_available ||
      result_late_publication.current_block_publication_consumption_available ||
      result_late_publication.source_in_destination_alias_available ||
      result_late_publication.fpr_or_vreg_store_value_retarget_available) {
    return fail("stack-result slot contract: call result late-publication query drifted");
  }
  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("late_source_register=yes") == std::string::npos ||
      prepared_dump.find("late_current_block_publication=no") == std::string::npos) {
    return fail("stack-result slot contract: prepared dump no longer exposes late-publication fact");
  }

  return 0;
}

int check_indirect_call_contract() {
  const auto prepared = prepare_module(make_indirect_call_contract_module());
  const auto* liveness = find_liveness_function(prepared, "indirect_call_contract");
  if (liveness == nullptr || liveness->call_points.size() != 1) {
    return fail("indirect-call contract: prepared liveness no longer records the indirect call point");
  }
  const auto* call_plans = find_call_plans_function(prepared, "indirect_call_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1) {
    return fail("indirect-call contract: call_plans no longer publish the indirect call");
  }
  const auto& call_plan = call_plans->calls.front();
  if (!call_plan.is_indirect || call_plan.direct_callee_name.has_value() ||
      !call_plan.indirect_callee.has_value() || call_plan.arguments.size() != 1 ||
      !call_plan.result.has_value()) {
    return fail("indirect-call contract: call_plans lost indirect-call shape");
  }
  if (call_plan.arguments.front().value_bank != prepare::PreparedRegisterBank::Gpr ||
      call_plan.result->value_bank != prepare::PreparedRegisterBank::Gpr) {
    return fail("indirect-call contract: call_plans lost GPR ownership");
  }
  const auto& indirect_callee = *call_plan.indirect_callee;
  const auto* storage_plan = find_storage_plan_function(prepared, "indirect_call_contract");
  if (storage_plan == nullptr) {
    return fail("indirect-call contract: storage plan no longer publishes the indirect callee value");
  }
  const auto* callee_storage = find_storage_value(prepared, *storage_plan, "callee.ptr");
  if (callee_storage == nullptr) {
    return fail("indirect-call contract: storage plan no longer resolves callee.ptr");
  }
  if (prepare::prepared_value_name(prepared.names, indirect_callee.value_name) != "callee.ptr" ||
      !indirect_callee.value_id.has_value() ||
      *indirect_callee.value_id != callee_storage->value_id ||
      indirect_callee.encoding != prepare::PreparedStorageEncodingKind::Register ||
      indirect_callee.bank != prepare::PreparedRegisterBank::Gpr ||
      !indirect_callee.register_name.has_value()) {
    return fail("indirect-call contract: call_plans lost the published indirect callee authority");
  }
  return 0;
}

int check_memory_return_call_contract() {
  const auto prepared = prepare_module(make_memory_return_call_contract_module());
  const auto* call_plans = find_call_plans_function(prepared, "memory_return_call_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1) {
    return fail("memory-return contract: call_plans no longer publish the aggregate-return call");
  }

  const auto& call_plan = call_plans->calls.front();
  if (call_plan.result.has_value() || !call_plan.memory_return.has_value() ||
      call_plan.arguments.size() != 2) {
    return fail("memory-return contract: call_plans lost explicit memory-return publication");
  }
  if (call_plan.arguments[1].source_encoding != prepare::PreparedStorageEncodingKind::Immediate ||
      !call_plan.arguments[1].source_literal.has_value() ||
      call_plan.arguments[1].source_literal->immediate != 13) {
    return fail("memory-return contract: call_plans lost the scalar immediate argument authority");
  }

  const auto* storage_object = find_stack_object(prepared, "lv.call.sret.storage");
  const auto* frame_slot =
      storage_object == nullptr ? nullptr : find_frame_slot(prepared, storage_object->object_id);
  if (storage_object == nullptr || frame_slot == nullptr) {
    return fail("memory-return contract: missing published stack storage for the sret destination");
  }

  const auto& memory_return = *call_plan.memory_return;
  if (memory_return.sret_arg_index != std::optional<std::size_t>{0} ||
      memory_return.storage_slot_name == c4c::kInvalidSlotName ||
      prepare::prepared_slot_name(prepared.names, memory_return.storage_slot_name) !=
          "lv.call.sret.storage" ||
      memory_return.encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      memory_return.slot_id != std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      memory_return.stack_offset_bytes != std::optional<std::size_t>{frame_slot->offset_bytes} ||
      memory_return.size_bytes != 8 || memory_return.align_bytes != 4) {
    return fail("memory-return contract: call_plans lost the published sret destination authority");
  }
  return 0;
}

int check_call_wrapper_kind_contract() {
  const auto prepared = prepare_module(make_call_wrapper_kind_contract_module());
  const auto* call_plans = find_call_plans_function(prepared, "call_wrapper_kind_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 4) {
    return fail("call-wrapper contract: call_plans no longer publish all wrapper shapes");
  }

  const auto& same_module_call = call_plans->calls[0];
  if (same_module_call.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule ||
      same_module_call.variadic_fpr_arg_register_count != 0 || same_module_call.is_indirect ||
      !same_module_call.direct_callee_name.has_value() ||
      same_module_call.indirect_callee.has_value() ||
      *same_module_call.direct_callee_name != "same_module_i32") {
    return fail("call-wrapper contract: same-module call lost explicit wrapper classification");
  }

  const auto& fixed_extern_call = call_plans->calls[1];
  if (fixed_extern_call.wrapper_kind !=
          prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      fixed_extern_call.variadic_fpr_arg_register_count != 0 ||
      fixed_extern_call.is_indirect || !fixed_extern_call.direct_callee_name.has_value() ||
      fixed_extern_call.indirect_callee.has_value() ||
      *fixed_extern_call.direct_callee_name != "extern_fixed_i32") {
    return fail("call-wrapper contract: direct fixed extern lost explicit wrapper classification");
  }

  const auto& variadic_extern_call = call_plans->calls[2];
  if (variadic_extern_call.wrapper_kind !=
          prepare::PreparedCallWrapperKind::DirectExternVariadic ||
      variadic_extern_call.variadic_fpr_arg_register_count != 1 ||
      variadic_extern_call.is_indirect || !variadic_extern_call.direct_callee_name.has_value() ||
      variadic_extern_call.indirect_callee.has_value() ||
      *variadic_extern_call.direct_callee_name != "extern_variadic_i32" ||
      variadic_extern_call.arguments.size() != 2) {
    return fail("call-wrapper contract: direct variadic extern lost explicit wrapper classification or FPR count");
  }

  const auto& indirect_call = call_plans->calls[3];
  if (indirect_call.wrapper_kind != prepare::PreparedCallWrapperKind::Indirect ||
      indirect_call.variadic_fpr_arg_register_count != 0 || !indirect_call.is_indirect ||
      indirect_call.direct_callee_name.has_value() || !indirect_call.indirect_callee.has_value() ||
      prepare::prepared_value_name(prepared.names, indirect_call.indirect_callee->value_name) !=
          "callee.ptr" ||
      indirect_call.indirect_callee->encoding != prepare::PreparedStorageEncodingKind::Register ||
      indirect_call.indirect_callee->bank != prepare::PreparedRegisterBank::Gpr ||
      indirect_call.arguments.size() != 1 ||
      indirect_call.arguments.front().source_encoding !=
          prepare::PreparedStorageEncodingKind::Immediate ||
      !indirect_call.arguments.front().source_literal.has_value() ||
      indirect_call.arguments.front().source_literal->immediate != 5) {
    return fail("call-wrapper contract: indirect call lost explicit wrapper classification");
  }

  if (!prepared.variadic_entry_plans.functions.empty()) {
    return fail("call-wrapper contract: variadic callsite metadata leaked into function-entry carriers");
  }

  return 0;
}

int check_call_wrapper_link_name_id_authority_contract() {
  const auto prepared = prepare_module(make_call_wrapper_link_name_id_contract_module());
  const auto* call_plans =
      find_call_plans_function(prepared, "call_wrapper_link_name_id_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 3) {
    return fail("call-wrapper LinkNameId contract: missing prepared call plans");
  }

  const auto& id_only_call = call_plans->calls[0];
  if (id_only_call.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule ||
      id_only_call.is_indirect || id_only_call.indirect_callee.has_value() ||
      id_only_call.direct_callee_name !=
          std::optional<std::string>{"same_module_id_target"}) {
    return fail("call-wrapper LinkNameId contract: ID-only call did not publish semantic callee authority");
  }

  const auto& raw_compat_call = call_plans->calls[1];
  if (raw_compat_call.wrapper_kind !=
          prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      raw_compat_call.is_indirect || raw_compat_call.indirect_callee.has_value() ||
      raw_compat_call.direct_callee_name !=
          std::optional<std::string>{"raw_compat_extern"}) {
    return fail("call-wrapper LinkNameId contract: raw-only compatibility call stopped resolving");
  }

  const auto& mismatch_call = call_plans->calls[2];
  if (mismatch_call.wrapper_kind != prepare::PreparedCallWrapperKind::Indirect ||
      mismatch_call.is_indirect || mismatch_call.direct_callee_name.has_value() ||
      mismatch_call.indirect_callee.has_value()) {
    return fail("call-wrapper LinkNameId contract: mismatched ID/raw callee did not fail closed");
  }

  return 0;
}

int check_call_argument_source_shape_contract() {
  const auto prepared = prepare_module(make_call_argument_source_shape_module());
  const auto* call_plans = find_call_plans_function(prepared, "call_argument_source_shape_contract");
  const auto* storage_plan =
      find_storage_plan_function(prepared, "call_argument_source_shape_contract");
  const auto* loaded_ptr =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "loaded.ptr");
  if (call_plans == nullptr || call_plans->calls.size() != 1) {
    return fail("call-argument source-shape contract: call_plans no longer publish the pointer call");
  }

  const auto& call_plan = call_plans->calls.front();
  if (call_plan.arguments.size() != 2 || !call_plan.result.has_value()) {
    return fail("call-argument source-shape contract: pointer call lost argument or result publication");
  }

  const auto& symbol_arg = call_plan.arguments[0];
  if (symbol_arg.source_encoding != prepare::PreparedStorageEncodingKind::SymbolAddress ||
      !symbol_arg.source_symbol_name.has_value() ||
      *symbol_arg.source_symbol_name != "@extern_data" ||
      !symbol_arg.source_symbol_name_id.has_value() ||
      prepare::prepared_link_name(prepared.names, *symbol_arg.source_symbol_name_id) !=
          "extern_data") {
    return fail("call-argument source-shape contract: call_plans lost symbol-address authority");
  }

  const auto& computed_arg = call_plan.arguments[1];
  if (computed_arg.source_encoding != prepare::PreparedStorageEncodingKind::ComputedAddress ||
      !computed_arg.source_base_value_id.has_value() ||
      loaded_ptr == nullptr || *computed_arg.source_base_value_id != loaded_ptr->value_id ||
      !computed_arg.source_base_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *computed_arg.source_base_value_name) !=
          "loaded.ptr" ||
      computed_arg.source_pointer_byte_delta != std::optional<std::int64_t>{4}) {
    return fail(
        "call-argument source-shape contract: call_plans lost computed-address base identity authority");
  }

  const auto symbol_routing =
      prepare::find_prepared_call_argument_publication_source_routing(symbol_arg);
  if (!symbol_routing.available ||
      symbol_routing.source_encoding !=
          prepare::PreparedStorageEncodingKind::SymbolAddress ||
      symbol_routing.source_selection != nullptr ||
      symbol_routing.direct_global_select_chain_dependency != nullptr) {
    return fail(
        "call-argument source-shape contract: symbol-address publication routing should be visible through shared query");
  }

  const auto computed_routing =
      prepare::find_prepared_call_argument_publication_source_routing(computed_arg);
  if (!computed_routing.available ||
      computed_routing.source_encoding !=
          prepare::PreparedStorageEncodingKind::ComputedAddress ||
      computed_routing.source_base_value_id != computed_arg.source_base_value_id ||
      computed_routing.source_base_value_name != computed_arg.source_base_value_name ||
      computed_routing.source_pointer_byte_delta !=
          std::optional<std::int64_t>{4} ||
      computed_routing.direct_global_select_chain_dependency != nullptr) {
    return fail(
        "call-argument source-shape contract: computed-address publication routing should be visible through shared query");
  }

  return 0;
}

int check_call_argument_source_producer_materializability_contract() {
  prepare::PreparedNameTables names;
  const auto function_name =
      names.function_names.intern("call_argument_source_producer_contract");
  const auto block_label = names.block_labels.intern("entry");
  const auto loaded_name = names.value_names.intern("%loaded");
  const auto sum_name = names.value_names.intern("%sum");
  const auto shifted_name = names.value_names.intern("%shifted");

  bir::Block block;
  block.label = "entry";
  block.label_id = block_label;
  block.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
      .slot_name = "slot",
      .align_bytes = 4,
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%loaded"),
      .rhs = bir::Value::immediate_i32(9),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Shl,
      .result = bir::Value::named(bir::TypeKind::I32, "%shifted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .rhs = bir::Value::immediate_i32(1),
  });
  block.insts.push_back(bir::CallInst{
      .callee = "consume_i32",
      .args = {bir::Value::named(bir::TypeKind::I32, "%sum")},
      .arg_types = {bir::TypeKind::I32},
      .return_type = bir::TypeKind::Void,
  });

  const auto* loaded = std::get_if<bir::LoadLocalInst>(&block.insts[0]);
  const auto* sum = std::get_if<bir::BinaryInst>(&block.insts[1]);
  const auto* shifted = std::get_if<bir::BinaryInst>(&block.insts[2]);
  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      loaded_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 0,
          .load_local = loaded,
      });
  source_producers.producers_by_value_name.emplace(
      sum_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 1,
          .binary = sum,
      });
  source_producers.producers_by_value_name.emplace(
      shifted_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 2,
          .binary = shifted,
      });

  const auto load_materialization =
      prepare::find_prepared_call_argument_source_producer_materialization(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%loaded"),
          3);
  if (!load_materialization.has_value() ||
      !load_materialization->materializable ||
      load_materialization->producer.producer.kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      load_materialization->producer.instruction_index != 0 ||
      load_materialization->producer.value_name != loaded_name) {
    return fail(
        "call-argument producer materializability contract: load-local source should be visible");
  }

  const auto sum_materialization =
      prepare::find_prepared_call_argument_source_producer_materialization(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          3);
  if (!sum_materialization.has_value() ||
      !sum_materialization->materializable ||
      sum_materialization->producer.producer.kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
      sum_materialization->producer.producer.binary != sum ||
      sum_materialization->producer.instruction_index != 1 ||
      sum_materialization->producer.value_name != sum_name) {
    return fail(
        "call-argument producer materializability contract: ordinary binary producer should be visible");
  }
  const auto sum_binary_fact =
      prepare::find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          3);
  if (!sum_binary_fact.has_value() ||
      !sum_binary_fact->materializable ||
      !sum_binary_fact->same_block_before_call ||
      sum_binary_fact->destination_value_name != sum_name ||
      sum_binary_fact->destination_value_type != bir::TypeKind::I32 ||
      sum_binary_fact->producer_block_label != block_label ||
      sum_binary_fact->producer_instruction_index != 1 ||
      sum_binary_fact->producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
      sum_binary_fact->producer_instruction != &block.insts[1] ||
      sum_binary_fact->binary != sum ||
      sum_binary_fact->binary_opcode != bir::BinaryOpcode::Add ||
      sum_binary_fact->lhs.name != "%loaded" ||
      sum_binary_fact->rhs.kind != bir::Value::Kind::Immediate ||
      sum_binary_fact->rhs.immediate != 9) {
    return fail(
        "call-argument producer materializability contract: binary producer fact should expose typed coherent payload");
  }
  const auto sum_current_block_publication =
      prepare::find_prepared_current_block_publication_consumption(
          names,
          &source_producers,
          block_label,
          &block,
          sum_name,
          3);
  if (!sum_current_block_publication.available ||
      sum_current_block_publication.source_producer == nullptr ||
      sum_current_block_publication.source_producer->binary != sum ||
      sum_current_block_publication.produced_value == nullptr ||
      sum_current_block_publication.produced_value->name != "%sum" ||
      sum_current_block_publication.instruction_index != 1 ||
      sum_current_block_publication.value_name != sum_name ||
      sum_current_block_publication.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) {
    return fail(
        "call-argument producer materializability contract: current-block publication consumption should be fact-backed");
  }
  const auto sum_bir_current_block_publication =
      mir::find_bir_current_block_publication_identity(
          mir::BirCurrentBlockPublicationIdentityRequest{
              .block = &block,
              .block_label = block.label,
              .root_value_name = prepare::prepared_value_name(names, sum_name),
              .root_value_type = bir::TypeKind::I32,
              .before_instruction_index = 3,
          });
  if (!sum_bir_current_block_publication.available ||
      sum_bir_current_block_publication.source_producer.inst !=
          sum_current_block_publication.instruction ||
      sum_bir_current_block_publication.instruction !=
          sum_current_block_publication.instruction ||
      sum_bir_current_block_publication.produced_value !=
          sum_current_block_publication.produced_value ||
      sum_bir_current_block_publication.produced_value_name != "%sum" ||
      sum_bir_current_block_publication.instruction_index !=
          sum_current_block_publication.instruction_index ||
      sum_bir_current_block_publication.value_name != "%sum" ||
      sum_bir_current_block_publication.source_producer_kind !=
          mir::SameBlockProducerKind::Binary) {
    return fail(
        "call-argument producer materializability contract: BIR current-block publication identity should match prepared semantic fields");
  }
  bir::Function route4_function{
      .name = "call_argument_source_producer_contract",
      .blocks = {block},
  };
  const auto& route4_block = route4_function.blocks.front();
  const auto route4_index =
      bir::route4_build_publication_availability_index(route4_function);
  const auto route4_sum_value = bir::Value::named(bir::TypeKind::I32, "%sum");
  const auto route4_sum_reference =
      bir::route4_validate_current_block_publication_reference(
          route4_index, route4_block, route4_sum_value, 3);
  if (!route4_sum_reference ||
      route4_sum_reference.status != bir::RouteIndexValidationStatus::Valid ||
      route4_sum_reference.route_status !=
          bir::Route4PublicationAvailabilityStatus::Available ||
      route4_sum_reference.current_block_record == nullptr ||
      route4_sum_reference.current_block_record->source_producer_instruction_index != 1) {
    return fail(
        "call-argument producer materializability contract: Route 4 current-block reference should validate matching source identity");
  }
  const auto route4_missing_reference =
      bir::route4_validate_current_block_publication_reference(
          route4_index,
          route4_block,
          bir::Value::named(bir::TypeKind::I32, "%missing"),
          3);
  const auto route4_wrong_type_reference =
      bir::route4_validate_current_block_publication_reference(
          route4_index,
          route4_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          3);
  auto route4_duplicate_index = route4_index;
  route4_duplicate_index.current_block_records.push_back(
      route4_index.current_block_records[1]);
  const auto route4_duplicate_reference =
      bir::route4_validate_current_block_publication_reference(
          route4_duplicate_index, route4_block, route4_sum_value, 3);
  const auto route4_stale_reference =
      bir::route4_validate_current_block_publication_reference(
          route4_index, block, route4_sum_value, 3);
  auto route4_wrong_relationship_index = route4_index;
  route4_wrong_relationship_index.current_block_records.clear();
  route4_wrong_relationship_index.value_records.push_back(
      bir::Route4PublicationValueRecord{
          .available = true,
          .scope = bir::Route4PublicationScope::BlockEntry,
          .status = bir::Route4PublicationAvailabilityStatus::Available,
          .value_role = bir::Route4PublicationValueRole::Produced,
          .value = bir::route1_source_value_identity(route4_sum_value, sum_name),
          .block_label = route4_block.label,
          .block_label_id = route4_block.label_id,
          .instruction_index = 1,
      });
  const auto route4_wrong_relationship_reference =
      bir::route4_validate_current_block_publication_reference(
          route4_wrong_relationship_index, route4_block, route4_sum_value, 3);
  if (route4_missing_reference ||
      route4_missing_reference.status !=
          bir::RouteIndexValidationStatus::MissingRecord ||
      route4_missing_reference.route_status !=
          bir::Route4PublicationAvailabilityStatus::MissingPublication ||
      route4_wrong_type_reference ||
      route4_wrong_type_reference.status !=
          bir::RouteIndexValidationStatus::WrongKey ||
      route4_wrong_type_reference.route_status !=
          bir::Route4PublicationAvailabilityStatus::NoMatch ||
      route4_duplicate_reference ||
      route4_duplicate_reference.status !=
          bir::RouteIndexValidationStatus::DuplicateReference ||
      route4_duplicate_reference.route_status !=
          bir::Route4PublicationAvailabilityStatus::NoMatch ||
      route4_stale_reference ||
      route4_stale_reference.status !=
          bir::RouteIndexValidationStatus::StaleOwner ||
      route4_stale_reference.route_status !=
          bir::Route4PublicationAvailabilityStatus::MissingBlock ||
      route4_wrong_relationship_reference ||
      route4_wrong_relationship_reference.status !=
          bir::RouteIndexValidationStatus::WrongRelationship ||
      route4_wrong_relationship_reference.route_status !=
          bir::Route4PublicationAvailabilityStatus::MissingPublication) {
    return fail(
        "call-argument producer materializability contract: Route 4 current-block reference should reject missing, mismatched, duplicate, stale, and wrong-relationship facts");
  }

  const auto join_label = names.block_labels.intern("call_contract.join");
  const auto join_value_name = names.value_names.intern("%join.arg");
  const auto join_bir_only_name = names.value_names.intern("%join.bir_only");
  prepare::PreparedValueLocationFunction entry_publication_locations{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 71,
              .function_name = function_name,
              .value_name = join_value_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"r12"},
          },
          prepare::PreparedValueHome{
              .value_id = 72,
              .function_name = function_name,
              .value_name = join_bir_only_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"r13"},
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind =
                  prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .source_parallel_copy_successor_label = join_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .to_value_id = 71,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_register_name = std::string{"r12"},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
      },
  };
  const auto entry_value_home_lookups =
      prepare::make_prepared_value_home_lookups(&entry_publication_locations);
  const auto prepared_entry_publication =
      prepare::find_prepared_current_block_entry_publication(
          prepare::PreparedCurrentBlockEntryPublicationQueryInputs{
              .names = &names,
              .value_locations = &entry_publication_locations,
              .value_home_lookups = &entry_value_home_lookups,
              .successor_label = join_label,
          },
          prepare::PreparedValueId{71});
  bir::Block join_block;
  join_block.label = "call_contract.join";
  join_block.label_id = join_label;
  join_block.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%join.arg"),
      .incomings = {
          bir::PhiIncoming{
              .label = block.label,
              .value = bir::Value::named(bir::TypeKind::I32, "%sum"),
              .label_id = block_label,
          },
      },
  });
  join_block.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%join.bir_only"),
      .incomings = {
          bir::PhiIncoming{
              .label = block.label,
              .value = bir::Value::named(bir::TypeKind::I32, "%loaded"),
              .label_id = block_label,
          },
      },
  });
  const auto bir_entry_publication =
      mir::find_bir_block_entry_publication_identity(
          mir::BirBlockEntryPublicationIdentityRequest{
              .successor_block = &join_block,
              .successor_label = join_block.label,
              .successor_label_id = join_label,
              .destination_value =
                  &std::get<bir::PhiInst>(join_block.insts.front()).result,
              .destination_value_id =
                  prepared_entry_publication.destination_value_id,
              .destination_value_name =
                  prepare::prepared_value_name(names, join_value_name),
              .destination_value_name_id = join_value_name,
              .destination_value_type = bir::TypeKind::I32,
          });
  if (prepared_entry_publication.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      !bir_entry_publication.available ||
      bir_entry_publication.status !=
          mir::BirBlockEntryPublicationStatus::Available ||
      bir_entry_publication.instruction != &join_block.insts.front() ||
      bir_entry_publication.phi !=
          &std::get<bir::PhiInst>(join_block.insts.front()) ||
      bir_entry_publication.instruction_index != 0 ||
      bir_entry_publication.destination_value_id !=
          prepared_entry_publication.destination_value_id ||
      bir_entry_publication.destination_value_name_id !=
          prepared_entry_publication.destination_value_name ||
      bir_entry_publication.destination_value == nullptr ||
      bir_entry_publication.destination_value_identity.value !=
          bir_entry_publication.destination_value ||
      bir_entry_publication.destination_value_identity.name !=
          bir_entry_publication.destination_value_name ||
      bir_entry_publication.destination_value_name != "%join.arg") {
    return fail(
        "call-argument producer materializability contract: BIR block-entry publication identity should match prepared semantic destination fields");
  }
  const auto prepared_bir_only_entry_publication =
      prepare::find_prepared_current_block_entry_publication(
          prepare::PreparedCurrentBlockEntryPublicationQueryInputs{
              .names = &names,
              .value_locations = &entry_publication_locations,
              .value_home_lookups = &entry_value_home_lookups,
              .successor_label = join_label,
          },
          prepare::PreparedValueId{72});
  const auto bir_only_entry_publication =
      mir::find_bir_block_entry_publication_identity(
          mir::BirBlockEntryPublicationIdentityRequest{
              .successor_block = &join_block,
              .successor_label = join_block.label,
              .successor_label_id = join_label,
              .destination_value_id =
                  prepared_bir_only_entry_publication.destination_value_id,
              .destination_value_name =
                  prepare::prepared_value_name(names, join_bir_only_name),
              .destination_value_name_id = join_bir_only_name,
              .destination_value_type = bir::TypeKind::I32,
          });
  if (prepared_bir_only_entry_publication.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::MissingPublication ||
      !bir_only_entry_publication.available ||
      bir_only_entry_publication.destination_value_name != "%join.bir_only" ||
      bir_only_entry_publication.destination_value_name_id !=
          prepared_bir_only_entry_publication.destination_value_name) {
    return fail(
        "call-argument producer materializability contract: BIR PHI-entry identity should not imply prepared entry-publication emission readiness");
  }

  if (prepare::find_prepared_call_argument_source_producer_materialization(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%shifted"),
          3)
          .has_value()) {
    return fail(
        "call-argument producer materializability contract: non-covered binary producer should fail closed");
  }
  if (prepare::find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%shifted"),
          3)
          .has_value()) {
    return fail(
        "call-argument producer materializability contract: binary fact should reject unsupported opcode");
  }
  if (prepare::find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%loaded"),
          3)
          .has_value()) {
    return fail(
        "call-argument producer materializability contract: binary fact should reject load-local cross-family payload");
  }
  if (prepare::find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          nullptr,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          3)
          .has_value()) {
    return fail(
        "call-argument producer materializability contract: binary fact should reject missing producer table");
  }
  if (prepare::find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          3)
          .has_value()) {
    return fail(
        "call-argument producer materializability contract: binary fact should reject mismatched destination type");
  }
  if (prepare::find_prepared_call_argument_source_producer_materialization(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          1)
          .has_value()) {
    return fail(
        "call-argument producer materializability contract: future producer should fail closed");
  }
  if (prepare::find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          1)
          .has_value()) {
    return fail(
        "call-argument producer materializability contract: binary fact should reject producer at or after call");
  }
  auto stale_source_producers = source_producers;
  stale_source_producers.producers_by_value_name[sum_name].binary = shifted;
  if (prepare::find_prepared_call_argument_binary_producer_materialization_fact(
          names,
          &stale_source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          3)
          .has_value()) {
    return fail(
        "call-argument producer materializability contract: binary fact should reject stale producer payload");
  }
  if (prepare::find_prepared_current_block_publication_consumption(
          names,
          &source_producers,
          block_label,
          &block,
          sum_name,
          1)
          .available) {
    return fail(
        "call-argument producer materializability contract: current-block publication should fail closed for future producer");
  }
  if (mir::find_bir_current_block_publication_identity(
          mir::BirCurrentBlockPublicationIdentityRequest{
              .block = &block,
              .block_label = block.label,
              .root_value_name = "%sum",
              .root_value_type = bir::TypeKind::I32,
              .before_instruction_index = 1,
          })) {
    return fail(
        "call-argument producer materializability contract: BIR current-block publication should fail closed for future producer");
  }
  auto mismatched_source_producers = source_producers;
  mismatched_source_producers.producers_by_value_name[sum_name].instruction_index = 0;
  if (prepare::find_prepared_current_block_publication_consumption(
          names,
          &mismatched_source_producers,
          block_label,
          &block,
          sum_name,
          3)
          .available) {
    return fail(
        "call-argument producer materializability contract: current-block publication should fail closed on mismatched producer fact");
  }
  if (!mir::find_bir_current_block_publication_identity(
          mir::BirCurrentBlockPublicationIdentityRequest{
              .block = &block,
              .block_label = block.label,
              .root_value_name = "%sum",
              .root_value_type = bir::TypeKind::I32,
              .before_instruction_index = 3,
          })) {
    return fail(
        "call-argument producer materializability contract: BIR current-block publication should not depend on mismatched prepared producer facts");
  }

  return 0;
}

int check_direct_global_select_chain_call_argument_contract() {
  const auto prepared =
      prepare_module(make_direct_global_select_chain_call_argument_module());
  const auto* call_plans = find_call_plans_function(
      prepared, "direct_global_select_chain_call_argument_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1) {
    return fail(
        "direct-global select-chain call argument contract: missing prepared call argument plan");
  }

  const auto& argument = call_plans->calls.front().arguments.front();
  const auto routing =
      prepare::find_prepared_call_argument_publication_source_routing(argument);
  const auto* dependency =
      routing.direct_global_select_chain_dependency;
  if (dependency == nullptr ||
      !routing.available ||
      routing.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
      dependency->source_value_name == c4c::kInvalidValueName ||
      prepare::prepared_value_name(prepared.names, dependency->source_value_name) !=
          "selected.global" ||
      !dependency->direct_global_dependency.contains_direct_global_load ||
      !dependency->direct_global_dependency.root_is_select ||
      dependency->direct_global_dependency.root_instruction_index !=
          std::optional<std::size_t>{1}) {
    return fail(
        "direct-global select-chain call argument contract: call plan lost prepared dependency authority");
  }

  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("direct_global_select_chain=yes") == std::string::npos ||
      (prepared_dump.find("direct_global_source=%selected.global") ==
           std::string::npos &&
       prepared_dump.find("direct_global_source=selected.global") ==
           std::string::npos) ||
      prepared_dump.find("direct_global_root_is_select=yes") ==
          std::string::npos ||
      prepared_dump.find("direct_global_root_inst=1") == std::string::npos) {
    return fail(
        "direct-global select-chain call argument contract: prepared dump lost dependency visibility");
  }

  return 0;
}

int check_call_argument_symbol_link_name_id_mismatch_contract() {
  const auto prepared = prepare_module(make_call_argument_symbol_link_name_id_mismatch_module());
  const auto* call_plans =
      find_call_plans_function(prepared, "call_argument_symbol_mismatch_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1) {
    return fail("call-argument symbol mismatch contract: missing prepared call plan");
  }

  const auto& arg = call_plans->calls.front().arguments.front();
  if (arg.source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress ||
      arg.source_symbol_name.has_value() || arg.source_symbol_name_id.has_value()) {
    return fail(
        "call-argument symbol mismatch contract: stale raw symbol was accepted over LinkNameId authority");
  }

  return 0;
}

int check_f128_symbol_backed_load_local_addressing_contract() {
  const auto prepared =
      prepare_aarch64_module(make_f128_symbol_backed_load_local_addressing_contract_module());
  const auto function_name_id = prepared.names.function_names.find(
      "f128_symbol_backed_load_local_addressing_contract");
  const auto entry_block_label_id = prepared.names.block_labels.find("entry");
  const auto* function_addressing =
      prepare::find_prepared_addressing(prepared, function_name_id);
  if (function_name_id == c4c::kInvalidFunctionName ||
      entry_block_label_id == c4c::kInvalidBlockLabel ||
      function_addressing == nullptr) {
    return fail(
        "F128 symbol-backed load-local addressing contract: missing prepared function addressing");
  }

  const auto* load_access =
      prepare::find_prepared_memory_access(*function_addressing, entry_block_label_id, 0);
  if (load_access == nullptr) {
    return fail(
        "F128 symbol-backed load-local addressing contract: missing prepared memory access");
  }
  if (!load_access->result_value_name.has_value() ||
      prepare::prepared_value_name(prepared.names, *load_access->result_value_name) !=
          "loaded.hfa31" ||
      load_access->stored_value_name.has_value() ||
      load_access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !load_access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *load_access->address.symbol_name) !=
          "hfa31" ||
      load_access->address.byte_offset != 0 ||
      load_access->address.size_bytes != 16 ||
      load_access->address.align_bytes != 16 ||
      !load_access->address.can_use_base_plus_offset ||
      load_access->address.global_address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct) {
    return fail(
        "F128 symbol-backed load-local addressing contract: wrong prepared global access facts");
  }

  return 0;
}

int check_stack_argument_slot_contract() {
  const auto prepared = prepare_module(make_stack_argument_slot_contract_module());
  const auto* call_plans = find_call_plans_function(prepared, "stack_argument_slot_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1) {
    return fail("stack-argument slot contract: call_plans no longer publish the byval call");
  }

  const auto* stack_object = find_stack_object(prepared, "p.byval");
  const auto* frame_slot =
      stack_object == nullptr ? nullptr : find_frame_slot(prepared, stack_object->object_id);
  if (stack_object == nullptr || frame_slot == nullptr) {
    return fail("stack-argument slot contract: prepared stack layout lost the byval home slot");
  }

  const auto& call_plan = call_plans->calls.front();
  if (call_plan.arguments.size() != 1) {
    return fail("stack-argument slot contract: byval call lost argument publication");
  }

  const auto& arg = call_plan.arguments.front();
  if (arg.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      arg.source_slot_id != std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      arg.source_stack_offset_bytes != std::optional<std::size_t>{frame_slot->offset_bytes}) {
    return fail("stack-argument slot contract: call_plans lost frame-slot-backed argument authority");
  }
  if (!arg.source_selection.has_value() ||
      arg.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue ||
      arg.source_selection->source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      arg.source_selection->source_stack_offset_bytes !=
          std::optional<std::size_t>{frame_slot->offset_bytes} ||
      arg.source_selection->source_size_bytes != std::optional<std::size_t>{8} ||
      arg.source_selection->source_align_bytes != std::optional<std::size_t>{4}) {
    return fail("stack-argument slot contract: missing shared frame-slot value source selection");
  }
  if (!arg.destination_register_name.has_value() ||
      arg.destination_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::AggregateAddress}) {
    return fail("stack-argument slot contract: call_plans lost the target ABI carrier destination");
  }

  return 0;
}

int check_cross_call_preservation_contract() {
  const auto prepared = prepare_riscv_module(make_cross_call_preservation_contract_module());
  const auto function_id = prepared.names.function_names.find("cross_call_preservation_contract");
  const auto* liveness = find_liveness_function(prepared, "cross_call_preservation_contract");
  const auto* call_plans = find_call_plans_function(prepared, "cross_call_preservation_contract");
  const auto* frame_plan = function_id == c4c::kInvalidFunctionName
                               ? nullptr
                               : prepare::find_prepared_frame_plan(prepared, function_id);
  const auto* storage_plan =
      find_storage_plan_function(prepared, "cross_call_preservation_contract");
  const auto* carry =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "carry");
  if (liveness == nullptr || liveness->call_points.size() != 1 || call_plans == nullptr ||
      call_plans->calls.size() != 1 || frame_plan == nullptr || storage_plan == nullptr ||
      carry == nullptr) {
    return fail("cross-call preservation contract: missing liveness, call-plan, or carry storage publication");
  }

  const auto& call_plan = call_plans->calls.front();
  if (call_plan.preserved_values.size() != 1) {
    return fail("cross-call preservation contract: expected exactly one preserved scalar across the call");
  }

  const auto& preserved = call_plan.preserved_values.front();
  const auto saved_it = std::find_if(frame_plan->saved_callee_registers.begin(),
                                     frame_plan->saved_callee_registers.end(),
                                     [&](const auto& saved) {
                                       return saved.register_name == "s1";
                                     });
  if (saved_it == frame_plan->saved_callee_registers.end()) {
    return fail("cross-call preservation contract: missing saved-register authority for s1");
  }
  if (preserved.value_id != carry->value_id ||
      prepare::prepared_value_name(prepared.names, preserved.value_name) != "carry" ||
      preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved.callee_saved_save_index !=
          std::optional<std::size_t>{saved_it->save_index} ||
      preserved.register_name != std::optional<std::string>{"s1"} ||
      preserved.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr}) {
    return fail("cross-call preservation contract: call_plans lost direct preserved-value authority");
  }
  if (!saved_it->slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(*saved_it->slot_placement) ||
      saved_it->slot_placement->bank != saved_it->bank ||
      saved_it->slot_placement->register_name != saved_it->register_name ||
      saved_it->slot_placement->occupied_register_names != saved_it->occupied_register_names ||
      saved_it->slot_placement->save_index != saved_it->save_index ||
      saved_it->slot_placement->register_placement != saved_it->placement ||
      saved_it->slot_placement->size_bytes != std::optional<std::size_t>{8} ||
      saved_it->slot_placement->align_bytes != std::optional<std::size_t>{8} ||
      !saved_it->slot_placement->fixed_location) {
    return fail("cross-call preservation contract: saved-register placement lost prepared slot facts");
  }
  if (saved_it->slot_placement->stack_offset_bytes !=
          std::optional<std::size_t>{frame_plan->frame_size_bytes} ||
      std::find(frame_plan->frame_slot_order.begin(),
                frame_plan->frame_slot_order.end(),
                *saved_it->slot_placement->slot_id) != frame_plan->frame_slot_order.end()) {
    return fail("cross-call preservation contract: saved-register placement perturbed frame-slot order");
  }

  return 0;
}

int check_prior_preservation_source_selection_contract() {
  const auto prepared =
      prepare_riscv_module(make_prior_preservation_source_selection_contract_module());
  const auto* call_plans =
      find_call_plans_function(prepared, "prior_preservation_source_selection_contract");
  const auto* storage_plan =
      find_storage_plan_function(prepared, "prior_preservation_source_selection_contract");
  const auto* carry =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "carry");
  if (call_plans == nullptr || call_plans->calls.size() != 2 || carry == nullptr) {
    return fail("prior-preservation source-selection contract: missing call or carry publication");
  }

  const auto& preserving_call = call_plans->calls.front();
  const auto preserved_it = std::find_if(
      preserving_call.preserved_values.begin(),
      preserving_call.preserved_values.end(),
      [carry](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == carry->value_id;
      });
  if (preserved_it == preserving_call.preserved_values.end()) {
    return fail("prior-preservation source-selection contract: missing prior preserved source");
  }

  const auto& consumer_call = call_plans->calls.back();
  if (consumer_call.arguments.size() != 1 ||
      !consumer_call.arguments.front().source_selection.has_value()) {
    return fail("prior-preservation source-selection contract: missing argument source selection");
  }
  const auto& source_selection = *consumer_call.arguments.front().source_selection;
  if (source_selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation ||
      source_selection.source_value_id != std::optional<prepare::PreparedValueId>{carry->value_id} ||
      source_selection.source_value_name != std::optional<c4c::ValueNameId>{preserved_it->value_name} ||
      source_selection.preserved_call_block_index != std::optional<std::size_t>{0} ||
      source_selection.preserved_call_instruction_index != std::optional<std::size_t>{2} ||
      source_selection.preservation_route != preserved_it->route ||
      source_selection.preserved_register_name != preserved_it->register_name ||
      source_selection.preserved_register_bank != preserved_it->register_bank ||
      source_selection.preserved_register_contiguous_width !=
          std::optional<std::size_t>{preserved_it->contiguous_width} ||
      source_selection.preserved_occupied_register_names != preserved_it->occupied_register_names ||
      source_selection.preserved_register_placement != preserved_it->register_placement ||
      source_selection.preserved_stack_slot_id != preserved_it->slot_id ||
      source_selection.preserved_stack_offset_bytes != preserved_it->stack_offset_bytes ||
      source_selection.preserved_stack_size_bytes != preserved_it->stack_size_bytes ||
      source_selection.preserved_stack_align_bytes != preserved_it->stack_align_bytes ||
      source_selection.preserved_callee_saved_save_index !=
          preserved_it->callee_saved_save_index ||
      source_selection.preserved_spill_slot_placement != preserved_it->spill_slot_placement) {
    return fail("prior-preservation source-selection contract: selection lost prepared preservation fields");
  }
  const auto prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("arg.source_selection=prior_preservation") == std::string::npos ||
      prepared_dump.find("selection_preserved_call_block=0") == std::string::npos ||
      prepared_dump.find("selection_preserved_call_inst=2") == std::string::npos ||
      (source_selection.preserved_callee_saved_save_index.has_value() &&
       prepared_dump.find("selection_preserved_save_index=") == std::string::npos)) {
    return fail("prior-preservation source-selection contract: printer hides prepared selection authority");
  }

  return 0;
}

int check_aarch64_formal_preservation_source_endpoint_contract() {
  const auto prepared = prepare_aarch64_module(
      make_aarch64_formal_preservation_source_endpoint_contract_module());
  const auto* call_plans = find_call_plans_function(
      prepared, "aarch64_formal_preservation_source_endpoint_contract");
  const auto* storage_plan = find_storage_plan_function(
      prepared, "aarch64_formal_preservation_source_endpoint_contract");
  const auto* formal =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "live.formal");
  if (call_plans == nullptr || call_plans->calls.size() != 1 || formal == nullptr) {
    return fail(
        "aarch64 formal preservation endpoint contract: missing call plan or formal storage");
  }

  const auto& call_plan = call_plans->calls.front();
  const auto preserved_it = std::find_if(
      call_plan.preserved_values.begin(),
      call_plan.preserved_values.end(),
      [formal](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == formal->value_id;
      });
  if (preserved_it == call_plan.preserved_values.end()) {
    return fail(
        "aarch64 formal preservation endpoint contract: missing preserved formal");
  }

  const auto& preserved = *preserved_it;
  if (preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved.preservation_source.storage_kind !=
          prepare::PreparedMoveStorageKind::Register ||
      preserved.preservation_source.register_name != std::optional<std::string>{"x0"} ||
      preserved.preservation_source.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
      preserved.preservation_source.value_id !=
          std::optional<prepare::PreparedValueId>{formal->value_id} ||
      preserved.preservation_destination.storage_kind !=
          prepare::PreparedMoveStorageKind::Register ||
      !preserved.preservation_destination.register_name.has_value() ||
      preserved.preservation_destination.register_name != preserved.register_name ||
      preserved.preservation_destination.register_name == preserved.preservation_source.register_name) {
    return fail(
        "aarch64 formal preservation endpoint contract: source must name live pre-call x0 and destination must name distinct callee-saved storage");
  }

  return 0;
}

int check_local_frame_address_source_selection_contract() {
  const auto prepared =
      prepare_aarch64_module(make_local_frame_address_source_selection_contract_module());
  const auto* call_plans =
      find_call_plans_function(prepared, "local_frame_address_source_selection_contract");
  const auto* stack_object = find_stack_object(prepared, "local.aggregate.0");
  const auto* frame_slot =
      stack_object == nullptr ? nullptr : find_frame_slot(prepared, stack_object->object_id);
  const auto source_name = prepared.names.value_names.find("local.aggregate");
  const auto entry_label = prepared.names.block_labels.find("entry");
  const auto* materialization =
      find_address_materialization(prepared,
                                   "local_frame_address_source_selection_contract",
                                   "entry",
                                   0,
                                   "local.aggregate");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1 ||
      stack_object == nullptr || frame_slot == nullptr ||
      source_name == c4c::kInvalidValueName ||
      entry_label == c4c::kInvalidBlockLabel || materialization == nullptr) {
    return fail("local frame address selection contract: missing prepared call or frame slot");
  }
  if (materialization->kind != prepare::PreparedAddressMaterializationKind::FrameSlot ||
      materialization->frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      materialization->byte_offset != static_cast<std::int64_t>(frame_slot->offset_bytes)) {
    return fail(
        "local frame address selection contract: producer did not publish explicit frame-slot authority");
  }

  const auto& arg = call_plans->calls.front().arguments.front();
  if (!arg.allows_local_aggregate_address_publication ||
      (arg.source_encoding != prepare::PreparedStorageEncodingKind::Register &&
       arg.source_encoding != prepare::PreparedStorageEncodingKind::ComputedAddress) ||
      !arg.source_selection.has_value()) {
    return fail(
        "local frame address selection contract: call argument lost prepared "
        "source selection");
  }
  const auto& selection = *arg.source_selection;
  if (selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::
              LocalFrameAddressMaterialization ||
      selection.source_value_name != std::optional<c4c::ValueNameId>{source_name} ||
      (selection.source_home_kind !=
           std::optional<prepare::PreparedValueHomeKind>{
               prepare::PreparedValueHomeKind::Register} &&
       selection.source_home_kind !=
           std::optional<prepare::PreparedValueHomeKind>{
               prepare::PreparedValueHomeKind::PointerBasePlusOffset}) ||
      selection.source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      selection.source_stack_offset_bytes !=
          std::optional<std::size_t>{frame_slot->offset_bytes} ||
      selection.address_materialization_block_label !=
          std::optional<c4c::BlockLabelId>{entry_label} ||
      selection.address_materialization_inst_index != std::optional<std::size_t>{0} ||
      selection.address_materialization_frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      selection.address_materialization_byte_offset !=
          std::optional<std::int64_t>{
              static_cast<std::int64_t>(frame_slot->offset_bytes)} ||
      selection.source_size_bytes != std::optional<std::size_t>{8} ||
      selection.source_align_bytes != std::optional<std::size_t>{8}) {
    return fail(
        "local frame address selection contract: missing prepared local frame "
        "address source fact");
  }
  if (arg.source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress &&
      (!arg.source_base_value_name.has_value() ||
       prepare::prepared_value_name(prepared.names, *arg.source_base_value_name) !=
           "local.aggregate.0" ||
       arg.source_pointer_byte_delta != std::optional<std::int64_t>{0})) {
    return fail(
        "local frame address selection contract: computed source lost local aggregate base authority");
  }
  return 0;
}

int check_derived_local_frame_address_source_selection_contract() {
  const auto prepared = prepare_aarch64_module(
      make_derived_local_frame_address_source_selection_contract_module());
  const auto* call_plans =
      find_call_plans_function(prepared,
                               "derived_local_frame_address_source_selection_contract");
  const auto* stack_object = find_stack_object(prepared, "local.aggregate.0");
  const auto* frame_slot =
      stack_object == nullptr ? nullptr : find_frame_slot(prepared, stack_object->object_id);
  const auto source_name = prepared.names.value_names.find("local.aggregate");
  const auto entry_label = prepared.names.block_labels.find("entry");
  const auto* materialization =
      find_address_materialization(prepared,
                                   "derived_local_frame_address_source_selection_contract",
                                   "entry",
                                   0,
                                   "local.aggregate");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1 ||
      stack_object == nullptr || frame_slot == nullptr ||
      source_name == c4c::kInvalidValueName ||
      entry_label == c4c::kInvalidBlockLabel || materialization == nullptr) {
    return fail(
        "derived local frame address selection contract: missing prepared call or frame slot");
  }
  if (materialization->kind != prepare::PreparedAddressMaterializationKind::FrameSlot ||
      materialization->frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      materialization->byte_offset !=
          static_cast<std::int64_t>(frame_slot->offset_bytes + 4U)) {
    return fail(
        "derived local frame address selection contract: producer did not publish explicit derived frame-slot authority");
  }

  const auto& arg = call_plans->calls.front().arguments.front();
  if (!arg.allows_local_aggregate_address_publication ||
      (arg.source_encoding != prepare::PreparedStorageEncodingKind::Register &&
       arg.source_encoding != prepare::PreparedStorageEncodingKind::ComputedAddress) ||
      !arg.source_selection.has_value()) {
    return fail(
        "derived local frame address selection contract: call argument lost "
        "prepared source selection");
  }
  const auto& selection = *arg.source_selection;
  if (selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::
              LocalFrameAddressMaterialization ||
      selection.source_value_name != std::optional<c4c::ValueNameId>{source_name} ||
      (selection.source_home_kind !=
           std::optional<prepare::PreparedValueHomeKind>{
               prepare::PreparedValueHomeKind::Register} &&
       selection.source_home_kind !=
           std::optional<prepare::PreparedValueHomeKind>{
               prepare::PreparedValueHomeKind::PointerBasePlusOffset}) ||
      selection.source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      selection.source_stack_offset_bytes !=
          std::optional<std::size_t>{frame_slot->offset_bytes + 4U} ||
      selection.source_pointer_byte_delta != std::optional<std::int64_t>{4} ||
      selection.address_materialization_block_label !=
          std::optional<c4c::BlockLabelId>{entry_label} ||
      selection.address_materialization_inst_index != std::optional<std::size_t>{0} ||
      selection.address_materialization_frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      selection.address_materialization_byte_offset !=
          std::optional<std::int64_t>{
              static_cast<std::int64_t>(frame_slot->offset_bytes + 4U)} ||
      selection.source_size_bytes != std::optional<std::size_t>{8} ||
      selection.source_align_bytes != std::optional<std::size_t>{8}) {
    return fail(
        "derived local frame address selection contract: missing prepared "
        "derived local frame address source fact");
  }
  if (arg.source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress &&
      (!arg.source_base_value_name.has_value() ||
       prepare::prepared_value_name(prepared.names, *arg.source_base_value_name) !=
           "local.aggregate.0" ||
       arg.source_pointer_byte_delta != std::optional<std::int64_t>{4})) {
    return fail(
        "derived local frame address selection contract: computed source lost immediate offset authority");
  }
  return 0;
}

prepare::PreparedBirModule make_byval_register_lane_aggregate_transport_contract_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("byval_register_lane_transport_contract");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("aggregate.byval");
  constexpr auto source_value_id = prepare::PreparedValueId{8101};
  constexpr auto source_slot_id = prepare::PreparedFrameSlotId{8102};
  constexpr auto source_object_id = prepare::PreparedObjectId{8103};

  bir::Function callee;
  callee.name = "extern_take_byval_register_lanes";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::Void;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "aggregate",
      .size_bytes = 16,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 16,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
          .byval_copy = true,
      },
      .is_byval = true,
  });
  prepared.module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "byval_register_lane_transport_contract";
  caller.return_type = bir::TypeKind::Void;
  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "extern_take_byval_register_lanes",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "aggregate.byval")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 16,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{};
  caller.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(caller));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
              .value_id = source_value_id,
              .function_name = function_name,
              .value_name = source_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = source_slot_id,
              .offset_bytes = std::size_t{96},
              .size_bytes = std::size_t{16},
              .align_bytes = std::size_t{8},
          }},
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .block_index = 0,
              .instruction_index = 0,
              .moves =
                  {prepare::PreparedMoveResolution{
                      .from_value_id = source_value_id,
                      .to_value_id = source_value_id,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_abi_index = std::size_t{0},
                      .destination_register_name = std::string{"x0"},
                      .destination_contiguous_width = 2,
                      .destination_occupied_register_names = {"x0", "x1"},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .reason = "call_arg_byval_aggregate_register_lanes",
                  }},
              .abi_bindings =
                  {prepare::PreparedAbiBinding{
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_abi_index = std::size_t{0},
                      .destination_register_name = std::string{"x0"},
                      .destination_contiguous_width = 2,
                      .destination_occupied_register_names = {"x0", "x1"},
                  }},
          }},
  });
  prepared.regalloc.functions.push_back(prepare::PreparedRegallocFunction{
      .function_name = function_name,
      .values =
          {prepare::PreparedRegallocValue{
              .value_id = source_value_id,
              .function_name = function_name,
              .value_name = source_name,
              .type = bir::TypeKind::Ptr,
              .register_class = prepare::PreparedRegisterClass::General,
              .allocation_status = prepare::PreparedAllocationStatus::Spilled,
          }},
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = source_object_id,
      .function_name = function_name,
      .value_name = source_name,
      .source_kind = "byval",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 16,
      .align_bytes = 8,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = source_slot_id,
      .object_id = source_object_id,
      .function_name = function_name,
      .offset_bytes = 96,
      .size_bytes = 16,
      .align_bytes = 8,
      .fixed_location = true,
  });

  prepare::populate_call_plans(prepared);
  return prepared;
}

int check_byval_register_lane_aggregate_transport_contract() {
  const auto prepared = make_byval_register_lane_aggregate_transport_contract_module();
  const auto* call_plans =
      find_call_plans_function(prepared, "byval_register_lane_transport_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1) {
    return fail(
        "byval register-lane transport contract: missing prepared call plan");
  }
  const auto& arg = call_plans->calls.front().arguments.front();
  if (!arg.source_selection.has_value() ||
      arg.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane) {
    return fail(
        "byval register-lane transport contract: missing prepared byval source selection");
  }
  if (!arg.aggregate_transport.has_value()) {
    return fail(
        "byval register-lane transport contract: missing aggregate transport publication");
  }
  const auto& transport = *arg.aggregate_transport;
  if (transport.kind !=
          prepare::PreparedAggregateTransportKind::ByvalRegisterLanes ||
      transport.payload_size_bytes != 16 || transport.copy_size_bytes != 16 ||
      transport.payload_align_bytes != 8 || transport.copy_align_bytes != 8 ||
      transport.source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{8102}} ||
      transport.source_stack_offset_bytes != std::optional<std::size_t>{96} ||
      transport.chunks.size() != 2 || transport.lanes.size() != 2) {
    return fail(
        "byval register-lane transport contract: aggregate transport lost size, source, or lane facts");
  }
  if (transport.chunks[0].payload_offset_bytes != 0 ||
      transport.chunks[0].source_offset_bytes != 96 ||
      transport.chunks[0].size_bytes != 8 ||
      transport.lanes[0].destination_register_name != std::optional<std::string>{"x0"} ||
      transport.lanes[1].lane_payload_offset_bytes != 8 ||
      transport.lanes[1].source_offset_bytes != 104 ||
      transport.lanes[1].destination_register_name != std::optional<std::string>{"x1"} ||
      transport.lanes[1].destination_occupied_register_names !=
          std::vector<std::string>{"x0", "x1"}) {
    return fail(
        "byval register-lane transport contract: aggregate transport lost chunk/lane binding facts");
  }

  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("arg.aggregate_transport=byval_register_lanes") ==
          std::string::npos ||
      prepared_dump.find("payload_size=16") == std::string::npos ||
      prepared_dump.find("copy_size=16") == std::string::npos ||
      prepared_dump.find("lane index=1") == std::string::npos ||
      prepared_dump.find("chunk index=1") == std::string::npos ||
      prepared_dump.find("dest_reg=x1") == std::string::npos) {
    return fail(
        "byval register-lane transport contract: prepared dump no longer exposes aggregate transport facts");
  }

  return 0;
}

bir::Module make_aarch64_global_byval_register_lane_source_selection_contract_module() {
  bir::Module module;
  module.target_triple = "aarch64-linux-gnu";

  bir::Function callee;
  callee.name = "same_module_take_global_byval";
  callee.return_type = bir::TypeKind::Void;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "aggregate",
      .size_bytes = 1,
      .align_bytes = 1,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 1,
          .align_bytes = 1,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
          .byval_copy = true,
      },
      .is_byval = true,
  });
  bir::Block callee_entry;
  callee_entry.label = "entry";
  callee_entry.terminator = bir::ReturnTerminator{};
  callee.blocks.push_back(std::move(callee_entry));
  module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "global_byval_register_lane_source_selection_contract";
  caller.return_type = bir::TypeKind::Void;
  caller.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "aggregate.byval",
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
      .name = "same_module.global.byte",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
  });
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "materialized.global.byte",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(
          bir::TypeKind::I8, "aggregate.byval.global.aggregate.load.0"),
      .slot_name = "same_module.global.byte",
      .align_bytes = 1,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "materialized.global.byte",
      .value = bir::Value::named(
          bir::TypeKind::I8, "aggregate.byval.global.aggregate.load.0"),
      .align_bytes = 1,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "same_module_take_global_byval",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "aggregate.byval")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 1,
          .align_bytes = 1,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator = bir::ReturnTerminator{};
  caller.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(caller));
  return module;
}

int check_aarch64_global_byval_register_lane_source_selection_contract() {
  const auto prepared = prepare_aarch64_module(
      make_aarch64_global_byval_register_lane_source_selection_contract_module());
  const auto* call_plans = find_call_plans_function(
      prepared, "global_byval_register_lane_source_selection_contract");
  const auto* source_slot_object =
      find_stack_object(prepared, "same_module.global.byte");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1 ||
      source_slot_object == nullptr) {
    return fail(
        "AArch64 global byval source-selection contract: missing prepared call or source slot");
  }
  const auto* source_frame_slot =
      find_frame_slot(prepared, source_slot_object->object_id);
  if (source_frame_slot == nullptr) {
    return fail(
        "AArch64 global byval source-selection contract: missing source frame slot");
  }

  const auto& arg = call_plans->calls.front().arguments.front();
  const auto loaded_lane_name =
      prepared.names.value_names.find("aggregate.byval.global.aggregate.load.0");
  if (!arg.source_selection.has_value() ||
      arg.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane ||
      arg.source_selection->source_value_name !=
          std::optional<c4c::ValueNameId>{loaded_lane_name} ||
      arg.source_selection->source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{source_frame_slot->slot_id} ||
      arg.source_selection->source_stack_offset_bytes !=
          std::optional<std::size_t>{source_frame_slot->offset_bytes} ||
      arg.source_selection->source_size_bytes != std::optional<std::size_t>{1} ||
      arg.source_selection->source_align_bytes != std::optional<std::size_t>{1} ||
      arg.source_selection->byval_lane_source_instruction_index !=
          std::optional<std::size_t>{0}) {
    return fail(
        "AArch64 global byval source-selection contract: byval lane source did not select the global aggregate payload load");
  }
  if (!arg.aggregate_transport.has_value() ||
      arg.aggregate_transport->kind !=
          prepare::PreparedAggregateTransportKind::ByvalRegisterLanes ||
      arg.aggregate_transport->source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{source_frame_slot->slot_id} ||
      arg.aggregate_transport->source_stack_offset_bytes !=
          std::optional<std::size_t>{source_frame_slot->offset_bytes} ||
      arg.aggregate_transport->payload_size_bytes != 1 ||
      arg.aggregate_transport->lanes.size() != 1 ||
      arg.aggregate_transport->lanes.front().destination_register_name !=
          std::optional<std::string>{"x0"}) {
    return fail(
        "AArch64 global byval source-selection contract: aggregate transport did not publish the loaded payload lane source");
  }
  return 0;
}

int check_missing_local_aggregate_frame_slot_address_source_selection_contract() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("missing_local_aggregate_selection_contract");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("local.aggregate");
  const auto source_lane_name = prepared.names.value_names.intern("local.aggregate.0");
  constexpr auto source_value_id = prepare::PreparedValueId{8001};
  constexpr auto source_slot_id = prepare::PreparedFrameSlotId{8002};
  constexpr auto local_slot_id = prepare::PreparedFrameSlotId{8003};

  bir::Function function;
  function.name = "missing_local_aggregate_selection_contract";
  function.return_type = bir::TypeKind::Void;
  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.consume_missing_local_address",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "local.aggregate")},
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
  entry.terminator = bir::ReturnTerminator{};
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
              .value_id = source_value_id,
              .function_name = function_name,
              .value_name = source_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = source_slot_id,
              .offset_bytes = std::size_t{144},
              .size_bytes = std::size_t{8},
              .align_bytes = std::size_t{8},
          }},
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .block_index = 0,
              .instruction_index = 0,
              .moves =
                  {prepare::PreparedMoveResolution{
                      .from_value_id = source_value_id,
                      .to_value_id = source_value_id,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_abi_index = std::size_t{0},
                      .destination_register_name = std::string{"x0"},
                      .destination_contiguous_width = 1,
                      .destination_occupied_register_names = {"x0"},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .reason = "call_arg_stack_to_register",
                  }},
              .abi_bindings =
                  {prepare::PreparedAbiBinding{
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_abi_index = std::size_t{0},
                      .destination_register_name = std::string{"x0"},
                      .destination_contiguous_width = 1,
                      .destination_occupied_register_names = {"x0"},
                  }},
          }},
  });
  prepared.regalloc.functions.push_back(prepare::PreparedRegallocFunction{
      .function_name = function_name,
      .values =
          {prepare::PreparedRegallocValue{
              .value_id = source_value_id,
              .function_name = function_name,
              .value_name = source_name,
              .type = bir::TypeKind::Ptr,
              .register_class = prepare::PreparedRegisterClass::General,
              .allocation_status = prepare::PreparedAllocationStatus::Spilled,
          }},
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = prepare::PreparedObjectId{8004},
      .function_name = function_name,
      .value_name = source_lane_name,
      .source_kind = "local_slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .address_exposed = false,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = local_slot_id,
      .object_id = prepare::PreparedObjectId{8004},
      .function_name = function_name,
      .offset_bytes = 64,
      .size_bytes = 4,
      .align_bytes = 4,
      .fixed_location = true,
  });

  prepare::populate_call_plans(prepared);
  const auto* call_plans =
      find_call_plans_function(prepared,
                               "missing_local_aggregate_selection_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 1) {
    return fail(
        "missing local aggregate frame-slot address contract: missing prepared call plan");
  }
  const auto& arg = call_plans->calls.front().arguments.front();
  if (arg.allows_local_aggregate_address_publication ||
      arg.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      arg.source_slot_id != std::optional<prepare::PreparedFrameSlotId>{source_slot_id} ||
      !arg.source_selection.has_value()) {
    return fail(
        "missing local aggregate frame-slot address contract: lost stack-homed argument facts");
  }
  const auto& selection = *arg.source_selection;
  if (selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress ||
      selection.source_value_id != std::optional<prepare::PreparedValueId>{source_value_id} ||
      selection.source_value_name != std::optional<c4c::ValueNameId>{source_name} ||
      selection.source_home_kind !=
          std::optional<prepare::PreparedValueHomeKind>{
              prepare::PreparedValueHomeKind::StackSlot} ||
      selection.source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{local_slot_id} ||
      selection.source_stack_offset_bytes != std::optional<std::size_t>{64} ||
      selection.source_size_bytes != std::optional<std::size_t>{8} ||
      selection.source_align_bytes != std::optional<std::size_t>{8}) {
    return fail(
        "missing local aggregate frame-slot address contract: prepared plan did not own the object address source");
  }
  const auto publication_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(arg);
  if (!publication_need.available ||
      publication_need.kind !=
          prepare::PreparedMissingFrameSlotCallArgumentPublicationKind::
              FrameSlotAddress ||
      publication_need.source_value_id != source_value_id ||
      publication_need.source_selection != &selection ||
      publication_need.destination_register_bank !=
          prepare::PreparedRegisterBank::Gpr ||
      publication_need.destination_contiguous_width != 1 ||
      !publication_need.source_materializes_address ||
      publication_need.may_emit_local_aggregate_address_payload) {
    return fail(
        "missing local aggregate frame-slot address contract: shared missing frame-slot publication need was not visible");
  }
  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("missing_frame_slot_arg_publication=yes") ==
          std::string::npos ||
      prepared_dump.find("missing_frame_slot_arg_kind=frame_slot_address") ==
          std::string::npos ||
      prepared_dump.find("missing_frame_slot_arg_source_materializes_address=yes") ==
          std::string::npos ||
      prepared_dump.find("missing_frame_slot_arg_may_emit_local_payload=no") ==
          std::string::npos) {
    return fail(
        "missing local aggregate frame-slot address contract: prepared dump lost shared missing frame-slot publication visibility");
  }
  return 0;
}

int check_frame_slot_address_source_route_bridge_contract() {
  prepare::PreparedCallArgumentSourceSelection selection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
      .source_value_id = prepare::PreparedValueId{41},
      .source_value_name = c4c::ValueNameId{42},
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .source_slot_id = prepare::PreparedFrameSlotId{7},
      .source_stack_offset_bytes = std::size_t{64},
      .source_size_bytes = std::size_t{8},
      .source_align_bytes = std::size_t{8},
      .address_materialization_block_label = c4c::BlockLabelId{3},
      .address_materialization_inst_index = std::size_t{9},
      .address_materialization_frame_slot_id = prepare::PreparedFrameSlotId{7},
      .address_materialization_byte_offset = std::int64_t{64},
  };

  const auto route = prepare::as_frame_slot_address_source_route(selection);
  if (!route.has_value() ||
      route->source_value_id !=
          std::optional<prepare::PreparedValueId>{prepare::PreparedValueId{41}} ||
      route->source_value_name !=
          std::optional<c4c::ValueNameId>{c4c::ValueNameId{42}} ||
      route->source_home_kind !=
          std::optional<prepare::PreparedValueHomeKind>{
              prepare::PreparedValueHomeKind::StackSlot} ||
      route->source_slot_id != prepare::PreparedFrameSlotId{7} ||
      route->source_stack_offset_bytes != 64 ||
      route->source_size_bytes != 8 ||
      route->source_align_bytes != 8 ||
      !route->address_materialization.has_value() ||
      route->address_materialization->block_label != c4c::BlockLabelId{3} ||
      route->address_materialization->instruction_index != 9 ||
      route->address_materialization->frame_slot_id !=
          prepare::PreparedFrameSlotId{7} ||
      route->address_materialization->byte_offset != 64) {
    return fail(
        "frame-slot address route bridge contract: valid route did not expose typed payload");
  }

  auto missing_required = selection;
  missing_required.source_size_bytes = std::nullopt;
  if (prepare::as_frame_slot_address_source_route(missing_required).has_value()) {
    return fail(
        "frame-slot address route bridge contract: missing required extent was accepted");
  }

  auto partial_materialization = selection;
  partial_materialization.address_materialization_byte_offset = std::nullopt;
  if (prepare::as_frame_slot_address_source_route(partial_materialization)
          .has_value()) {
    return fail(
        "frame-slot address route bridge contract: partial materialization was accepted");
  }

  auto contradictory_materialization = selection;
  contradictory_materialization.address_materialization_frame_slot_id =
      prepare::PreparedFrameSlotId{8};
  if (prepare::as_frame_slot_address_source_route(contradictory_materialization)
          .has_value()) {
    return fail(
        "frame-slot address route bridge contract: contradictory materialization slot was accepted");
  }

  auto cross_route_payload = selection;
  cross_route_payload.preserved_call_instruction_index = std::size_t{11};
  if (prepare::as_frame_slot_address_source_route(cross_route_payload).has_value()) {
    return fail(
        "frame-slot address route bridge contract: cross-route preservation payload was accepted");
  }

  prepare::PreparedCallArgumentPlan argument;
  argument.source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot;
  argument.source_value_id = prepare::PreparedValueId{41};
  argument.destination_register_bank = prepare::PreparedRegisterBank::Gpr;
  argument.destination_contiguous_width = 1;
  argument.source_selection = selection;
  const auto publication_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          argument);
  if (!publication_need.available ||
      publication_need.kind !=
          prepare::PreparedMissingFrameSlotCallArgumentPublicationKind::
              FrameSlotAddress ||
      publication_need.source_selection != &*argument.source_selection) {
    return fail(
        "frame-slot address route bridge contract: valid typed route was not visible to publication bridge");
  }

  argument.source_selection = contradictory_materialization;
  const auto rejected_publication_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          argument);
  if (rejected_publication_need.available) {
    return fail(
        "frame-slot address route bridge contract: contradictory typed route remained visible to publication bridge");
  }

  return 0;
}

int check_frame_slot_value_source_route_bridge_contract() {
  prepare::PreparedCallArgumentSourceSelection selection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
      .source_value_id = prepare::PreparedValueId{51},
      .source_value_name = c4c::ValueNameId{52},
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .source_slot_id = prepare::PreparedFrameSlotId{9},
      .source_stack_offset_bytes = std::size_t{72},
      .source_size_bytes = std::size_t{4},
      .source_align_bytes = std::size_t{4},
  };

  const auto route = prepare::as_frame_slot_value_source_route(selection);
  if (!route.has_value() ||
      route->source_value_id != prepare::PreparedValueId{51} ||
      route->source_value_name != c4c::ValueNameId{52} ||
      route->source_home_kind != prepare::PreparedValueHomeKind::StackSlot ||
      route->source_slot_id != prepare::PreparedFrameSlotId{9} ||
      route->source_stack_offset_bytes != 72 ||
      route->source_size_bytes != 4 ||
      route->source_align_bytes != 4) {
    return fail(
        "frame-slot value route bridge contract: valid route did not expose typed payload");
  }

  auto missing_identity = selection;
  missing_identity.source_value_name = std::nullopt;
  if (prepare::as_frame_slot_value_source_route(missing_identity).has_value()) {
    return fail(
        "frame-slot value route bridge contract: missing source value name was accepted");
  }

  auto missing_offset = selection;
  missing_offset.source_stack_offset_bytes = std::nullopt;
  if (prepare::as_frame_slot_value_source_route(missing_offset).has_value()) {
    return fail(
        "frame-slot value route bridge contract: missing source stack offset was accepted");
  }

  auto wrong_home = selection;
  wrong_home.source_home_kind = prepare::PreparedValueHomeKind::Register;
  if (prepare::as_frame_slot_value_source_route(wrong_home).has_value()) {
    return fail(
        "frame-slot value route bridge contract: non-stack source home was accepted");
  }

  auto address_payload = selection;
  address_payload.address_materialization_inst_index = std::size_t{3};
  if (prepare::as_frame_slot_value_source_route(address_payload).has_value()) {
    return fail(
        "frame-slot value route bridge contract: address materialization payload was accepted");
  }

  auto cross_route_payload = selection;
  cross_route_payload.byval_lane_extent_bytes = std::size_t{4};
  if (prepare::as_frame_slot_value_source_route(cross_route_payload).has_value()) {
    return fail(
        "frame-slot value route bridge contract: byval lane payload was accepted");
  }

  prepare::PreparedCallArgumentPlan argument;
  argument.source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot;
  argument.source_value_id = prepare::PreparedValueId{51};
  argument.destination_register_bank = prepare::PreparedRegisterBank::Gpr;
  argument.destination_contiguous_width = 1;
  argument.source_selection = selection;
  const auto publication_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          argument);
  if (!publication_need.available ||
      publication_need.kind !=
          prepare::PreparedMissingFrameSlotCallArgumentPublicationKind::
              FrameSlotValue ||
      publication_need.source_selection != &*argument.source_selection) {
    return fail(
        "frame-slot value route bridge contract: valid typed route was not visible to publication bridge");
  }

  argument.source_selection = address_payload;
  const auto rejected_publication_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          argument);
  if (rejected_publication_need.available) {
    return fail(
        "frame-slot value route bridge contract: invalid typed route remained visible to publication bridge");
  }

  return 0;
}

int check_local_frame_address_materialization_route_bridge_contract() {
  prepare::PreparedCallArgumentSourceSelection selection{
      .kind = prepare::PreparedCallArgumentSourceSelectionKind::
          LocalFrameAddressMaterialization,
      .source_value_id = prepare::PreparedValueId{61},
      .source_value_name = c4c::ValueNameId{62},
      .source_home_kind = prepare::PreparedValueHomeKind::Register,
      .source_slot_id = prepare::PreparedFrameSlotId{11},
      .source_stack_offset_bytes = std::size_t{84},
      .source_size_bytes = std::size_t{16},
      .source_align_bytes = std::size_t{8},
      .source_base_value_id = prepare::PreparedValueId{60},
      .source_pointer_byte_delta = std::int64_t{4},
      .address_materialization_block_label = c4c::BlockLabelId{6},
      .address_materialization_inst_index = std::size_t{12},
      .address_materialization_frame_slot_id = prepare::PreparedFrameSlotId{11},
      .address_materialization_byte_offset = std::int64_t{84},
  };

  const auto route =
      prepare::as_local_frame_address_materialization_route(selection);
  if (!route.has_value() ||
      route->source_value_id != prepare::PreparedValueId{61} ||
      route->source_value_name != c4c::ValueNameId{62} ||
      route->source_home_kind != prepare::PreparedValueHomeKind::Register ||
      route->source_base_value_id !=
          std::optional<prepare::PreparedValueId>{prepare::PreparedValueId{60}} ||
      route->source_pointer_byte_delta != 4 ||
      route->source_slot_id != prepare::PreparedFrameSlotId{11} ||
      route->source_stack_offset_bytes != 84 ||
      route->source_size_bytes != 16 ||
      route->source_align_bytes != 8 ||
      route->address_materialization_block_label != c4c::BlockLabelId{6} ||
      route->address_materialization_inst_index != 12 ||
      route->address_materialization_frame_slot_id !=
          prepare::PreparedFrameSlotId{11} ||
      route->address_materialization_byte_offset != 84) {
    return fail(
        "local frame address materialization route bridge contract: valid route did not expose typed payload");
  }

  auto missing_delta = selection;
  missing_delta.source_pointer_byte_delta = std::nullopt;
  if (prepare::as_local_frame_address_materialization_route(missing_delta)
          .has_value()) {
    return fail(
        "local frame address materialization route bridge contract: missing pointer delta was accepted");
  }

  auto wrong_home = selection;
  wrong_home.source_home_kind = prepare::PreparedValueHomeKind::StackSlot;
  if (prepare::as_local_frame_address_materialization_route(wrong_home)
          .has_value()) {
    return fail(
        "local frame address materialization route bridge contract: wrong source home was accepted");
  }

  auto mismatched_slot = selection;
  mismatched_slot.address_materialization_frame_slot_id =
      prepare::PreparedFrameSlotId{12};
  if (prepare::as_local_frame_address_materialization_route(mismatched_slot)
          .has_value()) {
    return fail(
        "local frame address materialization route bridge contract: mismatched materialization slot was accepted");
  }

  auto mismatched_offset = selection;
  mismatched_offset.address_materialization_byte_offset = std::int64_t{80};
  if (prepare::as_local_frame_address_materialization_route(mismatched_offset)
          .has_value()) {
    return fail(
        "local frame address materialization route bridge contract: mismatched materialization offset was accepted");
  }

  auto byval_payload = selection;
  byval_payload.byval_lane_extent_bytes = std::size_t{8};
  if (prepare::as_local_frame_address_materialization_route(byval_payload)
          .has_value()) {
    return fail(
        "local frame address materialization route bridge contract: byval lane payload was accepted");
  }

  prepare::PreparedCallArgumentPlan argument;
  argument.source_encoding = prepare::PreparedStorageEncodingKind::Register;
  argument.source_value_id = prepare::PreparedValueId{61};
  argument.destination_register_bank = prepare::PreparedRegisterBank::Gpr;
  argument.destination_contiguous_width = 1;
  argument.allows_local_aggregate_address_publication = true;
  argument.source_selection = selection;
  const auto publication_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          argument);
  if (publication_need.available) {
    return fail(
        "local frame address materialization route bridge contract: non-frame-slot source encoding should not request missing frame-slot publication");
  }
  argument.source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot;
  const auto frame_slot_publication_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          argument);
  if (!frame_slot_publication_need.available ||
      frame_slot_publication_need.kind !=
          prepare::PreparedMissingFrameSlotCallArgumentPublicationKind::
              LocalFrameAddressMaterialization ||
      frame_slot_publication_need.source_selection != &*argument.source_selection ||
      !frame_slot_publication_need.source_materializes_address ||
      !frame_slot_publication_need.may_emit_local_aggregate_address_payload) {
    return fail(
        "local frame address materialization route bridge contract: valid typed route was not visible to publication bridge");
  }

  argument.source_selection = mismatched_offset;
  const auto rejected_publication_need =
      prepare::find_prepared_missing_frame_slot_call_argument_publication_need(
          argument);
  if (rejected_publication_need.available) {
    return fail(
        "local frame address materialization route bridge contract: invalid typed route remained visible to publication bridge");
  }

  return 0;
}

prepare::PreparedBirModule make_rv64_same_module_byval_stack_copy_contract_module(
    bool publish_byval_metadata,
    bool duplicate_payload_materialization) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = riscv_target_profile();
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("rv64_byval_stack_copy_contract");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("aggregate.addr");
  const auto result_name = prepared.names.value_names.intern("call.result");
  constexpr auto source_value_id = prepare::PreparedValueId{8301};
  constexpr auto result_value_id = prepare::PreparedValueId{8302};
  constexpr auto source_slot_id = prepare::PreparedFrameSlotId{8303};
  constexpr auto source_object_id = prepare::PreparedObjectId{8304};

  const bir::CallArgAbiInfo byval_abi{
      .type = bir::TypeKind::Ptr,
      .size_bytes = publish_byval_metadata ? 24U : 0U,
      .align_bytes = publish_byval_metadata ? 8U : 0U,
      .primary_class = publish_byval_metadata ? bir::AbiValueClass::Memory
                                              : bir::AbiValueClass::Integer,
      .passed_in_register = false,
      .passed_on_stack = true,
      .byval_copy = publish_byval_metadata,
  };

  bir::Function callee;
  callee.name = "rv64_same_module_byval_callee";
  callee.return_type = bir::TypeKind::I32;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "aggregate",
      .size_bytes = byval_abi.size_bytes,
      .align_bytes = byval_abi.align_bytes,
      .abi = byval_abi,
      .is_byval = publish_byval_metadata,
  });
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "scalar",
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
  bir::Block callee_entry;
  callee_entry.label = "entry";
  callee_entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::immediate_i32(7)};
  callee.blocks.push_back(std::move(callee_entry));
  prepared.module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "rv64_byval_stack_copy_contract";
  caller.return_type = bir::TypeKind::I32;
  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.result"),
      .callee = "rv64_same_module_byval_callee",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "aggregate.addr"),
               bir::Value::immediate_i32(19)},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .arg_abi = {byval_abi,
                  bir::CallArgAbiInfo{
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
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "call.result")};
  caller.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(caller));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .address_materializations =
          {prepare::PreparedAddressMaterialization{
              .function_name = function_name,
              .block_label = block_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
              .result_value_name = source_name,
              .result_value_id = source_value_id,
              .result_home_kind = prepare::PreparedValueHomeKind::Register,
              .frame_slot_id = source_slot_id,
              .byte_offset = 96,
          }},
  });
  if (duplicate_payload_materialization) {
    prepared.addressing.functions.back().address_materializations.push_back(
        prepare::PreparedAddressMaterialization{
            .function_name = function_name,
            .block_label = block_label,
            .inst_index = 0,
            .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
            .result_value_name = source_name,
            .result_value_id = source_value_id,
            .result_home_kind = prepare::PreparedValueHomeKind::Register,
            .frame_slot_id = source_slot_id,
            .byte_offset = 128,
        });
  }
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
               .value_id = source_value_id,
               .function_name = function_name,
               .value_name = source_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = std::string{"s2"},
               .size_bytes = std::size_t{8},
               .align_bytes = std::size_t{8},
           },
           prepare::PreparedValueHome{
               .value_id = result_value_id,
               .function_name = function_name,
               .value_name = result_name,
               .kind = prepare::PreparedValueHomeKind::Register,
               .register_name = std::string{"t0"},
               .size_bytes = std::size_t{4},
               .align_bytes = std::size_t{4},
           }},
      .move_bundles =
          {prepare::PreparedMoveBundle{
               .function_name = function_name,
               .phase = prepare::PreparedMovePhase::BeforeCall,
               .block_index = 0,
               .instruction_index = 0,
               .moves =
                   {prepare::PreparedMoveResolution{
                       .from_value_id = source_value_id,
                       .to_value_id = source_value_id,
                       .destination_kind =
                           prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                       .destination_storage_kind =
                           prepare::PreparedMoveStorageKind::StackSlot,
                       .destination_abi_index = std::size_t{0},
                       .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                       .reason = "call_arg_register_to_stack",
                   }},
               .abi_bindings =
                   {prepare::PreparedAbiBinding{
                        .destination_kind =
                            prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                        .destination_storage_kind =
                            prepare::PreparedMoveStorageKind::StackSlot,
                        .destination_abi_index = std::size_t{0},
                    },
                    prepare::PreparedAbiBinding{
                        .destination_kind =
                            prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                        .destination_storage_kind =
                            prepare::PreparedMoveStorageKind::Register,
                        .destination_abi_index = std::size_t{1},
                        .destination_register_name = std::string{"a1"},
                        .destination_contiguous_width = 1,
                        .destination_occupied_register_names = {"a1"},
                    }},
           },
           prepare::PreparedMoveBundle{
               .function_name = function_name,
               .phase = prepare::PreparedMovePhase::AfterCall,
               .block_index = 0,
               .instruction_index = 0,
               .moves =
                   {prepare::PreparedMoveResolution{
                       .from_value_id = result_value_id,
                       .to_value_id = result_value_id,
                       .destination_kind =
                           prepare::PreparedMoveDestinationKind::CallResultAbi,
                       .destination_storage_kind =
                           prepare::PreparedMoveStorageKind::Register,
                       .destination_register_name = std::string{"a0"},
                       .destination_contiguous_width = 1,
                       .destination_occupied_register_names = {"a0"},
                       .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                       .reason = "call_result_register_to_value",
                   }},
               .abi_bindings =
                   {prepare::PreparedAbiBinding{
                       .destination_kind =
                           prepare::PreparedMoveDestinationKind::CallResultAbi,
                       .destination_storage_kind =
                           prepare::PreparedMoveStorageKind::Register,
                       .destination_register_name = std::string{"a0"},
                       .destination_contiguous_width = 1,
                       .destination_occupied_register_names = {"a0"},
                   }},
           }},
  });
  prepared.regalloc.functions.push_back(prepare::PreparedRegallocFunction{
      .function_name = function_name,
      .values =
          {prepare::PreparedRegallocValue{
               .value_id = source_value_id,
               .function_name = function_name,
               .value_name = source_name,
               .type = bir::TypeKind::Ptr,
               .register_class = prepare::PreparedRegisterClass::AggregateAddress,
               .allocation_status = prepare::PreparedAllocationStatus::AssignedRegister,
           },
           prepare::PreparedRegallocValue{
               .value_id = result_value_id,
               .function_name = function_name,
               .value_name = result_name,
               .type = bir::TypeKind::I32,
               .register_class = prepare::PreparedRegisterClass::General,
               .allocation_status = prepare::PreparedAllocationStatus::AssignedRegister,
           }},
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = source_object_id,
      .function_name = function_name,
      .value_name = source_name,
      .source_kind = "local_slot",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 24,
      .align_bytes = 8,
      .address_exposed = true,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = source_slot_id,
      .object_id = source_object_id,
      .function_name = function_name,
      .offset_bytes = 96,
      .size_bytes = 24,
      .align_bytes = 8,
      .fixed_location = true,
  });

  prepare::populate_call_plans(prepared);
  return prepared;
}

int check_rv64_same_module_byval_stack_copy_call_contract() {
  const auto prepared =
      make_rv64_same_module_byval_stack_copy_contract_module(true, false);
  const auto* call_plans =
      find_call_plans_function(prepared, "rv64_byval_stack_copy_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 2 ||
      !call_plans->calls.front().result.has_value()) {
    return fail(
        "RV64 same-module byval stack-copy contract: missing prepared call shape");
  }
  const auto& call = call_plans->calls.front();
  const auto& arg0 = call.arguments[0];
  const auto& arg1 = call.arguments[1];
  const auto& result = *call.result;
  if (call.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule ||
      call.direct_callee_name !=
          std::optional<std::string>{"rv64_same_module_byval_callee"} ||
      call.outgoing_stack_argument_area.has_value()) {
    return fail(
        "RV64 same-module byval stack-copy contract: lost direct same-module or static stack-copy boundary");
  }
  if (arg0.value_bank != prepare::PreparedRegisterBank::AggregateAddress ||
      arg0.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
      arg0.source_register_name != std::optional<std::string>{"s2"} ||
      arg0.source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::AggregateAddress} ||
      arg0.destination_register_name.has_value() ||
      arg0.destination_stack_offset_bytes.has_value() ||
      !arg0.source_selection.has_value() ||
      !arg0.aggregate_transport.has_value()) {
    return fail(
        "RV64 same-module byval stack-copy contract: arg0 lost aggregate-address register-source payload authority");
  }
  const auto& selection = *arg0.source_selection;
  const auto& transport = *arg0.aggregate_transport;
  if (selection.kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::
              LocalFrameAddressMaterialization ||
      selection.source_stack_offset_bytes != std::optional<std::size_t>{96} ||
      selection.source_size_bytes != std::optional<std::size_t>{24} ||
      selection.source_align_bytes != std::optional<std::size_t>{8} ||
      transport.kind != prepare::PreparedAggregateTransportKind::StackCopy ||
      transport.payload_size_bytes != 24 ||
      transport.payload_align_bytes != 8 ||
      transport.copy_size_bytes != 24 ||
      transport.copy_align_bytes != 8 ||
      transport.source_stack_offset_bytes != std::optional<std::size_t>{96} ||
      transport.destination_stack_offset_bytes.has_value() ||
      transport.chunks.size() != 3 ||
      !transport.lanes.empty()) {
    return fail(
        "RV64 same-module byval stack-copy contract: arg0 did not publish explicit byval payload size/alignment and chunks");
  }
  if (transport.chunks[0].source_offset_bytes != 96 ||
      transport.chunks[1].source_offset_bytes != 104 ||
      transport.chunks[2].source_offset_bytes != 112 ||
      transport.chunks[2].size_bytes != 8) {
    return fail(
        "RV64 same-module byval stack-copy contract: payload chunk offsets are not explicit");
  }
  if (arg1.value_bank != prepare::PreparedRegisterBank::Gpr ||
      arg1.source_encoding != prepare::PreparedStorageEncodingKind::Immediate ||
      !arg1.source_literal.has_value() ||
      arg1.source_literal->immediate != 19 ||
      arg1.destination_register_name != std::optional<std::string>{"a1"} ||
      arg1.destination_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr}) {
    return fail(
        "RV64 same-module byval stack-copy contract: scalar arg1 lost a1 placement");
  }
  if (result.source_register_name != std::optional<std::string>{"a0"} ||
      result.source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::Gpr} ||
      result.destination_register_name != std::optional<std::string>{"t0"}) {
    return fail(
        "RV64 same-module byval stack-copy contract: result lost a0-to-caller value authority");
  }

  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("arg.aggregate_transport=stack_copy") ==
          std::string::npos ||
      prepared_dump.find("payload_size=24") == std::string::npos ||
      prepared_dump.find("copy_align=8") == std::string::npos ||
      prepared_dump.find("chunk index=2") == std::string::npos) {
    return fail(
        "RV64 same-module byval stack-copy contract: prepared dump lost payload contract facts");
  }
  return 0;
}

int check_rv64_same_module_byval_stack_copy_contract_fails_closed() {
  const auto missing_metadata =
      make_rv64_same_module_byval_stack_copy_contract_module(false, false);
  const auto* missing_calls =
      find_call_plans_function(missing_metadata, "rv64_byval_stack_copy_contract");
  if (missing_calls == nullptr || missing_calls->calls.size() != 1 ||
      missing_calls->calls.front().arguments.empty()) {
    return fail(
        "RV64 same-module byval fail-closed contract: missing metadata fixture lost call plan");
  }
  if (missing_calls->calls.front().arguments.front().aggregate_transport.has_value()) {
    return fail(
        "RV64 same-module byval fail-closed contract: missing byval metadata still published payload transport");
  }

  const auto ambiguous_payload =
      make_rv64_same_module_byval_stack_copy_contract_module(true, true);
  const auto* ambiguous_calls =
      find_call_plans_function(ambiguous_payload, "rv64_byval_stack_copy_contract");
  if (ambiguous_calls == nullptr || ambiguous_calls->calls.size() != 1 ||
      ambiguous_calls->calls.front().arguments.empty()) {
    return fail(
        "RV64 same-module byval fail-closed contract: ambiguous fixture lost call plan");
  }
  const auto& ambiguous_arg = ambiguous_calls->calls.front().arguments.front();
  if (ambiguous_arg.source_selection.has_value() ||
      ambiguous_arg.aggregate_transport.has_value()) {
    return fail(
        "RV64 same-module byval fail-closed contract: ambiguous payload facts still selected a byval payload");
  }
  return 0;
}

int check_aarch64_prior_preservation_consumes_prepared_source_selection() {
  auto module = make_prior_preservation_source_selection_contract_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  const auto prepared = prepare_aarch64_module(module);
  const auto* call_plans =
      find_call_plans_function(prepared, "prior_preservation_source_selection_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 2 ||
      call_plans->calls.back().arguments.size() != 1 ||
      !call_plans->calls.back().arguments.front().source_selection.has_value()) {
    return fail(
        "aarch64 prior-preservation source-selection contract: missing prepared source selection");
  }

  const auto& consumer_call = call_plans->calls.back();
  const auto& source_selection = *consumer_call.arguments.front().source_selection;
  if (source_selection.kind !=
      prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation) {
    return fail(
        "aarch64 prior-preservation source-selection contract: argument did not publish prior-preservation selection");
  }

  const auto built = c4c::backend::aarch64::api::build_prepared_module(prepared);
  if (!built.module.has_value() || built.error.has_value()) {
    return fail(
        "aarch64 prior-preservation source-selection contract: prepared module was rejected");
  }

  const auto function_id =
      prepared.names.function_names.find("prior_preservation_source_selection_contract");
  const c4c::backend::aarch64::codegen::CallBoundaryMoveInstructionRecord* lowered_move =
      nullptr;
  for (const auto& function : built.module->mir.functions) {
    if (function.function_name != function_id) {
      continue;
    }
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block.instructions) {
        const auto& target = instruction.target;
        if (target.opcode !=
            c4c::backend::aarch64::codegen::MachineOpcode::CallBoundaryMove) {
          continue;
        }
        const auto* move =
            std::get_if<c4c::backend::aarch64::codegen::CallBoundaryMoveInstructionRecord>(
                &target.payload);
        if (move != nullptr &&
            move->phase == prepare::PreparedMovePhase::BeforeCall &&
            move->instruction_index == consumer_call.instruction_index &&
            move->move.destination_kind ==
                prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
            move->move.from_value_id == source_selection.source_value_id &&
            (move->source_register.has_value() || move->source_memory.has_value())) {
          lowered_move = move;
        }
      }
    }
  }

  if (lowered_move == nullptr) {
    return fail(
        "aarch64 prior-preservation source-selection contract: missing lowered argument move");
  }

  if (source_selection.preservation_route ==
      prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    const auto selected_register =
        source_selection.preserved_register_name.has_value()
            ? c4c::backend::aarch64::abi::parse_aarch64_register_name(
                  *source_selection.preserved_register_name)
            : std::nullopt;
    if (!source_selection.preserved_register_name.has_value() ||
        !source_selection.preserved_register_bank.has_value() ||
        !source_selection.preserved_register_contiguous_width.has_value() ||
        !selected_register.has_value() ||
        !lowered_move->source_register.has_value() ||
        lowered_move->source_register->reg.index != selected_register->index ||
        lowered_move->source_register->prepared_bank !=
            *source_selection.preserved_register_bank ||
        lowered_move->source_register->contiguous_width !=
            *source_selection.preserved_register_contiguous_width ||
        lowered_move->source_register->value_id != source_selection.source_value_id ||
        lowered_move->source_register->value_name != c4c::kInvalidValueName) {
      return fail(
          "aarch64 prior-preservation source-selection contract: lowered register source did not consume prepared selection authority");
    }
    return 0;
  }

  if (source_selection.preservation_route ==
      prepare::PreparedCallPreservationRoute::StackSlot) {
    if (!source_selection.preserved_stack_offset_bytes.has_value() ||
        !source_selection.preserved_stack_size_bytes.has_value() ||
        !source_selection.preserved_stack_align_bytes.has_value() ||
        !lowered_move->source_memory.has_value() ||
        lowered_move->source_memory->frame_slot_id !=
            source_selection.preserved_stack_slot_id ||
        lowered_move->source_memory->byte_offset !=
            static_cast<std::int64_t>(*source_selection.preserved_stack_offset_bytes) ||
        lowered_move->source_memory->size_bytes !=
            *source_selection.preserved_stack_size_bytes ||
        lowered_move->source_memory->align_bytes !=
            *source_selection.preserved_stack_align_bytes ||
        lowered_move->source_memory->result_value_id !=
            source_selection.source_value_id) {
      return fail(
          "aarch64 prior-preservation source-selection contract: lowered stack source did not consume prepared selection authority");
    }
    return 0;
  }

  return fail(
      "aarch64 prior-preservation source-selection contract: unsupported prepared preservation route");
}

int check_aarch64_prior_preservation_rejects_missing_source_selection() {
  auto module = make_prior_preservation_source_selection_contract_module();
  module.target_triple = "aarch64-unknown-linux-gnu";
  auto prepared = prepare_aarch64_module(module);
  auto function_id =
      prepared.names.function_names.find("prior_preservation_source_selection_contract");
  prepare::PreparedCallPlansFunction* mutable_call_plans = nullptr;
  for (auto& candidate : prepared.call_plans.functions) {
    if (candidate.function_name == function_id) {
      mutable_call_plans = &candidate;
      break;
    }
  }
  const auto* control_flow =
      prepare::find_prepared_control_flow_function(prepared.control_flow, function_id);
  if (mutable_call_plans == nullptr || control_flow == nullptr ||
      mutable_call_plans->calls.size() != 2 ||
      mutable_call_plans->calls.back().arguments.size() != 1 ||
      !mutable_call_plans->calls.back().arguments.front().source_selection.has_value()) {
    return fail(
        "aarch64 missing prior-preservation source-selection contract: fixture lost prepared source selection");
  }

  auto& consumer_call = mutable_call_plans->calls.back();
  consumer_call.arguments.front().source_selection.reset();
  if (consumer_call.block_index >= control_flow->blocks.size()) {
    return fail(
        "aarch64 missing prior-preservation source-selection contract: fixture call block is out of range");
  }

  const auto function_context =
      c4c::backend::aarch64::codegen::make_function_lowering_context(
          prepared, prepared.target_profile, *control_flow);
  const auto block_context =
      c4c::backend::aarch64::codegen::make_block_lowering_context(
          function_context,
          control_flow->blocks[consumer_call.block_index],
          consumer_call.block_index);
  c4c::backend::aarch64::module::ModuleLoweringDiagnostics diagnostics;
  const auto lowered = c4c::backend::aarch64::codegen::lower_before_call_moves(
      block_context,
      consumer_call,
      consumer_call.instruction_index,
      diagnostics);
  bool emitted_call_argument_source = false;
  for (const auto& instruction : lowered) {
    const auto* move =
        std::get_if<c4c::backend::aarch64::codegen::CallBoundaryMoveInstructionRecord>(
            &instruction.target.payload);
    if (move != nullptr &&
        move->move.destination_kind ==
            prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
        (move->source_register.has_value() || move->source_memory.has_value())) {
      emitted_call_argument_source = true;
    }
  }
  if (emitted_call_argument_source || diagnostics.empty()) {
    return fail(
        "aarch64 missing prior-preservation source-selection contract: lowering selected a prior source without prepared authority");
  }
  return 0;
}

prepare::PreparedBirModule make_aarch64_outgoing_stack_scalar_lifetime_contract_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = aarch64_target_profile();
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("aarch64_outgoing_stack_scalar_lifetime");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto source_name = prepared.names.value_names.intern("stack.scalar.source");
  constexpr auto source_value_id = prepare::PreparedValueId{9101};
  constexpr auto source_object_id = prepare::PreparedObjectId{9102};
  constexpr auto source_slot_id = prepare::PreparedFrameSlotId{9103};
  constexpr auto assigned_object_id = prepare::PreparedObjectId{9104};
  constexpr auto assigned_slot_id = prepare::PreparedFrameSlotId{9105};

  bir::Function callee;
  callee.name = "take_ten_i64";
  callee.is_declaration = true;
  callee.return_type = bir::TypeKind::Void;
  for (int index = 0; index < 10; ++index) {
    callee.params.push_back(bir::Param{
        .type = bir::TypeKind::I64,
        .name = "arg" + std::to_string(index),
        .size_bytes = 8,
        .align_bytes = 8,
        .abi = bir::CallArgAbiInfo{
            .type = bir::TypeKind::I64,
            .size_bytes = 8,
            .align_bytes = 8,
            .primary_class = bir::AbiValueClass::Integer,
            .passed_in_register = index < 8,
            .passed_on_stack = index >= 8,
        },
    });
  }
  prepared.module.functions.push_back(std::move(callee));

  bir::Function caller;
  caller.name = "aarch64_outgoing_stack_scalar_lifetime";
  caller.return_type = bir::TypeKind::Void;

  bir::CallInst call;
  call.callee = "take_ten_i64";
  call.return_type_name = "void";
  call.return_type = bir::TypeKind::Void;
  for (int index = 0; index < 10; ++index) {
    call.arg_types.push_back(bir::TypeKind::I64);
    call.arg_abi.push_back(bir::CallArgAbiInfo{
        .type = bir::TypeKind::I64,
        .size_bytes = 8,
        .align_bytes = 8,
        .primary_class = bir::AbiValueClass::Integer,
        .passed_in_register = index < 8,
        .passed_on_stack = index >= 8,
    });
    call.args.push_back(index == 8
                            ? bir::Value::named(bir::TypeKind::I64,
                                                "stack.scalar.source")
                            : bir::Value::immediate_i64(index + 1));
  }

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(std::move(call));
  entry.terminator = bir::ReturnTerminator{};
  caller.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(caller));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {prepare::PreparedValueHome{
              .value_id = source_value_id,
              .function_name = function_name,
              .value_name = source_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = assigned_slot_id,
              .offset_bytes = std::size_t{256},
              .size_bytes = std::size_t{8},
              .align_bytes = std::size_t{8},
          }},
      .move_bundles =
          {prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .block_index = 0,
              .instruction_index = 0,
              .moves =
                  {prepare::PreparedMoveResolution{
                      .from_value_id = source_value_id,
                      .to_value_id = source_value_id,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_abi_index = std::size_t{8},
                      .destination_stack_offset_bytes = std::size_t{0},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .reason = "call_arg_stack_to_stack",
                  }},
              .abi_bindings =
                  {prepare::PreparedAbiBinding{
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_abi_index = std::size_t{8},
                      .destination_stack_offset_bytes = std::size_t{0},
                  },
                   prepare::PreparedAbiBinding{
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_abi_index = std::size_t{9},
                      .destination_stack_offset_bytes = std::size_t{8},
                  }},
          }},
  });
  prepared.regalloc.functions.push_back(prepare::PreparedRegallocFunction{
      .function_name = function_name,
      .values =
          {prepare::PreparedRegallocValue{
              .value_id = source_value_id,
              .stack_object_id = assigned_object_id,
              .function_name = function_name,
              .value_name = source_name,
              .type = bir::TypeKind::I64,
              .register_class = prepare::PreparedRegisterClass::General,
              .allocation_status = prepare::PreparedAllocationStatus::Spilled,
              .assigned_stack_slot =
                  prepare::PreparedStackSlotAssignment{
                      .slot_id = assigned_slot_id,
                      .offset_bytes = 256,
                      .size_bytes = std::size_t{8},
                      .align_bytes = std::size_t{8},
                      .placement =
                          prepare::PreparedSpillSlotPlacement{
                              .slot_id = assigned_slot_id,
                              .offset_bytes = 256,
                          },
                  },
          }},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 264,
      .frame_alignment_bytes = 8,
      .accesses =
          {prepare::PreparedMemoryAccess{
              .function_name = function_name,
              .block_label = block_label,
              .inst_index = 0,
              .result_value_name = source_name,
              .address =
                  prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = source_slot_id,
                      .byte_offset = 0,
                      .size_bytes = 8,
                      .align_bytes = 8,
                      .can_use_base_plus_offset = true,
                  },
          }},
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = source_object_id,
      .function_name = function_name,
      .value_name = source_name,
      .source_kind = "spill",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = source_slot_id,
      .object_id = source_object_id,
      .function_name = function_name,
      .offset_bytes = 128,
      .size_bytes = 8,
      .align_bytes = 8,
      .fixed_location = true,
  });
  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = assigned_object_id,
      .function_name = function_name,
      .value_name = source_name,
      .source_kind = "spill",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = assigned_slot_id,
      .object_id = assigned_object_id,
      .function_name = function_name,
      .offset_bytes = 256,
      .size_bytes = 8,
      .align_bytes = 8,
      .fixed_location = true,
  });

  prepare::populate_call_plans(prepared);
  return prepared;
}

int check_aarch64_outgoing_stack_scalar_argument_lifetime_contract() {
  auto prepared = make_aarch64_outgoing_stack_scalar_lifetime_contract_module();
  const auto function_id =
      prepared.names.function_names.find("aarch64_outgoing_stack_scalar_lifetime");
  const auto source_name = prepared.names.value_names.find("stack.scalar.source");
  const auto* control_flow =
      prepare::find_prepared_control_flow_function(prepared.control_flow, function_id);
  const auto* call_plans = find_call_plans_function(
      prepared, "aarch64_outgoing_stack_scalar_lifetime");
  if (function_id == c4c::kInvalidFunctionName ||
      source_name == c4c::kInvalidValueName ||
      control_flow == nullptr || call_plans == nullptr ||
      call_plans->calls.size() != 1 ||
      call_plans->calls.front().arguments.size() != 10) {
    return fail(
        "AArch64 outgoing stack scalar lifetime contract: missing prepared call fixture");
  }

  const auto& call_plan = call_plans->calls.front();
  if (!call_plan.outgoing_stack_argument_area.has_value() ||
      call_plan.outgoing_stack_argument_area->size_bytes != 16) {
    return fail(
        "AArch64 outgoing stack scalar lifetime contract: shared outgoing stack argument area did not cover every complete stack destination");
  }

  const auto& second_stack_argument = call_plan.arguments.back();
  if (second_stack_argument.arg_index != 9 ||
      second_stack_argument.destination_stack_offset_bytes !=
          std::optional<std::size_t>{8} ||
      second_stack_argument.destination_stack_size_bytes !=
          std::optional<std::size_t>{8}) {
    return fail(
        "AArch64 outgoing stack scalar lifetime contract: second stack argument destination disappeared");
  }

  const auto& argument = call_plan.arguments[8];
  if (argument.arg_index != 8 ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      argument.source_value_id !=
          std::optional<prepare::PreparedValueId>{prepare::PreparedValueId{9101}} ||
      argument.source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{9105}} ||
      argument.source_stack_offset_bytes != std::optional<std::size_t>{256} ||
      argument.destination_stack_offset_bytes != std::optional<std::size_t>{0} ||
      !argument.source_selection.has_value() ||
      argument.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue ||
      argument.source_selection->source_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{9103}} ||
      argument.source_selection->source_stack_offset_bytes !=
          std::optional<std::size_t>{128}) {
    return fail(
        "AArch64 outgoing stack scalar lifetime contract: stack argument lost distinct assigned and selected frame-slot authority");
  }

  const auto boundary_effects = prepare::plan_prepared_call_boundary_effects(
      call_plan,
      prepare::find_prepared_move_bundle(
          *prepare::find_prepared_value_location_function(
              prepared, function_id),
          prepare::PreparedMovePhase::BeforeCall,
          call_plan.block_index,
          call_plan.instruction_index),
      nullptr);
  const prepare::PreparedCallBoundaryEffectPlan* stack_arg_effect = nullptr;
  for (const auto& effect : boundary_effects) {
    if (effect.effect_kind == prepare::PreparedCallBoundaryEffectKind::ExplicitMove &&
        effect.phase == prepare::PreparedMovePhase::BeforeCall &&
        effect.destination_kind ==
            prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
        effect.abi_index == std::optional<std::size_t>{8}) {
      stack_arg_effect = &effect;
      break;
    }
  }
  if (stack_arg_effect == nullptr ||
      stack_arg_effect->source.storage_kind !=
          prepare::PreparedMoveStorageKind::StackSlot ||
      stack_arg_effect->source.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{9103}} ||
      stack_arg_effect->source.stack_offset_bytes !=
          std::optional<std::size_t>{128} ||
      stack_arg_effect->source.stack_size_bytes != std::optional<std::size_t>{8} ||
      stack_arg_effect->destination.stack_offset_bytes !=
          std::optional<std::size_t>{0}) {
    return fail(
        "AArch64 outgoing stack scalar lifetime contract: boundary effect did not use the selected prepared frame-slot source");
  }

  const auto function_context =
      aarch64_codegen::make_function_lowering_context(
          prepared, prepared.target_profile, *control_flow);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(
          function_context, control_flow->blocks[call_plan.block_index],
          call_plan.block_index);
  c4c::backend::aarch64::module::ModuleLoweringDiagnostics diagnostics;
  const auto lowered = aarch64_codegen::lower_before_call_moves(
      block_context, call_plan, call_plan.instruction_index, diagnostics);

  const aarch64_codegen::InstructionRecord* stack_store = nullptr;
  const aarch64_codegen::MemoryInstructionRecord* memory_record = nullptr;
  for (const auto& instruction : lowered) {
    const auto* candidate =
        std::get_if<aarch64_codegen::MemoryInstructionRecord>(
            &instruction.target.payload);
    if (candidate == nullptr ||
        candidate->memory_kind != aarch64_codegen::MemoryInstructionKind::Store ||
        candidate->address.stored_value_id !=
            std::optional<prepare::PreparedValueId>{prepare::PreparedValueId{9101}} ||
        !candidate->value.has_value()) {
      continue;
    }
    stack_store = &instruction.target;
    memory_record = candidate;
    break;
  }
  if (stack_store == nullptr || memory_record == nullptr || !diagnostics.empty()) {
    return fail(
        "AArch64 outgoing stack scalar lifetime contract: missing lowered outgoing stack store");
  }
  const auto* source_memory =
      std::get_if<aarch64_codegen::MemoryOperand>(&memory_record->value->payload);
  if (memory_record->address.base_kind != aarch64_codegen::MemoryBaseKind::Register ||
      !memory_record->address.base_register.has_value() ||
      memory_record->address.byte_offset != 0 ||
      memory_record->address.size_bytes != 8 ||
      source_memory == nullptr ||
      source_memory->base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      source_memory->frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{prepare::PreparedFrameSlotId{9103}} ||
      source_memory->result_value_id !=
          std::optional<prepare::PreparedValueId>{prepare::PreparedValueId{9101}}) {
    return fail(
        "AArch64 outgoing stack scalar lifetime contract: lowered store did not keep source and destination operands distinct");
  }

  const auto has_prepared_frame_slot_source_use =
      std::any_of(stack_store->uses.begin(), stack_store->uses.end(),
                  [](const aarch64_codegen::MachineEffectResource& use) {
                    return use.kind ==
                               aarch64_codegen::MachineEffectResourceKind::Memory &&
                           use.frame_slot_id ==
                               std::optional<prepare::PreparedFrameSlotId>{
                                   prepare::PreparedFrameSlotId{9103}} &&
                           use.value_id ==
                               std::optional<prepare::PreparedValueId>{
                                   prepare::PreparedValueId{9101}};
                  });
  if (stack_store->defs.size() != 1 ||
      stack_store->defs.front().kind !=
          aarch64_codegen::MachineEffectResourceKind::Memory ||
      stack_store->defs.front().value_id !=
          std::optional<prepare::PreparedValueId>{prepare::PreparedValueId{9101}} ||
      stack_store->defs.front().value_name != source_name ||
      !has_prepared_frame_slot_source_use) {
    return fail(
        "AArch64 outgoing stack scalar lifetime contract: outgoing stack slot must be the def and prepared frame slot must stay a use");
  }
  return 0;
}

int check_saved_register_slot_placement_carrier_contract() {
  const prepare::PreparedRegisterPlacement register_placement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
      .slot_index = 1,
      .contiguous_width = 1,
  };
  const prepare::PreparedSavedRegisterSlotPlacement complete{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_name = "s1",
      .contiguous_width = 1,
      .occupied_register_names = {"s1"},
      .save_index = 3,
      .register_placement = register_placement,
      .slot_id = prepare::PreparedFrameSlotId{42},
      .stack_offset_bytes = std::size_t{24},
      .size_bytes = std::size_t{8},
      .align_bytes = std::size_t{8},
      .fixed_location = true,
  };
  const prepare::PreparedSavedRegister saved{
      .bank = complete.bank,
      .register_name = complete.register_name,
      .contiguous_width = complete.contiguous_width,
      .occupied_register_names = complete.occupied_register_names,
      .save_index = complete.save_index,
      .placement = complete.register_placement,
      .slot_placement = complete,
  };
  if (!prepare::has_complete_prepared_saved_register_slot_placement(complete) ||
      !saved.slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(*saved.slot_placement) ||
      saved.slot_placement->bank != saved.bank ||
      saved.slot_placement->register_name != saved.register_name ||
      saved.slot_placement->contiguous_width != saved.contiguous_width ||
      saved.slot_placement->occupied_register_names != saved.occupied_register_names ||
      saved.slot_placement->save_index != saved.save_index ||
      saved.slot_placement->register_placement != saved.placement ||
      saved.slot_placement->slot_id != std::optional<prepare::PreparedFrameSlotId>{42} ||
      saved.slot_placement->stack_offset_bytes != std::optional<std::size_t>{24} ||
      saved.slot_placement->size_bytes != std::optional<std::size_t>{8} ||
      saved.slot_placement->align_bytes != std::optional<std::size_t>{8} ||
      !saved.slot_placement->fixed_location) {
    return fail("saved-register slot-placement carrier lost structured register or frame-slot facts");
  }

  auto incomplete = complete;
  incomplete.stack_offset_bytes.reset();
  if (prepare::has_complete_prepared_saved_register_slot_placement(incomplete)) {
    return fail("saved-register slot-placement carrier should fail closed without stack offset");
  }
  incomplete = complete;
  incomplete.register_placement.reset();
  if (prepare::has_complete_prepared_saved_register_slot_placement(incomplete)) {
    return fail("saved-register slot-placement carrier should fail closed without register placement");
  }
  return 0;
}

int check_stack_cross_call_preservation_contract() {
  const auto prepared = prepare_module(make_stack_cross_call_preservation_contract_module());
  const auto function_id =
      prepared.names.function_names.find("stack_cross_call_preservation_contract");
  const auto* call_plans =
      find_call_plans_function(prepared, "stack_cross_call_preservation_contract");
  const auto* frame_plan = function_id == c4c::kInvalidFunctionName
                               ? nullptr
                               : prepare::find_prepared_frame_plan(prepared, function_id);
  const auto* storage_plan =
      find_storage_plan_function(prepared, "stack_cross_call_preservation_contract");
  if (call_plans == nullptr || call_plans->calls.size() != 1) {
    return fail("stack cross-call preservation contract: missing call-plan publication");
  }

  const auto& call_plan = call_plans->calls.front();
  const auto preserved_it = std::find_if(call_plan.preserved_values.begin(),
                                         call_plan.preserved_values.end(),
                                         [](const auto& value) {
                                           return value.route ==
                                                  prepare::PreparedCallPreservationRoute::StackSlot;
                                         });
  const auto* preserved =
      preserved_it == call_plan.preserved_values.end() ? nullptr : &*preserved_it;
  if (preserved == nullptr) {
    return fail("stack cross-call preservation contract: expected at least one stack-slot preserved scalar");
  }

  const auto* storage_value =
      storage_plan == nullptr
          ? nullptr
          : find_storage_value(
                prepared, *storage_plan, prepare::prepared_value_name(prepared.names, preserved->value_name));
  if (storage_value == nullptr ||
      storage_value->encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      preserved->slot_id != storage_value->slot_id ||
      preserved->stack_offset_bytes != storage_value->stack_offset_bytes ||
      preserved->register_name.has_value() || preserved->register_bank.has_value() ||
      preserved->callee_saved_save_index.has_value()) {
    return fail("stack cross-call preservation contract: call_plans lost direct frame-slot authority");
  }
  if (preserved->preservation_source.value_id !=
          std::optional<prepare::PreparedValueId>{preserved->value_id} ||
      preserved->preservation_source.value_name != preserved->value_name ||
      preserved->preservation_destination.storage_kind !=
          prepare::PreparedMoveStorageKind::StackSlot ||
      preserved->preservation_destination.slot_id != preserved->slot_id ||
      preserved->preservation_destination.stack_offset_bytes !=
          preserved->stack_offset_bytes ||
      preserved->preservation_destination.stack_size_bytes !=
          preserved->stack_size_bytes ||
      preserved->preservation_destination.stack_align_bytes !=
          preserved->stack_align_bytes ||
      preserved->preservation_reason.empty()) {
    return fail("stack cross-call preservation contract: missing explicit preservation source/destination facts");
  }
  if (frame_plan == nullptr || !preserved->slot_id.has_value() ||
      !preserved->stack_offset_bytes.has_value() ||
      !preserved->stack_size_bytes.has_value() ||
      frame_plan->frame_size_bytes <
          *preserved->stack_offset_bytes + *preserved->stack_size_bytes ||
      std::find(frame_plan->frame_slot_order.begin(),
                frame_plan->frame_slot_order.end(),
                *preserved->slot_id) == frame_plan->frame_slot_order.end()) {
    return fail("stack cross-call preservation contract: frame plan must cover regalloc stack homes consumed by call preservation");
  }
  const std::string prepared_dump = prepare::print(prepared);
  const std::string preserved_summary =
      std::string(prepare::prepared_value_name(prepared.names, preserved->value_name)) +
      "#" + std::to_string(preserved->value_id) + ":stack_slot:slot#" +
      std::to_string(*preserved->slot_id) + ":stack+" +
      std::to_string(*preserved->stack_offset_bytes);
  if (prepared_dump.find(preserved_summary) == std::string::npos) {
    return fail("stack cross-call preservation contract: prepared summary no longer publishes frame-slot identity");
  }

  return 0;
}

int check_grouped_cross_call_preservation_contract() {
  const auto prepared = prepare_grouped_riscv_module_with_overrides(
      make_grouped_cross_call_preservation_contract_module(),
      {{"p.vcarry", 2}, {"carry.pre", 2}, {"post.local", 1}});
  const auto function_id =
      prepared.names.function_names.find("grouped_cross_call_preservation_contract");
  const auto* call_plans =
      find_call_plans_function(prepared, "grouped_cross_call_preservation_contract");
  const auto* frame_plan = function_id == c4c::kInvalidFunctionName
                               ? nullptr
                               : prepare::find_prepared_frame_plan(prepared, function_id);
  const auto* storage_plan =
      find_storage_plan_function(prepared, "grouped_cross_call_preservation_contract");
  const auto* param_carry =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "p.vcarry");
  const auto* carry =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "carry.pre");
  if (call_plans == nullptr || call_plans->calls.size() != 1 || frame_plan == nullptr ||
      storage_plan == nullptr || param_carry == nullptr || carry == nullptr) {
    return fail("grouped cross-call preservation contract: missing grouped publication surfaces");
  }

  if (param_carry->bank != prepare::PreparedRegisterBank::Gpr ||
      param_carry->contiguous_width != 1 ||
      !param_carry->register_name.has_value() ||
      param_carry->occupied_register_names != std::vector<std::string>{*param_carry->register_name}) {
    return fail(
        "grouped cross-call preservation contract: storage plan published grouped span metadata onto the ABI-home parameter register");
  }

  const auto& call_plan = call_plans->calls.front();
  const auto preserved_it = std::find_if(
      call_plan.preserved_values.begin(),
      call_plan.preserved_values.end(),
      [carry](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == carry->value_id;
      });
  if (preserved_it == call_plan.preserved_values.end()) {
    return fail("grouped cross-call preservation contract: missing grouped preserved value");
  }
  const auto& preserved = *preserved_it;
  const auto saved_it = std::find_if(
      frame_plan->saved_callee_registers.begin(),
      frame_plan->saved_callee_registers.end(),
      [&](const prepare::PreparedSavedRegister& saved) {
        return saved.bank == prepare::PreparedRegisterBank::Vreg &&
               saved.occupied_register_names == preserved.occupied_register_names;
      });
  if (saved_it == frame_plan->saved_callee_registers.end()) {
    return fail("grouped cross-call preservation contract: missing grouped callee-saved span authority");
  }
  if (preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved.register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Vreg} ||
      preserved.contiguous_width != 2 ||
      preserved.occupied_register_names.size() != 2 ||
      preserved.register_name != std::optional<std::string>{saved_it->register_name} ||
      preserved.callee_saved_save_index !=
          std::optional<std::size_t>{saved_it->save_index}) {
    return fail("grouped cross-call preservation contract: call_plans lost grouped bank/span authority");
  }
  if (saved_it->contiguous_width != 2 || saved_it->occupied_register_names.size() != 2) {
    return fail("grouped cross-call preservation contract: frame plan lost grouped saved-register span");
  }
  if (!saved_it->slot_placement.has_value() ||
      !prepare::has_complete_prepared_saved_register_slot_placement(*saved_it->slot_placement) ||
      saved_it->slot_placement->bank != saved_it->bank ||
      saved_it->slot_placement->register_name != saved_it->register_name ||
      saved_it->slot_placement->contiguous_width != 2 ||
      saved_it->slot_placement->occupied_register_names != saved_it->occupied_register_names ||
      saved_it->slot_placement->size_bytes != std::optional<std::size_t>{32} ||
      saved_it->slot_placement->align_bytes != std::optional<std::size_t>{16} ||
      !saved_it->slot_placement->fixed_location) {
    return fail("grouped cross-call preservation contract: grouped saved-register placement lost prepared slot facts");
  }
  if (!saved_it->slot_placement->slot_id.has_value() ||
      !saved_it->slot_placement->stack_offset_bytes.has_value() ||
      *saved_it->slot_placement->stack_offset_bytes < frame_plan->frame_size_bytes ||
      std::find(frame_plan->frame_slot_order.begin(),
                frame_plan->frame_slot_order.end(),
                *saved_it->slot_placement->slot_id) != frame_plan->frame_slot_order.end()) {
    return fail("grouped cross-call preservation contract: grouped saved-register placement perturbed frame-slot order");
  }
  if (carry->bank != prepare::PreparedRegisterBank::Vreg ||
      carry->contiguous_width != 2 ||
      carry->register_name != preserved.register_name ||
      carry->occupied_register_names != preserved.occupied_register_names) {
    return fail("grouped cross-call preservation contract: storage plan lost grouped register home");
  }
  const auto grouped_clobber_it = std::find_if(
      call_plan.clobbered_registers.begin(),
      call_plan.clobbered_registers.end(),
      [](const prepare::PreparedClobberedRegister& clobber) {
        return clobber.bank == prepare::PreparedRegisterBank::Vreg &&
               clobber.contiguous_width == 2 &&
               clobber.occupied_register_names.size() == 2;
      });
  if (grouped_clobber_it == call_plan.clobbered_registers.end()) {
    return fail("grouped cross-call preservation contract: call_plans lost grouped caller-clobber spans");
  }

  return 0;
}

int check_dynamic_stack_contract() {
  const auto prepared = prepare_module(make_dynamic_stack_module());
  const auto* function = find_function(prepared.module, "dynamic_stack_contract");
  if (function == nullptr || function->blocks.empty() || function->blocks.front().insts.size() < 3) {
    return fail("dynamic-stack contract: missing dynamic-stack call sequence");
  }

  const auto* stack_save = std::get_if<bir::CallInst>(&function->blocks.front().insts[0]);
  const auto* dynamic_alloca = std::get_if<bir::CallInst>(&function->blocks.front().insts[1]);
  const auto* stack_restore = std::get_if<bir::CallInst>(&function->blocks.front().insts[2]);
  if (stack_save == nullptr || dynamic_alloca == nullptr || stack_restore == nullptr) {
    return fail("dynamic-stack contract: expected call-shaped stack operations");
  }
  if (stack_save->callee != "llvm.stacksave" ||
      dynamic_alloca->callee != "llvm.dynamic_alloca.i32" ||
      stack_restore->callee != "llvm.stackrestore") {
    return fail("dynamic-stack contract: canonical stack-save/alloca/restore calls drifted");
  }
  if (!stack_restore->arg_abi.empty() &&
      (stack_restore->arg_abi.front().type != bir::TypeKind::Ptr ||
       stack_restore->arg_abi.front().primary_class != bir::AbiValueClass::Integer ||
       !stack_restore->arg_abi.front().passed_in_register)) {
    return fail("dynamic-stack contract: stackrestore pointer ABI metadata drifted");
  }
  if (find_stack_object(prepared, "vla.buf") != nullptr) {
    return fail("dynamic-stack contract: dynamic alloca unexpectedly became a fixed stack object");
  }
  const auto* liveness = find_liveness_function(prepared, "dynamic_stack_contract");
  if (liveness == nullptr || liveness->call_points.size() != 3) {
    return fail("dynamic-stack contract: prepared liveness no longer records stack-state calls");
  }
  const auto function_id = prepared.names.function_names.find("dynamic_stack_contract");
  const auto* frame_plan = prepare::find_prepared_frame_plan(prepared, function_id);
  const auto* dynamic_plan = prepare::find_prepared_dynamic_stack_plan(prepared, function_id);
  if (frame_plan == nullptr || !frame_plan->has_dynamic_stack ||
      frame_plan->uses_frame_pointer_for_fixed_slots) {
    return fail(
        "dynamic-stack contract: frame_plan no longer marks dynamic stack correctly for a no-fixed-slot function");
  }
  if (dynamic_plan == nullptr || !dynamic_plan->requires_stack_save_restore ||
      dynamic_plan->uses_frame_pointer_for_fixed_slots ||
      dynamic_plan->operations.size() != 3) {
    return fail("dynamic-stack contract: dynamic_stack_plan no longer publishes the stack-state operations");
  }
  const auto* call_plans = find_call_plans_function(prepared, "dynamic_stack_contract");
  const auto* storage_plan = find_storage_plan_function(prepared, "dynamic_stack_contract");
  const auto* saved_sp =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "saved.sp");
  const auto* vla_buf =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "vla.buf");
  if (call_plans == nullptr || call_plans->calls.size() != 3) {
    return fail("dynamic-stack contract: call_plans lost the stack-state calls");
  }
  if (storage_plan == nullptr || saved_sp == nullptr || vla_buf == nullptr ||
      saved_sp->bank != prepare::PreparedRegisterBank::Gpr ||
      vla_buf->bank != prepare::PreparedRegisterBank::Gpr) {
    return fail("dynamic-stack contract: storage_plans lost dynamic-stack pointer homes");
  }
  return 0;
}

int check_dynamic_stack_plan_reads_authoritative_block_label_id() {
  const auto prepared = prepare_module(make_dynamic_stack_stale_raw_block_label_module());
  const auto function_id =
      prepared.names.function_names.find("dynamic_stack_stale_raw_block_label_contract");
  const auto* dynamic_plan = prepare::find_prepared_dynamic_stack_plan(prepared, function_id);
  const auto expected_block = prepared.names.block_labels.find("entry.authoritative");
  if (function_id == c4c::kInvalidFunctionName || dynamic_plan == nullptr ||
      dynamic_plan->operations.size() != 2 || expected_block == c4c::kInvalidBlockLabel) {
    return fail(
        "dynamic-stack authoritative block-label contract: fixture failed to publish dynamic-stack operations");
  }
  for (const auto& op : dynamic_plan->operations) {
    if (op.block_label != expected_block) {
      return fail(
          "dynamic-stack authoritative block-label contract: dynamic_stack_plan used stale raw block spelling instead of BlockLabelId authority");
    }
  }
  return 0;
}

int check_cross_block_dynamic_stack_cfg_contract() {
  const auto prepared = prepare_module(make_cross_block_dynamic_stack_module());
  const auto* function = find_function(prepared.module, "cross_block_dynamic_stack_contract");
  if (function == nullptr || function->blocks.size() != 4) {
    return fail("cross-block dynamic-stack contract: expected four semantic BIR blocks");
  }

  const auto* control_flow =
      find_control_flow_function(prepared, "cross_block_dynamic_stack_contract");
  if (control_flow == nullptr || control_flow->blocks.size() != 4) {
    return fail("cross-block dynamic-stack contract: prepared control-flow lost the loop topology");
  }

  const auto* entry = find_control_flow_block(prepared, *control_flow, "entry");
  const auto* loop = find_control_flow_block(prepared, *control_flow, "loop");
  const auto* restore = find_control_flow_block(prepared, *control_flow, "restore_then_loop");
  const auto* exit = find_control_flow_block(prepared, *control_flow, "exit");
  if (entry == nullptr || loop == nullptr || restore == nullptr || exit == nullptr) {
    return fail("cross-block dynamic-stack contract: missing prepared CFG blocks");
  }
  if (entry->terminator_kind != bir::TerminatorKind::Branch ||
      loop->terminator_kind != bir::TerminatorKind::CondBranch ||
      restore->terminator_kind != bir::TerminatorKind::Branch ||
      exit->terminator_kind != bir::TerminatorKind::Return) {
    return fail("cross-block dynamic-stack contract: prepared CFG terminators drifted");
  }

  const auto* liveness = find_liveness_function(prepared, "cross_block_dynamic_stack_contract");
  if (liveness == nullptr || liveness->call_points.size() != 3) {
    return fail(
        "cross-block dynamic-stack contract: expected stacksave, dynamic_alloca, and stackrestore call points");
  }
  const auto* xs_slot = find_stack_object(prepared, "lv.xs");
  if (xs_slot == nullptr) {
    return fail("cross-block dynamic-stack contract: fixed pointer slot for dynamic allocation handle disappeared");
  }
  if (find_stack_object(prepared, "loop.buf") != nullptr) {
    return fail("cross-block dynamic-stack contract: loop dynamic alloca became a fixed stack object");
  }
  const auto function_id = prepared.names.function_names.find("cross_block_dynamic_stack_contract");
  const auto* frame_plan = prepare::find_prepared_frame_plan(prepared, function_id);
  const auto* dynamic_plan = prepare::find_prepared_dynamic_stack_plan(prepared, function_id);
  if (frame_plan == nullptr || !frame_plan->has_dynamic_stack ||
      !frame_plan->uses_frame_pointer_for_fixed_slots) {
    return fail("cross-block dynamic-stack contract: frame_plan lost cross-block dynamic-stack ownership");
  }
  if (dynamic_plan == nullptr || !dynamic_plan->requires_stack_save_restore ||
      dynamic_plan->operations.size() != 3) {
    return fail("cross-block dynamic-stack contract: dynamic_stack_plan lost cross-block stack-state operations");
  }
  return 0;
}

int check_nested_dynamic_stack_contract() {
  const auto prepared = prepare_module(make_nested_dynamic_stack_module());
  const auto* function = find_function(prepared.module, "nested_dynamic_stack_contract");
  if (function == nullptr || function->blocks.size() != 4) {
    return fail("nested dynamic-stack contract: expected four semantic BIR blocks");
  }
  if (count_calls_to(*function, "llvm.stacksave") != 2 ||
      count_calls_to(*function, "llvm.dynamic_alloca.i32") != 2 ||
      count_calls_to(*function, "llvm.stackrestore") != 2) {
    return fail(
        "nested dynamic-stack contract: expected two nested stacksave/alloca/restore operations");
  }

  const auto* control_flow = find_control_flow_function(prepared, "nested_dynamic_stack_contract");
  if (control_flow == nullptr || control_flow->blocks.size() != 4) {
    return fail("nested dynamic-stack contract: prepared CFG lost nested stack topology");
  }
  const auto* inner = find_control_flow_block(prepared, *control_flow, "inner");
  const auto* restore_inner = find_control_flow_block(prepared, *control_flow, "restore_inner");
  const auto* restore_outer = find_control_flow_block(prepared, *control_flow, "restore_outer");
  if (inner == nullptr || restore_inner == nullptr || restore_outer == nullptr) {
    return fail("nested dynamic-stack contract: missing nested stack CFG blocks");
  }
  if (inner->terminator_kind != bir::TerminatorKind::CondBranch ||
      restore_inner->terminator_kind != bir::TerminatorKind::Branch ||
      restore_outer->terminator_kind != bir::TerminatorKind::Return) {
    return fail("nested dynamic-stack contract: nested restore CFG terminators drifted");
  }
  if (find_stack_object(prepared, "lv.fixed") == nullptr) {
    return fail("nested dynamic-stack contract: fixed local disappeared while dynamic stack was active");
  }
  if (find_stack_object(prepared, "outer.buf") != nullptr ||
      find_stack_object(prepared, "inner.buf") != nullptr) {
    return fail("nested dynamic-stack contract: nested dynamic allocas became fixed stack objects");
  }
  const auto* liveness = find_liveness_function(prepared, "nested_dynamic_stack_contract");
  if (liveness == nullptr || liveness->call_points.size() != 6) {
    return fail(
        "nested dynamic-stack contract: expected two stacksaves, two dynamic allocas, and two stackrestores as call points");
  }
  const auto function_id = prepared.names.function_names.find("nested_dynamic_stack_contract");
  const auto* frame_plan = prepare::find_prepared_frame_plan(prepared, function_id);
  const auto* dynamic_plan = prepare::find_prepared_dynamic_stack_plan(prepared, function_id);
  if (frame_plan == nullptr || !frame_plan->has_dynamic_stack ||
      !frame_plan->uses_frame_pointer_for_fixed_slots) {
    return fail("nested dynamic-stack contract: frame_plan lost nested dynamic-stack anchoring");
  }
  if (dynamic_plan == nullptr || !dynamic_plan->requires_stack_save_restore ||
      dynamic_plan->operations.size() != 6) {
    return fail("nested dynamic-stack contract: dynamic_stack_plan lost nested stack-state operations");
  }
  return 0;
}

int check_branch_restore_dynamic_stack_contract() {
  const auto prepared = prepare_module(make_branch_restore_dynamic_stack_module());
  const auto* function = find_function(prepared.module, "branch_restore_dynamic_stack_contract");
  if (function == nullptr || function->blocks.size() != 3) {
    return fail("branch-restore dynamic-stack contract: expected three semantic BIR blocks");
  }

  const auto& restore_true = function->blocks[1];
  const auto& restore_false = function->blocks[2];
  if (count_block_calls_to(restore_true, "llvm.stackrestore") != 1 ||
      count_block_calls_to(restore_false, "llvm.stackrestore") != 1) {
    return fail("branch-restore dynamic-stack contract: each restore branch must restore exactly once");
  }
  const auto* true_restore_call = std::get_if<bir::CallInst>(&restore_true.insts.front());
  const auto* false_restore_call = std::get_if<bir::CallInst>(&restore_false.insts.front());
  if (true_restore_call == nullptr || false_restore_call == nullptr ||
      true_restore_call->args.size() != 1 || false_restore_call->args.size() != 1 ||
      true_restore_call->args.front().name != "saved.sp" ||
      false_restore_call->args.front().name != "saved.sp") {
    return fail("branch-restore dynamic-stack contract: restore branches no longer pair to the published saved stack pointer");
  }

  const auto* control_flow =
      find_control_flow_function(prepared, "branch_restore_dynamic_stack_contract");
  if (control_flow == nullptr || control_flow->blocks.size() != 3) {
    return fail("branch-restore dynamic-stack contract: prepared CFG lost branch restore topology");
  }
  const auto* entry = find_control_flow_block(prepared, *control_flow, "entry");
  const auto* true_block = find_control_flow_block(prepared, *control_flow, "restore_true");
  const auto* false_block = find_control_flow_block(prepared, *control_flow, "restore_false");
  if (entry == nullptr || true_block == nullptr || false_block == nullptr) {
    return fail("branch-restore dynamic-stack contract: missing prepared CFG restore blocks");
  }
  if (entry->terminator_kind != bir::TerminatorKind::CondBranch ||
      true_block->terminator_kind != bir::TerminatorKind::Return ||
      false_block->terminator_kind != bir::TerminatorKind::Return) {
    return fail("branch-restore dynamic-stack contract: prepared CFG restore terminators drifted");
  }
  const auto* liveness = find_liveness_function(prepared, "branch_restore_dynamic_stack_contract");
  if (liveness == nullptr || liveness->call_points.size() != 4) {
    return fail(
        "branch-restore dynamic-stack contract: expected stacksave, dynamic_alloca, and one restore per branch as call points");
  }
  const auto function_id = prepared.names.function_names.find("branch_restore_dynamic_stack_contract");
  const auto* frame_plan = prepare::find_prepared_frame_plan(prepared, function_id);
  const auto* dynamic_plan = prepare::find_prepared_dynamic_stack_plan(prepared, function_id);
  if (frame_plan == nullptr || !frame_plan->has_dynamic_stack) {
    return fail("branch-restore dynamic-stack contract: frame_plan lost branch restore dynamic-stack ownership");
  }
  if (dynamic_plan == nullptr || !dynamic_plan->requires_stack_save_restore ||
      dynamic_plan->operations.size() != 4) {
    return fail("branch-restore dynamic-stack contract: dynamic_stack_plan lost per-branch restore operations");
  }
  return 0;
}

int check_x86_consumer_surface_reads_scalar_call_boundary_authority() {
  const auto prepared = prepare_module(make_call_contract_module());
  const auto function_id = prepare::resolve_prepared_function_name_id(prepared.names, "call_contract");
  if (!function_id.has_value()) {
    return fail("x86 consumer surface contract: failed to resolve call_contract");
  }

  const auto consumed = c4c::backend::x86::consume_plans(prepared, "call_contract");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != prepare::find_prepared_call_plans(prepared, *function_id) ||
      consumed.storage != prepare::find_prepared_storage_plan(prepared, *function_id)) {
    return fail(
        "x86 consumer surface contract: x86 no longer reads scalar call-boundary plans directly from prepared authority");
  }
  if (consumed.calls == nullptr || consumed.calls->calls.size() != 1 || consumed.storage == nullptr) {
    return fail("x86 consumer surface contract: missing call or storage plan for scalar call fixture");
  }
  if (consumed.dynamic_stack != nullptr) {
    return fail("x86 consumer surface contract: plain scalar call fixture unexpectedly reports dynamic-stack authority");
  }
  const auto* tmp_call = find_storage_value(prepared, *consumed.storage, "tmp.call");
  if (tmp_call == nullptr || consumed.calls->calls.front().result->destination_value_id !=
                                 std::optional<prepare::PreparedValueId>{tmp_call->value_id}) {
    return fail("x86 consumer surface contract: x86-consumed scalar call plans lost direct result/storage identity");
  }
  return 0;
}

int check_x86_consumer_surface_reads_memory_return_authority() {
  const auto prepared = prepare_module(make_memory_return_call_contract_module());
  const auto function_id =
      prepare::resolve_prepared_function_name_id(prepared.names, "memory_return_call_contract");
  if (!function_id.has_value()) {
    return fail("x86 consumer surface contract: failed to resolve memory_return_call_contract");
  }

  const auto consumed =
      c4c::backend::x86::consume_plans(prepared, "memory_return_call_contract");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != prepare::find_prepared_call_plans(prepared, *function_id) ||
      consumed.storage != prepare::find_prepared_storage_plan(prepared, *function_id)) {
    return fail(
        "x86 consumer surface contract: x86 no longer reads memory-return call authority directly from prepared plans");
  }

  const auto* sret_storage =
      consumed.storage == nullptr ? nullptr : find_storage_value(prepared, *consumed.storage, "lv.call.sret.storage");
  const auto* call = c4c::backend::x86::find_consumed_call_plan(consumed, 0, 0);
  const auto* scalar_arg =
      c4c::backend::x86::find_consumed_call_argument_plan(consumed, 0, 0, 1);
  const auto* storage_object = find_stack_object(prepared, "lv.call.sret.storage");
  const auto* frame_slot =
      storage_object == nullptr ? nullptr : find_frame_slot(prepared, storage_object->object_id);
  if (consumed.dynamic_stack != nullptr || consumed.calls == nullptr || consumed.storage == nullptr ||
      sret_storage == nullptr || call == nullptr || scalar_arg == nullptr ||
      storage_object == nullptr || frame_slot == nullptr) {
    return fail(
        "x86 consumer surface contract: memory-return fixture no longer exposes direct call/storage authority through x86");
  }

  if (call->wrapper_kind != prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      call->direct_callee_name != std::optional<std::string>{"extern_make_pair"} ||
      call->result.has_value() || !call->memory_return.has_value() ||
      scalar_arg != &call->arguments[1]) {
    return fail(
        "x86 consumer surface contract: memory-return fixture lost direct fixed-arity call and memory-return shape");
  }
  if (scalar_arg->source_encoding != prepare::PreparedStorageEncodingKind::Immediate ||
      !scalar_arg->source_literal.has_value() ||
      scalar_arg->source_literal->immediate != 13) {
    return fail(
        "x86 consumer surface contract: memory-return fixture lost the scalar argument source consumed by x86");
  }

  const auto& memory_return = *call->memory_return;
  if (prepare::prepared_slot_name(prepared.names, memory_return.storage_slot_name) !=
          "lv.call.sret.storage" ||
      memory_return.sret_arg_index != std::optional<std::size_t>{0} ||
      memory_return.slot_id != std::optional<prepare::PreparedFrameSlotId>{frame_slot->slot_id} ||
      memory_return.stack_offset_bytes != std::optional<std::size_t>{frame_slot->offset_bytes}) {
    return fail(
        "x86 consumer surface contract: memory-return fixture lost the shared frame-slot-backed sret storage seam");
  }
  return 0;
}

int check_x86_consumer_surface_reads_nested_dynamic_stack_call_boundary_authority() {
  const auto prepared = prepare_module(make_variadic_nested_dynamic_stack_call_module());
  const auto function_id = prepare::resolve_prepared_function_name_id(
      prepared.names, "variadic_nested_dynamic_stack_contract");
  if (!function_id.has_value()) {
    return fail("x86 consumer surface contract: failed to resolve variadic_nested_dynamic_stack_contract");
  }

  const auto consumed =
      c4c::backend::x86::consume_plans(prepared, "variadic_nested_dynamic_stack_contract");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != prepare::find_prepared_call_plans(prepared, *function_id) ||
      consumed.storage != prepare::find_prepared_storage_plan(prepared, *function_id)) {
    return fail(
        "x86 consumer surface contract: x86 no longer reads nested dynamic-stack call-boundary plans directly from prepared authority");
  }
  const auto* carry = consumed.storage == nullptr ? nullptr : find_storage_value(prepared, *consumed.storage, "carry");
  if (consumed.frame == nullptr || !consumed.frame->has_dynamic_stack || consumed.dynamic_stack == nullptr ||
      !consumed.dynamic_stack->requires_stack_save_restore || consumed.dynamic_stack->operations.size() != 6 ||
      consumed.calls == nullptr || consumed.calls->calls.size() != 7 || consumed.storage == nullptr ||
      carry == nullptr) {
    return fail(
        "x86 consumer surface contract: nested dynamic-stack scalar fixture no longer exposes frame/dynamic-stack/call/storage authority through x86");
  }
  const auto variadic_it = std::find_if(
      consumed.calls->calls.begin(),
      consumed.calls->calls.end(),
      [](const prepare::PreparedCallPlan& call_plan) {
        return call_plan.direct_callee_name == std::optional<std::string>{"extern_variadic_i32"} &&
               call_plan.wrapper_kind == prepare::PreparedCallWrapperKind::DirectExternVariadic;
      });
  if (variadic_it == consumed.calls->calls.end() ||
      variadic_it->arguments.front().source_value_id !=
          std::optional<prepare::PreparedValueId>{carry->value_id}) {
    return fail(
        "x86 consumer surface contract: nested dynamic-stack variadic call lost direct carry/storage authority");
  }
  return 0;
}

int check_x86_consumer_surface_reads_grouped_call_boundary_authority() {
  const auto prepared = prepare_grouped_riscv_module_with_overrides(
      make_grouped_cross_call_preservation_contract_module(),
      {{"p.vcarry", 2}, {"carry.pre", 2}, {"post.local", 1}});
  const auto function_id = prepare::resolve_prepared_function_name_id(
      prepared.names, "grouped_cross_call_preservation_contract");
  if (!function_id.has_value()) {
    return fail("x86 consumer surface contract: failed to resolve grouped_cross_call_preservation_contract");
  }

  const auto consumed =
      c4c::backend::x86::consume_plans(prepared, "grouped_cross_call_preservation_contract");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != prepare::find_prepared_call_plans(prepared, *function_id) ||
      consumed.storage != prepare::find_prepared_storage_plan(prepared, *function_id)) {
    return fail(
        "x86 consumer surface contract: x86 no longer reads grouped call-boundary plans directly from prepared authority");
  }
  const auto* carry =
      consumed.storage == nullptr ? nullptr : find_storage_value(prepared, *consumed.storage, "carry.pre");
  if (consumed.frame == nullptr || consumed.dynamic_stack != nullptr || consumed.calls == nullptr ||
      consumed.calls->calls.size() != 1 || consumed.storage == nullptr || carry == nullptr) {
    return fail(
        "x86 consumer surface contract: grouped fixture no longer exposes frame/call/storage authority through x86");
  }

  const auto& call_plan = consumed.calls->calls.front();
  const auto preserved_it = std::find_if(
      call_plan.preserved_values.begin(),
      call_plan.preserved_values.end(),
      [carry](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == carry->value_id;
      });
  if (preserved_it == call_plan.preserved_values.end()) {
    return fail("x86 consumer surface contract: grouped fixture lost direct preserved-value identity");
  }
  const auto saved_it = std::find_if(
      consumed.frame->saved_callee_registers.begin(),
      consumed.frame->saved_callee_registers.end(),
      [preserved_it](const prepare::PreparedSavedRegister& saved) {
        return saved.bank == prepare::PreparedRegisterBank::Vreg &&
               saved.occupied_register_names == preserved_it->occupied_register_names;
      });
  if (saved_it == consumed.frame->saved_callee_registers.end()) {
    return fail("x86 consumer surface contract: grouped fixture lost direct callee-saved span authority");
  }
  if (carry->bank != prepare::PreparedRegisterBank::Vreg || carry->contiguous_width != 2 ||
      preserved_it->register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Vreg} ||
      preserved_it->contiguous_width != 2 ||
      carry->register_name != preserved_it->register_name ||
      carry->occupied_register_names != preserved_it->occupied_register_names ||
      preserved_it->register_name != std::optional<std::string>{saved_it->register_name} ||
      preserved_it->callee_saved_save_index !=
          std::optional<std::size_t>{saved_it->save_index}) {
    return fail(
        "x86 consumer surface contract: grouped storage/call/frame plans lost direct shared bank-span authority");
  }
  const auto grouped_clobber_it = std::find_if(
      call_plan.clobbered_registers.begin(),
      call_plan.clobbered_registers.end(),
      [](const prepare::PreparedClobberedRegister& clobber) {
        return clobber.bank == prepare::PreparedRegisterBank::Vreg &&
               clobber.contiguous_width == 2 &&
               clobber.occupied_register_names.size() == 2;
      });
  if (grouped_clobber_it == call_plan.clobbered_registers.end()) {
    return fail(
        "x86 consumer surface contract: grouped fixture lost direct grouped caller-clobber authority");
  }

  const auto grouped_boundary_prepared = prepare_grouped_riscv_module_with_overrides(
      make_cross_call_preservation_contract_module(),
      {{"pre.only", 2}, {"call.out", 2}, {"carry", 2}});
  const auto grouped_boundary_consumed =
      c4c::backend::x86::consume_plans(grouped_boundary_prepared, "cross_call_preservation_contract");
  const auto* grouped_boundary_storage =
      grouped_boundary_consumed.storage == nullptr
          ? nullptr
          : find_storage_plan_function(grouped_boundary_prepared, "cross_call_preservation_contract");
  const auto* pre_only = grouped_boundary_storage == nullptr
                             ? nullptr
                             : find_storage_value(grouped_boundary_prepared,
                                                  *grouped_boundary_storage,
                                                  "pre.only");
  const auto* call_out = grouped_boundary_storage == nullptr
                             ? nullptr
                             : find_storage_value(grouped_boundary_prepared,
                                                  *grouped_boundary_storage,
                                                  "call.out");
  const auto* grouped_call =
      c4c::backend::x86::find_consumed_call_plan(grouped_boundary_consumed, 0, 2);
  const auto* grouped_arg =
      c4c::backend::x86::find_consumed_call_argument_plan(grouped_boundary_consumed, 0, 2, 0);
  const auto* grouped_result =
      c4c::backend::x86::find_consumed_call_result_plan(grouped_boundary_consumed, 0, 2);
  if (grouped_boundary_consumed.calls == nullptr || grouped_boundary_storage == nullptr ||
      pre_only == nullptr || call_out == nullptr || grouped_call == nullptr ||
      grouped_arg == nullptr || grouped_result == nullptr) {
    return fail(
        "x86 consumer surface contract: grouped argument/result fixture no longer exposes call-boundary authority through x86");
  }
  const auto grouped_arg_dest_span =
      grouped_call_argument_destination_span_summary(*grouped_arg);
  const auto grouped_result_source_span =
      grouped_call_result_source_span_summary(*grouped_result);
  if (grouped_call->direct_callee_name != std::optional<std::string>{"boundary_helper"} ||
      grouped_arg != &grouped_call->arguments.front() ||
      grouped_arg->source_value_id != std::optional<prepare::PreparedValueId>{pre_only->value_id} ||
      grouped_arg->destination_contiguous_width != 2 ||
      grouped_arg->destination_occupied_register_names.size() != 2 ||
      grouped_arg_dest_span.find("/w2[") == std::string::npos ||
      grouped_result->destination_value_id !=
          std::optional<prepare::PreparedValueId>{call_out->value_id} ||
      !grouped_call->result.has_value() || grouped_result != &*grouped_call->result ||
      grouped_result->source_contiguous_width != 2 ||
      grouped_result->source_occupied_register_names.size() != 2 ||
      grouped_result_source_span.find("/w2[") == std::string::npos) {
    return fail(
        "x86 consumer surface contract: grouped call argument/result selectors no longer return the published grouped span authority directly");
  }
  return 0;
}

int check_x86_consumer_surface_reads_grouped_spill_reload_authority() {
  const auto prepared = prepare_grouped_spill_reload_contract_module();
  const auto function_id =
      prepare::resolve_prepared_function_name_id(prepared.names, "grouped_spill_reload_contract");
  if (!function_id.has_value()) {
    return fail("x86 consumer surface contract: failed to resolve grouped_spill_reload_contract");
  }

  const auto consumed = c4c::backend::x86::consume_plans(prepared, "grouped_spill_reload_contract");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != prepare::find_prepared_call_plans(prepared, *function_id) ||
      consumed.regalloc == nullptr ||
      consumed.storage != prepare::find_prepared_storage_plan(prepared, *function_id)) {
    return fail(
        "x86 consumer surface contract: x86 no longer reads grouped spill/reload regalloc authority directly from prepared plans");
  }
  if (consumed.calls == nullptr || consumed.calls->calls.size() != 1 || consumed.storage == nullptr) {
    return fail(
        "x86 consumer surface contract: grouped spill/reload fixture no longer exposes call and storage authority through x86");
  }
  const auto* carry = find_storage_value(prepared, *consumed.storage, "carry");
  if (carry == nullptr || carry->bank != prepare::PreparedRegisterBank::Vreg ||
      carry->contiguous_width != 16 || !carry->register_name.has_value() ||
      carry->occupied_register_names.size() != 16) {
    return fail(
        "x86 consumer surface contract: grouped spill/reload fixture lost grouped storage-plan authority through x86");
  }

  const auto spill_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Vreg &&
               op.contiguous_width == 16 &&
               op.occupied_register_names.size() == 16 &&
               op.occupied_register_names.front() == "v0" &&
               op.occupied_register_names.back() == "v15";
      });
  if (spill_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped spill/reload fixture lost direct grouped spill authority");
  }
  const auto reload_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [spill_it](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Reload &&
               op.value_id == spill_it->value_id &&
               op.register_bank == spill_it->register_bank &&
               op.register_class == spill_it->register_class &&
               op.register_name == spill_it->register_name &&
               op.contiguous_width == spill_it->contiguous_width &&
               op.occupied_register_names == spill_it->occupied_register_names &&
               op.slot_id == spill_it->slot_id &&
               op.stack_offset_bytes == spill_it->stack_offset_bytes &&
               op.source_value_name == spill_it->source_value_name &&
               op.register_placement == spill_it->register_placement &&
               op.spill_slot_placement == spill_it->spill_slot_placement;
      });
  if (reload_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped spill/reload fixture lost reload authority for the spilled grouped value");
  }

  const auto* spill_value = find_regalloc_value(*consumed.regalloc, spill_it->value_id);
  if (spill_value == nullptr || !spill_it->slot_id.has_value() || !spill_it->stack_offset_bytes.has_value() ||
      !spill_value->assigned_stack_slot.has_value() ||
      spill_it->source_value_name != spill_value->value_name ||
      spill_value->assigned_stack_slot->slot_id != *spill_it->slot_id ||
      spill_value->assigned_stack_slot->offset_bytes != *spill_it->stack_offset_bytes) {
    return fail(
        "x86 consumer surface contract: grouped spill/reload fixture lost direct regalloc value authority for the grouped spill");
  }
  if (spill_value->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      spill_value->assigned_register.has_value() || !spill_value->spill_register_authority.has_value() ||
      spill_value->spill_register_authority->register_name != spill_it->register_name ||
      spill_value->spill_register_authority->reg_class != prepare::PreparedRegisterClass::Vector ||
      spill_it->register_class != prepare::PreparedRegisterClass::Vector ||
      spill_value->spill_register_authority->contiguous_width != spill_it->contiguous_width ||
      spill_value->spill_register_authority->occupied_register_names !=
          spill_it->occupied_register_names ||
      spill_value->spill_register_authority->placement != spill_it->register_placement ||
      spill_value->assigned_stack_slot->placement != spill_it->spill_slot_placement) {
    return fail(
        "x86 consumer surface contract: grouped spill/reload fixture lost published spill-register authority for the grouped stack-assigned value");
  }
  return 0;
}

int check_x86_consumer_surface_reads_general_grouped_spill_reload_authority() {
  const auto prepared = prepare_general_grouped_spill_reload_contract_module();
  const auto function_id = prepare::resolve_prepared_function_name_id(
      prepared.names, "general_grouped_spill_reload_contract");
  if (!function_id.has_value()) {
    return fail("x86 consumer surface contract: failed to resolve general_grouped_spill_reload_contract");
  }

  const auto consumed =
      c4c::backend::x86::consume_plans(prepared, "general_grouped_spill_reload_contract");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != prepare::find_prepared_call_plans(prepared, *function_id) ||
      consumed.regalloc == nullptr ||
      consumed.storage != prepare::find_prepared_storage_plan(prepared, *function_id)) {
    return fail(
        "x86 consumer surface contract: x86 no longer reads grouped general spill/reload regalloc authority directly from prepared plans");
  }
  const auto* carry = consumed.storage == nullptr ? nullptr : find_storage_value(prepared, *consumed.storage, "carry");
  if (consumed.calls == nullptr || consumed.calls->calls.size() != 1 || consumed.storage == nullptr ||
      carry == nullptr) {
    return fail(
        "x86 consumer surface contract: grouped general spill/reload fixture no longer exposes call and storage authority through x86");
  }
  if (carry->encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      carry->bank != prepare::PreparedRegisterBank::Gpr || carry->contiguous_width != 2 ||
      !carry->slot_id.has_value() || !carry->stack_offset_bytes.has_value()) {
    return fail(
        "x86 consumer surface contract: grouped general spill/reload fixture lost grouped stack-storage authority through x86");
  }

  const auto spill_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Gpr &&
               op.register_name == std::optional<std::string>{"s1"} &&
               op.contiguous_width == 2 &&
               op.occupied_register_names == std::vector<std::string>{"s1", "s2"};
      });
  if (spill_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped general spill/reload fixture lost direct grouped spill authority");
  }
  const auto reload_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [spill_it](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Reload &&
               op.value_id == spill_it->value_id &&
               op.register_bank == spill_it->register_bank &&
               op.register_class == spill_it->register_class &&
               op.register_name == spill_it->register_name &&
               op.contiguous_width == spill_it->contiguous_width &&
               op.occupied_register_names == spill_it->occupied_register_names &&
               op.slot_id == spill_it->slot_id &&
               op.stack_offset_bytes == spill_it->stack_offset_bytes &&
               op.source_value_name == spill_it->source_value_name &&
               op.register_placement == spill_it->register_placement &&
               op.spill_slot_placement == spill_it->spill_slot_placement;
      });
  if (reload_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped general spill/reload fixture lost reload authority for the spilled grouped value");
  }

  const auto* spill_value = find_regalloc_value(*consumed.regalloc, spill_it->value_id);
  if (spill_value == nullptr || !spill_it->slot_id.has_value() || !spill_it->stack_offset_bytes.has_value() ||
      !spill_value->assigned_stack_slot.has_value() ||
      spill_it->source_value_name != spill_value->value_name ||
      spill_value->assigned_stack_slot->slot_id != *spill_it->slot_id ||
      spill_value->assigned_stack_slot->offset_bytes != *spill_it->stack_offset_bytes ||
      carry->slot_id != spill_it->slot_id ||
      carry->stack_offset_bytes != spill_it->stack_offset_bytes) {
    return fail(
        "x86 consumer surface contract: grouped general spill/reload fixture lost direct stack-slot identity across storage and regalloc");
  }
  if (spill_value->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      spill_value->assigned_register.has_value() || !spill_value->spill_register_authority.has_value() ||
      spill_value->spill_register_authority->register_name != spill_it->register_name ||
      spill_value->spill_register_authority->reg_class != prepare::PreparedRegisterClass::General ||
      spill_it->register_class != prepare::PreparedRegisterClass::General ||
      spill_value->spill_register_authority->contiguous_width != spill_it->contiguous_width ||
      spill_value->spill_register_authority->occupied_register_names !=
          spill_it->occupied_register_names ||
      spill_value->spill_register_authority->placement != spill_it->register_placement ||
      spill_value->assigned_stack_slot->placement != spill_it->spill_slot_placement) {
    return fail(
        "x86 consumer surface contract: grouped general spill/reload fixture lost published spill-register authority for the grouped stack-assigned value");
  }
  return 0;
}

int check_x86_consumer_surface_reads_float_grouped_spill_reload_authority() {
  const auto prepared = prepare_float_grouped_spill_reload_contract_module();
  const auto function_id = prepare::resolve_prepared_function_name_id(
      prepared.names, "float_grouped_spill_reload_contract");
  if (!function_id.has_value()) {
    return fail("x86 consumer surface contract: failed to resolve float_grouped_spill_reload_contract");
  }

  const auto consumed =
      c4c::backend::x86::consume_plans(prepared, "float_grouped_spill_reload_contract");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != prepare::find_prepared_call_plans(prepared, *function_id) ||
      consumed.regalloc == nullptr ||
      consumed.storage != prepare::find_prepared_storage_plan(prepared, *function_id)) {
    return fail(
        "x86 consumer surface contract: x86 no longer reads grouped float spill/reload regalloc authority directly from prepared plans");
  }
  const auto* carry = consumed.storage == nullptr ? nullptr : find_storage_value(prepared, *consumed.storage, "carry");
  if (consumed.calls == nullptr || consumed.calls->calls.size() != 1 || consumed.storage == nullptr ||
      carry == nullptr) {
    return fail(
        "x86 consumer surface contract: grouped float spill/reload fixture no longer exposes call and storage authority through x86");
  }
  if (carry->encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      carry->bank != prepare::PreparedRegisterBank::Fpr || carry->contiguous_width != 2 ||
      !carry->slot_id.has_value() || !carry->stack_offset_bytes.has_value()) {
    return fail(
        "x86 consumer surface contract: grouped float spill/reload fixture lost grouped stack-storage authority through x86");
  }

  const auto spill_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Fpr &&
               op.register_name == std::optional<std::string>{"fs1"} &&
               op.contiguous_width == 2 &&
               op.occupied_register_names == std::vector<std::string>{"fs1", "fs2"};
      });
  if (spill_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped float spill/reload fixture lost direct grouped spill authority");
  }
  const auto reload_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [spill_it](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Reload &&
               op.value_id == spill_it->value_id &&
               op.register_bank == spill_it->register_bank &&
               op.register_class == spill_it->register_class &&
               op.register_name == spill_it->register_name &&
               op.contiguous_width == spill_it->contiguous_width &&
               op.occupied_register_names == spill_it->occupied_register_names &&
               op.slot_id == spill_it->slot_id &&
               op.stack_offset_bytes == spill_it->stack_offset_bytes &&
               op.source_value_name == spill_it->source_value_name &&
               op.register_placement == spill_it->register_placement &&
               op.spill_slot_placement == spill_it->spill_slot_placement;
      });
  if (reload_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped float spill/reload fixture lost reload authority for the spilled grouped value");
  }

  const auto* spill_value = find_regalloc_value(*consumed.regalloc, spill_it->value_id);
  if (spill_value == nullptr || !spill_it->slot_id.has_value() || !spill_it->stack_offset_bytes.has_value() ||
      !spill_value->assigned_stack_slot.has_value() ||
      spill_it->source_value_name != spill_value->value_name ||
      spill_value->assigned_stack_slot->slot_id != *spill_it->slot_id ||
      spill_value->assigned_stack_slot->offset_bytes != *spill_it->stack_offset_bytes ||
      carry->slot_id != spill_it->slot_id ||
      carry->stack_offset_bytes != spill_it->stack_offset_bytes) {
    return fail(
        "x86 consumer surface contract: grouped float spill/reload fixture lost direct stack-slot identity across storage and regalloc");
  }
  if (spill_value->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      spill_value->assigned_register.has_value() || !spill_value->spill_register_authority.has_value() ||
      spill_value->spill_register_authority->register_name != spill_it->register_name ||
      spill_value->spill_register_authority->reg_class != prepare::PreparedRegisterClass::Float ||
      spill_it->register_class != prepare::PreparedRegisterClass::Float ||
      spill_value->spill_register_authority->contiguous_width != spill_it->contiguous_width ||
      spill_value->spill_register_authority->occupied_register_names !=
          spill_it->occupied_register_names ||
      spill_value->spill_register_authority->placement != spill_it->register_placement ||
      spill_value->assigned_stack_slot->placement != spill_it->spill_slot_placement) {
    return fail(
        "x86 consumer surface contract: grouped float spill/reload fixture lost published spill-register authority for the grouped stack-assigned value");
  }
  return 0;
}

int check_x86_module_emitter_reads_grouped_spill_reload_authority() {
  auto prepared = prepare_grouped_spill_reload_contract_module();
  prepared.target_profile = x86_target_profile();
  prepared.module.target_triple = "x86_64-unknown-linux-gnu";
  const auto asm_text = c4c::backend::x86::api::emit_prepared_module(prepared);
  const auto consumed = c4c::backend::x86::consume_plans(prepared, "grouped_spill_reload_contract");

  if (consumed.regalloc == nullptr || consumed.storage == nullptr) {
    return fail(
        "x86 module emitter contract: grouped spill/reload fixture no longer exposes regalloc/storage authority");
  }
  const auto* carry = find_storage_value(prepared, *consumed.storage, "carry");
  if (carry == nullptr) {
    return fail(
        "x86 module emitter contract: grouped spill/reload fixture lost grouped storage identity");
  }

  const auto spill_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.contiguous_width == 16 && op.occupied_register_names.size() == 16;
      });
  const auto reload_it = spill_it == consumed.regalloc->spill_reload_ops.end()
                             ? consumed.regalloc->spill_reload_ops.end()
                             : std::find_if(
                                   consumed.regalloc->spill_reload_ops.begin(),
                                   consumed.regalloc->spill_reload_ops.end(),
                                   [spill_it](const prepare::PreparedSpillReloadOp& op) {
                                     return op.op_kind ==
                                                prepare::PreparedSpillReloadOpKind::Reload &&
                                            op.value_id == spill_it->value_id &&
                                            op.register_bank == spill_it->register_bank &&
                                            op.register_name == spill_it->register_name &&
                                            op.contiguous_width == spill_it->contiguous_width &&
                                            op.occupied_register_names ==
                                                spill_it->occupied_register_names &&
                                            op.slot_id == spill_it->slot_id &&
                                            op.stack_offset_bytes ==
                                                spill_it->stack_offset_bytes;
                                   });
  if (spill_it == consumed.regalloc->spill_reload_ops.end() ||
      reload_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 module emitter contract: grouped spill/reload fixture lost spill or reload authority");
  }
  const auto* spill_value = find_regalloc_value(*consumed.regalloc, spill_it->value_id);
  if (spill_value == nullptr) {
    return fail(
        "x86 module emitter contract: grouped spill/reload fixture lost grouped value identity");
  }

  const auto is_grouped = [](std::size_t contiguous_width,
                             const std::vector<std::string>& occupied_register_names) {
    return contiguous_width > 1 || occupied_register_names.size() > 1;
  };
  const auto grouped_saved_count =
      consumed.frame == nullptr
          ? std::size_t{0}
          : static_cast<std::size_t>(std::count_if(
                consumed.frame->saved_callee_registers.begin(),
                consumed.frame->saved_callee_registers.end(),
                [&is_grouped](const prepare::PreparedSavedRegister& saved) {
                  return is_grouped(saved.contiguous_width, saved.occupied_register_names);
                }));
  const auto grouped_storage_count =
      static_cast<std::size_t>(std::count_if(
          consumed.storage->values.begin(),
          consumed.storage->values.end(),
          [&is_grouped](const prepare::PreparedStoragePlanValue& value) {
            return is_grouped(value.contiguous_width, value.occupied_register_names);
          }));
  const auto grouped_spill_count =
      static_cast<std::size_t>(std::count_if(
          consumed.regalloc->spill_reload_ops.begin(),
          consumed.regalloc->spill_reload_ops.end(),
          [&is_grouped](const prepare::PreparedSpillReloadOp& op) {
            return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
                   is_grouped(op.contiguous_width, op.occupied_register_names);
          }));
  const auto grouped_reload_count =
      static_cast<std::size_t>(std::count_if(
          consumed.regalloc->spill_reload_ops.begin(),
          consumed.regalloc->spill_reload_ops.end(),
          [&is_grouped](const prepare::PreparedSpillReloadOp& op) {
            return op.op_kind == prepare::PreparedSpillReloadOpKind::Reload &&
                   is_grouped(op.contiguous_width, op.occupied_register_names);
          }));
  const auto grouped_preserved_count =
      consumed.calls == nullptr
          ? std::size_t{0}
          : [&]() {
              std::size_t count = 0;
              for (const auto& call : consumed.calls->calls) {
                count += static_cast<std::size_t>(std::count_if(
                    call.preserved_values.begin(),
                    call.preserved_values.end(),
                    [&is_grouped](const prepare::PreparedCallPreservedValue& preserved) {
                      return is_grouped(preserved.contiguous_width,
                                        preserved.occupied_register_names);
                    }));
              }
              return count;
            }();
  const auto grouped_clobbered_count =
      consumed.calls == nullptr
          ? std::size_t{0}
          : [&]() {
              std::size_t count = 0;
              for (const auto& call : consumed.calls->calls) {
                count += static_cast<std::size_t>(std::count_if(
                    call.clobbered_registers.begin(),
                    call.clobbered_registers.end(),
                    [&is_grouped](const prepare::PreparedClobberedRegister& clobber) {
                      return is_grouped(clobber.contiguous_width,
                                        clobber.occupied_register_names);
                    }));
              }
              return count;
            }();
  const auto grouped_call_argument_count =
      consumed.calls == nullptr
          ? std::size_t{0}
          : [&]() {
              std::size_t count = 0;
              for (const auto& call : consumed.calls->calls) {
                count += static_cast<std::size_t>(std::count_if(
                    call.arguments.begin(),
                    call.arguments.end(),
                    [&is_grouped](const prepare::PreparedCallArgumentPlan& argument) {
                      return is_grouped(argument.destination_contiguous_width,
                                        argument.destination_occupied_register_names);
                    }));
              }
              return count;
            }();
  const auto grouped_call_result_count =
      consumed.calls == nullptr
          ? std::size_t{0}
          : [&]() {
              std::size_t count = 0;
              for (const auto& call : consumed.calls->calls) {
                if (call.result.has_value() &&
                    is_grouped(call.result->source_contiguous_width,
                               call.result->source_occupied_register_names)) {
                  ++count;
                }
              }
              return count;
            }();
  const auto expected_summary =
      std::string("    # grouped authority: saved=") + std::to_string(grouped_saved_count) +
      " preserved=" + std::to_string(grouped_preserved_count) +
      " clobbered=" + std::to_string(grouped_clobbered_count) +
      " call_args=" + std::to_string(grouped_call_argument_count) +
      " call_results=" + std::to_string(grouped_call_result_count) +
      " spills=" + std::to_string(grouped_spill_count) +
      " reloads=" + std::to_string(grouped_reload_count) +
      " storage=" + std::to_string(grouped_storage_count);
  const auto expected_spill =
      std::string("    # grouped spill value_id=") + std::to_string(spill_it->value_id) +
      " value=" + std::string(prepare::prepared_value_name(prepared.names, spill_value->value_name)) +
      " span=" +
      grouped_span_summary(spill_it->register_bank,
                           spill_it->register_name.has_value()
                               ? std::optional<std::string_view>{*spill_it->register_name}
                               : std::nullopt,
                           spill_it->contiguous_width,
                           spill_it->occupied_register_names) +
      " slot_id=#" + std::to_string(*spill_it->slot_id) + " stack_offset=" +
      std::to_string(*spill_it->stack_offset_bytes);
  const auto expected_reload =
      std::string("    # grouped reload value_id=") + std::to_string(reload_it->value_id) +
      " value=" + std::string(prepare::prepared_value_name(prepared.names, spill_value->value_name)) +
      " span=" +
      grouped_span_summary(reload_it->register_bank,
                           reload_it->register_name.has_value()
                               ? std::optional<std::string_view>{*reload_it->register_name}
                               : std::nullopt,
                           reload_it->contiguous_width,
                           reload_it->occupied_register_names) +
      " slot_id=#" + std::to_string(*reload_it->slot_id) + " stack_offset=" +
      std::to_string(*reload_it->stack_offset_bytes);
  const auto expected_storage =
      std::string("    # grouped storage value=carry span=") +
      grouped_span_summary(carry->bank,
                           carry->register_name.has_value()
                               ? std::optional<std::string_view>{*carry->register_name}
                               : std::nullopt,
                           carry->contiguous_width,
                           carry->occupied_register_names);

  if (asm_text.find(expected_summary) == std::string::npos ||
      asm_text.find(expected_spill) == std::string::npos ||
      asm_text.find(expected_reload) == std::string::npos ||
      asm_text.find(expected_storage) == std::string::npos) {
    return fail(
        "x86 module emitter contract: grouped spill/reload comments no longer consume direct prepared authority");
  }
  return 0;
}

int check_x86_module_emitter_reads_grouped_call_boundary_authority() {
  auto prepared = prepare_grouped_riscv_module_with_overrides(
      make_grouped_cross_call_preservation_contract_module(),
      {{"p.vcarry", 2}, {"carry.pre", 2}, {"post.local", 1}});
  prepared.target_profile = x86_target_profile();
  prepared.module.target_triple = "x86_64-unknown-linux-gnu";
  const auto asm_text = c4c::backend::x86::api::emit_prepared_module(prepared);
  const auto consumed =
      c4c::backend::x86::consume_plans(prepared, "grouped_cross_call_preservation_contract");

  if (consumed.frame == nullptr || consumed.calls == nullptr || consumed.calls->calls.size() != 1 ||
      consumed.storage == nullptr) {
    return fail(
        "x86 module emitter contract: grouped call-boundary fixture no longer exposes frame/call/storage authority");
  }
  const auto* carry = find_storage_value(prepared, *consumed.storage, "carry.pre");
  if (carry == nullptr) {
    return fail(
        "x86 module emitter contract: grouped call-boundary fixture lost grouped storage identity");
  }

  const auto& call_plan = consumed.calls->calls.front();
  const auto preserved_it = std::find_if(
      call_plan.preserved_values.begin(),
      call_plan.preserved_values.end(),
      [carry](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == carry->value_id;
      });
  if (preserved_it == call_plan.preserved_values.end()) {
    return fail(
        "x86 module emitter contract: grouped call-boundary fixture lost preserved-value authority");
  }
  const auto saved_it = std::find_if(
      consumed.frame->saved_callee_registers.begin(),
      consumed.frame->saved_callee_registers.end(),
      [preserved_it](const prepare::PreparedSavedRegister& saved) {
        return saved.occupied_register_names == preserved_it->occupied_register_names;
      });
  if (saved_it == consumed.frame->saved_callee_registers.end()) {
    return fail(
        "x86 module emitter contract: grouped call-boundary fixture lost saved-register authority");
  }
  const auto grouped_clobber_it = std::find_if(
      call_plan.clobbered_registers.begin(),
      call_plan.clobbered_registers.end(),
      [](const prepare::PreparedClobberedRegister& clobber) {
        return clobber.contiguous_width == 2 && clobber.occupied_register_names.size() == 2;
      });
  if (grouped_clobber_it == call_plan.clobbered_registers.end()) {
    return fail("x86 module emitter contract: grouped call-boundary fixture lost clobber authority");
  }

  const auto is_grouped = [](std::size_t contiguous_width,
                             const std::vector<std::string>& occupied_register_names) {
    return contiguous_width > 1 || occupied_register_names.size() > 1;
  };
  const auto grouped_saved_count = static_cast<std::size_t>(std::count_if(
      consumed.frame->saved_callee_registers.begin(),
      consumed.frame->saved_callee_registers.end(),
      [&is_grouped](const prepare::PreparedSavedRegister& saved) {
        return is_grouped(saved.contiguous_width, saved.occupied_register_names);
      }));
  const auto grouped_preserved_count = static_cast<std::size_t>(std::count_if(
      call_plan.preserved_values.begin(),
      call_plan.preserved_values.end(),
      [&is_grouped](const prepare::PreparedCallPreservedValue& preserved) {
        return is_grouped(preserved.contiguous_width, preserved.occupied_register_names);
      }));
  const auto grouped_clobbered_count = static_cast<std::size_t>(std::count_if(
      call_plan.clobbered_registers.begin(),
      call_plan.clobbered_registers.end(),
      [&is_grouped](const prepare::PreparedClobberedRegister& clobber) {
        return is_grouped(clobber.contiguous_width, clobber.occupied_register_names);
      }));
  const auto grouped_call_argument_count = static_cast<std::size_t>(std::count_if(
      call_plan.arguments.begin(),
      call_plan.arguments.end(),
      [&is_grouped](const prepare::PreparedCallArgumentPlan& argument) {
        return is_grouped(argument.destination_contiguous_width,
                          argument.destination_occupied_register_names);
      }));
  const auto grouped_call_result_count =
      call_plan.result.has_value() &&
              is_grouped(call_plan.result->source_contiguous_width,
                         call_plan.result->source_occupied_register_names)
          ? std::size_t{1}
          : std::size_t{0};
  const auto grouped_storage_count = static_cast<std::size_t>(std::count_if(
      consumed.storage->values.begin(),
      consumed.storage->values.end(),
      [&is_grouped](const prepare::PreparedStoragePlanValue& value) {
        return is_grouped(value.contiguous_width, value.occupied_register_names);
      }));

  const auto expected_summary =
      std::string("    # grouped authority: saved=") + std::to_string(grouped_saved_count) +
      " preserved=" + std::to_string(grouped_preserved_count) +
      " clobbered=" + std::to_string(grouped_clobbered_count) +
      " call_args=" + std::to_string(grouped_call_argument_count) +
      " call_results=" + std::to_string(grouped_call_result_count) +
      " spills=0 reloads=0" +
      " storage=" + std::to_string(grouped_storage_count);
  const auto expected_saved =
      std::string("    # grouped saved save_index=") + std::to_string(saved_it->save_index) +
      " span=" +
      grouped_span_summary(saved_it->bank,
                           std::string_view(saved_it->register_name),
                           saved_it->contiguous_width,
                           saved_it->occupied_register_names);
  const auto expected_preserved =
      std::string("    # grouped preserve call#0 value=") +
      std::string(prepare::prepared_value_name(prepared.names, preserved_it->value_name)) +
      " route=" + std::string(prepare::prepared_call_preservation_route_name(preserved_it->route)) +
      " span=" +
      grouped_span_summary(
          preserved_it->register_bank.value_or(prepare::PreparedRegisterBank::None),
          preserved_it->register_name.has_value()
              ? std::optional<std::string_view>{*preserved_it->register_name}
              : std::nullopt,
          preserved_it->contiguous_width,
          preserved_it->occupied_register_names) +
      " save_index=" + std::to_string(*preserved_it->callee_saved_save_index);
  const auto expected_clobber =
      std::string("    # grouped clobber call#0 span=") + clobber_summary(*grouped_clobber_it);
  const auto expected_storage =
      std::string("    # grouped storage value=carry.pre span=") +
      grouped_span_summary(carry->bank,
                           carry->register_name.has_value()
                               ? std::optional<std::string_view>{*carry->register_name}
                               : std::nullopt,
                           carry->contiguous_width,
                           carry->occupied_register_names);

  if (asm_text.find(expected_summary) == std::string::npos ||
      asm_text.find(expected_saved) == std::string::npos ||
      asm_text.find(expected_preserved) == std::string::npos ||
      asm_text.find(expected_clobber) == std::string::npos ||
      asm_text.find(expected_storage) == std::string::npos) {
    return fail(
        "x86 module emitter contract: grouped call-boundary comments no longer consume direct prepared authority");
  }

  auto grouped_boundary_prepared = prepare_grouped_riscv_module_with_overrides(
      make_cross_call_preservation_contract_module(),
      {{"pre.only", 2}, {"call.out", 2}, {"carry", 2}});
  grouped_boundary_prepared.target_profile = x86_target_profile();
  grouped_boundary_prepared.module.target_triple = "x86_64-unknown-linux-gnu";
  const auto grouped_boundary_asm =
      c4c::backend::x86::api::emit_prepared_module(grouped_boundary_prepared);
  const auto grouped_boundary_consumed =
      c4c::backend::x86::consume_plans(grouped_boundary_prepared, "cross_call_preservation_contract");
  const auto* grouped_boundary_call =
      c4c::backend::x86::find_consumed_call_plan(grouped_boundary_consumed, 0, 2);
  const auto* grouped_boundary_arg =
      c4c::backend::x86::find_consumed_call_argument_plan(grouped_boundary_consumed, 0, 2, 0);
  const auto* grouped_boundary_result =
      c4c::backend::x86::find_consumed_call_result_plan(grouped_boundary_consumed, 0, 2);
  if (grouped_boundary_call == nullptr || grouped_boundary_arg == nullptr ||
      grouped_boundary_result == nullptr ||
      !grouped_boundary_arg->source_value_id.has_value() ||
      !grouped_boundary_result->destination_value_id.has_value()) {
    return fail(
        "x86 module emitter contract: grouped argument/result fixture lost direct grouped boundary authority");
  }
  const auto expected_boundary_arg =
      std::string("    # grouped arg call#0 arg#") +
      std::to_string(grouped_boundary_arg->arg_index) +
      " source_value_id=" + std::to_string(*grouped_boundary_arg->source_value_id) +
      " dest_span=" + grouped_call_argument_destination_span_summary(*grouped_boundary_arg);
  const auto expected_boundary_result =
      std::string("    # grouped result call#0 destination_value_id=") +
      std::to_string(*grouped_boundary_result->destination_value_id) + " source_span=" +
      grouped_call_result_source_span_summary(*grouped_boundary_result);
  if (grouped_boundary_asm.find(expected_boundary_arg) == std::string::npos ||
      grouped_boundary_asm.find(expected_boundary_result) == std::string::npos) {
    return fail(
        "x86 module emitter contract: grouped call argument/result comments no longer consume direct prepared authority");
  }
  return 0;
}

int check_x86_route_debug_reads_grouped_call_boundary_authority() {
  const auto prepared = prepare_grouped_riscv_module_with_overrides(
      make_grouped_cross_call_preservation_contract_module(),
      {{"p.vcarry", 2}, {"carry.pre", 2}, {"post.local", 1}});
  const auto summary = c4c::backend::x86::summarize_prepared_module_routes(
      prepared, "grouped_cross_call_preservation_contract");
  const auto trace = c4c::backend::x86::trace_prepared_module_routes(
      prepared, "grouped_cross_call_preservation_contract");

  const auto* frame = find_frame_plan_function(prepared, "grouped_cross_call_preservation_contract");
  const auto* calls = find_call_plans_function(prepared, "grouped_cross_call_preservation_contract");
  const auto* storage =
      find_storage_plan_function(prepared, "grouped_cross_call_preservation_contract");
  const auto* carry = storage == nullptr ? nullptr : find_storage_value(prepared, *storage, "carry.pre");
  if (frame == nullptr || calls == nullptr || calls->calls.size() != 1 || storage == nullptr ||
      carry == nullptr) {
    return fail(
        "x86 route debug contract: grouped fixture no longer exposes frame/call/storage authority");
  }

  const auto& call_plan = calls->calls.front();
  const auto preserved_it = std::find_if(
      call_plan.preserved_values.begin(),
      call_plan.preserved_values.end(),
      [carry](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == carry->value_id;
      });
  if (preserved_it == call_plan.preserved_values.end()) {
    return fail("x86 route debug contract: grouped fixture lost preserved-value authority");
  }
  const auto saved_it = std::find_if(
      frame->saved_callee_registers.begin(),
      frame->saved_callee_registers.end(),
      [preserved_it](const prepare::PreparedSavedRegister& saved) {
        return saved.occupied_register_names == preserved_it->occupied_register_names;
      });
  if (saved_it == frame->saved_callee_registers.end()) {
    return fail("x86 route debug contract: grouped fixture lost saved-register authority");
  }
  const auto grouped_clobber_it = std::find_if(
      call_plan.clobbered_registers.begin(),
      call_plan.clobbered_registers.end(),
      [](const prepare::PreparedClobberedRegister& clobber) {
        return clobber.contiguous_width == 2 && clobber.occupied_register_names.size() == 2;
      });
  if (grouped_clobber_it == call_plan.clobbered_registers.end()) {
    return fail("x86 route debug contract: grouped fixture lost clobber authority");
  }

  const auto is_grouped = [](std::size_t contiguous_width,
                             const std::vector<std::string>& occupied_register_names) {
    return contiguous_width > 1 || occupied_register_names.size() > 1;
  };
  const auto grouped_saved_count = static_cast<std::size_t>(std::count_if(
      frame->saved_callee_registers.begin(),
      frame->saved_callee_registers.end(),
      [&is_grouped](const prepare::PreparedSavedRegister& saved) {
        return is_grouped(saved.contiguous_width, saved.occupied_register_names);
      }));
  const auto grouped_preserved_count = static_cast<std::size_t>(std::count_if(
      call_plan.preserved_values.begin(),
      call_plan.preserved_values.end(),
      [&is_grouped](const prepare::PreparedCallPreservedValue& preserved) {
        return is_grouped(preserved.contiguous_width, preserved.occupied_register_names);
      }));
  const auto grouped_clobbered_count = static_cast<std::size_t>(std::count_if(
      call_plan.clobbered_registers.begin(),
      call_plan.clobbered_registers.end(),
      [&is_grouped](const prepare::PreparedClobberedRegister& clobber) {
        return is_grouped(clobber.contiguous_width, clobber.occupied_register_names);
      }));
  const auto grouped_call_argument_count = static_cast<std::size_t>(std::count_if(
      call_plan.arguments.begin(),
      call_plan.arguments.end(),
      [&is_grouped](const prepare::PreparedCallArgumentPlan& argument) {
        return is_grouped(argument.destination_contiguous_width,
                          argument.destination_occupied_register_names);
      }));
  const auto grouped_call_result_count =
      call_plan.result.has_value() &&
              is_grouped(call_plan.result->source_contiguous_width,
                         call_plan.result->source_occupied_register_names)
          ? std::size_t{1}
          : std::size_t{0};
  const auto grouped_storage_count = static_cast<std::size_t>(std::count_if(
      storage->values.begin(),
      storage->values.end(),
      [&is_grouped](const prepare::PreparedStoragePlanValue& value) {
        return is_grouped(value.contiguous_width, value.occupied_register_names);
      }));
  const auto expected_summary =
      std::string("grouped authority: saved=") + std::to_string(grouped_saved_count) +
      " preserved=" + std::to_string(grouped_preserved_count) +
      " clobbered=" + std::to_string(grouped_clobbered_count) +
      " call_args=" + std::to_string(grouped_call_argument_count) +
      " call_results=" + std::to_string(grouped_call_result_count) +
      " spills=0 reloads=0" +
      " storage=" + std::to_string(grouped_storage_count);
  if (summary.find(expected_summary) == std::string::npos) {
    return fail("x86 route debug contract: summary no longer reports grouped authority counts");
  }

  const auto expected_saved =
      std::string("grouped saved save_index=") + std::to_string(saved_it->save_index) + " span=" +
      grouped_span_summary(saved_it->bank,
                           std::string_view(saved_it->register_name),
                           saved_it->contiguous_width,
                           saved_it->occupied_register_names);
  const auto expected_preserved =
      std::string("grouped preserve call#0 value=") +
      std::string(prepare::prepared_value_name(prepared.names, preserved_it->value_name)) +
      " route=" + std::string(prepare::prepared_call_preservation_route_name(preserved_it->route)) +
      " span=" +
      grouped_span_summary(
          preserved_it->register_bank.value_or(prepare::PreparedRegisterBank::None),
          preserved_it->register_name.has_value()
              ? std::optional<std::string_view>{*preserved_it->register_name}
              : std::nullopt,
          preserved_it->contiguous_width,
          preserved_it->occupied_register_names) +
      " save_index=" + std::to_string(*preserved_it->callee_saved_save_index);
  const auto expected_clobber =
      std::string("grouped clobber call#0 span=") + clobber_summary(*grouped_clobber_it);
  const auto expected_storage =
      std::string("grouped storage value=carry.pre span=") +
      grouped_span_summary(carry->bank,
                           carry->register_name.has_value()
                               ? std::optional<std::string_view>{*carry->register_name}
                               : std::nullopt,
                           carry->contiguous_width,
                           carry->occupied_register_names);

  if (trace.find(expected_saved) == std::string::npos ||
      trace.find(expected_preserved) == std::string::npos ||
      trace.find(expected_clobber) == std::string::npos ||
      trace.find(expected_storage) == std::string::npos) {
    return fail(
        "x86 route debug contract: trace no longer reads grouped saved/preserved/clobber/storage authority directly");
  }

  const auto grouped_boundary_prepared = prepare_grouped_riscv_module_with_overrides(
      make_cross_call_preservation_contract_module(),
      {{"pre.only", 2}, {"call.out", 2}, {"carry", 2}});
  const auto grouped_boundary_summary = c4c::backend::x86::summarize_prepared_module_routes(
      grouped_boundary_prepared, "cross_call_preservation_contract");
  const auto grouped_boundary_trace = c4c::backend::x86::trace_prepared_module_routes(
      grouped_boundary_prepared, "cross_call_preservation_contract");
  const auto grouped_boundary_consumed =
      c4c::backend::x86::consume_plans(grouped_boundary_prepared, "cross_call_preservation_contract");
  const auto* grouped_boundary_call =
      c4c::backend::x86::find_consumed_call_plan(grouped_boundary_consumed, 0, 2);
  const auto* grouped_boundary_arg =
      c4c::backend::x86::find_consumed_call_argument_plan(grouped_boundary_consumed, 0, 2, 0);
  const auto* grouped_boundary_result =
      c4c::backend::x86::find_consumed_call_result_plan(grouped_boundary_consumed, 0, 2);
  if (grouped_boundary_consumed.calls == nullptr || grouped_boundary_call == nullptr ||
      grouped_boundary_arg == nullptr || grouped_boundary_result == nullptr ||
      !grouped_boundary_arg->source_value_id.has_value() ||
      !grouped_boundary_result->destination_value_id.has_value()) {
    return fail(
        "x86 route debug contract: grouped argument/result fixture lost direct grouped boundary authority");
  }
  const auto grouped_boundary_saved_count = grouped_boundary_consumed.frame == nullptr
                                                ? std::size_t{0}
                                                : static_cast<std::size_t>(std::count_if(
                                                      grouped_boundary_consumed.frame->saved_callee_registers.begin(),
                                                      grouped_boundary_consumed.frame->saved_callee_registers.end(),
                                                      [&is_grouped](const prepare::PreparedSavedRegister& saved) {
                                                        return is_grouped(saved.contiguous_width,
                                                                          saved.occupied_register_names);
                                                      }));
  const auto grouped_boundary_preserved_count = static_cast<std::size_t>(std::count_if(
      grouped_boundary_call->preserved_values.begin(),
      grouped_boundary_call->preserved_values.end(),
      [&is_grouped](const prepare::PreparedCallPreservedValue& preserved) {
        return is_grouped(preserved.contiguous_width, preserved.occupied_register_names);
      }));
  const auto grouped_boundary_clobbered_count = static_cast<std::size_t>(std::count_if(
      grouped_boundary_call->clobbered_registers.begin(),
      grouped_boundary_call->clobbered_registers.end(),
      [&is_grouped](const prepare::PreparedClobberedRegister& clobber) {
        return is_grouped(clobber.contiguous_width, clobber.occupied_register_names);
      }));
  const auto grouped_boundary_call_argument_count = static_cast<std::size_t>(std::count_if(
      grouped_boundary_call->arguments.begin(),
      grouped_boundary_call->arguments.end(),
      [&is_grouped](const prepare::PreparedCallArgumentPlan& argument) {
        return is_grouped(argument.destination_contiguous_width,
                          argument.destination_occupied_register_names);
      }));
  const auto grouped_boundary_call_result_count =
      grouped_boundary_call->result.has_value() &&
              is_grouped(grouped_boundary_call->result->source_contiguous_width,
                         grouped_boundary_call->result->source_occupied_register_names)
          ? std::size_t{1}
          : std::size_t{0};
  const auto grouped_boundary_storage_count = grouped_boundary_consumed.storage == nullptr
                                                  ? std::size_t{0}
                                                  : static_cast<std::size_t>(std::count_if(
                                                        grouped_boundary_consumed.storage->values.begin(),
                                                        grouped_boundary_consumed.storage->values.end(),
                                                        [&is_grouped](const prepare::PreparedStoragePlanValue& value) {
                                                          return is_grouped(value.contiguous_width,
                                                                            value.occupied_register_names);
                                                        }));
  const auto expected_boundary_summary =
      std::string("grouped authority: saved=") + std::to_string(grouped_boundary_saved_count) +
      " preserved=" + std::to_string(grouped_boundary_preserved_count) +
      " clobbered=" + std::to_string(grouped_boundary_clobbered_count) +
      " call_args=" + std::to_string(grouped_boundary_call_argument_count) +
      " call_results=" + std::to_string(grouped_boundary_call_result_count) +
      " spills=0 reloads=0" +
      " storage=" + std::to_string(grouped_boundary_storage_count);
  const auto expected_boundary_arg =
      std::string("grouped arg call#0 arg#") +
      std::to_string(grouped_boundary_arg->arg_index) +
      " source_value_id=" + std::to_string(*grouped_boundary_arg->source_value_id) +
      " dest_span=" + grouped_call_argument_destination_span_summary(*grouped_boundary_arg);
  const auto expected_boundary_result =
      std::string("grouped result call#0 destination_value_id=") +
      std::to_string(*grouped_boundary_result->destination_value_id) + " source_span=" +
      grouped_call_result_source_span_summary(*grouped_boundary_result);
  if (grouped_boundary_summary.find(expected_boundary_summary) == std::string::npos ||
      grouped_boundary_trace.find(expected_boundary_arg) == std::string::npos ||
      grouped_boundary_trace.find(expected_boundary_result) == std::string::npos) {
    return fail(
        "x86 route debug contract: grouped argument/result summaries no longer read direct prepared boundary authority");
  }
  return 0;
}

int check_x86_route_debug_reads_grouped_spill_reload_authority() {
  const auto prepared = prepare_grouped_spill_reload_contract_module();
  const auto summary =
      c4c::backend::x86::summarize_prepared_module_routes(prepared, "grouped_spill_reload_contract");
  const auto trace =
      c4c::backend::x86::trace_prepared_module_routes(prepared, "grouped_spill_reload_contract");
  const auto consumed = c4c::backend::x86::consume_plans(prepared, "grouped_spill_reload_contract");

  if (consumed.regalloc == nullptr || consumed.storage == nullptr) {
    return fail("x86 route debug contract: grouped spill/reload fixture no longer exposes regalloc and storage authority");
  }
  const auto* carry = find_storage_value(prepared, *consumed.storage, "carry");
  if (carry == nullptr) {
    return fail("x86 route debug contract: grouped spill/reload fixture lost grouped storage value authority");
  }

  const auto is_grouped = [](std::size_t contiguous_width,
                             const std::vector<std::string>& occupied_register_names) {
    return contiguous_width > 1 || occupied_register_names.size() > 1;
  };
  const auto grouped_spill_count = static_cast<std::size_t>(std::count_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [&is_grouped](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               is_grouped(op.contiguous_width, op.occupied_register_names);
      }));
  const auto grouped_reload_count = static_cast<std::size_t>(std::count_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [&is_grouped](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Reload &&
               is_grouped(op.contiguous_width, op.occupied_register_names);
      }));
  const auto grouped_storage_count = static_cast<std::size_t>(std::count_if(
      consumed.storage->values.begin(),
      consumed.storage->values.end(),
      [&is_grouped](const prepare::PreparedStoragePlanValue& value) {
        return is_grouped(value.contiguous_width, value.occupied_register_names);
      }));
  const auto grouped_saved_count = consumed.frame == nullptr
                                       ? std::size_t{0}
                                       : static_cast<std::size_t>(std::count_if(
                                             consumed.frame->saved_callee_registers.begin(),
                                             consumed.frame->saved_callee_registers.end(),
                                             [&is_grouped](const prepare::PreparedSavedRegister& saved) {
                                               return is_grouped(saved.contiguous_width,
                                                                 saved.occupied_register_names);
                                             }));
  const auto grouped_preserved_count =
      consumed.calls == nullptr
          ? std::size_t{0}
          : [&]() {
              std::size_t count = 0;
              for (const auto& call : consumed.calls->calls) {
                count += static_cast<std::size_t>(std::count_if(
                    call.preserved_values.begin(),
                    call.preserved_values.end(),
                    [&is_grouped](const prepare::PreparedCallPreservedValue& preserved) {
                      return is_grouped(preserved.contiguous_width,
                                        preserved.occupied_register_names);
                    }));
              }
              return count;
            }();
  const auto grouped_clobbered_count =
      consumed.calls == nullptr
          ? std::size_t{0}
          : [&]() {
              std::size_t count = 0;
              for (const auto& call : consumed.calls->calls) {
                count += static_cast<std::size_t>(std::count_if(
                    call.clobbered_registers.begin(),
                    call.clobbered_registers.end(),
                    [&is_grouped](const prepare::PreparedClobberedRegister& clobber) {
                      return is_grouped(clobber.contiguous_width,
                                        clobber.occupied_register_names);
                    }));
              }
              return count;
            }();
  const auto grouped_call_argument_count =
      consumed.calls == nullptr
          ? std::size_t{0}
          : [&]() {
              std::size_t count = 0;
              for (const auto& call : consumed.calls->calls) {
                count += static_cast<std::size_t>(std::count_if(
                    call.arguments.begin(),
                    call.arguments.end(),
                    [&is_grouped](const prepare::PreparedCallArgumentPlan& argument) {
                      return is_grouped(argument.destination_contiguous_width,
                                        argument.destination_occupied_register_names);
                    }));
              }
              return count;
            }();
  const auto grouped_call_result_count =
      consumed.calls == nullptr
          ? std::size_t{0}
          : [&]() {
              std::size_t count = 0;
              for (const auto& call : consumed.calls->calls) {
                if (call.result.has_value() &&
                    is_grouped(call.result->source_contiguous_width,
                               call.result->source_occupied_register_names)) {
                  ++count;
                }
              }
              return count;
            }();

  const auto expected_summary =
      std::string("grouped authority: saved=") + std::to_string(grouped_saved_count) +
      " preserved=" + std::to_string(grouped_preserved_count) +
      " clobbered=" + std::to_string(grouped_clobbered_count) +
      " call_args=" + std::to_string(grouped_call_argument_count) +
      " call_results=" + std::to_string(grouped_call_result_count) +
      " spills=" + std::to_string(grouped_spill_count) +
      " reloads=" + std::to_string(grouped_reload_count) +
      " storage=" + std::to_string(grouped_storage_count);
  if (summary.find(expected_summary) == std::string::npos) {
    return fail(
        "x86 route debug contract: summary no longer reports grouped spill/reload authority counts");
  }

  const auto spill_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [&is_grouped](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               is_grouped(op.contiguous_width, op.occupied_register_names);
      });
  if (spill_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail("x86 route debug contract: grouped spill/reload fixture lost grouped spill authority");
  }
  const auto reload_it = std::find_if(
      consumed.regalloc->spill_reload_ops.begin(),
      consumed.regalloc->spill_reload_ops.end(),
      [spill_it](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Reload &&
               op.value_id == spill_it->value_id &&
               op.register_bank == spill_it->register_bank &&
               op.register_name == spill_it->register_name &&
               op.contiguous_width == spill_it->contiguous_width &&
               op.occupied_register_names == spill_it->occupied_register_names &&
               op.slot_id == spill_it->slot_id &&
               op.stack_offset_bytes == spill_it->stack_offset_bytes;
      });
  if (reload_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 route debug contract: grouped spill/reload fixture lost grouped reload authority");
  }

  const auto* spill_value = find_regalloc_value(*consumed.regalloc, spill_it->value_id);
  if (spill_value == nullptr) {
    return fail("x86 route debug contract: grouped spill/reload fixture lost grouped regalloc value authority");
  }

  const auto expected_spill =
      std::string("grouped spill value_id=") + std::to_string(spill_it->value_id) + " value=" +
      std::string(prepare::prepared_value_name(prepared.names, spill_value->value_name)) + " span=" +
      grouped_span_summary(spill_it->register_bank,
                           spill_it->register_name.has_value()
                               ? std::optional<std::string_view>{*spill_it->register_name}
                               : std::nullopt,
                           spill_it->contiguous_width,
                           spill_it->occupied_register_names) +
      " slot_id=#" + std::to_string(*spill_it->slot_id) + " stack_offset=" +
      std::to_string(*spill_it->stack_offset_bytes);
  const auto expected_reload =
      std::string("grouped reload value_id=") + std::to_string(reload_it->value_id) + " value=" +
      std::string(prepare::prepared_value_name(prepared.names, spill_value->value_name)) + " span=" +
      grouped_span_summary(reload_it->register_bank,
                           reload_it->register_name.has_value()
                               ? std::optional<std::string_view>{*reload_it->register_name}
                               : std::nullopt,
                           reload_it->contiguous_width,
                           reload_it->occupied_register_names) +
      " slot_id=#" + std::to_string(*reload_it->slot_id) + " stack_offset=" +
      std::to_string(*reload_it->stack_offset_bytes);
  const auto expected_storage =
      std::string("grouped storage value=carry span=") +
      grouped_span_summary(carry->bank,
                           carry->register_name.has_value()
                               ? std::optional<std::string_view>{*carry->register_name}
                               : std::nullopt,
                           carry->contiguous_width,
                           carry->occupied_register_names);
  if (trace.find(expected_spill) == std::string::npos ||
      trace.find(expected_reload) == std::string::npos ||
      trace.find(expected_storage) == std::string::npos) {
    return fail(
        "x86 route debug contract: trace no longer reads grouped spill/reload/storage authority directly");
  }
  return 0;
}

int check_variadic_nested_dynamic_stack_call_contract() {
  const auto prepared = prepare_module(make_variadic_nested_dynamic_stack_call_module());
  const auto* function = find_function(prepared.module, "variadic_nested_dynamic_stack_contract");
  if (function == nullptr || function->blocks.size() != 4) {
    return fail("variadic nested dynamic-stack contract: expected four semantic BIR blocks");
  }
  if (count_calls_to(*function, "llvm.stacksave") != 2 ||
      count_calls_to(*function, "llvm.dynamic_alloca.i32") != 2 ||
      count_calls_to(*function, "llvm.stackrestore") != 2 ||
      count_calls_to(*function, "extern_variadic_i32") != 1) {
    return fail("variadic nested dynamic-stack contract: expected two stack-save pairs plus one variadic call");
  }

  const auto* liveness = find_liveness_function(prepared, "variadic_nested_dynamic_stack_contract");
  if (liveness == nullptr || liveness->call_points.size() != 7) {
    return fail(
        "variadic nested dynamic-stack contract: expected the variadic call to remain visible beside nested stack-state calls");
  }
  if (find_stack_object(prepared, "lv.fixed") == nullptr) {
    return fail("variadic nested dynamic-stack contract: fixed local disappeared while dynamic stack was active");
  }
  if (find_stack_object(prepared, "outer.buf") != nullptr ||
      find_stack_object(prepared, "inner.buf") != nullptr) {
    return fail("variadic nested dynamic-stack contract: dynamic allocas became fixed stack objects");
  }

  const auto function_id = prepared.names.function_names.find("variadic_nested_dynamic_stack_contract");
  const auto* frame_plan = prepare::find_prepared_frame_plan(prepared, function_id);
  const auto* dynamic_plan = prepare::find_prepared_dynamic_stack_plan(prepared, function_id);
  const auto* call_plans = find_call_plans_function(prepared, "variadic_nested_dynamic_stack_contract");
  const auto* storage_plan =
      find_storage_plan_function(prepared, "variadic_nested_dynamic_stack_contract");
  const auto* carry =
      storage_plan == nullptr ? nullptr : find_storage_value(prepared, *storage_plan, "carry");
  if (frame_plan == nullptr || !frame_plan->has_dynamic_stack ||
      !frame_plan->uses_frame_pointer_for_fixed_slots) {
    return fail("variadic nested dynamic-stack contract: frame_plan lost nested dynamic-stack anchoring");
  }
  if (dynamic_plan == nullptr || !dynamic_plan->requires_stack_save_restore ||
      dynamic_plan->operations.size() != 6) {
    return fail("variadic nested dynamic-stack contract: dynamic_stack_plan lost nested stack-state operations");
  }
  if (call_plans == nullptr || call_plans->calls.size() != 7 || storage_plan == nullptr || carry == nullptr) {
    return fail("variadic nested dynamic-stack contract: missing call-plan or storage-plan publication");
  }

  const auto variadic_it = std::find_if(call_plans->calls.begin(),
                                        call_plans->calls.end(),
                                        [](const prepare::PreparedCallPlan& call_plan) {
                                          return call_plan.direct_callee_name ==
                                                     std::optional<std::string>{"extern_variadic_i32"} &&
                                                 call_plan.wrapper_kind ==
                                                     prepare::PreparedCallWrapperKind::DirectExternVariadic;
                                        });
  if (variadic_it == call_plans->calls.end()) {
    return fail("variadic nested dynamic-stack contract: missing explicit variadic call-plan record");
  }

  const auto& variadic_call = *variadic_it;
  if (variadic_call.block_index != 1 || variadic_call.instruction_index != 4 ||
      variadic_call.variadic_fpr_arg_register_count != 1 || variadic_call.is_indirect ||
      variadic_call.arguments.size() != 2 || !variadic_call.result.has_value()) {
    return fail("variadic nested dynamic-stack contract: variadic call-plan lost wrapper, position, or result authority");
  }
  if (variadic_call.arguments.front().source_value_id !=
          std::optional<prepare::PreparedValueId>{carry->value_id} ||
      variadic_call.arguments.front().source_encoding != carry->encoding ||
      variadic_call.arguments.front().source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{carry->bank} ||
      variadic_call.arguments[1].source_encoding != prepare::PreparedStorageEncodingKind::Immediate ||
      !variadic_call.arguments[1].source_literal.has_value()) {
    return fail("variadic nested dynamic-stack contract: variadic arguments lost scalar source authority");
  }

  const auto preserved_it = std::find_if(
      variadic_call.preserved_values.begin(),
      variadic_call.preserved_values.end(),
      [carry](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == carry->value_id &&
               preserved.value_name == carry->value_name &&
               preserved.route != prepare::PreparedCallPreservationRoute::Unknown;
      });
  if (preserved_it == variadic_call.preserved_values.end()) {
    return fail("variadic nested dynamic-stack contract: call-plan no longer publishes the live scalar preserved across the variadic call");
  }

  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find("callsite block=1 inst=4 wrapper=direct_extern_variadic callee=extern_variadic_i32 variadic_fpr_args=1") ==
          std::string::npos ||
      prepared_dump.find(
          "prepared.func @variadic_nested_dynamic_stack_contract requires_stack_save_restore=yes") ==
          std::string::npos) {
    return fail("variadic nested dynamic-stack contract: prepared dump no longer exposes both variadic and dynamic-stack authority");
  }

  return 0;
}

lir::LirModule make_aarch64_f128_hfa_va_arg_metadata_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls.push_back("%struct.F128Hfa = type { fp128, fp128 }");
  module.type_decls.push_back("%struct.NonHfa32 = type { i64, i64, i64, i64 }");

  const c4c::StructNameId hfa_id = module.struct_names.intern("%struct.F128Hfa");
  const c4c::StructNameId non_hfa_id = module.struct_names.intern("%struct.NonHfa32");
  const lir::LirTypeRef hfa_ref = lir::LirTypeRef::struct_type("%struct.F128Hfa", hfa_id);
  const lir::LirTypeRef non_hfa_ref =
      lir::LirTypeRef::struct_type("%struct.NonHfa32", non_hfa_id);
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = hfa_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("fp128")},
                 lir::LirStructField{lir::LirTypeRef("fp128")}},
  });
  module.record_struct_decl(lir::LirStructDecl{
      .name_id = non_hfa_id,
      .fields = {lir::LirStructField{lir::LirTypeRef("i64")},
                 lir::LirStructField{lir::LirTypeRef("i64")},
                 lir::LirStructField{lir::LirTypeRef("i64")},
                 lir::LirStructField{lir::LirTypeRef("i64")}},
  });

  lir::LirFunction function;
  function.name = "aarch64_f128_hfa_va_arg_metadata";
  function.signature_text = "define void @aarch64_f128_hfa_va_arg_metadata(ptr %ap)";
  function.params.emplace_back("%ap", c4c::TypeSpec{.base = c4c::TB_VOID, .ptr_level = 1});

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirVaArgOp{
      .result = lir::LirOperand("%hfa"),
      .ap_ptr = lir::LirOperand("%ap"),
      .type_str = hfa_ref,
  });
  entry.insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.hfa"),
      .type_str = hfa_ref,
      .align = 16,
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = hfa_ref,
      .val = lir::LirOperand("%hfa"),
      .ptr = lir::LirOperand("%lv.hfa"),
  });
  entry.insts.push_back(lir::LirVaArgOp{
      .result = lir::LirOperand("%plain"),
      .ap_ptr = lir::LirOperand("%ap"),
      .type_str = non_hfa_ref,
  });
  entry.insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.plain"),
      .type_str = non_hfa_ref,
      .align = 16,
  });
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = non_hfa_ref,
      .val = lir::LirOperand("%plain"),
      .ptr = lir::LirOperand("%lv.plain"),
  });
  entry.terminator = lir::LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int check_aarch64_f128_hfa_va_arg_metadata_contract() {
  auto lowered = c4c::backend::try_lower_to_bir_with_options(
      make_aarch64_f128_hfa_va_arg_metadata_module(), c4c::backend::BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return fail("AAPCS64 F128 HFA va_arg metadata contract: LIR fixture did not lower");
  }
  const auto* function = find_function(*lowered.module, "aarch64_f128_hfa_va_arg_metadata");
  if (function == nullptr) {
    return fail("AAPCS64 F128 HFA va_arg metadata contract: missing lowered function");
  }
  const auto* hfa_call = find_call_with_destination(*function, "%hfa");
  const auto* plain_call = find_call_with_destination(*function, "%plain");
  if (hfa_call == nullptr || plain_call == nullptr) {
    return fail("AAPCS64 F128 HFA va_arg metadata contract: missing aggregate helpers");
  }
  if (hfa_call->arg_abi.size() != 2 ||
      !hfa_call->arg_abi[0].sret_pointer ||
      hfa_call->arg_abi[0].size_bytes != 32 ||
      hfa_call->arg_abi[0].align_bytes != 16 ||
      !hfa_call->va_arg_payload_abi.has_value() ||
      !hfa_call->va_arg_payload_abi->sret_pointer ||
      hfa_call->va_arg_payload_abi->size_bytes != 32 ||
      hfa_call->va_arg_payload_abi->align_bytes != 16 ||
      hfa_call->va_arg_hfa_lane_count != 2 ||
      hfa_call->va_arg_hfa_lane_size_bytes != 16) {
    return fail("AAPCS64 F128 HFA va_arg metadata contract: missing F128 lane facts");
  }
  if (plain_call->va_arg_hfa_lane_count != 0 ||
      plain_call->va_arg_hfa_lane_size_bytes != 0) {
    return fail("AAPCS64 F128 HFA va_arg metadata contract: non-floating aggregate gained HFA lane facts");
  }
  return 0;
}

bir::Module make_aapcs64_variadic_entry_helper_family_frame_module() {
  bir::Module module;
  module.target_triple = "aarch64-unknown-linux-gnu";

  bir::Function function;
  function.name = "aapcs64_variadic_entry_helper_family_frame_contract";
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
      .va_arg_payload_abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::F64, "next.f64"),
      .callee = "llvm.va_arg.f64",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "double",
      .return_type = bir::TypeKind::F64,
      .va_arg_payload_abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::F64,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Sse,
      },
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
      .va_arg_payload_abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      },
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
  return module;
}

int check_aapcs64_variadic_entry_helper_family_frame_contract() {
  const auto prepared = prepare::prepare_semantic_bir_module_with_options(
      make_aapcs64_variadic_entry_helper_family_frame_module(),
      c4c::target_profile_from_triple("aarch64-unknown-linux-gnu"),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
  const auto function_id =
      prepared.names.function_names.find("aapcs64_variadic_entry_helper_family_frame_contract");
  const auto* entry_plan = prepare::find_prepared_variadic_entry_plan(prepared, function_id);
  const auto* call_plans =
      find_call_plans_function(prepared, "aapcs64_variadic_entry_helper_family_frame_contract");
  if (entry_plan == nullptr || call_plans == nullptr || call_plans->calls.size() != 5) {
    return fail("AAPCS64 variadic helper-family frame contract: missing carrier or helper calls");
  }
  if (entry_plan->helper_resources.required_helpers.size() != 4 ||
      entry_plan->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{2} ||
      entry_plan->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      entry_plan->helper_operand_homes.size() != 5 ||
      entry_plan->register_save_area.size_bytes != std::optional<std::size_t>{192} ||
      !entry_plan->register_save_area.slot_id.has_value() ||
      !entry_plan->register_save_area.stack_offset_bytes.has_value() ||
      entry_plan->register_save_area.saved_gp_register_count !=
          std::optional<std::size_t>{7} ||
      entry_plan->register_save_area.saved_fp_register_count !=
          std::optional<std::size_t>{7} ||
      !entry_plan->overflow_area.base_slot_id.has_value() ||
      !entry_plan->overflow_area.base_stack_offset_bytes.has_value() ||
      entry_plan->overflow_area.align_bytes != std::optional<std::size_t>{8}) {
    return fail("AAPCS64 variadic helper-family frame contract: lost prepared entry scratch or storage facts");
  }
  const auto* aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 3);
  const auto* copy_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 4);
  const auto* va_arg_i32_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 1);
  const auto* va_arg_f64_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 2);
  if (aggregate_homes == nullptr || copy_homes == nullptr ||
      va_arg_i32_homes == nullptr || va_arg_f64_homes == nullptr ||
      !aggregate_homes->aggregate_destination_payload.has_value() ||
      !aggregate_homes->source_va_list.has_value() ||
      !copy_homes->destination_va_list.has_value() ||
      !copy_homes->source_va_list.has_value()) {
    return fail("AAPCS64 variadic helper-family frame contract: lost aggregate va_arg or va_copy operand homes");
  }
  if (!aggregate_homes->aggregate_access_plan.has_value() ||
      aggregate_homes->aggregate_access_plan->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      aggregate_homes->aggregate_access_plan->payload_size_bytes != 8 ||
      aggregate_homes->aggregate_access_plan->payload_align_bytes != 4 ||
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
    return fail("AAPCS64 variadic helper-family frame contract: lost aggregate va_arg access-plan carrier facts");
  }
  if (!va_arg_i32_homes->scalar_access_plan.has_value() ||
      va_arg_i32_homes->scalar_access_plan->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::GpRegisterSaveArea ||
      va_arg_i32_homes->scalar_access_plan->value_size_bytes != 4 ||
      va_arg_i32_homes->scalar_access_plan->value_align_bytes != 4 ||
      va_arg_i32_homes->scalar_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::GpRegisterSaveArea} ||
      va_arg_i32_homes->scalar_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::GpOffset} ||
      va_arg_i32_homes->scalar_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{8} ||
      !va_arg_i32_homes->scalar_access_plan->result_home.has_value() ||
      !va_arg_f64_homes->scalar_access_plan.has_value() ||
      va_arg_f64_homes->scalar_access_plan->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::FpRegisterSaveArea ||
      va_arg_f64_homes->scalar_access_plan->value_size_bytes != 8 ||
      va_arg_f64_homes->scalar_access_plan->value_align_bytes != 8 ||
      va_arg_f64_homes->scalar_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::FpRegisterSaveArea} ||
      va_arg_f64_homes->scalar_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::FpOffset} ||
      va_arg_f64_homes->scalar_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{16} ||
      !va_arg_f64_homes->scalar_access_plan->result_home.has_value()) {
    return fail("AAPCS64 variadic helper-family frame contract: lost scalar va_arg access-plan carrier facts");
  }
  const auto* typed_va_arg_i32_homes =
      prepare::find_prepared_variadic_scalar_va_arg_operand_homes(
          *va_arg_i32_homes);
  const auto* typed_va_arg_f64_homes =
      prepare::find_prepared_variadic_scalar_va_arg_operand_homes(
          *va_arg_f64_homes);
  if (typed_va_arg_i32_homes == nullptr ||
      typed_va_arg_i32_homes->scalar_access_plan.value_size_bytes != 4 ||
      typed_va_arg_f64_homes == nullptr ||
      typed_va_arg_f64_homes->scalar_access_plan.value_size_bytes != 8) {
    return fail("AAPCS64 variadic helper-family frame contract: scalar va_arg typed payloads were not published");
  }
  const auto* register_save_slot =
      prepare::find_prepared_frame_slot(prepared.stack_layout,
                                        *entry_plan->register_save_area.slot_id);
  const auto* overflow_base_slot =
      prepare::find_prepared_frame_slot(prepared.stack_layout,
                                        *entry_plan->overflow_area.base_slot_id);
  if (register_save_slot == nullptr ||
      overflow_base_slot == nullptr ||
      register_save_slot->offset_bytes != *entry_plan->register_save_area.stack_offset_bytes ||
      register_save_slot->size_bytes != *entry_plan->register_save_area.size_bytes ||
      overflow_base_slot->offset_bytes != *entry_plan->overflow_area.base_stack_offset_bytes) {
    return fail("AAPCS64 variadic helper-family frame contract: storage facts lost frame-slot authority");
  }
  for (const auto& call_plan : call_plans->calls) {
    if (call_plan.variadic_fpr_arg_register_count != 0 ||
        call_plan.wrapper_kind == prepare::PreparedCallWrapperKind::DirectExternVariadic) {
      return fail("AAPCS64 variadic helper-family frame contract: helper calls leaked call-boundary variadic metadata");
    }
  }
  return 0;
}

int check_rv64_variadic_entry_helper_missing_contract() {
  const auto prepared = prepare::prepare_semantic_bir_module_with_options(
      make_aapcs64_variadic_entry_helper_family_frame_module(),
      riscv_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
  const auto function_id =
      prepared.names.function_names.find("aapcs64_variadic_entry_helper_family_frame_contract");
  const auto* entry_plan = prepare::find_prepared_variadic_entry_plan(prepared, function_id);
  if (entry_plan == nullptr) {
    return fail("RV64 variadic helper missing contract: missing entry plan");
  }
  const auto has_missing_fact = [&](std::string_view fact) {
    return std::find(entry_plan->missing_required_facts.begin(),
                     entry_plan->missing_required_facts.end(),
                     fact) != entry_plan->missing_required_facts.end();
  };
  if (entry_plan->helper_resources.required_helpers.size() != 4 ||
      entry_plan->helper_resources.scratch_register_count !=
          std::optional<std::size_t>{3} ||
      entry_plan->helper_resources.scratch_stack_bytes !=
          std::optional<std::size_t>{0} ||
      entry_plan->helper_operand_homes.size() != 5 ||
      entry_plan->register_save_area.required ||
      !entry_plan->overflow_area.required ||
      entry_plan->overflow_area.align_bytes != std::optional<std::size_t>{8} ||
      !entry_plan->overflow_area.base_slot_id.has_value() ||
      !entry_plan->overflow_area.base_stack_offset_bytes.has_value() ||
      !entry_plan->va_list_layout.required ||
      entry_plan->va_list_layout.size_bytes != std::optional<std::size_t>{8} ||
      entry_plan->va_list_layout.align_bytes != std::optional<std::size_t>{8} ||
      entry_plan->va_list_layout.fields.size() != 1 ||
      entry_plan->va_list_layout.fields.front().kind !=
          prepare::PreparedVariadicVaListFieldKind::OverflowArgArea ||
      entry_plan->va_list_layout.fields.front().offset_bytes != 0 ||
      entry_plan->va_list_layout.fields.front().size_bytes != 8) {
    return fail("RV64 variadic helper missing contract: target ABI facts were not published");
  }
  const auto* va_start_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 0);
  if (va_start_homes == nullptr ||
      !prepare::has_complete_prepared_variadic_va_start_operand_homes(*va_start_homes)) {
    return fail("RV64 variadic helper missing contract: va_start operand homes were not materialized");
  }
  const auto* typed_va_start_homes =
      prepare::find_prepared_variadic_va_start_operand_homes(*va_start_homes);
  if (typed_va_start_homes == nullptr) {
    return fail("RV64 variadic helper missing contract: va_start typed operand homes were not published");
  }
  if (va_start_homes->destination_va_list->kind !=
          prepare::PreparedValueHomeKind::StackSlot ||
      !va_start_homes->destination_va_list->slot_id.has_value() ||
      va_start_homes->destination_va_list_address->kind !=
          prepare::PreparedValueHomeKind::Register ||
      !va_start_homes->destination_va_list_address->register_name.has_value()) {
    return fail("RV64 variadic helper missing contract: va_start storage and address homes were not distinguished");
  }
  prepare::PreparedVariadicEntryHelperOperandHomes mismatched_optional_va_start{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaArg,
      .destination_va_list = *va_start_homes->destination_va_list,
      .destination_va_list_address = *va_start_homes->destination_va_list_address,
  };
  prepare::publish_prepared_variadic_va_start_operand_homes(
      mismatched_optional_va_start);
  prepare::PreparedVariadicEntryHelperOperandHomes incomplete_optional_va_start{
      .helper = prepare::PreparedVariadicEntryHelperKind::VaStart,
      .destination_va_list = *va_start_homes->destination_va_list,
  };
  prepare::publish_prepared_variadic_va_start_operand_homes(
      incomplete_optional_va_start);
  if (prepare::find_prepared_variadic_va_start_operand_homes(
          mismatched_optional_va_start) != nullptr ||
      prepare::has_complete_prepared_variadic_va_start_operand_homes(
          mismatched_optional_va_start) ||
      prepare::find_prepared_variadic_va_start_operand_homes(
          incomplete_optional_va_start) != nullptr ||
      prepare::has_complete_prepared_variadic_va_start_operand_homes(
          incomplete_optional_va_start)) {
    return fail("RV64 variadic helper missing contract: invalid optional-bag combinations looked like typed va_start payloads");
  }
  const auto* overflow_base_slot =
      prepare::find_prepared_frame_slot(prepared.stack_layout,
                                        *entry_plan->overflow_area.base_slot_id);
  const auto* overflow_base_object =
      overflow_base_slot == nullptr
          ? nullptr
          : find_stack_object(prepared, "__rv64_variadic_overflow_area_base");
  if (overflow_base_slot == nullptr ||
      overflow_base_object == nullptr ||
      overflow_base_object->object_id != overflow_base_slot->object_id ||
      overflow_base_object->source_kind != "rv64_variadic_overflow_area_base" ||
      overflow_base_slot->align_bytes != 8 ||
      overflow_base_slot->offset_bytes !=
          *entry_plan->overflow_area.base_stack_offset_bytes ||
      overflow_base_slot->offset_bytes % 8 != 0) {
    return fail("RV64 variadic helper missing contract: overflow base did not publish frame-slot authority");
  }
  if (!entry_plan->named_register_counts.gp.has_value() ||
      *entry_plan->named_register_counts.gp > 8 ||
      overflow_base_slot->size_bytes !=
          (8 - *entry_plan->named_register_counts.gp) * 8 ||
      entry_plan->rv64_incoming_variadic_gpr_publications.size() !=
          8 - *entry_plan->named_register_counts.gp) {
    return fail("RV64 variadic helper missing contract: incoming GPR publication authority was not prepared");
  }
  for (const auto& publication :
       entry_plan->rv64_incoming_variadic_gpr_publications) {
    if (publication.destination_slot_id !=
            *entry_plan->overflow_area.base_slot_id ||
        publication.destination_stack_offset_bytes < overflow_base_slot->offset_bytes ||
        publication.destination_stack_offset_bytes + publication.size_bytes >
            overflow_base_slot->offset_bytes + overflow_base_slot->size_bytes ||
        publication.size_bytes != 8 ||
        publication.align_bytes != 8) {
      return fail("RV64 variadic helper missing contract: incoming GPR publication did not target the overflow backing slot");
    }
  }
  const auto* aggregate_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 3);
  const auto* va_arg_i32_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 1);
  const auto* va_arg_f64_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 2);
  const auto* va_copy_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 4);
  if (aggregate_homes == nullptr ||
      !aggregate_homes->aggregate_destination_payload.has_value() ||
      !aggregate_homes->source_va_list.has_value() ||
      !aggregate_homes->aggregate_access_plan.has_value() ||
      aggregate_homes->aggregate_access_plan->source_class !=
          prepare::PreparedVariadicAggregateVaArgSourceClass::OverflowArgArea ||
      aggregate_homes->aggregate_access_plan->payload_size_bytes != 8 ||
      aggregate_homes->aggregate_access_plan->payload_align_bytes != 4 ||
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
    return fail("RV64 variadic helper missing contract: aggregate va_arg overflow operand homes were not materialized");
  }
  if (has_missing_fact("target_abi.variadic_entry_state") ||
      has_missing_fact("target_abi.va_list_layout") ||
      has_missing_fact("helper_resources.scratch_register_count") ||
      has_missing_fact("helper_resources.scratch_stack_bytes") ||
      has_missing_fact("helper_operand_homes.va_start.destination_va_list") ||
      has_missing_fact("helper_operand_homes.va_start.destination_va_list_address") ||
      has_missing_fact("helper_operand_homes.va_arg.source_va_list") ||
      has_missing_fact("helper_operand_homes.va_arg.scalar_result") ||
      has_missing_fact("helper_operand_homes.va_arg.scalar_access_plan") ||
      !has_missing_fact("target_abi.va_arg.scalar_payload_abi") ||
      has_missing_fact("helper_operand_homes.va_arg_aggregate.source_va_list") ||
      has_missing_fact(
          "helper_operand_homes.va_arg_aggregate.aggregate_destination_payload") ||
      has_missing_fact("helper_operand_homes.va_arg_aggregate.aggregate_access_plan") ||
      has_missing_fact("target_abi.va_arg_aggregate.payload_abi") ||
      has_missing_fact("helper_operand_homes.va_copy.destination_va_list") ||
      has_missing_fact("helper_operand_homes.va_copy.source_va_list")) {
    return fail("RV64 variadic helper missing contract: missing explicit helper facts or retained target ABI facts");
  }
  if (has_missing_fact("rv64") ||
      has_missing_fact("aapcs64_variadic_entry_helper_family_frame_contract")) {
    return fail("RV64 variadic helper missing contract: missing facts became target or testcase shaped");
  }
  if (va_arg_i32_homes == nullptr ||
      !va_arg_i32_homes->source_va_list.has_value() ||
      !va_arg_i32_homes->scalar_result.has_value() ||
      !va_arg_i32_homes->scalar_access_plan.has_value() ||
      va_arg_i32_homes->scalar_access_plan->source_class !=
          prepare::PreparedVariadicScalarVaArgSourceClass::OverflowArgArea ||
      va_arg_i32_homes->scalar_access_plan->value_size_bytes != 4 ||
      va_arg_i32_homes->scalar_access_plan->value_align_bytes != 4 ||
      va_arg_i32_homes->scalar_access_plan->source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      va_arg_i32_homes->scalar_access_plan->source_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      va_arg_i32_homes->scalar_access_plan->source_slot_size_bytes !=
          std::optional<std::size_t>{4} ||
      va_arg_i32_homes->scalar_access_plan->progression_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      va_arg_i32_homes->scalar_access_plan->progression_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      va_arg_i32_homes->scalar_access_plan->progression_stride_bytes !=
          std::optional<std::size_t>{4} ||
      va_arg_i32_homes->scalar_access_plan->overflow_source_field !=
          std::optional<prepare::PreparedVariadicVaListFieldKind>{
              prepare::PreparedVariadicVaListFieldKind::OverflowArgArea} ||
      va_arg_i32_homes->scalar_access_plan->overflow_source_field_offset_bytes !=
          std::optional<std::size_t>{0} ||
      va_arg_i32_homes->scalar_access_plan->overflow_stride_bytes !=
          std::optional<std::size_t>{4} ||
      va_arg_f64_homes == nullptr ||
      !va_arg_f64_homes->source_va_list.has_value() ||
      !va_arg_f64_homes->scalar_result.has_value() ||
      va_arg_f64_homes->scalar_access_plan.has_value()) {
    return fail("RV64 variadic helper missing contract: scalar va_arg facts were not consumed or precisely diagnosed");
  }
  const auto* typed_rv64_va_arg_i32_homes =
      prepare::find_prepared_variadic_scalar_va_arg_operand_homes(
          *va_arg_i32_homes);
  if (typed_rv64_va_arg_i32_homes == nullptr ||
      typed_rv64_va_arg_i32_homes->scalar_access_plan.value_size_bytes != 4 ||
      prepare::find_prepared_variadic_scalar_va_arg_operand_homes(
          *va_arg_f64_homes) != nullptr) {
    return fail("RV64 variadic helper missing contract: scalar va_arg typed payload completeness regressed");
  }
  if (va_copy_homes == nullptr ||
      !prepare::has_complete_prepared_variadic_va_copy_operand_homes(*va_copy_homes)) {
    return fail("RV64 variadic helper missing contract: va_copy operand homes were not materialized");
  }
  return 0;
}

bir::Module make_rv64_variadic_entry_frame_slot_va_start_module() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function function;
  function.name = "rv64_variadic_entry_frame_slot_va_start_contract";
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
      .result = bir::Value::named(bir::TypeKind::Ptr, "heap.ap"),
      .callee = "malloc",
      .args = {bir::Value::immediate_i64(8)},
      .arg_types = {bir::TypeKind::I64},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "clobber.ap"),
      .callee = "malloc",
      .args = {bir::Value::immediate_i64(8)},
      .arg_types = {bir::TypeKind::I64},
      .return_type_name = "ptr",
      .return_type = bir::TypeKind::Ptr,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_start.p0",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "heap.ap")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "head")};
  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int check_rv64_variadic_entry_frame_slot_va_start_address_contract() {
  const auto prepared = prepare::prepare_semantic_bir_module_with_options(
      make_rv64_variadic_entry_frame_slot_va_start_module(),
      riscv_target_profile(),
      prepare::PrepareOptions{
          .run_legalize = true,
          .run_stack_layout = true,
          .run_liveness = true,
          .run_regalloc = true,
      });
  const auto function_id =
      prepared.names.function_names.find("rv64_variadic_entry_frame_slot_va_start_contract");
  const auto* entry_plan = prepare::find_prepared_variadic_entry_plan(prepared, function_id);
  if (entry_plan == nullptr ||
      entry_plan->helper_resources.required_helpers.size() != 1 ||
      entry_plan->helper_resources.required_helpers.front() !=
          prepare::PreparedVariadicEntryHelperKind::VaStart ||
      entry_plan->helper_operand_homes.size() != 2) {
    return fail("RV64 frame-slot va_start address contract: missing helper homes");
  }
  const auto has_missing_fact = [&](std::string_view fact) {
    return std::find(entry_plan->missing_required_facts.begin(),
                     entry_plan->missing_required_facts.end(),
                     fact) != entry_plan->missing_required_facts.end();
  };
  if (has_missing_fact("helper_operand_homes.va_start.destination_va_list") ||
      has_missing_fact("helper_operand_homes.va_start.destination_va_list_address")) {
    return fail("RV64 frame-slot va_start address contract: retained missing va_start helper facts");
  }

  const auto* register_va_start_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 0);
  const auto* frame_slot_va_start_homes =
      prepare::find_prepared_variadic_entry_helper_operand_homes(*entry_plan, 0, 3);
  if (register_va_start_homes == nullptr ||
      frame_slot_va_start_homes == nullptr ||
      !prepare::has_complete_prepared_variadic_va_start_operand_homes(
          *register_va_start_homes) ||
      !prepare::has_complete_prepared_variadic_va_start_operand_homes(
          *frame_slot_va_start_homes)) {
    return fail("RV64 frame-slot va_start address contract: incomplete va_start operand homes");
  }
  if (register_va_start_homes->destination_va_list_address->kind !=
          prepare::PreparedValueHomeKind::Register ||
      !register_va_start_homes->destination_va_list_address->register_name.has_value()) {
    return fail("RV64 frame-slot va_start address contract: register-backed address path regressed");
  }
  if (frame_slot_va_start_homes->destination_va_list_address->kind !=
          prepare::PreparedValueHomeKind::StackSlot ||
      !frame_slot_va_start_homes->destination_va_list_address->slot_id.has_value() ||
      !frame_slot_va_start_homes->destination_va_list_address->offset_bytes.has_value() ||
      !frame_slot_va_start_homes->destination_va_list_address->size_bytes.has_value() ||
      !frame_slot_va_start_homes->destination_va_list_address->align_bytes.has_value() ||
      frame_slot_va_start_homes->destination_va_list_address->value_name !=
          frame_slot_va_start_homes->destination_va_list->value_name ||
      frame_slot_va_start_homes->destination_va_list_address->slot_id ==
          frame_slot_va_start_homes->destination_va_list->slot_id) {
    return fail("RV64 frame-slot va_start address contract: frame-slot address home was not published distinctly");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int rc = check_aarch64_scalar_parameter_homes_and_storage_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_fixed_frame_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_call_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_same_module_i16_call_argument_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_float_call_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_stack_result_slot_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_indirect_call_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_memory_return_call_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_call_wrapper_kind_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_call_wrapper_link_name_id_authority_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_call_argument_source_shape_contract(); rc != 0) {
    return rc;
  }
  if (const int rc =
          check_call_argument_source_producer_materializability_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_direct_global_select_chain_call_argument_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_call_argument_symbol_link_name_id_mismatch_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_f128_symbol_backed_load_local_addressing_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_stack_argument_slot_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_cross_call_preservation_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_prior_preservation_source_selection_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_aarch64_formal_preservation_source_endpoint_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_riscv_fpr_formal_home_publishes_target_identity(
          "riscv64gc-unknown-linux-gnu");
      rc != 0) {
    return rc;
  }
  if (const int rc = check_riscv_fpr_formal_home_publishes_target_identity(
          "riscv64-linux-gnu");
      rc != 0) {
    return rc;
  }
  if (const int rc = check_riscv_i16_formal_home_publishes_gpr_identity();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_local_frame_address_source_selection_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_derived_local_frame_address_source_selection_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_byval_register_lane_aggregate_transport_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc =
          check_aarch64_global_byval_register_lane_source_selection_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc =
          check_missing_local_aggregate_frame_slot_address_source_selection_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_frame_slot_address_source_route_bridge_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_frame_slot_value_source_route_bridge_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc =
          check_local_frame_address_materialization_route_bridge_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_rv64_same_module_byval_stack_copy_call_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc =
          check_rv64_same_module_byval_stack_copy_contract_fails_closed();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_aarch64_prior_preservation_consumes_prepared_source_selection();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_aarch64_prior_preservation_rejects_missing_source_selection();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_aarch64_outgoing_stack_scalar_argument_lifetime_contract();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_saved_register_slot_placement_carrier_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_stack_cross_call_preservation_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_grouped_cross_call_preservation_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_dynamic_stack_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_dynamic_stack_plan_reads_authoritative_block_label_id(); rc != 0) {
    return rc;
  }
  if (const int rc = check_cross_block_dynamic_stack_cfg_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_nested_dynamic_stack_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_branch_restore_dynamic_stack_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_consumer_surface_reads_scalar_call_boundary_authority(); rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_consumer_surface_reads_memory_return_authority(); rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_consumer_surface_reads_nested_dynamic_stack_call_boundary_authority();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_consumer_surface_reads_grouped_call_boundary_authority(); rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_consumer_surface_reads_grouped_spill_reload_authority(); rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_consumer_surface_reads_general_grouped_spill_reload_authority();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_consumer_surface_reads_float_grouped_spill_reload_authority();
      rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_module_emitter_reads_grouped_call_boundary_authority(); rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_module_emitter_reads_grouped_spill_reload_authority(); rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_route_debug_reads_grouped_call_boundary_authority(); rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_route_debug_reads_grouped_spill_reload_authority(); rc != 0) {
    return rc;
  }
  if (const int rc = check_variadic_nested_dynamic_stack_call_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_aarch64_f128_hfa_va_arg_metadata_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_aapcs64_variadic_entry_helper_family_frame_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_rv64_variadic_entry_helper_missing_contract(); rc != 0) {
    return rc;
  }
  if (const int rc =
          check_rv64_variadic_entry_frame_slot_va_start_address_contract();
      rc != 0) {
    return rc;
  }
  return EXIT_SUCCESS;
}
