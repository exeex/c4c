Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 stack-layout activation packet by giving
`slot_assignment.cpp` an explicit fixed-location tier for address-exposed
stack objects before the existing reorderable home-slot packing tier. The
active `backend_prepare_stack_layout` test now proves that an address-exposed
root local slot keeps `fixed_location == true` and frame offset `0` even when
a wider non-address-exposed home slot would otherwise sort ahead, making the
active C++ frame-slot assignment materially closer to the Rust tiered model.

## Suggested Next

Continue Step 2 by comparing the remaining Rust `slot_assignment` phases
against the current C++ prepared-stack contract, focusing on whether the next
honest gap is additional permanent-vs-reorderable home-slot tiering or whether
real shared-slot reuse now requires new prepared contract inputs rather than
object-name heuristics.

## Watchouts

- `slot_assignment.cpp` now has two active home-slot tiers: fixed-location
  address-exposed objects first, then the existing size/alignment-packed
  reorderable homes
- fixed-location currently keys off `PreparedStackObject.address_exposed`, so
  any future widening of that tier needs an explicit Rust-reference comparison
  instead of ad hoc source-kind matching
- reorderable home-slot packing is still conservative size/alignment sorting;
  Rust-like shared-slot reuse is not active yet
- do not fake shared-slot reuse with source-name or testcase-shaped heuristics:
  the current `PreparedStackObject` contract still lacks block/lifetime data
  for honest reuse decisions
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran the delegated proof command successfully:
`cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
after adding the fixed-location frame-slot tier and focused activation
coverage. Canonical proof log: `test_after.log`.
