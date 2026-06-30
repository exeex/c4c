Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Prepared Scalar Call Arguments

# Current Packet

## Just Finished

Step 2 implemented prepared scalar GPR call-result publication for coherent
register-source to register-destination direct-call results in the RV64 object
route.

Completed work:

- `object_emission.cpp` now requires a prepared destination value id, looks up
  the destination object home, requires a register GPR home, checks that the
  prepared home matches the call-result plan destination register, rejects
  floating/non-GPR result payloads, and only then emits the post-call GPR move.
- `backend_riscv_object_emission_test.cpp` now covers the accepted scalar
  same-module call-result path and fail-closed shapes for missing destination
  value id, missing destination home, mismatched destination home, non-GPR
  destination bank, and non-GPR value bank.
- Existing pointer-valued register-to-register call-result movement remains
  accepted; this packet did not add pointer/address publication, frame-slot
  argument support, inline-asm carrier repair, aggregate ABI, or F128 support.

## Suggested Next

Execute Step 3: implement prepared scalar call-argument publication for the
next coherent argument-only packet, preferably frame-slot scalar value to GPR
call argument facts with `missing_frame_slot_arg_publication=yes`, while
leaving frame-slot address arguments, symbol-address arguments, aggregate
`byval`/`sret`, and inline-asm carrier repair out of scope.

## Watchouts

- Register-to-register pointer call results were pre-existing supported
  movement and should not be used as evidence for pointer/address publication
  progress.
- Do not implement inline-asm carrier repair in the first packet; rows with
  `missing_operand0_home` or `tied_input_output_home_mismatch` are not coherent
  prepared inline-asm facts.
- Keep frame-slot scalar call-argument publication, frame-slot address
  arguments, symbol-address arguments, and prior-preservation argument routing
  out of this completed scalar result packet.
- Keep aggregate `sret`/`byval`, F128 helper-call behavior, missing prepared
  metadata reconstruction, pointer/address publication, and
  expectation/accounting changes out of this plan.
- Do not key the route to representative filenames such as `src/pr38533.c`.

## Proof

Proof passed and is saved in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
