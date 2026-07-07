# RV64 20000819 Runtime Mismatch After Pointer Publication

Status: Closed
Type: Focused RV64 runtime follow-up
Parent: `ideas/closed/583_rv64_pointer_arithmetic_result_publication.md`
Owning Layer: RV64 object/runtime route for GCC torture representative execution

## Goal

Diagnose and repair the downstream RV64 runtime mismatch for
`tests/c/external/gcc_torture/src/20000819-1.c` now that the route advances
past the old pointer arithmetic publication owner.

## Why This Exists

Idea 583 repaired RV64 pointer-valued add/sub result publication. Its retained
representative no longer stops at the old compile-time
`unsupported_pointer_arithmetic` owner for `function=foo`, `block=entry`,
`instruction_index=7`, `owner=ptr %t4`.

The same representative now builds the C4C object and linked binary, then
fails in the runtime comparison with `[RV64_BACKEND_RUNTIME_MISMATCH]`:
`clang_exit=0`, `c4c_exit=Subprocess aborted`. No downstream compile-time
diagnostic owner was reported by the object route.

## In Scope

- Reproduce the `20000819-1.c` RV64 runtime mismatch after pointer publication.
- Compare C4C and clang execution enough to identify the first semantic
  divergence or abort cause.
- Repair the responsible RV64 lowering, object emission, runtime harness, or
  prepared-data publication bug once isolated.
- Add focused coverage for the repaired runtime behavior without making the
  representative itself the only proof.

## Out Of Scope

- Reopening pointer-valued add/sub result publication unless fresh evidence
  shows that repair is still incorrect.
- Marking `20000819-1.c` unsupported, weakening the runtime comparison, or
  changing expectations as a substitute for semantic repair.
- Broad rewrites of unrelated RV64 call, select, local-memory, or integer ALU
  lowering before the runtime abort has a concrete owner.
- Filename-, function-, block-, or value-name-specific shortcuts for this
  representative.

## Acceptance Criteria

- The route records the concrete runtime abort/divergence owner for
  `20000819-1.c` after pointer publication.
- A focused regression test proves the repaired behavior or, if the first
  owner is a separately scoped downstream capability, the route records that
  owner with enough context for a narrower follow-up.
- The representative no longer fails with `[RV64_BACKEND_RUNTIME_MISMATCH]`
  for the same abort cause.
- Relevant RV64 backend validation passes after the repair.

## Closure Notes

Closed after the route isolated the runtime abort owner to `main`, `entry`,
instruction `0`, where the computed global-address call argument `%t2` for
`@a + 4` was treated as already available in `s1` and copied to `a0`.

RV64 object emission now materializes computed global-address call arguments
through the semantic relocation path before the call. Focused
`backend_riscv_object_emission` coverage proves the shape without relying on
the `20000819-1.c` representative as the only assertion.

The representative
`tests/c/external/gcc_torture/src/20000819-1.c` now passes the RV64
object/runtime route. The previous `[RV64_BACKEND_RUNTIME_MISMATCH]` /
`Subprocess aborted` cause is gone; `main` materializes `a + 4` into `a0`
with a PC-relative relocation pair before `foo`, with no stale `mv a0,s1`
before that call.

Close proof: Step 5 backend validation passed with
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.
CTest reported `100% tests passed, 0 tests failed out of 346`.

No separate follow-up idea is required for this source idea.

## Reviewer Reject Signals

- Reject changes that classify `20000819-1.c` as unsupported, weaken the
  runtime comparison, or rewrite expectations without repairing the runtime
  cause.
- Reject testcase-shaped matching on the representative filename, `foo`,
  block names, value names, or exact command output.
- Reject reopening pointer arithmetic publication as the claimed fix unless
  logs prove the published pointer result is still semantically wrong.
- Reject broad RV64 rewrites that do not identify the first runtime abort or
  divergence owner.
- Reject helper renames, diagnostic reshuffling, or harness-only changes
  claimed as backend runtime progress.
