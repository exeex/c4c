Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct post-299 residual inventory

# Current Packet

## Just Finished

Completed `plan.md` Step 1: reconstructed the post-299 residual inventory from
the accepted broad backend-regex proof in `test_before.log`.

Proof basis: `test_before.log` reports 352 selected tests, 294 passed, and 58
failed after idea 299. No broad runtime rerun was performed.

Local backend/unit failures: none in the reported failure summary. The selected
local buckets such as `backend`, `backend_route`, `backend_runtime`, `bir`, and
AArch64 backend unit tests completed without listed failures.

c_testsuite residuals: all 58 listed failures are
`c_testsuite_aarch64_backend_src_*` tests.

- Machine-printer/frontend scalar cast: 7
  - `00035.c`, `00105.c`, `00126.c`, `00134.c`, `00135.c`, `00151.c`,
    `00208.c`
  - Symptoms include `zero_extend` requiring supported integer source/result
    width and `sign_extend` requiring a structured register source.
- Machine-printer/frontend symbol-store value: 5
  - `00176.c`, `00181.c`, `00182.c`, `00213.c`, `00214.c`
  - Symptom: `opcode=store: symbol store value is not a register or immediate
    operand`.
- Other machine-printer/frontend: 7
  - scalar mul/div/rem operand form: `00064.c`, `00139.c`
  - stack-slot store source scratch: `00173.c`, `00187.c`, `00194.c`
  - scalar unsigned reduction: `00205.c`
  - unselected machine node at printer: `00140.c`
- semantic `lir_to_bir` handoff failures: 2
  - `00204.c`, `00216.c`
  - Symptom: AArch64 assembly route requires semantic `lir_to_bir` lowering
    before prepared-module handoff, but semantic lowering failed outside the
    admitted feature families.
- Backend assembler invalid operand: 1
  - `00104.c`
  - Symptom: assembler rejects `sxtw w20, w13`.
- Runtime nonzero: 20
  - `00050.c`, `00066.c`, `00086.c`, `00089.c`, `00102.c`, `00111.c`,
    `00112.c`, `00113.c`, `00119.c`, `00121.c`, `00123.c`, `00137.c`,
    `00138.c`, `00144.c`, `00170.c`, `00179.c`, `00189.c`, `00200.c`,
    `00207.c`, `00215.c`
  - Includes ordinary nonzero exits plus at least one segmentation fault
    (`00089.c`).
- Runtime mismatch: 14
  - `00157.c`, `00159.c`, `00164.c`, `00168.c`, `00169.c`, `00172.c`,
    `00174.c`, `00175.c`, `00185.c`, `00186.c`, `00193.c`, `00195.c`,
    `00196.c`, `00218.c`
  - Symptom: compiled program exits but stdout differs from expected output.
- Timeout: 2
  - `00143.c`, `00220.c`
  - Treat as hang-specific/quarantine until evidence connects either timeout to
    another semantic family.

Best next focused-owner candidate: machine-printer scalar-cast residuals. This
is the clearest repeated semantic diagnostic bucket: 7 failures, all frontend
machine-printer failures, and the cases split into concrete cast operand-width
and structured-register-source symptoms. Runtime nonzero/mismatch buckets are
larger, but they need generated-assembly or narrower probes before a semantic
owner is defensible.

## Suggested Next

Run `plan.md` Step 2 to define the scalar-cast machine-printer owner candidate:
name the capability, include the 7 scalar-cast cases above, separate
zero/sign-extension sub-symptoms, and define proof scope before any
implementation starts.

## Watchouts

- Do not implement fixes under this umbrella plan.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 299 from failing counts alone.
- Runtime nonzero and mismatch are the largest buckets by count, but they are
  not owner-ready without generated-code or narrower diagnostic evidence.
- Timeout cases should stay isolated unless later evidence proves a shared
  semantic owner.

## Proof

Used existing accepted proof log only: `test_before.log` with 352 selected, 294
passed, and 58 failed. Per packet instruction, no broad runtime tests were
rerun and `test_after.log` was not touched.
