#include "src/backend/prealloc/publication_plans.hpp"

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

prepare::PreparedEdgePublication named_publication(const bir::Value& source) {
  static prepare::PreparedValueHome source_home{
      .value_id = prepare::PreparedValueId{20},
      .value_name = c4c::ValueNameId{20},
      .kind = prepare::PreparedValueHomeKind::Register,
  };
  return {
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .source_value = source,
      .source_value_id = prepare::PreparedValueId{20},
      .source_value_name = c4c::ValueNameId{20},
      .source_value_kind = bir::Value::Kind::Named,
      .source_home = &source_home,
      .source_home_kind = prepare::PreparedValueHomeKind::Register,
  };
}

prepare::PreparedEdgePublication immediate_publication(const bir::Value& immediate) {
  static prepare::PreparedValueHome destination_home{
      .value_id = prepare::PreparedValueId{10},
      .value_name = c4c::ValueNameId{10},
      .kind = prepare::PreparedValueHomeKind::Register,
  };
  return {
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .destination_value = bir::Value::named(bir::TypeKind::I32, "%join"),
      .source_value = immediate,
      .destination_value_id = prepare::PreparedValueId{10},
      .destination_value_name = c4c::ValueNameId{10},
      .source_value_kind = bir::Value::Kind::Immediate,
      .destination_home = &destination_home,
      .destination_home_kind = prepare::PreparedValueHomeKind::Register,
  };
}

bool authoritative(const prepare::PreparedEdgePublication& publication,
                   const bir::Inst& consumer,
                   const bir::Value& operand,
                   bool immediate_destination = false) {
  const auto authority =
      prepare::query_prepared_current_block_routed_operand_authority(
          publication, consumer, operand);
  return authority &&
         authority.prepared_source_identity == &publication.source_value &&
         authority.authoritative_value ==
             (immediate_destination ? &publication.destination_value
                                    : &publication.source_value) &&
         authority.immediate_destination_authority == immediate_destination;
}

}  // namespace

int main() {
  const auto source = bir::Value::named(bir::TypeKind::I32, "%source");
  const auto other = bir::Value::named(bir::TypeKind::I32, "%other");
  const auto immediate = bir::Value::immediate_i32(7);
  const auto result = bir::Value::named(bir::TypeKind::I32, "%result");
  const bir::Inst binary = bir::BinaryInst{
      .result = result, .operand_type = bir::TypeKind::I32,
      .lhs = source, .rhs = other};
  const bir::Inst cast = bir::CastInst{
      .result = result, .operand = source};
  const bir::Inst select = bir::SelectInst{
      .result = result, .compare_type = bir::TypeKind::I32,
      .lhs = other, .rhs = immediate, .true_value = source, .false_value = other};

  const auto named = named_publication(source);
  if (!authoritative(named, binary, source) ||
      !authoritative(named, cast, source) ||
      !authoritative(named, select, source)) {
    return 1;
  }
  if (authoritative(named, binary, other)) {
    return 2;
  }
  auto conflicting = named;
  conflicting.source_value_name = c4c::ValueNameId{21};
  if (authoritative(conflicting, binary, source)) {
    return 3;
  }

  const auto immediate_source = immediate_publication(immediate);
  if (!authoritative(immediate_source, select, immediate, true)) {
    return 4;
  }
  auto conflicting_destination = immediate_source;
  conflicting_destination.destination_value_name = c4c::ValueNameId{11};
  if (authoritative(conflicting_destination, select, immediate, true)) {
    return 5;
  }
  auto rewritten_source = immediate_source;
  rewritten_source.source_value = source;
  if (authoritative(rewritten_source, select, immediate, true)) {
    return 6;
  }
  if (authoritative(immediate_source, cast, immediate, true)) {
    return 7;
  }
  return 0;
}
