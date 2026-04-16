Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Started Step 3 by extending the public prepared-liveness contract with exact
per-value `definition_point` and ordered `use_points`, instead of publishing
only interval ranges. The active C++ liveness path now retains those exact
program points while it computes intervals, so downstream value-driven
consumers can distinguish true defs/uses from CFG live-through extension.

Focused `backend_prepare_liveness` coverage now proves that predecessor-edge
phi uses are exposed as real use points: `left.v` and `right.v` keep their
block-local definitions at points 2 and 4 while publishing the join-edge phi
uses at points 3 and 5, and the downstream `phi.v` / `sum` values likewise
publish their exact definition/use points on the active path.

## Suggested Next

Use the new exact def/use program-point contract to add the first real
value-driven Step 3/4 bridge: publish the minimum allocation-constraint or
interference seed data that `regalloc` will consume from liveness, without
falling back to stack-object identity or reintroducing stack-layout-only
heuristics.

## Watchouts

- exact `definition_point` / `use_points` now capture true local activity,
  while `live_interval` still represents the CFG-extended range after
  live-through propagation; downstream consumers must not collapse those into
  one concept
- the active contract still does not publish interference, spill weights, or
  allocation constraints; the next packet should add one real regalloc-facing
  bridge rather than inventing placeholder summaries
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
after teaching `liveness.cpp` to retain exact per-value definition/use program
points and extending focused activation coverage to prove predecessor-edge phi
uses are published on the active path.
Canonical proof log: `test_after.log`.
