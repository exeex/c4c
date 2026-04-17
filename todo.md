Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the first bounded Step 4 allocation slice by teaching
`run_regalloc()` to consume the liveness-derived regalloc seeds
semantically: the active C++ route now performs deterministic target-aware
register assignment for eligible values, reuses registers when live intervals
expire, and falls back to real stack-slot/home-slot assignments instead of
leaving every value unallocated.

Focused `backend_prepare_liveness` coverage now proves both sides of that
activation: on the phi path, `left.v` and `right.v` reuse the same real
register across disjoint intervals while overlapping `sum` falls back to a
real stack slot; a second byval-param case proves a `requires_home_slot`
value stays on a real home slot instead of taking a register.

## Suggested Next

Extend Step 4 beyond the first bounded allocator by making the active route
consume call-crossing constraints with a distinct callee-saved path and by
growing the allocator from single-register seeded pools toward the retained
Rust intent for spillover and broader priority handling.

## Watchouts

- exact `definition_point` / `use_points` now capture true local activity,
  while `live_interval` still represents the CFG-extended range after
  live-through propagation; downstream consumers must not collapse those into
  one concept
- `PreparedAllocationConstraint.cannot_cross_call` still needs a clearer
  semantic contract before call-heavy tests rely on it for allocator behavior
- the current bounded allocator uses intentionally tiny target register pools
  to prove active interval-based assignment and stack/home-slot fallback; later
  Step 4 work still needs a broader callee-saved spillover story closer to the
  retained Rust allocator
- Rust Tier 2 / Tier 3 shared-slot reuse is still reference-only for the
  active C++ route: `PreparedFrameSlot` remains a dedicated object-owned slot
  record, so do not fake value-level reuse with object names, source kinds, or
  cross-object slot alias shortcuts before a later packet consumes the new
  value-level contract semantically
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran the delegated proof command successfully:
`cmake --build --preset default --target c4c_backend -j4 && ctest --test-dir
build -j --output-on-failure -R ^backend_prepare_liveness$ >
test_after.log 2>&1`
after teaching `run_regalloc()` to make real register/stack decisions from the
liveness-derived seeds and extending focused activation coverage to prove both
register reuse and home-slot fallback.
Canonical proof log: `test_after.log`.
