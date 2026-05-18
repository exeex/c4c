Status: Active
Source Idea Path: ideas/open/277_aarch64_c_testsuite_call_boundary_move_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Identify Representative Failure Cases

# Current Packet

## Just Finished

Step 1 from `plan.md`: ran the delegated AArch64 backend c-testsuite scan and
classified the call-boundary move `deferred_unsupported` family from fresh
`test_after.log` evidence.

Representative failures:

- `tests/c/external/c-testsuite/src/00040.c`: recursive call-heavy board search
  with `calloc` setup; printer failure at function 0 block 9 instruction 3.
- `tests/c/external/c-testsuite/src/00056.c`: simple local integer values passed
  to `printf`; printer failure at function 0 block 0 instruction 0.
- `tests/c/external/c-testsuite/src/00125.c`: minimal `printf("hello world")`
  call; printer failure at function 0 block 0 instruction 0.
- `tests/c/external/c-testsuite/src/00131.c`: repeated `printf("Hello")` direct
  calls; printer failure at function 0 block 0 instruction 0.
- `tests/c/external/c-testsuite/src/00156.c`: loop-carried integer passed to
  `printf`; printer failure at function 0 block 3 instruction 0.

Observed failing form/message for all representatives:

`printer requires selected machine node, got deferred_unsupported: call-boundary move node requires prepared register source and destination`

Classification from this scan:

- `[RUNTIME_UNAVAILABLE]`: 121 cases. These are not passes; this host still has
  no configured `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.
- `[FRONTEND_FAIL]`: 85 cases total. 58 of these are the call-boundary move
  `deferred_unsupported` family above; the rest are other frontend/BIR/printer
  failures such as scalar printer coverage, immediate-range spelling, semantic
  `lir_to_bir`, or global lowering blockers.
- `[BACKEND_FAIL]`: 14 cases, currently dominated by undefined temporary symbol
  failures. These are separate from the call-boundary move family.

## Suggested Next

Execute Step 2/3 against the representative call-boundary move family. Trace why
the call-boundary move node reaches the AArch64 machine-node printer without
prepared register source and destination, then add focused backend proof for
the same semantic move form before implementation.

Suggested narrow proof command for the next implementation packet:

```sh
{ cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00040|00056|00125|00131|00156)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- The call-boundary move family is currently reported as `[FRONTEND_FAIL]` by
  the c-testsuite runner because the assembly route reaches the machine-node
  printer and fails there. Treat it as the backend capability family owned by
  this runbook, not as runtime proof.
- Keep unrelated `[BACKEND_FAIL]` undefined temporary symbol cases and
  non-call-boundary frontend/BIR cases out of the next implementation packet.

## Proof

Delegated proof command:

```sh
{ cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected 220 AArch64 backend tests and
reported 0 passes / 220 failures. This is expected for the scan route on this
host because stage-classified failures remain failures and runtime execution is
unavailable. Proof log: `test_after.log`.
