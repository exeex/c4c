Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md
Source Plan Path: plan.md
Current Step ID: 3-4
Current Step Title: Add Focused Backend Coverage / Implement Narrow Lowering Or Diagnostic Route

# Current Packet

## Just Finished

Steps 3 and 4 implemented the RV64 object route for GPR call arguments selected
by `PreparedCallArgumentSourceSelectionKind::FrameSlotAddress`.
`fragment_for_prepared_call` now accepts the guarded frame-slot address
selection and emits `addi dest, sp, selected_materialization_offset`, using the
selected frame-slot address materialization rather than the ordinary scalar
value home. The focused fixture verifies the key packet shape: ordinary value
home at stack offset 48, selected address materialization at stack offset 24,
and emitted `a0 = sp + 24`.

Focused coverage in `backend_riscv_object_emission_test` now covers the success
route, same-stack-slot preservation no-ops around the call, and fail-closed
mutations for missing source facts, value/slot/materialization mismatches,
non-default/TLS materialization facts, dynamic/fp fixed-slot frames, destination
shape mismatches, duplicate conflicting materializations, and out-of-range
offsets. Existing `LocalFrameAddressMaterialization` and `FrameSlotValue` routes
remain separate.

`va-arg-13.c` was rerun through the RV64 gcc-torture object runner. It now gets
past c4c RV64 object compilation; the next boundary is link failure on unresolved
`llvm.va_end.p0` at `.text+0xac` and `.text+0xdc` in `test`. The case log is
`build/agent_state/386_step3_va-arg-13.case.log`.

## Suggested Next

Decide whether the next packet should lower or erase the prepared
`llvm.va_end.p0` RV64 object-route call boundary for `va-arg-13.c`, or split that
as a separate variadic-helper cleanup if it belongs outside idea 386.

## Watchouts

- The new preservation admission is intentionally no-op only: source and
  destination must both be the same prepared frame-slot endpoint. It does not
  add stack-to-stack copy lowering.
- Keep idea 387 out of scope. This packet did not relax the
  `call_plan->memory_return.has_value()` gate for same-module sret calls.
- `va-arg-13.c` now reaches link, so the old unsupported-instruction fragment
  at `test` block 0 inst 9 is no longer the active boundary.

## Proof

Proof command completed successfully and was written to `test_after.log`:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: build succeeded; 326/326 `^backend_` tests passed.

Additional boundary rerun:

- RV64 gcc-torture object runner for `tests/c/external/gcc_torture/src/va-arg-13.c`
- Result: c4c object compile succeeded, then link failed on unresolved
  `llvm.va_end.p0`; see `build/agent_state/386_step3_va-arg-13.case.log`.
