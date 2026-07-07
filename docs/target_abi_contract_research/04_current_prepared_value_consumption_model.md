# 04. Current Prepared Value Consumption Model

Question: What is the current decision model for consuming a prepared value:
reuse an existing home, rematerialize from the producer, copy/publish into a
destination, or fail closed?

## Short Answer

The current model is distributed, not centralized. Prepared value consumption
is decided across value-home publication, call-plan source selection,
move-bundle classification, target backend lowering, and prepared-object
diagnostics.

The practical order is:

1. Publish the best available `PreparedValueHome` for each value.
2. Reuse that home directly when it is already the needed register, stack slot,
   immediate, symbol, or computed-address source.
3. For call arguments, classify source selections that can reuse prepared
   frame bytes or local-frame-address materializations.
4. If direct source selection does not prove the route, fall back to a unique
   prior preservation record.
5. Emit explicit copies or publication moves from prepared move bundles and
   call-boundary effects.
6. Rematerialize only the producer kinds that have explicit prepared facts and
   target-consumer support.
7. Fail closed when a consumer sees missing, ambiguous, incomplete, or
   unsupported authority.

That order is real but not owned by one function. It emerges from several
surfaces that each validate part of the contract.

## Reuse: Existing Homes First

The first consumption decision is made when value homes are published in
`src/backend/prealloc/regalloc/value_homes.cpp`.
`classify_prepared_value_home()` creates a `PreparedValueHome` with kind
`None`, then upgrades it to a concrete source when it can prove one:

- Formal parameters can become register homes from ABI argument registers, or
  stack homes for variadic, stack-passed `f128`, and RV64 stack-passed fixed
  formal cases.
- Assigned stack slots become `StackSlot` homes through
  `publish_assigned_stack_home()`.
- Assigned registers become `Register` homes with register storage facts and,
  for selected RV64 FPR paths, target register identity.
- Pointer carrier facts can become `PointerBasePlusOffset` homes when
  `has_semantic_pointer_carrier_authority()` proves the carrier.

Call planning consumes those homes in
`src/backend/prealloc/call_plans.cpp`. `plan_call_argument_source()` first
looks up the named argument's prepared home and maps home kind to source
encoding with `storage_encoding_from_home()`:

| Home kind | Source encoding |
| --- | --- |
| `Register` | `PreparedStorageEncodingKind::Register` |
| `StackSlot` | `PreparedStorageEncodingKind::FrameSlot` |
| `RematerializableImmediate` | `PreparedStorageEncodingKind::Immediate` |
| `PointerBasePlusOffset` | `PreparedStorageEncodingKind::ComputedAddress` |
| `None` | `PreparedStorageEncodingKind::None` |

That means direct reuse is the default when the prepared home already carries
the source location or literal. Later target consumers mostly expect this
source information to be complete before they emit a move or instruction.

## Copy And Publication Routes

The next decision layer is `select_prepared_call_argument_source()` in
`src/backend/prealloc/call_plans.cpp`. It only runs after the before-call move
for the argument has been found with `find_before_call_argument_move()`, so it
is tied to an explicit prepared move/publication.

Its source-selection order is:

1. Detect byval register-lane copies through
   `prepared_byval_lane_extent_bytes()` and select `ByvalRegisterLane`.
2. For frame-slot sources, select `FrameSlotAddress` for sret memory returns
   or proven local frame address materializations, otherwise select
   `FrameSlotValue` when a slot or stack offset is known.
3. For local aggregate address publication, select
   `LocalFrameAddressMaterialization` from either a register home derived from
   a local frame address or a `PointerBasePlusOffset` computed-address home.
4. Only after those source routes fail, look for a unique prior preservation
   source with `find_unique_indexed_prior_preserved_value_source()` and select
   `PriorPreservation`.

Aggregate copies are then planned by `plan_prepared_aggregate_transport()`.
`LocalFrameAddressMaterialization` becomes a `StackCopy` plan for byval memory
arguments. `ByvalRegisterLane` becomes a `ByvalRegisterLanes` transport plan
with byte chunks and lane destinations.

Call-boundary copies are also published as effects. `plan_prepared_call_boundary_effects()`
combines explicit before-call moves, preservation-home population, explicit
after-call moves, and preservation republication. That gives target consumers
a separate stream of "copy or publish this value" effects in addition to the
argument/result plans.

## Prior Preservation Fallback

Prior preservation is a fallback, not the first choice. In
`select_prepared_call_argument_source()`, the prior-preservation lookup happens
after direct frame-slot, frame-address, byval-lane, local-address, and
computed-address selections have already been attempted.

The fallback source comes from `PreparedCallPreservedValue` records built in
`build_call_preserved_values()`. That builder considers regalloc values whose
live interval crosses the call and publishes a preservation route when it can
prove one of these homes:

- `StackSlot`, from an existing value home, stack object, or assigned stack
  slot.
- `CalleeSavedRegister`, from an assigned register that matches the target
  callee-saved register spans.

The lookup state is incremental. `append_incremental_prior_preserved_values()`
indexes preserved values from completed call plans, and
`seed_supported_prior_call_preservations_from_current_call()` can seed earlier
calls from later callee-saved dependencies when the preservation source and
destination are complete enough.

AArch64 and RV64 both consume this fallback only when the payload is explicit.
`src/backend/mir/aarch64/codegen/calls.cpp` uses
`make_prior_preserved_call_argument_source()` and emits diagnostics such as
"AArch64 prior-preserved call argument requires prepared PriorPreservation
source selection" or "requires complete prepared source selection" when the
selection is missing or incomplete. `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
uses `emit_riscv_prior_preserved_gpr_argument()`,
`gpr_register_number_for_prior_preserved_selection()`, and
`stack_slot_offset_for_prior_preserved_gpr_selection()` to accept only complete
GPR callee-saved or stack preservation payloads.

## Explicit Rematerialization

There are three explicit rematerialization families today.

The first family is prepared value homes:
`classify_prepared_value_home()` publishes `RematerializableImmediate` homes
for:

- `f128` constant payload regalloc values through
  `value.constant_f128_payload`.
- Named `i32` computed values whose producer chain is based on an immediate
  and uses supported binary operations. The accepted operations in this home
  classifier are `Add`, `Mul`, `And`, `Or`, `Xor`, `Sub`, `Shl`, `LShr`, and
  `AShr`.

Consumers then verify those homes before materializing them. For example,
`src/backend/mir/riscv/codegen/object_emission.cpp` uses
`integer_immediate_for_value()` and
`verify_prepared_rematerializable_integer_immediate_contract()` before treating
a named value as an immediate. The same file rematerializes immediate sources
for select/join move paths when the contract is coherent.

The second family is call-argument producer materialization in
`src/backend/prealloc/call_plans.cpp`.
`find_prepared_call_argument_source_producer_materialization()` accepts:

- `PreparedEdgePublicationSourceProducerKind::LoadLocal`.
- Binary producers accepted by
  `prepared_call_argument_binary_producer_opcode_is_materializable()`.

That binary producer helper currently marks `Add`, `Sub`, `And`, `Or`, `Xor`,
`Mul`, `SDiv`, and `SRem` as materializable for this call-argument producer
fact. It explicitly rejects unsigned division/remainder, shifts, and compare
operators in this path.

The third family is target-specific rematerialization in RV64 object emission,
where prepared select/join and pure-instruction paths can materialize integer
immediates if `integer_immediate_for_value()` or the move's
`source_immediate_i32` provides a valid immediate. Examples include
`fragment_for_predecessor_select_publication_immediate_to_gpr()`,
`prepared_predecessor_select_publication_bundle_is_immediate_to_gpr_materialized()`,
and the phi/join move handling in `object_emission.cpp`.

## Target Consumer Decisions

RV64 call emission in
`src/backend/mir/riscv/codegen/prepared_call_emit.cpp` is representative of
the final consumer order:

1. Emit preservation-home population effects before the call.
2. For each argument, prefer the prepared call argument plan.
3. Emit byval aggregate address transport if the plan proves that special
   route.
4. Emit local frame address materialization when a
   `LocalFrameAddressMaterialization` route is complete.
5. Emit prior-preserved GPR arguments when the source selection is
   `PriorPreservation` and the callee-saved register payload is complete.
6. Emit register-to-register reuse or a frame-slot-address argument when the
   source encoding and selection prove it.
7. Fall back to `emit_move_to_register()` for ordinary BIR operand moves.
8. Emit result moves and preservation republication after the call.
9. Return `std::nullopt` when any required route field is missing or
   unsupported.

AArch64 call emission makes the same kind of choices in
`src/backend/mir/aarch64/codegen/calls.cpp`, but with richer diagnostics. It
has explicit lowering for immediate call arguments
(`lower_before_call_immediate_binding()`), prior-preserved arguments
(`make_prior_preserved_call_argument_source()`), local aggregate address
arguments, frame-slot and stack-lane publications, f128 carriers, and stack
preservation republication.

RV64 object emission adds another consumer family in
`src/backend/mir/riscv/codegen/object_emission.cpp`. It uses
`classify_prepared_object_select_consumer()`,
`classify_prepared_object_move_bundle_consumer()`, and
`diagnose_prepared_object_consumer()` to decide whether select publications,
move bundles, value homes, and frame slots have enough authority for object
emission.

## Fail-Closed Diagnostics

Unknown authority is protected by several fail-closed layers:

- `src/backend/prealloc/prepared_contract_verifier.cpp` verifies
  rematerializable integer immediates and pointer-base-plus-offset homes.
  Missing value homes, missing names, conflicting home kinds, missing
  immediate payloads, missing pointer bases, and missing deltas produce
  fail-closed reports.
- `src/backend/prealloc/prepared_object_traversal.cpp` diagnoses prepared
  object consumers for missing or ambiguous join transfers, missing names,
  missing value locations, missing prepared value homes, conflicting value
  IDs, unsupported home kinds, incomplete stack-slot homes, missing move
  bundles, mismatched bundle phase/block, and malformed frame-slot ownership.
- AArch64 call lowering appends concrete diagnostics when call-boundary moves,
  immediate call arguments, prior-preserved arguments, aggregate lane
  publications, local aggregate address routes, binary128 carriers, or
  variadic helper operand homes are missing required prepared facts.
- RV64 prepared call emission rejects incomplete routes by returning
  `std::nullopt`; RV64 object emission turns many of those failures into
  explicit `unsupported_*` diagnostics such as unsupported prepared move-bundle
  classification, unsupported local memory access, unsupported scalar compare
  publication, and unsupported variadic helper lowering.

These diagnostics are important because no single contract field proves
"this source is fresh and authoritative" for every consumer. The consumers
therefore reject incomplete payloads rather than silently guessing.

## Answer

The current prepared value consumption model is a distributed decision model.
It starts with prepared value homes, attempts direct reuse and explicit
copy/publication routes, uses prior preservation only as a later fallback,
rematerializes only explicit immediate, load-local, and selected binary
producer facts, and fails closed when a target consumer cannot prove the route.

The model is inspectable, but not centralized. The same value may be judged by
`classify_prepared_value_home()`, `plan_call_argument_source()`,
`select_prepared_call_argument_source()`, `build_call_preserved_values()`,
`plan_prepared_call_boundary_effects()`, AArch64 call lowering, RV64 prepared
call emission, RV64 object emission, and prepared-object diagnostics before it
is finally reused, rematerialized, copied, or rejected.
