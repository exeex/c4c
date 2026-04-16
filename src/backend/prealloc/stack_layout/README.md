# stack_layout/

Stack frame layout for prealloc BIR: determines which function-local objects
need frame slots and emits a provisional `PreparedStackLayout`.

## Why this exists

The prealloc pipeline needs a stable stack-layout phase between `legalize` and
the later `liveness` / `regalloc` summaries. This phase translates semantic BIR
function state into:

- `PreparedStackObject`: stack-relevant objects derived from BIR local slots and
  selected parameters
- `PreparedFrameSlot`: provisional frame-slot assignments for those objects
- aggregate frame metrics such as `frame_size_bytes` and `frame_alignment_bytes`

The current C++ implementation is a first-pass bridge. It compiles and produces
consistent `PreparedStackLayout` output, but it does not yet port the full Rust
three-tier allocation model.

## Current Inputs

`BirPreAlloc::run_stack_layout()` currently reads:

- `PreparedBirModule::module`
- each `bir::Function`
- each function's `params`, `local_slots`, `blocks`, and `CallInst` metadata
- the target-independent type/size/alignment information already present in BIR

Declarations are skipped.

## Current Outputs

For each non-declaration function, the C++ path currently:

1. Collects stack objects from `bir::LocalSlot` plus `byval` / `sret` params.
2. Applies conservative hints for alloca coalescing, copy coalescing, inline asm,
   and home-slot requirements.
3. Emits a fixed-location tier for permanent home slots, then packs the
   remaining reorderable home slots into dedicated aligned frame slots,
   including bounded gap filling when a smaller reorderable slot can consume
   padding left by the fixed tier.
4. Merges all per-function results into `PreparedStackLayout`.

This data is written into:

- `PreparedStackLayout::objects`
- `PreparedStackLayout::frame_slots`
- `PreparedStackLayout::frame_size_bytes`
- `PreparedStackLayout::frame_alignment_bytes`
- `PrepareNote` entries for per-function summaries

## Module layout

| File | Purpose |
|------|---------|
| `support.hpp` | Shared private helpers and function declarations for the C++ port |
| `analysis.cpp` | Collects `PreparedStackObject` records from BIR functions |
| `alloca_coalescing.cpp` | Applies conservative address-exposed and home-slot hints |
| `copy_coalescing.cpp` | Applies provisional copy-coalescing metadata hints |
| `inline_asm.cpp` | Summarizes inline-asm presence and side-effect metadata |
| `regalloc_helpers.cpp` | Applies conservative home-slot hints informed by local-slot metadata |
| `slot_assignment.cpp` | Performs aligned sequential frame-slot assignment |
| `*.rs` | Reference-only Rust source retained to guide future semantic porting |
| `../stack_layout.cpp` | Orchestrates the phase and writes `PreparedStackLayout` |

## Current Design Constraints

- The current bridge is intentionally conservative. It prefers over-allocating
  stack slots to missing required home slots.
- `PreparedFrameSlot` is still a dedicated object-to-slot record. The active
  contract does not represent Rust Tier 2 / Tier 3 shared-slot reuse, deferred
  slot finalization, or value-owned slot aliases.
- Frame-slot offsets are currently assigned as dedicated home slots per
  function, not by value liveness or interval packing.
- The remaining Rust `slot_assignment.rs` reuse logic depends on value-level
  interval data that belongs with Step 3 `liveness`, not on more
  `PreparedStackObject` heuristics.
- The Rust files are not part of the active build; they remain as design and
  porting references only.

## Planned Direction

The intended end state is still the richer stack-layout model described by the
Rust reference sources:

- tiered allocation
- alloca escape analysis
- copy aliasing and immediately-consumed optimization
- liveness-packed multi-block slot sharing
- tighter interaction with later liveness and regalloc phases

That behavior has not been fully ported yet, so this README should be read as a
description of the active C++ contract first, not of the legacy Rust algorithm.
