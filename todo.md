Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 stack-layout activation packet by threading
`CallInst.sret_storage_name` through `alloca_coalescing.cpp` as a real
`call_result_sret` prepared-object source, not just a generic direct-access
home-slot requirement. The active `backend_prepare_stack_layout` test now
proves the non-address-exposed sret-storage local slot keeps
`requires_home_slot == true`, publishes `permanent_home_slot == true`, and
lands in the fixed-location frame-slot tier at offset `0`, making the active
C++ prepared/frame-slot contract materially closer to the Rust-style permanent
home-slot split for direct call-result storage.

## Suggested Next

Continue Step 2 by auditing the remaining non-address-exposed direct-access
roots and routing any other permanent-home cases through explicit prepared
source kinds or contract bits instead of leaving them as generic
`requires_home_slot` locals.

## Watchouts

- `slot_assignment.cpp` now keys the fixed-location tier off
  `PreparedStackObject.permanent_home_slot`, not `address_exposed`
- the current packet now covers the `call_result_sret` / `sret_storage_name`
  direct-access path by setting `PreparedStackObject.source_kind` and
  `permanent_home_slot` together inside `alloca_coalescing.cpp`
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
after threading the active `call_result_sret` / permanent-home-slot contract
into stack layout and extending the focused activation coverage. Canonical
proof log: `test_after.log`.
