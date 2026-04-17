Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Finished the remaining per-phase Step 5 comparison coverage by adding
`src/backend/prealloc/stack_layout_comparison.md`, an explicit C++ vs Rust
comparison note for the active `stack_layout.cpp` plus `stack_layout/*.cpp`
route. The note maps the live prepared stack-layout pipeline to the retained
Rust module tree, records the current host-framework adaptations honestly, and
calls out the still-reference-only Rust behavior around full three-tier slot
allocation, dead-param elision, and inline-asm/regalloc helper scope instead
of leaving those gaps implicit.

## Suggested Next

Do not reopen implementation scope by inertia now that all three Step 5
comparison notes exist. The next coherent move is a supervisor-owned broader
backend checkpoint plus an acceptance readback across
`stack_layout_comparison.md`, `liveness_comparison.md`, and
`regalloc_comparison.md` to decide whether the active plan is ready for
closure or needs one more bounded parity packet.

## Watchouts

- keep the completed Step 5 comparison set honest about current bounded gaps:
  stack layout still stops at the prepared contract instead of porting Rust's
  full codegen-time three-tier allocator, and the C++ route still lacks
  Rust-side dead-param elision and explicit asm-clobber register filtering in
  stack-layout helpers
- the active stack-layout contract is object- and frame-metadata oriented,
  while `liveness` and `regalloc` stay value oriented; any final acceptance
  pass should preserve those phase boundaries instead of collapsing them into a
  single raw-IR narrative
- keep `.rs` files as references until the final comparison pass is complete;
  Step 5 comparison coverage now exists, but that is still not permission to
  delete the retained Rust notes before the supervisor finishes acceptance
- this packet only refreshed the narrow `backend_prepare_stack_layout` proof,
  so any acceptance-quality Step 5 milestone still needs supervisor-owned
  broader backend checkpointing before closure

## Proof

Focused Step 5 stack-layout proof passed:
`cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

The packet proof log is `test_after.log`. This slice only added the explicit
stack-layout comparison artifact plus the canonical todo update; it did not
widen validation beyond the delegated narrow stack-layout subset.
