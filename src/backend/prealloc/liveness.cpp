#include "prealloc.hpp"

#include <algorithm>

namespace c4c::backend::prepare {

namespace {

bool memory_address_targets_object(const std::optional<bir::MemoryAddress>& address,
                                   std::string_view source_name) {
  return address.has_value() && address->base_kind == bir::MemoryAddress::BaseKind::LocalSlot &&
         address->base_name == source_name;
}

bool named_value_targets_object(const bir::Value& value, std::string_view source_name) {
  return value.kind == bir::Value::Kind::Named && value.name == source_name;
}

PreparedValueKind prepared_value_kind_from_source(std::string_view source_kind) {
  if (source_kind == "call_result_sret") {
    return PreparedValueKind::CallResult;
  }
  if (source_kind == "byval_param" || source_kind == "sret_param") {
    return PreparedValueKind::Parameter;
  }
  return PreparedValueKind::StackObject;
}

std::size_t count_function_instructions(const bir::Function& function) {
  std::size_t instruction_count = 0;
  for (const auto& block : function.blocks) {
    instruction_count += block.insts.size();
  }
  return instruction_count;
}

std::vector<std::size_t> collect_call_instruction_indices(const bir::Function& function) {
  std::vector<std::size_t> call_indices;
  std::size_t instruction_index = 0;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::CallInst>(inst)) {
        call_indices.push_back(instruction_index);
      }
      ++instruction_index;
    }
  }
  return call_indices;
}

PreparedLivenessBlock build_liveness_block(const bir::Block& block,
                                           std::size_t block_index,
                                           std::size_t instruction_start_index) {
  PreparedLivenessBlock prepared_block{
      .block_name = block.label,
      .block_index = block_index,
      .instruction_start_index = instruction_start_index,
      .instruction_end_index = instruction_start_index + block.insts.size(),
  };
  return prepared_block;
}

PreparedLivenessValue build_liveness_value(const bir::Function& function,
                                           const PreparedStackObject& object,
                                           PreparedValueId value_id,
                                           std::size_t function_instruction_count,
                                           const std::vector<std::size_t>& call_instruction_indices) {
  PreparedLivenessValue value{
      .value_id = value_id,
      .stack_object_id = object.object_id,
      .function_name = object.function_name,
      .source_name = object.source_name,
      .source_kind = object.source_kind,
      .type = object.type,
      .value_kind = prepared_value_kind_from_source(object.source_kind),
      .address_taken = object.address_exposed,
      .requires_home_slot = object.requires_home_slot,
  };

  std::size_t block_index = 0;
  std::size_t linear_instruction_index = 0;
  bool has_definition = false;
  bool has_any_use = false;
  std::size_t first_use_instruction_index = 0;
  std::size_t last_use_instruction_index = 0;

  for (const auto& block : function.blocks) {
    std::size_t block_instruction_index = 0;
    for (const auto& inst : block.insts) {
      if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst); load != nullptr) {
        if (load->slot_name == object.source_name) {
          value.use_sites.push_back(PreparedLivenessUseSite{
              .block_index = block_index,
              .instruction_index = linear_instruction_index,
              .operand_index = 0,
              .use_kind = PreparedUseKind::Read,
          });
          if (!has_any_use) {
            first_use_instruction_index = linear_instruction_index;
            has_any_use = true;
          }
          last_use_instruction_index = linear_instruction_index;
        }
        if (memory_address_targets_object(load->address, object.source_name)) {
          value.use_sites.push_back(PreparedLivenessUseSite{
              .block_index = block_index,
              .instruction_index = linear_instruction_index,
              .operand_index = 1,
              .use_kind = PreparedUseKind::Address,
          });
          value.address_taken = true;
          if (!has_any_use) {
            first_use_instruction_index = linear_instruction_index;
            has_any_use = true;
          }
          last_use_instruction_index = linear_instruction_index;
        }
      } else if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst); store != nullptr) {
        if (store->slot_name == object.source_name) {
          value.use_sites.push_back(PreparedLivenessUseSite{
              .block_index = block_index,
              .instruction_index = linear_instruction_index,
              .operand_index = 0,
              .use_kind = PreparedUseKind::Write,
          });
          if (!has_definition) {
            value.definition_site = PreparedLivenessDefSite{
                .block_index = block_index,
                .instruction_index = linear_instruction_index,
                .definition_kind = "store_local",
            };
            has_definition = true;
          }
          if (!has_any_use) {
            first_use_instruction_index = linear_instruction_index;
            has_any_use = true;
          }
          last_use_instruction_index = linear_instruction_index;
        }
        if (memory_address_targets_object(store->address, object.source_name)) {
          value.use_sites.push_back(PreparedLivenessUseSite{
              .block_index = block_index,
              .instruction_index = linear_instruction_index,
              .operand_index = 1,
              .use_kind = PreparedUseKind::Address,
          });
          value.address_taken = true;
          if (!has_any_use) {
            first_use_instruction_index = linear_instruction_index;
            has_any_use = true;
          }
          last_use_instruction_index = linear_instruction_index;
        }
      } else if (const auto* call = std::get_if<bir::CallInst>(&inst); call != nullptr) {
        for (std::size_t operand_index = 0; operand_index < call->args.size(); ++operand_index) {
          if (!named_value_targets_object(call->args[operand_index], object.source_name)) {
            continue;
          }
          value.use_sites.push_back(PreparedLivenessUseSite{
              .block_index = block_index,
              .instruction_index = linear_instruction_index,
              .operand_index = operand_index,
              .use_kind = PreparedUseKind::CallArgument,
          });
          if (!has_any_use) {
            first_use_instruction_index = linear_instruction_index;
            has_any_use = true;
          }
          last_use_instruction_index = linear_instruction_index;
        }
      }

      ++block_instruction_index;
      ++linear_instruction_index;
    }
    ++block_index;
  }

  if (has_any_use) {
    value.live_segments.push_back(PreparedLivenessLiveSegment{
        .start_block_index = value.definition_site.has_value() ? value.definition_site->block_index : 0,
        .start_instruction_index =
            value.definition_site.has_value() ? value.definition_site->instruction_index
                                              : first_use_instruction_index,
        .end_block_index = function.blocks.empty() ? 0 : block_index - 1,
        .end_instruction_index = last_use_instruction_index,
    });

    std::copy_if(call_instruction_indices.begin(),
                 call_instruction_indices.end(),
                 std::back_inserter(value.crossed_call_instruction_indices),
                 [&](std::size_t instruction_index) {
                   const auto segment_start = value.live_segments.front().start_instruction_index;
                   return instruction_index > segment_start &&
                          instruction_index < value.live_segments.front().end_instruction_index;
                 });
    value.crosses_call = !value.crossed_call_instruction_indices.empty();
  }

  if (!has_definition && function_instruction_count != 0) {
    value.definition_site = PreparedLivenessDefSite{
        .block_index = 0,
        .instruction_index = 0,
        .definition_kind = "function_entry",
    };
  }

  return value;
}

}  // namespace

void BirPreAlloc::run_liveness() {
  prepared_.completed_phases.push_back("liveness");
  prepared_.liveness.functions.clear();
  prepared_.liveness.functions.reserve(prepared_.module.functions.size());

  PreparedValueId next_value_id = 0;
  for (const auto& function : prepared_.module.functions) {
    PreparedLivenessFunction prepared_function{
        .function_name = function.name,
        .instruction_count = count_function_instructions(function),
        .call_instruction_count = 0,
        .call_instruction_indices = collect_call_instruction_indices(function),
    };
    prepared_function.call_instruction_count = prepared_function.call_instruction_indices.size();

    std::size_t instruction_start_index = 0;
    for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
      const auto& block = function.blocks[block_index];
      auto prepared_block = build_liveness_block(block, block_index, instruction_start_index);
      if (block_index != 0) {
        prepared_block.predecessor_block_indices.push_back(block_index - 1);
      }
      if (block_index + 1 < function.blocks.size()) {
        prepared_block.successor_block_indices.push_back(block_index + 1);
      }
      instruction_start_index += block.insts.size();
      prepared_function.blocks.push_back(std::move(prepared_block));
    }

    for (const auto& object : prepared_.stack_layout.objects) {
      if (object.function_name != function.name) {
        continue;
      }
      prepared_function.values.push_back(build_liveness_value(
          function,
          object,
          next_value_id++,
          prepared_function.instruction_count,
          prepared_function.call_instruction_indices));
    }

    prepared_.liveness.functions.push_back(std::move(prepared_function));
  }

  prepared_.notes.push_back(PrepareNote{
      .phase = "liveness",
      .message =
          "liveness now publishes per-function value records with def/use sites, "
          "live segments, block summaries, and crossed-call observations as the "
          "new prepared-BIR lifetime contract",
  });
}

}  // namespace c4c::backend::prepare
