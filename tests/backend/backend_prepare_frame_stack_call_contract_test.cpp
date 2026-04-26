#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/x86/x86.hpp"
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
#include "src/target_profile.hpp"

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

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
  if (call_plan.result->value_bank != prepare::PreparedRegisterBank::Gpr ||
      call_plan.result->source_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !call_plan.result->source_register_name.has_value() ||
      call_plan.result->source_register_bank != prepare::PreparedRegisterBank::Gpr) {
    return fail("call contract: call_plans lost integer result ABI source");
  }
  if (call_plan.clobbered_registers.empty()) {
    return fail("call contract: call_plans lost the clobber contract");
  }
  for (const auto& clobber : call_plan.clobbered_registers) {
    if (clobber.contiguous_width != 1 || clobber.occupied_register_names.size() != 1 ||
        clobber.occupied_register_names.front() != clobber.register_name) {
      return fail("call contract: clobber metadata no longer publishes singleton occupancy");
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
  if (call_plan.result->destination_value_id !=
      std::optional<prepare::PreparedValueId>{tmp_call->value_id}) {
    return fail("call contract: call_plans lost direct integer result source identity");
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
  if (result.destination_value_id != std::optional<prepare::PreparedValueId>{spilled_result->value_id}) {
    return fail("stack-result slot contract: call_plans lost direct spilled-result source identity");
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
      *symbol_arg.source_symbol_name != "@extern_data") {
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

  return 0;
}

int check_stack_cross_call_preservation_contract() {
  const auto prepared = prepare_module(make_stack_cross_call_preservation_contract_module());
  const auto* call_plans =
      find_call_plans_function(prepared, "stack_cross_call_preservation_contract");
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
               op.register_name == spill_it->register_name &&
               op.contiguous_width == spill_it->contiguous_width &&
               op.occupied_register_names == spill_it->occupied_register_names &&
               op.slot_id == spill_it->slot_id &&
               op.stack_offset_bytes == spill_it->stack_offset_bytes;
      });
  if (reload_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped spill/reload fixture lost reload authority for the spilled grouped value");
  }

  const auto* spill_value = find_regalloc_value(*consumed.regalloc, spill_it->value_id);
  if (spill_value == nullptr || !spill_it->slot_id.has_value() || !spill_it->stack_offset_bytes.has_value() ||
      !spill_value->assigned_stack_slot.has_value() ||
      spill_value->assigned_stack_slot->slot_id != *spill_it->slot_id ||
      spill_value->assigned_stack_slot->offset_bytes != *spill_it->stack_offset_bytes) {
    return fail(
        "x86 consumer surface contract: grouped spill/reload fixture lost direct regalloc value authority for the grouped spill");
  }
  if (spill_value->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      spill_value->assigned_register.has_value() || !spill_value->spill_register_authority.has_value() ||
      spill_value->spill_register_authority->register_name != spill_it->register_name ||
      spill_value->spill_register_authority->reg_class != prepare::PreparedRegisterClass::Vector ||
      spill_value->spill_register_authority->contiguous_width != spill_it->contiguous_width ||
      spill_value->spill_register_authority->occupied_register_names !=
          spill_it->occupied_register_names) {
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
               op.register_name == spill_it->register_name &&
               op.contiguous_width == spill_it->contiguous_width &&
               op.occupied_register_names == spill_it->occupied_register_names &&
               op.slot_id == spill_it->slot_id &&
               op.stack_offset_bytes == spill_it->stack_offset_bytes;
      });
  if (reload_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped general spill/reload fixture lost reload authority for the spilled grouped value");
  }

  const auto* spill_value = find_regalloc_value(*consumed.regalloc, spill_it->value_id);
  if (spill_value == nullptr || !spill_it->slot_id.has_value() || !spill_it->stack_offset_bytes.has_value() ||
      !spill_value->assigned_stack_slot.has_value() ||
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
      spill_value->spill_register_authority->contiguous_width != spill_it->contiguous_width ||
      spill_value->spill_register_authority->occupied_register_names !=
          spill_it->occupied_register_names) {
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
               op.register_name == spill_it->register_name &&
               op.contiguous_width == spill_it->contiguous_width &&
               op.occupied_register_names == spill_it->occupied_register_names &&
               op.slot_id == spill_it->slot_id &&
               op.stack_offset_bytes == spill_it->stack_offset_bytes;
      });
  if (reload_it == consumed.regalloc->spill_reload_ops.end()) {
    return fail(
        "x86 consumer surface contract: grouped float spill/reload fixture lost reload authority for the spilled grouped value");
  }

  const auto* spill_value = find_regalloc_value(*consumed.regalloc, spill_it->value_id);
  if (spill_value == nullptr || !spill_it->slot_id.has_value() || !spill_it->stack_offset_bytes.has_value() ||
      !spill_value->assigned_stack_slot.has_value() ||
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
      spill_value->spill_register_authority->contiguous_width != spill_it->contiguous_width ||
      spill_value->spill_register_authority->occupied_register_names !=
          spill_it->occupied_register_names) {
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

}  // namespace

int main() {
  if (const int rc = check_fixed_frame_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_call_contract(); rc != 0) {
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
  if (const int rc = check_call_argument_source_shape_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_stack_argument_slot_contract(); rc != 0) {
    return rc;
  }
  if (const int rc = check_cross_call_preservation_contract(); rc != 0) {
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
  return EXIT_SUCCESS;
}
