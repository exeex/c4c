Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Started Step 5 acceptance work for prealloc liveness by adding
`src/backend/prealloc/liveness_comparison.md`, an explicit C++ vs Rust
comparison note for the active `liveness.cpp` route. The note maps the live C++
pipeline to `liveness.rs`, records the prepared-contract adaptations that are
intentional in c4c, and calls out the remaining Rust-only extensions
(`setjmp`, GEP-base, and F128-source liveness growth) as bounded divergences
instead of leaving them implicit.

## Suggested Next

Keep Step 5 moving with the same bounded shape: compare one remaining active
phase against its Rust reference and record bounded divergences explicitly
instead of returning to helper-call ABI expansion. The cleanest next packet is
the matching `regalloc.cpp` vs `regalloc.rs` acceptance artifact, using the
new liveness note as the template for what counts as explicit comparison
coverage.

## Watchouts

- keep the new comparison note honest about current gaps: Rust still has
  `extend_gep_base_liveness(...)`, `extend_f128_source_liveness(...)`, and
  `extend_intervals_for_setjmp(...)`, and the active C++ route does not yet
  port those extensions
- exact `definition_point` / `use_points` are local activity records, while
  `live_interval` remains the CFG-extended range after live-through
  propagation; future comparison notes should keep those concepts distinct
- the active liveness contract is intentionally value-oriented even when a
  unique stack object is linked; do not let later Step 5 notes drift back into
  object-identity reasoning for `liveness` or `regalloc`
- keep `.rs` files as references until the final comparison pass is complete;
  Step 5 is explicit audit work, not a cleanup excuse
- this packet only refreshed the narrow `backend_prepare_liveness` proof, so
  any acceptance-quality milestone still needs supervisor-owned broader
  checkpointing

## Proof

Focused Step 5 liveness proof passed:
`cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_liveness$' > test_after.log 2>&1`

The packet proof log is `test_after.log`. This slice only added the explicit
liveness comparison artifact plus the canonical todo update; it did not widen
validation beyond the delegated narrow liveness subset.
