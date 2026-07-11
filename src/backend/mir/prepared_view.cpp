#include "prepared_view.hpp"
#include "query.hpp"

#include "../prealloc/module.hpp"

#include <string>
#include <utility>

namespace c4c::backend::mir::prepared {

PreparedMirFunctionView::PreparedMirFunctionView(const PreparedMirCoreView* core,
                                                 const PreparedMirFunctionEntry* entry)
    : core_(core), entry_(entry) {}

PreparedMirFunctionView::operator bool() const {
  return core_ != nullptr && entry_ != nullptr;
}

FunctionNameId PreparedMirFunctionView::function_name() const {
  return entry_->function_name;
}

std::string_view PreparedMirFunctionView::function_name_text() const {
  return core_->function_name(entry_->function_name);
}

const bir::Function& PreparedMirFunctionView::bir_function() const {
  return *entry_->bir_function;
}

const prepare::PreparedControlFlowFunction& PreparedMirFunctionView::control_flow() const {
  return *entry_->control_flow;
}

const prepare::PreparedValueLocationFunction& PreparedMirFunctionView::value_locations() const {
  return *entry_->value_locations;
}

const prepare::PreparedStackLayout& PreparedMirFunctionView::stack_layout() const {
  return core_->module_->stack_layout;
}

const prepare::PreparedAddressingFunction& PreparedMirFunctionView::addressing() const {
  return *entry_->addressing;
}

const prepare::PreparedFunctionLookups& PreparedMirFunctionView::prepared_lookups() const {
  return entry_->prepared_lookups;
}

const std::vector<PreparedMirBlockView>& PreparedMirFunctionView::blocks() const {
  return entry_->blocks;
}

bool PreparedMirCoreComparisonReport::equal() const {
  return status == PreparedMirCoreComparisonStatus::Equal &&
         blocking_differences.empty() &&
         unsupported_reasons.empty();
}

namespace {

std::string bytes_hex(std::string_view bytes) {
  constexpr char kHexDigits[] = "0123456789abcdef";
  std::string out;
  out.reserve(bytes.size() * 2);
  for (const unsigned char byte : bytes) {
    out.push_back(kHexDigits[byte >> 4]);
    out.push_back(kHexDigits[byte & 0x0f]);
  }
  return out;
}

bool same_target_snapshot(const PreparedMirCoreTargetSnapshot& lhs,
                          const PreparedMirCoreTargetSnapshot& rhs) {
  return lhs.target_triple == rhs.target_triple &&
         lhs.profile_triple == rhs.profile_triple &&
         lhs.arch == rhs.arch &&
         lhs.os == rhs.os &&
         lhs.abi == rhs.abi &&
         lhs.relocation == rhs.relocation &&
         lhs.has_float_arg_registers == rhs.has_float_arg_registers &&
         lhs.has_float_return_registers == rhs.has_float_return_registers;
}

bool same_function_snapshot(const PreparedMirCoreFunctionSnapshot& lhs,
                            const PreparedMirCoreFunctionSnapshot& rhs) {
  return lhs.index == rhs.index &&
         lhs.name == rhs.name &&
         lhs.function_id == rhs.function_id &&
         lhs.has_function_id == rhs.has_function_id &&
         lhs.is_declaration == rhs.is_declaration &&
         lhs.block_count == rhs.block_count &&
         lhs.has_function_view == rhs.has_function_view;
}

bool same_global_snapshot(const PreparedMirCoreGlobalSnapshot& lhs,
                          const PreparedMirCoreGlobalSnapshot& rhs) {
  return lhs.index == rhs.index &&
         lhs.name == rhs.name &&
         lhs.link_name_id == rhs.link_name_id &&
         lhs.type == rhs.type &&
         lhs.is_extern == rhs.is_extern &&
         lhs.is_constant == rhs.is_constant &&
         lhs.size_bytes == rhs.size_bytes &&
         lhs.align_bytes == rhs.align_bytes;
}

bool same_string_snapshot(const PreparedMirCoreStringSnapshot& lhs,
                          const PreparedMirCoreStringSnapshot& rhs) {
  return lhs.index == rhs.index &&
         lhs.name == rhs.name &&
         lhs.byte_count == rhs.byte_count &&
         lhs.bytes_hex == rhs.bytes_hex &&
         lhs.align_bytes == rhs.align_bytes;
}

bool same_block_snapshot(const PreparedMirCoreBlockSnapshot& lhs,
                         const PreparedMirCoreBlockSnapshot& rhs) {
  return lhs.index == rhs.index &&
         lhs.label == rhs.label &&
         lhs.bir_present == rhs.bir_present &&
         lhs.prepared_present == rhs.prepared_present &&
         lhs.instruction_count == rhs.instruction_count &&
         lhs.cursor_count == rhs.cursor_count;
}

template <typename T, typename Equal>
bool same_snapshot_vector(const std::vector<T>& lhs,
                          const std::vector<T>& rhs,
                          Equal equal) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (!equal(lhs[index], rhs[index])) {
      return false;
    }
  }
  return true;
}

bool same_function_view_snapshot(const PreparedMirCoreFunctionViewSnapshot& lhs,
                                 const PreparedMirCoreFunctionViewSnapshot& rhs) {
  return lhs.name == rhs.name &&
         lhs.function_id == rhs.function_id &&
         lhs.bir_block_count == rhs.bir_block_count &&
         lhs.prepared_block_count == rhs.prepared_block_count &&
         lhs.branch_condition_count == rhs.branch_condition_count &&
         lhs.join_transfer_count == rhs.join_transfer_count &&
         lhs.parallel_copy_bundle_count == rhs.parallel_copy_bundle_count &&
         lhs.value_home_count == rhs.value_home_count &&
         lhs.move_bundle_count == rhs.move_bundle_count &&
         lhs.stack_object_count == rhs.stack_object_count &&
         lhs.stack_frame_slot_count == rhs.stack_frame_slot_count &&
         lhs.frame_size_bytes == rhs.frame_size_bytes &&
         lhs.frame_alignment_bytes == rhs.frame_alignment_bytes &&
         lhs.memory_access_count == rhs.memory_access_count &&
         lhs.address_materialization_count == rhs.address_materialization_count &&
         lhs.lookup_call_count == rhs.lookup_call_count &&
         lhs.lookup_address_block_count == rhs.lookup_address_block_count &&
         lhs.lookup_memory_position_count == rhs.lookup_memory_position_count &&
         lhs.lookup_value_home_count == rhs.lookup_value_home_count &&
         lhs.lookup_move_bundle_count == rhs.lookup_move_bundle_count &&
         lhs.lookup_edge_publication_count == rhs.lookup_edge_publication_count &&
         lhs.lookup_edge_source_producer_count == rhs.lookup_edge_source_producer_count &&
         lhs.lookup_branch_stack_load_count == rhs.lookup_branch_stack_load_count &&
         same_snapshot_vector(lhs.blocks, rhs.blocks, same_block_snapshot);
}

PreparedMirDirectEdgePublicationSourceStatus direct_edge_source_status_from_prealloc(
    prepare::PreparedEdgeCopySourceFactsStatus status) {
  switch (status) {
    case prepare::PreparedEdgeCopySourceFactsStatus::Available:
      return PreparedMirDirectEdgePublicationSourceStatus::Available;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingPublication:
    case prepare::PreparedEdgeCopySourceFactsStatus::AmbiguousPublication:
    case prepare::PreparedEdgeCopySourceFactsStatus::PublicationUnavailable:
    case prepare::PreparedEdgeCopySourceFactsStatus::EdgeMismatch:
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingPreparedLookups:
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingPredecessorLabel:
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSuccessorLabel:
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingDestinationValue:
      return PreparedMirDirectEdgePublicationSourceStatus::MissingPublication;
    case prepare::PreparedEdgeCopySourceFactsStatus::UnsupportedMove:
    case prepare::PreparedEdgeCopySourceFactsStatus::MoveEdgeMismatch:
    case prepare::PreparedEdgeCopySourceFactsStatus::PublicationMoveMismatch:
      return PreparedMirDirectEdgePublicationSourceStatus::UnsupportedMove;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceValue:
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceProducer:
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceMemoryAccess:
    case prepare::PreparedEdgeCopySourceFactsStatus::IncompleteSourceMemoryAccess:
      return PreparedMirDirectEdgePublicationSourceStatus::UnsupportedSource;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceHome:
      return PreparedMirDirectEdgePublicationSourceStatus::UnsupportedSourceHome;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceFreshnessAuthority:
      return PreparedMirDirectEdgePublicationSourceStatus::MissingSelectedFreshness;
    case prepare::PreparedEdgeCopySourceFactsStatus::InvalidSourceFreshnessAuthority:
      return PreparedMirDirectEdgePublicationSourceStatus::InvalidSourceFreshness;
    case prepare::PreparedEdgeCopySourceFactsStatus::AmbiguousSourceFreshnessAuthority:
      return PreparedMirDirectEdgePublicationSourceStatus::AmbiguousSourceFreshness;
  }
  return PreparedMirDirectEdgePublicationSourceStatus::UnsupportedSource;
}

PreparedMirDirectEdgePublicationSourceQueryStatus direct_edge_query_status_from_prealloc(
    prepare::PreparedCurrentBlockJoinParallelCopySourceStatus status) {
  switch (status) {
    case prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::Available:
      return PreparedMirDirectEdgePublicationSourceQueryStatus::Available;
    case prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::MissingNames:
      return PreparedMirDirectEdgePublicationSourceQueryStatus::MissingLookups;
    case prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::MissingValueLocations:
      return PreparedMirDirectEdgePublicationSourceQueryStatus::MissingValueLocations;
    case prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::
        MissingEdgePublicationLookups:
      return PreparedMirDirectEdgePublicationSourceQueryStatus::MissingEdgePublicationLookups;
    case prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::MissingBlock:
      return PreparedMirDirectEdgePublicationSourceQueryStatus::MissingBlock;
    case prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::MissingSuccessorLabel:
      return PreparedMirDirectEdgePublicationSourceQueryStatus::MissingSuccessorLabel;
  }
  return PreparedMirDirectEdgePublicationSourceQueryStatus::MissingLookups;
}

const prepare::PreparedRegallocFunction* find_regalloc_function(
    const prepare::PreparedRegalloc& regalloc,
    FunctionNameId function_name) {
  for (const auto& function : regalloc.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

}  // namespace

std::optional<PreparedMirInstructionCursor> PreparedMirFunctionView::instruction(
    std::size_t block_index,
    std::size_t instruction_index) const {
  if (!*this || block_index >= entry_->blocks.size()) {
    return std::nullopt;
  }
  const auto& block_view = entry_->blocks[block_index];
  if (block_view.block == nullptr ||
      instruction_index >= block_view.block->insts.size()) {
    return std::nullopt;
  }
  return PreparedMirInstructionCursor{
      .function_name = entry_->function_name,
      .block_label = block_view.block_label,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .function = entry_->bir_function,
      .block = block_view.block,
      .instruction = &block_view.block->insts[instruction_index],
      .prepared_block = block_view.control_flow,
  };
}

PreparedMirDirectEdgePublicationSourceQuery
PreparedMirFunctionView::current_block_direct_edge_publication_sources(
    std::size_t block_index) const {
  PreparedMirDirectEdgePublicationSourceQuery view_query;
  if (!*this) {
    view_query.status =
        PreparedMirDirectEdgePublicationSourceQueryStatus::MissingFunctionView;
    return view_query;
  }
  if (block_index >= entry_->blocks.size() ||
      entry_->blocks[block_index].block == nullptr) {
    view_query.status = PreparedMirDirectEdgePublicationSourceQueryStatus::MissingBlock;
    return view_query;
  }

  const auto& block = entry_->blocks[block_index];
  const auto* regalloc = find_regalloc_function(core_->module_->regalloc,
                                                entry_->function_name);
  const auto bir_identity = find_bir_current_block_join_source_identity(
      BirCurrentBlockJoinSourceRequest{
          .successor_block = block.block,
          .successor_label = block.block->label,
          .successor_label_id = block.block_label,
      });
  std::vector<prepare::PreparedFactBoundaryEvidence> join_source_evidence;
  if (bir_identity.status == BirCurrentBlockJoinSourceStatus::Available) {
    for (const auto& fact : bir_identity.facts) {
      if (fact.status != BirCurrentBlockJoinSourceStatus::Available ||
          fact.source_value_kind != bir::Value::Kind::Named ||
          !fact.source_producer_instruction_index.has_value()) {
        continue;
      }
      join_source_evidence.push_back(prepare::PreparedFactBoundaryEvidence{
          .status = prepare::PreparedFactBoundaryStatus::Available,
          .function_name = entry_->function_name,
          .block_label = block.block_label,
          .value_name = core_->prepared_names().value_names.find(
              fact.source_value_name),
          .instruction_index = *fact.source_producer_instruction_index,
      });
    }
  } else {
    join_source_evidence.push_back(prepare::PreparedFactBoundaryEvidence{
        .status = bir_identity.status ==
                          BirCurrentBlockJoinSourceStatus::MissingSourceProducer
                      ? prepare::PreparedFactBoundaryStatus::Incomplete
                      : prepare::PreparedFactBoundaryStatus::Missing,
        .function_name = entry_->function_name,
        .block_label = block.block_label,
    });
  }
  const auto source_facts =
      prepare::prepare_current_block_join_parallel_copy_source_facts(
          prepare::PreparedCurrentBlockJoinParallelCopySourceQueryInputs{
              .names = &core_->prepared_names(),
              .regalloc = regalloc,
              .value_locations = entry_->value_locations,
              .value_home_lookups = &entry_->prepared_lookups.value_homes,
              .edge_publications = &entry_->prepared_lookups.edge_publications,
              .join_source_evidence = std::move(join_source_evidence),
              .block = block.block,
              .successor_label = block.block_label,
          });
  view_query.status = direct_edge_query_status_from_prealloc(source_facts.status);
  if (source_facts.status !=
      prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::Available) {
    return view_query;
  }

  for (const auto& fact : source_facts.facts) {
    PreparedMirDirectEdgePublicationSourceView source_view{
        .status = direct_edge_source_status_from_prealloc(fact.status),
    };
    if (fact.destination_home == nullptr ||
        fact.destination_home->kind != prepare::PreparedValueHomeKind::Register ||
        !fact.destination_home->register_name.has_value()) {
      source_view.status =
          PreparedMirDirectEdgePublicationSourceStatus::UnsupportedDestinationHome;
    }
    if (source_view.status == PreparedMirDirectEdgePublicationSourceStatus::Available &&
        (!fact.source_freshness_authority.has_value() ||
         fact.source_freshness_authority->use_kind !=
             prepare::PreparedValueFreshnessUseKind::DirectEdgePublicationSource ||
         fact.source_freshness_authority->source_kind !=
             prepare::PreparedValueFreshnessSourceKind::DirectEdgePublication ||
         fact.source_freshness_authority->proof_kind !=
             prepare::PreparedValueFreshnessProofKind::DirectEdgePublicationMove ||
         fact.source_freshness_authority->rank !=
             prepare::PreparedValueFreshnessSourceRank::DirectEdgePublication)) {
      source_view.status =
          PreparedMirDirectEdgePublicationSourceStatus::InvalidSourceFreshness;
    }

    if (source_view.status != PreparedMirDirectEdgePublicationSourceStatus::Available) {
      view_query.sources.push_back(std::move(source_view));
      continue;
    }

    source_view.predecessor_label = fact.predecessor_label;
    source_view.successor_label = fact.successor_label;
    source_view.destination_value_id = fact.destination_value_id;
    source_view.destination_value_name = fact.destination_value_name;
    source_view.source_value_id = fact.source_value_id;
    source_view.source_value_name = fact.source_value_name;
    source_view.source_home_kind = fact.source_home_kind;
    source_view.destination_home_kind = fact.destination_home_kind;
    source_view.destination_storage_kind = fact.destination_storage_kind;
    source_view.destination_register_name = fact.destination_register_name;
    source_view.immediate_source = fact.immediate_source;
    source_view.source_freshness_status = fact.source_freshness_status;
    source_view.source_freshness_candidate_count =
        fact.source_freshness_authorities.size();
    source_view.freshness_use_kind = fact.source_freshness_authority->use_kind;
    source_view.freshness_source_kind = fact.source_freshness_authority->source_kind;
    source_view.freshness_proof_kind = fact.source_freshness_authority->proof_kind;
    source_view.freshness_rank = fact.source_freshness_authority->rank;
    if (fact.source_home != nullptr) {
      source_view.source_register_name = fact.source_home->register_name;
      source_view.source_stack_offset_bytes = fact.source_home->offset_bytes;
      source_view.source_immediate_i32 = fact.source_home->immediate_i32;
    }
    view_query.sources.push_back(std::move(source_view));
  }
  return view_query;
}

PreparedMirBranchStackLoadAuthorityView
query_prepared_mir_branch_stack_load_authority(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups& lookups,
    FunctionNameId function_name,
    const bir::Value* value,
    const prepare::PreparedValueHome* home,
    prepare::PreparedBranchStackLoadRole role,
    BlockLabelId block_label,
    std::size_t block_index,
    std::size_t terminator_instruction_index) {
  PreparedMirBranchStackLoadAuthorityView view{
      .role = role,
      .block_label = block_label,
      .block_index = block_index,
      .terminator_instruction_index = terminator_instruction_index,
  };
  if (home == nullptr || home->kind != prepare::PreparedValueHomeKind::StackSlot) {
    return view;
  }

  view.freshness_required = true;
  view.available = false;
  view.authority_status =
      prepare::PreparedBranchStackLoadAuthorityStatus::MissingSourceFreshnessAuthority;
  view.source_freshness_status =
      prepare::PreparedValueFreshnessQueryStatus::NoCandidate;

  if (value == nullptr || value->kind != bir::Value::Kind::Named ||
      value->name.empty()) {
    view.authority_status =
        prepare::PreparedBranchStackLoadAuthorityStatus::UnsupportedBranchValue;
    view.source_freshness_status =
        prepare::PreparedValueFreshnessQueryStatus::MissingValue;
    return view;
  }

  const auto value_name = names.value_names.find(value->name);
  if (value_name == kInvalidValueName || value_name != home->value_name ||
      home->value_id == prepare::PreparedValueId{0}) {
    view.authority_status =
        prepare::PreparedBranchStackLoadAuthorityStatus::HomeValueMismatch;
    view.source_freshness_status =
        prepare::PreparedValueFreshnessQueryStatus::MissingValue;
    return view;
  }

  for (const auto& record : lookups.branch_stack_load_authorities.records) {
    const auto& authority = record.authority;
    if (record.function_name != function_name ||
        record.role != role ||
        record.block_label != block_label ||
        authority.value_id != home->value_id ||
        authority.value_name != home->value_name ||
        authority.branch_block_index != block_index ||
        authority.branch_terminator_instruction_index !=
            terminator_instruction_index) {
      continue;
    }

    view.authority_status = authority.status;
    view.source_freshness_status = authority.source_freshness_status;
    view.source_freshness_candidate_count =
        authority.source_freshness_authorities.size();
    view.value_id = authority.value_id;
    view.value_name = authority.value_name;
    if (!prepare::prepared_branch_stack_load_authority_available(authority) ||
        authority.source_freshness_status !=
            prepare::PreparedValueFreshnessQueryStatus::Selected ||
        !authority.source_freshness_authority.has_value()) {
      return view;
    }

    const auto& freshness = *authority.source_freshness_authority;
    view.freshness_use_kind = freshness.use_kind;
    view.freshness_source_kind = freshness.source_kind;
    view.freshness_proof_kind = freshness.proof_kind;
    view.freshness_rank = freshness.rank;
    if (freshness.value_id == home->value_id &&
        freshness.value_name == home->value_name &&
        freshness.use_kind ==
            prepare::PreparedValueFreshnessUseKind::BranchStackLoadSource &&
        freshness.source_kind ==
            prepare::PreparedValueFreshnessSourceKind::BranchStackSlot &&
        freshness.proof_kind ==
            prepare::PreparedValueFreshnessProofKind::BranchTerminatorOrdering &&
        freshness.rank ==
            prepare::PreparedValueFreshnessSourceRank::BranchStackSlot &&
        freshness.reference.home == home &&
        freshness.reference.block_index == block_index &&
        freshness.reference.instruction_index == terminator_instruction_index) {
      view.available = true;
      return view;
    }

    view.authority_status =
        prepare::PreparedBranchStackLoadAuthorityStatus::
            UnsupportedSourceFreshnessAuthority;
    return view;
  }

  return view;
}

PreparedMirBranchStackLoadAuthorityView
PreparedMirFunctionView::branch_stack_load_authority(
    const bir::Value* value,
    const prepare::PreparedValueHome* home,
    prepare::PreparedBranchStackLoadRole role,
    BlockLabelId block_label,
    std::size_t block_index,
    std::size_t terminator_instruction_index) const {
  if (!*this) {
    return PreparedMirBranchStackLoadAuthorityView{
        .role = role,
        .block_label = block_label,
        .block_index = block_index,
        .terminator_instruction_index = terminator_instruction_index,
    };
  }
  return query_prepared_mir_branch_stack_load_authority(
      core_->prepared_names(),
      entry_->prepared_lookups,
      entry_->function_name,
      value,
      home,
      role,
      block_label,
      block_index,
      terminator_instruction_index);
}

PreparedMirCoreView::PreparedMirCoreView(const prepare::PreparedBirModule& module)
    : module_(&module) {
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    defined_functions_.push_back(&function);

    const auto function_name_id = module.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    auto* existing = const_cast<BirFunctionBinding*>(bir_binding(function_name_id));
    if (existing != nullptr) {
      existing->function = nullptr;
      existing->duplicate = true;
      continue;
    }
    bir_bindings_.push_back(BirFunctionBinding{
        .function_name = function_name_id,
        .function = &function,
    });
  }

  for (const auto& binding : bir_bindings_) {
    if (binding.duplicate || binding.function == nullptr) {
      continue;
    }

    const auto* control_flow =
        prepare::find_prepared_control_flow_function(module.control_flow,
                                                     binding.function_name);
    const auto* value_locations =
        prepare::find_prepared_value_location_function(module.value_locations,
                                                       binding.function_name);
    const auto* addressing =
        prepare::find_prepared_addressing_function(module.addressing,
                                                   binding.function_name);
    if (control_flow == nullptr || value_locations == nullptr || addressing == nullptr) {
      continue;
    }

    PreparedMirFunctionEntry entry{
        .function_name = binding.function_name,
        .bir_function = binding.function,
        .control_flow = control_flow,
        .value_locations = value_locations,
        .addressing = addressing,
        .prepared_lookups = prepare::make_prepared_function_lookups(module,
                                                                    *control_flow),
    };

    for (std::size_t block_index = 0; block_index < control_flow->blocks.size();
         ++block_index) {
      const bir::Block* bir_block = nullptr;
      if (block_index < binding.function->blocks.size()) {
        bir_block = &binding.function->blocks[block_index];
      }
      const auto& prepared_block = control_flow->blocks[block_index];
      entry.blocks.push_back(PreparedMirBlockView{
          .function_name = binding.function_name,
          .block_label = prepared_block.block_label,
          .block_index = block_index,
          .block = bir_block,
          .control_flow = &prepared_block,
      });
    }

    function_entries_.push_back(std::move(entry));
  }
}

const TargetProfile& PreparedMirCoreView::target_profile() const {
  return module_->target_profile;
}

std::string_view PreparedMirCoreView::target_triple() const {
  return module_->module.target_triple;
}

const bir::NameTables& PreparedMirCoreView::bir_names() const {
  return module_->module.names;
}

const prepare::PreparedNameTables& PreparedMirCoreView::prepared_names() const {
  return module_->names;
}

const std::vector<bir::Function>& PreparedMirCoreView::functions() const {
  return module_->module.functions;
}

const std::vector<const bir::Function*>& PreparedMirCoreView::defined_functions() const {
  return defined_functions_;
}

const std::vector<bir::Global>& PreparedMirCoreView::globals() const {
  return module_->module.globals;
}

const std::vector<bir::StringConstant>& PreparedMirCoreView::string_constants() const {
  return module_->module.string_constants;
}

std::optional<FunctionNameId> PreparedMirCoreView::resolve_function_name(
    std::string_view name) const {
  const FunctionNameId id = module_->names.function_names.find(name);
  if (id == kInvalidFunctionName || prepare::prepared_function_name(module_->names, id) != name) {
    return std::nullopt;
  }
  return id;
}

std::string_view PreparedMirCoreView::function_name(FunctionNameId id) const {
  if (id == kInvalidFunctionName) {
    return {};
  }
  return prepare::prepared_function_name(module_->names, id);
}

const bir::Function* PreparedMirCoreView::bir_function(FunctionNameId id) const {
  const auto* binding = bir_binding(id);
  if (binding == nullptr || binding->duplicate) {
    return nullptr;
  }
  return binding->function;
}

const prepare::PreparedControlFlowFunction* PreparedMirCoreView::control_flow(
    FunctionNameId id) const {
  return prepare::find_prepared_control_flow_function(module_->control_flow, id);
}

std::optional<PreparedMirFunctionView> PreparedMirCoreView::function_view(
    FunctionNameId id) const {
  const auto* function_entry = entry(id);
  if (function_entry == nullptr) {
    return std::nullopt;
  }
  return PreparedMirFunctionView{this, function_entry};
}

std::optional<PreparedMirFunctionView> PreparedMirCoreView::function_view(
    std::string_view name) const {
  const auto id = resolve_function_name(name);
  if (!id.has_value()) {
    return std::nullopt;
  }
  return function_view(*id);
}

PreparedMirCoreSnapshot PreparedMirCoreView::structural_snapshot() const {
  const auto id_value = [](auto id) {
    return static_cast<std::size_t>(id);
  };
  const auto block_label_text = [&](BlockLabelId id) {
    const std::string_view label = prepare::prepared_block_label(prepared_names(), id);
    return label.empty() ? std::string{"<invalid>"} : std::string{label};
  };

  PreparedMirCoreSnapshot snapshot;
  snapshot.target = PreparedMirCoreTargetSnapshot{
      .target_triple = std::string(target_triple()),
      .profile_triple = target_profile().triple,
      .arch = std::string(c4c::target_arch_name(target_profile().arch)),
      .os = std::string(c4c::target_os_name(target_profile().os)),
      .abi = std::string(c4c::backend_abi_name(target_profile().backend_abi)),
      .relocation =
          std::string(c4c::target_relocation_model_name(target_profile().relocation_model)),
      .has_float_arg_registers = target_profile().has_float_arg_registers,
      .has_float_return_registers = target_profile().has_float_return_registers,
  };

  for (std::size_t index = 0; index < functions().size(); ++index) {
    const auto& function = functions()[index];
    const auto function_id = resolve_function_name(function.name);
    snapshot.functions.push_back(PreparedMirCoreFunctionSnapshot{
        .index = index,
        .name = function.name,
        .function_id = function_id.has_value() ? id_value(*function_id) : 0,
        .has_function_id = function_id.has_value(),
        .is_declaration = function.is_declaration,
        .block_count = function.blocks.size(),
        .has_function_view = function_id.has_value() && function_view(*function_id).has_value(),
    });
  }

  for (const auto* function : defined_functions()) {
    snapshot.defined_functions.push_back(function != nullptr ? function->name : "<null>");
  }

  for (std::size_t index = 0; index < globals().size(); ++index) {
    const auto& global = globals()[index];
    snapshot.globals.push_back(PreparedMirCoreGlobalSnapshot{
        .index = index,
        .name = global.name,
        .link_name_id = id_value(global.link_name_id),
        .type = static_cast<std::size_t>(global.type),
        .is_extern = global.is_extern,
        .is_constant = global.is_constant,
        .size_bytes = global.size_bytes,
        .align_bytes = global.align_bytes,
    });
  }

  for (std::size_t index = 0; index < string_constants().size(); ++index) {
    const auto& constant = string_constants()[index];
    snapshot.string_constants.push_back(PreparedMirCoreStringSnapshot{
        .index = index,
        .name = constant.name,
        .byte_count = constant.bytes.size(),
        .bytes_hex = bytes_hex(constant.bytes),
        .align_bytes = constant.align_bytes,
    });
  }

  for (const auto& entry : function_entries_) {
    const PreparedMirFunctionView view{this, &entry};
    const auto& control_flow = view.control_flow();
    const auto& value_locations = view.value_locations();
    const auto& addressing = view.addressing();
    const auto& lookups = view.prepared_lookups();

    PreparedMirCoreFunctionViewSnapshot function_snapshot{
        .name = std::string(view.function_name_text()),
        .function_id = id_value(view.function_name()),
        .bir_block_count = view.bir_function().blocks.size(),
        .prepared_block_count = control_flow.blocks.size(),
        .branch_condition_count = control_flow.branch_conditions.size(),
        .join_transfer_count = control_flow.join_transfers.size(),
        .parallel_copy_bundle_count = control_flow.parallel_copy_bundles.size(),
        .value_home_count = value_locations.value_homes.size(),
        .move_bundle_count = value_locations.move_bundles.size(),
        .stack_object_count = view.stack_layout().objects.size(),
        .stack_frame_slot_count = view.stack_layout().frame_slots.size(),
        .frame_size_bytes = view.stack_layout().frame_size_bytes,
        .frame_alignment_bytes = view.stack_layout().frame_alignment_bytes,
        .memory_access_count = addressing.accesses.size(),
        .address_materialization_count = addressing.address_materializations.size(),
        .lookup_call_count = lookups.call_plans.calls_by_position.size(),
        .lookup_address_block_count =
            lookups.address_materializations.materializations_by_block.size(),
        .lookup_memory_position_count = lookups.memory_accesses.accesses_by_position.size(),
        .lookup_value_home_count = lookups.value_homes.homes_by_id.size(),
        .lookup_move_bundle_count = lookups.move_bundles.bundles_by_position.size(),
        .lookup_edge_publication_count = lookups.edge_publications.publications.size(),
        .lookup_edge_source_producer_count =
            lookups.edge_publication_source_producers.producers_by_value_name.size(),
        .lookup_branch_stack_load_count =
            lookups.branch_stack_load_authorities.records.size(),
    };

    for (const auto& block : view.blocks()) {
      const std::size_t instruction_count =
          block.block != nullptr ? block.block->insts.size() : 0;
      std::size_t cursor_count = 0;
      for (std::size_t instruction_index = 0; instruction_index < instruction_count;
           ++instruction_index) {
        if (view.instruction(block.block_index, instruction_index).has_value()) {
          ++cursor_count;
        }
      }
      function_snapshot.blocks.push_back(PreparedMirCoreBlockSnapshot{
          .index = block.block_index,
          .label = block_label_text(block.block_label),
          .bir_present = block.block != nullptr,
          .prepared_present = block.control_flow != nullptr,
          .instruction_count = instruction_count,
          .cursor_count = cursor_count,
      });
    }

    snapshot.function_views.push_back(std::move(function_snapshot));
  }

  return snapshot;
}

std::string PreparedMirCoreView::canonical_dump() const {
  std::string out;
  const auto append_line = [&](std::string line) {
    out += std::move(line);
    out += '\n';
  };
  const auto bool_text = [](bool value) -> const char* {
    return value ? "1" : "0";
  };
  const auto id_text = [](auto id) {
    return std::to_string(static_cast<std::size_t>(id));
  };
  const auto block_label_text = [&](BlockLabelId id) {
    const std::string_view label = prepare::prepared_block_label(prepared_names(), id);
    return label.empty() ? std::string{"<invalid>"} : std::string{label};
  };
  const auto value_name_text = [&](ValueNameId id) {
    const std::string_view name = prepare::prepared_value_name(prepared_names(), id);
    return name.empty() ? std::string{"<invalid>"} : std::string{name};
  };

  append_line("prepared_mir_core_view schema=1");
  append_line("target triple=" + std::string(target_triple()) +
              " profile_triple=" + target_profile().triple +
              " arch=" + c4c::target_arch_name(target_profile().arch) +
              " os=" + c4c::target_os_name(target_profile().os) +
              " abi=" + c4c::backend_abi_name(target_profile().backend_abi) +
              " relocation=" +
              c4c::target_relocation_model_name(target_profile().relocation_model) +
              " float_args=" + bool_text(target_profile().has_float_arg_registers) +
              " float_returns=" + bool_text(target_profile().has_float_return_registers));
  append_line("module functions=" + std::to_string(functions().size()) +
              " defined_functions=" + std::to_string(defined_functions().size()) +
              " globals=" + std::to_string(globals().size()) +
              " strings=" + std::to_string(string_constants().size()));

  for (std::size_t index = 0; index < functions().size(); ++index) {
    const auto& function = functions()[index];
    const auto function_id = resolve_function_name(function.name);
    const bool has_view = function_id.has_value() && function_view(*function_id).has_value();
    append_line("function index=" + std::to_string(index) +
                " name=" + function.name +
                " id=" + (function_id.has_value() ? id_text(*function_id) : std::string{"invalid"}) +
                " declaration=" + bool_text(function.is_declaration) +
                " blocks=" + std::to_string(function.blocks.size()) +
                " view=" + bool_text(has_view));
  }

  for (std::size_t index = 0; index < defined_functions().size(); ++index) {
    const auto* function = defined_functions()[index];
    append_line("defined_function index=" + std::to_string(index) +
                " name=" + (function != nullptr ? function->name : std::string{"<null>"}));
  }

  for (std::size_t index = 0; index < globals().size(); ++index) {
    const auto& global = globals()[index];
    append_line("global index=" + std::to_string(index) +
                " name=" + global.name +
                " link_id=" + id_text(global.link_name_id) +
                " type=" + std::to_string(static_cast<unsigned>(global.type)) +
                " extern=" + bool_text(global.is_extern) +
                " constant=" + bool_text(global.is_constant) +
                " size=" + std::to_string(global.size_bytes) +
                " align=" + std::to_string(global.align_bytes));
  }

  for (std::size_t index = 0; index < string_constants().size(); ++index) {
    const auto& constant = string_constants()[index];
    append_line("string index=" + std::to_string(index) +
                " name=" + constant.name +
                " bytes=" + std::to_string(constant.bytes.size()) +
                " bytes_hex=" + bytes_hex(constant.bytes) +
                " align=" + std::to_string(constant.align_bytes));
  }

  for (const auto& entry : function_entries_) {
    const PreparedMirFunctionView view{this, &entry};
    const auto& control_flow = view.control_flow();
    const auto& value_locations = view.value_locations();
    const auto& addressing = view.addressing();
    const auto& lookups = view.prepared_lookups();

    append_line("function_view name=" + std::string(view.function_name_text()) +
                " id=" + id_text(view.function_name()) +
                " bir_blocks=" + std::to_string(view.bir_function().blocks.size()) +
                " prepared_blocks=" + std::to_string(control_flow.blocks.size()));
    append_line("control_flow function=" + std::string(view.function_name_text()) +
                " blocks=" + std::to_string(control_flow.blocks.size()) +
                " branch_conditions=" + std::to_string(control_flow.branch_conditions.size()) +
                " join_transfers=" + std::to_string(control_flow.join_transfers.size()) +
                " parallel_copy_bundles=" +
                std::to_string(control_flow.parallel_copy_bundles.size()));
    append_line("value_locations function=" + std::string(view.function_name_text()) +
                " homes=" + std::to_string(value_locations.value_homes.size()) +
                " move_bundles=" + std::to_string(value_locations.move_bundles.size()));
    for (const auto& home : value_locations.value_homes) {
      append_line("value_home function=" + std::string(view.function_name_text()) +
                  " value_id=" + std::to_string(home.value_id) +
                  " value_name=" + value_name_text(home.value_name) +
                  " kind=" + std::string(prepare::prepared_value_home_kind_name(home.kind)));
    }
    append_line("stack_layout function=" + std::string(view.function_name_text()) +
                " objects=" + std::to_string(view.stack_layout().objects.size()) +
                " frame_slots=" + std::to_string(view.stack_layout().frame_slots.size()) +
                " frame_size=" + std::to_string(view.stack_layout().frame_size_bytes) +
                " frame_align=" + std::to_string(view.stack_layout().frame_alignment_bytes));
    append_line("addressing function=" + std::string(view.function_name_text()) +
                " accesses=" + std::to_string(addressing.accesses.size()) +
                " materializations=" +
                std::to_string(addressing.address_materializations.size()) +
                " frame_size=" + std::to_string(addressing.frame_size_bytes) +
                " frame_align=" + std::to_string(addressing.frame_alignment_bytes));
    append_line("lookups function=" + std::string(view.function_name_text()) +
                " calls=" + std::to_string(lookups.call_plans.calls_by_position.size()) +
                " address_blocks=" +
                std::to_string(lookups.address_materializations.materializations_by_block.size()) +
                " memory_positions=" +
                std::to_string(lookups.memory_accesses.accesses_by_position.size()) +
                " value_homes=" + std::to_string(lookups.value_homes.homes_by_id.size()) +
                " move_bundles=" +
                std::to_string(lookups.move_bundles.bundles_by_position.size()) +
                " edge_publications=" +
                std::to_string(lookups.edge_publications.publications.size()) +
                " edge_source_producers=" +
                std::to_string(lookups.edge_publication_source_producers.producers_by_value_name.size()) +
                " branch_stack_loads=" +
                std::to_string(lookups.branch_stack_load_authorities.records.size()));

    for (const auto& block : view.blocks()) {
      const std::size_t instruction_count =
          block.block != nullptr ? block.block->insts.size() : 0;
      append_line("block function=" + std::string(view.function_name_text()) +
                  " index=" + std::to_string(block.block_index) +
                  " label=" + block_label_text(block.block_label) +
                  " bir_present=" + bool_text(block.block != nullptr) +
                  " prepared_present=" + bool_text(block.control_flow != nullptr) +
                  " instructions=" + std::to_string(instruction_count));
      for (std::size_t instruction_index = 0; instruction_index < instruction_count;
           ++instruction_index) {
        const auto cursor = view.instruction(block.block_index, instruction_index);
        append_line("cursor function=" + std::string(view.function_name_text()) +
                    " block_index=" + std::to_string(block.block_index) +
                    " instruction_index=" + std::to_string(instruction_index) +
                    " label=" + block_label_text(block.block_label) +
                    " bound=" + bool_text(cursor.has_value()));
      }
    }
  }

  return out;
}

const PreparedMirFunctionEntry* PreparedMirCoreView::entry(
    FunctionNameId id) const {
  for (const auto& function_entry : function_entries_) {
    if (function_entry.function_name == id) {
      return &function_entry;
    }
  }
  return nullptr;
}

const PreparedMirCoreView::BirFunctionBinding* PreparedMirCoreView::bir_binding(
    FunctionNameId id) const {
  for (const auto& binding : bir_bindings_) {
    if (binding.function_name == id) {
      return &binding;
    }
  }
  return nullptr;
}

PreparedMirCoreComparisonReport compare_prepared_mir_core_views(
    const PreparedMirCoreView& lhs,
    const PreparedMirCoreView& rhs,
    const PreparedMirCoreComparisonOptions& options) {
  const PreparedMirCoreSnapshot lhs_snapshot = lhs.structural_snapshot();
  const PreparedMirCoreSnapshot rhs_snapshot = rhs.structural_snapshot();

  PreparedMirCoreComparisonReport report;
  const auto record_difference = [&](std::string difference) {
    report.blocking_differences.push_back(std::move(difference));
  };

  if (lhs_snapshot.schema_version != rhs_snapshot.schema_version) {
    record_difference("schema_version");
  }
  if (options.compare_target_identity &&
      !same_target_snapshot(lhs_snapshot.target, rhs_snapshot.target)) {
    record_difference("target_identity");
  }
  if (!same_snapshot_vector(lhs_snapshot.functions,
                            rhs_snapshot.functions,
                            same_function_snapshot)) {
    record_difference("functions");
  }
  if (lhs_snapshot.defined_functions != rhs_snapshot.defined_functions) {
    record_difference("defined_functions");
  }
  if (!same_snapshot_vector(lhs_snapshot.globals,
                            rhs_snapshot.globals,
                            same_global_snapshot)) {
    record_difference("globals");
  }
  if (!same_snapshot_vector(lhs_snapshot.string_constants,
                            rhs_snapshot.string_constants,
                            same_string_snapshot)) {
    record_difference("string_constants");
  }
  if (!same_snapshot_vector(lhs_snapshot.function_views,
                            rhs_snapshot.function_views,
                            same_function_view_snapshot)) {
    record_difference("function_views");
  }

  if (!report.unsupported_reasons.empty()) {
    report.status = PreparedMirCoreComparisonStatus::Unsupported;
  } else if (!report.blocking_differences.empty()) {
    report.status = PreparedMirCoreComparisonStatus::Different;
  }
  return report;
}

}  // namespace c4c::backend::mir::prepared
