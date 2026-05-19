Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Step 1: Reconstruct Post-306 Backend-Regex Inventory completed from the
supervisor-created broad `test_before.log` capture without rerunning tests.
The command represented by the log is:
`cmake --build --preset default` followed by
`ctest --test-dir build -j10 -R backend --output-on-failure --timeout 30`.

Inventory summary:

- Selected: 352 backend-regex tests.
- Passed: 306.
- Failed total: 46, all under `c_testsuite_aarch64_backend_*`.
- Failed non-timeout: 43.
- Timed out: 3.
- Skipped/not run/interrupted: none visible in the CTest summary.
- Local backend/unit/CLI residuals: none visible; local backend, CLI, route,
  AArch64 unit, and prepared-BIR tests selected by `-R backend` passed.

Failing test names:

- `c_testsuite_aarch64_backend_src_00035_c`
- `c_testsuite_aarch64_backend_src_00066_c`
- `c_testsuite_aarch64_backend_src_00086_c`
- `c_testsuite_aarch64_backend_src_00089_c`
- `c_testsuite_aarch64_backend_src_00102_c`
- `c_testsuite_aarch64_backend_src_00111_c`
- `c_testsuite_aarch64_backend_src_00112_c`
- `c_testsuite_aarch64_backend_src_00113_c`
- `c_testsuite_aarch64_backend_src_00119_c`
- `c_testsuite_aarch64_backend_src_00123_c`
- `c_testsuite_aarch64_backend_src_00137_c`
- `c_testsuite_aarch64_backend_src_00138_c`
- `c_testsuite_aarch64_backend_src_00140_c`
- `c_testsuite_aarch64_backend_src_00143_c`
- `c_testsuite_aarch64_backend_src_00144_c`
- `c_testsuite_aarch64_backend_src_00151_c`
- `c_testsuite_aarch64_backend_src_00157_c`
- `c_testsuite_aarch64_backend_src_00159_c`
- `c_testsuite_aarch64_backend_src_00164_c`
- `c_testsuite_aarch64_backend_src_00168_c`
- `c_testsuite_aarch64_backend_src_00169_c`
- `c_testsuite_aarch64_backend_src_00170_c`
- `c_testsuite_aarch64_backend_src_00172_c`
- `c_testsuite_aarch64_backend_src_00173_c`
- `c_testsuite_aarch64_backend_src_00174_c`
- `c_testsuite_aarch64_backend_src_00175_c`
- `c_testsuite_aarch64_backend_src_00176_c`
- `c_testsuite_aarch64_backend_src_00179_c`
- `c_testsuite_aarch64_backend_src_00181_c`
- `c_testsuite_aarch64_backend_src_00182_c`
- `c_testsuite_aarch64_backend_src_00185_c`
- `c_testsuite_aarch64_backend_src_00186_c`
- `c_testsuite_aarch64_backend_src_00187_c`
- `c_testsuite_aarch64_backend_src_00189_c`
- `c_testsuite_aarch64_backend_src_00193_c`
- `c_testsuite_aarch64_backend_src_00195_c`
- `c_testsuite_aarch64_backend_src_00196_c`
- `c_testsuite_aarch64_backend_src_00200_c`
- `c_testsuite_aarch64_backend_src_00204_c`
- `c_testsuite_aarch64_backend_src_00207_c`
- `c_testsuite_aarch64_backend_src_00208_c`
- `c_testsuite_aarch64_backend_src_00214_c`
- `c_testsuite_aarch64_backend_src_00215_c`
- `c_testsuite_aarch64_backend_src_00216_c`
- `c_testsuite_aarch64_backend_src_00218_c`
- `c_testsuite_aarch64_backend_src_00220_c`

Step 2 read-only bucket classification evidence recorded:

- Machine-printer/prepared-node gaps: `00140`, `00164`, `00214`.
  Evidence: frontend-stage AArch64 assembly route reached the machine-node
  printer and failed before assembly. Diagnostics name call-boundary move
  outside the selected subset (`00140`), scalar `mul` with incomplete printable
  rhs facts (`00164`), and scalar `add` with memory operands requiring prepared
  frame-slot sources (`00214`). These are crisp but not a single shared owner.
- Semantic `lir_to_bir` admission gaps: `00204`, `00216`.
  Evidence: both fail before prepared-module handoff. `00204` reports
  `myprintf` failed in `gep local-memory`; `00216` reports `foo` failed in
  `load local-memory` over aggregate/initializer-heavy source. The common
  surface is local-memory admission, but the operation shapes differ enough to
  need a focused probe before merging them.
- Assembler/linker emission gaps: `00182`, `00189`.
  Evidence: `00182` emits illegal AArch64 immediate materialization
  `mov x0, #1234567`; `00189` links an object with non-PIC
  `R_AARCH64_ADR_PREL_PG_HI21` relocation against external `stdout`. These
  are clear single-test candidates, not one shared bucket.
- Runtime nonzero with empty output: `00035`, `00066`, `00086`, `00102`,
  `00111`, `00112`, `00113`, `00119`, `00123`, `00137`, `00138`, `00144`,
  `00151`. Evidence: runner reports `[RUNTIME_NONZERO] exit=1` with no
  stdout/stderr payload. Source shapes include constants, strings, floating
  comparisons, pointer/null conversions, and aggregate initializers; no shared
  generated-code root is proven by this log alone.
- Runtime signal/crash: `00089` bus error; `00170`, `00173`, `00179`,
  `00207`, `00208`, `00215` segmentation faults. Evidence: runner reports
  `[RUNTIME_NONZERO]` with signal exits. Source shapes include function
  pointers, enum/forward enum, string/library calls, VLA/goto, struct local
  arrays, and loop/control cases; keep parked until generated-code inspection
  proves a common memory/control-flow root.
- Runtime mismatch, scalar/control/local-memory visible: `00157`, `00159`,
  `00168`, `00169`, `00172`, `00176`, `00185`, `00186`, `00193`, `00196`,
  `00200`, `00218`. Evidence: mismatches cover array stores/loads, simple
  function calls, recursion, nested loop induction, pointer comparison,
  quicksort/global array mutation, local array initializers, sprintf buffer
  updates, switch return/break control, short-circuit calls, shift promotion
  typing, and unsigned enum bit-field zero extension. This is too broad for
  one owner without narrower generated-code probes.
- Runtime mismatch, floating/aggregate layout visible: `00174`, `00175`,
  `00195`. Evidence: `00174` prints all-zero float math results and corrupt
  comparison booleans; `00175` corrupts scalar conversions and function
  argument values; `00195` corrupts `double` fields in a global struct array.
  This may overlap floating/register or aggregate-memory handling but is not
  proven as one semantic owner from the log alone.
- Runtime output storm/crash: `00181`. Evidence: Tower of Hanoi run prints
  repeated corrupt tower state for about 1.28 seconds, then segfaults. Keep
  separate from ordinary segmentation faults because the failure produces a
  large output storm and likely bad recursive/global array state.
- Timeout residuals: `00143`, `00187`, `00220`. Evidence: CTest reports
  `***Timeout` at the 5-second per-test timeout with no detailed runner
  diagnostic in the tail summary. Source shapes are Duff's device/copy loop,
  file I/O/fgetc loop, and wide-character UTF-8 output. Parked until bounded
  per-test probes can distinguish infinite loop from slow/output behavior.

Parked uncertain buckets:

- The 13 empty-output `exit=1` tests need generated-code or per-test runner
  evidence before they can be merged with mismatch/crash families.
- Runtime crash and output-storm cases need bounded probes before owner
  selection because the same signal result covers unrelated source shapes.
- Runtime mismatch buckets are evidence-rich but semantically mixed; selecting
  an owner from them now risks testcase-overfit.
- Timeout cases require stale-process-safe focused probes before classification
  beyond "timeout".

## Suggested Next

Step 3 should choose a focused owner only after one crisp capability boundary is
confirmed. The best immediate owner candidate from this read-only pass is the
single-test AArch64 immediate materialization failure in `00182`
(`mov x0, #1234567` rejected by the assembler), with boundaries excluding
PIC/global-symbol relocation (`00189`) and runtime mismatch behavior. If the
supervisor wants a multi-test owner instead, the missing evidence is a bounded
generated-code probe tying one runtime mismatch/crash subgroup to the same
semantic backend capability.

## Watchouts

- Do not implement repairs under umbrella idea 295.
- Do not reopen closed owners 285 through 306 from failure counts alone.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or proof-log policy.
- `00181` caused a very large output section in `test_before.log`; any future
  focused runtime probe should keep the supervisor's stale-process and timeout
  safeguards.
- The broad capture proves the local backend/unit/CLI side is not the residual
  source, but it does not prove a shared root cause for the runtime buckets.

## Proof

No tests were rerun for this packet, per supervisor instruction. Proof was
read-only classification from the existing `test_before.log` broad backend
capture and generated artifacts under `build/c_testsuite_aarch64_backend/`.
`test_after.log` was not produced or modified for this classification-only
packet.
