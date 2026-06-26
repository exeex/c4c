Status: Active
Source Idea Path: ideas/open/377_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Focused Backend Coverage

# Current Packet

## Just Finished

Steps 2-3: Implement Focused Instruction Lowering and Prove Focused Backend
Coverage completed for scalar integer call-result publication from a prepared
ABI result register to a prepared stack-slot home.

`fragment_for_prepared_call()` now admits the focused result shape when
prepared metadata shows a one-lane GPR source register, a stack-slot
destination, a destination value id, matching destination slot/offset facts, a
matching prepared stack-slot value home, and a supported scalar integer result
size. The emitted publication stores the ABI result register to the prepared
stack offset after the call pair.

Focused backend tests cover an admitted `i16` result publication from `a0` to
stack slot offset `4`, and fail-closed adjacent shapes for missing source
register, FPR/source-bank mismatch, multi-lane destination width, missing
destination slot, destination offset drift, destination home drift, non-integer
result type, pointer-typed result publication with otherwise matching 8-byte
stack-slot metadata, and unsupported destination storage.

## Suggested Next

Step 4: rerun the single `src/20000217-1.c` RV64 gcc-torture backend object
representative with the supervisor-selected allowlist/stop-on-failure command,
then record whether the prior call-result publication blocker is gone and
whether the representative passes or advances to a distinct next owner.

## Watchouts

- The admitted lowering is metadata-driven; it does not key on function names,
  value ids, frame-slot ids, concrete registers beyond prepared ABI/source
  metadata, instruction indexes, source syntax, or prepared-BIR text.
- FPR/non-integer, pointer-typed result publication, non-stack destination,
  missing home, mismatched home/plan slot facts, offset drift, and multi-lane
  result plans remain rejected by the object route.
- The supported fixture keeps callee arithmetic on an already-supported i32
  path and isolates the caller-side `i16` stack-result publication.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Focused subset used: `backend_prepare_frame_stack_call_contract`,
`backend_prepared_printer`, `backend_riscv_object_emission`.
