# 03. Where Target Facts Are Split

Question: Where are target facts currently split between `target_profile`,
BIR lowering, prepared/prealloc target register profiles, and backend emission,
and which splits are healthy versus accidental?

## Ownership Split Table

| Code surface | What it owns today | Split status | Evidence and rationale |
| --- | --- | --- | --- |
| `src/target_profile.hpp` and `src/target_profile.cpp` | The target triple, architecture, OS, backend ABI kind, relocation model, and coarse float argument/result register availability in `TargetProfile`. | Healthy | This is the right root of target selection. It chooses facts such as `BackendAbiKind::Aapcs64`, `RiscvLp64*`, and `SysV_X86_64` before BIR or backend lowering. It should not own individual ABI register names, frame routes, or value freshness. |
| BIR ABI payloads in `src/backend/bir/bir.hpp` | Semantic call ABI records: `CallArgAbiInfo`, `CallResultAbiInfo`, parameter ABI fields, call argument ABI fields, call result ABI fields, and function return ABI fields. | Healthy | These records describe calling-convention classification, not final allocation. They carry type, size, alignment, register/stack classification, byval/sret flags, memory return facts, result register count, and AArch64 HFA lane facts. |
| BIR ABI computation in `src/backend/bir/lir_to_bir/call_abi.cpp` and call/signature lowering in `src/backend/bir/lir_to_bir/calling.cpp` | The target-profile-dependent conversion from LIR signatures and call operands into BIR ABI facts. | Healthy with a narrow watchout | `compute_call_arg_abi()`, `compute_function_return_abi()`, `lower_return_info_from_type()`, and declaration/call lowering are the right place to derive semantic ABI records from `TargetProfile`. The watchout is that this layer also contains target-specific semantic exceptions such as AArch64 HFA handling and RV64 hard-float availability; those are acceptable here only while they stay semantic and do not become physical register policy. |
| ABI pressure passes in `src/backend/bir/lir_to_bir/module.cpp` | Late target-specific correction of BIR ABI records, including AArch64 HFA pressure and RV64 ordinary C stack pressure. | Suspicious | These passes are still working on semantic ABI records, but the ownership is less obvious than `call_abi.cpp` because the facts are adjusted after initial lowering. This is not accidental by itself, but it makes the final ABI fact owner split across multiple BIR files. |
| Prepared register placement types in `src/backend/prealloc/frame.hpp` | Abstract prepared register slots: `PreparedRegisterBank`, `PreparedRegisterSlotPool`, and `PreparedRegisterPlacement`. | Healthy | This is the shared allocator-facing representation. It deliberately names bank, pool, slot index, and contiguous width without forcing every consumer to know a concrete target register spelling. |
| Physical identity types in `src/backend/prealloc/regalloc.hpp` and value homes in `src/backend/prealloc/value_locations.hpp` | `PreparedTargetRegisterIdentity` and optional `PreparedValueHome::target_register_identity`, plus value-home kinds for registers, stack slots, immediates, and pointer-base-plus-offset facts. | Suspicious | The type is a good shared physical identity shape, but publication is uneven. It is expressive enough for a target-independent handoff, yet the current producers publish target identities only in selected paths. |
| Target register policy in `src/backend/prealloc/target_register_profile.cpp` | Concrete ABI register names, caller/callee-saved pools, argument/result placements, register banks/classes, contiguous register spans, and the ABI-placement-to-physical-identity helper. | Healthy as a boundary, suspicious as a partial policy | This is the strongest current owner of physical target ABI policy. It maps x86, AArch64, and RV64 register pools and call argument/result slots. The suspicious part is incompleteness: `target_register_identity_for_abi_register_placement()` currently gives RV64 ABI identities, while AArch64 and x86 rely more on names or backend-local conversion. |
| Regalloc placement identity in `src/backend/prealloc/regalloc_placement_identity.cpp` | Normalization of assigned registers and spills into `PreparedRegisterPlacement` for regalloc outputs. | Healthy | `populate_regalloc_placement_identity()` is the right place to attach prepared placement facts to allocation results. It should remain target-profile-driven through the prepared register profile rather than inventing backend-specific spelling. |
| Value-home publication in `src/backend/prealloc/regalloc/value_homes.cpp` | Prepared value homes, formal-argument homes, rematerializable facts, pointer-base-plus-offset facts, stack homes, register homes, and selected target identities. | Suspicious | This surface necessarily owns value-home publication, but it also contains target-specific formal-home and physical-identity details such as AArch64 formal bank checks, RV64 stack-passed formal lookup, and RV64 FPR identity publication. The split is acceptable while publishing homes, but suspicious where target ABI identity is rediscovered instead of coming uniformly from target ABI policy. |
| Call plans in `src/backend/prealloc/call_plans.cpp` and `src/backend/prealloc/calls.hpp` | `PreparedCallPlan`, argument/result plans, source selections, outgoing stack area, call clobbers, preserved values, prior-preservation payloads, call-boundary effects, and before/after-call move-bundle classification. | Suspicious | This is the main bridge from semantic ABI facts to backend-consumable prepared plans, so it must combine BIR ABI, allocation, stack layout, move bundles, and value homes. The risk is breadth: it owns destination register selection, stack argument sizing, source-route selection, preservation fallback, and boundary effects. That makes it hard to identify a single freshness or physical-identity authority. |
| Move bundles and ABI bindings in `src/backend/prealloc/value_locations.hpp` and `src/backend/prealloc/regalloc.hpp` | Before/after call moves, before-return moves, block-entry moves, `PreparedAbiBinding`, destination register names/placements, destination storage, and move authority kind. | Suspicious | Move bundles are a real shared publication surface, but their authority is mostly destination-oriented. They know enough to copy or publish a destination, not always enough to prove that the source home is fresh or that producer rematerialization should lose to preservation. |
| AArch64 ABI conversion and backend consumption in `src/backend/mir/aarch64/abi/abi.cpp` and `src/backend/mir/aarch64/codegen/calls.cpp` | Conversion of prepared placements/names into AArch64 operands, prepared call-plan validation, call argument/result lowering, prior-preservation checks, stack/value publication, and diagnostics. | Suspicious | AArch64 correctly consumes prepared call plans and fails closed in many missing-contract cases. The suspicious split is that backend code still owns substantial interpretation of prepared placements, register views, HFA lanes, call-boundary source identity, and preservation payload completeness. Some of that is target emission; some looks like policy that should be uniformly published before backend emission. |
| RV64 prepared call/object consumers in `src/backend/mir/riscv/codegen/prepared_call_emit.cpp` and `src/backend/mir/riscv/codegen/object_emission.cpp` | RV64 call emission, prepared argument/result moves, stack argument copies, prior-preserved source resolution, object-emission register-number checks, and prepared-consumer diagnostics. | Suspicious | RV64 consumes the shared prepared surface but also needs stable physical register numbers for object emission. That makes the RV64 path expose the contract gap earlier than AArch64: if identity is missing, RV64 has to fail or rediscover target facts locally. |
| x86 target ABI helpers and module emission in `src/backend/mir/x86/abi/abi.cpp` and `src/backend/mir/x86/module/module.cpp` | x86 target-profile validation, symbol spelling, narrow register-name rendering, prepared-module consumption, and a limited prepared call/move/value-location route. | Accidental for shared-regalloc policy | x86 already consumes prepared value locations, call plans, and move bundles in parts of `module.cpp`, while `target_register_profile.cpp` also contains x86 caller/callee-saved and ABI register pools. The backend still owns a large amount of physical register spelling and route-specific narrowing. For a shared x86/AArch64/RV64 allocator, this is the clearest accidental split because x86-specific backend rendering currently doubles as policy for some physical-register choices. |
| Backend front doors in `src/backend/backend.cpp` | Target dispatch, `prepare_semantic_bir_pipeline()`, per-target prepared-module handoff, and target-specific public entry behavior such as AArch64 clearing `prepared.regalloc.functions`. | Suspicious | Dispatch belongs here. Prepared-module mutation at the handoff boundary is riskier: clearing AArch64 regalloc functions means one target consumes a weaker prepared contract than RV64, which reinforces target-asymmetric ownership of prepared placement and identity facts. |

## Healthy Boundaries

The healthiest split is the high-level direction:

1. `TargetProfile` owns target selection and coarse ABI capabilities.
2. BIR lowering owns semantic ABI classification.
3. Prealloc owns allocation, stack layout, prepared register placements, call
   plans, move bundles, value homes, and preservation records.
4. Target backends own final instruction and object emission.

That split supports a shared register allocator because target facts flow
forward before allocation. The allocator can see semantic ABI facts, and the
prepared surface can publish target-profile-relative placements rather than
asking each backend to reconstruct the call ABI from raw triples.

`PreparedRegisterPlacement` is also a healthy intermediate abstraction. It is
specific enough to represent caller-saved, callee-saved, call-argument, and
call-result pools, but it is not itself a concrete machine-register spelling.
That keeps shared regalloc policy from depending on a backend assembler string.

## Suspicious Splits

The suspicious splits are the ones where a fact has a reasonable shared owner
but is not uniformly published there.

Physical register identity is the first example. `PreparedTargetRegisterIdentity`
is a good shared shape, and `target_register_profile.cpp` is the best current
source of ABI register pool policy. But identity publication is target
asymmetric: RV64 ABI placements can become physical indices, while AArch64
often consumes names/placements and x86 narrows/render names in backend code.
That means the shared contract can say "call result slot 0" more reliably than
it can say "this is physical target register identity X" for every target.

Preservation freshness is the second example. `PreparedCallPreservedValue`,
`PreparedCallArgumentSourceSelection`, prior-preserved lookups, call-boundary
effects, and move bundles together carry preservation and publication facts.
No one field states that a preserved home is fresh relative to a producer,
rematerializable value, or later publication. That leaves both AArch64 and RV64
consumers doing local completeness checks.

Call plans are the third example. `PreparedCallPlan` is the right authoritative
handoff for calls, but it currently owns many layers at once: semantic ABI
references, target register names, prepared placements, stack layout, argument
source selection, result routing, preservation, clobbers, and boundary effects.
The breadth is useful for emission, but it blurs whether target ABI policy,
value freshness, or move authority is the reason a consumer may trust a route.

## Accidental Splits

The clearest accidental split is backend-local physical register policy that
duplicates or bypasses prepared target policy. x86 is the most visible case:
`target_register_profile.cpp` owns x86 ABI pools, but `src/backend/mir/x86`
still owns target-profile validation, register narrowing, prepared call/move
consumption, and route-specific physical rendering in backend code. Some final
rendering must remain target-local, but allocator policy and ABI register
identity should not be learned from backend emitter helpers.

AArch64 and RV64 have smaller versions of the same issue. AArch64 consumers
convert prepared placements and validate preservation payloads locally. RV64
object emission needs concrete physical indices and therefore notices missing
`PreparedTargetRegisterIdentity` sooner. These are not wrong as target
emission details, but they become accidental when they are the only place a
shared allocator can learn whether a prepared ABI register is physically
usable.

## Shared Regalloc Blocker

Yes, one split blocks the shared x86/AArch64/RV64 register allocator goal: the
contract does not yet have a single, uniform owner for target ABI register
policy plus physical identity publication.

The shared allocator needs a target-profile-specific policy surface that can
answer the same questions for all three targets:

- Which ABI argument/result registers exist for each bank and ABI class?
- Which caller-saved and callee-saved pools are available?
- What prepared placement corresponds to each ABI slot?
- What stable `PreparedTargetRegisterIdentity` corresponds to that placement?
- Which target-specific register spelling is only a final emission view, not
  allocator policy?

Today those answers are split across `TargetProfile`, BIR ABI lowering,
`target_register_profile.cpp`, `call_plans.cpp`, value-home publication, and
backend consumers. The split is workable for narrow paths, but it blocks a
clean shared allocator because x86, AArch64, and RV64 cannot all rely on the
same published physical identity and freshness authority.

## Smallest Coherent Consolidation Candidate

The smallest coherent consolidation candidate is the prepared target register
profile layer, centered on `src/backend/prealloc/target_register_profile.cpp`
and its header.

That surface already owns the closest thing to target ABI register policy:
ABI register names, register banks/classes, caller/callee-saved pools,
argument/result placements, contiguous spans, and the current
ABI-placement-to-physical-identity helper. Consolidation should make that
surface the single policy source for target ABI register pools and physical
identity publication, while leaving final instruction spelling in the target
backends and semantic ABI classification in BIR lowering.

The consolidation should not start by moving all call-plan or preservation
logic into `target_register_profile.cpp`. The smallest useful step is narrower:
make the target register profile publish uniform x86/AArch64/RV64 ABI register
placements and `PreparedTargetRegisterIdentity` facts, then let call plans,
value homes, move bundles, and backend consumers depend on those facts instead
of deriving target-local policy independently.

## Answer

The current split is directionally healthy but uneven. `TargetProfile` and BIR
ABI lowering own the right semantic layers; prealloc owns the right prepared
handoff layer; target backends own final emission. The suspicious and
accidental splits are physical target identity, preservation freshness, and
backend-local register policy. For shared x86/AArch64/RV64 regalloc, the
smallest consolidation point is the prepared target register profile: make it
the coherent source of ABI register pools, placements, and physical identities
for all targets, without moving final instruction rendering or semantic ABI
classification out of their current layers.
