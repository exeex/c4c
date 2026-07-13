# P06 Aggregate Canonicalization Pass Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: pass
Phase-ID: B6 / P06
Upstream: exact immutable B5 / P05 `MemoryCanonical` module wave
Downstream: exact immutable B6 `AggregatesCanonical` module wave consumed by B7 / P07
Owner-Path: `src/backend/bir/passes/aggregate/README.md`
Last-Reconciled-Commit: `d4c73bdf4`

## Purpose

P06 is the target-independent B6 module transformation. It consumes the exact
accepted B5 wave, normalizes aggregate values, copies, projections, semantic
vectors and by-value boundaries without physical decomposition, then publishes
one verified B6 wave or fails atomically.

## Owns

- closed `PassId::AggregateCanonicalize` / B6 module occurrence;
- registered array/record/union/complex/multivalue/vector value and type-
  preserving descriptor normalization;
- aggregate construction, copy, insert/extract/projection, layout-independent
  field/index paths and semantic by-value call/return boundary normalization;
- typed use repair caused directly by its replacements;
- P06 local/cumulative postconditions and `AggregatesCanonical` property request.

## Does Not Own

- P01-P05 type/scalar/CFG/SSA/memory forms or P07 intrinsic identity;
- physical field/lane layout, aggregate scalarization, storage decomposition,
  stack copies, sret/register classification or target address arithmetic;
- helper selection, ABI placement, preparation, constraints, allocation, MIR,
  renderer or emission;
- inference from names, source spelling, pointer identity, vector position,
  rendered text or legacy records;
- transaction, revision, cache, property, checkpoint or stage publication.

## Inputs

P06 consumes exactly one immutable whole-module B5 checkpoint. P05
postconditions, exact CFG/SSA/phi and memory profiles, core def-use and the
configured verifier accept the same module/function revisions. Resolved
semantic aggregate/vector types, stable IDs, source-semantic attributes and
typed declaration/definition/call boundaries are authoritative.

### Exact input acceptance matrix

| Input axis | Exact required state | Optional/empty form | Failure / forbidden substitution |
|---|---|---|---|
| owning checkpoint | exact successful immutable B5 module occurrence | empty module remains valid | wrong/missing capability is `AggregateWrongInput` |
| module/function key | exact `ModuleEpoch`, `ModuleRevision`, ordered `(FunctionId, FunctionRevision)` digest, B5 stamp and plan fingerprint | empty module has empty exact digest | stale/foreign/mixed wave is `AggregateStaleInput` |
| cumulative properties | `RawVerified` + `TypesLegal` + `ScalarsCanonical` + `CfgCanonical` + `SsaCanonical` + `MemoryCanonical` | none | missing/self-asserted property is `AggregateWrongInput` |
| P05 acceptance | complete P05 postconditions, exact CFG/SSA/memory and configured verifier | no earlier-owner form may remain | mismatch is `AggregateP05ContractFailure` |
| semantic type registry | resolved stable aggregate/vector type IDs, record/field identities, element types/count semantics and declaration consistency | opaque declaration may remain only where no inspection/projection occurs | unknown/inconsistent required type is `AggregateTypeInvalid` |
| value/use graph | typed stable values, constructors, copies, projections, calls/returns, exact def-use and origins | declarations/zero-member aggregates are valid | malformed type/use/result is inherited verifier failure |
| source-semantic attributes | exact field/index identity, address space, sizes/alignments and by-value language semantics already represented in BIR | unspecified source attribute remains absent | target default or ABI reinterpretation is `AggregateForbiddenAuthority` |
| opaque/later families | exact P05 GEP/address descriptors, inline-asm payload and intrinsic semantic nodes | aggregate-typed ordinary edge is valid | changing opaque/later form is `AggregatePreservedFormChanged` |
| target/helper exclusion | no target layout/lane realization, ABI/helper/preparation/allocation/MIR input | none | later-domain fact is `AggregateForbiddenAuthority` |

P06 has no name/text/pointer/position lookup, physical offset calculator,
calling-convention table, target feature query or compatibility side map.

## Outputs

The pipeline publishes the complete P06 module wave as one immutable B6
checkpoint. It derives exact module/function revisions and one authoritative
`MutationSummary`, records B6, retains all P01-P05 properties, and establishes
`AggregatesCanonical` only after the complete transaction and verifier succeed.

### Exact output handoff matrix

| Output/product | Exact consumer | Required binding and acceptance | Failure / forbidden escape |
|---|---|---|---|
| immutable B6 module wave | B7/P07 intrinsics | exact epoch/module/function digest, B6 stamp and no B5 alias | no function prefix/editor/mixed wave substitutes |
| cumulative properties | B7 precondition and later B8 verifier | Raw + P01 + P02 + P03 + P04 + P05 retained; framework establishes `AggregatesCanonical` | P06 cannot mint property/report/capability |
| canonical aggregate/value graph | B7 and exact-revision analyses | unique target-independent aggregate/vector values, copies and projections | no physical decomposition, earlier-form recreation or side table |
| canonical by-value boundaries | B7 and later ABI preparation | semantic parameter/result/type identity only | no sret/register/stack placement or helper choice |
| preserved semantic attributes | B7 and later target stages | exact stable types/fields/indices/paths/address spaces/source sizes/alignments | no target offset/lane/layout reinterpretation |
| occurrence `MutationSummary` | pass/analysis frameworks | exact type/value/use/call/declaration/module effects | declarations cannot override observed changes |
| structured diagnostics | pipeline observation only | stable rule/type/value/path/call anchor and deterministic order | cannot authorize partial wave, fallback or B7 |

B7 accepts only this exact immutable B6 `AggregatesCanonical` wave with every
P01-P06 postcondition current. A stale analysis, similar reconstruction,
partial prefix or diagnostic report cannot substitute.

## Adjacent-Stage Contract

[P05 memory](../memory/README.md) supplies the exact `MemoryCanonical` wave and
layout-independent semantic GEP/address paths. P06 alone owns the registered
aggregate rewrites below; core owns stable types/IDs/def-use and the pass
framework owns transactions/properties/publication.

[B7/P07 intrinsics](../intrinsics/README.md) consumes the exact B6 output. P06
preserves registered intrinsic identity and opaque `InlineAsm`, and cannot
recreate an earlier noncanonical form for P07 to repair.

## Ordered Behavior

1. Validate exact B5 module/function keys/properties/P05 postconditions and
   resolved semantic type/declaration consistency.
2. Inventory every aggregate/vector type, value, copy, projection, path and
   by-value boundary in canonical stable-ID order. Assign exactly one closed
   matrix disposition.
3. Prove every normalization from typed semantic identity only. Preserve
   declared opaque/later forms and accumulate deterministic rejects before edit.
4. Fork one private whole-module occurrence transaction, reserve deterministic
   IDs and build the complete cross-function/declaration replacement/use plan.
5. Apply typed atomic RAUW and complete def-use/origin repair without changing
   CFG, phi coverage or memory semantics. Derive authoritative mutation effects
   from actual edits.
6. Run cumulative Raw+P01+P02+P03+P04+P05+P06 postconditions, exact
   CFG/SSA/memory preservation and verifier-on-commit over the entire candidate.
7. Atomically publish B6 and establish `AggregatesCanonical` only on complete
   success; otherwise discard the candidate and retain B5/cache unchanged.

## Invariants

- Stable type/value/field identities and typed field/index paths are semantic;
  physical offsets, lane storage, names, spelling and vector position are not.
- Source-semantic type, size, alignment and address-space attributes remain
  exact. P06 never computes target layout or calling-convention placement.
- Aggregate/vector normalization does not scalarize, select helpers, create
  stack copies or introduce a P01-P05 noncanonical scalar/memory operation.
- CFG terminators/edges, SSA definitions/uses/phi multiplicity and P05 memory/
  GEP/address semantics remain exact.
- Union/complex/multivalue/vector semantics preserve poison/undef, inactive/
  unspecified state, ordering and exceptional behavior.
- `InlineAsm` payload/constraints remain opaque; aggregate/vector typed edges
  are ordinary typed def-use connections.

## Exhaustive P06 Aggregate-Form Disposition Matrix

| Applicable B5 input form | Lossless facts / precondition | Disposition | Exact P06 result or stable failure | Next owner / B7 visibility | Implementation |
|---|---|---|---|---|---|
| empty module, declaration or empty function | exact empty member and type state | Preserve | unchanged; no failure | B7 sees exact empty member | absent |
| canonical array type/value | stable element type/count and value identity | Preserve | unchanged; no failure | B7 sees canonical array | absent |
| canonical record/struct type/value | stable record and ordered field IDs/types | Preserve | unchanged; no failure | B7 sees canonical record | absent |
| canonical union type/value | stable union/member identity and semantic active/unknown state | Preserve | unchanged; no failure | B7 sees canonical union | absent |
| canonical complex value | exact component type/order and exceptional semantics | Preserve | unchanged; no failure | B7 sees canonical complex | absent |
| canonical multivalue/tuple value | exact ordered semantic result roles/types | Preserve | unchanged; no failure | B7 sees canonical multivalue | absent |
| canonical fixed semantic vector value | exact element type and source-semantic lane count | Preserve | unchanged; no failure | B7 sees semantic vector | absent |
| canonical scalable semantic vector value | exact element type and scalable count expression | Preserve | unchanged; no failure and no physical lane count selected | later target stages retain realization | absent |
| admitted aggregate/vector type alias | exact stable underlying semantic type | Normalize | canonical type reference; else `AggregateTypeAliasUnproved` | B7 sees unique type identity | absent |
| opaque aggregate declaration with no inspection | stable opaque type/declaration identity | Preserve | unchanged; no failure | later declaration resolution remains explicit | absent |
| projection/copy requiring opaque aggregate inspection | layout/field/member semantics unavailable | Reject | `AggregateOpaqueInspection` | no B6 output | absent |
| canonical aggregate constructor/literal | exact type and complete ordered semantic members | Preserve | unchanged; no failure | B7 sees canonical construction | absent |
| admitted constructor/literal alias | exact member roles/types and poison/undef semantics | Normalize | canonical constructor; else `AggregateConstructorUnproved` | B7 sees canonical construction | absent |
| constructor with missing/extra/type-invalid member | no complete lossless semantic value | Reject | `AggregateConstructorInvalid` | no B6 output | absent |
| zero-member/zero-length aggregate value | resolved semantic type and exact empty members | Preserve | unchanged; no failure | B7 sees explicit empty aggregate | absent |
| canonical typed aggregate copy | identical semantic source/result types and copy semantics | Preserve | unchanged; no failure | B7 sees canonical copy | absent |
| admitted aggregate copy alias | exact typed copy, overlap and effect semantics | Normalize | canonical aggregate copy descriptor; else `AggregateCopyUnproved` | B7 sees canonical copy | absent |
| incompatible aggregate copy | source/result semantic types cannot match losslessly | Reject | `AggregateCopyTypeMismatch` | no B6 output | absent |
| canonical insert by stable field/index path | exact base/result type, path and inserted value type | Preserve | unchanged; no failure | B7 sees canonical insert | absent |
| admitted insert alias | exact semantic path/value/base/result identities | Normalize | canonical insert; else `AggregateInsertUnproved` | B7 sees canonical insert | absent |
| canonical extract by stable field/index path | exact source/result type and path identity | Preserve | unchanged; no failure | B7 sees canonical extract | absent |
| admitted extract/projection alias | exact semantic path/source/result identities | Normalize | canonical extract; else `AggregateExtractUnproved` | B7 sees canonical projection | absent |
| nested typed path in noncanonical segmented form | exact ordered field/index components | Normalize | canonical single layout-independent path; else `AggregatePathUnproved` | B7 sees canonical path | absent |
| out-of-range or type-incompatible path | resolved semantic type rejects component | Reject | `AggregatePathInvalid` | no B6 output | absent |
| spelling/name/physical-offset-derived field path | no stable semantic field/index identity | Reject | `AggregateForbiddenIdentity` | no B6 output | absent |
| dynamic semantic array/vector index operation | exact typed index value and bounds semantics | Preserve | unchanged; no failure and no target bounds/layout inference | B7 sees semantic dynamic projection | absent |
| canonical complex construct/project operation | exact real/imaginary role and typed semantics | Preserve | unchanged; no failure | B7 sees semantic complex operation | absent |
| admitted complex operation alias | exact component roles/types/exception behavior | Normalize | canonical complex operation; else `AggregateComplexUnproved` | B7 sees canonical complex operation | absent |
| canonical multivalue construct/project operation | exact stable result roles/types | Preserve | unchanged; no failure | B7 sees semantic multivalue operation | absent |
| admitted multivalue alias | exact role/type mapping | Normalize | canonical multivalue operation; else `AggregateMultivalueUnproved` | B7 sees canonical multivalue operation | absent |
| canonical vector insert/extract/splat/shuffle | exact semantic lane selectors/mask/type and poison/undef rules | Preserve | unchanged; no failure | B7 sees semantic vector operation | absent |
| admitted vector operation alias | exact semantic selector/mask/type mapping independent of target lanes | Normalize | canonical vector operation; else `AggregateVectorUnproved` | B7 sees canonical vector operation | absent |
| vector operation requiring target lane realization | no target-independent semantic result is registered | Reject | `AggregateTargetLaneRequired` | no B6 output | absent |
| canonical semantic by-value parameter/result boundary | exact type, parameter/result role and language semantic attribute | Preserve | unchanged; no failure and no ABI placement implied | B7 sees by-value semantics | absent |
| admitted hidden-result/by-value source alias | exact semantic boundary without physical placement | Normalize | canonical by-value parameter/result descriptor; else `AggregateBoundaryUnproved` | B7 sees semantic boundary only | absent |
| declaration/definition/call boundary mismatch | cross-module types/roles cannot reconcile losslessly | Reject | `AggregateBoundaryMismatch` | no B6 output | absent |
| P05 canonical aggregate GEP/address path | exact memory path/type/object/address-space semantics | Preserve | unchanged; no failure | B7 retains canonical memory form | absent |
| aggregate/vector typed phi/select/call edge | already canonical CFG/SSA/scalar/intrinsic ordinary typed edge | Preserve | unchanged; no failure | owning earlier/later form remains exact | absent |
| aggregate/vector typed `InlineAsm` edge | ordinary typed edge with opaque payload/constraints | Preserve | unchanged; no failure | P07 preserves opaque asm | absent |
| helper-eligible aggregate semantic operation | exact portable semantic identity only | Preserve | unchanged; no failure and no helper selected | later helper planning retains identity | absent |
| physical decomposition, target layout or ABI placement form | later-domain authority appears before C/D | Reject | `AggregateForbiddenAuthority` | no B6 output | absent |
| rewrite requiring fresh P01-P05 noncanonical form | canonical stage cannot recreate earlier work | Reject | `AggregateEarlierFormRequired` | no B6 output | absent |
| unfamiliar valid aggregate/vector form with no declared owner | registry has no closed disposition | Reject | `AggregateUnknownForm` | no B6 output | absent |

Every applicable form has exactly one disposition. Preserve is valid only for
the complete semantic form stated in its row; it is not silent delegation of a
P06-owned noncanonical form. Every Reject aborts the complete occurrence.

## Verification and Publication

The exact private P06 output must satisfy:

- cumulative Raw, P01 `TypesLegal`, P02 `ScalarsCanonical`, P03
  `CfgCanonical`, P04 `SsaCanonical`, P05 `MemoryCanonical` and P06 aggregate
  postconditions on one module/function wave;
- every aggregate/vector value, copy, projection, path and by-value boundary
  has its unique closed P06 result;
- stable types/IDs/fields/indices, source-semantic attributes, typed def-use and
  origins remain exact;
- CFG/SSA/phi/memory/GEP forms and opaque inline-asm/intrinsics remain exact;
- no target layout/lane/physical decomposition, helper, ABI/preparation or
  allocation fact exists;
- a second P06 occurrence proposes no mutation.

Only the pass framework publishes B6 and establishes `AggregatesCanonical`.
P06 cannot mint a property, analysis result, checkpoint or stage token.

## Failure and Diagnostics

Stable failures include `AggregateWrongInput`, `AggregateStaleInput`,
`AggregateP05ContractFailure`, `AggregateForbiddenAuthority`, every matrix
failure, `AggregatePreservedFormChanged`, `AggregateNonconvergent`,
deterministic resource exhaustion, cancellation, verifier rejection and
transaction/occurrence publication failure.

Diagnostics carry exact checkpoint keys and stable module/function/type/value/
path/field/index/call/rule anchors in deterministic order. Failure rolls back
the complete module wave and publishes no function prefix, aggregate/use
repair, revision, property, candidate analysis/cache entry, report or token.

## Analysis and Invalidation

Any committed type, value, result, use, aggregate path, call boundary,
declaration or module-table change invalidates publication/value-flow, memory
effects, provenance, call graph, liveness and all transitive dependents that
observe it. CFG/SSA/memory semantics are preserved, but installation under a
new B6 key still requires registered traits and complete semantic equality.
Old B5 handles remain stale after revision increment.

Candidate-local facts die on rollback. P06 cannot patch or retarget analyses;
the manager alone may install exact-B6 results after commit rules succeed.

## Target and ABI Rules

P06 preserves source-semantic types/sizes/alignments/address spaces and
by-value meaning but has no target layout/lane count realization, physical
offset/decomposition, ABI/helper/preparation, constraint, home, spill, frame
or MIR input. It never creates sret, register classes or stack-copy sequences.

## Implementation State

Implementation is absent. No checked-in `PassId::AggregateCanonicalize`
implementation/registration, form registry, module transaction route, verifier
hook, analysis integration, build target or runtime proof exists. The matrix is
design authority, not implementation coverage.

## Proof Requirements

- verify metadata/core-first order and substantive input/output matrices;
- cover every aggregate/value/copy/projection/vector/by-value row and exact
  Normalize/Preserve/Reject result;
- prove stable IDs/types/paths/source attributes and exact CFG/SSA/memory
  preservation without physical/target/ABI/helper authority;
- prove atomic whole-module rollback, cumulative P01-P06 postconditions,
  idempotence and mutation-derived transitive invalidation;
- prove B7 accepts only exact B6 `AggregatesCanonical` output;
- reconcile implementation/build truth.

## Open Questions

None for v1. New aggregate/vector families require registered ownership.

## Review Checklist

- [x] Exact B5 `MemoryCanonical` input and B7 handoff are explicit.
- [x] Every aggregate/vector/by-value form has one closed disposition.
- [x] Stable identities, semantic paths and source attributes remain exact.
- [x] CFG/SSA/memory and opaque later-owner forms are preserved.
- [x] Target layout/decomposition/helper/ABI placement are excluded.
- [x] Atomic rollback, cumulative postconditions and invalidation are explicit.
- [x] Implementation truth is absent.
