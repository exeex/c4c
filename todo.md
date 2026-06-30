Status: Active
Source Idea Path: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Lower Prepared Scalar Inline-Asm Fragments

# Current Packet

## Just Finished

Step 3 verified the prepared scalar call-argument publication packet for
frame-slot scalar value to GPR call arguments marked by coherent
`FrameSlotValue` selection facts, including the `missing_frame_slot_arg_publication`
shape.

Completed work:

- Existing `object_emission.cpp` code routes
  `PreparedStorageEncodingKind::FrameSlot` arguments through
  `prepared_frame_slot_call_argument_offset`, verifies the prepared
  `FrameSlotValue` source selection contract, confirms the source value id,
  source stack-slot home, slot id, stack offset, scalar size/alignment, and ABI
  GPR destination register, then emits a stack load into the destination GPR.
- Existing focused backend coverage proves
  `make_prepared_frame_slot_value_arg_call_module()` builds an object with the
  frame-slot payload reloaded into `a0` before the call.
- Existing fail-closed coverage rejects frame-slot address selection,
  missing/incoherent source slot homes, non-GPR destination bank, non-scalar
  floating argument type, impossible alignment, mismatched stack offsets, and
  unsupported stack-frame offsets.
- No new code changes were required for this packet; no frame-slot address
  argument, symbol-address argument, prior-preservation argument, aggregate
  `byval`/`sret`, inline-asm carrier repair, F128, or expectation/accounting
  behavior was added.

## Suggested Next

Execute Step 4: classify and implement only coherent prepared scalar inline-asm
input/output materialization, starting with rows whose prepared inline-asm
carrier facts are complete. Keep rows with `missing_operand0_home` or
`tied_input_output_home_mismatch` out of the implementation packet unless a
separate producer/carrier repair packet is selected.

## Watchouts

- Step 3 did not add new code; review should treat the existing object route
  and focused backend tests as the proof for the selected frame-slot scalar
  argument packet.
- Register-to-register pointer call results were pre-existing supported
  movement and should not be used as evidence for pointer/address publication
  progress.
- Do not implement inline-asm carrier repair in the next packet; rows with
  `missing_operand0_home` or `tied_input_output_home_mismatch` are not coherent
  prepared inline-asm facts.
- Keep frame-slot address arguments, symbol-address arguments, and
  prior-preservation argument routing out of this completed scalar argument
  packet.
- Keep aggregate `sret`/`byval`, F128 helper-call behavior, missing prepared
  metadata reconstruction, pointer/address publication, and
  expectation/accounting changes out of this plan.
- Do not key the route to representative filenames such as `src/pr38533.c`.

## Proof

Proof passed and is saved in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
