#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

namespace c4c::backend::bir {

using ModuleEpoch = std::uint64_t;
using SlotIndex = std::uint32_t;
using Generation = std::uint32_t;

struct FunctionId {
  ModuleEpoch epoch = 0;
  SlotIndex slot = 0;
  Generation generation = 0;

  constexpr bool valid() const noexcept { return epoch != 0 && generation != 0; }
};

// Append-only module identities.  Unlike the source LIR's integer name IDs,
// these carry their owning Raw-BIR epoch and cannot be confused across name
// domains or modules.
struct LinkNameId {
  ModuleEpoch epoch = 0;
  SlotIndex slot = 0;
  constexpr bool valid() const noexcept { return epoch != 0; }
};

struct StructNameId {
  ModuleEpoch epoch = 0;
  SlotIndex slot = 0;
  constexpr bool valid() const noexcept { return epoch != 0; }
};

struct StructDeclId {
  ModuleEpoch epoch = 0;
  SlotIndex slot = 0;
  constexpr bool valid() const noexcept { return epoch != 0; }
};

struct ConstantId {
  ModuleEpoch epoch = 0;
  SlotIndex slot = 0;
  constexpr bool valid() const noexcept { return epoch != 0; }
};

struct BlockId {
  FunctionId owner{};
  SlotIndex slot = 0;
  Generation generation = 0;

  constexpr bool valid() const noexcept { return owner.valid() && generation != 0; }
};

struct InstId {
  FunctionId owner{};
  SlotIndex slot = 0;
  Generation generation = 0;

  constexpr bool valid() const noexcept { return owner.valid() && generation != 0; }
};

enum class ValueKind : std::uint8_t { Parameter, Ordinary };

struct ValueId {
  FunctionId owner{};
  ValueKind kind = ValueKind::Parameter;
  SlotIndex slot = 0;
  Generation generation = 0;

  constexpr bool valid() const noexcept { return owner.valid() && generation != 0; }
};

// A typed reference back to an authoritative function-local source value ID.
// Source value zero is valid; UINT32_MAX is the invalid sentinel used by LIR.
struct SourceValueId {
  FunctionId owner{};
  std::uint32_t value = std::numeric_limits<std::uint32_t>::max();

  constexpr bool valid() const noexcept {
    return owner.valid() && value != std::numeric_limits<std::uint32_t>::max();
  }
};

constexpr bool operator==(FunctionId lhs, FunctionId rhs) noexcept {
  return lhs.epoch == rhs.epoch && lhs.slot == rhs.slot &&
         lhs.generation == rhs.generation;
}
constexpr bool operator!=(FunctionId lhs, FunctionId rhs) noexcept { return !(lhs == rhs); }
constexpr bool operator==(LinkNameId lhs, LinkNameId rhs) noexcept {
  return lhs.epoch == rhs.epoch && lhs.slot == rhs.slot;
}
constexpr bool operator!=(LinkNameId lhs, LinkNameId rhs) noexcept { return !(lhs == rhs); }
constexpr bool operator==(StructNameId lhs, StructNameId rhs) noexcept {
  return lhs.epoch == rhs.epoch && lhs.slot == rhs.slot;
}
constexpr bool operator!=(StructNameId lhs, StructNameId rhs) noexcept { return !(lhs == rhs); }
constexpr bool operator==(StructDeclId lhs, StructDeclId rhs) noexcept {
  return lhs.epoch == rhs.epoch && lhs.slot == rhs.slot;
}
constexpr bool operator!=(StructDeclId lhs, StructDeclId rhs) noexcept { return !(lhs == rhs); }
constexpr bool operator==(ConstantId lhs, ConstantId rhs) noexcept {
  return lhs.epoch == rhs.epoch && lhs.slot == rhs.slot;
}
constexpr bool operator!=(ConstantId lhs, ConstantId rhs) noexcept {
  return !(lhs == rhs);
}
constexpr bool operator==(BlockId lhs, BlockId rhs) noexcept {
  return lhs.owner == rhs.owner && lhs.slot == rhs.slot &&
         lhs.generation == rhs.generation;
}
constexpr bool operator!=(BlockId lhs, BlockId rhs) noexcept { return !(lhs == rhs); }
constexpr bool operator==(InstId lhs, InstId rhs) noexcept {
  return lhs.owner == rhs.owner && lhs.slot == rhs.slot &&
         lhs.generation == rhs.generation;
}
constexpr bool operator!=(InstId lhs, InstId rhs) noexcept { return !(lhs == rhs); }
constexpr bool operator==(ValueId lhs, ValueId rhs) noexcept {
  return lhs.owner == rhs.owner && lhs.kind == rhs.kind && lhs.slot == rhs.slot &&
         lhs.generation == rhs.generation;
}
constexpr bool operator!=(ValueId lhs, ValueId rhs) noexcept { return !(lhs == rhs); }
constexpr bool operator==(SourceValueId lhs, SourceValueId rhs) noexcept {
  return lhs.owner == rhs.owner && lhs.value == rhs.value;
}
constexpr bool operator!=(SourceValueId lhs, SourceValueId rhs) noexcept {
  return !(lhs == rhs);
}

namespace detail {

inline std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
  return seed ^ (value + static_cast<std::size_t>(0x9e3779b9U) + (seed << 6U) +
                 (seed >> 2U));
}

inline std::size_t hash_function_id(FunctionId id) noexcept {
  auto result = std::hash<ModuleEpoch>{}(id.epoch);
  result = hash_combine(result, std::hash<SlotIndex>{}(id.slot));
  return hash_combine(result, std::hash<Generation>{}(id.generation));
}

}  // namespace detail

static_assert(std::is_trivially_copyable_v<FunctionId>);
static_assert(std::is_trivially_copyable_v<LinkNameId>);
static_assert(std::is_trivially_copyable_v<StructNameId>);
static_assert(std::is_trivially_copyable_v<StructDeclId>);
static_assert(std::is_trivially_copyable_v<ConstantId>);
static_assert(std::is_trivially_copyable_v<BlockId>);
static_assert(std::is_trivially_copyable_v<InstId>);
static_assert(std::is_trivially_copyable_v<ValueId>);
static_assert(std::is_trivially_copyable_v<SourceValueId>);

}  // namespace c4c::backend::bir

namespace std {

template <>
struct hash<c4c::backend::bir::FunctionId> {
  size_t operator()(c4c::backend::bir::FunctionId id) const noexcept {
    return c4c::backend::bir::detail::hash_function_id(id);
  }
};

template <>
struct hash<c4c::backend::bir::LinkNameId> {
  size_t operator()(c4c::backend::bir::LinkNameId id) const noexcept {
    return c4c::backend::bir::detail::hash_combine(hash<c4c::backend::bir::ModuleEpoch>{}(id.epoch), hash<c4c::backend::bir::SlotIndex>{}(id.slot));
  }
};

template <>
struct hash<c4c::backend::bir::StructNameId> {
  size_t operator()(c4c::backend::bir::StructNameId id) const noexcept {
    return c4c::backend::bir::detail::hash_combine(hash<c4c::backend::bir::ModuleEpoch>{}(id.epoch), hash<c4c::backend::bir::SlotIndex>{}(id.slot));
  }
};

template <>
struct hash<c4c::backend::bir::StructDeclId> {
  size_t operator()(c4c::backend::bir::StructDeclId id) const noexcept {
    return c4c::backend::bir::detail::hash_combine(hash<c4c::backend::bir::ModuleEpoch>{}(id.epoch), hash<c4c::backend::bir::SlotIndex>{}(id.slot));
  }
};

template <>
struct hash<c4c::backend::bir::ConstantId> {
  size_t operator()(c4c::backend::bir::ConstantId id) const noexcept {
    return c4c::backend::bir::detail::hash_combine(
        hash<c4c::backend::bir::ModuleEpoch>{}(id.epoch),
        hash<c4c::backend::bir::SlotIndex>{}(id.slot));
  }
};

template <>
struct hash<c4c::backend::bir::BlockId> {
  size_t operator()(c4c::backend::bir::BlockId id) const noexcept {
    auto result = c4c::backend::bir::detail::hash_function_id(id.owner);
    result = c4c::backend::bir::detail::hash_combine(
        result, hash<c4c::backend::bir::SlotIndex>{}(id.slot));
    return c4c::backend::bir::detail::hash_combine(
        result, hash<c4c::backend::bir::Generation>{}(id.generation));
  }
};

template <>
struct hash<c4c::backend::bir::InstId> {
  size_t operator()(c4c::backend::bir::InstId id) const noexcept {
    auto result = c4c::backend::bir::detail::hash_function_id(id.owner);
    result = c4c::backend::bir::detail::hash_combine(
        result, hash<c4c::backend::bir::SlotIndex>{}(id.slot));
    return c4c::backend::bir::detail::hash_combine(
        result, hash<c4c::backend::bir::Generation>{}(id.generation));
  }
};

template <>
struct hash<c4c::backend::bir::ValueId> {
  size_t operator()(c4c::backend::bir::ValueId id) const noexcept {
    auto result = c4c::backend::bir::detail::hash_function_id(id.owner);
    result = c4c::backend::bir::detail::hash_combine(
        result, hash<std::underlying_type_t<c4c::backend::bir::ValueKind>>{}(
                    static_cast<std::underlying_type_t<c4c::backend::bir::ValueKind>>(
                        id.kind)));
    result = c4c::backend::bir::detail::hash_combine(
        result, hash<c4c::backend::bir::SlotIndex>{}(id.slot));
    return c4c::backend::bir::detail::hash_combine(
        result, hash<c4c::backend::bir::Generation>{}(id.generation));
  }
};

template <>
struct hash<c4c::backend::bir::SourceValueId> {
  size_t operator()(c4c::backend::bir::SourceValueId id) const noexcept {
    return c4c::backend::bir::detail::hash_combine(
        c4c::backend::bir::detail::hash_function_id(id.owner),
        hash<std::uint32_t>{}(id.value));
  }
};

}  // namespace std
