Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 provenance boundary repair for scalar subobject
addressability.

`memory/provenance.cpp` now threads `LocalSlotAddress::type_ref` and
`PointerAddress::type_ref` into the scalar subobject classifier. When a
StructNameId-bearing type ref reaches that boundary, the classifier uses the
structured backend layout table and rejects structured lookup misses instead
of falling back through rendered type spelling. The rendered
`TypeDeclMap` scalar-facts bridge remains only for provenance callers that
still have no metadata-bearing type ref, such as globals and legacy/no-id
local or pointer address state.

## Suggested Next

Suggested Next: supervisor review/commit for the Step 4 local aggregate
provenance slice, then decide whether any remaining no-id-only global or
legacy provenance paths need separate source intent before continuing the
universal model string escape-hatch deletion.

## Watchouts

- The central `types.cpp` raw `TypeDeclMap` fallback is still indirectly
  exercised by this helper only when no StructNameId-bearing type ref reaches
  provenance. Metadata-bearing local and pointer addresses bypass that route.
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
