Status: Active
Source Idea Path: ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun The Representative And Preserve 572 Evidence

# Current Packet

## Just Finished

Step 4 reran the delegated representative proof for `src/20000622-1.c` after
the ptrtoint materialization repair. The build succeeded, the parent 572
focused object-emission protection subset passed, and the representative still
advanced past the old ordinary same-module `CallInst` fallback.

The original classified `baz` wrong-argument family remains fixed in the
current artifacts: `baz` materializes from incoming `a2`, preserves the loaded
local in `s2` across `bar`, and passes `s2` as `foo` argument 0. The remaining
failure is still the later `foo` logical/select family: the object-route rerun
reports `[RV64_BACKEND_RUNTIME_MISMATCH] clang_exit=0 c4c_exit=Subprocess
aborted`.

## Suggested Next

The next coherent packet should classify and repair the later `foo`
logical/select publication behavior, preserving the fixed `baz`
pointer-to-integer local materialization and the parent 572 same-module
call/result protection.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, runtime comparison
  behavior, or the gcc_torture runner.
- Do not add filename-specific handling for `src/20000622-1.c`.
- Preserve the same-module call/result behavior repaired under 572.
- The old 572 ordinary `CallInst` fallback remains absent in this rerun; the
  representative reaches runtime comparison instead of compile-time admission
  failure.
- The prior classified bad value in `baz` remains fixed; do not regress the
  `a2 -> ptrtoint -> local -> s2 -> foo arg0` route.
- Remaining failure is in `foo` logical/select lowering, not `bar(a, 1)` result
  publication and not the original `baz` arg0 source selection.
- Artifact directory:
  `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/`.

## Proof

Focused Step 4 proof log: `test_after.log`.

Delegated combined proof was run exactly. `cmake --build --preset default`
passed, and
`ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
passed. The gcc_torture representative rerun preserved updated binaries and
objects under
`build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/`
and still fails with `[RV64_BACKEND_RUNTIME_MISMATCH] clang_exit=0
c4c_exit=Subprocess aborted`, classified as the later `foo` logical/select
condition family rather than the repaired `baz` ptrtoint materialization
family.
