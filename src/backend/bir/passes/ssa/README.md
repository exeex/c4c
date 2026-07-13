# P04 SSA Canonicalization Pass Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: pass
Phase-ID: B4 / P04
Upstream: exact B3 `CfgCanonical` plus exact-current `Cfg`, `Dominance`, `PublicationValueFlow`
Downstream: exact immutable B4 `SsaCanonical` revision consumed by B5 / P05
Owner-Path: `src/backend/bir/passes/ssa/README.md`
Last-Reconciled-Commit: `5eb4d6f43`

## Purpose

P04 is the target-independent B4 function transformation. It consumes one
exact accepted B3 checkpoint and matching CFG/dominance/value-flow facts,
constructs and normalizes explicit-`Phi` SSA, then publishes one verified B4
wave or fails atomically. It never materializes edge copies or changes CFG.

## Owns

- closed `PassId::SsaCanonicalize` / B4 occurrence;
- registered promotability/definition/use dispositions, dominance-frontier phi
  placement, deterministic renaming and exact incoming construction/repair;
- trivial-phi/registered alias elimination, typed atomic RAUW and dead cleanup
  caused only by its rewrites;
- local/cumulative P04 postconditions and `SsaCanonical` property request.

## Does Not Own

- successor topology, terminators, edge splitting, CFG or stable-ID semantics;
- arbitrary phi values, memory promotion outside the registered P04 class,
  memory/effect normalization or later aggregate/intrinsic forms;
- D5 phi materialization/parallel copies, target/helper/ABI/preparation,
  constraints, allocation, MIR or emission;
- revisions, transactions, analysis/cache/property/checkpoint publication.

## Inputs

P04 consumes one immutable B3 checkpoint plus exact-current schema-1 `Cfg`,
`Dominance(Dominators)` and `PublicationValueFlow` handles. Every complete key
matches the same epoch/module/function revision; dominance depends on that CFG
and publication depends on both. P03 postconditions, core def-use and the
configured verifier accept the same revision.

### Exact input and dependency matrix

| Input axis | Exact required state | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| owning checkpoint | exact successful immutable B3 occurrence | declaration/empty function is valid no-op member | wrong/missing capability is `SsaWrongInput` |
| module/function key | exact `ModuleEpoch`, `ModuleRevision`, `FunctionId`, `FunctionRevision` and ordered digest | empty body retains exact keys | stale/foreign/mixed wave is `SsaStaleInput` |
| cumulative properties | `RawVerified` + `TypesLegal` + `ScalarsCanonical` + `CfgCanonical` | none | missing/self-asserted property is `SsaWrongInput` |
| P03 acceptance | complete P03 postcondition/configured verifier and terminator-only CFG authority | no earlier-owner form remains | mismatch is `SsaP03ContractFailure` |
| CFG dependency | freshly recomputed schema-1 `AnalysisId::Cfg` at exact B3 key | empty CFG is complete | missing/stale/pre-planning handle is `SsaStaleAnalysis` |
| dominance dependency | schema-1 `AnalysisId::Dominance(Dominators)` with matching CFG/key | empty dominance is complete | stale/mismatched product/dependency is `SsaStaleAnalysis` |
| value-flow dependency | schema-1 `AnalysisId::PublicationValueFlow` with matching CFG/dominance/key | `Unknown`/`Absent` facts preserve valid form unless a row says reject | stale/mismatched dependency is `SsaStaleAnalysis` |
| semantic graph | stable definitions/uses/types/order, exact `EdgeKey` phi occurrences and registered promotability | phi may be absent; parallel incoming values may differ | malformed def-use/type/edge coverage is inherited verifier failure |
| target/helper exclusion | no target/profile/layout/helper/ABI/preparation/allocation/MIR fact | none | later-domain fact is `SsaForbiddenAuthority` |

No earlier-revision cache, predecessor-block set, name/position/dense index,
rendered-text probe, block-argument compatibility map or legacy publication
record may substitute for an exact input.

## Outputs

The pipeline publishes the complete P04 function wave as one immutable B4
checkpoint. It derives exact revisions and one authoritative `MutationSummary`,
records B4, retains cumulative properties through `CfgCanonical`, and
establishes `SsaCanonical` only after all functions and postconditions succeed.

### Exact output handoff matrix

| Output/product | Exact consumer | Required binding and acceptance | Failure / forbidden escape |
|---|---|---|---|
| immutable B4 checkpoint | B5/P05 memory | exact epoch/module/function revisions, ordered digest and B4 stamp | no function prefix, editor, B3 alias or mixed wave substitutes |
| cumulative properties | B5 precondition and later B8 verifier | Raw + P01 + P02 + P03 retained; framework establishes `SsaCanonical` after proof | P04 cannot mint property/report/capability |
| canonical definitions/uses | B5 and exact-revision semantic analyses | one registered definition class, dominance of ordinary uses and complete def-use | no name/position/legacy/dense identity escapes |
| canonical explicit phis | B5 and later D5 | exact incoming `EdgeKey -> ValueId` multiset equals reachable incoming occurrences, including parallels | no block argument, invented value or predecessor-block collapse |
| occurrence `MutationSummary` | pass/analysis frameworks | exact definition/use/phi/order/type effects | declarations cannot override effects or preserve stale handles |
| structured diagnostics | pipeline observation only | stable rule/entity/edge anchor and deterministic order | cannot authorize partial repair, fallback SSA or B5 |

B5 accepts only this exact immutable B4 wave with `SsaCanonical`, complete
def-use and exact `EdgeKey` phi coverage. It may request freshly recomputed or
checked-preserved analyses under the B4 key; no B3 handle crosses revision.

## Adjacent-Stage Contract

[P03 CFG](../cfg/README.md) supplies the exact B3 checkpoint and fresh CFG.
[Dominance](../../analysis/dominance/README.md) and
[publication/value-flow](../../analysis/publication/README.md) supply matching
planning facts. P04 alone owns the closed SSA rewrites below. Core owns IDs,
def-use and edge keys; frameworks own transactions/results/publication.

[B5/P05 memory](../memory/README.md) accepts the exact successful B4 output.
P04 preserves terminator topology and non-SSA semantics; B5 cannot repair an
incomplete P04 wave or accept an analysis report as `SsaCanonical`.

## Ordered Behavior

1. Validate exact B3 checkpoint/P03 postconditions and mutually matching CFG,
   dominance and value-flow complete keys.
2. Inventory every definition, use, promotable class, frontier placement,
   existing phi/incoming and alias candidate in canonical stable-ID/edge order.
   Assign exactly one closed matrix disposition.
3. Prove registered placement/renaming/incoming values from typed semantics,
   exact dominance and `Known` value-flow facts. Preserve valid unproved forms
   only where the matrix assigns Preserve; accumulate all rejects before edits.
4. Fork one private transaction per function in the wave. Reserve deterministic
   IDs and build one closed plan of inserted definitions/phis, exact incoming
   keys/values, replaced uses, origin composition and deletions.
5. Apply typed atomic RAUW, complete def-use repair and exact phi occurrence
   updates without changing terminators/slots. Derive function/aggregate
   `MutationSummary` from actual edits.
6. Run local and cumulative Raw+P01+P02+P03+P04 postconditions, recompute
   candidate-local dominance/value-flow as needed, and verifier-on-commit.
7. If every function succeeds, atomically publish B4 and let the framework
   establish `SsaCanonical`; otherwise discard all candidates and retain B3.

## Invariants

- One definition dominates every ordinary use. Phi incoming values are checked
  on their exact incoming edge, not at destination entry.
- Canonical v1 uses explicit `Phi` only; no block-argument alternative exists.
- Each reachable incoming `EdgeKey {source, role, index}` appears exactly once
  per phi. Parallel occurrences remain distinct and may carry different values.
- A new/remapped incoming receives a value only through a registered SSA rule;
  names, position, predecessor-block equality and arbitrary duplication cannot
  invent it.
- P04 never changes terminators, blocks or successor slots. CFG remains
  canonical; D5 later owns edge-copy placement and phi materialization.
- Stable IDs, types, poison/undef semantics, atomic RAUW, def-use and origins
  remain exact. No target/later-domain fact enters.

## Exhaustive P04 SSA-Form Disposition Matrix

| Applicable B3 input form | Lossless facts / precondition | Disposition | Exact P04 result or stable failure | Next owner / B5 visibility | Implementation |
|---|---|---|---|---|---|
| declaration or valid empty function | complete empty analyses and no body | Preserve | unchanged; no failure | B5 sees empty exact function | absent |
| canonical ordinary SSA definition/use | one typed definition dominates every use | Preserve | unchanged; no failure | B5 sees canonical def-use | absent |
| function parameter entry definition | typed stable parameter/result identity | Preserve | unchanged; no failure | B5 sees entry value | absent |
| call or inline-asm ordinary result definition | registered typed result roles and complete uses | Preserve | unchanged; no failure | later owners retain semantic node | absent |
| later-owner memory/effect definition class | registry marks ineligible for P04 promotion | Preserve | unchanged; no failure | B5 owns memory normalization | absent |
| admitted promotable multi-definition variable | registered class, exact sites, frontier and incoming values | Normalize | insert deterministic phis and rename all uses; else `SsaConstructionFailed` | B5 sees one SSA version per definition | absent |
| admitted promotable local/stack semantic variable | registered non-escaping promotion rule and complete effects | Normalize | construct SSA and remove only owned storage traffic; else `SsaPromotionFailed` | B5 sees promoted values | absent |
| ordinary use not dominated but repairable by registered renaming | exact reaching definition proven for every path | Normalize | rename to exact SSA value; else `SsaRenameFailed` | B5 sees dominated use | absent |
| ordinary use with no definition | no registered source value exists | Reject | `SsaUndefinedUse` | no B4 output | absent |
| non-dominating use of ineligible/non-promotable value | no P04 rule may create replacement | Reject | `SsaNonDominatingUse` | no B4 output | absent |
| missing phi at required dominance frontier | registered promotable definition set and exact frontier | Normalize | insert deterministic typed phi; else `SsaPhiPlacementFailed` | B5 sees required phi | absent |
| already canonical explicit phi | exact type/result and one incoming per reachable edge occurrence | Preserve | unchanged; no failure | B5 sees canonical phi | absent |
| block-argument SSA alternative | canonical v1 admits explicit `Phi` only | Reject | `SsaBlockArgumentForbidden` | no B4 output | absent |
| missing phi incoming with registered reaching value | exact edge occurrence and edge-dominating value proved | Normalize | add exact `EdgeKey -> ValueId`; else `SsaIncomingValueMissing` | B5 sees complete coverage | absent |
| missing phi incoming without provable value | value-flow is `Unknown`/`Absent` and no construction rule applies | Reject | `SsaIncomingValueMissing` | no B4 output | absent |
| extra phi incoming for dead/nonexistent edge | exact-current CFG proves key absent | Normalize | delete exact incoming occurrence; else `SsaPhiCoverageInvalid` | B5 sees exact live multiset | absent |
| stale/remapped phi incoming key with registered P03 mapping | exact old/new occurrence relation and same incoming semantics | Normalize | replace with exact current key; else `SsaPhiCoverageInvalid` | B5 sees current key | absent |
| foreign or malformed phi incoming key | source/role/index not valid in exact CFG | Reject | `SsaForeignEdgeKey` | no B4 output | absent |
| duplicate incoming entries for one exact edge key | canonical multiset requires exactly one | Reject | `SsaDuplicateIncoming` | no B4 output | absent |
| parallel incoming edge occurrences | distinct exact keys, each with typed value | Preserve | retain all occurrences; no failure | B5 sees exact multiplicity | absent |
| noncanonical phi incoming storage order | exact key/value pairs complete | Normalize | sort by canonical edge order; else `SsaPhiOrderInvalid` | B5 sees deterministic order | absent |
| phi result/incoming type mismatch | no lossless registered conversion in P04 | Reject | `SsaPhiTypeMismatch` | no B4 output | absent |
| trivial phi with one proven semantic replacement | all live occurrences equal under poison/undef rules | Normalize | typed atomic RAUW then delete phi; else `SsaTrivialProofFailed` | B5 sees replacement value | absent |
| apparent trivial phi with unproved poison/undef equality | value-flow cannot prove one semantic replacement | Preserve | retain phi; no failure | B5 sees explicit uncertainty-preserving phi | absent |
| nontrivial or cyclic phi component | valid exact coverage and no registered elimination proof | Preserve | retain component; no failure | later D5 consumes exact phis | absent |
| registered transparent alias carrier | one exact replacement proven by value-flow/dominance | Normalize | atomic RAUW and delete owned carrier; else `SsaAliasProofFailed` | B5 sees canonical definition | absent |
| dead definition created by P04 rewrite | zero exact uses after complete repair and safe owned deletion | Normalize | delete owned definition; else `SsaDeadCleanupFailed` | B5 sees no P04 remnant | absent |
| unrelated pre-existing dead semantic node | not caused/owned by P04 | Preserve | unchanged; no failure | declared later owner retains it | absent |
| exact P03 split/merge phi remnants already coherent | current edge coverage and def-use agree | Preserve | unchanged; no failure | B5 sees exact current graph/SSA | absent |
| rewrite that would require successor topology change | P04 has no CFG authority | Reject | `SsaCfgMutationRequired` | no B4 output | absent |
| name/position/dense/legacy-derived identity or incoming value | no stable semantic proof exists | Reject | `SsaForbiddenIdentity` | no B4 output | absent |
| unfamiliar definition/use/phi form with no declared owner | registry has no closed disposition | Reject | `SsaUnknownForm` | no B4 output | absent |

Every applicable form has one disposition. `Unknown`/`Absent` analysis facts
preserve only the rows explicitly marked Preserve; they never permit invented
incoming values or speculative RAUW. Every Reject aborts the whole occurrence.

## Verification and Publication

The exact private P04 output must satisfy:

- cumulative Raw, P01 `TypesLegal`, P02 `ScalarsCanonical`, P03
  `CfgCanonical` and P04 SSA postconditions on one revision;
- every ordinary use has one typed dominating definition and complete def-use;
- every phi has exact reachable incoming `EdgeKey` multiplicity/order/type and
  each incoming value dominates its particular edge;
- no block argument, unresolved alias, stale edge key, duplicate incoming,
  invented value or P04-owned promotable non-SSA form remains;
- terminator topology and all preserved non-SSA/opaque semantics are exact;
- stable IDs/order/origins and target/later-domain exclusion hold;
- a second P04 occurrence proposes no mutation.

Only the pass framework publishes the complete B4 wave and establishes
`SsaCanonical`. P04 cannot mint a property, analysis result, checkpoint or
stage token.

## Failure and Diagnostics

Stable failures include `SsaWrongInput`, `SsaStaleInput`,
`SsaP03ContractFailure`, `SsaStaleAnalysis`, `SsaForbiddenAuthority`, every
matrix failure, `SsaNonconvergent`, `SsaPreservedFormChanged`, deterministic
resource exhaustion, cancellation, verifier rejection and transaction/
occurrence publication failure.

Diagnostics carry exact checkpoint/dependency keys and stable function/block/
instruction/value/edge/rule anchors in deterministic order. Failure rolls back
the entire wave and publishes no phi/definition/use repair, function prefix,
revision, property, candidate analysis/cache entry, report or stage token.

## Analysis and Invalidation

P04 mutations to definitions, results, uses, operands, phi incoming, types or
instruction order invalidate publication/value-flow, dominance instruction/
value queries, liveness, memory effects, provenance and all transitive
dependents. Terminator topology is unchanged, but CFG preservation at a new
revision still requires registered mutation traits and checked installation
under the B4 key. Old B3 handles remain stale after revision increment.

Candidate-local recomputation verifies dominance/def-use/phi coverage, then
dies on rollback or is installed only by the manager after publication rules.
P04 cannot patch handles. B5 requests exact-B4 facts as needed; no failed or
stale analysis can justify its handoff.

## Target and ABI Rules

P04 has no target profile/layout, helper registry decision, ABI/preparation
product, constraint binding, register/stack home, frame or MIR input. It does
not schedule phi copies or select physical transfer locations. Output remains
target-independent and unallocated.

## Implementation State

Implementation is absent. No checked-in `PassId::SsaCanonicalize`
implementation/registration, SSA rule registry, transaction route, analysis
integration, verifier hook, build target or runtime proof exists. The matrix is
design authority, not implementation coverage.

## Proof Requirements

- verify metadata/core-first order, substantive input/output matrices and all
  closed form dispositions;
- test mutually exact analysis dependency keys, stale rejection, stable-ID
  definitions/uses and parallel-edge phi availability/dominance;
- prove no invented incoming, name/position/dense identity or topology change;
- prove deterministic IDs/order, atomic full-wave rollback, cumulative
  postconditions, idempotence and transitive invalidation;
- prove B5 accepts only the exact `SsaCanonical` B4 wave;
- reconcile implementation/build truth.

## Open Questions

None for v1. New promotability or SSA representation families require a
registered owner/schema decision.

## Review Checklist

- [x] Exact B3 plus CFG/dominance/publication keys are mutually current.
- [x] Definitions, uses, phis and SSA forms have closed dispositions.
- [x] Parallel `EdgeKey` coverage and edge-dominating values are exact.
- [x] No invented value or name/position/dense/legacy identity is admitted.
- [x] Atomic rollback, cumulative properties and invalidation are explicit.
- [x] B5 accepts only the exact successful B4 wave.
- [x] Target/later-domain inputs are excluded.
- [x] Implementation truth is absent.
