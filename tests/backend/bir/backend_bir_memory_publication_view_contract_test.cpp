#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/bir_memory_access_view.hpp"
#include "src/backend/bir/bir_publication_view.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>

namespace {
namespace bir = c4c::backend::bir;

int fail(const char* message) { std::cerr << message << '\n'; return 1; }

bir::LoadLocalInst load(const char* result, const char* slot) {
  bir::MemoryAddress address;
  address.base_kind = bir::MemoryAddress::BaseKind::LocalSlot;
  address.base_name = slot;
  address.size_bytes = 8;
  return {.result = bir::Value::named(bir::TypeKind::I64, result),
          .slot_name = slot,
          .address = address};
}

int memory_states() {
  bir::Block block;
  block.label = "memory";
  block.insts.emplace_back(load("%unique", "slot.a"));
  block.insts.emplace_back(load("%duplicate", "slot.b"));
  block.insts.emplace_back(load("%duplicate", "slot.c"));
  const auto view = bir::make_bir_memory_access_view(block);
  const auto available = bir::find_memory_access(view, 0);
  const auto unavailable = bir::find_memory_access(view, 10);
  const auto incomplete = bir::find_memory_access_source(
      view, bir::Value::named(bir::TypeKind::I64, ""), 3);
  const auto ambiguous = bir::find_memory_access_source(
      view, bir::Value::named(bir::TypeKind::I64, "%duplicate"), 3);
  if (!available || available.kind != bir::BirMemoryAccessKind::LoadLocal ||
      available.base_kind != bir::BirMemoryBaseKind::LocalSlot ||
      available.base_name != "slot.a" || available.result_value == nullptr ||
      unavailable.status != bir::BirViewStatus::Unavailable || unavailable ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete ||
      ambiguous.status != bir::BirViewStatus::Ambiguous || ambiguous) {
    return fail("memory view did not expose all explicit states");
  }
  return 0;
}

bir::BinaryInst producer(const char* name) {
  return {.opcode = bir::BinaryOpcode::Add,
          .result = bir::Value::named(bir::TypeKind::I64, name),
          .operand_type = bir::TypeKind::I64,
          .lhs = bir::Value::immediate_i64(1),
          .rhs = bir::Value::immediate_i64(2)};
}

int publication_states() {
  bir::Function function;
  function.name = "publication";
  function.blocks.emplace_back();
  auto& block = function.blocks.back();
  block.label = "entry";
  block.insts.emplace_back(producer("%unique"));
  block.insts.emplace_back(producer("%duplicate"));
  block.insts.emplace_back(producer("%duplicate"));
  const auto view = bir::make_bir_publication_view(function);
  const auto available = bir::find_current_block_publication(
      view, block, bir::Value::named(bir::TypeKind::I64, "%unique"), 3);
  const auto unavailable = bir::find_current_block_publication(
      view, block, bir::Value::named(bir::TypeKind::I64, "%missing"), 3);
  const auto incomplete = bir::find_current_block_publication(
      view, block, bir::Value::named(bir::TypeKind::I64, ""), 3);
  const auto ambiguous = bir::find_current_block_publication(
      view, block, bir::Value::named(bir::TypeKind::I64, "%duplicate"), 3);
  if (!available || available.kind != bir::BirPublicationKind::CurrentBlock ||
      available.published_value == nullptr ||
      unavailable.status != bir::BirViewStatus::Unavailable || unavailable ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete ||
      ambiguous.status != bir::BirViewStatus::Ambiguous || ambiguous) {
    return fail("publication view did not expose all explicit states");
  }
  return 0;
}

int headers_are_route_free() {
  for (const char* path : {C4C_BIR_MEMORY_VIEW_HEADER,
                           C4C_BIR_PUBLICATION_VIEW_HEADER}) {
    std::ifstream input(path);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    for (const char* forbidden : {"Route1", "Route2", "Route3", "Route4",
                                  "Route5", "Route6", "Route7", "Route8",
                                  "RouteIndex", "route_index"}) {
      if (source.find(forbidden) != std::string::npos) {
        return fail("named view header exposed forbidden route vocabulary");
      }
    }
  }
  return 0;
}
}  // namespace

int main() {
  static_assert(std::is_default_constructible_v<bir::BirMemoryAccessView>);
  static_assert(std::is_default_constructible_v<bir::BirPublicationView>);
  if (const int status = memory_states(); status != 0) return status;
  if (const int status = publication_states(); status != 0) return status;
  return headers_are_route_free();
}
