Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Call And Frame Machine Records

# Current Packet

## Just Finished

Continued Step 2, Add Call And Frame Machine Records, with the call-boundary
move/ABI-binding record-carrier slice.

Implemented structured AArch64 call-boundary carriers without emitting argument
or result moves from traversal:

- Added an explicit `CallBoundary` instruction family and opcodes for
  `CallBoundaryMove` and `CallBoundaryAbiBinding`.
- Added `CallBoundaryMoveInstructionRecord` to preserve
  `PreparedMoveBundle`/`PreparedMoveResolution` facts: function, phase,
  authority, block/instruction indexes, parallel-copy predecessor/successor
  labels, the prepared move snapshot, and source bundle/move provenance.
- Added `CallBoundaryAbiBindingInstructionRecord` to preserve
  `PreparedMoveBundle`/`PreparedAbiBinding` facts: function, phase, authority,
  block/instruction indexes, binding index, parallel-copy labels, prepared ABI
  binding snapshot, and source bundle/binding provenance.
- Added construction helpers with fail-closed selection diagnostics when
  prepared move or binding provenance is missing.
- Added printer diagnostics that reject selected call-boundary move and ABI
  binding records until a later packet owns concrete AArch64 move lowering.
- Updated focused AArch64 tests for record preservation and fail-closed printer
  diagnostics for selected and missing-provenance call-boundary records.

What this slice intentionally did not do:

- It did not emit argument/result moves, call-boundary copies, or ABI-binding
  text from traversal/dispatch.
- It did not choose registers, stack slots, scratch registers, stack argument
  areas, sret storage, or any concrete AArch64 move form locally.
- It did not turn call-boundary metadata into printable `mov`/`ldr`/`str`
  instructions; the printer rejects these records until lowering owns the
  prepared source/destination facts.

## Suggested Next

Continue Step 2 by either wiring prepared frame/call-boundary facts into the
machine-node stream as placeholders, or split into Step 3 lowering for one
concrete family. Keep the next packet narrow: do not lower printable moves until
the prepared record source/destination categories and scratch authority are
explicitly consumed.

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
