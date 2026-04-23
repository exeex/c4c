#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <cstdlib>
#include <iostream>
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

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

prepare::PreparedBirModule prepare_module(const bir::Module& module) {
  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;
  return prepare::prepare_semantic_bir_module_with_options(module, x86_target_profile(), options);
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
  if (find_stack_object(prepared, "vla.buf") != nullptr) {
    return fail("dynamic-stack contract: dynamic alloca unexpectedly became a fixed stack object");
  }
  const auto* liveness = find_liveness_function(prepared, "dynamic_stack_contract");
  if (liveness == nullptr || liveness->call_points.size() != 3) {
    return fail("dynamic-stack contract: prepared liveness no longer records stack-state calls");
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
  if (const int rc = check_dynamic_stack_contract(); rc != 0) {
    return rc;
  }
  return EXIT_SUCCESS;
}
