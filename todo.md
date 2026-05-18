# AArch64 Call Argument Register Authority Todo

Status: Active
Source Idea Path: ideas/open/291_aarch64_call_argument_register_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Call-Boundary Argument Source Selection

# Current Packet

## Just Finished

Resolved the Step 2 validation blocker in
`backend_aarch64_prepared_scalar_cast_records`. Scalar casts now enforce
register-view authority when a register spelling is the only source of storage
truth, while still allowing placement-backed scalar cast operands to derive the
typed view from placement.

The existing call-boundary repair semantics were preserved: `calls.cpp` still
treats an explicit prepared call argument register spelling as authoritative
when both a register name and a placement are present, falling back to placement
only when no spelling exists.

Focused coverage in `backend_aarch64_instruction_dispatch` now gives the
symbol-address call argument a semantic source register of `x21` plus a stale
callee-saved placement that would otherwise select `x20`; the selected move and
printed assembly must use `mov x0, x21`.

Generated `src/00210.c` assembly now preserves the attributed function-pointer
calls as indirect `blr x20` calls and passes the `printf` format pointer through
the authoritative `.str0` source register:

- `adrp x20, actual_function` / `add x20, x20, :lo12:actual_function` /
  `blr x20`
- `adrp x21, .str0` / `add x21, x21, :lo12:.str0` / `mov x0, x21` /
  `bl printf`

## Suggested Next

Delegate owner-boundary validation or closure review for this call-argument
register-authority slice. The `00210` owner proof is green and the backend
bucket is now green.

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
  separated; this packet only repaired call-boundary argument source selection
  and the scalar-cast register-view validation blocker it exposed.
- Do not weaken the scalar-cast fail-closed check for a spelling-only `xN`
  source used as an `I32`/`wN` operand. Placement-backed scalar casts remain the
  compatibility path for typed view adaptation.

## Proof

Focused backend proof:

- `ctest --test-dir build --output-on-failure -R '^backend_aarch64_prepared_scalar_cast_records$'`
  passed.
- `cmake --build build --target backend_aarch64_instruction_dispatch_test &&
  ctest --test-dir build --output-on-failure -R '^backend_aarch64_instruction_dispatch$'`
  passed in the prior Step 2 packet and remains covered by the backend bucket.

Backend suite proof:

- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  139/139.

Delegated owner proof:

- `{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00210_c$'; } > test_after.log 2>&1`
  passed.
