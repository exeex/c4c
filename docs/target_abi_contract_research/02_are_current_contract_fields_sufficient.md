# 02. Are Current Contract Fields Sufficient

Question: Are `CallArgAbiInfo`, `CallResultAbiInfo`,
`PreparedRegisterPlacement`, `PreparedTargetRegisterIdentity`, move bundles,
call plans, and preservation plans sufficient to carry the ABI facts needed by
both AArch64 and RV64?

## Classification

Answer: sufficient with named limitations.

The current contract is sufficient for the main shared direction: BIR records
target-derived call ABI facts, prealloc turns those facts into prepared call
plans, and AArch64/RV64 consumers can lower many argument/result moves from
those plans. It is not simply sufficient because several facts needed by hard
value-consumption paths are either target-asymmetric, stored as derived
physical names rather than a normalized identity, or distributed across call
plans, value homes, move bundles, and preservation records.

## Contract Surface Table

| Surface | Current carried facts | Fact class | AArch64 use | RV64 use | Sufficiency |
| --- | --- | --- | --- | --- | --- |
| `CallArgAbiInfo` in `src/backend/bir/bir.hpp` | Type, size, align, primary/secondary ABI class, register/stack flags, byval/sret flags, AArch64 HFA lane count/index. Produced through `compute_call_arg_abi()` in `src/backend/bir/lir_to_bir/call_abi.cpp` and adjusted by pressure passes in `src/backend/bir/lir_to_bir/module.cpp`. | Semantic ABI facts. It says what the calling convention requires, not which prepared register object owns the value. | Needs HFA lane metadata, integer/float class split, sret special handling, stack sizing, and byval/register-lane distinctions. `src/backend/mir/aarch64/codegen/calls.cpp` and `prologue.cpp` still inspect these facts. | Needs integer/float class split, hard/soft float availability, register/stack passing, byval and ordinary stack argument flags. RV64 call emission also uses argument type when consuming prior-preserved stack values. | Sufficient for ABI classification. Limited because it does not carry physical register identity, freshness, publication authority, or a target-independent argument register ordinal after target-specific pressure adjustments. |
| `CallResultAbiInfo` in `src/backend/bir/bir.hpp` | Type, primary/secondary ABI class, memory-return flag, register count. Produced through `compute_function_return_abi()` in `call_abi.cpp`. | Semantic ABI facts. | Needs memory-return and register class facts to choose result source register(s), including float/vector register view handling. | Needs memory-return, hard-float return availability, and result register count for `a0`/`fa0` style result sources. | Sufficient for ordinary return classification. Limited for multi-lane or concrete identity-sensitive paths because the physical result register identity is derived later. |
| `PreparedRegisterPlacement` in `src/backend/prealloc/frame.hpp` | Prepared bank, slot pool, slot index, contiguous width. `target_register_profile.cpp` maps call arguments/results into `CallArgument` / `CallResult` pools. | Allocation-policy plus abstract physical target fact. It identifies a target-profile-relative slot, not a universal register number. | AArch64 can convert placements to concrete registers in `src/backend/mir/aarch64/abi/abi.cpp::convert_prepared_register()` and can also consume prepared register names. | RV64 uses placement for ABI slots and can map ABI placements to physical identity through `target_register_identity_for_abi_register_placement()`. | Sufficient for many shared call-placement decisions. Limited because a placement alone is not a stable physical identity for every target and pool; the identity helper currently handles RV64 ABI placements only. |
| `PreparedTargetRegisterIdentity` in `src/backend/prealloc/regalloc.hpp` | Target arch, prepared bank, register class, physical index. Value homes can carry it through `PreparedValueHome::target_register_identity`. | Physical target fact. | AArch64 inline asm can manufacture and compare concrete identities in `src/backend/mir/aarch64/codegen/inline_asm.cpp`, but the shared ABI placement identity helper does not publish AArch64 ABI identities. | RV64 uses identities for FPR/GPR object emission paths, including `fpr_register_number_for_target_identity()` in `src/backend/mir/riscv/codegen/object_emission.cpp`; prealloc publishes RV64 assigned FPR identities in `regalloc/value_homes.cpp`. | Insufficient as a shared contract surface today. It is expressive enough, but publication is target-asymmetric and not uniformly attached to ABI call placements or all value homes. |
| Move bundles (`PreparedMoveBundle`, `PreparedMoveResolution`, `PreparedAbiBinding`) in `src/backend/prealloc/value_locations.hpp` and `regalloc.hpp` | Phase, authority kind, block/instruction position, moves, ABI bindings, destination kind/storage, destination ABI index, destination register name, occupied names, stack offset, placement, and move authority. | Allocation-policy plus physical target facts plus limited authority facts. | AArch64 call lowering requires authoritative before/after call bundles and checks prepared source selection or preservation authority before using some call-argument routes. | RV64 call and object emission consume move bundles for ABI moves, stack-destination fan-in, and unsupported-move diagnostics. | Sufficient for direct ABI move publication and many backend copies. Limited because authority is mostly attached to destination moves; it does not fully encode source freshness or producer-vs-preservation precedence. |
| `PreparedCallPlan` and nested argument/result plans in `src/backend/prealloc/calls.hpp` | Call position, wrapper kind, variadic FPR count, direct/indirect callee, memory return, outgoing stack area, argument plans, result plan, preserved values, and clobbered registers. Argument/result plans carry source/destination storage, register names, occupied names, stack offsets, placements, aggregate transport, source selection, and late-publication facts. Built in `src/backend/prealloc/call_plans.cpp`. | Bridge surface containing semantic ABI references, allocation policy, physical target facts, and value-consumption routing. | AArch64 `require_prepared_call_plan()` treats the plan as authoritative, then uses source selections, prepared names/placements, aggregate transport, and prior-preservation payloads. | RV64 simple call emission and object emission consume the same plan family for argument registers, stack arguments, result copies, prior preservation, and aggregate/byval routes. | Sufficient as the main shared handoff for ordinary calls. Limited because it aggregates several concerns instead of making freshness and physical identity uniformly explicit. |
| Preservation plans (`PreparedCallPreservedValue`, `PreparedCallArgumentSourceSelection`, boundary effects, prior-preserved lookups) in `src/backend/prealloc/calls.hpp` | Preservation route, preserved register/stack payload, call position, source/destination endpoints, callee-saved save index, spill slot placement, reason strings, prior-preservation lookup tables, and `PriorPreservation` source selections. | Value freshness and authority facts, plus allocation-policy and physical target facts. | AArch64 rejects prior-preserved call arguments when `PriorPreservation` source-selection payloads are missing or incomplete, and requires register placement for callee-saved preservation. | RV64 consumes prior-preserved GPR/register and stack-slot selections in `prepared_call_emit.cpp` and object emission. | Sufficient for known preservation routes when all payload fields are present. Limited because the model does not give every consumer a single explicit freshness proof that the preserved home outranks producer rematerialization or stale-home risk. |

## AArch64 Versus RV64 Requirements

AArch64 and RV64 agree on the high-level contract: BIR should describe call ABI
classification, prealloc should decide prepared source/destination routes, and
the target backend should consume prepared plans rather than recomputing the
whole calling convention. Both need argument/result register classes, stack
argument sizes and offsets, before/after call move bundles, preservation
payloads, and fail-closed diagnostics when the prepared route is missing.

The targets differ in the physical facts they need to trust:

- AArch64 paths can often consume register names or
  `PreparedRegisterPlacement` directly. `src/backend/mir/aarch64/abi/abi.cpp`
  converts prepared placements into AArch64 register operands, and
  `src/backend/mir/aarch64/codegen/calls.cpp` requires `PreparedCallPlan` and
  prepared source-selection payloads for hard call routes.
- RV64 object emission more often needs a stable physical register number.
  `src/backend/prealloc/target_register_profile.cpp::target_register_identity_for_abi_register_placement()`
  maps RV64 ABI call argument/result placements to physical indices `10 + slot`,
  while `src/backend/mir/riscv/codegen/object_emission.cpp` checks
  `PreparedTargetRegisterIdentity` before emitting some concrete FPR operations.
- AArch64 has target-specific semantic ABI needs that are already in
  `CallArgAbiInfo`, especially HFA lane count/index and sret use of `x8`.
  RV64 has hard/soft-float register availability and stack argument behavior
  driven by the target profile and BIR ABI flags.
- Both targets need preservation source authority, but they consume it through
  target-specific checks. AArch64 rejects missing prior-preservation payloads in
  `calls.cpp`; RV64 resolves prior-preserved GPR and stack-slot sources in
  `prepared_call_emit.cpp`.

## Named Limitations

1. Physical register identity is not yet a uniform shared ABI field.
   `PreparedTargetRegisterIdentity` is general enough to express the fact, but
   `target_register_identity_for_abi_register_placement()` currently returns
   ABI identities only for RV64. AArch64 can still lower many calls from names
   and placements, but the shared contract is asymmetric.

2. `PreparedRegisterPlacement` is an allocation-policy slot, not always a
   concrete target register. It is sufficient when the consuming target has a
   complete placement-to-register conversion for that pool and bank. It is not
   sufficient when consumers need a stable physical index, tied-register
   equality, or object-emission identity without recomputing target policy.

3. Preservation freshness is distributed. `PreparedCallPreservedValue`,
   `PreparedCallArgumentSourceSelection`, boundary effects, and
   prior-preserved lookup tables carry enough payload for some known routes,
   but no single field states that a preservation home is fresh relative to a
   same-block producer, rematerialization opportunity, or later publication.

4. Move-bundle authority is destination-oriented. `PreparedMoveAuthorityKind`
   and `PreparedAbiBinding` make ABI moves and several special move families
   inspectable, but the bundle does not fully encode source authority,
   producer freshness, or the reason a backend may prefer a move bundle over
   rematerializing the producer.

5. Call plans are authoritative but broad. `PreparedCallPlan` is the right
   shared handoff for call lowering, yet it currently mixes semantic ABI,
   allocation, physical register spelling, stack layout, preservation, and
   publication facts. That works for many direct cases, but it makes missing
   or stale subfacts harder for AArch64 and RV64 consumers to reject
   consistently.

## Conclusion

The current fields are sufficient with named limitations. They can carry the
ordinary AArch64/RV64 call ABI path and many prepared value-consumption
decisions, but they are not a complete shared target ABI/value-consumption
contract. The exact gaps are uniform physical register identity publication,
explicit freshness authority for preservation versus producer
rematerialization, and stronger move-bundle source authority.
