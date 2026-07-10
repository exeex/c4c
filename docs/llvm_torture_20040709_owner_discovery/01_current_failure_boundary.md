# Current Failure Boundary

## Question

What is the current first observable failure boundary for the
`20040709_2.c` and `20040709_3.c` LLVM torture rows?

## Evidence

Current baseline:
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`,
2026-07-10 04:20:43 UTC.

- Row 1941:
  `llvm_gcc_c_torture_src_20040709_2_c` failed.
- Row 1942:
  `llvm_gcc_c_torture_src_20040709_3_c` failed.

Current triage:
`docs/backend_baseline_history_triage/failure_classification.md` records both
rows as `intentionally deferred`, not assigned to an implementation owner.
`docs/backend_baseline_history_triage/follow_up_order.md` keeps
`ideas/open/668_llvm_torture_20040709_research.md` as research-only because
the earlier classification did not prove a first implementation owner for
rows 1941 and 1942.

Focused reproduction command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_20040709_2_c|llvm_gcc_c_torture_src_20040709_3_c)$') > test_after.log 2>&1
```

Focused reproduction result:

- Command exit: `8`.
- Build phase: `ninja: no work to do`.
- CTest subset: `0% tests passed, 2 tests failed out of 2`.
- `llvm_gcc_c_torture_src_20040709_2_c`: `[RUNTIME_FAIL]`,
  `clang_exit=0`, `c2ll_exit=Subprocess aborted`, empty `clang_out`, empty
  `c2ll_out`.
- `llvm_gcc_c_torture_src_20040709_3_c`: `[RUNTIME_FAIL]`,
  `clang_exit=0`, `c2ll_exit=Subprocess aborted`, empty `clang_out`, empty
  `c2ll_out`.

The harness definition in
`tests/c/external/gcc_torture/RunCase.cmake` reaches `[RUNTIME_FAIL]` only
after clang compiles and runs the source, `c4cll | clang -x ir -` produces the
`OUT_C2LL_BIN` binary, and the generated binary is executed for comparison.
The focused run produced current `c2ll` binaries under
`build/llvm_gcc_c_torture/src/`.

## Classification

| Row | Current first observable boundary | Evidence | Implementation evidence |
| --- | --- | --- | --- |
| `llvm_gcc_c_torture_src_20040709_2_c` | Runtime | The focused run reports `[RUNTIME_FAIL]` with clang execution exiting `0` and the `c2ll` binary aborting after compilation succeeds. | Not enough. The current evidence proves a generated-program runtime abort, but not whether the first repair owner is frontend semantic lowering, prepared/backend handoff, target/object lowering, runtime support, or another shared producer. |
| `llvm_gcc_c_torture_src_20040709_3_c` | Runtime | Same focused run result: `[RUNTIME_FAIL]`, clang execution exits `0`, and the `c2ll` binary aborts after compilation succeeds. | Not enough. The current evidence proves the same first observable runtime symptom, but not a concrete first implementation owner. |

## Evidence Versus Inference

Observed facts:

- Both rows remain current failures in
  `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.
- The current focused subset reproduces both failures.
- Both failures reach the runtime comparison point rather than a frontend,
  compile-pipe, backend-output, timeout, or harness-missing-output boundary.
- The clang-built binaries exit successfully; the generated `c2ll` binaries
  abort.

Inference:

- The current first observable boundary for both rows is runtime.
- Step 1 does not justify assigning either row to an implementation follow-up.
  The next packet should map the runtime evidence against existing follow-up
  ideas and should name an owner only if row-specific evidence proves that
  owner as the first repair layer.
