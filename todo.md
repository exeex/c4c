Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the bounded Step 2 `.cpp` vs `.rs` audit for
`stack_layout/alloca_coalescing`: no new code change was warranted because the
active C++ helper already covers the repo's current `bir::Inst` pointer shapes
through direct local-slot uses, pointer-alias tracking, pointer-address
bridges, call/return/branch escape handling, and the host-framework
`CallInst::sret_storage_name` retention path. The remaining Rust-only
differences are instruction families that the current BIR does not model, so
this packet closed as a comparison result plus refreshed focused proof rather
than a semantic patch.

## Suggested Next

Continue Step 2 in `stack_layout` by comparing
`src/backend/prealloc/stack_layout/regalloc_helpers.cpp` against
`stack_layout/regalloc_helpers.rs`, then take the next bounded packet only if
that comparison exposes one active current-pipeline semantic gap in how
stack-layout objects keep or drop home-slot requirements.

## Watchouts

- the active `alloca_coalescing.cpp` route now has both focused proof and a
  bounded `.cpp` vs `.rs` audit for the current BIR surface, so do not reopen
  that helper unless a new active instruction or address form lands in BIR
- the remaining `alloca_coalescing.rs` differences are Rust-only instruction
  families outside today's `bir::Inst` model; adding speculative C++ handling
  for them here would be route drift, not Step 2 progress
- `CallInst::sret_storage_name` remains a bounded host-framework divergence
  from the Rust reference helper, so keep it framed as host adaptation during
  the eventual acceptance comparison
- `regalloc_helpers.cpp` still keeps most live local-slot objects conservative,
  so broader Rust-like alloca tiering is not active yet
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
successfully after the bounded `alloca_coalescing` parity audit; canonical
proof log: `test_after.log`. No broader checkpoint was run in this executor
packet.
