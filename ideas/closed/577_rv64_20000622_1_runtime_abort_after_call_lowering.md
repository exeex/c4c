# RV64 20000622-1 Runtime Abort After Call Lowering

Status: Closed
Type: Capability repair
Parent: `ideas/open/572_rv64_same_module_call_result_lowering.md`
Owning Layer: RV64 object-route runtime correctness after ordinary call lowering

## Goal

Classify and repair the `src/20000622-1.c` RV64 object-route runtime abort that
appears after ordinary same-module call/result lowering no longer fails through
the old generic `CallInst` fallback.

## Why This Exists

The 572 Step 5 rerun moved `src/20000622-1.c` past the old ordinary same-module
call fallback. The remaining first visible failure is now a runtime comparison
failure, not a call-lowering admission failure:

- Artifact summary:
  `build/agent_state/572_rv64_same_module_call_result_lowering/summary.tsv`
- Case log:
  `build/agent_state/572_rv64_same_module_call_result_lowering/src_20000622-1.c/object-route.log`
- Failure:
  `[RV64_BACKEND_RUNTIME_MISMATCH]`
- Reference exit:
  `clang_exit=0`
- c4c exit:
  `c4c_exit=Subprocess aborted`

This is separate from 572 because 572 is scoped to ordinary same-module
`CallInst` lowering, GPR argument materialization, and call-result publication.
The remaining abort needs fresh classification before it is assigned to a
lowering family.

## In Scope

- Reproduce the `src/20000622-1.c` RV64 object-route abort using the existing
  gcc_torture object-route runner.
- Capture enough focused evidence to identify the first semantic mismatch
  family after call lowering succeeds.
- Add focused backend or runtime coverage for the identified family before
  changing lowering behavior.
- Repair the underlying RV64 object-route runtime correctness issue for that
  family.
- Preserve the already-repaired same-module call emission and result
  publication behavior.

## Out Of Scope

- Reopening the ordinary same-module call fallback work unless new evidence
  shows the abort is still caused by call argument/result lowering.
- Expectation rewrites, unsupported-marker changes, allowlist edits, or runtime
  comparison contract changes.
- Filename-specific special cases for `src/20000622-1.c`.
- Broad rewrites of unrelated RV64 lowering families before the abort is
  classified.

## Acceptance Criteria

- The route identifies the first real post-call-lowering failure family for
  `src/20000622-1.c` with concrete artifact evidence.
- Focused tests or a focused runtime case prove the repaired family without
  depending on the exact `src/20000622-1.c` filename.
- The `src/20000622-1.c` object-route rerun no longer aborts for the same
  reason, or it advances to a newly classified later family recorded with
  artifacts.
- Existing 572 same-module call/result focused tests and representative
  evidence remain intact.

## Closure Notes

Closed after commit `866d0c314` repaired the first classified family: `baz`
now materializes `d = (long)c` from incoming `a2`, preserves the loaded local
in `s2` across `bar`, and passes `s2` as `foo` argument 0. The Step 4
representative rerun still aborts, but it advanced to a distinct later
`foo` logical/select condition family recorded under
`build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/`.

Follow-up idea:
`ideas/open/578_rv64_20000622_1_foo_logical_select_runtime_abort.md`.

## Reviewer Reject Signals

- Reject a slice that claims progress by changing expectations, unsupported
  markers, allowlists, runtime comparison behavior, or the gcc_torture runner
  instead of repairing object-route semantics.
- Reject filename-specific matching for `20000622-1.c`, exact source text, or
  exact generated symbol names.
- Reject a route that labels the abort as fixed without artifact evidence
  showing the first post-call-lowering failure family.
- Reject a patch that reintroduces the old ordinary `CallInst`
  `unsupported_instruction_fragment` fallback for this representative.
- Reject broad RV64 rewrites that do not include focused proof for the
  classified runtime-mismatch family.
