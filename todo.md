Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 permanent-home producer audit by proving
`analysis.cpp` already emits explicit prepared contracts for `byval` and
`sret` params and that the active stack-layout path preserves those contracts
without any downstream `source_kind` re-derivation. The focused
`backend_prepare_stack_layout` test now covers parameter-object collection,
post-`apply_regalloc_hints()` preservation, and full frame-slot activation for
`byval_param` plus `sret_param`, while a plain parameter remains outside the
prepared stack-object set.

## Suggested Next

Continue Step 2 by comparing the remaining C++ stack-layout fixed-tier behavior
against the retained Rust `slot_assignment.rs` path, especially around
parameter/alloca ordering and any dead-parameter elision rules that are still
missing from the active C++ route.

## Watchouts

- `slot_assignment.cpp` now keys the fixed-location tier off
  `PreparedStackObject.permanent_home_slot`, not `address_exposed`
- `byval_copy`, `phi`, and `call_result_sret` are now the explicit
  non-address-exposed permanent-home source kinds covered by focused activation
  tests
- `analysis.cpp` no longer seeds generic local slots, including
  `lowering_scratch`, with `requires_home_slot = true`; later passes must add
  home-slot requirements from explicit contracts or observed use/address data
- `regalloc_helpers.cpp` now trusts prepared object flags instead of
  recomputing home-slot metadata from `source_kind`; generic non-copy
  `LoweringScratch` locals only widen from real slot evidence such as address
  exposure
- generic non-copy `LoweringScratch` locals no longer inherit
  `requires_home_slot` or `permanent_home_slot` from storage/source kind
  alone; live scratch slots still keep home slots when real direct slot
  accesses or address exposure require them
- unused generic `LoweringScratch` locals now elide like other dead stack
  objects even though they still publish `source_kind == lowering_scratch`
- `call_result_sret` still gets its permanent-home contract upstream in
  `alloca_coalescing.cpp`; keep checking that explicit producer path instead of
  reintroducing downstream `source_kind` fallbacks
- `byval_param` and `sret_param` remain explicit producers in
  `analysis.cpp`; `apply_regalloc_hints()` leaves them unchanged because they
  are not rediscovered from `bir::LocalSlot`
- plain params still do not produce `PreparedStackObject`s; any future
  parameter-slot work must come from a real contract change, not from test-only
  expectation widening
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
`cmake --build --preset default --target c4c_backend -j4 && ctest --test-dir
build -j --output-on-failure -R ^backend_prepare_stack_layout$ >
test_after.log 2>&1`
after adding focused byval/sret parameter coverage for object collection,
regalloc-hint preservation, and active frame-slot assignment. Canonical proof
log: `test_after.log`.
