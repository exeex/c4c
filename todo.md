# AArch64 Scalar Parameter ALU Authority Todo

Status: Active
Source Idea Path: ideas/open/290_aarch64_scalar_parameter_alu_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Selected ALU Operand Consumption

# Current Packet

## Just Finished

Executed Step 3, "Repair Selected ALU Operand Consumption", for the `00124`
scalar parameter inputs.

Repaired AArch64 prepared scalar ALU operand conversion so spelling-authority
GPR homes such as `x0`/`x1` can be consumed with the scalar view required by
the operation. For I32 ALU operands, the selected node now retargets the same
physical registers to `w0`/`w1` instead of rejecting the prepared record and
falling back to stale allocation registers.

Added focused AArch64 scalar ALU coverage in
`tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp` for
the exact authority shape exposed by `00124`: I32 scalar operands whose
prepared homes/storage spell ABI GPRs as `x0` and `x1` select W-register
def/use facts.

Generated `f2` no longer consumes stale `w19`/`w20`:

```asm
f2:
    sub w0, w0, w1
    ret
```

The function-pointer return and indirect-call path remains valid: `f1`
materializes `f2` into `x0`, `main` calls `f1` through `blr x20`, and then
calls the returned function pointer through `blr x13`.

## Suggested Next

Run the plan-owner close/deactivate decision for the scalar parameter ALU
authority idea, or delegate a closure-review packet if the supervisor wants one
more focused owner subset first.

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
  only changed AArch64 scalar ALU consumption of those authoritative facts.
- Keep the `00210` call-argument/register-authority blocker separate.

## Proof

Baseline proof was already captured in `test_before.log` with:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00124_c$'; } > test_before.log 2>&1
```

Result: `c_testsuite_aarch64_backend_src_00124_c` failed
`[RUNTIME_NONZERO] exit=216`.

Step 3 focused test target:

```sh
cmake --build build --target backend_aarch64_prepared_scalar_alu_records_test
ctest --test-dir build --output-on-failure -R '^backend_aarch64_prepared_scalar_alu_records$'
```

Result: passed.

Delegated backend proof subset:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed 139/139.

Ran the delegated Step 3 narrow proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00124_c$'; } > test_after.log 2>&1
```

Result: passed. `test_after.log` is the canonical proof log for this Step 3
packet.
