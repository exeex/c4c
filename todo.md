Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the next Step 2 stack-layout activation packet by making the
remaining explicit non-address-exposed permanent-home local-slot roots follow
their prepared source kinds instead of raw slot metadata in
`regalloc_helpers.cpp`. `analysis.cpp` now derives `permanent_home_slot` for
`byval_copy` and `phi` objects from their published `source_kind`, and the
active `backend_prepare_stack_layout` test now proves both that
`byval_copy` keeps its explicit prepared source kind and that a
non-address-exposed phi-observed local slot publishes `source_kind == "phi"`,
keeps `requires_home_slot == true`, publishes `permanent_home_slot == true`,
and lands in the fixed-location frame-slot tier at offset `0`.

## Suggested Next

Continue Step 2 by auditing the still-generic `LoweringScratch` permanence
path and decide whether the active non-copy scratch cases need an explicit
prepared source kind or a separate prepared contract bit instead of relying on
raw local-slot metadata in `regalloc_helpers.cpp`.

## Watchouts

- `slot_assignment.cpp` now keys the fixed-location tier off
  `PreparedStackObject.permanent_home_slot`, not `address_exposed`
- `byval_copy`, `phi`, and `call_result_sret` are now the explicit
  non-address-exposed permanent-home source kinds covered by focused activation
  tests
- `regalloc_helpers.cpp` still widens non-copy `LoweringScratch` locals from
  raw slot metadata; that is the main remaining generic permanence path inside
  Step 2
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
after moving `byval_copy` / `phi` permanence to explicit prepared-source
semantics and extending the focused activation coverage. Canonical proof log:
`test_after.log`.
