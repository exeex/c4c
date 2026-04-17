Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Started the first real Step 3/4 bridge by replacing the `run_regalloc()`
no-op with active liveness projection: the C++ prealloc route now publishes
per-function `PreparedRegallocValue` seed records, type-driven allocation
constraints, and live-interval interference edges derived from active liveness
data instead of relying on stack-object-only summaries or a stub phase.

Focused `backend_prepare_liveness` coverage now proves that the active route
reaches regalloc for the phi test function: `phi.v` keeps its value-level
interval and priority in `PreparedRegallocValue`, receives a general-register
constraint, and interferes with `sum` because their live intervals overlap,
while disjoint predecessors `left.v` and `right.v` remain non-interfering.

## Suggested Next

Make the next Step 4 slice consume these published regalloc seeds
semantically: teach `run_regalloc()` to make the first bounded allocation
decision from value intervals plus call-crossing/home-slot constraints, rather
than stopping at unallocated seed data.

## Watchouts

- exact `definition_point` / `use_points` now capture true local activity,
  while `live_interval` still represents the CFG-extended range after
  live-through propagation; downstream consumers must not collapse those into
  one concept
- the current regalloc bridge seeds general/float eligibility, priorities, and
  interval-overlap interference, but it still leaves every value unallocated;
  the next packet should consume these seeds to make at least one real
  allocation or stack-slot decision
- `PreparedAllocationConstraint.cannot_cross_call` still needs a clearer
  semantic contract before call-heavy tests rely on it for allocator behavior
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
after teaching `regalloc.cpp` to publish liveness-derived value seeds and
extending focused activation coverage to prove those seeds are active on the
phi path.
Canonical proof log: `test_after.log`.
