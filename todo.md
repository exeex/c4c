Status: Active
Source Idea Path: ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Narrow Aggregate Va Arg Stride Repair

# Current Packet

## Just Finished

Steps 3 and 4 added focused coverage and implemented the narrow RV64 aggregate
`va_arg` cursor stride repair.

Implementation:
- `make_rv64_aggregate_va_arg_access_plan` now refines the generic aggregate
  overflow plan when explicit RV64 incoming variadic GPR publication facts
  describe a contiguous 8-byte GPR save-area slot layout.
- For the supported publication-backed aggregate shape, the plan preserves
  `payload_size_bytes` and `copy_size_bytes` from the aggregate payload, but
  takes `source_slot_size_bytes`, `progression_stride_bytes`, and
  `overflow_stride_bytes` from the matched RV64 GPR publication slot size.
- Larger ordinary overflow-memory aggregates keep the generic overflow behavior
  derived from aggregate size/alignment.
- Malformed RV64 publication layouts fail closed during access-plan
  construction.

Focused coverage:
- `backend_prepared_printer_test` now checks the 4-byte RV64 aggregate
  `va_arg` shape publishes `copy_size=4` with `source_slot=8` and
  `progression_stride=8`, while the existing 8-byte aggregate shape still uses
  an 8-byte copy/stride.
- `backend_riscv_object_emission_test` now rejects a malformed prepared
  aggregate access plan whose source-slot size does not match the cursor
  stride, using the existing aggregate helper unsupported diagnostic.
- The dump/CLI helper-contract tests prove the prepared dump and RV64 object
  route accept the repaired representative helper contract.

## Suggested Next

Execute Step 5 by rerunning `tests/c/external/gcc_torture/src/920908-1.c`
through the RV64 object representative and classify whether the second
aggregate `va_arg` now reads `20` and the runtime abort is gone, or whether a
later boundary remains.

## Watchouts

- The repair does not change caller publication, same-module sret emission, or
  generic AAPCS64/non-RV64 aggregate behavior.
- The generic overflow aggregate helper still handles payloads larger than one
  RV64 GPR save-area slot.
- The object-emission guard remains a prepared-fact consumer: it rejects
  malformed aggregate plans where the copy range does not fit the declared
  source slot or the source slot size disagrees with cursor stride.

## Proof

Delegated proof command run:
`{ cmake --build --preset default --target backend_prepared_printer_test backend_riscv_object_emission_test c4cll; ctest --test-dir build -R 'backend_prepared_printer|backend_riscv_object_emission|backend_dump_riscv64_variadic_aggregate_overflow_helper_contract|backend_cli_riscv64_variadic_aggregate_overflow_helper_contract_obj' --output-on-failure; } > test_after.log 2>&1`

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed
out of 4`.

Supervisor acceptance proof:
`ctest --test-dir build -L backend -j --output-on-failure > test_after.log 2>&1`
followed by
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.

Result: PASS. The backend suite remained at 326/326 before and 326/326 after
with no new failures.
