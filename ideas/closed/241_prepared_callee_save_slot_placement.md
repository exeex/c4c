# Prepared Callee-Save Slot Placement

Status: Closed
Created: 2026-05-14
Closed: 2026-05-15
Parent Context: ideas/open/231_aarch64_call_frame_machine_nodes.md

## Intent

Add an explicit prepared-frame fact that maps each saved callee register to
the frame slot and stack offset where that register is saved.

The active AArch64 call/frame route discovered that callee-save save/restore
lowering cannot proceed without this fact. Inferring placement from
`save_index`, `frame_slot_order`, register names, or generic frame-slot offsets
would move ABI and frame-layout authority into AArch64 codegen, which violates
the active route's core rule.

## Why This Exists

Current prepared frame facts expose the ingredients separately:

- `PreparedSavedRegister` carries register identity and `save_index`
- `PreparedFramePlanFunction` carries `saved_callee_registers` and
  `frame_slot_order`
- `PreparedFrameSlot` carries `PreparedFrameSlotId`, object identity,
  `offset_bytes`, size, alignment, and fixed-location state

There is no explicit prepared relationship tying a specific
`PreparedSavedRegister` to a `PreparedFrameSlotId` and stack offset.

AArch64 callee-save lowering needs that relationship before it can emit
structured `CalleeSaveStore` and `CalleeSaveLoad` records.

## Scope

- Define a prepared/shared carrier for callee-save slot placement.
- Connect each saved callee register to a `PreparedFrameSlotId` and concrete
  stack offset.
- Preserve register bank, register name, contiguous width, occupied register
  names, and save/restore ordering as structured facts.
- Update prepared dumps or frame-plan observations so the mapping is visible
  before target lowering consumes it.
- Prove that a later AArch64 machine-node route can consume the mapping without
  deriving layout locally.

## Out Of Scope

- Do not emit AArch64 callee-save stores or loads in this idea unless a later
  active plan explicitly owns target lowering.
- Do not change AArch64 codegen to infer slots from `save_index`,
  `frame_slot_order`, register names, or sorted offsets.
- Do not redesign general frame-slot allocation beyond the callee-save mapping
  needed by prepared frame consumers.
- Do not implement variadic register-save areas or outgoing call stack areas.

## Acceptance Criteria

- For a function with saved callee registers, prepared frame facts expose an
  explicit mapping from each saved register to a frame slot and stack offset.
- The mapping is stable enough for AArch64 lowering to emit matched
  save/restore records without reconstructing frame layout.
- Existing simple fixed-frame cases continue to expose the same frame facts.
- Missing or unsupported callee-save placement states fail with explicit
  diagnostics instead of target-local inference.

## Completion Note

Closed after the active runbook added the prepared saved-register slot
placement surface and direct prepared-printer observations for fixed-frame
callee-save homes. The carrier records slot id, stack offset, size, alignment,
fixed-location state, register bank/name, save index, contiguous width,
occupied registers, and structured register placement. Dynamic-stack cases
remain fail-closed instead of fabricating target-local save homes.

Follow-up AArch64 save/restore lowering should consume
`PreparedSavedRegister::slot_placement` directly and must not reconstruct
callee-save homes from `save_index`, `frame_slot_order`, register names, or
sorted offsets.

## Reviewer Reject Signals

Reject the route if it adds AArch64-side inference from `save_index`,
`frame_slot_order`, register names, or generic slot order; if it weakens
callee-save tests or marks them unsupported to claim progress; if it renames
existing fields without adding the explicit saved-register-to-slot mapping; if
it hides ABI/frame-layout decisions inside target codegen; or if it broadens
into variadic save areas, outgoing call areas, or unrelated frame allocation
rewrites.
