Status: Active
Source Idea Path: ideas/open/574_rv64_floating_point_binary_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Double FP Binary Lowering

# Current Packet

## Just Finished

Completed Step 3: implemented RV64 object-route lowering for prepared scalar
double floating-point binary operations with FPR homes.

Added a prepared F64 binary fragment path that requires F64 result, operand
type, and F64 lhs/rhs values, then emits RV64D register-register operations
from prepared FPR homes. `bir.sdiv double` now lowers semantically to
`fdiv.d` and the Step 2 focused object-emission test passes.

The supported F64 path also maps the same prepared FPR shape for `Add`, `Sub`,
and `Mul` to `fadd.d`, `fsub.d`, and `fmul.d`. Unsupported FP binary forms
still fail closed: `bir.sdiv fp128` and `bir.srem double` remain rejected by
the existing unsupported instruction diagnostic in the focused suite.

## Suggested Next

Execute Step 4 from `plan.md`: run the delegated representative object-route
proof for the localized `20000605-1.c` failure and record whether it advances
past the old `double %t5` unsupported binary owner.

## Watchouts

- The new object-emission path handles only prepared F64 values already in FPR
  homes. It does not materialize F64 values from stack homes or immediates.
- The representative owner observed in Step 1 uses `BinaryOpcode::SDiv` for
  floating division, so Step 4 should verify the object route reaches `fdiv.d`
  for that owner rather than the integer division path.
- The F128 and F64 remainder cases remain deliberate fail-closed coverage.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: passed. `backend_riscv_object_emission` reported 1/1 test passing, and
the focused F64 binary coverage now builds an object containing
`fdiv.d ft0, fa0, fa1` followed by `ret`.

Supervisor guard result:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 346/346.

Proof log: `test_before.log` after supervisor roll-forward.
