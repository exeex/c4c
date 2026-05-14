#include "traversal.hpp"

#include "dispatch.hpp"

#include <cstddef>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

[[nodiscard]] const c4c::backend::bir::Function* find_bir_function(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedControlFlowFunction& function) {
  if (function.function_name == c4c::kInvalidFunctionName) {
    return nullptr;
  }

  const std::string_view prepared_function_name =
      prepare::prepared_function_name(prepared.names, function.function_name);
  if (prepared_function_name.empty()) {
    return nullptr;
  }

  for (const auto& bir_function : prepared.module.functions) {
    if (std::string_view{bir_function.name} == prepared_function_name) {
      return &bir_function;
    }
  }
  return nullptr;
}

[[nodiscard]] bool has_simple_fixed_frame_plan(
    const module::FunctionLoweringContext& context) {
  const auto* frame = context.frame_plan;
  if (frame == nullptr || context.control_flow == nullptr) {
    return false;
  }
  if (frame->function_name != context.control_flow->function_name ||
      frame->function_name == c4c::kInvalidFunctionName ||
      frame->frame_alignment_bytes == 0 || frame->frame_size_bytes == 0) {
    return false;
  }
  if (frame->has_dynamic_stack || context.dynamic_stack_plan != nullptr ||
      !frame->saved_callee_registers.empty()) {
    return false;
  }
  return true;
}

[[nodiscard]] module::MachineInstruction make_frame_machine_instruction(
    const module::FunctionLoweringContext& context,
    FrameInstructionKind frame_kind,
    c4c::BlockLabelId block_label,
    std::size_t block_index) {
  const auto* frame = context.frame_plan;
  InstructionRecord target = make_frame_instruction(FrameInstructionRecord{
      .frame_kind = frame_kind,
      .function_name = frame != nullptr ? frame->function_name : c4c::kInvalidFunctionName,
      .frame_size_bytes = frame != nullptr ? frame->frame_size_bytes : 0,
      .frame_alignment_bytes = frame != nullptr ? frame->frame_alignment_bytes : 0,
      .frame_slot_order = frame != nullptr
                              ? frame->frame_slot_order
                              : std::vector<prepare::PreparedFrameSlotId>{},
      .saved_callee_registers = frame != nullptr
                                    ? frame->saved_callee_registers
                                    : std::vector<prepare::PreparedSavedRegister>{},
      .has_dynamic_stack = frame != nullptr ? frame->has_dynamic_stack : false,
      .uses_frame_pointer_for_fixed_slots =
          frame != nullptr ? frame->uses_frame_pointer_for_fixed_slots : false,
      .source_frame = frame,
  });
  target.block_label = block_label;
  target.block_index = block_index;

  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::Synthetic,
              .function_name = frame != nullptr ? frame->function_name : c4c::kInvalidFunctionName,
              .block_label = block_label,
          },
  };
}

void insert_simple_fixed_frame_nodes(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedControlFlowFunction& prepared_function,
    module::MachineFunction& function) {
  if (!has_simple_fixed_frame_plan(context) || function.blocks.empty()) {
    return;
  }

  auto& entry = function.blocks.front();
  entry.instructions.insert(
      entry.instructions.begin(),
      make_frame_machine_instruction(context,
                                     FrameInstructionKind::PrologueSetup,
                                     entry.block_label,
                                     entry.index));

  for (std::size_t block_index = 0; block_index < prepared_function.blocks.size() &&
                                  block_index < function.blocks.size();
       ++block_index) {
    if (prepared_function.blocks[block_index].terminator_kind !=
        c4c::backend::bir::TerminatorKind::Return) {
      continue;
    }
    auto& block = function.blocks[block_index];
    auto insertion_point = block.instructions.end();
    for (auto it = block.instructions.begin(); it != block.instructions.end(); ++it) {
      if (std::holds_alternative<ReturnInstructionRecord>(it->target.payload)) {
        insertion_point = it;
        break;
      }
    }
    block.instructions.insert(
        insertion_point,
        make_frame_machine_instruction(context,
                                       FrameInstructionKind::EpilogueTeardown,
                                       block.block_label,
                                       block.index));
  }
}

}  // namespace

module::FunctionLoweringContext make_function_lowering_context(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    const prepare::PreparedControlFlowFunction& function) {
  return module::FunctionLoweringContext{
      .prepared = &prepared,
      .target_profile = &target_profile,
      .control_flow = &function,
      .bir_function = find_bir_function(prepared, function),
      .value_locations =
          prepare::find_prepared_value_location_function(prepared, function.function_name),
      .storage_plan = prepare::find_prepared_storage_plan(prepared, function.function_name),
      .regalloc = [&prepared, function_name = function.function_name]()
          -> const prepare::PreparedRegallocFunction* {
        for (const auto& regalloc_function : prepared.regalloc.functions) {
          if (regalloc_function.function_name == function_name) {
            return &regalloc_function;
          }
        }
        return nullptr;
      }(),
      .frame_plan = prepare::find_prepared_frame_plan(prepared, function.function_name),
      .dynamic_stack_plan =
          prepare::find_prepared_dynamic_stack_plan(prepared, function.function_name),
      .call_plans = prepare::find_prepared_call_plans(prepared, function.function_name),
  };
}

std::vector<module::MachineFunction> lower_prepared_functions(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineFunction> functions;
  functions.reserve(prepared.control_flow.functions.size());

  for (const auto& prepared_function : prepared.control_flow.functions) {
    const auto function_context =
        make_function_lowering_context(prepared, target_profile, prepared_function);

    module::MachineFunction function{
        .function_name = prepared_function.function_name,
        .blocks = {},
    };
    function.blocks.reserve(prepared_function.blocks.size());

    for (std::size_t block_index = 0; block_index < prepared_function.blocks.size();
         ++block_index) {
      const auto& prepared_block = prepared_function.blocks[block_index];
      function.blocks.push_back(module::MachineBlock{
          .block_label = prepared_block.block_label,
          .index = block_index,
          .instructions = {},
      });
      const auto block_context =
          make_block_lowering_context(function_context, prepared_block, block_index);
      (void)dispatch_prepared_block(block_context, function.blocks.back(), diagnostics);
    }

    if (prepared_function.function_name == c4c::kInvalidFunctionName) {
      diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
          .kind = module::ModuleLoweringDiagnosticKind::MissingFunctionContext,
          .function_name = prepared_function.function_name,
          .message = "prepared control-flow function is missing durable function identity",
      });
    }

    insert_simple_fixed_frame_nodes(function_context, prepared_function, function);

    functions.push_back(std::move(function));
  }

  return functions;
}

}  // namespace c4c::backend::aarch64::codegen
