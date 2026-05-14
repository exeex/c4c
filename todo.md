Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Prepared Frame Facts

# Current Packet

## Just Finished

Started Step 3, Lower Prepared Frame Facts, with simple fixed-frame traversal
emission.

Implemented prepared fixed-frame lowering into structured machine nodes:

- Added traversal helpers that recognize a complete simple fixed frame only
  when `PreparedFramePlanFunction` exists, matches the prepared function, has
  nonzero frame size and alignment, has no dynamic stack plan/flag, and has no
  saved callee-register requirements.
- Inserted a structured `FrameInstructionRecord` prologue setup at the start of
  the first block for that function.
- Inserted structured `FrameInstructionRecord` epilogue teardown nodes before
  lowered return instructions in prepared return blocks.
- Preserved `PreparedFramePlanFunction` provenance, function/block identity,
  frame size, frame alignment, frame-slot order, saved-register list, dynamic
  flag, and frame-pointer flag through the existing frame records.
- Kept dynamic-stack and callee-save frame plans explicitly deferred: traversal
  does not emit fake setup/teardown and does not invent save/restore slots or
  dynamic-stack anchoring.
- Updated focused dispatch/traversal coverage to prove setup/teardown ordering
  around the machine stream and deferred behavior for dynamic-stack/callee-save
  frame plans.

What this slice intentionally did not do:

- It did not lower callee-save stores/loads, frame-pointer setup, dynamic-stack
  anchoring, outgoing call areas, or any call-boundary moves.
- It did not invent frame size, alignment, save slots, stack offsets, scratch
  registers, or ABI placement policy locally.
- It did not add new text-only frame output paths; traversal emits structured
  frame machine nodes that the existing printer can handle for simple fixed
  frames.

## Suggested Next

Continue Step 3 by lowering exactly one additional prepared frame family, most
likely callee-save save/restore records if the prepared slot/offset facts are
available. Keep dynamic-stack anchoring and frame-pointer policy deferred until
their prepared facts are explicitly consumed.

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
- Call-boundary move and ABI-binding records are selected carriers only. Their
  printer rejection is intentional until a later lowering packet owns concrete
  AArch64 move forms.
- Current frame traversal skips zero-byte frames. If later policy needs explicit
  zero-frame records, that should be a separate contract decision rather than
  inferred here.

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
