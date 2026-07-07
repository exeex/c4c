Status: Active
Source Idea Path: ideas/open/581_rv64_ordinary_floating_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Backend Closure Readiness

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by running the supervisor-selected backend
validation subset after the focused ordinary floating-cast coverage,
implementation, and representative route proof.

Backend validation passed: `ctest` reported `100% tests passed, 0 tests failed
out of 346` after `cmake --build --preset default`.

Closure-readiness assessment:
- Focused backend coverage now covers ordinary retained F32/F64 width casts,
  including immediate-source `FPTrunc F64 -> F32`, immediate-source
  `FPExt F32 -> F64`, and the retained `UIToFP i32 -> F32` plus
  `FPExt F32 -> F64` chain.
- Step 4 route evidence shows `src/920618-1.c`,
  `src/ieee/pr67218.c`, and `src/pr23941.c` all advance past the old
  `unsupported_floating_cast` owner to downstream
  `unsupported_terminator_fragment`.
- F128, long-double, soft-float helper, scalar compare, and variadic helper
  rows remain outside this source idea and were not used as acceptance
  evidence.
- The downstream terminator owner is a separate follow-up candidate, not part
  of ordinary floating-cast lowering.

The source idea is ready for supervisor/plan-owner closure evaluation.

## Suggested Next

Supervisor should commit this `todo.md` validation slice, then ask the
plan-owner to evaluate closure for
`ideas/open/581_rv64_ordinary_floating_cast_lowering.md`.

## Watchouts

- Keep closure scoped to ordinary F32/F64 casts and the retained justified
  integer-to-FP chain.
- Do not widen this lane to the downstream
  `unsupported_terminator_fragment` owner.
- Do not use F128, long-double, soft-float helper, scalar compare, or variadic
  helper work as justification for this idea.
- Unrelated untracked
  `ideas/open/583_rv64_pointer_arithmetic_result_publication.md` remains
  untouched.

## Proof

Proof output is in `test_after.log`.

- `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`: pass.
- `ctest` result: `100% tests passed, 0 tests failed out of 346`.
- This backend subset is sufficient for closure-readiness evaluation because
  the code slice already has focused RV64 object-emission coverage and Step 4
  representative route evidence for all retained ordinary rows.
