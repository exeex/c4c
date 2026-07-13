#pragma once

#include "ids.hpp"
#include "result.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace c4c::backend::bir {

class ModuleBuilder;
class FunctionBuilder;
class FoundationVerifier;

enum class ResolveError {
  WrongEpoch,
  WrongOwner,
  OutOfRange,
  Tombstone,
  StaleGeneration,
  WrongKind,
};

namespace detail {

enum class StorageError { InvalidOwner, InvalidId, SlotIndexExhausted };
enum class OrderError { Duplicate, Unknown };

template <class T>
struct Slot {
  Generation generation = 1;
  std::optional<T> value;
};

template <class Id, class Owner>
struct IdAccess;

template <>
struct IdAccess<FunctionId, ModuleEpoch> {
  template <class T>
  static FunctionId make(ModuleEpoch owner, SlotIndex slot, Generation generation,
                         const T&) {
    return {owner, slot, generation};
  }
  static bool owner_valid(ModuleEpoch owner) { return owner != 0; }
  static ResolveError check_owner(ModuleEpoch owner, FunctionId id) {
    return owner == id.epoch ? ResolveError::WrongOwner : ResolveError::WrongEpoch;
  }
  static bool owner_matches(ModuleEpoch owner, FunctionId id) { return owner == id.epoch; }
  static SlotIndex slot(FunctionId id) { return id.slot; }
  static Generation generation(FunctionId id) { return id.generation; }
  template <class T>
  static bool kind_matches(FunctionId, const T&) { return true; }
};

template <>
struct IdAccess<BlockId, FunctionId> {
  template <class T>
  static BlockId make(FunctionId owner, SlotIndex slot, Generation generation, const T&) {
    return {owner, slot, generation};
  }
  static bool owner_valid(FunctionId owner) { return owner.valid(); }
  static ResolveError check_owner(FunctionId owner, BlockId id) {
    return owner.epoch != id.owner.epoch ? ResolveError::WrongEpoch
                                        : ResolveError::WrongOwner;
  }
  static bool owner_matches(FunctionId owner, BlockId id) { return owner == id.owner; }
  static SlotIndex slot(BlockId id) { return id.slot; }
  static Generation generation(BlockId id) { return id.generation; }
  template <class T>
  static bool kind_matches(BlockId, const T&) { return true; }
};

template <>
struct IdAccess<InstId, FunctionId> {
  template <class T>
  static InstId make(FunctionId owner, SlotIndex slot, Generation generation, const T&) {
    return {owner, slot, generation};
  }
  static bool owner_valid(FunctionId owner) { return owner.valid(); }
  static ResolveError check_owner(FunctionId owner, InstId id) {
    return owner.epoch != id.owner.epoch ? ResolveError::WrongEpoch
                                        : ResolveError::WrongOwner;
  }
  static bool owner_matches(FunctionId owner, InstId id) { return owner == id.owner; }
  static SlotIndex slot(InstId id) { return id.slot; }
  static Generation generation(InstId id) { return id.generation; }
  template <class T>
  static bool kind_matches(InstId, const T&) { return true; }
};

template <>
struct IdAccess<ValueId, FunctionId> {
  template <class T>
  static ValueId make(FunctionId owner, SlotIndex slot, Generation generation,
                      const T& value) {
    return {owner, value.kind, slot, generation};
  }
  static bool owner_valid(FunctionId owner) { return owner.valid(); }
  static ResolveError check_owner(FunctionId owner, ValueId id) {
    return owner.epoch != id.owner.epoch ? ResolveError::WrongEpoch
                                        : ResolveError::WrongOwner;
  }
  static bool owner_matches(FunctionId owner, ValueId id) { return owner == id.owner; }
  static SlotIndex slot(ValueId id) { return id.slot; }
  static Generation generation(ValueId id) { return id.generation; }
  template <class T>
  static bool kind_matches(ValueId id, const T& value) { return id.kind == value.kind; }
};

template <class T, class Id, class Owner>
class SlotMap {
 public:
  Result<std::reference_wrapper<const T>, ResolveError> get(const Owner& owner,
                                                             Id id) const {
    auto checked = resolve_slot(owner, id);
    if (!checked)
      return Result<std::reference_wrapper<const T>, ResolveError>::failure(
          checked.error());
    return Result<std::reference_wrapper<const T>, ResolveError>::success(
        std::cref(*checked.value().get().value));
  }

  bool contains(const Owner& owner, Id id) const { return get(owner, id).has_value(); }

 private:
  Result<Id, StorageError> emplace(const Owner& owner, T value) {
    if (!IdAccess<Id, Owner>::owner_valid(owner))
      return Result<Id, StorageError>::failure(StorageError::InvalidOwner);

    SlotIndex index;
    if (!free_.empty()) {
      index = free_.back();
      free_.pop_back();
      auto& slot = slots_[index];
      slot.value.emplace(std::move(value));
      return Result<Id, StorageError>::success(
          IdAccess<Id, Owner>::make(owner, index, slot.generation, *slot.value));
    }

    if (slots_.size() > std::numeric_limits<SlotIndex>::max())
      return Result<Id, StorageError>::failure(StorageError::SlotIndexExhausted);
    index = static_cast<SlotIndex>(slots_.size());
    slots_.push_back(Slot<T>{1, std::optional<T>(std::move(value))});
    return Result<Id, StorageError>::success(
        IdAccess<Id, Owner>::make(owner, index, 1, *slots_.back().value));
  }

  Result<std::reference_wrapper<T>, ResolveError> get_mut(const Owner& owner, Id id) {
    auto checked = resolve_slot(owner, id);
    if (!checked)
      return Result<std::reference_wrapper<T>, ResolveError>::failure(checked.error());
    return Result<std::reference_wrapper<T>, ResolveError>::success(
        std::ref(*checked.value().get().value));
  }

  Result<void, StorageError> erase(const Owner& owner, Id id) {
    auto checked = resolve_slot(owner, id);
    if (!checked)
      return Result<void, StorageError>::failure(StorageError::InvalidId);

    auto& slot = checked.value().get();
    slot.value.reset();
    if (slot.generation != std::numeric_limits<Generation>::max()) {
      ++slot.generation;
      free_.push_back(IdAccess<Id, Owner>::slot(id));
    }
    return Result<void, StorageError>::success();
  }
  Result<std::reference_wrapper<Slot<T>>, ResolveError> resolve_slot(const Owner& owner,
                                                                     Id id) {
    if (!IdAccess<Id, Owner>::owner_matches(owner, id))
      return Result<std::reference_wrapper<Slot<T>>, ResolveError>::failure(
          IdAccess<Id, Owner>::check_owner(owner, id));
    const auto index = IdAccess<Id, Owner>::slot(id);
    if (static_cast<std::size_t>(index) >= slots_.size())
      return Result<std::reference_wrapper<Slot<T>>, ResolveError>::failure(
          ResolveError::OutOfRange);
    auto& slot = slots_[index];
    if (!slot.value)
      return Result<std::reference_wrapper<Slot<T>>, ResolveError>::failure(
          ResolveError::Tombstone);
    if (slot.generation != IdAccess<Id, Owner>::generation(id))
      return Result<std::reference_wrapper<Slot<T>>, ResolveError>::failure(
          ResolveError::StaleGeneration);
    if (!IdAccess<Id, Owner>::kind_matches(id, *slot.value))
      return Result<std::reference_wrapper<Slot<T>>, ResolveError>::failure(
          ResolveError::WrongKind);
    return Result<std::reference_wrapper<Slot<T>>, ResolveError>::success(std::ref(slot));
  }

  Result<std::reference_wrapper<const Slot<T>>, ResolveError> resolve_slot(
      const Owner& owner, Id id) const {
    if (!IdAccess<Id, Owner>::owner_matches(owner, id))
      return Result<std::reference_wrapper<const Slot<T>>, ResolveError>::failure(
          IdAccess<Id, Owner>::check_owner(owner, id));
    const auto index = IdAccess<Id, Owner>::slot(id);
    if (static_cast<std::size_t>(index) >= slots_.size())
      return Result<std::reference_wrapper<const Slot<T>>, ResolveError>::failure(
          ResolveError::OutOfRange);
    const auto& slot = slots_[index];
    if (!slot.value)
      return Result<std::reference_wrapper<const Slot<T>>, ResolveError>::failure(
          ResolveError::Tombstone);
    if (slot.generation != IdAccess<Id, Owner>::generation(id))
      return Result<std::reference_wrapper<const Slot<T>>, ResolveError>::failure(
          ResolveError::StaleGeneration);
    if (!IdAccess<Id, Owner>::kind_matches(id, *slot.value))
      return Result<std::reference_wrapper<const Slot<T>>, ResolveError>::failure(
          ResolveError::WrongKind);
    return Result<std::reference_wrapper<const Slot<T>>, ResolveError>::success(
        std::cref(slot));
  }

  std::vector<Slot<T>> slots_;
  std::vector<SlotIndex> free_;

  friend class ::c4c::backend::bir::ModuleBuilder;
  friend class ::c4c::backend::bir::FunctionBuilder;
  friend class ::c4c::backend::bir::FoundationVerifier;
};

template <class Id>
class IdOrder {
 public:
  const std::vector<Id>& ids() const noexcept { return ids_; }

 private:
  Result<void, OrderError> append(Id id) {
    if (std::find(ids_.begin(), ids_.end(), id) != ids_.end())
      return Result<void, OrderError>::failure(OrderError::Duplicate);
    ids_.push_back(id);
    return Result<void, OrderError>::success();
  }

  Result<void, OrderError> erase(Id id) {
    const auto found = std::find(ids_.begin(), ids_.end(), id);
    if (found == ids_.end())
      return Result<void, OrderError>::failure(OrderError::Unknown);
    ids_.erase(found);
    return Result<void, OrderError>::success();
  }

  std::vector<Id> ids_;

  friend class ::c4c::backend::bir::ModuleBuilder;
  friend class ::c4c::backend::bir::FunctionBuilder;
  friend class ::c4c::backend::bir::FoundationVerifier;
};

}  // namespace detail

}  // namespace c4c::backend::bir
