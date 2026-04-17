Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Kept Step 5 acceptance moving by adding
`src/backend/prealloc/regalloc_comparison.md`, an explicit C++ vs Rust
comparison note for the active `regalloc.cpp` route. The note maps the live
C++ allocator to `regalloc.rs`, records the prepared-pipeline adaptations that
are intentional in c4c, and calls out the remaining bounded divergences around
register-pool scope, float-register seeding, and Rust-only raw-IR eligibility
rules instead of leaving them implicit.

## Suggested Next

Finish the remaining explicit Step 5 comparison coverage instead of reopening
implementation scope. The cleanest next packet is the matching
`stack_layout.cpp` / `stack_layout/*.cpp` vs retained Rust comparison artifact,
using the new liveness and regalloc notes as the acceptance template and
calling out any host-framework deviations explicitly.

## Watchouts

- keep the new regalloc note honest about current gaps: the active C++ route
  still uses a deliberately small built-in general-register pool, does not yet
  seed float registers directly, and does not port every Rust-side
  raw-IR eligibility filter one-to-one
- the active regalloc contract is downstream of prepared liveness and stack
  layout; future Step 5 notes should preserve that phase boundary instead of
  collapsing back into raw object-identity or ad hoc ABI helper narratives
- keep `.rs` files as references until the final comparison pass is complete;
  Step 5 is explicit audit work, not a cleanup excuse
- this packet only refreshed the narrow `backend_prepare_liveness` proof, so
  any acceptance-quality Step 5 milestone still needs supervisor-owned broader
  backend checkpointing before closure

## Proof

Focused Step 5 regalloc proof passed:
`cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_liveness$' > test_after.log 2>&1`

The packet proof log is `test_after.log`. This slice only added the explicit
regalloc comparison artifact plus the canonical todo update; it did not widen
validation beyond the delegated narrow liveness subset.
