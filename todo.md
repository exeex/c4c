Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 stack-layout activation packet by adding an explicit
`PreparedStackObject.permanent_home_slot` contract bit and routing
`slot_assignment.cpp` through it instead of inferring the permanent tier from
address exposure alone. The active `backend_prepare_stack_layout` test now
proves a non-address-exposed `byval_copy` local slot stays
`permanent_home_slot == true`, takes the fixed-location tier, and anchors frame
offset `0` ahead of a wider reorderable home slot, making the active C++
prepared/frame-slot contract materially closer to Rust's permanent-vs-
reorderable split.

## Suggested Next

Continue Step 2 by threading the remaining non-address-exposed permanent-home
sources through the prepared contract, especially direct-access roots such as
call-result / sret storage that currently keep `requires_home_slot` without yet
advertising `permanent_home_slot`.

## Watchouts

- `slot_assignment.cpp` now keys the fixed-location tier off
  `PreparedStackObject.permanent_home_slot`, not `address_exposed`
- the current packet covers address-exposed, phi/byval/param, and non-elided
  lowering-scratch permanence, but direct-access non-address-exposed roots such
  as the existing sret-storage path still need an explicit permanence signal
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
after adding the explicit permanent-home-slot contract and focused activation
coverage. Canonical proof log: `test_after.log`.
