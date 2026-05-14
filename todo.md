Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Prepared Frame Facts

# Current Packet

## Just Finished

Continued Step 3, Lower Prepared Frame Facts, by inspecting whether callee-save
save/restore lowering has explicit prepared slot and offset facts.

No callee-save lowering code was added in this packet because the required
prepared fact is missing:

- `PreparedSavedRegister` currently carries the saved register identity
  (`bank`, `register_name`, `contiguous_width`, `occupied_register_names`,
  `save_index`) plus optional register placement.
- `PreparedFramePlanFunction` carries `saved_callee_registers` and a general
  `frame_slot_order`.
- `PreparedFrameSlot` carries `slot_id`, `object_id`, `function_name`,
  `offset_bytes`, size, alignment, and fixed-location state.
- There is no explicit prepared mapping from each `PreparedSavedRegister` to a
  `PreparedFrameSlotId` and stack offset, and `PreparedFrameSlot` is not tagged
  as a callee-save slot for a specific saved register.
- Lowering callee-save stores/loads from `save_index`, `frame_slot_order`, or
  generic frame-slot offsets would invent ABI/frame-layout policy locally, so
  this packet leaves callee-save lowering deferred.

What this slice intentionally did not do:

- It did not derive saved-register frame slots from ordering or register names.
- It did not add testcase-shaped save/restore records or rendered-string
  payloads.
- It did not change dynamic-stack anchoring, frame-pointer policy, outgoing call
  areas, or call-boundary moves.

## Suggested Next

Add a prepared callee-save slot-placement fact before lowering save/restore
records. The narrow source-side shape should explicitly connect each saved
register to a frame slot and offset, for example in `PreparedSavedRegister` or a
dedicated frame-plan child record. After that, traversal can emit structured
`CalleeSaveStore` and `CalleeSaveLoad` records without inventing layout.

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

## Proof

Proof log: `test_after.log`.

Command run exactly:

`(cmake --build build --target backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_prepare_frame_stack_call_contract_test -j2 && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_prepare_frame_stack_call_contract)$') > test_after.log 2>&1`

Result: not rerun in this packet because no code changed. The existing
`test_after.log` remains from the prior Step 3 proof and was not refreshed.
