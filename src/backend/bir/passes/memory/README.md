# P05 Memory Canonicalization Pass Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: pass
Phase-ID: B5 / P05
Upstream: exact immutable B4 `SsaCanonical` plus matching effects/provenance facts
Downstream: exact immutable B5 `MemoryCanonical` module wave consumed by B6 / P06
Owner-Path: `src/backend/bir/passes/memory/README.md`
Last-Reconciled-Commit: `b6cabf1d2`

## Purpose

P05 is the target-independent B5 module transformation. It consumes the exact
accepted B4 wave and matching exact-current memory-effects/provenance closure,
normalizes memory, address, atomic and explicit effect-bearing forms, then
publishes one verified B5 wave or fails atomically.

## Owns

- closed `PassId::MemoryCanonicalize` / B5 module occurrence;
- registered target-independent local/global/TLS/label address, semantic path/
  offset, access, copy/set, allocation/lifetime/stack-state, atomic/fence and
  explicit effect-operand normalization;
- typed use repair caused by its replacements and P05 local/cumulative
  postconditions;
- the `MemoryCanonical` property request after framework verification.

## Does Not Own

- CFG/SSA topology, phi values, earlier scalar/SSA or later aggregate/intrinsic
  canonical forms;
- stored provenance/alias/effect conclusions or analysis mutation;
- target layout/size/address modes, ABI/helper selection, aggregate
  decomposition, preparation, constraints, allocation, MIR or emission;
- inference from names, pointers, text, positions, legacy routes or side data;
- revision, transaction, cache, property, checkpoint or stage publication.

## Inputs

P05 consumes one immutable whole-module B4 checkpoint and, for every function,
matching schema-1 `MemoryEffects` and `Provenance` handles. Provenance closes
over exact-B4 CFG, dominance, publication/value-flow and effects dependencies.
P04 postconditions, exact phi coverage, core def-use and the configured verifier
accept the same module/function revisions.

### Exact input and dependency matrix

| Input axis | Exact required state | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| owning checkpoint | exact successful immutable B4 module wave | declaration/empty functions remain members | wrong/missing capability is `MemoryPassWrongInput` |
| module/function key | exact `ModuleEpoch`, `ModuleRevision`, ordered `(FunctionId, FunctionRevision)` digest and B4 stamp | empty module has empty exact digest | stale/foreign/mixed wave is `MemoryPassStaleInput` |
| cumulative properties | `RawVerified` + `TypesLegal` + `ScalarsCanonical` + `CfgCanonical` + `SsaCanonical` | none | missing/self-asserted property is `MemoryPassWrongInput` |
| P04 acceptance | complete P04 postconditions, exact def-use/phi coverage and configured verifier | no earlier-owner form may remain | mismatch is `MemoryPassP04ContractFailure` |
| effects facts | complete ordered schema-1 `AnalysisId::MemoryEffects` handle set at exact B4 function keys | known empty/`Absent`/`Unknown` effects are valid facts | missing/stale/mixed handle is `MemoryPassStaleAnalysis` |
| provenance facts | complete ordered schema-1 `AnalysisId::Provenance` set with exact dependency closure | `Absent`/`Unknown` preserves valid form unless a row rejects | stale/mismatched dependency is `MemoryPassStaleAnalysis` |
| semantic memory graph | typed stable objects/values/types/address spaces, source sizes/alignments, exact SSA/CFG and registered op descriptors | optional source alignment may be absent | malformed graph is inherited verifier failure |
| opaque/later families | exact inline-asm bytes/constraints and aggregate/intrinsic semantic forms | unfamiliar valid later-owner relation remains unchanged | changing opaque/later semantics is `MemoryPassPreservedFormChanged` |
| target/helper exclusion | no target layout/size/address-mode, ABI/helper/preparation/allocation/MIR input | none | later-domain fact is `MemoryPassForbiddenAuthority` |

No name/text/pointer/position inference, earlier cache, target default, prepared
lookup, legacy access view or compatibility route may substitute.

## Outputs

One atomic B5 module wave retains the epoch, derives exact module/function
revisions and authoritative `MutationSummary`, records B5, retains cumulative
properties through `SsaCanonical`, and establishes `MemoryCanonical` only after
all functions/module boundaries and postconditions succeed.

### Exact output handoff matrix

| Output/product | Exact consumer | Required binding and acceptance | Failure / forbidden escape |
|---|---|---|---|
| immutable B5 module wave | B6/P06 aggregate | exact epoch/module/function digest, B5 stamp and no B4 aliases | no function prefix/editor/mixed wave substitutes |
| cumulative properties | B6 precondition and B8 verifier | Raw + P01 + P02 + P03 + P04 retained; framework establishes `MemoryCanonical` | P05 cannot mint property/report/capability |
| canonical memory/address graph | B6 and exact-revision analyses | one typed target-independent form per closed row; CFG/SSA exact | no target layout/address mode/helper/side table escapes |
| preserved semantic attributes | B6 and later target preparation | exact source sizes, alignments, object/address-space/type/path identities | no target reinterpretation/default/ABI placement |
| occurrence `MutationSummary` | analysis/pass frameworks | exact object/address/access/atomic/effect/use/type/module effects | declared preservation cannot override edits |
| structured diagnostics | pipeline observation only | stable rule/entity/object/operand anchor and deterministic order | cannot authorize partial wave, fallback or B6 |

B6 accepts only this exact immutable B5 `MemoryCanonical` wave with all
P01-P05 postconditions current. It cannot repair a P05 form or accept stale
effects/provenance as a capability.

## Adjacent-Stage Contract

[P04 SSA](../ssa/README.md) supplies the exact B4 checkpoint.
[Memory effects](../../analysis/memory_effects/README.md) and
[provenance](../../analysis/provenance/README.md) supply matching immutable
facts. P05 alone owns the registered rewrites below; frameworks own
transactions/analysis/publication and core owns semantic identities.

[B6/P06 aggregate](../aggregate/README.md) consumes the exact B5 output. P05
preserves aggregate semantic paths/boundaries for B6 and never introduces a
target decomposition or recreates a P01-P04 form.

## Ordered Behavior

1. Validate exact B4 module/function keys/properties/P04 postconditions and the
   complete same-key effects/provenance handle sets.
2. Inventory every memory/address/atomic/effect form and cross-declaration
   boundary in canonical stable-ID order. Assign one closed matrix disposition.
3. Prove registered rules from typed semantics and `Known` analysis facts.
   Preserve valid unproved rows; accumulate deterministic rejects before edits.
4. Fork one private whole-module occurrence transaction, reserve deterministic
   IDs and build a closed plan for every replacement/use/effect repair.
5. Apply typed atomic RAUW and complete def-use repair without changing CFG/phi
   coverage. Preserve source-semantic attributes exactly and derive the
   authoritative mutation summary from actual edits.
6. Run cumulative Raw+P01+P02+P03+P04+P05 postconditions, exact CFG/SSA
   preservation and verifier-on-commit across the complete private wave.
7. Atomically publish B5 and establish `MemoryCanonical` only if all work
   succeeds; otherwise discard the candidate and retain B4/cache unchanged.

## Invariants

- Semantic object, type, address-space, field/index/path, size and alignment
  facts remain exact. They are not target layout or ABI placement.
- Address/path rewrites prove semantic equivalence without target pointer width,
  byte layout, address mode or helper selection.
- Volatility, ordering, scope, fences, barriers, lifetime, escape and poison/
  undef/exception behavior are preserved exactly.
- CFG terminators/edges and SSA definitions/uses/phi incoming multiplicity do
  not change. P05 repairs only uses caused by its own typed replacements.
- `InlineAsm` payload/constraints remain opaque; only explicit effect operands
  are observed/preserved.
- Stable IDs/roles identify entities; pointers, names, text, positions and
  legacy records do not.

## Exhaustive P05 Memory-Form Disposition Matrix

| Applicable B4 input form | Lossless facts / precondition | Disposition | Exact P05 result or stable failure | Next owner / B6 visibility | Implementation |
|---|---|---|---|---|---|
| declaration, empty module or empty function | complete empty facts and keys | Preserve | unchanged; no failure | B6 sees exact empty member | absent |
| canonical typed null address | exact type/address space | Preserve | unchanged; no failure | B6 sees canonical null | absent |
| source alias for null address | exact typed zero/null semantics independent of target width | Normalize | canonical typed null; else `MemoryNullInvalid` | B6 sees canonical null | absent |
| canonical global address | exact `SymbolId`, object/type/address space | Preserve | unchanged; no failure | B6 sees semantic global identity | absent |
| noncanonical global/TLS/label address spelling | `Known` exact symbol class and semantic identity | Normalize | canonical typed address node; else `MemorySymbolOriginUnproved` | B6 sees stable symbol address | absent |
| canonical local/object address | exact `ObjectId`, type, scope and address space | Preserve | unchanged; no failure | B6 sees semantic object address | absent |
| object/address with ambiguous or absent identity | no registered exact origin proves rewrite | Preserve | retain valid typed form; no failure | later exact analysis may remain unknown | absent |
| malformed/incomplete required object identity | core descriptor requires identity but none exists | Reject | `MemoryObjectIdentityMissing` | no B5 output | absent |
| canonical semantic GEP/field/index path | exact base/type and layout-independent components | Preserve | unchanged; no failure | B6 preserves aggregate path | absent |
| noncanonical typed field/index path | exact provenance base and semantic components | Normalize | canonical ordered path; else `MemoryPathUnproved` | B6 sees layout-independent path | absent |
| semantic byte-offset chain | exact typed offset arithmetic independent of target layout | Normalize | canonical single offset/path descriptor; else `MemoryOffsetUnproved` | B6 sees semantic offset | absent |
| zero semantic offset/path component | exact zero and poison/undef semantics | Normalize | remove component/identity RAUW; else `MemoryOffsetUnproved` | B6 sees minimal path | absent |
| path requiring physical field offset or pointer width | semantic identity cannot prove target-independent result | Reject | `MemoryTargetLayoutRequired` | no B5 output | absent |
| canonical typed load | exact value/address/type/alignment/volatile/effect operands | Preserve | unchanged; no failure | B6 sees canonical load | absent |
| admitted load opcode/descriptor alias | exact typed access semantics and effects | Normalize | canonical load preserving all attributes; else `MemoryAccessUnproved` | B6 sees canonical load | absent |
| canonical typed store | exact address/value/type/alignment/volatile/effect operands | Preserve | unchanged; no failure | B6 sees canonical store | absent |
| admitted store opcode/descriptor alias | exact typed access semantics and effects | Normalize | canonical store preserving all attributes; else `MemoryAccessUnproved` | B6 sees canonical store | absent |
| unspecified source alignment | absence is source-semantic and no target default is legal | Preserve | retain `Absent` alignment; no failure | later target stage decides legality | absent |
| explicit source-semantic alignment | exact declared/admitted alignment | Preserve | retain exact value; no failure | later target stage observes it | absent |
| noncanonical alignment encoding with exact same value | exact mathematical alignment and source meaning | Normalize | canonical encoding; else `MemoryAlignmentInvalid` | B6 sees exact alignment | absent |
| canonical memory copy/move/set | exact source/destination/size/alignment/overlap/volatile effects | Preserve | unchanged; no failure | B6 sees canonical memory op | absent |
| admitted copy/move/set alias | all semantic operands/effects exact | Normalize | canonical descriptor; else `MemoryBulkOpUnproved` | B6 sees canonical memory op | absent |
| dynamic allocation/free semantic node | exact size/alignment/object/lifetime/effect semantics | Preserve | unchanged; no failure and no helper selected | later intrinsic/target owners retain semantics | absent |
| stack save/restore or lifetime marker | exact typed scope/object/effect operands | Preserve | unchanged; no failure | later owners retain state semantics | absent |
| canonical atomic load/store/RMW/CAS | exact type/address/order/scope/volatile/effect semantics | Preserve | unchanged; no failure | B6 sees canonical atomic | absent |
| admitted atomic descriptor alias | exact operation and ordering/scope semantics | Normalize | canonical atomic descriptor; else `MemoryAtomicUnproved` | B6 sees canonical atomic | absent |
| invalid atomic ordering/scope combination | registry rejects semantic combination | Reject | `MemoryAtomicInvalid` | no B5 output | absent |
| canonical fence/barrier | exact ordering/scope/effect operands | Preserve | unchanged; no failure | B6 sees canonical barrier | absent |
| admitted fence/barrier alias | exact registered semantic equivalence | Normalize | canonical barrier; else `MemoryFenceUnproved` | B6 sees canonical barrier | absent |
| noncanonical explicit effect operands | complete typed semantic effect set proven | Normalize | canonical ordered/deduplicated effect operands; else `MemoryEffectUnproved` | B6 sees explicit semantic effects | absent |
| analysis-only effect/provenance conclusion | derived fact is not stored semantic truth | Preserve | do not materialize conclusion; no failure | analyses remain recomputable | absent |
| call/intrinsic with declared memory effects | exact typed call/bundle/effect semantics | Preserve | unchanged; no failure and no helper/address route selected | P07 retains intrinsic owner | absent |
| opaque `InlineAsm` memory/effect node | payload/constraints opaque and explicit effects exact | Preserve | unchanged; no failure | later owners observe conservative effects | absent |
| aggregate-typed memory boundary/path | exact semantic aggregate type/path, no decomposition | Preserve | unchanged; no failure | B6 owns aggregate normalization | absent |
| target address-mode, ABI placement or selected helper form | later-domain authority appears before C/D stages | Reject | `MemoryForbiddenAuthority` | no B5 output | absent |
| name/text/pointer/legacy-derived object or access | no stable semantic identity/proof | Reject | `MemoryForbiddenIdentity` | no B5 output | absent |
| unfamiliar valid memory form with no declared owner | registry has no closed disposition | Reject | `MemoryUnknownForm` | no B5 output | absent |

Every form has one disposition. An analysis `Unknown`/`Absent` preserves only
valid rows explicitly marked Preserve; it never authorizes guesswork. Every
Reject aborts the complete module occurrence.

## Verification and Publication

The exact private P05 output must satisfy:

- cumulative Raw, P01 `TypesLegal`, P02 `ScalarsCanonical`, P03
  `CfgCanonical`, P04 `SsaCanonical` and P05 memory postconditions;
- every memory/address/atomic/effect form has its unique closed P05 result;
- exact source-semantic objects/types/address spaces/sizes/alignments/paths,
  volatility, ordering, scope and effects are preserved;
- CFG/SSA/phi coverage, stable IDs, def-use, origin composition and opaque
  inline-asm/later-owner forms remain exact;
- no target layout/address mode/ABI/helper/preparation/allocation fact exists;
- a second P05 occurrence proposes no mutation.

Only the pass framework publishes B5 and establishes `MemoryCanonical`.
Analyses remain disposable exact-revision facts; P05 cannot mint a handle,
property, checkpoint or stage token.

## Failure and Diagnostics

Stable failures include `MemoryPassWrongInput`, `MemoryPassStaleInput`,
`MemoryPassP04ContractFailure`, `MemoryPassStaleAnalysis`,
`MemoryPassForbiddenAuthority`, every matrix failure,
`MemoryPassPreservedFormChanged`, `MemoryNonconvergent`, deterministic resource
exhaustion, cancellation, verifier rejection and transaction/publication failure.

Diagnostics carry exact checkpoint/analysis keys and stable module/function/
instruction/value/object/operand/rule anchors. Failure rolls back the entire
module wave and publishes no function prefix, memory/use/effect repair,
revision, property, candidate analysis/cache entry, report or stage token.

## Analysis and Invalidation

Any committed object/address/access/atomic/effect/definition/use/type/call/body
or module-table change invalidates memory effects, provenance,
publication/value-flow, liveness and all transitive dependents observing it.
CFG/SSA topology is preserved, but new-revision installation still requires
registered traits and checked semantic equality. Old B4 handles remain stale.

Candidate-local facts die on rollback and publish only through the manager
under exact B5 keys. P05 cannot patch/retarget results. B6 accepts the verified
B5 graph, not an analysis preservation declaration.

## Target and ABI Rules

P05 preserves source-semantic sizes, alignments and address spaces exactly but
has no target layout/size, pointer-width, address-mode, ABI/helper/preparation,
constraint, home, spill, frame or MIR input. It never selects storage
realization, physical offsets or helper calls.

## Implementation State

Implementation is absent. No checked-in `PassId::MemoryCanonicalize`
implementation/registration, form registry, transaction route, analysis
integration, verifier hook, build target or runtime proof exists. This matrix
is design authority, not implementation coverage.

## Proof Requirements

- verify metadata/core-first order, substantive matrices and all dispositions;
- test exact B4 effects/provenance keys, stale/Unknown forms and stable IDs;
- prove source-semantic attribute preservation and target/ABI/helper exclusion;
- prove deterministic whole-module transaction/rollback, cumulative
  postconditions, idempotence and exact transitive invalidation;
- prove B6 accepts only exact B5 `MemoryCanonical` output;
- reconcile implementation/build truth.

## Open Questions

None for v1. New memory/address/atomic forms require registered ownership.

## Review Checklist

- [x] Exact B4 plus effects/provenance dependency closure required.
- [x] Every memory/address/atomic/effect form has one disposition.
- [x] Stable identities and source-semantic attributes are exact.
- [x] Target/layout/ABI/helper and text/name/pointer guessing are forbidden.
- [x] Atomic rollback, cumulative properties and invalidation are explicit.
- [x] B6 accepts only exact successful B5 output.
- [x] Implementation truth is absent.
