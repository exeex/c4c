Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the bounded Step 2 `.cpp` vs `.rs` audit for
`stack_layout/regalloc_helpers`: no new code change was warranted because the
active C++ helper already covers the current stack-layout home-slot inputs from
`PreparedStackObject.source_kind`, `bir::LocalSlot` metadata, and inline-asm
side effects. The remaining Rust-only helper responsibilities are backend
register-filtering and clobber-merge paths that belong to later `regalloc`
activation rather than today's active stack-layout route, so this packet
closed as a parity result plus refreshed focused proof rather than a semantic
patch.

## Suggested Next

Continue Step 2 in `stack_layout` by comparing
`src/backend/prealloc/stack_layout/slot_assignment.cpp` against
`stack_layout/slot_assignment.rs`, then take the next bounded packet only if
that comparison exposes one active current-pipeline semantic gap in how the
current C++ bridge assigns or elides frame slots for prepared stack objects.

## Watchouts

- the active `regalloc_helpers.cpp` route already consumes every current
  stack-layout home-slot signal exposed by `analysis.cpp`, `legalize.cpp`, and
  `inline_asm.cpp`: `byval_param`, `sret_param`, `call_result_sret`,
  `is_address_taken`, `is_byval_copy`, `LoweringScratch`, and
  `phi_observation`
- the remaining `regalloc_helpers.rs` differences are Rust-side register
  allocation setup helpers, not missing C++ stack-layout home-slot behavior, so
  porting them in this packet would drift into Step 4
- `CallInst::sret_storage_name` remains a bounded host-framework divergence
  from the Rust reference surface and should stay documented as host adaptation
  during the eventual acceptance comparison
- `slot_assignment.cpp` is still a sequential conservative bridge, so Rust-like
  slot tiering and interval packing are not active yet
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
successfully after the bounded `regalloc_helpers` parity audit; canonical proof
log: `test_after.log`. No broader checkpoint was run in this executor packet.
