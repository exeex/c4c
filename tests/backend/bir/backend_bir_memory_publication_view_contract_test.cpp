#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/bir_memory_access_view.hpp"
#include "src/backend/bir/bir_publication_view.hpp"
#include "src/backend/bir/query.hpp"
#include "src/backend/mir/query.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>

namespace {
namespace bir = c4c::backend::bir;

int fail(const char* message) { std::cerr << message << '\n'; return 1; }

bir::LoadLocalInst load(const char* result, const char* slot,
                        c4c::SlotNameId slot_id) {
  bir::MemoryAddress address;
  address.base_kind = bir::MemoryAddress::BaseKind::LocalSlot;
  address.base_name = slot;
  address.base_slot_id = slot_id;
  address.size_bytes = 8;
  address.align_bytes = 8;
  address.address_space = bir::AddressSpace::Fs;
  address.is_volatile = true;
  return {.result = bir::Value::named(bir::TypeKind::I64, result),
          .slot_name = slot,
          .address = address};
}

bir::LoadGlobalInst global_load(const char* result, const char* name,
                                c4c::LinkNameId name_id,
                                bir::MemoryAddress::BaseKind base_kind) {
  bir::MemoryAddress address;
  address.base_kind = base_kind;
  address.base_name = name;
  address.base_link_name_id = name_id;
  address.size_bytes = 8;
  address.align_bytes = 4;
  return {.result = bir::Value::named(bir::TypeKind::I64, result),
          .global_name = name,
          .global_name_id = name_id,
          .address = address};
}

bir::StoreLocalInst store(const char* value, const char* slot,
                          c4c::SlotNameId slot_id, std::int64_t offset = 0,
                          std::size_t size = 8) {
  bir::MemoryAddress address;
  address.base_kind = bir::MemoryAddress::BaseKind::LocalSlot;
  address.base_name = slot;
  address.base_slot_id = slot_id;
  address.byte_offset = offset;
  address.size_bytes = size;
  address.align_bytes = 8;
  return {.slot_name = slot,
          .value = bir::Value::named(bir::TypeKind::I64, value),
          .address = address};
}

int memory_states() {
  bir::Block block;
  block.label = "memory";
  block.insts.emplace_back(load("%unique", "slot.a", 11));
  block.insts.emplace_back(load("%duplicate", "slot.b", 12));
  block.insts.emplace_back(load("%duplicate", "slot.c", 13));
  block.insts.emplace_back(global_load("%global", "global.a", 21,
                                       bir::MemoryAddress::BaseKind::GlobalSymbol));
  block.insts.emplace_back(global_load("%string", ".str.0", 22,
                                       bir::MemoryAddress::BaseKind::StringConstant));
  const auto view = bir::make_bir_memory_access_view(block);
  const auto available = bir::find_memory_access(view, 0);
  const auto unavailable = bir::find_memory_access(view, 10);
  const auto incomplete = bir::find_memory_access_source(
      view, bir::Value::named(bir::TypeKind::I64, ""), 3);
  const auto ambiguous = bir::find_memory_access_source(
      view, bir::Value::named(bir::TypeKind::I64, "%duplicate"), 3);
  const auto global = bir::find_memory_access(view, 3);
  const auto string = bir::find_memory_access(view, 4);
  if (!available || available.kind != bir::BirMemoryAccessKind::LoadLocal ||
      available.base_kind != bir::BirMemoryBaseKind::LocalSlot ||
      available.instruction != &block.insts[0] ||
      available.base_name != "slot.a" || available.local_slot_name != "slot.a" ||
      available.local_slot_id != 11 || !available.global_name.empty() ||
      !available.string_constant_name.empty() || available.global_name_id != 0 ||
      available.string_constant_name_id != 0 ||
      available.address_space != bir::AddressSpace::Fs ||
      !available.is_volatile || available.align_bytes != 8 ||
      available.result_value == nullptr || available.result_value_name != "%unique" ||
      !available.stored_value_name.empty() || !available.pointer_base_name.empty() ||
      unavailable.status != bir::BirViewStatus::Unavailable || unavailable ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete ||
      ambiguous.status != bir::BirViewStatus::Ambiguous || ambiguous) {
    return fail("memory view did not expose all explicit states");
  }
  if (!global || global.global_name != "global.a" || global.global_name_id != 21 ||
      !global.local_slot_name.empty() || !global.string_constant_name.empty() ||
      !string || string.string_constant_name != ".str.0" ||
      string.string_constant_name_id != 22 || !string.local_slot_name.empty() ||
      !string.global_name.empty()) {
    return fail("memory view did not preserve distinct applicable base identities");
  }
  return 0;
}

int memory_identity_fails_closed() {
  bir::Block block;
  block.label = "invalid-memory";
  block.insts.emplace_back(load("%missing-id", "slot.a", c4c::kInvalidSlotName));
  block.insts.emplace_back(load("%conflicting", "slot.b", 12));
  auto& conflicting = std::get<bir::LoadLocalInst>(block.insts.back());
  conflicting.address->base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol;
  conflicting.address->base_link_name_id = 22;
  const auto view = bir::make_bir_memory_access_view(block);
  const auto incomplete = bir::find_memory_access(view, 0);
  const auto mismatched = bir::find_memory_access(view, 1);
  if (incomplete || incomplete.status != bir::BirViewStatus::Incomplete ||
      mismatched || mismatched.status != bir::BirViewStatus::Incomplete) {
    return fail("memory view accepted incomplete or mismatched stable identity");
  }
  return 0;
}

int stored_value_source_states() {
  bir::Block exact;
  exact.label = "stored-value";
  exact.insts.emplace_back(store("%stored", "slot.a", 31));
  exact.insts.emplace_back(load("%loaded", "slot.a", 31));
  const auto query = [&](bir::TypeKind type, std::string_view name,
                         const bir::Block* block = nullptr) {
    return bir::find_same_block_store_local_source({
        .block = block != nullptr ? block : &exact,
        .value_name = name,
        .value_type = type,
        .before_instruction_index =
            block != nullptr ? block->insts.size() : exact.insts.size(),
    });
  };
  const auto available = query(bir::TypeKind::I64, "%loaded");
  const auto mismatched = query(bir::TypeKind::I32, "%loaded");
  const auto incomplete = query(bir::TypeKind::I64, "");
  if (!available || available.load != &std::get<bir::LoadLocalInst>(exact.insts[1]) ||
      available.store != &std::get<bir::StoreLocalInst>(exact.insts[0]) ||
      available.loaded_value != &available.load->result ||
      available.stored_value != &available.store->value ||
      available.load_access.local_slot_id != 31 ||
      available.store_access.local_slot_id != 31 ||
      mismatched.status != bir::BirViewStatus::Unavailable || mismatched ||
      incomplete.status != bir::BirViewStatus::Incomplete || incomplete) {
    return fail("stored-value source did not preserve identity or explicit fail-closed states");
  }

  auto ambiguous = exact;
  ambiguous.insts.emplace_back(load("%loaded", "slot.a", 31));
  const auto duplicate = query(bir::TypeKind::I64, "%loaded", &ambiguous);
  auto overlap = exact;
  std::get<bir::LoadLocalInst>(overlap.insts[1]).address->byte_offset = 4;
  const auto partial = query(bir::TypeKind::I64, "%loaded", &overlap);
  if (duplicate.status != bir::BirViewStatus::Ambiguous || duplicate ||
      partial.status != bir::BirViewStatus::Incomplete || partial) {
    return fail("stored-value source accepted ambiguous or partial-range evidence");
  }
  namespace mir = c4c::backend::mir;
  const auto common_available =
      mir::find_bir_same_block_load_local_stored_value_source_identity({
          .block = &exact, .block_label = exact.label,
          .root_value_name = "%loaded", .root_value_type = bir::TypeKind::I64,
          .before_instruction_index = exact.insts.size()});
  const auto common_mismatched =
      mir::find_bir_same_block_load_local_stored_value_source_identity({
          .block = &exact, .block_label = exact.label,
          .root_value_name = "%loaded", .root_value_type = bir::TypeKind::I32,
          .before_instruction_index = exact.insts.size()});
  const auto common_ambiguous =
      mir::find_bir_same_block_load_local_stored_value_source_identity({
          .block = &ambiguous, .block_label = ambiguous.label,
          .root_value_name = "%loaded", .root_value_type = bir::TypeKind::I64,
          .before_instruction_index = ambiguous.insts.size()});
  if (!common_available ||
      common_available.status != bir::BirViewStatus::Available ||
      common_available.store_local != available.store ||
      common_available.stored_value.value != available.stored_value ||
      common_mismatched.status != bir::BirViewStatus::Unavailable ||
      common_mismatched ||
      common_ambiguous.status != bir::BirViewStatus::Ambiguous ||
      common_ambiguous) {
    return fail("common store-local source adapter did not preserve identity and explicit status");
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
  if (const int status = memory_identity_fails_closed(); status != 0) return status;
  if (const int status = stored_value_source_states(); status != 0) return status;
  if (const int status = publication_states(); status != 0) return status;
  return headers_are_route_free();
}
