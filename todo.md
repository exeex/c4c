Status: Active
Source Idea Path: ideas/open/438_prepared_pointer_value_memory_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 3: added producer/prepared contract coverage for pointer-value
memory authority without editing RV64 target lowering.

Implemented contract surface:

- Added `prepare::prepared_pointer_value_memory_has_proven_authority` in
  `src/backend/prealloc/addressing.hpp`.
- The classifier accepts only pointer-value memory addresses with
  `pointer_value_name`, base-plus-offset permission, scalar-sized
  size/alignment facts, concrete non-bare provenance identity, layout authority
  stronger than unknown/opaque compatibility, complete object extent, matching
  requested range, and `range_verdict=ProvenInBounds`.

Focused coverage:

- Existing pointer-indirect producer fixtures in
  `tests/backend/bir/backend_prepare_stack_layout_test.cpp` now assert that
  current unknown-authority records remain non-target-consumable.
- Added an explicit coherent prepared pointer-value memory fixture that
  classifies target-consumable.
- Added fail-closed variants for `layout_authority=unknown`,
  `range_verdict=UnknownCompatible`, bare `PointerValue` identity, missing
  `pointer_value_name`, missing base-plus-offset permission, and
  out-of-bounds range.

Boundary: existing RV64 manually authored pointer-value tests remain
consumer-only proof. `930930-1` still needs producer output stronger than
`layout_authority=unknown` plus `range_verdict=unknown_compatible` before RV64
lowering should consume it.

Artifacts:

- `build/agent_state/438_step3_pointer_value_authority_coverage/summary.md`

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

Recommended packet:

- Re-probe or inspect the `930930-1` prepared/object evidence against the new
  authority classifier.
- Record whether the remaining blocker is still producer publication of
  proven pointer-value memory authority, or whether a separate follow-up idea
  is needed for opaque compatibility or target consumption.
- Do not select an RV64 lowering packet unless prepared records carry proven
  authority under the Step 3 classifier.

## Watchouts

- Do not infer pointer-value memory ownership, layout, range, or provenance in
  RV64 from raw pointer values, integer casts, filenames, function names,
  object labels, or dump shape.
- Keep `layout_authority=unknown` and `range_verdict=unknown_compatible`
  fail-closed until producer facts prove stronger authority.
- Do not fold store-source/global-memory publication, direct-global
  return/select-chain, or terminator/select publication work into this plan.
- Do not reopen pointer-cast movement from idea 429 or selected global
  object-data support from idea 433.
- Existing RV64 pointer-value tests are manually-authored consumer fixtures;
  they are not proof that `930930-1` has producer authority.
- Treat `OpaqueCompatibility` as a named producer-owned exception only after
  focused coverage exists; do not let it silently admit arbitrary
  `UnknownCompatible` pointer-value accesses.
- The new classifier deliberately keeps `OpaqueCompatibility` non-proven for
  this packet. A future opaque-compatibility policy must be separately named
  and tested.

## Proof

Step 3 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
