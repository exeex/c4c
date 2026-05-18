# AArch64 Call Argument Register Authority Todo

Status: Active
Source Idea Path: ideas/open/291_aarch64_call_argument_register_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Destination ABI Register Readiness

# Current Packet

## Just Finished

Completed Step 3 validation for destination ABI register readiness and recorded
Step 4 owner-boundary evidence from the same proof. No implementation or test
files were changed in this packet.

Focused call-boundary backend proof passed, covering the selected call-boundary
move semantics. Generated `src/00210.c` assembly confirms the `printf` format
argument source is the authoritative `.str0` register `x21`, and the ABI
destination register is `x0`:

- `adrp x20, actual_function` / `add x20, x20, :lo12:actual_function` /
  `blr x20`
- `adrp x21, .str0` / `add x21, x21, :lo12:.str0` / `mov x0, x21` /
  `bl printf`

The attributed function-pointer calls remain indirect `blr x20` calls, so the
closed function-pointer materialization owner is still preserved.

## Suggested Next

Delegate closure review for this call-argument register-authority slice. No
remaining owner-boundary blocker is known: the source register, destination ABI
register, indirect-call shape, focused backend proof, and `00210` owner proof
are all green.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Do not reopen the closed function-pointer materialization owner unless new
  proof contradicts the closure review.
- Do not reopen the closed scalar parameter/ALU owner unless new proof
  contradicts that closure review.
- Preserve existing attributed function-pointer indirect calls through
  `actual_function`.
- Reject named-case shortcuts for `src/00210.c`, `.str0`, `printf`, one
  symbol, or one emitted move shape.
- Keep the scalar parameter/ALU and function-pointer materialization closures
  separated; this packet only validates call-argument destination/source
  authority and owner boundary.
- Do not weaken the scalar-cast fail-closed check for a spelling-only `xN`
  source used as an `I32`/`wN` operand. Placement-backed scalar casts remain the
  compatibility path for typed view adaptation.

## Proof

Focused backend proof:

- `ctest --test-dir build --output-on-failure -R '^backend_aarch64_instruction_dispatch$'`
  passed.

Backend suite proof:

- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  139/139 in the prior validation-blocker packet; no new backend-bucket run was
  delegated for this todo-only boundary validation.

Delegated owner proof:

- `{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00210_c$'; } > test_after.log 2>&1`
  passed. `test_after.log` contains the owner proof.
