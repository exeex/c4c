Status: Active
Source Idea Path: ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Existing Call Return Frame Assumptions

# Current Packet

## Just Finished

Step 1 inspected the existing AArch64 call, return, frame, prologue, ABI, and
ledger assumptions without implementation or contract edits.

Concrete contract targets for Step 2:

- Define `module::CallRecord` as the pre-selection call-boundary snapshot over
  `PreparedCallPlan`: direct callee, indirect callee, wrapper kind, arguments,
  result, memory return, preserved live-across-call values, and clobbers.
- Define `module::AbiBindingRecord` and `module::MoveRecord` as the movement
  authority for argument/result/return-register placement, including
  `BeforeCall`, `AfterCall`, and `BeforeReturn` phases.
- Define `module::FrameRecord`, `FrameSlotRecord`, `CalleeSaveRecord`, and
  `DynamicStackRecord` as frame/prologue/epilogue input snapshots only; concrete
  stack adjustment, save/restore instructions, and final frame materialization
  remain later machine-node work.
- Define `module::CallPreservedValueRecord` and call clobber records as the
  caller-saved/callee-save handoff. They should consume prepared preservation
  routes and prepared clobbered registers, not let call lowering choose local
  save slots or scratch registers.
- Define a return-control target-MIR responsibility. Current `BlockRecord`
  only preserves BIR terminator kind/labels, and `codegen::ReturnInstructionRecord`
  is a generic record-only payload; there is no dedicated AArch64 return-boundary
  record that owns `x30`, return side effects, or before-return ABI movement.

Stale or legacy assumptions found:

- `codegen/calls.md`, `returns.md`, `prologue.md`, and `variadic.md` still
  preserve old `ArmCodegen` behavior as legacy reference material. They already
  warn that `bl`/`blr`, `ret`, stack adjustment, stack-relative stores, helper
  calls, and register spelling are printer/selection details, not current
  backend authority.
- `codegen/calls.md` records legacy outgoing stack-area layout, indirect
  function-pointer spill-at-stack-argument-area behavior, F128 helper fallback,
  and zero fallbacks for missing operands. Step 2 should keep these as risks or
  deferrals, not as accepted target-MIR behavior.
- `codegen/prologue.md` records legacy parameter storage, variadic save-area
  offsets, callee-save save order, and `current_return_type` state. These are
  policy candidates for the contract, but current authoritative carriers are
  prepared frame/call/move facts plus AArch64 module snapshots.
- `codegen/returns.md` records legacy return-register assumptions for `x0:x1`,
  `s0/d0/q0`, second FP components, and helper calls. Current target-MIR should
  express return value movement through prepared moves/ABI bindings and defer
  final helper-call or instruction spelling.

Sufficient prepared and AArch64 carriers:

- `PreparedCallPlans` carries wrapper kind, direct/indirect callee, memory
  return, argument plans, result plan, preserved values, clobbered registers,
  and `variadic_fpr_arg_register_count`.
- `PreparedCallArgumentPlan` carries source value/base/literal/symbol/slot,
  source stack offset, destination register/stack offset, bank, and occupied
  register metadata.
- `PreparedCallResultPlan` carries source/destination storage kind, result
  value id, source register/stack offset, destination register/slot/stack
  offset, bank, width, and occupied-register metadata.
- `PreparedMemoryReturnPlan` carries sret argument index, storage slot name,
  encoding, slot id, prepared stack offset, size, and alignment.
- `PreparedCallPreservedValue` and `PreparedClobberedRegister` carry
  live-across-call preservation route, callee-save index or stack slot, register
  name/bank, width, occupied registers, and call-clobber registers.
- `PreparedFramePlan`, `PreparedSavedRegister`, `PreparedStackLayout`,
  `PreparedDynamicStackPlan`, allocation records, value locations, move bundles,
  ABI bindings, spill/reload ops, and storage plans are present and already
  snapshotted by AArch64 module records.

Special-register assumptions to make explicit in Step 2:

- `x8` is represented in ABI helpers as `sret_register()` and is classified as
  special/forbidden for ordinary long-lived allocation. The contract still
  needs to bind `PreparedMemoryReturnPlan`/ABI binding authority to the `x8`
  indirect result-location role.
- `x16` and `x17` are exposed as `indirect_call_scratch_registers()` and are
  special/forbidden ordinary homes. The contract needs to state whether target
  MIR records can name their indirect-call, PLT, veneer, or linker-sensitive
  role, because current call records only identify the indirect callee value and
  do not carry an explicit IP scratch role.
- `x29` is exposed as `frame_pointer_register()` and is only allocatable when
  frame-pointer reservation is not in force. `FrameRecord` carries
  `uses_frame_pointer_for_fixed_slots`, but the contract needs to define how
  that fact reserves `x29` for prologue/epilogue and frame-slot addressing.
- `x30` is exposed as `link_register()`, included in caller-saved GPRs, and
  classified special/forbidden for ordinary homes. The contract needs to make
  link-register return control/clobbering explicit for call and return records.

Missing prepared-carrier candidates for separate ideas if Step 2 cannot defer
them cleanly:

- Dedicated return-boundary carrier for AArch64 return control, `x30` effects,
  return side effects, and return-value ABI resources beyond generic
  `BeforeReturn` moves.
- Explicit outgoing call-area carrier: total call-time stack adjustment,
  per-argument outgoing stack slot layout, 16-byte alignment for wide arguments,
  and ownership of any indirect-call function-pointer spill area.
- Explicit target ABI classification carrier for parameter entry and call
  arguments/results if existing prepared call plans are not precise enough for
  stack-vs-register, aggregate, HFA/F128/i128, and memory-return cases.
- Explicit variadic function-entry carrier for the AAPCS64 save-area layout:
  `va_list` fields, GP/FP save offsets, named GP/FP counts, named stack bytes,
  and whether FP/SIMD variadic save usage is disabled.
- Explicit special-register role carrier for `x8`, `x16`, `x17`, `x29`, and
  `x30` if the contract needs machine-verifiable role records instead of using
  ABI helper classification plus prepared call/frame facts.
- Explicit runtime-helper call resource carrier for F128/soft-float paths if
  those paths are accepted rather than deferred; legacy docs mention
  `__extenddftf2` and `__trunctfdf2`, but current call records should not infer
  helper-call clobbers from text.

## Suggested Next

Proceed to Step 2 with a docs-only packet that drafts the AAPCS64 target-MIR
contract under `src/backend/mir/aarch64/`, using the carriers above and
explicitly deferring or splitting the missing-carrier candidates.

## Watchouts

- Do not start call, return, or prologue instruction selection under this plan.
- Do not treat legacy assembly-text emitter behavior as current backend
  authority.
- Keep `x8`, `x16`, `x17`, `x29`, and `x30` special-register roles explicit.
- Split missing prepared carriers into separate open ideas instead of inventing
  AArch64-local facts.
- The existing AArch64 module records snapshot prepared stack offsets in several
  places; Step 2 should call those prepared snapshots, not final frame-layout
  authority.
- `CallRecord` does not currently copy
  `PreparedCallPlan::variadic_fpr_arg_register_count`; if the contract wants
  that field observable in target MIR rather than only through `source_call`,
  Step 4 may need a narrow record addition.

## Proof

Inspection-only Step 1. No build required and no `test_after.log` produced per
the delegated proof.
