Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Lower Direct And Indirect Calls

# Current Packet

## Just Finished

Completed lifecycle review for Step 3 after discovering a missing prepared
callee-save slot-placement fact.

Decision:

- Narrowed plan Step 3 to the completed simple fixed-frame subset.
- Split the missing prepared callee-save placement fact into
  `ideas/open/241_prepared_callee_save_slot_placement.md`.
- Advanced the active runbook pointer to Step 4, Lower Direct And Indirect
  Calls, so implementation can continue on call lowering without violating the
  active plan's prepared-fact rule.

Reason:

- `PreparedSavedRegister` carries register identity and `save_index`.
- `PreparedFramePlanFunction` carries `saved_callee_registers` and
  `frame_slot_order`.
- `PreparedFrameSlot` carries `PreparedFrameSlotId` and `offset_bytes`.
- No explicit prepared fact maps each saved register to a frame slot and stack
  offset.
- Inferring that mapping in AArch64 codegen from `save_index`,
  `frame_slot_order`, register names, or sorted offsets would recreate
  frame-layout authority in the target backend.

## Suggested Next

Continue with Step 4: lower direct and indirect calls from `PreparedCallPlan`
and call-boundary move bundles. Keep callee-save store/load lowering deferred
until `ideas/open/241_prepared_callee_save_slot_placement.md` is completed or
the supervisor explicitly switches to that dependency.

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

## Proof

Lifecycle-only plan review/split; no build or test proof required for this
packet.
