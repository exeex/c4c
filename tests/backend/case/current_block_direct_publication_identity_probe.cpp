#include "src/backend/prealloc/publication_plans.hpp"

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

prepare::PreparedEdgePublication publication(const bir::Value& source,
                                             const bir::BinaryInst& producer) {
  static prepare::PreparedValueHome home{
      .value_id = prepare::PreparedValueId{41},
      .value_name = c4c::ValueNameId{41},
      .kind = prepare::PreparedValueHomeKind::Register,
  };
  return {
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .source_value = source,
      .source_value_id = prepare::PreparedValueId{41},
      .source_value_name = c4c::ValueNameId{41},
      .source_value_kind = bir::Value::Kind::Named,
      .source_producer_kind =
          prepare::PreparedEdgePublicationSourceProducerKind::Binary,
      .source_binary = &producer,
      .source_home = &home,
      .source_home_kind = prepare::PreparedValueHomeKind::Register,
  };
}

}  // namespace

int main() {
  const auto published = bir::Value::named(bir::TypeKind::I32, "%published");
  const auto dependency = bir::Value::named(bir::TypeKind::I32, "%dependency");
  const auto result = bir::Value::named(bir::TypeKind::I32, "%result");
  const bir::BinaryInst producer{
      .result = published,
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  };
  const bir::Inst consumer = bir::BinaryInst{
      .result = result,
      .operand_type = bir::TypeKind::I32,
      .lhs = published,
      .rhs = dependency,
  };

  const auto direct = publication(published, producer);
  const auto authority =
      prepare::query_prepared_current_block_routed_operand_authority(
          direct, consumer, published);
  if (!authority || authority.prepared_source_identity != &direct.source_value ||
      authority.authoritative_value != &direct.source_value) {
    return 1;
  }

  // A dependency is not allowed to replace the publication's direct identity.
  if (prepare::query_prepared_current_block_routed_operand_authority(
          direct, consumer, dependency)) {
    return 2;
  }
  auto rewritten = direct;
  rewritten.source_value = dependency;
  if (prepare::query_prepared_current_block_routed_operand_authority(
          rewritten, consumer, dependency)) {
    return 3;
  }
  auto conflicting_name = direct;
  conflicting_name.source_value_name = c4c::ValueNameId{42};
  if (prepare::query_prepared_current_block_routed_operand_authority(
          conflicting_name, consumer, published)) {
    return 4;
  }
  auto missing_id = direct;
  missing_id.source_value_id.reset();
  if (prepare::query_prepared_current_block_routed_operand_authority(
          missing_id, consumer, published)) {
    return 5;
  }
  auto missing_home = direct;
  missing_home.source_home = nullptr;
  if (prepare::query_prepared_current_block_routed_operand_authority(
          missing_home, consumer, published)) {
    return 6;
  }
  auto conflicting_home = direct;
  auto other_home = *direct.source_home;
  other_home.value_id = prepare::PreparedValueId{42};
  conflicting_home.source_home = &other_home;
  if (prepare::query_prepared_current_block_routed_operand_authority(
          conflicting_home, consumer, published)) {
    return 7;
  }
  const bir::Inst non_consumer = bir::CastInst{
      .result = result,
      .operand = dependency,
  };
  if (prepare::query_prepared_current_block_routed_operand_authority(
          direct, non_consumer, published)) {
    return 8;
  }
  return 0;
}
