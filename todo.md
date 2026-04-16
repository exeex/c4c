Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 stack-layout activation packet by removing the last
generic `LoweringScratch -> permanent_home_slot` widening from
`regalloc_helpers.cpp`. Generic non-copy scratch slots now stay reorderable
unless an explicit prepared contract marks them permanent, and the active
`backend_prepare_stack_layout` test now proves that a non-copy
`lowering_scratch` object keeps `requires_home_slot == true`, publishes
`permanent_home_slot == false`, stays out of the fixed-location tier, and packs
after a wider reorderable slot while the explicit permanent-home source kinds
remain covered separately.

## Suggested Next

Continue Step 2 by auditing the remaining generic
`LoweringScratch -> requires_home_slot` path and decide whether the active
non-copy scratch cases should keep that behavior or move to a more explicit
prepared contract that better matches the retained Rust phase split.

## Watchouts

- `slot_assignment.cpp` now keys the fixed-location tier off
  `PreparedStackObject.permanent_home_slot`, not `address_exposed`
- `byval_copy`, `phi`, and `call_result_sret` are now the explicit
  non-address-exposed permanent-home source kinds covered by focused activation
  tests
- `regalloc_helpers.cpp` no longer widens generic non-copy `LoweringScratch`
  locals into `permanent_home_slot`; only explicit prepared source kinds and
  address exposure should enter the fixed-location tier
- generic non-copy `LoweringScratch` locals still inherit a home-slot
  requirement from raw slot metadata; that is now the main remaining generic
  scratch path inside Step 2
- reorderable home-slot packing is still conservative size/alignment sorting;
  Rust-like shared-slot reuse is not active yet
- do not fake shared-slot reuse or permanence widening with source-name
  heuristics; the next packet should come from real prepared-contract inputs
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran the delegated proof command successfully:
`cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
after removing the generic `LoweringScratch` permanence widening and extending
the focused activation coverage for reorderable scratch behavior. Canonical
proof log: `test_after.log`.
