#pragma once

#include "../bir/bir.hpp"
#include "../prealloc/addressing.hpp"
#include "../prealloc/control_flow.hpp"
#include "../prealloc/names.hpp"
#include "../prealloc/prepared_lookups.hpp"
#include "../prealloc/publication_plans.hpp"
#include "../prealloc/stack_layout/stack_layout.hpp"
#include "../prealloc/value_locations.hpp"
#include "../../target_profile.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {
struct PreparedBirModule;
}  // namespace c4c::backend::prepare

namespace c4c::backend::mir::prepared {

class PreparedMirCoreView;

struct PreparedMirBlockView {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t block_index = 0;
  const bir::Block* block = nullptr;
  const prepare::PreparedControlFlowBlock* control_flow = nullptr;
};

struct PreparedMirFunctionEntry {
  FunctionNameId function_name = kInvalidFunctionName;
  const bir::Function* bir_function = nullptr;
  const prepare::PreparedControlFlowFunction* control_flow = nullptr;
  const prepare::PreparedValueLocationFunction* value_locations = nullptr;
  const prepare::PreparedAddressingFunction* addressing = nullptr;
  prepare::PreparedFunctionLookups prepared_lookups;
  std::vector<PreparedMirBlockView> blocks;
};

struct PreparedMirInstructionCursor {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  const bir::Function* function = nullptr;
  const bir::Block* block = nullptr;
  const bir::Inst* instruction = nullptr;
  const prepare::PreparedControlFlowBlock* prepared_block = nullptr;
};

struct PreparedMirCoreTargetSnapshot {
  std::string target_triple;
  std::string profile_triple;
  std::string arch;
  std::string os;
  std::string abi;
  std::string relocation;
  bool has_float_arg_registers = false;
  bool has_float_return_registers = false;
};

struct PreparedMirCoreFunctionSnapshot {
  std::size_t index = 0;
  std::string name;
  std::size_t function_id = 0;
  bool has_function_id = false;
  bool is_declaration = false;
  std::size_t block_count = 0;
  bool has_function_view = false;
};

struct PreparedMirCoreGlobalSnapshot {
  std::size_t index = 0;
  std::string name;
  std::size_t link_name_id = 0;
  std::size_t type = 0;
  bool is_extern = false;
  bool is_constant = false;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
};

struct PreparedMirCoreStringSnapshot {
  std::size_t index = 0;
  std::string name;
  std::size_t byte_count = 0;
  std::string bytes_hex;
  std::size_t align_bytes = 0;
};

struct PreparedMirCoreBlockSnapshot {
  std::size_t index = 0;
  std::string label;
  bool bir_present = false;
  bool prepared_present = false;
  std::size_t instruction_count = 0;
  std::size_t cursor_count = 0;
};

struct PreparedMirCoreFunctionViewSnapshot {
  std::string name;
  std::size_t function_id = 0;
  std::size_t bir_block_count = 0;
  std::size_t prepared_block_count = 0;
  std::size_t branch_condition_count = 0;
  std::size_t join_transfer_count = 0;
  std::size_t parallel_copy_bundle_count = 0;
  std::size_t value_home_count = 0;
  std::size_t move_bundle_count = 0;
  std::size_t stack_object_count = 0;
  std::size_t stack_frame_slot_count = 0;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  std::size_t memory_access_count = 0;
  std::size_t address_materialization_count = 0;
  std::size_t lookup_call_count = 0;
  std::size_t lookup_address_block_count = 0;
  std::size_t lookup_memory_position_count = 0;
  std::size_t lookup_value_home_count = 0;
  std::size_t lookup_move_bundle_count = 0;
  std::size_t lookup_edge_publication_count = 0;
  std::size_t lookup_edge_source_producer_count = 0;
  std::size_t lookup_branch_stack_load_count = 0;
  std::vector<PreparedMirCoreBlockSnapshot> blocks;
};

struct PreparedMirCoreSnapshot {
  std::size_t schema_version = 1;
  PreparedMirCoreTargetSnapshot target;
  std::vector<PreparedMirCoreFunctionSnapshot> functions;
  std::vector<std::string> defined_functions;
  std::vector<PreparedMirCoreGlobalSnapshot> globals;
  std::vector<PreparedMirCoreStringSnapshot> string_constants;
  std::vector<PreparedMirCoreFunctionViewSnapshot> function_views;
};

struct PreparedMirCoreComparisonOptions {
  bool compare_target_identity = true;
};

enum class PreparedMirCoreComparisonStatus {
  Equal,
  Different,
  Unsupported,
};

struct PreparedMirCoreComparisonReport {
  PreparedMirCoreComparisonStatus status = PreparedMirCoreComparisonStatus::Equal;
  std::vector<std::string> blocking_differences;
  std::vector<std::string> unsupported_reasons;

  [[nodiscard]] bool equal() const;
};

enum class PreparedMirDirectEdgePublicationSourceQueryStatus {
  Available,
  MissingFunctionView,
  MissingBlock,
  MissingLookups,
  MissingValueLocations,
  MissingEdgePublicationLookups,
  MissingSuccessorLabel,
};

enum class PreparedMirDirectEdgePublicationSourceStatus {
  Available,
  MissingPublication,
  MissingSelectedFreshness,
  AmbiguousSourceFreshness,
  InvalidSourceFreshness,
  UnsupportedSourceHome,
  UnsupportedDestinationHome,
  UnsupportedMove,
  UnsupportedSource,
};

struct PreparedMirDirectEdgePublicationSourceView {
  PreparedMirDirectEdgePublicationSourceStatus status =
      PreparedMirDirectEdgePublicationSourceStatus::MissingPublication;
  const prepare::PreparedMoveBundle* bundle = nullptr;
  const prepare::PreparedMoveResolution* move = nullptr;
  const prepare::PreparedEdgePublication* publication = nullptr;
  bir::Value destination_value;
  bir::Value source_value;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  prepare::PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  std::optional<prepare::PreparedValueId> source_value_id;
  ValueNameId source_value_name = kInvalidValueName;
  prepare::PreparedValueHomeKind source_home_kind =
      prepare::PreparedValueHomeKind::None;
  prepare::PreparedValueHomeKind destination_home_kind =
      prepare::PreparedValueHomeKind::None;
  prepare::PreparedMoveStorageKind destination_storage_kind =
      prepare::PreparedMoveStorageKind::None;
  std::optional<std::string> source_register_name;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::int32_t> source_immediate_i32;
  std::optional<std::string> destination_register_name;
  bool immediate_source = false;
  prepare::PreparedEdgePublicationSourceProducerKind source_producer_kind =
      prepare::PreparedEdgePublicationSourceProducerKind::Unknown;
  std::optional<BlockLabelId> source_producer_block_label;
  std::optional<std::size_t> source_producer_instruction_index;
  const bir::LoadLocalInst* source_load_local = nullptr;
  const bir::LoadGlobalInst* source_load_global = nullptr;
  const bir::CastInst* source_cast = nullptr;
  const bir::BinaryInst* source_binary = nullptr;
  const bir::SelectInst* source_select = nullptr;
  prepare::PreparedValueFreshnessQueryStatus source_freshness_status =
      prepare::PreparedValueFreshnessQueryStatus::NoCandidate;
  std::size_t source_freshness_candidate_count = 0;
  prepare::PreparedValueFreshnessUseKind freshness_use_kind =
      prepare::PreparedValueFreshnessUseKind::Unknown;
  prepare::PreparedValueFreshnessSourceKind freshness_source_kind =
      prepare::PreparedValueFreshnessSourceKind::Unknown;
  prepare::PreparedValueFreshnessProofKind freshness_proof_kind =
      prepare::PreparedValueFreshnessProofKind::Unknown;
  prepare::PreparedValueFreshnessSourceRank freshness_rank =
      prepare::PreparedValueFreshnessSourceRank::None;
  std::optional<prepare::PreparedValueFreshnessAuthority>
      selected_freshness_authority;
};

struct PreparedMirDirectEdgePublicationSourceQuery {
  PreparedMirDirectEdgePublicationSourceQueryStatus status =
      PreparedMirDirectEdgePublicationSourceQueryStatus::MissingFunctionView;
  std::vector<PreparedMirDirectEdgePublicationSourceView> sources;
};

struct PreparedMirBranchStackLoadAuthorityView {
  bool freshness_required = false;
  bool available = true;
  prepare::PreparedBranchStackLoadAuthorityStatus authority_status =
      prepare::PreparedBranchStackLoadAuthorityStatus::Available;
  prepare::PreparedValueFreshnessQueryStatus source_freshness_status =
      prepare::PreparedValueFreshnessQueryStatus::Selected;
  std::size_t source_freshness_candidate_count = 0;
  prepare::PreparedBranchStackLoadRole role =
      prepare::PreparedBranchStackLoadRole::Condition;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t terminator_instruction_index = 0;
  prepare::PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  prepare::PreparedValueFreshnessUseKind freshness_use_kind =
      prepare::PreparedValueFreshnessUseKind::Unknown;
  prepare::PreparedValueFreshnessSourceKind freshness_source_kind =
      prepare::PreparedValueFreshnessSourceKind::Unknown;
  prepare::PreparedValueFreshnessProofKind freshness_proof_kind =
      prepare::PreparedValueFreshnessProofKind::Unknown;
  prepare::PreparedValueFreshnessSourceRank freshness_rank =
      prepare::PreparedValueFreshnessSourceRank::None;
};

// A bounded feature-facing copy of the producer-owned stack-destination row.
// This carrier deliberately has no lookup inputs: consuming it cannot select a
// route, inspect source order, or reconstruct authority from MIR.
struct PreparedMirStackDestinationAuthorityView {
  prepare::PreparedStackDestinationComposerInputStatus status =
      prepare::PreparedStackDestinationComposerInputStatus::MissingEvidence;
  prepare::PreparedStackDestinationPublicationStatus upstream_relationship_status =
      prepare::PreparedStackDestinationPublicationStatus::MissingPublication;
  prepare::PreparedEdgeCopySourceFactsStatus upstream_source_facts_status =
      prepare::PreparedEdgeCopySourceFactsStatus::MissingPublication;
  const prepare::PreparedStackDestinationPublication* relationship = nullptr;
  const prepare::PreparedEdgePublication* publication = nullptr;
  const prepare::PreparedMoveResolution* move = nullptr;
  const prepare::PreparedValueHome* source_home = nullptr;
  const prepare::PreparedValueHome* destination_home = nullptr;
  const prepare::PreparedValueFreshnessAuthority* source_freshness = nullptr;
  const prepare::PreparedFrameSlot* destination_frame_slot = nullptr;
  const prepare::PreparedStackObject* destination_stack_object = nullptr;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  std::optional<prepare::PreparedValueId> source_value_id;
  prepare::PreparedValueId destination_value_id = 0;
  std::optional<std::size_t> cursor_block_index;
  std::optional<std::size_t> cursor_instruction_index;
  prepare::PreparedStackDestinationEvidenceApplicability branch_stack_load_applicability =
      prepare::PreparedStackDestinationEvidenceApplicability::NotApplicable;
  prepare::PreparedStackDestinationEvidenceReason branch_stack_load_reason =
      prepare::PreparedStackDestinationEvidenceReason::None;
  prepare::PreparedStackDestinationEvidenceApplicability aggregate_source_applicability =
      prepare::PreparedStackDestinationEvidenceApplicability::NotApplicable;
  prepare::PreparedStackDestinationEvidenceReason aggregate_source_reason =
      prepare::PreparedStackDestinationEvidenceReason::None;
};

[[nodiscard]] PreparedMirStackDestinationAuthorityView
consume_prepared_stack_destination_authority(
    const prepare::PreparedStackDestinationAuthorityView& authority);

[[nodiscard]] PreparedMirBranchStackLoadAuthorityView
query_prepared_mir_branch_stack_load_authority(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedFunctionLookups& lookups,
    FunctionNameId function_name,
    const bir::Value* value,
    const prepare::PreparedValueHome* home,
    prepare::PreparedBranchStackLoadRole role,
    BlockLabelId block_label,
    std::size_t block_index,
    std::size_t terminator_instruction_index);

class PreparedMirFunctionView {
 public:
  PreparedMirFunctionView() = default;

  [[nodiscard]] explicit operator bool() const;

  [[nodiscard]] FunctionNameId function_name() const;
  [[nodiscard]] std::string_view function_name_text() const;

  [[nodiscard]] const bir::Function& bir_function() const;
  [[nodiscard]] const prepare::PreparedControlFlowFunction& control_flow() const;
  [[nodiscard]] const prepare::PreparedValueLocationFunction& value_locations() const;
  [[nodiscard]] const prepare::PreparedStackLayout& stack_layout() const;
  [[nodiscard]] const prepare::PreparedAddressingFunction& addressing() const;
  [[nodiscard]] const prepare::PreparedFunctionLookups& prepared_lookups() const;

  [[nodiscard]] const std::vector<PreparedMirBlockView>& blocks() const;
  [[nodiscard]] std::optional<PreparedMirInstructionCursor> instruction(
      std::size_t block_index,
      std::size_t instruction_index) const;
  [[nodiscard]] PreparedMirDirectEdgePublicationSourceQuery
  current_block_direct_edge_publication_sources(std::size_t block_index) const;
  [[nodiscard]] PreparedMirBranchStackLoadAuthorityView branch_stack_load_authority(
      const bir::Value* value,
      const prepare::PreparedValueHome* home,
      prepare::PreparedBranchStackLoadRole role,
      BlockLabelId block_label,
      std::size_t block_index,
      std::size_t terminator_instruction_index) const;

 private:
  friend class PreparedMirCoreView;

  PreparedMirFunctionView(const PreparedMirCoreView* core,
                          const PreparedMirFunctionEntry* entry);

  const PreparedMirCoreView* core_ = nullptr;
  const PreparedMirFunctionEntry* entry_ = nullptr;
};

class PreparedMirCoreView {
 public:
  explicit PreparedMirCoreView(const prepare::PreparedBirModule& module);

  [[nodiscard]] const TargetProfile& target_profile() const;
  [[nodiscard]] std::string_view target_triple() const;

  [[nodiscard]] const bir::NameTables& bir_names() const;
  [[nodiscard]] const prepare::PreparedNameTables& prepared_names() const;

  [[nodiscard]] const std::vector<bir::Function>& functions() const;
  [[nodiscard]] const std::vector<const bir::Function*>& defined_functions() const;
  [[nodiscard]] const std::vector<bir::Global>& globals() const;
  [[nodiscard]] const std::vector<bir::StringConstant>& string_constants() const;

  [[nodiscard]] std::optional<FunctionNameId> resolve_function_name(
      std::string_view name) const;
  [[nodiscard]] std::string_view function_name(FunctionNameId id) const;
  [[nodiscard]] const bir::Function* bir_function(FunctionNameId id) const;
  [[nodiscard]] const prepare::PreparedControlFlowFunction* control_flow(
      FunctionNameId id) const;

  [[nodiscard]] std::optional<PreparedMirFunctionView> function_view(
      FunctionNameId id) const;
  [[nodiscard]] std::optional<PreparedMirFunctionView> function_view(
      std::string_view name) const;
  [[nodiscard]] PreparedMirCoreSnapshot structural_snapshot() const;
  [[nodiscard]] std::string canonical_dump() const;

 private:
  friend class PreparedMirFunctionView;

  struct BirFunctionBinding {
    FunctionNameId function_name = kInvalidFunctionName;
    const bir::Function* function = nullptr;
    bool duplicate = false;
  };

  [[nodiscard]] const PreparedMirFunctionEntry* entry(FunctionNameId id) const;
  [[nodiscard]] const BirFunctionBinding* bir_binding(FunctionNameId id) const;

  const prepare::PreparedBirModule* module_ = nullptr;
  std::vector<const bir::Function*> defined_functions_;
  std::vector<BirFunctionBinding> bir_bindings_;
  std::vector<PreparedMirFunctionEntry> function_entries_;
};

[[nodiscard]] PreparedMirCoreComparisonReport compare_prepared_mir_core_views(
    const PreparedMirCoreView& lhs,
    const PreparedMirCoreView& rhs,
    const PreparedMirCoreComparisonOptions& options = {});

}  // namespace c4c::backend::mir::prepared
