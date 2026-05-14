Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Lower Memory-Return And Preservation Cases

# Current Packet

## Just Finished

Started Step 5, Lower Memory-Return And Preservation Cases, by reflecting
prepared call clobber records into selected AArch64 call machine-node effects.

Implemented the narrow prepared clobber effect path:

- Populated `InstructionRecord::clobbers` for selected direct and indirect call
  nodes from `CallInstructionRecord::clobbered_registers`, which is copied from
  the authoritative `PreparedCallPlan`.
- Converted only explicit prepared register spellings already present in each
  `PreparedClobberedRegister`; no local caller-saved ABI set, call-clobber
  policy, or placement inference was introduced.
- Preserved raw prepared clobber records in the call payload while exposing
  convertible GPR, FPR, and vector/grouped clobbers as structured register
  machine effects with `CallAbi` register operands.
- Kept malformed or unconvertible clobber facts fail-closed by omitting their
  structured machine clobber effect while leaving the original prepared record
  attached to the call payload for provenance.
- Added target-record, dispatch, and printer coverage for singleton GPR/FPR
  clobbers, a grouped vector clobber, printer stability with clobber effects,
  and malformed clobber omission.

## Suggested Next

Continue Step 5 with the next preservation/memory-return packet. A coherent
next packet would be memory-return or preserved-value machine effects only if
the existing `PreparedCallPlan` facts already provide explicit storage,
slot/offset, and register identity; otherwise leave those cases deferred and
route to the missing prepared-fact source.

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
- Call-boundary move and ABI-binding records are selected carriers only. Move
  printer support is intentionally limited to selected before-call
  register-to-register argument moves and after-call register-to-register
  result moves.
- Current frame traversal skips zero-byte frames. If later policy needs
  explicit zero-frame records, that should be a separate contract decision
  rather than inferred here.
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
- After-call result move lowering currently selects only explicit GPR
  register-to-register moves. Stack results, memory returns, FPR moves, grouped
  register spans beyond the plain single-register `mov` subset, and
  scratch-mediated transfers remain outside this packet.
- Call clobber effects now reflect prepared clobber spellings and occupied
  register names only. The helper intentionally does not use prepared placement
  metadata to remap clobbers, because current clobber placement records can
  describe the reserved-scratch authority identity rather than the physical
  clobbered register spelling.
- Malformed prepared clobber records are not diagnostics today because the call
  instruction record builder has no diagnostics channel; the raw prepared
  records remain in the payload and the structured effect simply fails closed.

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
