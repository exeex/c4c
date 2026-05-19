# LIR To BIR Global Pointer Aggregate Projection Todo

Status: Active
Source Idea Path: ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Focused Projection Failures

# Current Packet

## Just Finished

Lifecycle switch completed from umbrella inventory idea 295 to focused idea
298. No implementation files or canonical logs were modified.

## Suggested Next

Start Step 1 by inspecting the focused residual projection cases in the
accepted `test_before.log` backend-regex baseline: 352 selected, 291 passed,
61 failed.

## Watchouts

- Keep this owner limited to global scalar-array, pointer-derived, global
  aggregate, bootstrap/global aggregate, and possible `00216`
  pointer-parameter/flexible-array projection admission.
- Do not absorb machine-printer residuals, runtime nonzero/mismatch buckets,
  or standalone timeout `00220` without new lifecycle evidence.
- Do not touch expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, implementation files outside
  the focused code path, or canonical logs during lifecycle setup.
- Do not reopen idea 297 from failing counts alone.

## Proof

Lifecycle-only activation. No tests were run.
