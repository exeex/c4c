#include "liveness.hpp"

#include <algorithm>

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/liveness.rs for the intended
// liveness/dataflow analysis shape.

namespace c4c::backend::prepare {

namespace {

enum class LivenessAccessKind {
  None,
  DirectRead,
  DirectWrite,
  AddressedAccess,
  CallArgumentExposure,
};

struct LivenessObjectAccessSummary {
  std::size_t direct_read_count = 0;
  std::size_t direct_write_count = 0;
  std::size_t addressed_access_count = 0;
  std::size_t call_arg_exposure_count = 0;
  bool has_access_window = false;
  std::size_t first_access_instruction_index = 0;
  std::size_t last_access_instruction_index = 0;
  LivenessAccessKind first_access_kind = LivenessAccessKind::None;
  LivenessAccessKind last_access_kind = LivenessAccessKind::None;
  std::vector<std::size_t> call_instruction_indices;
};

void record_access(LivenessObjectAccessSummary& summary,
                   std::size_t instruction_index,
                   LivenessAccessKind access_kind) {
  if (!summary.has_access_window) {
    summary.has_access_window = true;
    summary.first_access_instruction_index = instruction_index;
    summary.first_access_kind = access_kind;
  }
  summary.last_access_instruction_index = instruction_index;
  summary.last_access_kind = access_kind;
}

std::string_view liveness_contract_kind(std::string_view source_kind) {
  if (source_kind == "address_taken_local_slot" || source_kind == "sret_param" ||
      source_kind == "byval_param" || source_kind == "call_result_sret" ||
      source_kind == "va_arg_aggregate_result") {
    return "address_exposed_storage";
  }
  return "value_storage";
}

bool memory_address_targets_object(const std::optional<bir::MemoryAddress>& address,
                                   std::string_view source_name) {
  return address.has_value() && address->base_kind == bir::MemoryAddress::BaseKind::LocalSlot &&
         address->base_name == source_name;
}

bool named_value_targets_object(const bir::Value& value, std::string_view source_name) {
  return value.kind == bir::Value::Kind::Named && value.name == source_name;
}

std::string_view liveness_access_kind_name(LivenessAccessKind access_kind) {
  switch (access_kind) {
    case LivenessAccessKind::None:
      return "none";
    case LivenessAccessKind::DirectRead:
      return "direct_read";
    case LivenessAccessKind::DirectWrite:
      return "direct_write";
    case LivenessAccessKind::AddressedAccess:
      return "addressed_access";
    case LivenessAccessKind::CallArgumentExposure:
      return "call_argument_exposure";
  }
  return "unknown";
}

std::string_view liveness_access_shape_name(const LivenessObjectAccessSummary& summary) {
  const bool has_direct_read = summary.direct_read_count != 0;
  const bool has_direct_write = summary.direct_write_count != 0;
  const bool has_addressed_access = summary.addressed_access_count != 0;
  const bool has_call_arg_exposure = summary.call_arg_exposure_count != 0;
  const std::size_t active_category_count = static_cast<std::size_t>(has_direct_read) +
                                            static_cast<std::size_t>(has_direct_write) +
                                            static_cast<std::size_t>(has_addressed_access) +
                                            static_cast<std::size_t>(has_call_arg_exposure);
  if (active_category_count == 0) {
    return "no_access";
  }
  if (active_category_count == 1) {
    if (has_direct_read) {
      return "direct_read_only";
    }
    if (has_direct_write) {
      return "direct_write_only";
    }
    if (has_addressed_access) {
      return "addressed_access_only";
    }
    return "call_argument_exposure_only";
  }
  if (active_category_count == 2 && has_direct_read && has_direct_write &&
      !has_addressed_access && !has_call_arg_exposure) {
    return "direct_read_write";
  }
  return "mixed_access_shape";
}

LivenessObjectAccessSummary summarize_object_accesses(const bir::Function& function,
                                                      std::string_view source_name) {
  LivenessObjectAccessSummary summary;
  std::size_t instruction_index = 0;

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst); load != nullptr) {
        if (load->slot_name == source_name) {
          ++summary.direct_read_count;
          record_access(summary, instruction_index, LivenessAccessKind::DirectRead);
        }
        if (memory_address_targets_object(load->address, source_name)) {
          ++summary.addressed_access_count;
          record_access(summary, instruction_index, LivenessAccessKind::AddressedAccess);
        }
        ++instruction_index;
        continue;
      }

      if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst); store != nullptr) {
        if (store->slot_name == source_name) {
          ++summary.direct_write_count;
          record_access(summary, instruction_index, LivenessAccessKind::DirectWrite);
        }
        if (memory_address_targets_object(store->address, source_name)) {
          ++summary.addressed_access_count;
          record_access(summary, instruction_index, LivenessAccessKind::AddressedAccess);
        }
        ++instruction_index;
        continue;
      }

      const auto* call = std::get_if<bir::CallInst>(&inst);
      if (call == nullptr) {
        ++instruction_index;
        continue;
      }

      summary.call_instruction_indices.push_back(instruction_index);
      for (const auto& arg : call->args) {
        if (named_value_targets_object(arg, source_name)) {
          ++summary.call_arg_exposure_count;
          record_access(summary, instruction_index, LivenessAccessKind::CallArgumentExposure);
        }
      }
      ++instruction_index;
    }
  }

  return summary;
}

bool crosses_call_boundary(const LivenessObjectAccessSummary& summary) {
  if (!summary.has_access_window ||
      summary.first_access_instruction_index == summary.last_access_instruction_index) {
    return false;
  }
  return std::any_of(summary.call_instruction_indices.begin(),
                     summary.call_instruction_indices.end(),
                     [&](std::size_t instruction_index) {
                       return instruction_index > summary.first_access_instruction_index &&
                              instruction_index < summary.last_access_instruction_index;
                     });
}

std::size_t count_function_instructions(const bir::Function& function) {
  std::size_t instruction_count = 0;
  for (const auto& block : function.blocks) {
    instruction_count += block.insts.size();
  }
  return instruction_count;
}

}  // namespace

void run_liveness(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("liveness");
  module.notes.push_back(PrepareNote{
      .phase = "liveness",
      .message = "liveness skeleton is wired; no live-interval computation is performed yet",
  });
}

void run_liveness(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("liveness");
  module.liveness.functions.clear();
  module.liveness.objects.clear();
  module.liveness.functions.reserve(module.module.functions.size());
  module.liveness.objects.reserve(module.stack_layout.objects.size());

  for (const auto& function : module.module.functions) {
    PreparedLivenessFunction prepared_function{
        .function_name = function.name,
        .instruction_count = count_function_instructions(function),
    };
    std::size_t linear_instruction_index = 0;
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        if (std::holds_alternative<bir::CallInst>(inst)) {
          prepared_function.call_instruction_indices.push_back(linear_instruction_index);
        }
        ++linear_instruction_index;
      }
    }

    prepared_function.call_instruction_count = prepared_function.call_instruction_indices.size();

    for (const auto& object : module.stack_layout.objects) {
      if (object.function_name != function.name) {
        continue;
      }
      auto summary = summarize_object_accesses(function, object.source_name);
      module.liveness.objects.push_back(PreparedLivenessObject{
          .function_name = object.function_name,
          .source_name = object.source_name,
          .source_kind = object.source_kind,
          .contract_kind = std::string(liveness_contract_kind(object.source_kind)),
          .access_shape = std::string(liveness_access_shape_name(summary)),
          .first_access_kind = std::string(liveness_access_kind_name(summary.first_access_kind)),
          .last_access_kind = std::string(liveness_access_kind_name(summary.last_access_kind)),
          .direct_read_count = summary.direct_read_count,
          .direct_write_count = summary.direct_write_count,
          .addressed_access_count = summary.addressed_access_count,
          .call_arg_exposure_count = summary.call_arg_exposure_count,
          .has_access_window = summary.has_access_window,
          .first_access_instruction_index = summary.first_access_instruction_index,
          .last_access_instruction_index = summary.last_access_instruction_index,
          .crosses_call_boundary = crosses_call_boundary(summary),
      });
      ++prepared_function.object_count;
    }

    module.liveness.functions.push_back(std::move(prepared_function));
  }

  module.notes.push_back(PrepareNote{
      .phase = "liveness",
      .message =
          "liveness now classifies prepared stack-layout objects into value-storage "
          "or address-exposed storage contracts, plus direct read/write, "
          "addressed-access, and call-argument exposure counts, compact "
          "access-shape summaries, and instruction-order access windows with "
          "call-crossing cues for downstream prepared-BIR consumers; full "
          "live-interval computation remains future work",
  });
}

}  // namespace c4c::backend::prepare
