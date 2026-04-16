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

Finished the follow-up audit of the remaining Rust `slot_assignment.rs`
behavior and recorded the bounded Step 2 divergence in the active C++
documentation: Rust Tier 2 / Tier 3 shared-slot reuse, deferred slot
finalization, and alias-style slot ownership still depend on value-level live
intervals that the current `PreparedStackObject` / `PreparedFrameSlot`
contract does not publish. The active route remains intentionally limited to
dedicated home-slot packing plus fixed-tier gap filling rather than extending
object-shaped heuristics that would impersonate Step 3 liveness.

## Suggested Next

Start Step 3 by making the public prepared-liveness contract publish the
minimum value-identity and interval data needed to replace the newly recorded
Step 2 slot-reuse gap with real Rust-like value ownership, rather than growing
more stack-layout-only object heuristics.

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
- Rust Tier 2 / Tier 3 shared-slot reuse is still reference-only for the
  active C++ route: `PreparedFrameSlot` remains a dedicated object-owned slot
  record, so do not fake value-level reuse with object names, source kinds, or
  cross-object slot alias shortcuts before Step 3 liveness publishes the
  required interval data
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

The follow-up Step 2 audit was documentation-only and reused the accepted
stack-layout proof baseline without changing executable behavior.
