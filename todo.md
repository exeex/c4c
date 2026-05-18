Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Select the Next Semantic Owner

# Current Packet

## Just Finished

Completed Step 1 by reusing the current post-292 timeout-protected AArch64
backend scan artifacts from the scalar-control close validation. No broad scan
rerun was needed.

Current totals from `/tmp/c4c_aarch64_scalar_step4_broad.log`:

- 212 AArch64 backend c-testsuite cases scanned.
- 118 passed, 94 failed.
- Failure markers: 55 `FRONTEND_FAIL`, 24 `RUNTIME_NONZERO`, 13
  `RUNTIME_MISMATCH`, 2 `TIMEOUT`.
- Closed Step 292 starter representatives now pass in the broad scan:
  `00009`, `00012`, `00056`, `00156`, `00161`, and `00211`.

Representative remaining buckets:

- Printer/admission frontend-labeled failures: branch printer `00041`,
  `00054`, `00127`, `00203`; scalar printer `00024`, `00027`, `00028`,
  `00029`, `00134`, `00139`, `00213`; memory printer `00173`, `00187`,
  `00194`; semantic `lir_to_bir` local-memory gaps `00046`, `00143`,
  `00157`, `00176`, `00205`, `00216`, `00218`.
- Runtime nonzero failures: `00050`, `00121`, `00089`, `00019`, `00032`,
  `00042`, `00066`, `00086`, `00102`, `00109`, `00112`, `00113`, `00111`,
  `00119`, `00123`, `00130`, `00138`, `00137`, `00144`, `00170`, `00180`,
  `00179`, `00186`, `00189`.
- Runtime mismatch failures: `00159`, `00164`, `00169`, `00168`, `00172`,
  `00175`, `00174`, `00183`, `00193`, `00196`, `00202`, `00217`, `00219`.
- Timeout failures: `00132` and `00220`.

The stale generated-runtime process artifact
`/tmp/c4c_aarch64_scalar_step4_broad.pgrep` is empty, so no stale generated
runtime process was recorded after the scan.

## Suggested Next

Step 2 should sample the runtime pointer/aggregate and side-effect/control
value bucket first, using representatives `00159`, `00164`, `00168`, `00169`,
`00172`, `00183`, `00193`, `00202`, and `00217`. This is the smallest
backend/runtime-focused candidate set left after Step 292 because it avoids
printer/admission gaps, timeout cases, and the mostly compile-stage
local-memory `lir_to_bir` failures while still covering call return values,
loop-carried locals, pointer dereference/comparison, switch control flow,
compound assignment side effects, and string/aggregate storage symptoms.

## Watchouts

- Do not implement fixes under this umbrella idea.
- Do not reopen closed AArch64 owners without contradictory generated-code
  evidence.
- The `FRONTEND_FAIL` bucket is mostly AArch64 backend admission/printer failure
  text in this artifact, not necessarily frontend source-language ownership.
- Timeout cases `00132` and `00220` require explicit timeout handling and
  stale-process cleanup before they become a repair owner.
- Defer scalar printer-only cases such as `00024`, `00027`, `00028`, `00029`,
  `00134`, `00139`, and `00213` unless Step 2 evidence shows the runtime
  bucket is not coherent.

## Proof

Reused existing timeout-protected artifacts; no root proof logs were written and
no broad scan was rerun.

Artifacts used:

- `/tmp/c4c_aarch64_scalar_step4_broad.log`
- `/tmp/c4c_aarch64_scalar_step4_summary.txt`
- `/tmp/c4c_aarch64_scalar_step4_new_passes.txt`
- `/tmp/c4c_aarch64_scalar_step4_broad.pgrep`

Classification checks:

- `rg -o "\[(FRONTEND_FAIL|RUNTIME_NONZERO|RUNTIME_MISMATCH)\]" /tmp/c4c_aarch64_scalar_step4_broad.log | sort | uniq -c`
- `rg -n "Timeout|TIMEOUT" /tmp/c4c_aarch64_scalar_step4_broad.log`
