#pragma once

#include "bir_producer_view.hpp"

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

namespace c4c::backend::bir {

struct Block;
struct Function;
struct Value;

enum class BirPublicationKind : unsigned char {
  Unknown,
  CurrentBlock,
  BlockEntry,
  CfgEdge,
};

struct BirPublicationResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  BirPublicationKind kind = BirPublicationKind::Unknown;
  const Value* published_value = nullptr;
  const Value* source_value = nullptr;
  std::size_t instruction_index = 0;
  std::string_view block_label;
  std::string_view predecessor_label;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available;
  }
};

class BirPublicationView {
 public:
  BirPublicationView() = default;
  [[nodiscard]] explicit operator bool() const { return implementation_ != nullptr; }

 private:
  struct Implementation;
  explicit BirPublicationView(std::shared_ptr<const Implementation> implementation)
      : implementation_(std::move(implementation)) {}
  std::shared_ptr<const Implementation> implementation_;

  friend BirPublicationView make_bir_publication_view(const Function& function);
  friend BirPublicationResult find_current_block_publication(
      const BirPublicationView&, const Block&, const Value&, std::size_t);
  friend BirPublicationResult find_block_entry_publication(
      const BirPublicationView&, const Block&, const Value&);
  friend struct BirPublicationCompatibilityAccess;
};

[[nodiscard]] BirPublicationView make_bir_publication_view(const Function& function);
[[nodiscard]] BirPublicationResult find_current_block_publication(
    const BirPublicationView& view,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index);
[[nodiscard]] BirPublicationResult find_block_entry_publication(
    const BirPublicationView& view,
    const Block& successor_block,
    const Value& destination_value);

}  // namespace c4c::backend::bir
