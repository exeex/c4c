Status: Active
Source Idea Path: ideas/open/369_rv64_object_route_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Semantic RV64 Terminator Emission

# Current Packet

## Just Finished

Completed Step 2 and Step 3 of `plan.md` for the first audited terminator
shape: added focused RV64 object-emission coverage for a prepared fused
`sgt i32 %reg, %reg` conditional branch, then lowered that shape semantically
by normalizing it to an RV64 `slt` branch with swapped operands.

The focused tests also keep adjacent fail-closed coverage for unsupported
fused branch predicates and non-i32 `sgt` branch shapes.

## Suggested Next

Run the Step 5 representative RV64 GCC torture backend subset for
`src/20000224-1.c` and `src/20000112-1.c` to see whether the first case now
advances and to confirm the next first unsupported terminator shape. If the
allowlist order remains unchanged, the likely next implementation packet is
the pointer-null `ne` fused branch for `src/20000112-1.c`.

## Watchouts

- `sgt i32` support is intentionally implemented as branch predicate
  normalization, not by testcase names or prepared-BIR text matching.
- The implementation does not broaden unsupported `sle` or non-i32 `sgt`
  fused branch shapes; those remain covered by the object-emission test's
  fail-closed assertions.
- `src/20000112-1.c` previously reached a pointer-typed null `ne` fused branch;
  that path was not part of this packet and still needs representative rerun
  confirmation before implementation.

## Proof

Ran the supervisor-selected proof:

`( cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure ) > test_after.log 2>&1`

Result: passed. `test_after.log` contains the fresh build and three-test CTest
run.
