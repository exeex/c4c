# `dead_code.cpp` extraction

## Purpose and current responsibility

This file hosts two local dead-code cleanup passes inside the x86 peephole pipeline:

- remove register-to-register `movq` instructions whose destination is overwritten before any observable use
- remove `%rbp` stack-slot stores that are overwritten before any observed read

It does not own line classification, register parsing, alias analysis, or pipeline scheduling. It consumes precomputed `LineInfo` facts plus shared helper parsing and mutates matching instructions into `Nop`.

## Important APIs and contract surfaces

Primary exported surface from this file:

```cpp
bool eliminate_dead_reg_moves(const LineStore* store, LineInfo* infos);
bool eliminate_dead_stores(const LineStore* store, LineInfo* infos);
```

Consumed contract from the directory index surface:

```cpp
struct LineInfo {
  LineKind kind;
  std::uint16_t trim_start;
  RegId dest_reg;
  bool has_indirect_mem;
  std::int32_t rbp_offset;
  std::uint16_t reg_refs;
  bool is_nop() const;
  bool is_barrier() const;
};
```

Local rewrite action is intentionally destructive and metadata-only:

```cpp
void mark_nop(LineInfo& info);
```

`mark_nop` clears kind and several analysis fields, but it does not edit the original source line text. The wider peephole pipeline therefore depends on later stages treating `LineInfo::Nop` as authoritative.

## Dependency direction and hidden inputs

Direction is inward toward shared peephole infrastructure:

- reads assembly text from `LineStore`
- trusts `trim_start`, `kind`, `dest_reg`, `rbp_offset`, and `reg_refs` in `LineInfo`
- relies on helper parsing and hazard predicates from `passes/helpers.cpp`
- relies on `LineInfo::is_barrier()` to define control-flow and side-effect boundaries

Hidden inputs that materially shape behavior:

- fixed scan horizons: `24` lines for dead moves, `16` lines for dead stores
- hard-coded exclusion of register ids `4` and `5` (`%rsp` and `%rbp` families) from dead-move deletion
- stack-slot overlap is modeled as fixed-width 8-byte ranges
- textual `(%rbp)` search is used as a fallback read detector for `Other` and `Cmp` lines
- implicit register users immediately stop dead-move scanning instead of being modeled precisely

## Responsibility buckets

### 1. Local line normalization

`trimmed_line(...)` rebuilds the trimmed text view from `LineStore` plus `LineInfo::trim_start`. This file therefore depends on earlier trimming/classification remaining consistent with the stored raw line.

### 2. Dead register move elimination

The pass:

- accepts only parsed register-to-register `movq`
- rejects non-GP, `%rsp`, and `%rbp` destinations
- scans forward until barrier, implicit-register usage, explicit read, or overwrite
- treats overwrite-before-read as proof that the original move is dead
- preserves instructions that may read their destination as part of read-modify-write or compare behavior

This is a local liveness approximation, not full dataflow.

### 3. Dead stack store elimination

The pass:

- accepts only `StoreRbp` lines with a known `rbp_offset`
- scans forward for overlapping `LoadRbp`, overlapping `StoreRbp`, or a conservative textual `(%rbp)` use
- marks the earlier store dead only when an overlapping store arrives before any observed read

This is effectively a short-window stack-slot overwrite check.

### 4. Overlap and nop helpers

`ranges_overlap(...)` and `mark_nop(...)` are small local utilities, but they encode semantic policy:

- overlapping stack slots are treated as aliasing
- dead instructions are erased by metadata mutation, not line deletion

## Notable fast paths, compatibility paths, and overfit pressure

Fast paths:

- early skip for `Nop` and barrier lines
- no deeper parsing unless the candidate line already looks like a supported move or `%rbp` store
- short bounded windows keep runtime predictable

Legacy compatibility paths:

- `has_implicit_reg_usage(...)` is a broad escape hatch that aborts optimization around instructions with hard-to-model implicit operands
- textual `(%rbp)` detection compensates for incomplete structured classification of generic memory users
- compare and read-modify-write exceptions avoid deleting producers for instructions that both write and consume the same register family

Overfit pressure:

- fixed windows can miss real dead code outside the arbitrary horizon
- the `%rbp` textual search is shape-based and may both miss structured cases and retain too much code
- the stack-store logic assumes 8-byte accesses and does not model size-sensitive overlap
- excluding `%rsp` and `%rbp` by raw register id is practical but embeds calling-convention detail directly in the pass
- this file mixes semantic deadness decisions with string-pattern escape hatches inherited from incomplete upstream facts

## Rebuild ownership guidance

This file should own only local deadness decisions over already-classified instruction facts and the minimal rewrite operation that marks an instruction dead.

This file should not own raw-text pattern recovery, implicit-operand heuristics, stack alias policy beyond a clean abstract query, register-family special cases by magic ids, or broader peephole pipeline coordination in a rebuild.
