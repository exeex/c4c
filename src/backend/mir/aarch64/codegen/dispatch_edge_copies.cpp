#include "dispatch_edge_copies.hpp"

#include "dispatch.hpp"
#include "../abi/abi.hpp"
#include "alu.hpp"
#include "comparison.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_value_materialization.hpp"
#include "fp_value_materialization.hpp"
#include "instruction.hpp"
#include "memory.hpp"
#include "operands.hpp"
#include "prepared_value_home_materialization.hpp"
#include "select_materialization.hpp"
#include "variadic.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "../../../prealloc/publication_plans.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

}  // namespace

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block);

[[nodiscard]] bool prepared_edge_select_source_is_destination_register(
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home) {
  return prepare::prepared_value_homes_share_register_name(source_home, destination_home);
}
[[nodiscard]] std::optional<module::BlockLoweringContext>
prepared_edge_publication_producer_block_context(
    const module::BlockLoweringContext& context,
    c4c::BlockLabelId block_label) {
  if (context.function.control_flow == nullptr ||
      block_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < context.function.control_flow->blocks.size(); ++index) {
    const auto& block = context.function.control_flow->blocks[index];
    if (block.block_label != block_label) {
      continue;
    }
    return module::BlockLoweringContext{
        .function = context.function,
        .control_flow_block = &block,
        .bir_block = find_bir_block(context.function, block),
        .block_index = index,
    };
  }
  return std::nullopt;
}
[[nodiscard]] bool prepared_edge_publication_source_matches_value(
    const prepare::PreparedEdgePublication& publication,
    const bir::Value& value) {
  if (publication.source_value_kind != value.kind ||
      publication.source_value.type != value.type) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return publication.source_value.immediate == value.immediate;
  }
  if (value.kind == bir::Value::Kind::Named) {
    return publication.source_value.name == value.name;
  }
  return false;
}
[[nodiscard]] std::optional<EdgeProducerContext>
prepared_edge_publication_producer_context(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublication& publication) {
  if (!publication.source_producer_block_label.has_value() ||
      !publication.source_producer_instruction_index.has_value()) {
    return std::nullopt;
  }
  auto producer_context =
      prepared_edge_publication_producer_block_context(
          context, *publication.source_producer_block_label);
  if (!producer_context.has_value() || producer_context->bir_block == nullptr ||
      *publication.source_producer_instruction_index >=
          producer_context->bir_block->insts.size()) {
    return std::nullopt;
  }
  const auto& inst =
      producer_context->bir_block->insts[*publication.source_producer_instruction_index];
  switch (publication.source_producer_kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      if (publication.source_load_local != std::get_if<bir::LoadLocalInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      if (publication.source_load_global != std::get_if<bir::LoadGlobalInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      if (publication.source_cast != std::get_if<bir::CastInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      if (publication.source_binary != std::get_if<bir::BinaryInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      if (publication.source_select != std::get_if<bir::SelectInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return std::nullopt;
  }
  return EdgeProducerContext{
      .context = *producer_context,
      .producer = &inst,
      .instruction_index = *publication.source_producer_instruction_index,
  };
}
[[nodiscard]] std::optional<EdgeProducerContext>
prepared_edge_source_producer_context(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducer& producer) {
  if (producer.block_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  auto producer_context =
      prepared_edge_publication_producer_block_context(context, producer.block_label);
  if (!producer_context.has_value() || producer_context->bir_block == nullptr ||
      producer.instruction_index >= producer_context->bir_block->insts.size()) {
    return std::nullopt;
  }
  const auto& inst = producer_context->bir_block->insts[producer.instruction_index];
  switch (producer.kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      if (producer.load_local != std::get_if<bir::LoadLocalInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      if (producer.load_global != std::get_if<bir::LoadGlobalInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      if (producer.cast != std::get_if<bir::CastInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      if (producer.binary != std::get_if<bir::BinaryInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      if (producer.select != std::get_if<bir::SelectInst>(&inst)) {
        return std::nullopt;
      }
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return std::nullopt;
  }
  return EdgeProducerContext{
      .context = *producer_context,
      .producer = &inst,
      .instruction_index = producer.instruction_index,
  };
}
[[nodiscard]] std::optional<EdgeProducerContext>
prepared_edge_named_source_producer_context(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value() || context.function.prepared_lookups == nullptr) {
    return std::nullopt;
  }
  const auto* producer =
      prepare::find_indexed_prepared_edge_publication_source_producer(
          &context.function.prepared_lookups->edge_publication_source_producers,
          *value_name);
  return producer != nullptr
             ? prepared_edge_source_producer_context(context, *producer)
             : std::nullopt;
}

[[nodiscard]] bool prepared_edge_copy_source_facts_have_materializable_producer(
    const prepare::PreparedEdgeCopySourceFacts& facts) {
  return facts.status == prepare::PreparedEdgeCopySourceFactsStatus::Available &&
         facts.publication != nullptr &&
         facts.source_producer_kind !=
             prepare::PreparedEdgePublicationSourceProducerKind::Unknown &&
         facts.source_producer_kind !=
             prepare::PreparedEdgePublicationSourceProducerKind::Immediate;
}

struct EdgeSelectChainState {
  std::size_t label_index = 0;
  std::vector<std::string_view> active_values;
  std::optional<module::BlockLoweringContext> root_context;
  std::size_t root_instruction_index = 0;
  c4c::ValueNameId root_value_name = c4c::kInvalidValueName;
};

[[nodiscard]] bool emit_edge_value_publication_to_register_impl(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    const prepare::PreparedEdgePublication* prepared_publication,
    EdgeSelectChainState& select_chain_state);

[[nodiscard]] bool emit_edge_load_local_to_register_impl(
    const module::BlockLoweringContext& edge_context,
    const EdgeProducerContext& producer,
    const bir::LoadLocalInst& load,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    const prepare::PreparedEdgePublication* prepared_publication,
    EdgeSelectChainState& select_chain_state);

[[nodiscard]] bool prepared_publication_source_home_matches_source(
    const prepare::PreparedEdgePublication& publication) {
  return publication.source_home != nullptr &&
         publication.source_home->value_name == publication.source_value_name &&
         publication.source_home->kind == publication.source_home_kind;
}
[[nodiscard]] bool prepared_publication_source_memory_matches_access(
    const prepare::PreparedEdgePublication& publication,
    const prepare::PreparedMemoryAccess& access) {
  return access.result_value_name.has_value() &&
         *access.result_value_name == publication.source_value_name &&
         publication.source_memory_base_kind == access.address.base_kind &&
         publication.source_memory_frame_slot_id == access.address.frame_slot_id &&
         publication.source_memory_symbol_name == access.address.symbol_name &&
         publication.source_memory_pointer_value_name ==
             access.address.pointer_value_name &&
         publication.source_memory_byte_offset == access.address.byte_offset &&
         publication.source_memory_size_bytes == access.address.size_bytes &&
         publication.source_memory_align_bytes == access.address.align_bytes &&
         publication.source_memory_address_space == access.address_space &&
         publication.source_memory_is_volatile == access.is_volatile &&
         publication.source_memory_can_use_base_plus_offset ==
             access.address.can_use_base_plus_offset;
}
[[nodiscard]] const prepare::PreparedMemoryAccess*
prepared_publication_source_memory_access(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublication& publication,
    const bir::LoadLocalInst& load) {
  if (!prepared_edge_publication_source_matches_value(publication, load.result) ||
      publication.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      publication.source_load_local != &load ||
      publication.source_memory_access_status !=
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available ||
      publication.source_memory_access == nullptr ||
      !prepared_publication_source_home_matches_source(publication) ||
      !prepared_memory_access_matches_instruction(
          context, publication.source_memory_access, load) ||
      !prepared_publication_source_memory_matches_access(
          publication, *publication.source_memory_access)) {
    return nullptr;
  }
  return publication.source_memory_access;
}
[[nodiscard]] std::optional<abi::RegisterReference>
prepared_publication_source_register(
    const prepare::PreparedEdgePublication& publication,
    const bir::Value& value) {
  if (!prepared_edge_publication_source_matches_value(publication, value) ||
      !prepared_publication_source_home_matches_source(publication) ||
      publication.source_home == nullptr ||
      publication.source_home->kind != prepare::PreparedValueHomeKind::Register ||
      !publication.source_home->register_name.has_value()) {
    return std::nullopt;
  }
  const auto expected_view = scalar_view_for_type(value.type);
  const auto parsed =
      abi::parse_aarch64_register_name(*publication.source_home->register_name);
  if (!expected_view.has_value() ||
      !parsed.has_value() ||
      parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  return abi::gp_register(parsed->index, *expected_view);
}
[[nodiscard]] bool edge_value_publication_may_read_register_index(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t,
    std::uint8_t register_index,
    const prepare::PreparedEdgePublication* prepared_publication,
    unsigned depth) {
  if (prepared_publication == nullptr) {
    return false;
  }
  if (depth == 0U && prepared_publication != nullptr &&
      !prepared_edge_publication_source_matches_value(*prepared_publication, value)) {
    return false;
  }
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  const bool require_prepared_producer =
      prepared_publication != nullptr &&
      prepared_edge_publication_source_matches_value(*prepared_publication, value);
  const auto producer =
      require_prepared_producer
          ? prepared_edge_publication_producer_context(edge_context, *prepared_publication)
          : prepared_edge_named_source_producer_context(edge_context, value);
  if (!producer.has_value() || producer->producer == nullptr) {
    const auto* home =
        require_prepared_producer && prepared_publication->source_home != nullptr
            ? prepared_publication->source_home
            : prepared_value_home_for_value(successor_context, value);
    return home != nullptr &&
           prepared_value_home_reads_register_index(*home, register_index);
  }
  const auto& dependency_successor_context =
      producer->context;
  if (const auto* cast = std::get_if<bir::CastInst>(producer->producer);
      cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    return edge_value_publication_may_read_register_index(edge_context,
                                                          dependency_successor_context,
                                                          operand,
                                                          producer->instruction_index,
                                                          register_index,
                                                          prepared_publication,
                                                          depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer->producer);
      binary != nullptr) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    return edge_value_publication_may_read_register_index(edge_context,
                                                          dependency_successor_context,
                                                          lhs,
                                                          producer->instruction_index,
                                                          register_index,
                                                          prepared_publication,
                                                          depth + 1) ||
           edge_value_publication_may_read_register_index(edge_context,
                                                          dependency_successor_context,
                                                          rhs,
                                                          producer->instruction_index,
                                                          register_index,
                                                          prepared_publication,
                                                          depth + 1);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(producer->producer);
      select != nullptr) {
    return edge_value_publication_may_read_register_index(edge_context,
                                                          dependency_successor_context,
                                                          select->true_value,
                                                          producer->instruction_index,
                                                          register_index,
                                                          prepared_publication,
                                                          depth + 1) ||
           edge_value_publication_may_read_register_index(edge_context,
                                                          dependency_successor_context,
                                                          select->false_value,
                                                          producer->instruction_index,
                                                          register_index,
                                                          prepared_publication,
                                                          depth + 1);
  }
  if (const auto* load = std::get_if<bir::LoadLocalInst>(producer->producer);
      load != nullptr &&
      prepared_va_list_field_address(producer->context, load->slot_name).has_value()) {
    const auto* home = prepared_value_home_for_value(producer->context, load->result);
    return home != nullptr &&
           prepared_value_home_reads_register_index(*home, register_index);
  }
  return false;
}
[[nodiscard]] bool emit_edge_load_local_to_register(
    const module::BlockLoweringContext& edge_context,
    const EdgeProducerContext& producer,
    const bir::LoadLocalInst& load,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    const prepare::PreparedEdgePublication* prepared_publication) {
  if (prepared_publication == nullptr) {
    return false;
  }
  EdgeSelectChainState select_chain_state;
  return emit_edge_load_local_to_register_impl(edge_context,
                                               producer,
                                               load,
                                               target_index,
                                               scratch_index,
                                               lines,
                                               prepared_publication,
                                               select_chain_state);
}

[[nodiscard]] bool emit_edge_load_local_to_register_impl(
    const module::BlockLoweringContext& edge_context,
    const EdgeProducerContext& producer,
    const bir::LoadLocalInst& load,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    const prepare::PreparedEdgePublication* prepared_publication,
    EdgeSelectChainState& select_chain_state) {
  const auto mnemonic = scalar_load_mnemonic(load.result.type);
  const auto target_view = scalar_view_for_type(load.result.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  if (!mnemonic.has_value() || !target.has_value()) {
    return false;
  }
  if (emit_prepared_va_list_field_carrier_to_register(
          producer.context, load, target_index, lines)) {
    return true;
  }
  if (emit_prepared_va_list_field_load_to_register(
          producer.context, load, target_index, lines)) {
    return true;
  }
  const bool require_prepared_source_memory =
      prepared_publication != nullptr &&
      prepared_edge_publication_source_matches_value(*prepared_publication,
                                                     load.result);
  const auto* access =
      require_prepared_source_memory
          ? prepared_publication_source_memory_access(
                producer.context, *prepared_publication, load)
          : prepared_memory_access(producer.context, producer.instruction_index);
  if (access == nullptr) {
    if (require_prepared_source_memory) {
      return false;
    }
    if (prepared_publication != nullptr) {
      if (const auto source =
              prepared_publication_source_register(*prepared_publication,
                                                   load.result)) {
        const auto source_name = abi::register_name(*source);
        lines.push_back("mov " + *target + ", " + source_name);
        return true;
      }
    }
    const auto* home = prepared_value_home_for_value(producer.context, load.result);
    return home != nullptr &&
           emit_prepared_value_home_to_register(
               producer.context.function.prepared != nullptr
                   ? &producer.context.function.prepared->stack_layout
                   : nullptr,
               *home,
               load.result.type,
               target_index,
               lines,
               fixed_slots_use_frame_pointer(producer.context.function));
  }
  const auto base_kind =
      require_prepared_source_memory ? prepared_publication->source_memory_base_kind
                                     : access->address.base_kind;
  const auto frame_slot_id =
      require_prepared_source_memory ? prepared_publication->source_memory_frame_slot_id
                                     : access->address.frame_slot_id;
  const auto pointer_value_name =
      require_prepared_source_memory
          ? prepared_publication->source_memory_pointer_value_name
          : access->address.pointer_value_name;
  const auto byte_offset =
      require_prepared_source_memory ? prepared_publication->source_memory_byte_offset
                                     : access->address.byte_offset;
  const auto can_use_base_plus_offset =
      require_prepared_source_memory
          ? prepared_publication->source_memory_can_use_base_plus_offset
          : access->address.can_use_base_plus_offset;
  if (base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
      can_use_base_plus_offset &&
      frame_slot_id.has_value()) {
    if (producer.context.function.prepared == nullptr) {
      return false;
    }
    const auto* slot =
        find_frame_slot(producer.context.function.prepared->stack_layout,
                        *frame_slot_id);
    if (slot == nullptr) {
      return false;
    }
    lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                    frame_slot_address(producer.context.function,
                                       slot->offset_bytes +
                                           static_cast<std::size_t>(
                                               byte_offset)));
    return true;
  }
  if (base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !pointer_value_name.has_value() ||
      !can_use_base_plus_offset ||
      producer.context.function.prepared == nullptr) {
    return false;
  }
  const auto pointer_name = prepare::prepared_value_name(
      producer.context.function.prepared->names, *pointer_value_name);
  if (pointer_name.empty()) {
    return false;
  }
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!address.has_value()) {
    return false;
  }
  const auto nested_scratch = scratch_index == 9 ? 10 : 9;
  if (!emit_edge_value_publication_to_register_impl(
          edge_context,
          producer.context,
          bir::Value::named(bir::TypeKind::Ptr, std::string{pointer_name}),
          producer.instruction_index,
          scratch_index,
          nested_scratch,
          lines,
          prepared_publication,
          select_chain_state)) {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                  register_indirect_address(*address,
                                            static_cast<std::size_t>(
                                                byte_offset)));
  return true;
}
[[nodiscard]] bool emit_edge_value_publication_to_register(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t successor_before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    const prepare::PreparedEdgePublication* prepared_publication) {
  if (prepared_publication == nullptr ||
      !prepared_edge_publication_source_matches_value(*prepared_publication, value)) {
    return false;
  }
  EdgeSelectChainState select_chain_state;
  return emit_edge_value_publication_to_register_impl(edge_context,
                                                      successor_context,
                                                      value,
                                                      successor_before_instruction_index,
                                                      target_index,
                                                      scratch_index,
                                                      lines,
                                                      prepared_publication,
                                                      select_chain_state);
}

[[nodiscard]] bool emit_edge_value_publication_to_register_impl(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    const prepare::PreparedEdgePublication* prepared_publication,
    EdgeSelectChainState& select_chain_state) {
  const auto target_view = scalar_view_for_type(value.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  if (!target_view.has_value() || !target.has_value()) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    lines.push_back("mov " + *target + ", #" + std::to_string(value.immediate));
    return true;
  }
  if (value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const bool require_prepared_producer =
      prepared_publication != nullptr &&
      prepared_edge_publication_source_matches_value(*prepared_publication, value);
  const auto producer =
      require_prepared_producer
          ? prepared_edge_publication_producer_context(edge_context, *prepared_publication)
          : prepared_edge_named_source_producer_context(edge_context, value);
  if (require_prepared_producer && !producer.has_value()) {
    return false;
  }
  if (!producer.has_value() || producer->producer == nullptr) {
    const auto* home = prepared_value_home_for_value(successor_context, value);
    return home != nullptr &&
           emit_prepared_value_home_to_register(
               successor_context.function.prepared != nullptr
                   ? &successor_context.function.prepared->stack_layout
                   : nullptr,
               *home,
               value.type,
               target_index,
               lines,
               fixed_slots_use_frame_pointer(successor_context.function));
  }
  if (const auto* load = std::get_if<bir::LoadLocalInst>(producer->producer);
      load != nullptr) {
    std::vector<std::string> load_lines;
    if (emit_edge_load_local_to_register_impl(
            edge_context,
            *producer,
            *load,
            target_index,
            scratch_index,
            load_lines,
            require_prepared_producer ? prepared_publication : nullptr,
            select_chain_state)) {
      lines.insert(lines.end(), load_lines.begin(), load_lines.end());
      return true;
    }
    if (require_prepared_producer &&
        prepared_publication->source_producer_kind ==
            prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) {
      return false;
    }
    const auto* home =
        require_prepared_producer && prepared_publication->source_home != nullptr
            ? prepared_publication->source_home
            : prepared_value_home_for_value(
                  require_prepared_producer ? producer->context : successor_context,
                  value);
    return home != nullptr &&
           emit_prepared_value_home_to_register(
               producer->context.function.prepared != nullptr
                   ? &producer->context.function.prepared->stack_layout
                   : nullptr,
               *home,
               value.type,
               target_index,
               lines,
               fixed_slots_use_frame_pointer(producer->context.function));
  }
  const auto* dependency_publication =
      prepared_publication;
  const auto& dependency_successor_context =
      producer->context;
  if (const auto* cast = std::get_if<bir::CastInst>(producer->producer);
      cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                      dependency_successor_context,
                                                      operand,
                                                      producer->instruction_index,
                                                      target_index,
                                                      scratch_index,
                                                      lines,
                                                      dependency_publication,
                                                      select_chain_state)) {
      return false;
    }
    const auto source_bits = integer_bit_width(cast->operand.type);
    const auto result_bits = integer_bit_width(cast->result.type);
    const auto source_name = gp_register_name(target_index, abi::RegisterView::W);
    const auto target_name = gp_register_name(target_index, *target_view);
    if (!source_bits.has_value() || !result_bits.has_value() ||
        !source_name.has_value() || !target_name.has_value()) {
      return false;
    }
    if (cast->opcode == bir::CastOpcode::SExt) {
      if (*source_bits == 8U) {
        lines.push_back("sxtb " + *target_name + ", " + *source_name);
      } else if (*source_bits == 16U) {
        lines.push_back("sxth " + *target_name + ", " + *source_name);
      } else if (*source_bits == 32U && *result_bits == 64U) {
        lines.push_back("sxtw " + *target_name + ", " + *source_name);
      } else if (*source_bits != *result_bits) {
        return false;
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::ZExt) {
      if (*source_bits < 32U || *source_bits < *result_bits) {
        const auto widened = gp_register_name(target_index, abi::RegisterView::X);
        if (!widened.has_value()) {
          return false;
        }
        if (*source_bits < 32U) {
          lines.push_back("ubfx " + *widened + ", " + *widened +
                          ", #0, #" + std::to_string(*source_bits));
        }
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::Trunc) {
      return true;
    }
    return false;
  }
  if (const auto* select = std::get_if<bir::SelectInst>(producer->producer);
      select != nullptr) {
    const auto condition = branch_condition_suffix(select->predicate);
    const auto compare_view = scalar_view_for_type(select->compare_type);
    const auto lhs_name =
        compare_view.has_value() ? gp_register_name(target_index, *compare_view)
                                 : std::nullopt;
    if (!condition.has_value() || !compare_view.has_value() ||
        !lhs_name.has_value()) {
      return false;
    }

    for (const auto active : select_chain_state.active_values) {
      if (active == value.name) {
        return false;
      }
    }
    const bool seeded_root_context = !select_chain_state.root_context.has_value();
    if (seeded_root_context) {
      select_chain_state.root_context = dependency_successor_context;
      select_chain_state.root_instruction_index = producer->instruction_index;
      select_chain_state.root_value_name =
          prepared_publication != nullptr
              ? prepared_publication->source_value_name
              : prepared_named_value_id(dependency_successor_context, value)
                    .value_or(c4c::kInvalidValueName);
    }

    select_chain_state.active_values.push_back(value.name);
    const auto current_label = select_chain_state.label_index++;
    const auto fail_select = [&]() {
      select_chain_state.active_values.pop_back();
      if (seeded_root_context) {
        select_chain_state.root_context.reset();
        select_chain_state.root_instruction_index = 0;
        select_chain_state.root_value_name = c4c::kInvalidValueName;
      }
      return false;
    };
    const auto true_label =
        select_chain_local_label_reference(current_label, "true");
    const auto true_definition =
        select_chain_local_label_definition(current_label, "true");
    const auto end_label =
        select_chain_local_label_reference(current_label, "end");
    const auto end_definition =
        select_chain_local_label_definition(current_label, "end");

    auto lhs = select->lhs;
    lhs.type = select->compare_type;
    auto rhs = select->rhs;
    rhs.type = select->compare_type;
    std::optional<std::string> rhs_name;
    const auto nested_scratch = scratch_index == 9 ? 10 : 9;
    if (rhs.kind == bir::Value::Kind::Immediate &&
        is_cmp_immediate_encodable(rhs.immediate)) {
      rhs_name = "#" + std::to_string(rhs.immediate);
      if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                        dependency_successor_context,
                                                        lhs,
                                                        producer->instruction_index,
                                                        target_index,
                                                        scratch_index,
                                                        lines,
                                                        dependency_publication,
                                                        select_chain_state)) {
        return fail_select();
      }
    } else {
      rhs_name = gp_register_name(scratch_index, *compare_view);
      const bool rhs_reads_target = edge_value_publication_may_read_register_index(
          edge_context,
          dependency_successor_context,
          rhs,
          producer->instruction_index,
          target_index,
          dependency_publication,
          dependency_publication != nullptr ? 1U : 0U);
      const bool lhs_reads_scratch = edge_value_publication_may_read_register_index(
          edge_context,
          dependency_successor_context,
          lhs,
          producer->instruction_index,
          scratch_index,
          dependency_publication,
          dependency_publication != nullptr ? 1U : 0U);
      if (!rhs_name.has_value()) {
        return fail_select();
      }
      if (rhs_reads_target && !lhs_reads_scratch) {
        if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                          dependency_successor_context,
                                                          rhs,
                                                          producer->instruction_index,
                                                          scratch_index,
                                                          nested_scratch,
                                                          lines,
                                                          dependency_publication,
                                                          select_chain_state) ||
            !emit_edge_value_publication_to_register_impl(edge_context,
                                                          dependency_successor_context,
                                                          lhs,
                                                          producer->instruction_index,
                                                          target_index,
                                                          scratch_index,
                                                          lines,
                                                          dependency_publication,
                                                          select_chain_state)) {
          return fail_select();
        }
      } else if (!emit_edge_value_publication_to_register_impl(
                     edge_context,
                     dependency_successor_context,
                     lhs,
                     producer->instruction_index,
                     target_index,
                     scratch_index,
                     lines,
                     dependency_publication,
                     select_chain_state) ||
                 !emit_edge_value_publication_to_register_impl(
                     edge_context,
                     dependency_successor_context,
                     rhs,
                     producer->instruction_index,
                     scratch_index,
                     nested_scratch,
                     lines,
                     dependency_publication,
                     select_chain_state)) {
        return fail_select();
      }
    }
    lines.push_back("cmp " + *lhs_name + ", " + *rhs_name);
    lines.push_back("b." + std::string{*condition} + " " + true_label);

    auto false_value = select->false_value;
    false_value.type = select->result.type;
    if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                      dependency_successor_context,
                                                      false_value,
                                                      producer->instruction_index,
                                                      target_index,
                                                      scratch_index,
                                                      lines,
                                                      dependency_publication,
                                                      select_chain_state)) {
      return fail_select();
    }
    lines.push_back("b " + end_label);
    lines.push_back(true_definition);

    auto true_value = select->true_value;
    true_value.type = select->result.type;
    if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                      dependency_successor_context,
                                                      true_value,
                                                      producer->instruction_index,
                                                      target_index,
                                                      scratch_index,
                                                      lines,
                                                      dependency_publication,
                                                      select_chain_state)) {
      return fail_select();
    }
    lines.push_back(end_definition);
    select_chain_state.active_values.pop_back();
    if (seeded_root_context) {
      select_chain_state.root_context.reset();
      select_chain_state.root_instruction_index = 0;
      select_chain_state.root_value_name = c4c::kInvalidValueName;
    }
    return true;
  }
  const auto* binary = std::get_if<bir::BinaryInst>(producer->producer);
  if (binary == nullptr) {
    if (require_prepared_producer) {
      return false;
    }
    return emit_value_publication_to_register(producer->context,
                                              value,
                                              producer->instruction_index + 1,
                                              target_index,
                                              scratch_index,
                                              lines);
  }
  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  if (const auto condition = branch_condition_suffix(binary->opcode);
      condition.has_value()) {
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto lhs_name =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto result_name = gp_register_name(target_index, *target_view);
    if (!operand_view.has_value() || !lhs_name.has_value() ||
        !result_name.has_value()) {
      return false;
    }
    if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                      dependency_successor_context,
                                                      lhs,
                                                      producer->instruction_index,
                                                      target_index,
                                                      scratch_index,
                                                      lines,
                                                      dependency_publication,
                                                      select_chain_state)) {
      return false;
    }
    std::optional<std::string> rhs_name;
    if (rhs.kind == bir::Value::Kind::Immediate && is_cmp_immediate_encodable(rhs.immediate)) {
      rhs_name = "#" + std::to_string(rhs.immediate);
    } else {
      rhs_name = gp_register_name(scratch_index, *operand_view);
      const auto nested_scratch = scratch_index == 9 ? 10 : 9;
      if (!rhs_name.has_value() ||
          !emit_edge_value_publication_to_register_impl(edge_context,
                                                        dependency_successor_context,
                                                        rhs,
                                                        producer->instruction_index,
                                                        scratch_index,
                                                        nested_scratch,
                                                        lines,
                                                        dependency_publication,
                                                        select_chain_state)) {
        return false;
      }
    }
    lines.push_back("cmp " + *lhs_name + ", " + *rhs_name);
    lines.push_back("cset " + *result_name + ", " + std::string(*condition));
    return true;
  }
  if (binary->opcode == bir::BinaryOpcode::Add ||
      binary->opcode == bir::BinaryOpcode::Sub ||
      binary->opcode == bir::BinaryOpcode::Mul) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    const std::uint8_t nested_scratch = scratch_index == 9 ? 10 : 9;
    if (binary->opcode == bir::BinaryOpcode::Mul &&
        rhs.kind == bir::Value::Kind::Immediate &&
        rhs.immediate >= 0) {
      if (const auto shift =
              power_of_two_shift(static_cast<std::uint64_t>(rhs.immediate))) {
        if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                          dependency_successor_context,
                                                          lhs,
                                                          producer->instruction_index,
                                                          target_index,
                                                          scratch_index,
                                                          lines,
                                                          dependency_publication,
                                                          select_chain_state)) {
          return false;
        }
        const auto result =
            gp_register_name(target_index,
                             scalar_view_for_type(binary->operand_type)
                                 .value_or(abi::RegisterView::X));
        if (!result.has_value()) {
          return false;
        }
        if (*shift != 0U) {
          lines.push_back("lsl " + *result + ", " + *result +
                          ", #" + std::to_string(*shift));
        }
        return true;
      }
    }
    const bool rhs_reads_target = edge_value_publication_may_read_register_index(
        edge_context,
        dependency_successor_context,
        rhs,
        producer->instruction_index,
        target_index,
        dependency_publication,
        dependency_publication != nullptr ? 1U : 0U);
    const bool lhs_reads_scratch = edge_value_publication_may_read_register_index(
        edge_context,
        dependency_successor_context,
        lhs,
        producer->instruction_index,
        scratch_index,
        dependency_publication,
        dependency_publication != nullptr ? 1U : 0U);
    if (rhs_reads_target && !lhs_reads_scratch) {
      if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                        dependency_successor_context,
                                                        rhs,
                                                        producer->instruction_index,
                                                        scratch_index,
                                                        nested_scratch,
                                                        lines,
                                                        dependency_publication,
                                                        select_chain_state) ||
          !emit_edge_value_publication_to_register_impl(edge_context,
                                                        dependency_successor_context,
                                                        lhs,
                                                        producer->instruction_index,
                                                        target_index,
                                                        scratch_index,
                                                        lines,
                                                        dependency_publication,
                                                        select_chain_state)) {
        return false;
      }
    } else {
      if (!emit_edge_value_publication_to_register_impl(edge_context,
                                                        dependency_successor_context,
                                                        lhs,
                                                        producer->instruction_index,
                                                        target_index,
                                                        scratch_index,
                                                        lines,
                                                        dependency_publication,
                                                        select_chain_state) ||
          !emit_edge_value_publication_to_register_impl(edge_context,
                                                        dependency_successor_context,
                                                        rhs,
                                                        producer->instruction_index,
                                                        scratch_index,
                                                        nested_scratch,
                                                        lines,
                                                        dependency_publication,
                                                        select_chain_state)) {
        return false;
      }
    }
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto result =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto rhs_name =
        operand_view.has_value() ? gp_register_name(scratch_index, *operand_view)
                                 : std::nullopt;
    if (!operand_view.has_value() || !result.has_value() || !rhs_name.has_value()) {
      return false;
    }
    std::string_view opcode = "add";
    if (binary->opcode == bir::BinaryOpcode::Sub) {
      opcode = "sub";
    } else if (binary->opcode == bir::BinaryOpcode::Mul) {
      opcode = "mul";
    }
    lines.push_back(std::string{opcode} + " " + *result + ", " + *result +
                    ", " + *rhs_name);
    return true;
  }
  if (require_prepared_producer) {
    return false;
  }
  return emit_value_publication_to_register(producer->context,
                                            value,
                                            producer->instruction_index + 1,
                                            target_index,
                                            scratch_index,
                                            lines);
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_predecessor_join_source_publication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublication& publication,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home,
    BlockScalarLoweringState& scalar_state) {
  if (source_home.value_name == c4c::kInvalidValueName ||
      destination_home.kind != prepare::PreparedValueHomeKind::Register ||
      !destination_home.register_name.has_value()) {
    return std::nullopt;
  }
  if (publication.source_producer_kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::Unknown ||
      publication.source_producer_kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::Immediate) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*destination_home.register_name);
  if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  std::uint8_t scratch_index = scratches.front().index;
  for (const auto scratch : scratches) {
    if (scratch.index != parsed->index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (scratch_index == parsed->index) {
    return std::nullopt;
  }
  const auto typed_stack_source =
      prepare::prepare_same_width_i32_stack_source_publication(&publication);
  std::vector<std::string> lines;
  std::optional<abi::RegisterReference> loaded_register;
  if (auto typed_emission =
          emit_same_width_i32_stack_source_publication(context, typed_stack_source)) {
    loaded_register = typed_emission->destination_register;
    lines = std::move(typed_emission->lines);
  } else {
    auto producer = prepared_edge_publication_producer_context(context, publication);
    if (!producer.has_value() || producer->context.bir_block == nullptr) {
      return std::nullopt;
    }
    auto successor_context = producer->context;
    if (!emit_edge_value_publication_to_register(context,
                                                 successor_context,
                                                 publication.source_value,
                                                 producer->instruction_index + 1,
                                                 parsed->index,
                                                 scratch_index,
                                                 lines,
                                                 &publication) ||
        lines.empty()) {
      return std::nullopt;
    }
  }
  const auto expected_view =
      scalar_view_for_type(publication.source_value.type).value_or(parsed->view);
  auto source_reg = loaded_register.value_or(
      abi::gp_register(parsed->index, expected_view).value_or(*parsed));
  RegisterOperand emitted{
      .reg = source_reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = source_home.value_id,
      .value_name = source_home.value_name,
      .expected_view = expected_view,
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  RegisterOperand destination = emitted;
  destination.value_id = destination_home.value_id;
  destination.value_name = destination_home.value_name;
  record_emitted_scalar_register(scalar_state, destination.value_name, destination);
  return make_select_chain_materialization_instruction(
      context, context.bir_block != nullptr ? context.bir_block->insts.size() : 0U,
      std::move(lines));
}
[[nodiscard]] bool should_emit_block_entry_edge_copy_move(
    const module::BlockLoweringContext& context,
    const module::MachineInstruction& instruction) {
  if (block_entry_move_clobbers_current_join_publication(context, instruction)) {
    return false;
  }
  const auto* move =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move != nullptr && move->destination_register.has_value() &&
      move->destination_register->reg.bank == abi::RegisterBank::GeneralPurpose &&
      context.function.prepared_lookups != nullptr &&
      context.control_flow_block != nullptr) {
    for (const auto& publication :
         context.function.prepared_lookups->edge_publications.publications) {
      const auto source_facts = prepare::prepare_edge_copy_source_facts(
          &context.function.prepared_lookups->edge_publications,
          publication.predecessor_label,
          publication.successor_label,
          publication.destination_value_id);
      if (!prepared_edge_copy_source_facts_have_materializable_producer(source_facts) ||
          source_facts.publication->phase != prepare::PreparedMovePhase::BlockEntry ||
          source_facts.predecessor_label != context.control_flow_block->block_label) {
        continue;
      }
      auto producer =
          prepared_edge_publication_producer_context(context, *source_facts.publication);
      if (!producer.has_value()) {
        continue;
      }
      if (edge_value_publication_may_read_register_index(
              context,
              producer->context,
              source_facts.source_value,
              producer->instruction_index + 1,
              move->destination_register->reg.index,
              source_facts.publication,
              0U)) {
        return false;
      }
    }
  }
  if (move == nullptr ||
      move->authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      move->source_parallel_copy_predecessor_label !=
          std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label} ||
      !move->source_parallel_copy_successor_label.has_value()) {
    return true;
  }
  if (context.function.prepared_lookups == nullptr) {
    return true;
  }
  const auto source_facts =
      prepare::prepare_edge_copy_source_facts(
          &context.function.prepared_lookups->edge_publications,
          *move->source_parallel_copy_predecessor_label,
          *move->source_parallel_copy_successor_label,
          move->move.to_value_id);
  if (source_facts.status != prepare::PreparedEdgeCopySourceFactsStatus::Available ||
      source_facts.publication == nullptr ||
      source_facts.publication->phase != prepare::PreparedMovePhase::BlockEntry) {
    return true;
  }
  if (prepare::prepared_edge_publication_redundant_block_entry_parallel_copy_move(
          *source_facts.publication, move->source_move)) {
    return false;
  }
  if (move->source_move != nullptr &&
      prepared_edge_copy_source_facts_have_materializable_producer(source_facts) &&
      source_facts.source_home != nullptr &&
      prepare::prepared_edge_publication_matches_parallel_copy_move_source(
          *source_facts.publication, *move->source_move, *source_facts.source_home)) {
    return false;
  }
  return true;
}
[[nodiscard]] std::vector<module::MachineInstruction>
lower_predecessor_select_parallel_copy_sources(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.bir_function == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::Branch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::Branch) {
    return lowered;
  }

  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      prepare::PreparedMovePhase::BlockEntry,
      context.block_index,
      0);
  if (bundle == nullptr ||
      bundle->authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      bundle->source_parallel_copy_predecessor_label !=
          std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label} ||
      !bundle->source_parallel_copy_successor_label.has_value()) {
    return lowered;
  }

  const auto successor_label = prepare::prepared_block_label(
      context.function.prepared->names,
      *bundle->source_parallel_copy_successor_label);
  if (successor_label.empty() ||
      successor_label != context.bir_block->terminator.target_label) {
    return lowered;
  }
  for (const auto& move : bundle->moves) {
    if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.source_immediate_i32.has_value() ||
        move.from_value_id == move.to_value_id) {
      continue;
    }
    if (context.function.prepared_lookups == nullptr) {
      continue;
    }
    const auto source_facts =
        prepare::prepare_block_entry_parallel_copy_edge_source_facts(
            &context.function.prepared_lookups->edge_publications,
            context.control_flow_block->block_label,
            *bundle->source_parallel_copy_successor_label,
            move);
    if (!prepared_edge_copy_source_facts_have_materializable_producer(source_facts) ||
        source_facts.source_home == nullptr ||
        source_facts.destination_home == nullptr ||
        source_facts.source_home->value_name == c4c::kInvalidValueName ||
        source_facts.destination_home->kind != prepare::PreparedValueHomeKind::Register ||
        !prepare::prepared_edge_publication_matches_parallel_copy_move_source(
            *source_facts.publication, move, *source_facts.source_home)) {
      continue;
    }
    auto source_lowered = lower_predecessor_join_source_publication(
        context,
        *source_facts.publication,
        *source_facts.source_home,
        *source_facts.destination_home,
        scalar_state);
    if (!source_lowered.has_value()) {
      lowered.clear();
      return lowered;
    }
    lowered.push_back(std::move(*source_lowered));
    return lowered;
  }
  return lowered;
}
}  // namespace c4c::backend::aarch64::codegen
