Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 owner-boundary conversion for aggregate PHI planning in
`src/backend/bir/lir_to_bir/cfg.cpp` and
`src/backend/bir/lir_to_bir/lowering.hpp`.

`PhiLoweringPlan` now carries the PHI `boundary_value_type` as a structured
`LirTypeRef`. Aggregate PHI layout/alignment planning prefers
`lookup_backend_aggregate_type_ref_layout_result(...)` when that ref has a
`StructNameId`, and metadata-bearing refs now fail closed instead of falling
back through rendered `type_str`. The retained text fallback is documented as
legacy no-id compatibility for hand-built or inline aggregate PHI LIR only.

## Suggested Next

Suggested Next: choose the next single owner-boundary conversion from the
retained Step 4 fallback set, likely local aggregate slot state or byval copy
state, and thread a structured aggregate type ref or explicit no-id marker
before removing another rendered-text layout bridge.

## Watchouts

- `PhiLoweringPlan::type_text` is still intentionally retained for generated
  aggregate slot state; this packet only moved aggregate PHI layout/alignment
  selection off rendered text when structured PHI metadata exists.
- PHI refs without `StructNameId` still use the legacy text path for no-id or
  inline aggregate cases. Do not widen that fallback back to metadata-bearing
  refs.
- The exact backend subset selected by the supervisor ran cleanly, but it did
  not include frontend/verify PHI metadata producers.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supplemental PHI producer/verifier smoke also passed:
`ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|verify_tests_)'`
reported 12/12 passing after the fresh build.
