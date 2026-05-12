Status: Active
Source Idea Path: ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate And Prepare Completion Evidence

# Current Packet

## Just Finished

Completed Step 5 by recording final validation and completion evidence for the
selected dynamic global scalar-array `LinkNameId` route.

Implementation summary:
- `DynamicGlobalAggregateArrayAccess` and `DynamicGlobalScalarArrayAccess` now
  carry structured `LinkNameId` provenance for dynamic global scalar-array
  materialization.
- GEP-derived projections preserve that structured global id through the
  selected route instead of relying on final raw spelling.
- Dynamic global scalar-array load materialization resolves and validates the
  carried id against the global type/link-name tables, then emits
  `bir::LoadGlobalInst` with the structured `global_name_id`.
- Raw/no-id compatibility remains available only at the explicit
  `kInvalidLinkName` boundary, where existing spelling-based lookup is retained
  for legacy/no-metadata inputs.

Focused evidence in `tests/backend/backend_lir_to_bir_notes_test.cpp`:
- `expect_dynamic_global_scalar_array_loads_carry_link_name_id()` proves
  matching structured success despite drifted display spelling: the fixture
  renames the LIR global display name to `drifted_cases_display`, keeps the
  structured `LinkNameId` spelling as `cases`, and requires each emitted
  `bir::LoadGlobalInst` to carry `global_name == "cases"` plus the original
  `cases_id`.
- `expect_dynamic_global_scalar_array_loads_reject_missing_link_name_spelling()`
  proves the metadata-rich route fails closed when the carried `LinkNameId`
  cannot resolve to spelling text: the fixture keeps a `LinkNameId` on the
  global, detaches the link-name text table, and requires lowering to reject
  instead of falling back to raw final spelling.
- `expect_dynamic_global_scalar_array_loads_keep_no_id_compatibility()` proves
  raw/no-id compatibility remains separated: the fixture has no structured
  global id metadata, still lowers successfully, keeps `global_name == "cases"`,
  and requires `LoadGlobalInst::global_name_id == kInvalidLinkName`.

Full-suite regression evidence:
- Supervisor ran
  `cp test_baseline.log test_before.log && cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`.
- Supervisor then ran
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
- Result: PASS, before `3137/3137`, after `3137/3137`, with no new failures.

Idea 194 is ready for plan-owner closure review.

## Suggested Next

Ask the plan owner to review whether idea 194 should close, deactivate, or
split now that the runbook evidence is complete.

## Watchouts

- Do not re-prove only the addressed-global pointer route already closed by
  idea 187.
- Do not treat local route names, local slots, SSA temporaries, or block labels
  as semantic global symbols.
- Raw/no-id fallback remains available only when the selected access carries
  `kInvalidLinkName`; that fallback is compatibility lookup, not semantic
  global identity.
- `LoadGlobalInst::global_name_id` is the downstream BIR identity carrier for
  the selected route; no additional downstream handoff was needed.

## Proof

Final evidence-recording packet only; no implementation files, tests, or proof
logs were changed in this executor slice.

Supervisor-supplied proof:
`cp test_baseline.log test_before.log && cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`
followed by
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.

Result: PASS, before `3137/3137`, after `3137/3137`, no new failures.
Proof log path: `test_after.log`.
