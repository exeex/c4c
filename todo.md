Status: Active
Source Idea Path: ideas/open/277_aarch64_c_testsuite_call_boundary_move_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement General Move Lowering Or Selection

# Current Packet

## Just Finished

Step 4 from `plan.md`: implemented the general AArch64 before-call GPR move
selection rule for symbol-address call arguments when prepared storage already
provides a concrete source register and ABI destination register.

Changed implementation file:

- `src/backend/mir/aarch64/codegen/calls.cpp`

The focused Step 3 backend test now passes. The rule selects the semantic
`source_encoding=SymbolAddress` / `source_register_name=x13` / destination
`x0` form as a normal prepared GPR call-boundary move, preserving the existing
prepared value-home agreement check before emitting the selected machine node.

The packet also keeps unsupported call-boundary move shapes from reaching the
machine-node printer as half-selected records. Non-selected call-argument and
call-result move shapes now report targeted lowering diagnostics before
printing, while the printer still retains its fail-closed diagnostic for
genuinely malformed call-boundary move records that reach it.

Representative c-testsuite status after this packet:

- `00056.c`, `00125.c`, and `00131.c`: progressed to `[RUNTIME_UNAVAILABLE]`
  because no `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is configured.
- `00040.c` and `00156.c`: progressed to `[BACKEND_FAIL]` undefined temporary
  label diagnostics.
- None of the five representatives now fail with
  `deferred_unsupported: call-boundary move node requires prepared register source and destination`.

## Suggested Next

Run plan-owner or supervisor review of whether this runbook should close or
split. The symbol-address call-argument family is repaired, but true runtime
pass evidence is still blocked without an AArch64 runner, and the next visible
backend capability family is undefined temporary labels in representative
cases such as `00040.c` and `00156.c`.

If continuing with implementation instead of lifecycle review, the next
coherent packet should target the undefined temporary label backend family with
a focused backend test independent of c-testsuite filenames.

Do not treat `[RUNTIME_UNAVAILABLE]` as pass evidence.

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
- The immediate symbol-address call-argument support is no longer blocked.
- Remaining representative blockers are not the old call-boundary symbol-address
  move path.
- `00040.c` and `00156.c` now expose undefined temporary label backend failures;
  keep that as a separate capability family instead of folding it into the
  call-boundary move packet.
- `00056.c`, `00125.c`, and `00131.c` need an AArch64 runner or host for runtime
  evidence.

## Proof

Delegated proof command used:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00040|00056|00125|00131|00156)_c$'; } 2>&1 | tee test_after.log
```

Result:

- `cmake --build --preset default` succeeded.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` selected
  139 backend tests and all 139 passed.
- AArch64 scan configure/build succeeded.
- The representative c-testsuite command selected five tests and failed as
  expected for blockers outside this packet: three `[RUNTIME_UNAVAILABLE]`
  results and two `[BACKEND_FAIL]` undefined temporary label results.
- The old call-boundary move printer diagnostic is gone from all five
  representatives.

Proof log: `test_after.log`. The delegated proof command exits nonzero because
the representative c-testsuite cases still include runtime-unavailable and
undefined-label blockers; the Step 4 owned backend slice passed its focused
backend proof and moved the representatives to the next owner layer.
