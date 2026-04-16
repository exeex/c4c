Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 `slot_assignment` audit against the retained Rust
reference and landed the real same-contract packing gain the active C++
prepared object model can support today: reorderable home slots now fill
alignment holes left by earlier fixed-tier slots before a wider reorderable
slot is placed. Focused `backend_prepare_stack_layout` activation coverage now
proves that a fixed 4-byte root, a reorderable 8-byte local, and a reorderable
4-byte local shrink from a naive 24-byte frame to the hole-filled 16-byte
layout without introducing testcase-shaped sharing heuristics.

## Suggested Next

Continue Step 2 by deciding whether any remaining Rust `slot_assignment.rs`
behavior still maps onto the active `PreparedStackObject` /
`PreparedFrameSlot` contract, or whether the rest of the Rust slot-reuse logic
is now cleanly blocked on Step 3 liveness-style value intervals and should be
recorded as a bounded contract gap instead of extended with object-slot
heuristics.

## Watchouts

- `slot_assignment.cpp` still operates on `PreparedStackObject`s that each
  require distinct home-slot records; the new hole-filling path is dedicated
  slot packing, not Rust-style Tier 2 / Tier 3 shared-slot reuse
- the retained Rust shared-slot logic depends on value-level def/use or live
  interval structure that the active Step 2 stack-layout contract still does
  not publish; do not fake that with object-name or source-kind heuristics
- the fixed-tier ordering rule is unchanged: `byval_param` / `sret_param`
  permanent homes stay ahead of later fixed-location locals, and reorderable
  gap filling only happens after the full fixed tier has been emitted
- the new gap-filling helper only consumes space that would otherwise be pure
  alignment padding before the current highest-priority reorderable object; it
  does not reorder fixed slots or merge multiple objects into one slot
- `slot_assignment.cpp` now keys the fixed-location tier off
  `PreparedStackObject.permanent_home_slot`, not `address_exposed`
- `analysis.cpp` no longer seeds generic local slots, including
  `lowering_scratch`, with `requires_home_slot = true`; later passes must add
  home-slot requirements from explicit contracts or observed use/address data
- `regalloc_helpers.cpp` now trusts prepared object flags instead of
  recomputing home-slot metadata from `source_kind`; generic non-copy
  `LoweringScratch` locals only widen from real slot evidence such as address
  exposure
- Rust `dead_param_alloca` elision is still reference-only for the active C++
  route: current `bir::Param` inputs do not expose `param_alloca_values`,
  `ParamRef` destinations, or callee-saved reg assignment, so unused
  `byval_param` / `sret_param` objects intentionally remain materialized and
  keep fixed-tier slots for now
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran the delegated proof command successfully:
`cmake --build --preset default --target c4c_backend -j4 && ctest --test-dir
build -j --output-on-failure -R ^backend_prepare_stack_layout$ >
test_after.log 2>&1`
after teaching `slot_assignment.cpp` to fill fixed-tier alignment holes with
smaller reorderable home slots and adding focused activation coverage for that
live packing path.
Canonical proof log: `test_after.log`.
