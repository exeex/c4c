Status: Active
Source Idea Path: ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Remaining Duff Fallthrough Copy Emission

# Current Packet

## Just Finished

Step 4's prepared memory validation regression was corrected while preserving
the Duff fallthrough fixed-offset local copy emission repair. Structured
frame-slot validation still accepts prepared frame-slot authority for a local or
pointer address when the prepared access has no residual byte offset, which
covers exact resolved frame-slot addresses. It now compares nonzero prepared
residual offsets against the BIR structured address offset again, so mismatched
BIR/prepared offsets fail closed.

The focused Step 4 dispatch coverage still emits the retained/prepared-index
fallthrough local copy path, including the memory ops whose retained and
prepared indices differ by value identity.

## Suggested Next

Classify or repair the now-exposed `00143` control-flow residual around the
Duff do-while latch. The generated `.LBB1_29` stores `--n`, then reloads `n`,
subtracts one again, and branches on that second `n - 1` compare, so the backend
terminates the loop one iteration early.

## Watchouts

- The remaining `00143` failure is no longer the fixed-offset fallthrough copy
  emission gap from idea 341 Step 4; the first bad generated-assembly fact has
  moved to the latch condition.
- Prepared frame-slot validation must stay fail-closed when both BIR and
  prepared records carry residual offset facts and those facts disagree.
- Temporary evidence paths from this packet: `/tmp/c4c_00143_step4.s` and
  `/tmp/c4c_00143_step4_prepared_bir.txt`.

## Proof

Ran the delegated proof:
`cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test backend_aarch64_prepared_memory_operand_records_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|machine_printer|prepared_memory_operand_records)|c_testsuite_aarch64_backend_src_(00086|00111|00143)_c' > test_after.log 2>&1`.
The build succeeded. `backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_instruction_dispatch`, `backend_aarch64_machine_printer`,
`00086`, and `00111` passed. `00143` still fails `[RUNTIME_NONZERO]` with
exit 1, matching the already classified latch-condition residual rather than
the Step 4 fallthrough copy emission path. Proof log path: `test_after.log`.
