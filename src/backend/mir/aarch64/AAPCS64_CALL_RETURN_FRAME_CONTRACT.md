# AArch64 AAPCS64 Call Return Frame Contract

This contract defines the AAPCS64 target-MIR responsibilities for call
boundaries, return boundaries, frame obligations, and prologue/epilogue inputs.
It is a pre-instruction-selection contract over prepared facts and AArch64
module records.

## Pipeline Position

The accepted lowering path is:

```text
PreparedBirModule
  -> AArch64 target MIR call, return, frame, move, ABI-binding, and allocation records
  -> later AArch64 machine instruction nodes
  -> optional printer, encoder, object, and linker consumers
```

`PreparedBirModule` owns semantic and prepared backend facts. AArch64 target
MIR records snapshot those facts with target register, stack, call, and frame
roles made explicit. Later machine instruction nodes may select concrete
operations, but this contract does not select final call, return, prologue,
epilogue, stack-adjust, save/restore, branch, or memory instructions.

Legacy markdown under `codegen/` may contain historical AAPCS64 examples. It is
not current authority for selecting registers, stack slots, outgoing call
areas, variadic save areas, helper calls, or final instruction spelling.

## Authoritative Carriers

The contract is defined over these prepared and target-MIR surfaces:

- `PreparedCallPlan`: call site identity, wrapper kind, direct or indirect
  callee facts, memory-return facts, argument plans, result plan, preserved
  values, clobbered registers, and variadic FP argument-register count.
- `PreparedCallArgumentPlan`: source value/base/literal/symbol/slot facts,
  source stack snapshot, destination register or destination stack snapshot,
  register bank, contiguous width, and occupied-register metadata.
- `PreparedCallResultPlan`: source and destination storage kinds, result value
  identity, source register or stack snapshot, destination register, slot, or
  stack snapshot, register bank, contiguous width, and occupied-register
  metadata.
- `PreparedMemoryReturnPlan`: sret argument index, storage slot name, storage
  encoding, prepared frame slot id, prepared stack snapshot, size, and
  alignment.
- `PreparedCallPreservedValue` and `PreparedClobberedRegister`: live-across-call
  preservation routes, callee-save indexes, stack slots, physical register
  names, banks, widths, occupied registers, and clobber resources.
- `PreparedMoveBundle`, `PreparedMoveResolution`, and `PreparedAbiBinding`:
  movement authority for `BeforeCall`, `AfterCall`, and `BeforeReturn` phases.
- `PreparedFramePlan`, `PreparedStackLayout`, `PreparedSavedRegister`, and
  `PreparedDynamicStackPlan`: frame size and alignment snapshots, frame slots,
  callee-save obligations, fixed-slot frame-pointer policy, and dynamic-stack
  operations.
- AArch64 `module::CallRecord`, `CallArgumentRecord`, `CallResultRecord`,
  `CallPreservedValueRecord`, `FrameRecord`, `FrameSlotRecord`,
  `CalleeSaveRecord`, `DynamicStackRecord`, `MoveRecord`, `AbiBindingRecord`,
  `SpillReloadRecord`, `TargetRegisterRecord`, and `BlockRecord`: target-MIR
  snapshots with prepared provenance.
- `abi::RegisterReference` and ABI helper functions for typed register roles,
  special-register classification, caller-saved/callee-saved pools, reserved
  MIR scratch pools, and AAPCS64 handoff validation.

If a later lowering step needs a fact not present in these carriers, it must
fail closed or open a separate source idea. It must not infer ABI policy from
assembly text, register spellings alone, legacy examples, or local testcase
shape.

## Direct Calls

Direct calls are target-MIR call boundaries backed by a `PreparedCallPlan` with
`wrapper_kind` set to `SameModule`, `DirectExternFixedArity`, or
`DirectExternVariadic`, and `is_indirect` false.

`module::CallRecord` owns the direct-call snapshot before instruction
selection. It must preserve:

- the owning block and instruction indexes
- the direct callee display label from the prepared plan
- wrapper kind, including same-module, fixed-arity extern, and variadic extern
  distinction
- all argument, result, memory-return, preserved-value, and clobber records
- the source prepared call pointer as provenance

Target MIR may state that a later selected node will perform a call transfer,
but it must not choose the concrete call instruction, relocation form,
symbol-operand encoding, PLT route, or printer spelling in this contract.

## Indirect Calls

Indirect calls are target-MIR call boundaries backed by a `PreparedCallPlan`
with `is_indirect` true and an `indirect_callee` plan.

`module::CallRecord` owns the indirect-call snapshot. The indirect callee value
identity, storage encoding, prepared value id, source register, frame slot,
stack snapshot, immediate, pointer base value, and pointer byte delta remain
prepared-plan facts. Current `CallRecord` directly preserves indirect callee
value identity; instruction selection must consult `source_call` for the full
prepared indirect-callee carrier until a narrower target-MIR record field is
added.

`x16` and `x17` are intra-procedure-call scratch registers with PLT, veneer, and
linker-sensitive roles. They are not ordinary long-lived homes. Target MIR may
reserve or name them only when the prepared call or a later accepted
linker/veneer carrier authorizes that role. A call-lowering slice must not
borrow `x16` or `x17` as generic scratch for unrelated materialization.

The exact indirect-call register, branch-through-register operation, PLT entry
shape, veneer sequence, GOT load, and relocation behavior are later
machine-node, linker, or object-layer decisions.

## Arguments

Arguments are placed by prepared argument plans and ABI movement records, not
by local call lowering.

For each argument, `module::CallArgumentRecord` preserves:

- source value identity, base value identity, literal, symbol, frame slot, or
  stack snapshot
- source storage encoding and source bank
- destination register name, destination register bank, contiguous width, and
  occupied registers when the argument is register-passed
- destination stack snapshot when the argument is stack-passed
- source prepared argument provenance

`module::MoveRecord` and `module::AbiBindingRecord` with
`PreparedMovePhase::BeforeCall` are the movement authority that places values
at the call boundary. They may describe value-to-ABI moves, stack destinations,
coalesced assigned storage, cycle-temp use, and structured destination facts.

Target MIR must preserve the prepared distinction between register arguments
and stack arguments. It must not reclassify aggregate, integer, floating,
vector, or pointer arguments locally. If the prepared classification is
insufficient for a case such as HFA, F128, i128, aggregate splitting, or
stack/register boundary behavior, that is a shared prepared-carrier gap.

## Returns And Result Bindings

Call results are consumed by prepared result plans and `AfterCall` movement
records. Function returns are produced by `BeforeReturn` movement and ABI
binding records plus BIR return terminators.

For a call result, `module::CallResultRecord` preserves:

- result value bank
- source and destination storage kinds
- destination prepared value id
- source register or stack snapshot
- destination register, frame slot, or stack snapshot
- contiguous width, occupied registers, and prepared provenance

For a function return, `module::MoveRecord` and `module::AbiBindingRecord` with
`PreparedMovePhase::BeforeReturn` are the authority for placing return payloads
in ABI resources before the return-control boundary. `module::BlockRecord`
preserves the BIR return terminator kind and source block identity. There is no
dedicated AArch64 return-boundary record today; until one is added, return
control is represented by the return terminator plus `BeforeReturn` moves and
ABI bindings.

Return lowering must not choose concrete return-register policy from legacy
text. It must consume prepared `BeforeReturn` movement. Missing explicit
return-boundary metadata for return side effects, link-register effects, or
multi-register return resources is a deferred carrier candidate.

## Memory Returns And `x8`

AAPCS64 indirect result-location ownership is represented by
`PreparedMemoryReturnPlan` and the ABI helper `sret_register()`, which is `x8`.
`x8` is special for this role and is not an ordinary long-lived allocation
resource while the memory-return role is in force.

`module::CallRecord` preserves memory-return slot id and prepared stack
snapshot when present, and retains the source prepared call for the complete
memory-return carrier: sret argument index, storage slot name, storage
encoding, frame slot id, stack snapshot, size, and alignment.

Target MIR must distinguish:

- the hidden memory-return location pointer carried through `x8`
- ordinary GPR arguments carried through `x0` through `x7`
- the destination storage slot that receives the returned aggregate
- any prepared move or ABI binding that materializes the hidden pointer

The contract does not select the final move into `x8`, a store to the returned
object, or a helper-call sequence. If an implementation needs an observable
target-MIR `x8` role record beyond the prepared memory-return plan and ABI
binding, that is a narrow record-surface or separate-idea candidate.

## Caller-Saved Clobbers

Call clobbering is represented by `PreparedClobberedRegister` and snapshotted
as `module::CalleeSaveRecord` entries inside `module::CallRecord`'s
`clobbered_registers` vector. Despite the record type name, those call entries
are call-clobber facts sourced from prepared clobbered registers, not prologue
save obligations.

Caller-saved resources include AAPCS64 caller-saved GPR and FP/SIMD pools from
`abi::caller_saved_gp_registers()` and
`abi::caller_saved_fp_simd_registers()`, subject to special-role exclusions
from `ALLOCATION_CONTRACT.md`. `x30` is caller-saved and link-register-owned;
`x16` and `x17` are caller-saved but IP/linker-sensitive; `x8` is
caller-saved but may be memory-return-owned.

A value live across a call may remain in a caller-saved physical register only
when prepared preservation facts record a valid route. Target MIR must consume
`module::CallPreservedValueRecord` for callee-saved-register and stack-slot
preservation. It must not invent temporary save slots or choose scratch
registers locally.

## Callee-Save And Frame Obligations

Function-level callee-save obligations are represented by prepared saved
registers and snapshotted as `module::FrameRecord::callee_saves`. They are
frame/prologue/epilogue inputs, not selected save/restore instructions.

`module::FrameRecord` owns:

- prepared frame size and frame alignment snapshots
- whether dynamic stack operations exist
- whether stack save/restore operations are required
- whether fixed slots require a frame pointer
- frame slot order
- frame slots with prepared slot ids, object ids, size, alignment, fixed-location
  policy, address exposure, home-slot policy, and prepared offsets
- callee-save records with bank, register name, contiguous width,
  occupied-register metadata, save index, and prepared provenance
- dynamic-stack records with block/instruction identity and prepared operation
  kind

Target MIR must preserve these obligations for later prologue and epilogue
selection. It must not compute final stack adjustments, concrete save/restore
instructions, red-zone-like policy, local stack reordering, or frame material
layout from legacy text.

## Stack Arguments, Alignment, And Outgoing Call Area

Stack argument facts are prepared call facts. `CallArgumentRecord` records a
destination stack snapshot when the prepared plan places an argument on the
call stack. Source stack snapshots describe where an argument source currently
lives; destination stack snapshots describe the outgoing argument location.

Prepared frame and stack records own function-frame snapshots. They do not by
themselves define a complete outgoing call-area object. Current prepared call
plans can describe per-argument destination stack offsets, but they do not
publish a dedicated total outgoing call-area carrier, call-time stack
adjustment, or per-call alignment object.

The accepted minimum contract is:

- preserve per-argument destination stack snapshots exactly as prepared
- preserve prepared frame alignment and frame size snapshots
- keep stack offsets marked as prepared snapshots, not final instruction
  addressing authority
- require later machine-node/frame work to fail closed if total outgoing area,
  call-time stack adjustment, or 16-byte stack-alignment proof is needed but not
  present as a structured prepared fact

Adding an explicit outgoing call-area carrier is a deferred separate-idea
candidate.

## Return Control And `x30`

`x30` is the AAPCS64 link register and is special/forbidden for ordinary
long-lived allocation while the link-register role is in force. Calls clobber
or define return-control state through `x30`; returns consume that state.

Current target MIR has no dedicated return-control record. The accepted
pre-node representation is:

- call boundaries preserve call clobber facts, including `x30` when prepared
  clobbers include it
- return blocks preserve BIR return terminator identity through
  `module::BlockRecord`
- return payload placement is represented by `BeforeReturn` moves and ABI
  bindings
- ABI helper `link_register()` identifies the typed `x30` role

Later instruction selection may turn this boundary into selected return-control
nodes. This contract does not select the concrete return instruction, default
link-register operand, epilogue branch, or tail-call route. A dedicated
return-boundary carrier for `x30` side effects and return-control provenance is
a deferred record-surface candidate.

## Frame Pointer Policy And `x29`

`x29` is the AAPCS64 frame pointer. It is a callee-saved GPR generally, but when
`module::FrameRecord::uses_frame_pointer_for_fixed_slots` is true, it is
reserved for frame-pointer policy and is not an ordinary long-lived home.

Target MIR must use this policy to keep frame-slot addressing, callee-save
obligations, dynamic-stack operations, and return-address-sensitive consumers
from treating `x29` as a general scratch or allocation result. The ABI helper
`frame_pointer_register()` identifies the typed register role, and
`abi::is_special_or_forbidden(..., frame_pointer_reserved)` must reflect the
prepared frame-pointer reservation state.

The contract does not select the final frame-pointer setup, teardown, save
slot, restore sequence, or frame-relative addressing instruction.

## Variadic Behavior

The accepted minimum variadic call contract is to preserve
`PreparedCallPlan::wrapper_kind == DirectExternVariadic` and
`PreparedCallPlan::variadic_fpr_arg_register_count` as call-boundary facts.
`module::CallRecord` directly preserves wrapper kind and retains
`source_call`; until a direct target-MIR field is added, consumers that need the
FP argument-register count must read it from `source_call`.

Full AAPCS64 variadic function-entry support is deferred unless a prepared
carrier explicitly publishes the needed facts. The missing accepted carrier
would include `va_list` layout ownership, GP and FP/SIMD register save-area
layout, named GP/FP counts, named stack bytes, signed offsets, overflow-area
base, and whether FP/SIMD variadic save usage is disabled.

Lowering variadic calls may use the prepared call plan. Lowering variadic
function entry or `va_arg` must not reconstruct save areas from legacy
prologue text or local parameter scanning.

## Deferred Carrier Candidates

The following are not blockers for this contract, but later slices must split
them into separate ideas or narrow record-surface work before relying on them:

- a dedicated AArch64 return-boundary record for `x30`, return side effects,
  return-control provenance, and return-value ABI resources
- a direct `CallRecord` field for `variadic_fpr_arg_register_count`
- a complete outgoing call-area carrier: total call-time stack adjustment,
  per-call stack alignment proof, and ownership of any indirect-callee spill
  area
- explicit special-register role records for `x8`, `x16`, `x17`, `x29`, and
  `x30` when ABI helper classification and prepared call/frame facts are not
  enough for machine-verifiable lowering
- an AAPCS64 variadic function-entry carrier for `va_list` and register
  save-area layout
- target ABI classification carriers for HFA, F128, i128, aggregate splitting,
  and stack/register boundary cases if prepared call plans prove insufficient
- runtime-helper call resource carriers for F128 or soft-float paths

## Rejected Routes

These routes are not accepted AArch64 call, return, or frame lowering:

- choosing final call, indirect-call, return, prologue, epilogue, save/restore,
  stack-adjust, or addressing instructions in target MIR contract work
- deriving argument, result, stack-argument, memory-return, variadic, or frame
  policy from legacy assembly text or markdown examples
- using `x8`, `x16`, `x17`, `x29`, or `x30` as ordinary scratch or long-lived
  homes when their special roles are active
- recomputing call-preservation routes, callee-save obligations, frame layout,
  or outgoing call-area layout locally in an instruction-selection slice
- treating rendered register names as sufficient authority when a prepared id,
  typed `RegisterReference`, or prepared source record is available
- adding testcase-shaped exceptions for one call or return case instead of
  consuming prepared carriers or opening a carrier-gap idea
