#pragma once

#include <cstddef>
#include <string_view>

#include "bir_producer_view.hpp"

namespace c4c::backend::bir {

struct Block;
struct LoadGlobalInst;
struct Value;

enum class BirSelectDependencyStatus : unsigned char {
  CompleteDirectGlobal,
  CompleteNoDependency,
  CompleteStopped,
  Unavailable,
  Incomplete,
  Ambiguous,
  Mismatched,
};

struct BirSelectDependencyRequest {
  const Block* block = nullptr;
  const Value* root_value = nullptr;
  std::string_view root_value_name;
  std::string_view block_label;
  std::size_t before_instruction_index = 0;
};

struct BirSelectDependencyResult {
  BirSelectDependencyStatus status = BirSelectDependencyStatus::Unavailable;
  const Block* block = nullptr;
  std::string_view block_label;
  const Value* root_value = nullptr;
  BirProducerResult root_producer;
  std::size_t root_instruction_index = 0;
  const Value* dependency_value = nullptr;
  const LoadGlobalInst* dependency_load = nullptr;
  std::size_t dependency_instruction_index = 0;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] bool complete() const {
    return status == BirSelectDependencyStatus::CompleteDirectGlobal ||
           status == BirSelectDependencyStatus::CompleteNoDependency ||
           status == BirSelectDependencyStatus::CompleteStopped;
  }
};

[[nodiscard]] BirSelectDependencyResult find_bir_select_dependency(
    BirSelectDependencyRequest request);

}  // namespace c4c::backend::bir
