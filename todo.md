Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Close Readiness

# Current Packet

## Just Finished

Completed Step 5: reran the focused AArch64 backend smoke route in
`build-aarch64-scan` and preserved the output in `test_after.log`.

Configure and the `c4cll` build completed. CTest selected the three smoke
tests with label `aarch64_backend_smoke`: `src/00001.c`, `src/00002.c`, and
`src/00003.c`. All three reached an AArch64 backend binary and failed as
`[RUNTIME_UNAVAILABLE]` because `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is unset
on this non-AArch64 host. Runtime-unavailable remains distinct and is not
counted as pass.

Close-readiness accounting:

- Focused route registration: confirmed by the three selected smoke tests.
- Focused backend path: confirmed far enough to produce AArch64 backend
  binaries before the host runtime boundary.
- Broader-route evidence: use the existing Step 4 inventory from the previous
  packet, which selected 220 `aarch64_backend` tests and classified failures as
  121 `[RUNTIME_UNAVAILABLE]`, 85 `[FRONTEND_FAIL]`, and 14 `[BACKEND_FAIL]`,
  with zero fallback-IR, output-missing, runtime-nonzero, or runtime-mismatch
  cases in that run.
- Expectations and allowlists: no expectations, allowlists, implementation
  files, CMake runner files, or tests were changed in this packet.
- Closure status: ready for plan-owner closure review as route-registration
  and failure-inventory evidence. The exact remaining blocker for true runtime
  pass evidence is environmental: configure
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` or run on an AArch64 host, then rerun
  the smoke route so runtime-unavailable cases can become runtime pass,
  runtime-nonzero, or runtime-mismatch.

## Suggested Next

Ask the plan owner for closure review of the route-readiness plan. Do not start
a new capability repair under this packet.

## Watchouts

- The delegated proof command pipes through `tee`, so the shell command exits
  successfully even though CTest reports three failed tests; the meaningful
  proof result is the CTest failure classification in `test_after.log`.
- Runtime-unavailable is not a pass. It is the current host limitation for the
  three focused smoke tests and for the 121 broader-route cases from Step 4.
- The close-readiness evidence should not be used to claim backend capability
  repairs. Non-runtime Step 4 failures remain separate frontend/backend
  capability work outside this packet.

## Proof

Ran the delegated Step 5 proof command and preserved output in
`test_after.log`:

```sh
{ cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend_smoke; } 2>&1 | tee test_after.log
```

Configure and build completed. CTest selected three tests with label
`aarch64_backend_smoke` and reported `0% tests passed, 3 tests failed out of
3`. All three failures are `[RUNTIME_UNAVAILABLE]` due to the unset
`C_TESTSUITE_AARCH64_BACKEND_RUNNER`; this proves the focused route reaches the
runtime boundary but is not green runtime acceptance evidence on this host.
