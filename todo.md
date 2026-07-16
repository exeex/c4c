Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 4 owner-boundary conversion for local aggregate slot state.

`LocalAggregateSlots` now carries optional structured aggregate type identity
as `LirTypeRef`. The aggregate PHI declaration path threads
`PhiLoweringPlan::boundary_type_ref` into the generated local aggregate slots,
and `collect_sorted_leaf_slots(...)` now prefers
`lookup_backend_aggregate_type_ref_layout_result(...)` for metadata-bearing
slots. The retained rendered-text layout fallback is documented as legacy/no-id
compatibility for slots whose construction site still lacks structured type
metadata.

## Suggested Next

Suggested Next: choose one remaining local aggregate slot construction path
that already has structured type metadata available, thread `LirTypeRef` into
the slot state there, and keep text-only callers on the explicit no-id fallback.

## Watchouts

- `module.cpp` was touched only for the direct aggregate PHI slot declaration
  site, so the structured ref captured in `cfg.cpp` reaches
  `LocalAggregateSlots`.
- Most `declare_local_aggregate_slots(...)` callers still create text-only
  slot state by design for this packet. Do not treat the new optional field as
  a reason to migrate every memory path in one slice.
- Metadata-bearing local aggregate slots now fail closed in the leaf-slot
  consumer if the structured layout lookup cannot resolve; absent/no-id slots
  continue through the documented rendered-text fallback.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.

Supplemental PHI producer/verifier smoke also passed:
`ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|verify_tests_)'`
reported 12/12 passing after the fresh backend proof.
