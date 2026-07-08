Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broaden Within ABI Consumer Authority

# Current Packet

## Just Finished

Completed the Step 4 scalar GPR same-module call/result consumer packet in
RV64 object emission.

The object route now treats explicit scalar GPR source storage as authoritative
when address-provenance selections are also present:

- Register sources with `local_frame_address_materialization` provenance now
  move the prepared source register to `a0`-`a7` instead of incorrectly trying
  to materialize a frame address.
- Frame-slot sources with `frame_slot_address` provenance still use the
  existing address-publication route when publication facts exist; otherwise a
  new fail-closed explicit scalar frame-slot helper loads only when the prepared
  source value id, source slot id, stack offset, GPR bank, stack home, slot
  bounds, scalar width, and destination register facts all agree.
- Existing scalar GPR result publication from `register:a0` to prepared
  destination registers and non-pointer stack slots remained covered by the
  prior consumer path.

Focused probes after the change:

- `src/20001017-2.c` now compiles through `--codegen obj`.
- `src/20010118-1.c` now compiles through `--codegen obj`.
- `src/20001101.c` moved past `unsupported_call_abi` to
  `unsupported_terminator_fragment`.
- `src/20040625-1.c` moved past `unsupported_call_abi` to a downstream
  `unsupported_move_bundle_target_shape` register/pointer-base stack
  destination.
- `src/20030715-1.c` still stops at `unsupported_call_abi`; the remaining
  shape is pointer call result to prepared stack slot, and removing that guard
  broke an existing fail-closed test, so it was left out of this packet.

Focused tests added coverage for register and frame-slot scalar sources that
carry address-provenance selections without weakening true frame-slot address
publication handling.

## Suggested Next

Refresh Step 4 residuals after the scalar GPR argument/source-storage slice.
Classify whether any remaining same-authority ABI consumer packet exists, or
advance to Step 5 residual split/close-readiness if remaining rows are pointer
stack-result policy, producer authority, FPR/frame policy, aggregate outgoing
stack transport, generic move-bundle, local/global, runtime/library/variadic,
or terminator/instruction-fragment owners.

## Watchouts

- `src/20000808-1.c` remains under idea 624 at `unsupported_call_abi`.
- `src/20020529-1.c` remains under idea 625 at `unsupported_call_abi`.
- `src/20040811-1.c` remains under idea 626 at `unsupported_stack_frame`.
- `src/20001130-2.c` and `src/20080719-1.c` remain return destination-home
  authority gaps under `return_stack_to_register` move-bundle ownership.
- `src/20021219-1.c` remains downstream
  `malformed_prepared_join_transfer_carrier`; `src/pr77767.c` still compiles.
- Do not infer pointer stack-result policy from `src/20030715-1.c`; it needs a
  separate decision because the existing pointer stack-result guard is covered
  by a fail-closed object-emission test.

## Proof

Focused proof:

`cmake --build --preset default --target c4cll backend_riscv_object_emission_test && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`

Result: passed.

Supervisor-delegated proof ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests. `test_after.log` reports
`100% tests passed, 0 tests failed out of 346`.
