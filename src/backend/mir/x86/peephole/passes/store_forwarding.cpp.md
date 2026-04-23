# `store_forwarding.cpp` extraction

## Purpose and current responsibility

This file implements `global_store_forwarding`, but the current behavior is narrower than the name suggests. It does not rewrite loads into direct register uses or perform general memory value propagation. It tracks a small map from `%rbp` stack-slot offsets to the register family most recently stored there, then removes a later `LoadRbp` only when that load targets the same register family already known to hold the slot value.

Its real responsibility today is:

- remember simple `%rbp` slot-to-register facts while walking one linear instruction stream
- clear those facts at conservative control-flow and side-effect boundaries
- mark redundant same-register stack reloads as `Nop`

It does not model aliases, merge control-flow facts, validate move sizes, or rewrite text in `LineStore`.

## Important APIs and contract surfaces

Primary exported surface from this file:

```cpp
bool global_store_forwarding(LineStore* store, LineInfo* infos);
```

Core consumed model from the directory index surface:

```cpp
struct LineInfo {
  LineKind kind;
  std::uint16_t trim_start;
  RegId dest_reg;
  bool has_indirect_mem;
  std::int32_t rbp_offset;
  std::uint16_t reg_refs;
  bool is_nop() const;
};
```

Local destructive rewrite surface:

```cpp
void mark_nop(LineInfo& info);
```

`mark_nop` changes only metadata. The original assembly line stays in `LineStore`, so the wider peephole pipeline must treat `LineKind::Nop` as the true deletion signal.

## Dependency direction and hidden inputs

Dependency direction is inward toward shared peephole classification:

- consumes `LineStore` only to rebuild a trimmed text view for fallback barrier checks
- trusts `LineInfo::kind`, `dest_reg`, `rbp_offset`, and `trim_start`
- relies on upstream classification to distinguish `StoreRbp`, `LoadRbp`, labels, jumps, calls, returns, directives, `Cmp`, and `Other`

Hidden inputs that materially shape behavior:

- `slot_map` is keyed only by `rbp_offset`; move width is ignored even though `StoreRbp` and `LoadRbp` carry size upstream
- load elimination requires exact `dest_reg` equality with the remembered store register, so the pass only removes reloads into the same family, not equivalent value copies into another register
- label handling depends on `prev_was_jump`: a label after a jump clears state, while an ordinary fallthrough label preserves it
- calls, returns, and directives always clear tracked slots
- generic `Other` and `Cmp` lines trigger a full clear when their trimmed text contains `(%rbp)`
- `has_indirect_mem` exists in `LineInfo` but is not used here; the pass falls back to raw string inspection instead

## Responsibility buckets

### 1. Metadata-only nop conversion

The file owns a tiny helper that erases a line by resetting its `LineInfo` fields. This is policy, not just mechanics: removal happens through analysis metadata, not source rewriting.

### 2. Linear slot tracking

The main loop keeps one forward-only map:

- `StoreRbp` records `rbp_offset -> dest_reg`
- `LoadRbp` checks whether the same slot is still mapped to the same register family
- a matching load is treated as redundant and becomes `Nop`

This is a straight-line local fact cache, not a full reaching-definitions analysis.

### 3. Barrier and invalidation policy

The pass decides when stack-slot facts are no longer trustworthy:

- labels after jumps clear state
- unconditional, indirect, and conditional jumps arm the next label as a clear point
- calls, returns, and directives clear immediately
- textual `%rbp` references in `Other` and `Cmp` lines clear immediately

### 4. Text fallback for unstructured memory users

`trimmed_line(...)` reconstructs the trimmed assembly slice from `LineStore` and `trim_start`, then performs a blunt `(%rbp)` substring search. This is a compatibility escape hatch for memory uses not represented as structured `LoadRbp` or `StoreRbp`.

## Notable fast paths, compatibility paths, and overfit pressure

Fast paths:

- skip `Nop` lines immediately
- handle recognized `StoreRbp` and `LoadRbp` without reparsing text
- clear the whole map instead of tracking partial kills

Compatibility paths:

- preserve behavior as a mechanical mirror of the legacy Rust-origin pass rather than a richer forwarding system
- use raw `(%rbp)` text detection to stay conservative around generic memory-touching instructions
- keep label handling simple by using `prev_was_jump` instead of explicit CFG construction

Overfit pressure:

- the pass name invites expansion into ad hoc “forward more cases” logic without first defining a real value-flow seam
- exact `dest_reg` matching makes the current optimization very narrow and testcase-shaped if extended incrementally
- full-map clears on any textual `(%rbp)` reference are safe but coarse; patching around missed wins with more string cases would push the file deeper into shape matching
- size-insensitive slot tracking can become incorrect if future edits try to treat partial-width stores and loads as interchangeable forwarded values

## Rebuild ownership

In a rebuild, this area should own only a clearly defined stack-slot value-forwarding analysis over normalized instruction facts, plus the minimal rewrite that removes a provably redundant reload.

It should not own raw assembly substring recovery, CFG reconstruction by local booleans, size-agnostic alias assumptions, or a growing collection of case-by-case forwarding exceptions.
