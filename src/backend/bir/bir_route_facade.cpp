#include "bir.hpp"

namespace c4c::backend::bir {

RouteIndexReferenceFacade route_index_reference_facade(
    const Route4PublicationAvailabilityIndex& route4_publications) {
  return RouteIndexReferenceFacade{
      .route4_publications = &route4_publications,
  };
}

RouteIndexReferenceFacade route_index_reference_facade(
    const Route7ComparisonConditionIndex& route7_comparisons) {
  return RouteIndexReferenceFacade{
      .route7_comparisons = &route7_comparisons,
  };
}

RouteIndexReferenceFacade route_index_reference_facade(
    const Route4PublicationAvailabilityIndex& route4_publications,
    const Route7ComparisonConditionIndex& route7_comparisons) {
  return RouteIndexReferenceFacade{
      .route4_publications = &route4_publications,
      .route7_comparisons = &route7_comparisons,
  };
}

Route4IndexReferenceValidation validate_block_entry_publication_reference(
    const Route4PublicationAvailabilityIndex& index,
    const Block& successor_block,
    const Value& destination_value) {
  return route4_validate_block_entry_publication_reference(
      index, successor_block, destination_value);
}

Route4IndexReferenceValidation
route_index_validate_current_block_publication_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index) {
  if (facade.route4_publications == nullptr) {
    return Route4IndexReferenceValidation{
        .valid = false,
        .status = RouteIndexValidationStatus::MissingRecord,
        .route_status = Route4PublicationAvailabilityStatus::MissingBlock,
        .reference =
            RouteIndexRecordReference{
                .route = RouteIndexRoute::Route4PublicationAvailability,
                .owner_scope = RouteIndexOwnerScope::None,
                .record_category =
                    RouteIndexRecordCategory::Route4CurrentBlockPublication,
                .relationship =
                    RouteIndexRelationshipKind::Route4CurrentBlockPublication,
                .block = &block,
                .block_label = block.label,
                .block_label_id = block.label_id,
                .instruction_index = before_instruction_index,
                .before_instruction_index = before_instruction_index,
                .value = route1_source_value_identity(value),
            },
    };
  }
  return route4_validate_current_block_publication_reference(
      *facade.route4_publications, block, value, before_instruction_index);
}

Route4IndexReferenceValidation
route_index_validate_block_entry_publication_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& successor_block,
    const Value& destination_value) {
  if (facade.route4_publications == nullptr) {
    return Route4IndexReferenceValidation{
        .valid = false,
        .status = RouteIndexValidationStatus::MissingRecord,
        .route_status = Route4PublicationAvailabilityStatus::MissingBlock,
        .reference =
            RouteIndexRecordReference{
                .route = RouteIndexRoute::Route4PublicationAvailability,
                .owner_scope = RouteIndexOwnerScope::None,
                .record_category =
                    RouteIndexRecordCategory::Route4BlockEntryPublication,
                .relationship =
                    RouteIndexRelationshipKind::Route4BlockEntryPublication,
                .block = &successor_block,
                .block_label = successor_block.label,
                .block_label_id = successor_block.label_id,
                .value = route1_source_value_identity(destination_value),
            },
    };
  }
  return route4_validate_block_entry_publication_reference(
      *facade.route4_publications, successor_block, destination_value);
}

Route7IndexReferenceValidation route_index_validate_comparison_operand_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role) {
  if (facade.route7_comparisons == nullptr) {
    return Route7IndexReferenceValidation{
        .valid = false,
        .status = RouteIndexValidationStatus::MissingRecord,
        .route_status = Route7ComparisonStatus::MissingBlock,
        .reference =
            RouteIndexRecordReference{
                .route = RouteIndexRoute::Route7ComparisonCondition,
                .owner_scope = RouteIndexOwnerScope::None,
                .record_category = RouteIndexRecordCategory::Route7ComparisonOperand,
                .relationship = RouteIndexRelationshipKind::Route7Operand,
                .block = &block,
                .block_label = block.label,
                .block_label_id = block.label_id,
                .instruction_index = before_instruction_index,
                .before_instruction_index = before_instruction_index,
                .value = route1_source_value_identity(value),
                .operand_role = role,
            },
    };
  }
  return route7_validate_comparison_operand_reference(
      *facade.route7_comparisons, block, value, before_instruction_index, role);
}

Route7IndexReferenceValidation
route_index_validate_materialized_condition_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index) {
  if (facade.route7_comparisons == nullptr) {
    return Route7IndexReferenceValidation{
        .valid = false,
        .status = RouteIndexValidationStatus::MissingRecord,
        .route_status = Route7ComparisonStatus::MissingBlock,
        .reference =
            RouteIndexRecordReference{
                .route = RouteIndexRoute::Route7ComparisonCondition,
                .owner_scope = RouteIndexOwnerScope::None,
                .record_category =
                    RouteIndexRecordCategory::Route7ComparisonInstruction,
                .relationship =
                    RouteIndexRelationshipKind::Route7MaterializedCondition,
                .block = &block,
                .block_label = block.label,
                .block_label_id = block.label_id,
                .instruction_index = before_instruction_index,
                .before_instruction_index = before_instruction_index,
                .value = route1_source_value_identity(condition_value),
                .operand_role = Route7ComparisonOperandRole::ConditionValue,
            },
    };
  }
  return route7_validate_materialized_condition_reference(
      *facade.route7_comparisons,
      block,
      condition_value,
      before_instruction_index);
}

}  // namespace c4c::backend::bir
