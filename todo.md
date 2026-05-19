Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct post-300 residual inventory

# Current Packet

## Just Finished

Step 1: Reconstruct post-300 residual inventory is complete from the accepted
post-idea-300 broad backend-regex proof in `test_before.log`.

Inventory summary:

- Selected proof set: 352 tests, 298 passed, 54 failed.
- Local backend/unit residuals: 0 failed. The selected local backend, backend
  route, backend runtime, BIR, CLI, AArch64 MIR/machine-printer, and smoke
  tests all passed; every residual is a `c_testsuite` AArch64 backend case.
- `c_testsuite` residuals: 54 failed total.

`c_testsuite` residual buckets:

- Frontend/machine-printer failures: 14
  - Scalar selected-node operand-shape gaps: `00064` div, `00139` mul.
  - Call-boundary move outside selected register subset: `00140`.
  - Memory store source scratch not printable: `00173`, `00187`, `00194`.
  - Memory store symbol value not register/immediate: `00176`, `00181`,
    `00182`, `00213`, `00214`.
  - Scalar unsigned reduction/logical-shift-right operand-shape gap: `00205`.
  - Semantic `lir_to_bir` local-memory handoff gaps: `00216` load
    local-memory, `00204` gep local-memory.
- Backend assembler failure: 1
  - `00104`: assembler rejects `sxtw w20, w13` as an invalid operand.
- Runtime nonzero failures: 23
  - Plain nonzero exits: `00035`, `00050`, `00066`, `00086`, `00102`,
    `00111`, `00112`, `00113`, `00119`, `00121`, `00123`, `00137`,
    `00138`, `00144`, `00151`, `00200`.
  - Runtime segmentation faults: `00089`, `00170`, `00179`, `00189`,
    `00207`, `00208`, `00215`.
- Runtime mismatch failures: 14
  - `00157`, `00159`, `00164`, `00168`, `00169`, `00172`, `00174`,
    `00175`, `00185`, `00186`, `00193`, `00195`, `00196`, `00218`.
- Runtime timeouts: 2
  - `00143`, `00220`.

## Suggested Next

Best next focused-owner candidate: AArch64 machine-printer memory-store operand
materialization, covering the 8 compile-blocking memory store diagnostics
(`00173`, `00176`, `00181`, `00182`, `00187`, `00194`, `00213`, `00214`).
This is the most coherent ready owner because the failures share explicit
printer diagnostics and do not require runtime-result triage.

## Watchouts

- Do not implement fixes under this umbrella plan.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 300 from failing counts alone.
- Runtime scans need timeout handling and stale-process cleanup.
- The 23 runtime-nonzero and 14 runtime-mismatch cases are real residuals, but
  they are less owner-ready from this inventory alone because the log records
  outcomes rather than a common lowering diagnostic.
- `00204` remains valuable as a prepared-BIR probe case, but its current broad
  failure is a semantic `gep local-memory` admission gap, not a local
  backend/unit failure.

## Proof

No tests rerun. Used existing accepted `test_before.log` broad proof:
352 selected, 298 passed, 54 failed after idea 300. `test_after.log` was not
modified for this inventory-only packet.
