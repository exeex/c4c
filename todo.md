Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run Broader AArch64 Backend C-Testsuite Route

# Current Packet

## Just Finished

Completed Step 4: ran the broader registered AArch64 backend c-testsuite route
with label `aarch64_backend` in `build-aarch64-scan` and preserved the output
in `test_after.log`.

CTest selected 220 registered AArch64 backend tests. Configure and the
`c4cll` build completed; CTest registration was not the blocker. Result:
0 passed, 220 failed. Because `C_TESTSUITE_AARCH64_BACKEND_RUNNER` was not
configured on this host, runtime-unavailable cases are classified distinctly
and are not counted as passes.

Failure-stage summary from `test_after.log`:

- `[RUNTIME_UNAVAILABLE]`: 121 cases. Representative cases:
  `src/00001.c`, `src/00002.c`, `src/00003.c`, `src/00004.c`,
  `src/00009.c`. These reached an AArch64 backend binary but could not run
  directly on this host without an AArch64 runner/emulator command.
- `[FRONTEND_FAIL]`: 85 cases. Representative nontrivial families:
  `src/00040.c` / `src/00056.c` for deferred unsupported call-boundary move
  nodes reaching the machine-node printer; `src/00024.c` for scalar `sub`
  with immediate lhs and register rhs; `src/00027.c`, `src/00028.c`,
  `src/00029.c`, and `src/00035.c` for scalar `or`/`and`/`xor` outside the
  printable add/sub subset; `src/00031.c` for add/sub immediate range;
  `src/00143.c` / `src/00157.c` for semantic `lir_to_bir` capability-bucket
  failures; `src/00204.c` for bootstrap `lir_to_bir` global support limits.
- `[BACKEND_FAIL]`: 14 cases. Representative cases: `src/00005.c`,
  `src/00006.c`, `src/00007.c`, `src/00008.c`, `src/00010.c`,
  `src/00033.c`, `src/00034.c`, `src/00101.c`, `src/00127.c`,
  `src/00129.c`; these fail in backend assembly/link handling with
  undefined temporary symbols such as `.LBB1_2` / `.LBB2_5`.
- `[BACKEND_OUTPUT_MISSING]`, `[BACKEND_FALLBACK_IR]`, `[RUNTIME_NONZERO]`,
  and `[RUNTIME_MISMATCH]`: 0 cases in this run.

## Suggested Next

Take the largest non-runtime failure family as the next implementation packet:
repair or further isolate the AArch64 call-boundary move lowering/selection
path that currently reaches the machine-node printer as
`deferred_unsupported`, represented by `src/00040.c` and `src/00056.c`.

## Watchouts

- A passing test must use AArch64 backend `.s`, not LLVM IR fallback.
- Runtime-unavailable is not a pass; it is the current host limitation for
  121 cases in this broader run.
- The current broad route is registered and executable; the blocker is not
  configure, build, or CTest registration.
- Do not weaken expected outputs or shrink allowlists to get green.
- Do not add named-case AArch64 lowering shortcuts for the representative
  c-testsuite sources; fix semantic lowering/selection families.
- If rerun on a host with `C_TESTSUITE_AARCH64_BACKEND_RUNNER` configured,
  reclassify the 121 runtime-unavailable cases because they may become
  `[RUNTIME_NONZERO]`, `[RUNTIME_MISMATCH]`, or true passes.

## Proof

Ran the delegated proof command and preserved output in `test_after.log`:

```sh
{ cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

Configure and build completed. CTest selected 220 tests with label
`aarch64_backend` and reported `0% tests passed, 220 tests failed out of 220`.
The proof is sufficient for route execution and failure-stage classification;
it is not a green acceptance proof for backend capability.
