Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 stack-layout activation packet by removing the
remaining generic `LoweringScratch -> requires_home_slot` widening from
`alloca_coalescing.cpp` and `regalloc_helpers.cpp`. Generic non-copy scratch
slots now need real direct slot use, address exposure, or another explicit
prepared contract to keep a dedicated home slot, and the active
`backend_prepare_stack_layout` test now proves both that a live
`lowering_scratch` slot still keeps `requires_home_slot == true` from real
loads/stores and that an unused generic scratch slot drops
`requires_home_slot`, skips frame-slot assignment, and shrinks the prepared
frame metrics.

## Suggested Next

Continue Step 2 by comparing the remaining `lowering_scratch` source-kind path
in `analysis.cpp` against the retained Rust stack-layout references and decide
whether any live scratch cases still need an explicit prepared contract beyond
the direct-access and address-exposure signals that now drive home-slot
retention.

## Watchouts

- `slot_assignment.cpp` now keys the fixed-location tier off
  `PreparedStackObject.permanent_home_slot`, not `address_exposed`
- `byval_copy`, `phi`, and `call_result_sret` are now the explicit
  non-address-exposed permanent-home source kinds covered by focused activation
  tests
- `regalloc_helpers.cpp` no longer widens generic non-copy `LoweringScratch`
  locals into `permanent_home_slot`; only explicit prepared source kinds and
  address exposure should enter the fixed-location tier
- generic non-copy `LoweringScratch` locals no longer inherit
  `requires_home_slot` from storage kind alone; live scratch slots still keep
  home slots when real direct slot accesses or address exposure require them
- unused generic `LoweringScratch` locals now elide like other dead stack
  objects even though they still publish `source_kind == lowering_scratch`
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
after removing the generic `LoweringScratch` home-slot widening and extending
the focused activation coverage for both live and dead scratch behavior.
Canonical proof log: `test_after.log`.
