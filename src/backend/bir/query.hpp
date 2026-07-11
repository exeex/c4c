#pragma once

#include "bir_memory_access_view.hpp"

#include <cstddef>
#include <string_view>

namespace c4c::backend::bir {

struct Block;
struct LoadGlobalInst;
struct LoadLocalInst;
struct Value;
enum class TypeKind : unsigned char;

struct BirSameBlockGlobalLoadRequest {
  const Block* block = nullptr;
  const Value* value = nullptr;
  std::string_view value_name;
  TypeKind value_type;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr && (value != nullptr || !value_name.empty());
  }
};

struct BirSameBlockGlobalLoadResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  BirMemoryAccessResult access;
  const LoadGlobalInst* load = nullptr;
  const Value* result_value = nullptr;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available && access && load != nullptr &&
           result_value != nullptr;
  }
};

[[nodiscard]] BirSameBlockGlobalLoadResult find_same_block_global_load(
    BirSameBlockGlobalLoadRequest request);

struct BirSameBlockLoadLocalRequest {
  const Block* block = nullptr;
  const Value* value = nullptr;
  std::string_view value_name;
  TypeKind value_type;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr && (value != nullptr || !value_name.empty());
  }
};

struct BirSameBlockLoadLocalResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  BirMemoryAccessResult access;
  const LoadLocalInst* load = nullptr;
  const Value* result_value = nullptr;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available && access && load != nullptr &&
           result_value != nullptr;
  }
};

[[nodiscard]] BirSameBlockLoadLocalResult find_same_block_load_local_source(
    BirSameBlockLoadLocalRequest request);

}  // namespace c4c::backend::bir
