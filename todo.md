Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Call And Frame Machine Records

# Current Packet

## Just Finished

Continued Step 2, Add Call And Frame Machine Records, with the frame/prologue/
epilogue record-carrier slice.

Implemented structured AArch64 frame machine-record/provenance surface without
emitting frame setup from traversal:

- Added explicit frame instruction family, frame instruction kinds, frame setup/
  teardown and callee-save opcodes, frame setup/teardown side-effect names, and
  central mnemonic mapping for simple setup/teardown.
- Added `FrameInstructionRecord` and `CalleeSaveInstructionRecord` carriers that
  preserve prepared frame facts from `PreparedFramePlanFunction`: function
  identity, frame size/alignment, frame-slot order, saved callee-register list,
  dynamic-stack flag, frame-pointer fixed-slot flag, callee-save slot/offset
  snapshot, and `source_frame` provenance.
- Added `make_frame_instruction` with fail-closed selection diagnostics for
  missing prepared frame facts and incomplete callee-save records.
- Added conservative printer support for only complete simple fixed-frame
  setup/teardown records with no dynamic stack and no callee-save records:
  `sub sp, sp, #N` and `add sp, sp, #N`.
- Added fail-closed printer diagnostics for missing prepared frame provenance,
  missing alignment, dynamic-stack frames, callee-save frames, unprintable frame
  kinds, and frame adjustments outside the current plain immediate subset.
- Updated focused AArch64 tests for frame record preservation, central mnemonic
  coverage, simple frame printer output, and fail-closed frame diagnostics.

What this slice intentionally did not do:

- It did not emit frame setup/teardown from traversal or dispatch.
- It did not invent frame size, callee-save slots, frame-pointer policy,
  dynamic-stack anchoring, ABI register choices, scratch-register choices, or
  save/restore instruction forms inside AArch64 lowering.
- It did not implement printable callee-save stores/loads yet; those records are
  structured carriers and printer-fail closed until the later lowering packet
  owns those facts.

## Suggested Next

Continue Step 2 by wiring prepared frame facts into dispatch/traversal only as
records if the supervisor wants the machine-node stream to carry prologue/
epilogue placeholders, or move to the next call-boundary record slice. Keep the
next packet narrow and do not lower callee-save save/restore text until the
prepared slot/offset ownership is explicit.

## Watchouts

- Do not reconstruct call ABI classification, frame layout, outgoing stack
  areas, callee-save slots, sret storage, or scratch policy inside AArch64
  codegen.
- Do not implement full variadic function-entry `va_list` behavior in this
  route.
- Reject named-case call/frame shortcuts, weakened expectations, or text-only
  printer patches as progress.
- The contract markdown mentions richer `module::CallRecord`/`FrameRecord`
  surfaces, but the current C++ implementation does not define those records.
  Treat that as an implementation gap, not permission to infer facts locally.
- Direct-call printer output currently uses the prepared direct-callee label.
  Relocation/linker/object behavior remains outside this packet.
- Missing prepared call facts now fail through `MissingPreparedCallPlan`; do not
  reopen a fallback that derives call ABI shape from retained BIR alone.
- Simple fixed-frame printer output is intentionally narrow and only covers
  complete no-save/no-dynamic records. Callee-save and dynamic-stack records
  preserve prepared facts but fail closed in the printer.

## Proof

Proof log: `test_after.log`.

Command run exactly:

`(cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_prepare_frame_stack_call_contract_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_frame_stack_call_contract)$') > test_after.log 2>&1`

Result: green. The build completed and CTest reported `100% tests passed, 0
tests failed out of 4` for:

- `backend_aarch64_instruction_dispatch`
- `backend_aarch64_target_instruction_records`
- `backend_aarch64_machine_printer`
- `backend_prepare_frame_stack_call_contract`
