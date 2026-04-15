#include "stack_layout.hpp"

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/stack_layout/ for the intended
// frame, slot-assignment, and related analysis shape.

namespace c4c::backend::prepare {

namespace {

void append_stack_object(PreparedStackLayout& layout,
                         std::string_view function_name,
                         std::string_view source_name,
                         std::string_view source_kind,
                         bir::TypeKind type,
                         std::size_t size_bytes,
                         std::size_t align_bytes) {
  layout.objects.push_back(PreparedStackObject{
      .function_name = std::string(function_name),
      .source_name = std::string(source_name),
      .source_kind = std::string(source_kind),
      .type = type,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
  });
}

void append_param_memory_route_objects(PreparedStackLayout& layout, const bir::Function& function) {
  for (const auto& param : function.params) {
    if (param.is_sret) {
      append_stack_object(layout,
                          function.name,
                          param.name,
                          "sret_param",
                          param.type,
                          param.size_bytes,
                          param.align_bytes);
      continue;
    }
    if (param.is_byval) {
      append_stack_object(layout,
                          function.name,
                          param.name,
                          "byval_param",
                          param.type,
                          param.size_bytes,
                          param.align_bytes);
    }
  }
}

void append_call_result_objects(PreparedStackLayout& layout, const bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<bir::CallInst>(&inst);
      if (call == nullptr || !call->sret_storage_name.has_value() || call->args.empty() ||
          call->arg_abi.empty()) {
        continue;
      }
      if (!call->arg_abi.front().sret_pointer || call->args.front().type != bir::TypeKind::Ptr ||
          call->args.front().kind != bir::Value::Kind::Named ||
          call->args.front().name != *call->sret_storage_name) {
        continue;
      }
      append_stack_object(layout,
                          function.name,
                          *call->sret_storage_name,
                          "call_result_sret",
                          bir::TypeKind::Ptr,
                          call->arg_abi.front().size_bytes,
                          call->arg_abi.front().align_bytes);
    }
  }
}

void append_variadic_aggregate_output_objects(PreparedStackLayout& layout,
                                              const bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<bir::CallInst>(&inst);
      if (call == nullptr || call->callee != "llvm.va_arg.aggregate" || call->args.empty() ||
          call->arg_abi.empty()) {
        continue;
      }
      if (!call->arg_abi.front().sret_pointer || call->args.front().type != bir::TypeKind::Ptr ||
          call->args.front().kind != bir::Value::Kind::Named) {
        continue;
      }
      append_stack_object(layout,
                          function.name,
                          call->args.front().name,
                          "va_arg_aggregate_result",
                          bir::TypeKind::Ptr,
                          call->arg_abi.front().size_bytes,
                          call->arg_abi.front().align_bytes);
    }
  }
}

bool has_addressed_local_access(const bir::Function& function, std::string_view slot_name) {
  const auto matches_addressed_slot = [&](const auto& inst) {
    return inst.address.has_value() &&
           inst.address->base_kind == bir::MemoryAddress::BaseKind::LocalSlot &&
           inst.address->base_name == slot_name;
  };

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst); load != nullptr &&
          matches_addressed_slot(*load)) {
        return true;
      }
      if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst); store != nullptr &&
          matches_addressed_slot(*store)) {
        return true;
      }
    }
  }
  return false;
}

std::string_view stack_object_source_kind(const bir::Function& function, const bir::LocalSlot& slot) {
  if (slot.is_byval_copy) {
    return "byval_copy_slot";
  }
  if (slot.phi_observation.has_value()) {
    return "phi_materialize_slot";
  }
  if (slot.is_address_taken || has_addressed_local_access(function, slot.name)) {
    return "address_taken_local_slot";
  }
  return "local_slot";
}

}  // namespace

void run_stack_layout(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("stack_layout");
  module.notes.push_back(PrepareNote{
      .phase = "stack_layout",
      .message = "stack layout skeleton is wired; no frame/slot allocation is performed yet",
  });
}

void run_stack_layout(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("stack_layout");
  module.stack_layout.objects.clear();
  for (const auto& function : module.module.functions) {
    append_param_memory_route_objects(module.stack_layout, function);
    append_call_result_objects(module.stack_layout, function);
    append_variadic_aggregate_output_objects(module.stack_layout, function);
    for (const auto& slot : function.local_slots) {
      append_stack_object(module.stack_layout,
                          function.name,
                          slot.name,
                          stack_object_source_kind(function, slot),
                          slot.type,
                          slot.size_bytes,
                          slot.align_bytes);
    }
  }
  module.notes.push_back(PrepareNote{
      .phase = "stack_layout",
      .message =
          "stack layout now publishes local-slot, address-taken local-slot, and "
          "phi-materialize stack objects plus byval/sret memory-route frame objects, "
          "aggregate call-result sret storage, and aggregate va_arg output storage as "
          "prepared artifacts; frame offset assignment remains future work",
  });
}

}  // namespace c4c::backend::prepare
