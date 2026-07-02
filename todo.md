Status: Active
Source Idea Path: ideas/open/425_scalar_fpr_salvage_from_try_gcc_torture.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Step 5 - Validate And Handoff recorded validation readiness after Step 4
published the scalar FPR salvage follow-up idea. Active idea 425 remains open;
this packet does not close the plan.

Published follow-up idea:

- `ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md`
- Boundary: residual scalar F32/F64 cast object lowering for the three fresh
  `unsupported_floating_cast` rows: `src/cvt-1.c`, `src/920618-1.c`, and
  `src/pr66233.c`.
- The idea carries the Step 3 scope, out-of-scope list, acceptance criteria,
  dependencies, and reviewer reject signals. It keeps older scalar residual
  rows as supporting context only and quarantines F128/helper ABI/local-memory,
  aggregate/byval, stack-frame, and `conversion.c`-only work.

Validation result:

- `cmake --build --preset default` completed with exit status `0`.
- `ctest --test-dir build -j --output-on-failure` completed with exit status
  `8` because the full suite still has the known baseline
  `string_authority_guard` failure.
- `test_before.log` and `test_after.log` both report exactly one failing test:
  `1 - string_authority_guard (Failed)`. The diagnostic is the same known
  `src/backend/bir/lir_to_bir/module.cpp:51:
  SameModuleFormalPointerProvenanceMap` string-authority classification issue.
  No new full-suite failures were introduced by the Step 4 publication.
- `git diff --check -- todo.md ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md`
  completed with exit status `0`.

Residual risks:

- The follow-up idea still needs row inspection before implementation. If any
  of the three fresh rows lacks explicit prepared scalar-cast facts, the next
  lifecycle should split a prepared/BIR producer fact idea rather than widening
  RV64 object emission.
- Full-suite validation remains non-green due to the pre-existing
  `string_authority_guard` failure, but the failure is stable against the
  supervisor-provided `test_before.log`.
- The usual quarantine boundaries still apply: no F128/helper ABI/local-memory,
  aggregate/byval, stack-frame, broad branch/select/local-store, direct
  cherry-pick, expectation rewrite, or `conversion.c`-only route should be
  treated as scalar FPR salvage.

Executor handoff assessment: active idea 425 appears ready for plan-owner
closure or lifecycle transition. The accepted follow-up idea exists, rejected
and quarantined candidates are recorded, formatting proof is clean, and
full-suite comparison shows no new failures beyond the known baseline
`string_authority_guard` issue.

## Suggested Next

Hand off to the plan owner to decide closure for active idea 425. Do not
activate idea 517 until the active 425 lifecycle state is closed or otherwise
transitioned by the plan owner.

## Watchouts

`test_after.log` intentionally preserves the nonzero CTest result. Treat it as
stable relative to `test_before.log`, not all-green. The only observed full
suite failure is the known `string_authority_guard` baseline failure.

## Proof

Proof is captured in `test_after.log`:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure`
- `git diff --check -- todo.md ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md`
