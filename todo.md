Status: Active
Source Idea Path: ideas/open/110_call_planning_frame_address_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Replace Or Narrow Name-Derived Call-Planning Authority

# Current Packet

## Just Finished

Completed Step 3 producer repair while keeping ordinary name-derived
call-planning fallback removal in `src/backend/prealloc/call_plans.cpp`.

Producer evidence:

- Removed the duplicate binary-lhs frame-slot materialization in
  `append_address_materializations()`. The old path emitted the same
  `PreparedAddressMaterializationKind::FrameSlot` fact twice at one instruction,
  causing `find_latest_frame_slot_materialization()` to fail closed on
  same-instruction ambiguity after the fallback was removed.
- Added binary-result frame-slot materialization for pointer add/sub with an
  immediate offset over an already-published frame-address operand. Direct and
  derived local aggregate pointer results such as `local.aggregate =
  local.aggregate.0 + 4` now have explicit `FrameSlot` facts before call
  planning consumes them.
- Extended `apply_frame_address_publication_hints()` so pointer uses of an
  aggregate slice-family name publish the zero-offset slice as that family's
  explicit frame-address value. This repaired the byval-over-16 address path
  that must pass the aggregate address instead of loading a preserved pointer
  spill.
- Kept only a bounded no-addressing compatibility path in the `FrameSlot`
  source-selection branch for manually assembled `populate_call_plans()`
  fixtures with no `PreparedAddressingFunction`. Normal prepared pipelines
  still require explicit address materialization facts.

## Suggested Next

Proceed to the next coherent packet by tightening contracts around the retained
no-addressing compatibility path and the direct/derived producer routes, without
expanding name-derived fallback back into ordinary prepared call planning.

## Watchouts

- The compatibility branch in `select_prepared_call_argument_source()` is
  intentionally gated on `addressing == nullptr`; do not let it become ordinary
  supported authority for prepared pipelines.
- The binary-result producer depends on the base operand already having
  frame-address publication authority; it does not infer stack-object ownership
  from arbitrary result names.
- The aggregate family publication path maps only the zero-offset slice to the
  family address value.

## Proof

Delegated proof command:
`bash -lc 'git diff --check && cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`.

Result: passed; `test_after.log` is the canonical proof log. The build
completed and the backend CTest subset passed 179 of 179 tests.
