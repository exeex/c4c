Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the remaining Step 2 `analysis.cpp` convergence packet by removing
the last unconditional local-slot `requires_home_slot` seed during prepared
object collection. Generic `lowering_scratch` locals now start without a
home-slot requirement unless an explicit prepared contract already exists
(`is_address_taken`, `byval_copy`, or `phi`), and the focused
`backend_prepare_stack_layout` test now pins that source-level behavior
directly through `collect_function_stack_objects()` alongside the existing
runtime stack-layout coverage.

## Suggested Next

Continue Step 2 by checking whether any remaining prepared-object or regalloc
metadata paths still infer generic local-slot permanence from source-kind or
storage-kind alone instead of explicit prepared contracts plus real use data.

## Watchouts

- `slot_assignment.cpp` now keys the fixed-location tier off
  `PreparedStackObject.permanent_home_slot`, not `address_exposed`
- `byval_copy`, `phi`, and `call_result_sret` are now the explicit
  non-address-exposed permanent-home source kinds covered by focused activation
  tests
- `analysis.cpp` no longer seeds generic local slots, including
  `lowering_scratch`, with `requires_home_slot = true`; later passes must add
  home-slot requirements from explicit contracts or observed use/address data
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
`cmake --build --preset default --target c4c_backend -j4 && ctest --test-dir
build -j --output-on-failure -R ^backend_prepare_stack_layout >
test_after.log 2>&1`
after removing the last unconditional generic local-slot home-slot seed from
`analysis.cpp` and adding direct object-collection coverage for explicit vs.
generic `lowering_scratch` contracts. Canonical proof log: `test_after.log`.
