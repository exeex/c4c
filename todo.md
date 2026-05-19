Status: Active
Source Idea Path: ideas/open/302_aarch64_scalar_machine_node_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar Arithmetic Operand Forms

# Current Packet

## Just Finished

Step 2 repaired scalar `mul`/`div`/`rem` operand forms for prepared and
published scalar arithmetic records. Changed
`src/backend/mir/aarch64/codegen/alu.cpp` so the selected scalar arithmetic
printer materializes immediate lhs operands into scratch registers, avoids
scratch/result aliasing across immediate pairs, and retargets prepared scalar
ALU input operands to already-emitted scalar registers before reading stale
storage-plan homes. Updated
`tests/backend/mir/backend_aarch64_machine_printer_test.cpp` with focused
coverage for immediate lhs and immediate-pair `mul`/`sdiv`/`srem` forms.

Focused proof now has `c_testsuite_aarch64_backend_src_00064_c` and
`c_testsuite_aarch64_backend_src_00139_c` passing; neither fails from the old
scalar `mul`/`div` operand-form printer diagnostic.

## Suggested Next

Delegate Step 3 for `00205`: repair the unsigned power-of-two reduction path so
selected `logical_shift_right`/unsigned reduction records publish a register
lhs plus immediate reduction operand in the printable form.

## Watchouts

- `00064` exposed a second semantic issue after immediate lhs materialization:
  the following prepared scalar `sub` read `%t0` from stale prepared storage
  after the preceding `sdiv` had been emitted directly into the return
  register. The Step 2 repair retargets prepared scalar ALU operands to emitted
  scalar registers when available; keep this behavior in mind for future
  return-chain arithmetic packets.
- `00205` still fails with the Step 3 unsigned-reduction diagnostic:
  `scalar unsigned reduction node requires register lhs, immediate reduction,
  and result register`.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or root proof-log policy.

## Proof

Local development proof:
`cmake --build --preset default --target backend_aarch64_machine_printer_test &&
./build/tests/backend/mir/backend_aarch64_machine_printer_test` passed.

Supervisor-selected proof run exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00064|00139|00205)_c'; } > test_after.log 2>&1`.
Result: failed only because `c_testsuite_aarch64_backend_src_00205_c` remains
assigned to Step 3. `c_testsuite_aarch64_backend_src_00064_c` passed and
`c_testsuite_aarch64_backend_src_00139_c` passed. Proof log:
`test_after.log`.
