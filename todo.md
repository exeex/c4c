Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Lower Direct And Indirect Calls

# Current Packet

## Just Finished

Started Step 4, Lower Direct And Indirect Calls, by adding the first prepared
indirect-call lowering slice.

Implemented the narrow prepared-register indirect-call path:

- Generalized call dispatch from direct-only to direct-or-indirect while keeping
  the existing missing-plan and direct-call behavior.
- Lowered only prepared indirect calls whose retained BIR call is also indirect
  and whose `PreparedIndirectCalleePlan` explicitly has
  `encoding == Register`, `bank == Gpr`, and a concrete `register_name`.
- Converted the prepared callee register through AArch64 register conversion
  with expected X-register view, then preserved it as a structured
  `RegisterOperand` with `CallAbi` role.
- Preserved `PreparedIndirectCalleePlan` on `CallInstructionRecord` for
  provenance.
- Printed selected indirect calls as `blr <prepared-register>` through the
  structured operand.
- Kept stack-slot, frame-slot, immediate, pointer-base, missing-register,
  direct/indirect mismatch, and register-conversion failure paths fail-closed.
- Added focused dispatch and printer tests for successful `blr x9`, prepared
  provenance preservation, and stack/slot indirect callee rejection without
  scratch selection.

## Suggested Next

Continue Step 4 with one narrow call-boundary move family, such as prepared
register-to-register argument moves, only where `PreparedMoveResolution` and
`PreparedAbiBinding` already provide explicit source/destination register
authority. Keep stack arguments, memory returns, scratch-mediated indirect
callees, and variadic entry work deferred.

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
- Callee-save lowering is blocked on an explicit saved-register-to-frame-slot
  and offset prepared fact. Do not infer it from `save_index` or
  `frame_slot_order`.
- The active source idea cannot be closed as fully complete while callee-save
  proof remains delegated to `ideas/open/241_prepared_callee_save_slot_placement.md`
  unless the supervisor explicitly accepts a narrower completion boundary.
- Indirect calls still reject non-register callees rather than selecting x16/x17
  or any scratch register locally.

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
