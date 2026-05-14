Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Call And Frame Machine Records

# Current Packet

## Just Finished

Completed Step 2, Add Call And Frame Machine Records, first direct-call slice.

Implemented the minimum structured AArch64 call machine-record/provenance
surface:

- Threaded `PreparedFramePlanFunction`, `PreparedDynamicStackPlanFunction`, and
  `PreparedCallPlansFunction` through `FunctionLoweringContext`.
- Added explicit direct/indirect call opcodes and printer mnemonic kinds.
- Extended `CallInstructionRecord` with prepared call provenance:
  direct callee label, wrapper kind, variadic FP argument-register count,
  memory return, prepared arguments, prepared result, preserved values,
  clobbered registers, and `source_call`.
- Added direct-call dispatch plumbing that looks up `PreparedCallPlan` by block
  and instruction index, emits a selected direct-call node for prepared direct
  calls, and records an explicit `MissingPreparedCallPlan` diagnostic when the
  prepared call fact is absent.
- Added direct-call printer support for `bl <prepared-direct-callee-label>`.
  The printer rejects selected call nodes that lack prepared callee provenance.
- Updated focused AArch64 tests for record preservation, fail-closed dispatch,
  and direct-call printer output.

What this slice intentionally did not do:

- It did not place call arguments/results, choose ABI registers, allocate stack
  arguments, choose outgoing call area size, select sret storage, or borrow
  scratch registers.
- It did not implement indirect calls, memory-return lowering, preserved-value
  moves, clobber handling beyond record preservation, or frame prologue/
  epilogue records.
- It did not add richer C++ frame records yet; frame plans are only threaded
  through context for later owned packets.

## Suggested Next

Continue Step 2 with either structured frame/prologue record definitions or
call-boundary move/ABI-binding records. Keep the next packet narrow: consume
prepared movement facts into records before trying to place call arguments or
collect results.

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
