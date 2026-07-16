Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 local aggregate memory identity repair for the direct
local-slot chain feeding intrinsic fill/copy and local pointer reconstruction.

`LocalSlotAddress` and `PointerAddress` now carry the existing
`LocalAggregateSlots::type_ref` metadata when an address is derived from a
metadata-bearing aggregate carrier. `memory/intrinsics.cpp` now resolves local
aggregate memset/memcpy layouts through structured `LirTypeRef` lookup when
that carrier is present, and fails closed on structured misses instead of
recovering through rendered type spelling. `memory/local_slots.cpp` now uses
the same structured local aggregate layout path when materializing HFA
aggregate stores, publishing local aggregate pointer addresses, and rebuilding
aggregate views from loaded local pointer slot state.

## Suggested Next

Suggested Next: keep Step 4 active and run a bounded provenance/central-layout
packet that either threads the new `PointerAddress::type_ref` into scalar
subobject provenance classification or records the exact no-id boundaries that
still cannot receive structured layout context. Include the central
`types.cpp` raw `TypeDeclMap` fallback in that same audit only where it is
directly exercised by the remaining provenance helper.

## Watchouts

- `memory/provenance.cpp` still classifies scalar subobject addressability with
  only rendered type text and `TypeDeclMap`; its current helper signatures do
  not receive `BackendStructuredLayoutTable`, so fully consuming
  `PointerAddress::type_ref` there is a separate plumbing packet.
- Raw text fallbacks remain legitimate for legacy/no-id local address state
  where `type_ref` is absent. Do not relabel metadata-bearing misses as no-id.
- Do not reopen already classified call ABI, global, aggregate-parameter,
  memory/addressing, or local GEP comments unless this local memory repair
  exposes a direct contradiction.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
