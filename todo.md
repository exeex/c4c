Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the Step 2 `regalloc_helpers.cpp` metadata cleanup packet by removing
the last `source_kind`-based home-slot fallback from `apply_regalloc_hints()`.
Regalloc-hint application now preserves the prepared object contract it is
handed and only widens generic locals from real slot evidence such as
`is_address_taken`, while the focused `backend_prepare_stack_layout` test now
pins that helper behavior directly for generic `lowering_scratch`,
`byval_copy`, `phi`, and addressed local-slot objects.

## Suggested Next

Continue Step 2 by auditing the remaining explicit permanent-home producers,
especially `call_result_sret` and parameter-owned stack objects, and add
focused coverage proving they no longer depend on downstream `source_kind`
re-derivation after `apply_regalloc_hints()`.

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
build -j --output-on-failure -R ^backend_prepare_stack_layout >
test_after.log 2>&1`
after removing `apply_regalloc_hints()` source-kind home-slot fallbacks and
adding direct helper coverage for generic vs. explicit prepared local-slot
contracts. Canonical proof log: `test_after.log`.
