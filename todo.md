Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Lower Direct And Indirect Calls

# Current Packet

## Just Finished

Continued Step 4, Lower Direct And Indirect Calls, by lowering the first
prepared call-boundary move family: before-call register-to-register argument
moves.

Implemented the narrow selected register argument move path:

- Added structured source and destination `RegisterOperand` carriers to
  `CallBoundaryMoveInstructionRecord`.
- Selected only `BeforeCall` `CallArgumentAbi` moves whose
  `PreparedMoveResolution`, matching `PreparedAbiBinding`, prepared call
  argument plan, and prepared value home all agree on an explicit GPR
  register-to-register transfer.
- Emitted prepared before-call move nodes before the direct call node in block
  dispatch without deriving ABI placement, stack arguments, scratch registers,
  or local classification.
- Printed selected call-boundary register moves as `mov <dst>, <src>`.
- Kept missing provenance, unsupported phases/destinations/op kinds, missing
  register operands, non-register destinations, and non-GPR argument moves
  fail-closed through node selection/printer diagnostics.
- Added dispatch, target-record, and printer coverage for selected `x2 -> x0`
  argument moves and retained fail-closed coverage for unresolved
  call-boundary move records.

## Suggested Next

Continue Step 4 with the next narrow call-boundary move family, such as
prepared after-call register-to-register result moves, only where prepared move
records, ABI bindings, call-result plans, and value homes already provide
explicit register authority. Keep stack arguments, memory returns,
scratch-mediated indirect callees, and variadic entry work deferred.

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
  printer support is intentionally limited to selected before-call
  register-to-register argument moves.
- Current frame traversal skips zero-byte frames. If later policy needs explicit
  zero-frame records, that should be a separate contract decision rather than
  inferred here.
- Callee-save lowering is blocked on an explicit saved-register-to-frame-slot
  and offset prepared fact. Do not infer it from `save_index` or
  `frame_slot_order`.
- The active source idea cannot be closed as fully complete while callee-save
  proof remains delegated to `ideas/open/241_prepared_callee_save_slot_placement.md`
  unless the supervisor explicitly accepts a narrower completion boundary.
- Indirect calls still reject non-register callees rather than selecting x16/x17
  or any scratch register locally.
- Before-call argument move lowering currently selects only explicit GPR
  register-to-register moves. Stack arguments, stack-to-register loads, FPR
  moves, grouped register spans beyond the plain single-register `mov` subset,
  and cycle-temp moves remain outside this packet.

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
