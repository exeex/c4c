#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir/lowering.hpp"
#include "src/backend/mir/x86/abi/abi.hpp"
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

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

c4c::FunctionNameId find_function_name_id(const prepare::PreparedBirModule& prepared,
                                          std::string_view function_name) {
  return prepared.names.function_names.find(function_name);
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

bir::Module make_x86_param_passthrough_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "id_i32";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "p.x"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

std::string minimal_i32_return_register() {
  const auto abi =
      c4c::backend::lir_to_bir_detail::compute_function_return_abi(x86_target_profile(),
                                                                   bir::TypeKind::I32,
                                                                   false);
  if (!abi.has_value()) {
    throw std::runtime_error("missing canonical i32 return ABI for x86 handoff test");
  }
  const auto wide_register =
      prepare::call_result_destination_register_name(x86_target_profile(), *abi);
  if (!wide_register.has_value()) {
    throw std::runtime_error("missing canonical i32 return register for x86 handoff test");
  }
  return c4c::backend::x86::abi::narrow_i32_register_name(*wide_register);
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
}

std::string expected_minimal_local_i32_immediate_guard_asm(const char* function_name,
                                                           int immediate) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov DWORD PTR [rsp], " + std::to_string(immediate) + "\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    cmp eax, " + std::to_string(immediate) + "\n"
         "    je .L" + function_name + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, 16\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, 16\n"
         "    ret\n";
}

std::string expected_minimal_local_i32_immediate_guard_asm_with_frame_size(const char* function_name,
                                                                           int immediate,
                                                                           int frame_size) {
  return asm_header(function_name) + "    sub rsp, " + std::to_string(frame_size) + "\n"
         "    mov DWORD PTR [rsp], " + std::to_string(immediate) + "\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    cmp eax, " + std::to_string(immediate) + "\n"
         "    je .L" + function_name + "_block_2\n"
         "    mov eax, 1\n"
         "    add rsp, " + std::to_string(frame_size) + "\n"
         "    ret\n"
         ".L" + function_name + "_block_2:\n"
         "    mov eax, 0\n"
         "    add rsp, " + std::to_string(frame_size) + "\n"
         "    ret\n";
}

std::string expected_minimal_local_i32_return_asm(const char* function_name, int immediate) {
  return asm_header(function_name) + "    sub rsp, 16\n"
         "    mov DWORD PTR [rsp], " + std::to_string(immediate) + "\n"
         "    mov eax, DWORD PTR [rsp]\n"
         "    add rsp, 16\n"
         "    ret\n";
}

bir::Module make_x86_local_i32_immediate_guard_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
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
      .slot_name = "%lv.x",
      .value = bir::Value::immediate_i32(123),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .slot_name = "%lv.x",
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
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_x86_local_i32_return_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.ret",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.ret",
      .value = bir::Value::immediate_i32(123),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .slot_name = "%lv.ret",
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "loaded"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

int check_route_outputs(const bir::Module& module,
                        const std::string& expected_asm,
                        const std::string& expected_bir_fragment,
                        const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto prepared_bir_text = bir::print(prepared.module);

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

  const auto generic_asm =
      c4c::backend::emit_module(c4c::backend::BackendModuleInput{module},
                                c4c::backend::BackendOptions{
                                    .target_profile = x86_target_profile(),
                                });
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

int check_local_i32_return_route_requires_authoritative_prepared_frame_access_contract(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* function_addressing =
      prepare::find_prepared_addressing(prepared, find_function_name_id(prepared, function_name));
  if (function_addressing == nullptr || function_addressing->accesses.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local return prepared frame-slot accesses")
                    .c_str());
  }

  auto mutated = prepared;
  prepare::PreparedAddressingFunction* mutable_addressing = nullptr;
  for (auto& candidate : mutated.addressing.functions) {
    if (prepare::prepared_function_name(mutated.names, candidate.function_name) == function_name) {
      mutable_addressing = &candidate;
      break;
    }
  }
  if (mutable_addressing == nullptr) {
    return fail((std::string(failure_context) +
                 ": mutated local return fixture lost its prepared frame-slot accesses")
                    .c_str());
  }

  auto& function = mutated.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || entry_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared local return fixture no longer exposes the expected entry load/store carriers")
                    .c_str());
  }
  auto* entry_store = std::get_if<bir::StoreLocalInst>(&entry_block->insts.front());
  auto* entry_load = std::get_if<bir::LoadLocalInst>(&entry_block->insts[1]);
  if (entry_store == nullptr || entry_load == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared local return fixture no longer exposes the expected direct local memory carriers")
                    .c_str());
  }

  entry_store->slot_name = "%drifted.local.return.store";
  entry_load->slot_name = "%drifted.local.return.load";
  mutable_addressing->accesses.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(mutated);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly fell back to raw local slot carriers after prepared frame-slot access loss")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected missing prepared frame-slot accesses with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_local_i32_guard_route_consumes_prepared_branch_contract(
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
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local guard prepared branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || entry_block->insts.size() < 3) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer has the expected entry shape")
                    .c_str());
  }
  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.back());
  if (entry_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer exposes the bounded compare carrier")
                    .c_str());
  }

  const std::string original_true_label = entry_block->terminator.true_label;
  const std::string original_false_label = entry_block->terminator.false_label;
  entry_block->terminator.condition.name = "contract.guard.condition";
  entry_block->terminator.true_label = "contract.guard.true";
  entry_block->terminator.false_label = "contract.guard.false";
  entry_compare->opcode = bir::BinaryOpcode::Eq;
  entry_compare->lhs = bir::Value::immediate_i32(7);
  entry_compare->rhs = bir::Value::immediate_i32(9);
  function.blocks.push_back(bir::Block{
      .label = "contract.guard.true",
      .terminator = bir::BranchTerminator{.target_label = original_true_label},
  });
  function.blocks.push_back(bir::Block{
      .label = "contract.guard.false",
      .terminator = bir::BranchTerminator{.target_label = original_false_label},
  });

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared branch contract over local guard carrier state")
                    .c_str());
  }

  return 0;
}

int check_local_i32_guard_route_requires_authoritative_prepared_branch_labels(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local guard prepared branch contract")
                    .c_str());
  }

  auto& branch_condition = control_flow->branch_conditions.front();
  branch_condition.false_label = prepared.names.block_labels.intern("drifted.guard.false");

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted drifted prepared guard labels")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected drifted prepared guard labels with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_local_i32_guard_route_requires_authoritative_prepared_branch_record(
    const bir::Module& module,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->blocks.empty() ||
      control_flow->branch_conditions.size() != 1 || !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local guard prepared branch contract")
                    .c_str());
  }

  control_flow->branch_conditions.clear();

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly reopened the raw local guard matcher after the prepared branch record was removed")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find("canonical prepared-module handoff") ==
        std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the missing prepared branch record with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_local_i32_guard_helper_publishes_prepared_compare_contract(const bir::Module& module,
                                                                     const char* function_name,
                                                                     const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr || control_flow->branch_conditions.size() != 1 ||
      !control_flow->join_transfers.empty()) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local guard prepared branch contract")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || entry_block->insts.size() < 3) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer has the expected entry shape")
                    .c_str());
  }
  auto* entry_compare = std::get_if<bir::BinaryInst>(&entry_block->insts.back());
  if (entry_compare == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer exposes the bounded compare carrier")
                    .c_str());
  }

  entry_compare->opcode = bir::BinaryOpcode::Eq;
  entry_compare->lhs = bir::Value::immediate_i32(7);
  entry_compare->rhs = bir::Value::immediate_i32(9);

  const auto* prepared_branch_condition = prepare::find_prepared_i32_immediate_branch_condition(
      prepared.names, *control_flow, entry_block->label, "loaded");
  if (prepared_branch_condition == nullptr) {
    return fail((std::string(failure_context) +
                 ": shared helper no longer recognizes the authoritative local guard compare contract")
                    .c_str());
  }
  if (!prepared_branch_condition->predicate.has_value() ||
      *prepared_branch_condition->predicate != bir::BinaryOpcode::Ne ||
      !prepared_branch_condition->lhs.has_value() ||
      prepared_branch_condition->lhs->name != "loaded" ||
      !prepared_branch_condition->rhs.has_value() ||
      prepared_branch_condition->rhs->kind != bir::Value::Kind::Immediate ||
      prepared_branch_condition->rhs->immediate != 123) {
    return fail((std::string(failure_context) +
                 ": shared helper stopped publishing the authoritative local guard compare semantics")
                    .c_str());
  }

  return 0;
}

int check_local_i32_guard_route_consumes_prepared_frame_contract(const bir::Module& module,
                                                                 const std::string& expected_asm,
                                                                 const char* function_name,
                                                                 const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* function_addressing =
      prepare::find_prepared_addressing(prepared, find_function_name_id(prepared, function_name));
  if (function_addressing == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes prepared frame facts for the local guard function")
                    .c_str());
  }

  auto mutated = prepared;
  prepare::PreparedAddressingFunction* mutable_addressing = nullptr;
  for (auto& candidate : mutated.addressing.functions) {
    if (prepare::prepared_function_name(mutated.names, candidate.function_name) == function_name) {
      mutable_addressing = &candidate;
      break;
    }
  }
  if (mutable_addressing == nullptr) {
    return fail((std::string(failure_context) +
                 ": mutated prepared local guard fixture lost its prepared frame facts")
                    .c_str());
  }
  mutable_addressing->frame_size_bytes = 24;
  mutable_addressing->frame_alignment_bytes = 8;

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(mutated);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following the authoritative prepared frame facts for local guard stack adjustment")
                    .c_str());
  }

  return 0;
}

int check_local_i32_guard_route_consumes_prepared_frame_access_contract(
    const bir::Module& module,
    const std::string& expected_asm,
    const char* function_name,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto* function_addressing =
      prepare::find_prepared_addressing(prepared, find_function_name_id(prepared, function_name));
  if (function_addressing == nullptr || function_addressing->accesses.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepare no longer publishes the local guard prepared frame-slot accesses")
                    .c_str());
  }

  auto& function = prepared.module.functions.front();
  auto* entry_block = find_block(function, "entry");
  if (entry_block == nullptr || entry_block->insts.size() < 2) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer exposes the expected entry load/store carriers")
                    .c_str());
  }
  auto* entry_store = std::get_if<bir::StoreLocalInst>(&entry_block->insts.front());
  auto* entry_load = std::get_if<bir::LoadLocalInst>(&entry_block->insts[1]);
  if (entry_store == nullptr || entry_load == nullptr) {
    return fail((std::string(failure_context) +
                 ": prepared local guard fixture no longer exposes the expected direct local memory carriers")
                    .c_str());
  }

  entry_store->slot_name = "%drifted.local.guard";
  entry_load->slot_name = "%drifted.local.guard";

  const auto prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer stopped following authoritative prepared frame-slot accesses over drifted local slot carriers")
                    .c_str());
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_local_i32_guard_tests() {
  if (const auto status =
          check_route_outputs(make_x86_local_i32_return_module(),
                              expected_minimal_local_i32_return_asm("main", 123),
                              "bir.store_local %lv.ret, i32 123",
                              "minimal local-slot return route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_return_route_requires_authoritative_prepared_frame_access_contract(
              make_x86_local_i32_return_module(),
              "main",
              "minimal local-slot return prepared frame-slot access ownership rejects raw local carrier fallback");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_outputs(make_x86_local_i32_immediate_guard_module(),
                              expected_minimal_local_i32_immediate_guard_asm("main", 123),
                              "bir.store_local %lv.x, i32 123",
                              "minimal local-slot compare-against-immediate guard route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_guard_route_consumes_prepared_branch_contract(
              make_x86_local_i32_immediate_guard_module(),
              expected_minimal_local_i32_immediate_guard_asm("main", 123),
              "main",
              "minimal local-slot compare-against-immediate guard prepared branch-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_guard_route_requires_authoritative_prepared_branch_labels(
              make_x86_local_i32_immediate_guard_module(),
              "main",
              "minimal local-slot compare-against-immediate guard prepared branch-contract ownership rejects drifted prepared labels instead of falling back to the local guard matcher");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_guard_route_requires_authoritative_prepared_branch_record(
              make_x86_local_i32_immediate_guard_module(),
              "main",
              "minimal local-slot compare-against-immediate guard prepared branch-contract ownership rejects reopening the raw local guard matcher when the prepared branch record is missing");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_guard_route_consumes_prepared_frame_contract(
              make_x86_local_i32_immediate_guard_module(),
              expected_minimal_local_i32_immediate_guard_asm_with_frame_size("main", 123, 32),
              "main",
              "minimal local-slot compare-against-immediate guard prepared frame-contract ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_guard_route_consumes_prepared_frame_access_contract(
              make_x86_local_i32_immediate_guard_module(),
              expected_minimal_local_i32_immediate_guard_asm("main", 123),
              "main",
              "minimal local-slot compare-against-immediate guard prepared frame-slot access ownership");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_local_i32_guard_helper_publishes_prepared_compare_contract(
              make_x86_local_i32_immediate_guard_module(),
              "main",
              "minimal local-slot compare-against-immediate guard helper publishes prepared compare contract");
      status != 0) {
    return status;
  }

  return 0;
}
