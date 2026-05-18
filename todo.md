# AArch64 Scalar Parameter ALU Authority Todo

Status: Active
Source Idea Path: ideas/open/290_aarch64_scalar_parameter_alu_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Owner Boundary

# Current Packet

## Just Finished

Executed Step 4, "Validate Owner Boundary", for the scalar parameter ALU
authority idea.

The focused scalar ALU backend test passes, and the owner proof now passes
`src/00124.c`. There are no remaining blockers for this scalar parameter/ALU
owner.

Generated `f2` consumes the authoritative incoming scalar parameter registers:

```asm
f2:
    sub w0, w0, w1
    ret
```

The function-pointer return and indirect-call path remains valid: `f1`
materializes `f2` into `x0`, `main` calls `f1` through `blr x20`, and then
calls the returned function pointer through `blr x13`.

## Suggested Next

Hand off to the plan owner for the close/deactivate decision for the scalar
parameter ALU authority idea.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Do not reopen the closed function-pointer materialization owner unless new
  proof contradicts the closure review.
- Preserve the existing valid function-pointer return and indirect `blr`
  shape.
- Reject named-case shortcuts for `src/00124.c`, one function name, one
  parameter name, or one arithmetic expression.
- Step 2 parameter publication remains unchanged and covered; this packet
  validates AArch64 scalar ALU consumption of those authoritative facts.
- Keep the `00210` call-argument/register-authority blocker separate.
- No remaining owner-boundary blocker is known for `00124`.

## Proof

Baseline proof was already captured in `test_before.log` with:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00124_c$'; } > test_before.log 2>&1
```

Result: `c_testsuite_aarch64_backend_src_00124_c` failed
`[RUNTIME_NONZERO] exit=216`.

Step 4 focused scalar ALU backend test:

```sh
ctest --test-dir build --output-on-failure -R '^backend_aarch64_prepared_scalar_alu_records$'
```

Result: passed.

Ran the delegated Step 4 owner proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00124_c$'; } > test_after.log 2>&1
```

Result: passed. `test_after.log` is the canonical proof log for this Step 4
packet.
