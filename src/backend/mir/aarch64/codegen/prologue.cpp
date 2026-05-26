#include "prologue.hpp"
#include "../../../prealloc/target_register_profile.hpp"
#include "constant_materialization.hpp"
#include "../abi/abi.hpp"
#include "dispatch.hpp"

#include "instruction.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

constexpr std::size_t kStackPointerAlignmentBytes = 16;

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
  if (frame->has_dynamic_stack || context.dynamic_stack_plan != nullptr) {
    return false;
  }
  return true;
}

[[nodiscard]] bool has_non_leaf_call(const module::MachineFunction& function) {
  for (const auto& block : function.blocks) {
    for (const auto& instruction : block.instructions) {
      if (instruction.target.family == InstructionFamily::Call) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const auto remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

struct FrameBoundaryFacts {
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  bool preserves_link_register = false;
  std::optional<std::size_t> link_register_save_offset_bytes;
  bool preserves_frame_pointer = false;
  std::optional<std::size_t> frame_pointer_save_offset_bytes;
};

[[nodiscard]] std::optional<FrameBoundaryFacts> frame_boundary_facts(
    const module::FunctionLoweringContext& context,
    const module::MachineFunction& function) {
  const bool simple_fixed_frame = has_simple_fixed_frame_plan(context);
  const bool non_leaf = has_non_leaf_call(function);
  const bool dynamic_fixed_slots =
      context.frame_plan != nullptr &&
      context.frame_plan->has_dynamic_stack &&
      context.frame_plan->uses_frame_pointer_for_fixed_slots;
  if (!simple_fixed_frame && !non_leaf && !dynamic_fixed_slots) {
    return std::nullopt;
  }
  if (context.frame_plan != nullptr &&
      (context.frame_plan->has_dynamic_stack || context.dynamic_stack_plan != nullptr) &&
      !dynamic_fixed_slots) {
    return std::nullopt;
  }

  const auto* frame = context.frame_plan;
  const std::size_t prepared_frame_alignment =
      frame != nullptr ? frame->frame_alignment_bytes : 0;
  std::size_t frame_alignment =
      std::max<std::size_t>(prepared_frame_alignment, kStackPointerAlignmentBytes);
  std::size_t prepared_frame_size = frame != nullptr ? frame->frame_size_bytes : 0;
  if (frame != nullptr) {
    for (const auto& saved : frame->saved_callee_registers) {
      if (!saved.slot_placement.has_value() ||
          !prepare::has_complete_prepared_saved_register_slot_placement(
              *saved.slot_placement)) {
        if (dynamic_fixed_slots) {
          continue;
        }
        return std::nullopt;
      }
      const auto& placement = *saved.slot_placement;
      prepared_frame_size = std::max(prepared_frame_size,
                                     *placement.stack_offset_bytes +
                                         *placement.size_bytes);
      frame_alignment = std::max(frame_alignment, *placement.align_bytes);
    }
  }
  FrameBoundaryFacts facts;
  facts.frame_alignment_bytes = frame_alignment;
  facts.frame_size_bytes = align_to(prepared_frame_size, frame_alignment);
  if (non_leaf) {
    facts.preserves_link_register = true;
    prepared_frame_size = align_to(prepared_frame_size, 8U);
    facts.link_register_save_offset_bytes = prepared_frame_size;
    prepared_frame_size += 16U;
  }
  if (dynamic_fixed_slots) {
    facts.preserves_frame_pointer = true;
    prepared_frame_size = align_to(prepared_frame_size, 8U);
    facts.frame_pointer_save_offset_bytes = prepared_frame_size;
    prepared_frame_size += 8U;
  }
  facts.frame_size_bytes = align_to(prepared_frame_size, frame_alignment);
  if (facts.frame_size_bytes == 0) {
    return std::nullopt;
  }
  return facts;
}

[[nodiscard]] std::vector<prepare::PreparedSavedRegister> printable_saved_registers(
    const prepare::PreparedFramePlanFunction* frame) {
  std::vector<prepare::PreparedSavedRegister> saved_registers;
  if (frame == nullptr) {
    return saved_registers;
  }
  for (const auto& saved : frame->saved_callee_registers) {
    if (saved.slot_placement.has_value() &&
        prepare::has_complete_prepared_saved_register_slot_placement(
            *saved.slot_placement)) {
      saved_registers.push_back(saved);
    }
  }
  return saved_registers;
}

[[nodiscard]] module::MachineInstruction make_frame_machine_instruction(
    const module::FunctionLoweringContext& context,
    const FrameBoundaryFacts& facts,
    FrameInstructionKind frame_kind,
    c4c::BlockLabelId block_label,
    std::size_t block_index) {
  const auto* frame = context.frame_plan;
  InstructionRecord target = make_frame_instruction(FrameInstructionRecord{
      .frame_kind = frame_kind,
      .function_name = context.control_flow != nullptr
                           ? context.control_flow->function_name
                           : (frame != nullptr ? frame->function_name
                                               : c4c::kInvalidFunctionName),
      .frame_size_bytes = facts.frame_size_bytes,
      .frame_alignment_bytes = facts.frame_alignment_bytes,
      .preserves_link_register = facts.preserves_link_register,
      .link_register_save_offset_bytes = facts.link_register_save_offset_bytes,
      .frame_slot_order = frame != nullptr
                              ? frame->frame_slot_order
                              : std::vector<prepare::PreparedFrameSlotId>{},
      .saved_callee_registers = printable_saved_registers(frame),
      .has_dynamic_stack = frame != nullptr ? frame->has_dynamic_stack : false,
      .uses_frame_pointer_for_fixed_slots =
          frame != nullptr ? frame->uses_frame_pointer_for_fixed_slots : false,
      .preserves_frame_pointer = facts.preserves_frame_pointer,
      .frame_pointer_save_offset_bytes = facts.frame_pointer_save_offset_bytes,
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
              .function_name = context.control_flow != nullptr
                                   ? context.control_flow->function_name
                                   : (frame != nullptr ? frame->function_name
                                                       : c4c::kInvalidFunctionName),
              .block_label = block_label,
          },
  };
}

}  // namespace

void insert_prepared_frame_boundary_nodes(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedControlFlowFunction& prepared_function,
    module::MachineFunction& function) {
  const auto facts = frame_boundary_facts(context, function);
  if (!facts.has_value() || function.blocks.empty()) {
    return;
  }

  auto& entry = function.blocks.front();
  entry.instructions.insert(
      entry.instructions.begin(),
      make_frame_machine_instruction(context,
                                     *facts,
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
                                       *facts,
                                       FrameInstructionKind::EpilogueTeardown,
                                       block.block_label,
                                       block.index));
  }
}

namespace {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::optional<abi::RegisterView> entry_formal_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::F128:
      return abi::RegisterView::Q;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> entry_formal_load_opcode(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return "ldrb";
    case bir::TypeKind::I16:
      return "ldrh";
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return "ldr";
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> entry_formal_store_opcode(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return "strb";
    case bir::TypeKind::I16:
      return "strh";
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return "str";
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool entry_formal_same_aarch64_register_bank(
    const bir::CallArgAbiInfo& lhs,
    const bir::CallArgAbiInfo& rhs) {
  const auto is_float_bank = [](const bir::CallArgAbiInfo& abi) {
    return abi.primary_class == bir::AbiValueClass::Sse ||
           abi.primary_class == bir::AbiValueClass::X87;
  };
  if (is_float_bank(lhs) || is_float_bank(rhs)) {
    return is_float_bank(lhs) == is_float_bank(rhs);
  }
  return lhs.primary_class == bir::AbiValueClass::Integer &&
         rhs.primary_class == bir::AbiValueClass::Integer;
}

[[nodiscard]] std::size_t entry_formal_aarch64_register_slot_count(
    const bir::CallArgAbiInfo& abi) {
  if (abi.type == bir::TypeKind::Ptr && abi.byval_copy &&
      !abi.sret_pointer &&
      abi.primary_class == bir::AbiValueClass::Integer &&
      abi.size_bytes > 0 && abi.size_bytes <= 16) {
    return align_to(std::max<std::size_t>(abi.size_bytes, 8), 8) / 8;
  }
  return 1;
}

[[nodiscard]] std::optional<std::size_t> entry_formal_aarch64_register_slot_start(
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size() ||
      !function.params[param_index].abi.has_value()) {
    return std::nullopt;
  }
  const auto& param_abi = *function.params[param_index].abi;
  if (!param_abi.passed_in_register ||
      (param_abi.type == bir::TypeKind::Ptr && param_abi.sret_pointer)) {
    return std::nullopt;
  }

  std::size_t register_index = 0;
  for (std::size_t candidate_index = 0; candidate_index < param_index; ++candidate_index) {
    const auto& candidate = function.params[candidate_index];
    if (!candidate.abi.has_value() || !candidate.abi->passed_in_register) {
      continue;
    }
    if (candidate.abi->type == bir::TypeKind::Ptr && candidate.abi->sret_pointer) {
      continue;
    }
    if (entry_formal_same_aarch64_register_bank(*candidate.abi, param_abi)) {
      register_index += entry_formal_aarch64_register_slot_count(*candidate.abi);
    }
  }

  const auto required_slots = entry_formal_aarch64_register_slot_count(param_abi);
  if (param_abi.primary_class == bir::AbiValueClass::Integer &&
      register_index + required_slots > 8U) {
    return std::nullopt;
  }
  return register_index;
}

[[nodiscard]] std::optional<std::size_t> entry_formal_abi_register_index(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size()) {
    return std::nullopt;
  }
  const auto& param = function.params[param_index];
  if (!param.abi.has_value() || !param.abi->passed_in_register) {
    if (target_profile.arch == c4c::TargetArch::Aarch64 &&
        param.abi.has_value() &&
        param.abi->type == bir::TypeKind::Ptr &&
        param.abi->sret_pointer) {
      return 0;
    }
    return std::nullopt;
  }
  if (target_profile.arch != c4c::TargetArch::Aarch64) {
    return param_index;
  }
  if (param.abi->type == bir::TypeKind::Ptr && param.abi->sret_pointer) {
    return 0;
  }

  return entry_formal_aarch64_register_slot_start(function, param_index);
}

[[nodiscard]] std::optional<abi::RegisterReference> entry_formal_source_register(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size()) {
    return std::nullopt;
  }
  const auto& param = function.params[param_index];
  if (!param.abi.has_value()) {
    return std::nullopt;
  }
  const auto abi_register_index =
      entry_formal_abi_register_index(target_profile, function, param_index);
  const auto register_name =
      abi_register_index.has_value()
          ? prepare::call_arg_destination_register_name(
                target_profile, *param.abi, *abi_register_index)
          : std::nullopt;
  const auto expected_view = entry_formal_register_view(param.type);
  if (!register_name.has_value() || !expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*register_name);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  if (parsed->bank == abi::RegisterBank::GeneralPurpose) {
    return abi::gp_register(parsed->index, *expected_view);
  }
  if (parsed->bank == abi::RegisterBank::FpSimd) {
    return abi::fp_simd_register(parsed->index, *expected_view);
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::string> entry_formal_store_lines(
    abi::RegisterReference source,
    bir::TypeKind type,
    std::size_t stack_offset_bytes,
    std::string_view base_register) {
  const auto opcode = entry_formal_store_opcode(type);
  if (!opcode.has_value()) {
    return {};
  }
  const std::string source_name{abi::register_name(source)};
  std::vector<std::string> lines;
  if (stack_offset_bytes <= 4095U) {
    std::string line = std::string{*opcode} + " " + source_name + ", [" +
                       std::string{base_register};
    if (stack_offset_bytes != 0U) {
      line += ", #";
      line += std::to_string(stack_offset_bytes);
    }
    line += "]";
    lines.push_back(std::move(line));
    return lines;
  }
  const auto scratch = abi::reserved_mir_scratch_gp_registers().front();
  lines = materialize_integer_constant_lines(scratch, stack_offset_bytes, 64);
  lines.push_back("add " + std::string{abi::register_name(scratch)} +
                  ", " + std::string{base_register} + ", " +
                  std::string{abi::register_name(scratch)});
  lines.push_back(std::string{*opcode} + " " + source_name + ", [" +
                  std::string{abi::register_name(scratch)} + "]");
  return lines;
}

[[nodiscard]] std::vector<std::string> entry_formal_load_lines(
    abi::RegisterReference destination,
    bir::TypeKind type,
    std::size_t stack_offset_bytes) {
  const auto opcode = entry_formal_load_opcode(type);
  if (!opcode.has_value()) {
    return {};
  }
  const std::string destination_name{abi::register_name(destination)};
  std::vector<std::string> lines;
  if (stack_offset_bytes <= 4095U) {
    std::string line = std::string{*opcode} + " " + destination_name + ", [sp";
    if (stack_offset_bytes != 0U) {
      line += ", #";
      line += std::to_string(stack_offset_bytes);
    }
    line += "]";
    lines.push_back(std::move(line));
    return lines;
  }
  const auto scratch = abi::reserved_mir_scratch_gp_registers()[1];
  lines = materialize_integer_constant_lines(scratch, stack_offset_bytes, 64);
  lines.push_back("add " + std::string{abi::register_name(scratch)} +
                  ", sp, " + std::string{abi::register_name(scratch)});
  lines.push_back(std::string{*opcode} + " " + destination_name + ", [" +
                  std::string{abi::register_name(scratch)} + "]");
  return lines;
}

[[nodiscard]] std::size_t entry_formal_stack_argument_size_bytes(
    const bir::CallArgAbiInfo& abi) {
  return align_to(std::max<std::size_t>(abi.size_bytes, 8), 8);
}

[[nodiscard]] std::size_t entry_formal_stack_argument_alignment_bytes(
    const bir::CallArgAbiInfo& abi) {
  const std::size_t abi_alignment = abi.align_bytes == 0 ? abi.size_bytes : abi.align_bytes;
  return std::min<std::size_t>(std::max<std::size_t>(abi_alignment, 8), 16);
}

[[nodiscard]] bool entry_formal_uses_incoming_stack(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size()) {
    return false;
  }
  const auto& param = function.params[param_index];
  if (!param.abi.has_value()) {
    return false;
  }
  if (param.abi->passed_on_stack) {
    return true;
  }
  return target_profile.arch == c4c::TargetArch::Aarch64 &&
         param.abi->passed_in_register &&
         entry_formal_aarch64_register_slot_start(function, param_index) == std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> entry_formal_incoming_stack_offset(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size() ||
      !entry_formal_uses_incoming_stack(target_profile, function, param_index)) {
    return std::nullopt;
  }

  std::size_t next_offset = 0;
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    const auto& param = function.params[index];
    if (!entry_formal_uses_incoming_stack(target_profile, function, index)) {
      continue;
    }
    next_offset = align_to(next_offset, entry_formal_stack_argument_alignment_bytes(*param.abi));
    if (index == param_index) {
      return next_offset;
    }
    next_offset += entry_formal_stack_argument_size_bytes(*param.abi);
  }
  return std::nullopt;
}

[[nodiscard]] bool function_has_call(const bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::CallInst>(inst)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::optional<std::size_t> entry_formal_frame_size_bytes(
    const module::FunctionLoweringContext& context) {
  const auto* frame = context.frame_plan;
  const auto* function = context.bir_function;
  if (frame == nullptr || function == nullptr ||
      frame->has_dynamic_stack || context.dynamic_stack_plan != nullptr) {
    return std::nullopt;
  }

  std::size_t frame_alignment =
      std::max<std::size_t>(frame->frame_alignment_bytes, kStackPointerAlignmentBytes);
  std::size_t prepared_frame_size = frame->frame_size_bytes;
  for (const auto& saved : frame->saved_callee_registers) {
    if (!saved.slot_placement.has_value() ||
        !saved.slot_placement->stack_offset_bytes.has_value() ||
        !saved.slot_placement->size_bytes.has_value() ||
        !saved.slot_placement->align_bytes.has_value()) {
      return std::nullopt;
    }
    prepared_frame_size = std::max(
        prepared_frame_size,
        *saved.slot_placement->stack_offset_bytes + *saved.slot_placement->size_bytes);
    frame_alignment = std::max(frame_alignment, *saved.slot_placement->align_bytes);
  }

  std::size_t frame_size = align_to(prepared_frame_size, frame_alignment);
  if (function_has_call(*function)) {
    frame_size = align_to(prepared_frame_size + 16, frame_alignment);
  }
  return frame_size == 0 ? std::optional<std::size_t>{}
                         : std::optional<std::size_t>{frame_size};
}

[[nodiscard]] std::string_view entry_formal_fixed_home_base_register(
    const module::FunctionLoweringContext& context) {
  return context.frame_plan != nullptr &&
                 context.frame_plan->uses_frame_pointer_for_fixed_slots
             ? std::string_view{"x29"}
             : std::string_view{"sp"};
}

void append_entry_formal_byte_store(std::vector<std::string>& lines,
                                    abi::RegisterReference byte_source,
                                    std::size_t stack_offset_bytes,
                                    std::string_view base_register) {
  const std::string source_name{abi::register_name(byte_source)};
  if (stack_offset_bytes <= 4095U) {
    std::string line = "strb " + source_name + ", [" + std::string{base_register};
    if (stack_offset_bytes != 0U) {
      line += ", #";
      line += std::to_string(stack_offset_bytes);
    }
    line += "]";
    lines.push_back(std::move(line));
    return;
  }

  const auto address_scratch = abi::reserved_mir_scratch_gp_registers()[1];
  const std::string address_name{abi::register_name(address_scratch)};
  auto address_lines =
      materialize_integer_constant_lines(address_scratch, stack_offset_bytes, 64);
  lines.insert(lines.end(), address_lines.begin(), address_lines.end());
  lines.push_back("add " + address_name + ", " + std::string{base_register} + ", " +
                  address_name);
  lines.push_back("strb " + source_name + ", [" + address_name + "]");
}

void append_entry_formal_byte_load(std::vector<std::string>& lines,
                                   abi::RegisterReference byte_destination,
                                   std::size_t stack_offset_bytes) {
  const std::string destination_name{abi::register_name(byte_destination)};
  if (stack_offset_bytes <= 4095U) {
    std::string line = "ldrb " + destination_name + ", [sp";
    if (stack_offset_bytes != 0U) {
      line += ", #";
      line += std::to_string(stack_offset_bytes);
    }
    line += "]";
    lines.push_back(std::move(line));
    return;
  }

  const auto address_scratch = abi::reserved_mir_scratch_gp_registers()[1];
  const std::string address_name{abi::register_name(address_scratch)};
  auto address_lines =
      materialize_integer_constant_lines(address_scratch, stack_offset_bytes, 64);
  lines.insert(lines.end(), address_lines.begin(), address_lines.end());
  lines.push_back("add " + address_name + ", sp, " + address_name);
  lines.push_back("ldrb " + destination_name + ", [" + address_name + "]");
}

[[nodiscard]] std::vector<std::string> entry_formal_byval_aggregate_store_lines(
    const bir::Param& param,
    abi::RegisterReference source,
    std::size_t stack_offset_bytes,
    std::string_view destination_base_register) {
  if (!param.abi.has_value() || param.type != bir::TypeKind::Ptr ||
      !param.abi->byval_copy || !param.abi->passed_in_register ||
      param.abi->primary_class != bir::AbiValueClass::Integer ||
      !abi::is_gp_register(source) || param.size_bytes == 0U) {
    return {};
  }

  const std::size_t lane_count = (param.size_bytes + 7U) / 8U;
  if (lane_count == 0U || lane_count > 2U ||
      source.index + lane_count > 8U) {
    return {};
  }

  const auto value_scratch = abi::reserved_mir_scratch_gp_registers()[0];
  const auto value_scratch_w =
      abi::gp_register(value_scratch.index, abi::RegisterView::W);
  if (!value_scratch_w.has_value()) {
    return {};
  }

  std::vector<std::string> lines;
  std::size_t byte_index = 0;
  while (byte_index + 8U <= param.size_bytes) {
    const std::size_t lane_index = byte_index / 8U;
    const auto lane_x =
        abi::gp_register(static_cast<std::uint8_t>(source.index + lane_index),
                         abi::RegisterView::X);
    if (!lane_x.has_value()) {
      return {};
    }
    auto lane_lines = entry_formal_store_lines(*lane_x,
                                               bir::TypeKind::I64,
                                               stack_offset_bytes + byte_index,
                                               destination_base_register);
    lines.insert(lines.end(), lane_lines.begin(), lane_lines.end());
    byte_index += 8U;
  }
  for (; byte_index < param.size_bytes; ++byte_index) {
    const std::size_t lane_index = byte_index / 8U;
    const std::size_t lane_byte = byte_index % 8U;
    const auto lane_x =
        abi::gp_register(static_cast<std::uint8_t>(source.index + lane_index),
                         abi::RegisterView::X);
    const auto lane_w =
        abi::gp_register(static_cast<std::uint8_t>(source.index + lane_index),
                         abi::RegisterView::W);
    if (!lane_x.has_value() || !lane_w.has_value()) {
      return {};
    }

    abi::RegisterReference byte_source = *lane_w;
    if (lane_byte != 0U) {
      lines.push_back("lsr " + std::string{abi::register_name(value_scratch)} +
                      ", " + std::string{abi::register_name(*lane_x)} +
                      ", #" + std::to_string(lane_byte * 8U));
      byte_source = *value_scratch_w;
    }
    append_entry_formal_byte_store(lines,
                                   byte_source,
                                   stack_offset_bytes + byte_index,
                                   destination_base_register);
  }
  return lines;
}

[[nodiscard]] std::vector<std::string>
entry_formal_byval_aggregate_stack_source_publication_lines(
    const bir::Param& param,
    const prepare::PreparedValueHome& home,
    std::size_t source_stack_offset_bytes,
    std::string_view destination_base_register) {
  if (!param.abi.has_value() || param.type != bir::TypeKind::Ptr ||
      !param.abi->byval_copy ||
      param.abi->primary_class != bir::AbiValueClass::Integer ||
      home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value() || param.size_bytes == 0U) {
    return {};
  }

  const auto value_scratch = abi::reserved_mir_scratch_gp_registers()[0];
  const auto value_scratch_w =
      abi::gp_register(value_scratch.index, abi::RegisterView::W);
  if (!value_scratch_w.has_value()) {
    return {};
  }

  std::vector<std::string> lines;
  for (std::size_t byte_index = 0; byte_index < param.size_bytes; ++byte_index) {
    append_entry_formal_byte_load(lines,
                                  *value_scratch_w,
                                  source_stack_offset_bytes + byte_index);
    append_entry_formal_byte_store(lines,
                                   *value_scratch_w,
                                   *home.offset_bytes + byte_index,
                                   destination_base_register);
  }
  return lines;
}

[[nodiscard]] std::optional<abi::RegisterReference> entry_formal_destination_register(
    const prepare::PreparedValueHome& home,
    bir::TypeKind type) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return std::nullopt;
  }
  const auto view = entry_formal_register_view(type);
  if (!view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  if (parsed->bank == abi::RegisterBank::GeneralPurpose) {
    return abi::gp_register(parsed->index, *view);
  }
  if (parsed->bank == abi::RegisterBank::FpSimd) {
    return abi::fp_simd_register(parsed->index, *view);
  }
  return std::nullopt;
}

void record_entry_formal_register_home(
    BlockScalarLoweringState& scalar_state,
    const prepare::PreparedValueHome& home,
    bir::TypeKind type) {
  const auto destination = entry_formal_destination_register(home, type);
  if (!destination.has_value()) {
    return;
  }
  prepare::PreparedRegisterClass prepared_class = prepare::PreparedRegisterClass::None;
  prepare::PreparedRegisterBank prepared_bank = prepare::PreparedRegisterBank::None;
  if (destination->bank == abi::RegisterBank::GeneralPurpose) {
    prepared_class = prepare::PreparedRegisterClass::General;
    prepared_bank = prepare::PreparedRegisterBank::Gpr;
  } else if (destination->bank == abi::RegisterBank::FpSimd) {
    prepared_class = (destination->view == abi::RegisterView::Q)
                         ? prepare::PreparedRegisterClass::Vector
                         : prepare::PreparedRegisterClass::Float;
    prepared_bank = (destination->view == abi::RegisterView::Q)
                        ? prepare::PreparedRegisterBank::Vreg
                        : prepare::PreparedRegisterBank::Fpr;
  }
  record_emitted_scalar_register(
      scalar_state,
      home.value_name,
      RegisterOperand{
          .reg = *destination,
          .role = RegisterOperandRole::StoragePlan,
          .value_id = home.value_id,
          .value_name = home.value_name,
          .prepared_class = prepared_class,
          .prepared_bank = prepared_bank,
          .expected_view = destination->view,
      });
}

[[nodiscard]] std::vector<std::string> entry_formal_stack_source_publication_lines(
    const module::BlockLoweringContext& context,
    const bir::Param& param,
    const prepare::PreparedValueHome& home,
    std::size_t param_index) {
  if (context.function.bir_function == nullptr) {
    return {};
  }
  const auto incoming_offset =
      entry_formal_incoming_stack_offset(context.function.prepared->target_profile,
                                         *context.function.bir_function,
                                         param_index);
  const auto frame_size = entry_formal_frame_size_bytes(context.function);
  if (!incoming_offset.has_value() || !frame_size.has_value()) {
    return {};
  }
  if (param.is_byval) {
    return entry_formal_byval_aggregate_stack_source_publication_lines(
        param,
        home,
        *frame_size + *incoming_offset,
        entry_formal_fixed_home_base_register(context.function));
  }
  if (home.kind == prepare::PreparedValueHomeKind::None &&
      param.type == bir::TypeKind::F128) {
    const auto scratch_base = abi::reserved_mir_scratch_fp_simd_registers().front();
    const auto scratch = abi::fp_simd_register(scratch_base.index, abi::RegisterView::Q);
    if (!scratch.has_value()) {
      return {};
    }
    auto lines = entry_formal_load_lines(*scratch,
                                         param.type,
                                         *frame_size + *incoming_offset);
    auto stores = entry_formal_store_lines(*scratch, param.type, *incoming_offset, "sp");
    lines.insert(lines.end(), stores.begin(), stores.end());
    return lines;
  }
  if (home.kind == prepare::PreparedValueHomeKind::Register) {
    const auto destination = entry_formal_destination_register(home, param.type);
    if (!destination.has_value()) {
      return {};
    }
    return entry_formal_load_lines(*destination, param.type, *frame_size + *incoming_offset);
  }
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value()) {
    return {};
  }
  const auto view = entry_formal_register_view(param.type);
  if (!view.has_value()) {
    return {};
  }
  const auto scratch_base = abi::reserved_mir_scratch_fp_simd_registers().front();
  const auto scratch = abi::fp_simd_register(scratch_base.index, *view);
  if (!scratch.has_value()) {
    return {};
  }
  auto lines = entry_formal_load_lines(*scratch, param.type, *frame_size + *incoming_offset);
  auto stores = entry_formal_store_lines(*scratch,
                                         param.type,
                                         *home.offset_bytes,
                                         entry_formal_fixed_home_base_register(context.function));
  lines.insert(lines.end(), stores.begin(), stores.end());
  return lines;
}

[[nodiscard]] std::vector<std::string> entry_formal_register_move_lines(
    abi::RegisterReference source,
    abi::RegisterReference destination) {
  if (source.bank != destination.bank) {
    return {};
  }
  if (source.index == destination.index) {
    return {};
  }
  const std::string source_name{abi::register_name(source)};
  const std::string destination_name{abi::register_name(destination)};
  const bool scalar_fp_register_move =
      source.bank == abi::RegisterBank::FpSimd &&
      (source.view == abi::RegisterView::S || source.view == abi::RegisterView::D) &&
      source.view == destination.view;
  return {std::string{scalar_fp_register_move ? "fmov" : "mov"} + " " +
          destination_name + ", " + source_name};
}

[[nodiscard]] module::MachineInstruction make_entry_formal_publication_instruction(
    const module::BlockLoweringContext& context,
    std::size_t param_index,
    std::vector<std::string> lines) {
  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = param_index,
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = std::move(asm_text),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = param_index,
          },
  };
}


}  // namespace

[[nodiscard]] std::vector<module::MachineInstruction> lower_entry_formal_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  if (context.block_index != 0 || context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.bir_function == nullptr) {
    return lowered;
  }
  const auto publication_plans = prepare::plan_prepared_formal_publications(
      prepare::PreparedFormalPublicationInputs{
          .names = &context.function.prepared->names,
          .function = context.function.bir_function,
          .value_locations = context.function.value_locations,
          .value_home_lookups = context.function.value_home_lookups,
      });
  for (const auto& publication : publication_plans) {
    if (!prepare::prepared_formal_publication_available(publication) ||
        publication.formal == nullptr ||
        publication.home == nullptr) {
      continue;
    }
    const auto param_index = publication.formal_index;
    const auto& param = *publication.formal;
    const auto& home = *publication.home;
    std::vector<std::string> lines;
    if (entry_formal_uses_incoming_stack(context.function.prepared->target_profile,
                                         *context.function.bir_function,
                                         param_index)) {
      lines = entry_formal_stack_source_publication_lines(context, param, home, param_index);
      if (!lines.empty() && !param.is_byval &&
          publication.home_kind == prepare::PreparedValueHomeKind::Register) {
        record_entry_formal_register_home(scalar_state, home, param.type);
      }
    } else {
      const auto source = entry_formal_source_register(
          context.function.prepared->target_profile,
          *context.function.bir_function,
          param_index);
      if (!source.has_value()) {
        continue;
      }
      if (publication.home_kind == prepare::PreparedValueHomeKind::StackSlot &&
          home.offset_bytes.has_value()) {
        if (param.is_byval) {
          lines = entry_formal_byval_aggregate_store_lines(param,
                                                           *source,
                                                           *home.offset_bytes,
                                                           entry_formal_fixed_home_base_register(
                                                               context.function));
        } else {
          lines = entry_formal_store_lines(*source,
                                           param.type,
                                           *home.offset_bytes,
                                           entry_formal_fixed_home_base_register(
                                               context.function));
        }
      } else if (!param.is_byval &&
                 publication.home_kind == prepare::PreparedValueHomeKind::Register) {
        const auto destination = entry_formal_destination_register(home, param.type);
        if (destination.has_value()) {
          lines = entry_formal_register_move_lines(*source, *destination);
          record_entry_formal_register_home(scalar_state, home, param.type);
        }
      }
    }
    if (lines.empty()) {
      continue;
    }
    lowered.push_back(make_entry_formal_publication_instruction(
        context, param_index, std::move(lines)));
  }
  return lowered;
}


}  // namespace c4c::backend::aarch64::codegen
