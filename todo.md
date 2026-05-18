Status: Active
Source Idea Path: ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Identify Representative Undefined-Label Cases

# Current Packet

## Just Finished

Step 1 from `plan.md` identified a representative undefined temporary-label
family in the AArch64 backend c-testsuite route. The delegated subset selected
five cases, and all five failed at `[BACKEND_FAIL]` with undefined `.LBB...`
temporary symbols:

- `00005.c`: missing `.LBB1_6`
- `00006.c`: missing `.LBB1_2`
- `00040.c`: missing `.LBB99_2`, `.LBB99_7`, `.LBB99_9`, `.LBB99_11`,
  `.LBB99_13`, `.LBB100_16`, `.LBB100_25`
- `00156.c`: missing `.LBB89_2`
- `00220.c`: missing `.LBB164_2`

This packet did not observe `[RUNTIME_UNAVAILABLE]`, scalar printer,
`lir_to_bir`, or unrelated diagnostics in the selected subset. Runtime
unavailable remains separate and was not counted as pass evidence.

## Suggested Next

Run Step 2: trace label ownership for the representative cases from prepared
module/block facts through AArch64 branch lowering and assembly emission. The
next packet should identify the general rule that should define referenced
temporary labels before implementation, without matching c-testsuite filenames
or exact `.LBB` spellings.

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Do not match c-testsuite filenames, exact case names, or exact `.LBB`
  spellings instead of repairing label authority or emission semantics.
- The first implementation packet should use the same representative subset as
  its narrow c-testsuite proof:
  `ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00005|00006|00040|00156|00220)_c$'`.
- The current failing stage is `[BACKEND_FAIL]`; keep assembler/link,
  backend-printer, frontend, `lir_to_bir`, and runtime classifications
  stage-specific if later packets uncover them.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00005|00006|00040|00156|00220)_c$'; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected 5 tests, 0 passed, 5 failed.
All selected failures are `[BACKEND_FAIL]` undefined temporary `.LBB...` labels.
The nonzero proof exit is the expected baseline for this classification-only
packet. Proof log path: `test_after.log`.
