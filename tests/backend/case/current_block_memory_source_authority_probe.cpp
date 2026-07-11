#include "src/backend/prealloc/publication_plans.hpp"

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int main() {
  const auto source = bir::Value::named(bir::TypeKind::I32, "%memory.source");
  const auto other = bir::Value::named(bir::TypeKind::I32, "%other");
  const auto result = bir::Value::named(bir::TypeKind::I32, "%result");
  const bir::BinaryInst producer{
      .result = source,
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  };
  const bir::Inst consumer = bir::BinaryInst{
      .result = result,
      .operand_type = bir::TypeKind::I32,
      .lhs = source,
      .rhs = other,
  };
  prepare::PreparedValueHome stack_home{
      .value_id = prepare::PreparedValueId{51},
      .value_name = c4c::ValueNameId{51},
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{3},
      .offset_bytes = 16,
      .size_bytes = 4,
      .align_bytes = 4,
  };
  prepare::PreparedEdgePublication publication{
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .source_value = source,
      .source_value_id = prepare::PreparedValueId{51},
      .source_value_name = c4c::ValueNameId{51},
      .source_value_kind = bir::Value::Kind::Named,
      .source_producer_kind =
          prepare::PreparedEdgePublicationSourceProducerKind::Binary,
      .source_binary = &producer,
      .source_home = &stack_home,
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
  };

  const auto authority =
      prepare::query_prepared_current_block_routed_operand_authority(
          publication, consumer, source);
  if (!authority || authority.prepared_source_identity != &publication.source_value ||
      authority.authoritative_value != &publication.source_value) {
    return 1;
  }

  auto inconsistent = publication;
  inconsistent.source_home_kind = prepare::PreparedValueHomeKind::Register;
  if (prepare::query_prepared_current_block_routed_operand_authority(
          inconsistent, consumer, source)) {
    return 2;
  }
  auto home_only = publication;
  home_only.source_value_id.reset();
  if (prepare::query_prepared_current_block_routed_operand_authority(
          home_only, consumer, source)) {
    return 3;
  }
  return 0;
}
