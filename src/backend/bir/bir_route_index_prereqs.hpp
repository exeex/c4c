#pragma once

#include <cstddef>
#include <string_view>

#include "../../shared/text_id_table.hpp"

namespace c4c::backend::bir {

struct Function;
struct Block;
struct Value;
struct Route1SourceValueIdentity;
struct Route4CurrentBlockPublicationRecord;
struct Route4BlockEntryPublicationRecord;
struct Route4PublicationAvailabilityIndex;
struct Route7ComparisonInstructionRecord;
struct Route7ComparisonOperandRecord;
struct Route7BranchConditionRecord;
struct Route7ComparisonConditionIndex;

enum class Route4PublicationAvailabilityStatus : unsigned char {
  Unavailable,
  Available,
  MissingBlock,
  MissingValue,
  MissingPublication,
  AlternateSource,
  NoMatch,
};

enum class Route7ComparisonStatus : unsigned char {
  Unavailable,
  Available,
  MissingBlock,
  MissingInstruction,
  WrongInstruction,
  NonComparison,
  MissingConditionValue,
  MissingOperandProducer,
  DuplicateProducer,
  AbsentProvenance,
  NoMatch,
};

enum class Route7ComparisonOperandRole : unsigned char {
  None,
  Lhs,
  Rhs,
  ConditionValue,
};

}  // namespace c4c::backend::bir
