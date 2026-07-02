#pragma once

enum class RouteIndexRoute : unsigned char {
  Unknown,
  Route4PublicationAvailability,
  Route7ComparisonCondition,
};

enum class RouteIndexOwnerScope : unsigned char {
  None,
  Function,
  Block,
};

enum class RouteIndexRecordCategory : unsigned char {
  Unknown,
  Route4CurrentBlockPublication,
  Route4BlockEntryPublication,
  Route7ComparisonInstruction,
  Route7ComparisonOperand,
  Route7BranchCondition,
};

enum class RouteIndexRelationshipKind : unsigned char {
  None,
  Route4CurrentBlockPublication,
  Route4BlockEntryPublication,
  Route7Instruction,
  Route7Operand,
  Route7MaterializedCondition,
  Route7BranchCondition,
};

enum class RouteIndexValidationStatus : unsigned char {
  Unavailable,
  Valid,
  MissingRecord,
  StaleOwner,
  WrongRecordCategory,
  WrongRelationship,
  WrongKey,
  DuplicateReference,
  AbsentProvenance,
  NoMatch,
  Diverged,
};

struct RouteIndexRecordReference {
  RouteIndexRoute route = RouteIndexRoute::Unknown;
  RouteIndexOwnerScope owner_scope = RouteIndexOwnerScope::None;
  RouteIndexRecordCategory record_category =
      RouteIndexRecordCategory::Unknown;
  RouteIndexRelationshipKind relationship =
      RouteIndexRelationshipKind::None;
  const Function* function = nullptr;
  const Block* block = nullptr;
  std::string_view block_label;
  BlockLabelId block_label_id = kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  std::size_t before_instruction_index = 0;
  Route1SourceValueIdentity value;
  Route7ComparisonOperandRole operand_role =
      Route7ComparisonOperandRole::None;
  std::size_t record_index = 0;
};

struct RouteIndexReferenceFacade {
  const Route4PublicationAvailabilityIndex* route4_publications = nullptr;
  const Route7ComparisonConditionIndex* route7_comparisons = nullptr;

  [[nodiscard]] explicit operator bool() const {
    return route4_publications != nullptr || route7_comparisons != nullptr;
  }
};

struct Route4IndexReferenceValidation {
  bool valid = false;
  RouteIndexValidationStatus status =
      RouteIndexValidationStatus::Unavailable;
  Route4PublicationAvailabilityStatus route_status =
      Route4PublicationAvailabilityStatus::Unavailable;
  RouteIndexRecordReference reference;
  const Route4CurrentBlockPublicationRecord* current_block_record = nullptr;
  const Route4BlockEntryPublicationRecord* block_entry_record = nullptr;

  [[nodiscard]] explicit operator bool() const { return valid; }
};

struct Route7IndexReferenceValidation {
  bool valid = false;
  RouteIndexValidationStatus status =
      RouteIndexValidationStatus::Unavailable;
  Route7ComparisonStatus route_status =
      Route7ComparisonStatus::Unavailable;
  RouteIndexRecordReference reference;
  const Route7ComparisonInstructionRecord* comparison_record = nullptr;
  const Route7ComparisonOperandRecord* operand_record = nullptr;
  const Route7BranchConditionRecord* branch_condition_record = nullptr;

  [[nodiscard]] explicit operator bool() const { return valid; }
};

[[nodiscard]] Route4IndexReferenceValidation
route4_validate_current_block_publication_reference(
    const Route4PublicationAvailabilityIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index);
[[nodiscard]] Route4IndexReferenceValidation
route4_validate_block_entry_publication_reference(
    const Route4PublicationAvailabilityIndex& index,
    const Block& successor_block,
    const Value& destination_value);
[[nodiscard]] Route7IndexReferenceValidation
route7_validate_comparison_instruction_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    std::size_t instruction_index);
[[nodiscard]] Route7IndexReferenceValidation
route7_validate_comparison_operand_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role);
[[nodiscard]] Route7IndexReferenceValidation
route7_validate_materialized_condition_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index);
[[nodiscard]] Route7IndexReferenceValidation
route7_validate_branch_condition_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block);
[[nodiscard]] RouteIndexReferenceFacade route_index_reference_facade(
    const Route4PublicationAvailabilityIndex& route4_publications);
[[nodiscard]] RouteIndexReferenceFacade route_index_reference_facade(
    const Route7ComparisonConditionIndex& route7_comparisons);
[[nodiscard]] RouteIndexReferenceFacade route_index_reference_facade(
    const Route4PublicationAvailabilityIndex& route4_publications,
    const Route7ComparisonConditionIndex& route7_comparisons);
[[nodiscard]] Route4IndexReferenceValidation
route_index_validate_current_block_publication_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index);
[[nodiscard]] Route4IndexReferenceValidation
route_index_validate_block_entry_publication_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& successor_block,
    const Value& destination_value);
[[nodiscard]] Route7IndexReferenceValidation
route_index_validate_comparison_operand_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role);
[[nodiscard]] Route7IndexReferenceValidation
route_index_validate_materialized_condition_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index);
